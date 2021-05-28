/**********
Copyright (c) 2019, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

/*******************************************************************************
Description: Vitis Vector Addition using Streams
*******************************************************************************/

#define DATA_SIZE 4096
#define INCR_VALUE 10

#define DRM_CTRL_ADDR           0x0000000
#define DRM_ACTR_ADDR           0x0010000
#define DRM_ACTR_ADDR_RANGE     0x10000

#define ACT_STATUS_REG_OFFSET 0x38
#define MAILBOX_REG_OFFSET    0x3C
#define INC_EVENT_REG_OFFSET  0x40


#include <unistd.h>
#include <vector>
#include <xclhal2.h>

#include "xcl2.hpp"

// Accelize DRMLib
#include "accelize/drm.h"

using namespace std;
using std::vector;

using namespace Accelize::DRM;


xclDeviceHandle boardHandler;


/*
 * Read Register Function
 */
int32_t read_register(uint32_t addr, uint32_t* value)
{
    if(xclLockDevice(boardHandler)) {
        cout << "[ERROR] xclLock failed ..." << endl;
        return 1;
    }
    int ret = (int)xclRead(boardHandler, XCL_ADDR_KERNEL_CTRL, addr, value, 4);
    if(ret <= 0) {
        cout << "[ERROR] " << __FUNCTION__ << ": Failed to read @0x" << hex << addr << dec << endl;
        return 1;
    }
    if(xclUnlockDevice(boardHandler)) {
        cout << "[ERROR] xclUnlock failed ..." << endl;
        return -1;
    }
    return 0;
}

/*
 * Write Register Function
 */
int32_t write_register(uint32_t addr, uint32_t value)
{
    if(xclLockDevice(boardHandler)) {
        cout << "[ERROR] xclLock failed ..." << endl;
        return 1;
    }
    int ret = (int)xclWrite(boardHandler, XCL_ADDR_KERNEL_CTRL, addr, &value, 4);
    if(ret <= 0) {
        cout << "[ERROR] " << __FUNCTION__ << ": Failed to write @0x" << hex << addr << dec << endl;
        return 1;
    }
    if(xclUnlockDevice(boardHandler)) {
        cout << "[ERROR] xclUnlock failed ..." << endl;
        return -1;
    }
    return 0;
}


/**
 * Entry point
 */
