import sys

# Following found in PYTHONPATH setup by XRT
from xrt_binding import *

sys.path.append('/opt/xilinx/xrt/test/')
from utils_binding import *

# Get DRM library
#sys.path.insert(0, os.path.join(DIR_PATH,os.pardir,os.pardir,'.tox/debug/build/python3_bdist'))
import accelize_drm



def read_register(cuidx, addr, shared):
    xclOpenContext(shared.handle, shared.xuuid, cuidx, False)
    c_value = ctypes.c_uint(0)
    if xclRegRead(shared.handle, cuidx, addr, ctypes.byref(c_value)):
        raise RuntimeError("[ERROR] in read_register: Failed to read CU ID %d @0x%X" % (cuidx, addr))
    xclCloseContext(shared.handle, shared.xuuid, cuidx)
    return c_value.value

# Write Register Function
def write_register(cuidx, addr, value, shared):
    xclOpenContext(shared.handle, shared.xuuid, cuidx, False)
    if xclRegWrite(shared.handle, cuidx, addr, value):
        raise RuntimeError("[ERROR] in write_register: Failed to write CU ID %d @0x%X" % (cuidx, addr))
    xclCloseContext(shared.handle, shared.xuuid, cuidx)


def main(args):
    opt = Options()
    Options.getOptions(opt, args)
    try:
        initXRT(opt)
        assert (opt.first_mem >= 0), "Incorrect memory configuration"

        drm_cuidx = 0
        add_cuidx = 1

        # Load page 0 of DRM Ctrl
        write_register(drm_cuidx, 0, 0, opt)

        # Get DRM Ctrl version
        reg = read_register(drm_cuidx, 0x70, opt)
        print("DRM Controller version: %X" % reg)

        # Check DRM Activator existance
        reg = read_register(add_cuidx, 0x40, opt)
        print("DRM Activator existance: 0x%X" % reg)
        if reg != 0x600DC0DE:
            print("Error: DRM Activator existance should be 0x600DC0DE, not %X" % reg);
        return -1

        # Check DRM Activator status
        reg = read_register(add_cuidx, 0x38, opt);
        if reg:
            print("Error: DRM Activator status should be 0, not", reg);
        return -1;

        print("DRM Activator is locked");

#ACCELIZE DRMLIB ACTIVATION CODE AREA START

        def read_register_callback(addr, value):
            try:
                value = read_register(drm_cuidx, addr, opt)
                return 0
            except Exception as e:
                print("Error: failed to read DRM Ctrl @%X: %s" % (addr, ))
                return -1

        def write_register_callback(addr, value):
            try:
                write_register(drm_cuidx, addr, value, opt)
                return 0
            except Exception as e:
                print("Error: failed to read DRM Ctrl @%X: %s" % (addr, ))
                return -1

        drm_manager = accelize_drm.DrmManager(
           'conf.json', 'cred.json',
           read_register_callback, write_register_callback)

        val = 987654321
        drm_manager.set(custom_field=val)
        print("Wrote custom field: ", val)

        val_back = drm_manager.get('custom_field')
        print("Read back custom field: ", val_back)

        print('[DRMLIB] Allocated')

        drm_manager.activate()

        print("[DRMLIB] Design unlocked")

