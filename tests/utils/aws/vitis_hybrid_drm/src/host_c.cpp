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
#define INCR_VALUE 1
#define DRM_CTRL_ADDRESS 0x0000000
#define DRM_ACTR_ADDRESS 0x0010000

#include <unistd.h>
#include <vector>

#include "xcl2.hpp"

// Communication between DRMLib and SW Controller
#include "SharedMemoryManager.hpp"

// Accelize DRMLib
#include "accelize/drmc.h"

using namespace std;
using std::vector;


#define EXIT_FUNC_IF_FAIL(error, call)                                         \
  call;                                                                        \
  if (error) {                                                                 \
    printf("%s:%d Error calling " #call ", error code is: %d\n", __FILE__,     \
           __LINE__, error);                                                   \
    return EXIT_FAILURE;                                                       \
  }

#define EXIT_APP_IF_FAIL(error, call)                                          \
  call;                                                                        \
  if (error) {                                                                 \
    printf("%s:%d Error calling " #call ", error code is: %d\n", __FILE__,     \
           __LINE__, error);                                                   \
    return EXIT_FAILURE;                                                       \
  }
  
#define EXIT_SAFELY_IF_FAIL(call)                                              \
  if (call) {                                                                  \
    printf("%s:%d Error calling " #call "\n", __FILE__, __LINE__);             \
    goto EXIT_SAFELY;                                                          \
  }  


SharedMemoryManagerLibrary::SharedMemoryManager gSharedMemoryManager;


/*
 * DRMLib Read Callback Function
 */
int32_t drm_read_callback(uint32_t addr, uint32_t* value, void* context) {
	return gSharedMemoryManager.readSharedMemory(addr, value);
}

/*
 * DRMLib Write Callback Function
 */
int32_t drm_write_callback(uint32_t addr, uint32_t value, void* context) {
	return(gSharedMemoryManager.writeSharedMemory(addr, value));
}

/*
 * DRMLib Error Callback Function
 */
void drm_error_callback( const char* errmsg, void* context ){
    printf("ERROR [DRM]: %s", errmsg);
}



cl::CommandQueue CmdQ;
cl::Context Context;
cl::Program Program;
cl::Kernel KrnlInputStage;
cl::Kernel KrnlOutputStage;


int pl_load(const char* bin_file) {
	std::string binaryFile(bin_file);

	// OPENCL HOST CODE AREA START
	cl_int err;

	// Create Program and Kernels.
	auto devices = xcl::get_xil_devices();

	// read_binary_file() is a utility API which will load the binaryFile
	// and will return the pointer to file buffer.
	auto fileBuf = xcl::read_binary_file(binaryFile);
	cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
	bool valid_device = false;
	for (unsigned int i = 0; i < devices.size(); i++) {
		auto device = devices[i];
		// Creating Context and Command Queue for selected Device
		EXIT_FUNC_IF_FAIL(err, Context = cl::Context(device, nullptr, nullptr, nullptr, &err));
		EXIT_FUNC_IF_FAIL(err, CmdQ = cl::CommandQueue(Context, device,
											CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE, &err));

		std::cout << "Trying to program device[" << i << "]: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
		Program = cl::Program(Context, {device}, bins, nullptr, &err);
		if (err != CL_SUCCESS) {
			std::cout << "Failed to program device[" << i << "] with xclbin file!\n";
		} else {
			std::cout << "Device[" << i << "]: program successful!\n";
			valid_device = true;
			break; // we break because we found a valid device
		}
	}
	if (!valid_device) {
		std::cout << "Failed to program any device found, exit!\n";
		exit(EXIT_FAILURE);
	}

	// Create kernels
	EXIT_FUNC_IF_FAIL(err, KrnlInputStage = cl::Kernel(Program, "krnl_input_stage_rtl", &err));
	EXIT_FUNC_IF_FAIL(err, KrnlOutputStage = cl::Kernel(Program, "krnl_output_stage_rtl", &err));

	return 0;
}

int pl_unload() {
	// Xilinx API does not require explicit releasing of resources
	return 0;
}



/*
 * Read Register Function
 */
int32_t read_register(uint32_t addr, uint32_t* value)
{
    return 0;
}

/*
 * Write Register Function
 */
int32_t write_register(uint32_t addr, uint32_t value)
{
    return 0;
}


/**
 * Entry point
 */
int main(int argc, char **argv) {
	
    if (argc != 2) {
        printf("Usage: %s  <XCLBIN File>", argv[0]);
        return EXIT_FAILURE;
    }
    
	std::string binaryFile = argv[1];
    
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
    unsigned int reg = 0;
    
    drm_write_callback(0, 0, NULL);
    
    drm_read_callback(0x70, &reg, NULL);
    printf("DRM version: 0x%08X\n", reg);
    
    drm_read_callback(0xFFF0, &reg, NULL);
    printf("Freq detection version: 0x%08X\n", reg);
    
    drm_write_callback(0, 0, NULL);
    drm_read_callback(0, &reg, NULL);
    if (reg != 0)
		printf("Unexpected read value: got %d, exp %d\n", reg, 0);
    drm_write_callback(0, 1, NULL);
    drm_read_callback(0, &reg, NULL);
    if (reg != 1)
		printf("Unexpected read value: got %d, exp %d\n", reg, 1);
		
	DrmManager *pDrmManager = NULL;

//    EXIT_SAFELY_IF_FAIL(pl_load(binaryFile.c_str()))
/*        
    // Read DRM Controller version register
    uint32_t reg;
    write_register(DRM_CTRL_ADDRESS + 0x0, 0);
    read_register(DRM_CTRL_ADDRESS + 0x70, &reg);
    printf("DRM Controller version: %X\n", reg);

    // Check DRM Activator existance
    read_register(DRM_ACTR_ADDRESS + 0x40, &reg);
    printf("DRM Activator existance: %X\n", reg);
    if (reg != 0x600DC0DE) {
        printf("Error: DRM Activator existance should be 0x600DC0DE\n");
        return -1;
    }

    // Check DRM Activator status
    read_register(DRM_ACTR_ADDRESS + 0x38, &reg);
    if (reg != 0) {
        printf("Error: DRM Activator status should be 0, not %d\n", reg);
        return -1;
    }

    // Check DRM Detection Method
    read_register(DRM_CTRL_ADDRESS + 0xFFFC, &reg);
    printf("DRM Controller frequency counter = 0x%08X\n", reg);
    write_register(DRM_CTRL_ADDRESS + 0xFFFC, 0);
    sleep(1);
    read_register(DRM_CTRL_ADDRESS + 0xFFFC, &reg);
    printf("DRM Controller frequency counter = 0x%08X\n", reg);
*/
    //ACCELIZE DRMLIB CODE AREA START
    if (DRM_OK != DrmManager_alloc(&pDrmManager,
            "conf.json",
            "cred.json",
            drm_read_callback, drm_write_callback, drm_error_callback,
            NULL)) {
        printf("Error allocating DRM Manager object: %s", pDrmManager->error_message);
        return -1;
    }
    printf("[DRMLIB] Allocated\n");

    if (DRM_OK != DrmManager_activate(pDrmManager, false)) {
        printf("Error activating DRM Manager object: %s", pDrmManager->error_message);
        DrmManager_free(&pDrmManager);
        return -1;
    }
/*    // Check DRM Activator status
    read_register(DRM_ACTR_ADDRESS + 0x38, &reg);
    if (reg != 3) {
		printf("Error: DRM Activator status should be 3, not %d\n", reg);
        DrmManager_free(&pDrmManager);
        return -1;
    }*/
	printf("[DRMLIB] Design unlocked\n");
    //ACCELIZE DRMLIB CODE AREA STOP


    //ACCELIZE DRMLIB CODE AREA START
    if (DRM_OK != DrmManager_deactivate(pDrmManager, false))
        printf("Error deactivating DRM Manager object: %s", pDrmManager->error_message);
    else
        printf("[DRMLIB] Design locked\n");

    if (DRM_OK != DrmManager_free(&pDrmManager))
        printf("Error deallocating DRM Manager object: %s", pDrmManager->error_message);
    //ACCELIZE DRMLIB CODE AREA STOP
/*
    // Check DRM Activator status
    read_register(DRM_ACTR_ADDRESS + 0x38, &reg);
    if (reg != 0) {
        printf("Error: DRM Activator status should be 0, not 0x%X\n", reg);
        return -1;
    }
*/
EXIT_SAFELY:
    pl_unload();

    return EXIT_SUCCESS;
}
