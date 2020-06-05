import sys
import array
from ctypes import c_uint, c_int, byref, sizeof, memset, cast, POINTER

# Following found in PYTHONPATH setup by XRT
from xrt_binding import *
from ert_binding import * 

#sys.path.append('/opt/xilinx/xrt/test/')
from utils_binding import *

# Get DRM library
sys.path.insert(0, '/opt/accelize/drmlib_jbl/build/python3_bdist')
import accelize_drm

INCR_VALUE = 10
DATA_SIZE = 4096

CONTROL_ADDR_AP_CTRL = 0x00
CONTROL_ADDR_GIE = 0x04
CONTROL_ADDR_IER = 0x08
CONTROL_ADDR_ISR = 0x0c
CONTROL_ADDR_BUFFER_ADDR = 0x10
CONTROL_ADDR_DATA_SIZE = 0x1C

ADDER_CONTROL_ADDR_INC_SIZE = 0x10
ADDER_CONTROL_ADDR_DATA_SIZE = 0x14


# Global Read Register Function
def read_register(cuidx, addr, value, shared):
    xclOpenContext(shared.handle, shared.xuuid, cuidx, False)
    try:
        return xclRegRead(shared.handle, cuidx, addr, value)        
    finally:
        xclCloseContext(shared.handle, shared.xuuid, cuidx)    
    
# Global Write Register Function
def write_register(cuidx, addr, value, shared):
    xclOpenContext(shared.handle, shared.xuuid, cuidx, False)
    try:
        return xclRegWrite(shared.handle, cuidx, addr, value)        
    finally:
        xclCloseContext(shared.handle, shared.xuuid, cuidx)
    
def py_read_register(cuidx, addr, shared):
    reg = c_uint(0)
    if read_register(cuidx, addr, byref(reg), shared):
        raise RuntimeError("[ERROR] Failed to read CU ID %d @0x%X" % (cuidx, addr))        
    return reg.value

def py_write_register(cuidx, addr, value, shared):
    reg = c_uint(value)
    if write_register(cuidx, addr, reg, shared):
        raise RuntimeError("[ERROR] Failed to write CU ID %d @0x%X" % (cuidx, addr))    


def main(args):
    opt = Options()
    Options.getOptions(opt, args)
    try:
        initXRT(opt)
        assert (opt.first_mem >= 0), "Incorrect memory configuration"

        drm_cuidx = 0
        add_cuidx = 1
        
        # Load page 0 of DRM Ctrl
        py_write_register(drm_cuidx, 0, 0, opt)
        
        # Get DRM Ctrl version
        reg = py_read_register(drm_cuidx, 0x70, opt)
        print("DRM Controller version: %X" % reg)

        # Check DRM Activator existance
        reg = py_read_register(add_cuidx, 0x40, opt)
        print("DRM Activator existance: 0x%X" % reg)
        if reg != 0x600DC0DE:
            print("Error: DRM Activator existance should be 0x600DC0DE, not %X" % reg);
            return -1

        # Check DRM Activator status
        reg = py_read_register(add_cuidx, 0x38, opt);
        if reg != 0:
            print("Error: DRM Activator status should be 0, not", reg);
            return -1

        print("DRM Activator is locked");

#ACCELIZE DRMLIB ACTIVATION CODE AREA START        
        try:
            drm_manager = accelize_drm.DrmManager(
               'conf.json', 
               'cred.json',
               lambda addr, value: read_register(drm_cuidx, addr, value, opt),
               lambda addr, value: write_register(drm_cuidx, addr, value, opt)
            )
            print('[DRMLIB] Allocated')

            drm_manager.activate()
            print("[DRMLIB] Design unlocked")        