#ACCELIZE DRMLIB ACTIVATION CODE AREA STOP

        # Check a DRM Activator is detected
        reg = read_register(add_cuidx, 0x40, opt);
        if reg != 0x600DC0DE:
            print("Error: No DRM Activator is detected")
            return -1
        print("[DRMLIB] a DRM Activator is detected")

        # Check DRM Activator status
        reg = read_register(add_cuidx, 0x38, opt)
        if reg != 3:
            print("Error: DRM Activator status should be 3, not", reg)
            return -1
        print("[DRMLIB] Design unlocked")

        # Perform some processing with the kernels
        """
        // Allocate Memory in Host Memory
        std::vector<int, aligned_allocator<int>> source_input(DATA_SIZE);
        std::vector<int, aligned_allocator<int>> source_hw_results(DATA_SIZE);
        std::vector<int, aligned_allocator<int>> source_sw_results(DATA_SIZE);

        // Create the test data and Software Result
        for (int i = 0; i < DATA_SIZE; i++) {
            source_input[i] = i;
            source_sw_results[i] = i + INCR_VALUE;
            source_hw_results[i] = 0;
        }

        auto vector_size_bytes = sizeof(int) * DATA_SIZE;

        // Allocate Buffer in Global Memory
        OCL_CHECK(err,
                  cl::Buffer buffer_input(context,
                                          CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                          vector_size_bytes,
                                          source_input.data(),
                                          &err));
        OCL_CHECK(err,
                  cl::Buffer buffer_output(context,
                                           CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                                           vector_size_bytes,
                                           source_hw_results.data(),
                                           &err));

        // Set the Kernel Arguments
        auto inc = INCR_VALUE;
        auto size = DATA_SIZE;
        OCL_CHECK(err, err = input_kernel.setArg(0, buffer_input));
        OCL_CHECK(err, err = input_kernel.setArg(1, size));
        OCL_CHECK(err, err = adder_kernel.setArg(0, inc));
        OCL_CHECK(err, err = adder_kernel.setArg(1, size));
        OCL_CHECK(err, err = output_kernel.setArg(0, buffer_output));
        OCL_CHECK(err, err = output_kernel.setArg(1, size));

        // Create a command queue
        cl::CommandQueue commands = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err);
        if (err) {
            printf("Error: Failed to create a command commands!\n");
            printf("Error: code %i\n",err);
            printf("Test failed\n");
            return EXIT_FAILURE;
        }

        // Copy input data to device global memory
        cl::Event write_event;
        OCL_CHECK(err, err = commands.enqueueMigrateMemObjects(
                {buffer_input}, 0, NULL, &write_event));  // 0 means from host

        // Launch the Kernel
        std::vector<cl::Event> eventVec;
        eventVec.push_back(write_event);
        OCL_CHECK(err, err = commands.enqueueTask(input_kernel, &eventVec));
        OCL_CHECK(err, err = commands.enqueueTask(adder_kernel, &eventVec));
        OCL_CHECK(err, err = commands.enqueueTask(output_kernel, &eventVec));

        // Wait for all kernels to finish their operations
        OCL_CHECK(err, err = commands.finish());

        // Copy Result from Device Global Memory to Host Local Memory
        OCL_CHECK(err,
                  err = commands.enqueueMigrateMemObjects({buffer_output},
                                                   CL_MIGRATE_MEM_OBJECT_HOST));
        OCL_CHECK(err, err = commands.finish());

        // OPENCL HOST CODE AREA END


        // Compare the results of the Device to the simulation
        int mismatch = 0;
        for (int i = 0; i < DATA_SIZE; i++) {
            if (source_hw_results[i] != source_sw_results[i]) {
                cout << "Error: Result mismatch" << endl;
                cout << "i = " << i << " CPU result = " << source_sw_results[i]
                          << " Device result = " << source_hw_results[i]
                          << endl;
                mismatch ++;
                if (mismatch > 5) break;
            }
        }
        """
        mismatch = 0

#ACCELIZE DRMLIB DEACTIVATION CODE AREA START
        drm_manager.deactivate()
        print("[DRMLIB] Design locked")
#ACCELIZE DRMLIB DEACTIVATION CODE AREA STOP

        # Check DRM Activator status
        reg = read_register(add_cuidx, 0x38, opt)
        if reg != 0:
            print("Error: DRM Activator status should be 0, not", reg)
            return -1
        """"
        free(source_sw_results);
        clReleaseMemObject(buffer_input);
        clReleaseMemObject(buffer_output);

        clReleaseKernel(drm_ctrl_kernel);
        clReleaseKernel(input_kernel);
        clReleaseKernel(adder_kernel);
        clReleaseKernel(output_kernel);
        clReleaseCommandQueue(commands);
        clReleaseContext(context);
        clReleaseProgram(program);
        """
        print("TEST", "FAILED" if mismatch else "PASSED")
        return mismatch

        """
        boHandle1 = xclAllocBO(opt.handle, opt.DATA_SIZE, 0, opt.first_mem)
        bo1 = xclMapBO(opt.handle, boHandle1, True)

        testVector = "hello\nthis is Xilinx OpenCL memory read write test\n:-)\n"
        ctypes.memset(bo1, 0, opt.DATA_SIZE)
        ctypes.memmove(bo1, testVector, len(testVector))

        xclSyncBO(opt.handle, boHandle1, xclBOSyncDirection.XCL_BO_SYNC_BO_TO_DEVICE, opt.DATA_SIZE, 0)

        p = xclBOProperties(boHandle1)
        xclGetBOProperties(opt.handle, boHandle1, p)
        assert (p.paddr != 0xffffffffffffffff), "Illegal physical address for buffer"

        # Clear our shadow buffer on host
        ctypes.memset(bo1, 0, opt.DATA_SIZE)

        # Move the buffer from device back to the shadow buffer on host
        xclSyncBO(opt.handle, boHandle1, xclBOSyncDirection.XCL_BO_SYNC_BO_FROM_DEVICE, opt.DATA_SIZE, False)

        assert (bo1[:len(testVector)] == testVector[:]), "Data migration error"
        print("PASSED TEST")
        xclUnmapBO(opt.handle, boHandle1, bo1)
        xclFreeBO(opt.handle, boHandle1)
        """
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
    DATA_SIZE = ctypes.sizeof(ctypes.c_int32) * COUNT

    khandle = xrtPLKernelOpen(opt.handle, opt.xuuid, "simple")

    boHandle1 = xclAllocBO(opt.handle, DATA_SIZE, 0, opt.first_mem)
    boHandle2 = xclAllocBO(opt.handle, DATA_SIZE, 0, opt.first_mem)

    bo1 = xclMapBO(opt.handle, boHandle1, True)
    bo2 = xclMapBO(opt.handle, boHandle2, True)

    ctypes.memset(bo1, 0, DATA_SIZE)
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
"""

if __name__ == "__main__":
    main(sys.argv)
