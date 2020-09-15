import sys
from ctypes import c_uint, c_int, byref, sizeof, memset, cast, POINTER

# Following found in PYTHONPATH setup by XRT
from xrt_binding import *

#sys.path.append('/opt/xilinx/xrt/test/')
from utils_binding import *

# Get DRM library
sys.path.insert(0, '/opt/accelize/drmlib_jbl/build/python3_bdist')
import accelize_drm

INCR_VALUE = 10
DATA_SIZE = 4096


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
    mismatch = DATA_SIZE
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

        try:            
            # Check a DRM Activator is detected
            reg = py_read_register(add_cuidx, 0x40, opt);
            if reg != 0x600DC0DE:
                raise RuntimeError("Error: No DRM Activator is detected")
            print("A DRM Activator is detected")

            # Check DRM Activator status
            reg = py_read_register(add_cuidx, 0x38, opt)
            if reg != 3:
                raise RuntimeError("Error: DRM Activator status should be 3, not", reg)
            print("Checked Activator is unlocked")
            
            # Perform some processing with the kernels
            try:
                c_data_size = sizeof(c_int) * DATA_SIZE

                input_kernel = adder_kernel = output_kernel = None
                rInputhandle = rAdderhandle = rOutputhandle = None
                input_bo_handle = output_bo_handle = None
                input_bo = output_bo = None        

                input_kernel = xrtPLKernelOpen(opt.handle, opt.xuuid, 'krnl_input_stage_rtl'.encode('utf-8'))
                adder_kernel = xrtPLKernelOpen(opt.handle, opt.xuuid, 'krnl_adder_stage_rtl'.encode('utf-8'))
                output_kernel = xrtPLKernelOpen(opt.handle, opt.xuuid, 'krnl_output_stage_rtl'.encode('utf-8'))

                input_bo_handle = xclAllocBO(opt.handle, c_data_size, 0, opt.first_mem)
                input_bo = xclMapBO(opt.handle, input_bo_handle, True)
                memset(input_bo, 0, DATA_SIZE)
                input_bo_int = cast(input_bo, POINTER(c_int))
                for i in range(DATA_SIZE):
                    input_bo_int[i] = i

                output_bo_handle = xclAllocBO(opt.handle, c_data_size, 0, opt.first_mem)
                output_bo = xclMapBO(opt.handle, output_bo_handle, True)
                memset(output_bo, 0, DATA_SIZE)

                xclSyncBO(opt.handle, input_bo_handle, xclBOSyncDirection.XCL_BO_SYNC_BO_TO_DEVICE, c_data_size, 0)
                xclSyncBO(opt.handle, output_bo_handle, xclBOSyncDirection.XCL_BO_SYNC_BO_TO_DEVICE, c_data_size, 0)

                print("Issue kernel start requests")
                rInputhandle = xrtKernelRun(input_kernel, input_bo_handle, DATA_SIZE)
                rAdderhandle = xrtKernelRun(adder_kernel, INCR_VALUE, DATA_SIZE)
                rOutputhandle = xrtKernelRun(output_kernel, output_bo_handle, DATA_SIZE)

                print("Now wait for the kernels to finish")
                xrtRunWait(rInputhandle)
                xrtRunWait(rAdderhandle)
                xrtRunWait(rOutputhandle)
                
                print("Get the output data produced by the kernel runs from the device")
                xclSyncBO(opt.handle, output_bo_handle, xclBOSyncDirection.XCL_BO_SYNC_BO_FROM_DEVICE, c_data_size, 0)

                # Compare result
                hw_result_int = cast(output_bo, POINTER(c_int))
                for i in range(DATA_SIZE):
                    if hw_result_int[i] == i + INCR_VALUE:
                        mismatch -= 1

            except Exception as e:
                print('Error:', str(e))

            finally:
                xrtRunClose(rInputhandle)
                xrtRunClose(rAdderhandle)
                xrtRunClose(rOutputhandle)
                
                xrtKernelClose(input_kernel)
                xrtKernelClose(adder_kernel)
                xrtKernelClose(output_kernel)

                xclUnmapBO(opt.handle, input_bo_handle, input_bo)
                xclFreeBO(opt.handle, input_bo_handle)
                xclUnmapBO(opt.handle, output_bo_handle, output_bo)
                xclFreeBO(opt.handle, output_bo_handle)            
                
        finally:
#ACCELIZE DRMLIB DEACTIVATION CODE AREA START
            drm_manager.deactivate()
            del drm_manager
            print("[DRMLIB] Design locked")
#ACCELIZE DRMLIB DEACTIVATION CODE AREA STOP

        # Check DRM Activator status
        reg = py_read_register(add_cuidx, 0x38, opt)
        if reg != 0:
            print("Error: DRM Activator status should be 0, not", reg)
            return -1
        print("Checked Activator is locked")

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


if __name__ == "__main__":
    main(sys.argv)