#ACCELIZE DRMLIB ACTIVATION CODE AREA STOP

            # Check a DRM Activator is detected
            reg = py_read_register(add_cuidx, 0x40, opt);
            if reg != 0x600DC0DE:
                print("Error: No DRM Activator is detected")
                return -1
            print("[DRMLIB] a DRM Activator is detected")

            # Check DRM Activator status
            reg = py_read_register(add_cuidx, 0x38, opt)
            if reg != 3:
                print("Error: DRM Activator status should be 3, not", reg)
                return -1
            '''
            # Perform some processing with the kernels
            xclOpenContext(opt.handle, opt.xuuid, 0, False)
            
            try:
                inputHandle = -1
                outputHandle = -1
                execHandle = -1
                C_DATA_SIZE = sizeof(c_int) * DATA_SIZE
                                
                inputHandle = xclAllocBO(opt.handle, C_DATA_SIZE, 0, opt.first_mem)
                if inputHandle == -1:
                    raise RuntimeError("Error: Unabe to alloc Input BO")
                    
                outputHandle = xclAllocBO(opt.handle, C_DATA_SIZE, 0, opt.first_mem)
                if outputHandle == -1:
                    raise RuntimeError("Error: Unabe to allocOutnput BO")
                    
                input_bo = xclMapBO(opt.handle, inputHandle, True)
                if not input_bo: # checks NULLBO
                    raise RuntimeError("Error: Unable to map Input BO")
                    
                output_bo = xclMapBO(opt.handle, outputHandle, True)
                if not output_bo: # checks NULLBO
                    raise RuntimeError("Error: Unable to map Output BO")
                    
                memset(input_bo, 0, DATA_SIZE)
                memset(output_bo, 0, DATA_SIZE)
                
                source_input = cast(input_bo, POINTER(c_int))
                source_hw_results = cast(input_bo, POINTER(c_int))
                source_sw_results = array.array('L', [0]*DATA_SIZE)
                
                for i in range(DATA_SIZE):
                    source_input[i] = i
                    source_sw_results[i] = i + INCR_VALUE
                    source_sw_results[i] = 0
                    
                if xclSyncBO(opt.handle, inputHandle, xclBOSyncDirection.XCL_BO_SYNC_BO_TO_DEVICE, C_DATA_SIZE, 0):
                    raise RuntimeError("Error: Unable to sync Input BO")

                if xclSyncBO(opt.handle, outputHandle, xclBOSyncDirection.XCL_BO_SYNC_BO_TO_DEVICE, C_DATA_SIZE, 0):
                    raise RuntimeError("Error: Unable to sync Output BO")
                    
                p = xclBOProperties()
                
                input_bodevAddr = p.paddr if not (xclGetBOProperties(opt.handle, inputHandle, p)) else -1
                if input_bodevAddr is -1:
                    raise RuntimeError("Error: Unable to get Input BO properties")
                output_bodevAddr = p.paddr if not (xclGetBOProperties(opt.handle, outputHandle, p)) else -1
                if output_bodevAddr is -1:
                    raise RuntimeError("Error: Unable to get Output BO properties")
                
                # Allocate the exec_bo
                execHandle = xclAllocBO(opt.handle, opt.DATA_SIZE, 0, (1 << 31))
                if execHandle == -1:
                    raise RuntimeError("Error: Unable to alloc exec BO")
                execData = xclMapBO(opt.handle, execHandle, True)  # returns mmap()
                if not execData:
                    raise RuntimeError("Error: Unable to map exec BO")

                print("Construct the exec command to run the Input kernel on FPGA")
                
                # construct the exec buffer cmd to start the Input kernel
                config_cmd = ert_configure_cmd.from_buffer(execData.contents)
    
                start_cmd = ert_start_kernel_cmd.from_buffer(execData.contents)
                rsz = int((CONTROL_ADDR_DATA_SIZE / 4 + 1) + 1)  # regmap array size
                memset(execData.contents, 0, sizeof(ert_start_kernel_cmd) + rsz*4)
                start_cmd.m_uert.m_start_cmd_struct.state = 1  # ERT_CMD_STATE_NEW
                start_cmd.m_uert.m_start_cmd_struct.opcode = 0  # ERT_START_CU
                start_cmd.m_uert.m_start_cmd_struct.count = 1 + rsz
                start_cmd.cu_mask = 0x1
                
                # Prepare kernel reg map
                new_data = (c_uint * rsz).from_buffer(execData.contents, 8)
                new_data[CONTROL_ADDR_AP_CTRL] = 0x0
                new_data[int(CONTROL_ADDR_BUFFER_ADDR / 4)] = input_bodevAddr
                new_data[int(CONTROL_ADDR_BUFFER_ADDR / 4 + 1)] = (input_bodevAddr >> 32) & 0xFFFFFFFF
                new_data[int(CONTROL_ADDR_DATA_SIZE / 4)] = DATA_SIZE
                
                if xclExecBuf(opt.handle, execHandle):
                    raise RuntimeError("Error: Unable to issue exec buf")
                print("Kernel start command issued through xclExecBuf : start_kernel")
                print("Now wait until the kernel finish")

                print("Wait until the command finish")
                while start_cmd.m_uert.m_start_cmd_struct.state < ert_cmd_state.ERT_CMD_STATE_COMPLETED:
                    while xclExecWait(opt.handle, 100) == 0:
                        print(".")

                print("Get the output data from the device")
                if xclSyncBO(opt.handle, inputHandle, xclBOSyncDirection.XCL_BO_SYNC_BO_FROM_DEVICE, opt.DATA_SIZE, 0):
                    raise RuntimeError("Error: Unable to sync BO")

                result = input_bo.contents[:len("Hello World")]
                print("Result string = [%s]\n" % result.decode("utf-8"))
               
            except Exception as e:
                print('Error:', str(e))
                raise
            finally:
                if inputHandle != -1:
                    xclFreeBO(opt.handle, inputHandle)
                if outputHandle != -1:
                    xclFreeBO(opt.handle, outputHandle)
                if execHandle != -1:
                    xclFreeBO(opt.handle, execHandle)
                xclCloseContext(opt.handle, opt.xuuid, 0)
            '''
            # Compare result
            mismatch = 0            
                
        finally:
#ACCELIZE DRMLIB DEACTIVATION CODE AREA START
            drm_manager.deactivate()
            print("[DRMLIB] Design locked")
            del drm_manager
            
#ACCELIZE DRMLIB DEACTIVATION CODE AREA STOP

        # Check DRM Activator status
        reg = py_read_register(add_cuidx, 0x38, opt)
        if reg != 0:
            print("Error: DRM Activator status should be 0, not", reg)
            return -1
            
        print("TEST", "FAILED" if mismatch else "PASSED")
        return mismatch
        
    except OSError as o:
        print(o)
        print("FAILED TEST")
        sys.exit(o.errno)
    except AssertionError as a:
        print(a)
        print("FAILED TEST")
        sys.exit(1)
    except Exception as e:
        print(e)
        print("FAILED TEST")
        sys.exit(1)
    finally:
        xclClose(opt.handle)

"""
def runKernel(opt):
    COUNT = 1024
    DATA_SIZE = sizeof(c_int) * COUNT

    khandle = xrtPLKernelOpen(opt.handle, opt.xuuid, "simple")

    boHandle1 = xclAllocBO(opt.handle, DATA_SIZE, 0, opt.first_mem)
    boHandle2 = xclAllocBO(opt.handle, DATA_SIZE, 0, opt.first_mem)

    bo1 = xclMapBO(opt.handle, boHandle1, True)
    bo2 = xclMapBO(opt.handle, boHandle2, True)

    memset(bo1, 0, DATA_SIZE)
    ctypes.memset(bo2, 0, DATA_SIZE)

    bo1Int = ctypes.cast(bo1, ctypes.POINTER(ctypes.c_int))
    bo2Int = ctypes.cast(bo2, ctypes.POINTER(ctypes.c_int))

    for i in range(COUNT):
        bo2Int[i] = i

    # bufReference
    bufReference = [i + i*16 for i in range(COUNT)]

    xclSyncBO(opt.handle, boHandle1, xclBOSyncDirection.XCL_BO_SYNC_BO_TO_DEVICE, DATA_SIZE, 0)
    xclSyncBO(opt.handle, boHandle2, xclBOSyncDirection.XCL_BO_SYNC_BO_TO_DEVICE, DATA_SIZE, 0)

    print("Issue kernel start requests using xrtKernelRun()")
    rhandle1 = xrtKernelRun(khandle, boHandle1, boHandle2, 0x10)

    print("Now wait for the kernels to finish using xrtRunWait()")
    xrtRunWait(rhandle1)

    # get the output xclSyncBO
    print("Get the output data from the device")
    xclSyncBO(opt.handle, boHandle1, xclBOSyncDirection.XCL_BO_SYNC_BO_FROM_DEVICE, DATA_SIZE, 0)

    xrtRunClose(rhandle1)
    xrtKernelClose(khandle)

    assert (bufReference[:COUNT] == bo1Int[:COUNT]), "Computed value does not match reference"
    xclUnmapBO(opt.handle, boHandle2, bo2)
    xclFreeBO(opt.handle, boHandle2)
    xclUnmapBO(opt.handle, boHandle1, bo1)
    xclFreeBO(opt.handle, boHandle1)

def main(args):
    opt = Options()
    Options.getOptions(opt, args)

    try:
        initXRT(opt)
        assert (opt.first_mem >= 0), "Incorrect memory configuration"

        runKernel(opt)
        print("PASSED TEST")

    except OSError as o:
        print(o)
        print("FAILED TEST")
        sys.exit(o.errno)
    except AssertionError as a:
        print(a)
        print("FAILED TEST")
        sys.exit(1)
    except Exception as e:
        print(e)
        print("FAILED TEST")
        sys.exit(1)
    finally:
        xclClose(opt.handle)
"""

if __name__ == "__main__":
    main(sys.argv)