int main(int argc, char **argv) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <XCLBIN File>" << endl;
        return EXIT_FAILURE;
    }

    string binaryFile = argv[1];

    //Allocate Memory in Host Memory
    auto vector_size_bytes = sizeof(int) * DATA_SIZE;

    vector<int, aligned_allocator<int>> source_input(DATA_SIZE);
    vector<int, aligned_allocator<int>> source_hw_results(DATA_SIZE);
    vector<int, aligned_allocator<int>> source_sw_results(DATA_SIZE);

    // Create the test data and Software Result
    for (int i = 0; i < DATA_SIZE; i++) {
        source_input[i] = i;
        source_sw_results[i] = i + INCR_VALUE;
        source_hw_results[i] = 0;
    }

    //OPENCL HOST CODE AREA START
    cl_int err;
    cl::CommandQueue q;
    cl::Context context;
    cl::Program program;
    //Create Program and Kernels.
    auto devices = xcl::get_xil_devices();

    // read_binary_file() is a utility API which will load the binaryFile
    // and will return the pointer to file buffer.
    auto fileBuf = xcl::read_binary_file(binaryFile);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    int valid_device = 0;
    for (unsigned int i = 0; i < devices.size(); i++) {
        auto device = devices[i];
        // Creating Context and Command Queue for selected Device
        OCL_CHECK(err, context = cl::Context({device}, NULL, NULL, NULL, &err));
        OCL_CHECK(err,
                  q = cl::CommandQueue(context,
                                       device,
                                       CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE |
                                           CL_QUEUE_PROFILING_ENABLE,
                                       &err));

        cout << "Trying to program device[" << i
                  << "]: " << device.getInfo<CL_DEVICE_NAME>() << endl;
                  program = cl::Program(context, {device}, bins, NULL, &err);
        if (err != CL_SUCCESS) {
            cout << "Failed to program device[" << i
                      << "] with xclbin file!\n";
        } else {
            cout << "Device[" << i << "]: program successful!\n";
            valid_device++;
            break; // we break because we found a valid device
        }
    }
    if (valid_device == 0) {
        cout << "Failed to program any device found, exit!\n";
        exit(EXIT_FAILURE);
    }

    // Init xclhal2 library
    if(xclProbe() < 1) {
        cout << "[ERROR] xclProbe failed ..." << endl;
        return -1;
    }
    boardHandler = xclOpen(0, "xclhal2_logfile.log", XCL_INFO);
    if(boardHandler == NULL) {
        cout << "[ERROR] xclOpen failed ..." << endl;
        return -1;
    }

    // Read DRM Controller version register
    uint32_t reg;
    write_register(DRM_CTRL_ADDR + 0x0, 0);
    read_register(DRM_CTRL_ADDR + 0x70, &reg);
    cout << "DRM Controller version: " << hex << reg << dec << endl;

    // Check DRM Activator existance
    read_register(DRM_ACTR_ADDR + 0x40, &reg);
    cout << "DRM Activator existance: " << hex << reg << dec << endl;
    if (reg != 0x600DC0DE) {
        cout << "Error: DRM Activator existance should be 0x600DC0DE" << endl;
        return -1;
    }

    // Check DRM Activator status
    read_register(DRM_ACTR_ADDR + 0x38, &reg);
    if (reg != 0) {
        cout << "Error: DRM Activator status should be 0, not " << reg << endl;
        return -1;
    }

    // Check DRM Detection Method
    read_register(DRM_CTRL_ADDR + 0xFFFC, &reg);
    cout << "DRM Controller frequency counter = 0x" << hex << reg << dec << endl;
    write_register(DRM_CTRL_ADDR + 0xFFFC, 0);
    sleep(1);
    read_register(DRM_CTRL_ADDR + 0xFFFC, &reg);
    cout << "DRM Controller frequency counter = 0x" << hex << reg << dec << endl;

    //ACCELIZE DRMLIB CODE AREA START
    DrmManager *pDrmManager = new DrmManager(
            string("conf.json"),
            string("/dev/shm/cred.json"),
            [&]( uint32_t offset, uint32_t* p_value ) { // Read DRM register callback
                return read_register( DRM_CTRL_ADDR + offset, p_value, pci_bar_handle );
            },
            [&]( uint32_t offset, uint32_t value ) {    // Write DRM register callback
                return write_register( DRM_CTRL_ADDR + offset, value, pci_bar_handle );
            },
            [&]( const string& errmsg ) {    // DRM Error callback
                cout << "ERROR: From async callback: " << errmsg.c_str() << endl;
            }
        );
    cout << "[DRMLIB] Allocated" << endl;

    try {

        pDrmManager->activate(true);

        // Check DRM Activator status
        read_register(DRM_ACTR_ADDR + 0x38, &reg);
        if (reg != 3) {
            throw exception("Error: DRM Activator status should be 3");
        }
        cout << "[DRMLIB] Design unlocked" << endl;
        //ACCELIZE DRMLIB CODE AREA STOP

        OCL_CHECK(
            err,
            cl::Kernel krnl_adder_stage(program, "krnl_adder_stage_rtl", &err));
        OCL_CHECK(
            err,
            cl::Kernel krnl_input_stage(program, "krnl_input_stage_rtl", &err));
        OCL_CHECK(
            err,
            cl::Kernel krnl_output_stage(program, "krnl_output_stage_rtl", &err));

        //Allocate Buffer in Global Memory
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

        auto inc = INCR_VALUE;
        auto size = DATA_SIZE;
        //Set the Kernel Arguments
        OCL_CHECK(err, err = krnl_input_stage.setArg(0, buffer_input));
        OCL_CHECK(err, err = krnl_input_stage.setArg(1, size));
        OCL_CHECK(err, err = krnl_adder_stage.setArg(0, inc));
        OCL_CHECK(err, err = krnl_adder_stage.setArg(1, size));
        OCL_CHECK(err, err = krnl_output_stage.setArg(0, buffer_output));
        OCL_CHECK(err, err = krnl_output_stage.setArg(1, size));

        //Copy input data to device global memory
        cl::Event write_event;
        OCL_CHECK(
            err,
            err = q.enqueueMigrateMemObjects(
                {buffer_input}, 0 /* 0 means from host*/, NULL, &write_event));

        //Launch the Kernel
        vector<cl::Event> eventVec;
        eventVec.push_back(write_event);
        OCL_CHECK(err, err = q.enqueueTask(krnl_input_stage, &eventVec));
        OCL_CHECK(err, err = q.enqueueTask(krnl_adder_stage, &eventVec));
        OCL_CHECK(err, err = q.enqueueTask(krnl_output_stage, &eventVec));

        //wait for all kernels to finish their operations
        OCL_CHECK(err, err = q.finish());

        //Copy Result from Device Global Memory to Host Local Memory
        OCL_CHECK(err,
                  err = q.enqueueMigrateMemObjects({buffer_output},
                                                   CL_MIGRATE_MEM_OBJECT_HOST));
        OCL_CHECK(err, err = q.finish());

        //OPENCL HOST CODE AREA END

        //ACCELIZE DRMLIB CODE AREA START
        pDrmManager=>deactivate(false)
        cout << "[DRMLIB] Design locked" << endl;

    } catch( const Exception& e ) {
        cout << "ERROR: [DRMLIB] Following error occurred: " << e.what() << endl;
    } catch( const exception& e ) {
        cout << "ERROR: Following error occurred: " << e.what() << endl;
    } catch(...) {
        cout << "ERROR: Unsupported error" << endl;
    }
    delete pDrmManager;
    pDrmManager = nullptr;
    //ACCELIZE DRMLIB CODE AREA STOP

    // Check DRM Activator status
    read_register(DRM_ACTR_ADDR + 0x38, &reg);
    if (reg != 0) {
        cout << "Error: DRM Activator status should be 0, not " << reg << endl;
        return -1;
    }

    // Release xclhal2 board handler
    xclClose(boardHandler);

    // Compare the results of the Device to the simulation
    int match = 0;
    for (int i = 0; i < DATA_SIZE; i++) {
        if (source_hw_results[i] != source_sw_results[i]) {
            cout << "Error: Result mismatch" << endl;
            cout << "i = " << i << " CPU result = " << source_sw_results[i]
                      << " Device result = " << source_hw_results[i]
                      << endl;
            match = 1;
            break;
        }
    }


    cout << "TEST " << (match ? "FAILED" : "PASSED") << endl;
    return (match ? EXIT_FAILURE : EXIT_SUCCESS);
}
