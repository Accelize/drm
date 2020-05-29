/*******************************************************************************
Description: Vitis DRM Controller Register access
*******************************************************************************/


#include <unistd.h>
#include <iostream>
#include <cerrno>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <uuid/uuid.h>


#include <fstream>
#include <iomanip>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>


#include <CL/opencl.h>
#include <CL/cl_ext.h>
#include <CL/cl_ext_xilinx.h>

#include "xclhal2.h"
#include "experimental/xclbin-util.h"

#include "accelize/drmc.h"


#define TARGET_DEVICE "xilinx_u50_gen3x16_xdma_201920_3"

#define INCR_VALUE 10
#define DATA_SIZE 4096
#define MAX_LENGTH 8192
#define MEM_ALIGNMENT 4096

#if defined(SDX_PLATFORM) && !defined(TARGET_DEVICE)
#define STR_VALUE(arg)      #arg
#define GET_STRING(name) STR_VALUE(name)
#define TARGET_DEVICE GET_STRING(SDX_PLATFORM)
#endif


using namespace std;

xclDeviceHandle handle;
uuid_t xclbinId;


/*
 * Read Register Function
 */
int32_t read_register(uint32_t cuidx, uint32_t addr, uint32_t* value)
{
    xclOpenContext(handle, xclbinId, cuidx, false);
    int ret = xclRegRead(handle, cuidx, addr, value);
    if (ret) {
        cout << "[ERROR] " << __FUNCTION__ << ": Failed to read CU ID" << cuidx << " @0x" << hex << addr << dec << endl;
    }
    xclCloseContext(handle, xclbinId, cuidx);
    return ret;
}

/*
 * Write Register Function
 */
int32_t write_register(uint32_t cuidx, uint32_t addr, uint32_t value)
{
    xclOpenContext(handle, xclbinId, cuidx, false);
    int ret = xclRegWrite(handle, cuidx, addr, value);
    if (ret) {
        cout << "[ERROR] " << __FUNCTION__ << ": Failed to write CU ID" << cuidx << " @0x" << hex << addr << dec << endl;
    }
    xclCloseContext(handle, xclbinId, cuidx);
    return ret;
}

/*
 * DRMLib Read Callback Function
 */
int32_t drm_read_callback(uint32_t addr, uint32_t* value, void* context)
{
    return read_register(*(int*)context, addr, value);
}

/*
 * DRMLib Write Callback Function
 */
int32_t drm_write_callback(uint32_t addr, uint32_t value, void* context)
{
    return write_register(*(int*)context, addr, value);
}

/*
 * DRMLib Error Callback Function
 */
void drm_error_callback( const char* errmsg, void* context ){
    printf("ERROR [DRM]: %s", errmsg);
}

std::vector<unsigned char> readBinary(const std::string &fileName) {
    std::ifstream file(fileName, std::ios::binary | std::ios::ate);
    if (file) {
      file.seekg(0, std::ios::end);
      streamsize size = file.tellg();
      file.seekg(0, std::ios::beg);
      std::vector<unsigned char> buffer(size);
      file.read((char *)buffer.data(), size);
      return buffer;
    } else {
      return std::vector<unsigned char>(0);
    }
}


/**
 * Entry point
 */
int main(int argc, char **argv) {

    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <XCLBIN File>" << endl;
        return EXIT_FAILURE;
    }

    cl_int err;                           // error code returned from api calls
    cl_uint check_status = 0;
    const cl_uint number_of_words = 4096; // 16KB of data

    cl_platform_id platform_id;         // platform id
    cl_device_id device_id;             // compute device id
    cl_context context;                 // compute context

    char cl_platform_vendor[1001];
    char target_device_name[1001] = TARGET_DEVICE;

    // Get all platforms and then select Xilinx platform
    cl_platform_id platforms[16];       // platform id
    cl_uint platform_count;
    cl_uint platform_found = 0;
    err = clGetPlatformIDs(16, platforms, &platform_count);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to find an OpenCL platform!\n");
        printf("Test failed\n");
        return EXIT_FAILURE;
    }
    printf("INFO: Found %d platforms\n", platform_count);

    // Find Xilinx Plaftorm
    for (cl_uint iplat=0; iplat<platform_count; iplat++) {
        err = clGetPlatformInfo(platforms[iplat], CL_PLATFORM_VENDOR, 1000, (void *)cl_platform_vendor,NULL);
        if (err != CL_SUCCESS) {
            printf("Error: clGetPlatformInfo(CL_PLATFORM_VENDOR) failed!\n");
            printf("Test failed\n");
            return EXIT_FAILURE;
        }
        if (strcmp(cl_platform_vendor, "Xilinx") == 0) {
            printf("INFO: Selected platform %d from %s\n", iplat, cl_platform_vendor);
            platform_id = platforms[iplat];
            platform_found = 1;
        }
    }
    if (!platform_found) {
        printf("ERROR: Platform Xilinx not found. Exit.\n");
        return EXIT_FAILURE;
    }

    // Get Accelerator compute device
    cl_uint num_devices;
    cl_uint device_found = 0;
    cl_device_id devices[16];  // compute device id
    char cl_device_name[1001];
    err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ACCELERATOR, 16, devices, &num_devices);
    printf("INFO: Found %d devices\n", num_devices);
    if (err != CL_SUCCESS) {
        printf("ERROR: Failed to create a device group!\n");
        printf("ERROR: Test failed\n");
        return -1;
    }

    // Iterate all devices to select the target device.
    for (cl_uint i=0; (i<num_devices) && (device_found==0); i++) {
        err = clGetDeviceInfo(devices[i], CL_DEVICE_NAME, 1024, cl_device_name, 0);
        if (err != CL_SUCCESS) {
            printf("Error: Failed to get device name for device %d!\n", i);
            printf("Test failed\n");
            return EXIT_FAILURE;
        }
        printf("CL_DEVICE_NAME %s\n", cl_device_name);
        if(strcmp(cl_device_name, target_device_name) == 0) {
            device_id = devices[i];
            device_found = 1;
            printf("Selected %s as the target device\n", cl_device_name);
        }
    }

    if (!device_found) {
        printf("Target device %s not found. Exit.\n", target_device_name);
        return EXIT_FAILURE;
    }

    // Create a compute context
    //
    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    if (!context) {
        printf("Error: Failed to create a compute context!\n");
        printf("Test failed\n");
        return EXIT_FAILURE;
    }

    cl_int status;

    // Create Program Objects
    // Load binary from disk
    char *xclbin = argv[1];

    // Read xclbin and create program
    printf("INFO: loading xclbin %s\n", xclbin);
    std::vector<unsigned char> binary = readBinary(xclbin);
    size_t binary_size = binary.size();
    const unsigned char *kernelbinary = binary.data();

    // Create the compute program from offline
    cl_program program = clCreateProgramWithBinary(context, 1, &device_id, &binary_size,
                                    &kernelbinary, &status, &err);

    if ((!program) || (err!=CL_SUCCESS)) {
        printf("Error: Failed to create compute program from binary %d!\n", err);
        printf("Test failed\n");
        return EXIT_FAILURE;
    }

    cl_kernel drm_ctrl_kernel = clCreateKernel(program, "kernel_drm_controller", &err);
    if (!drm_ctrl_kernel || err != CL_SUCCESS) {
        printf("Error: Failed to create compute kernel_drm_controller!\n");
        printf("Test failed\n");
        return EXIT_FAILURE;
    }

    cl_kernel input_kernel = clCreateKernel(program, "krnl_input_stage_rtl", &err);
    if (!input_kernel || err != CL_SUCCESS) {
        printf("Error: Failed to create compute krnl_input_stage_rtl!\n");
        printf("Test failed\n");
        return EXIT_FAILURE;
    }

    cl_kernel adder_kernel = clCreateKernel(program, "krnl_adder_stage_rtl", &err);
    if (!adder_kernel || err != CL_SUCCESS) {
        printf("Error: Failed to create compute krnl_adder_stage_rtl!\n");
        printf("Test failed\n");
        return EXIT_FAILURE;
    }

    cl_kernel output_kernel = clCreateKernel(program, "krnl_output_stage_rtl", &err);
    if (!input_kernel || err != CL_SUCCESS) {
        printf("Error: Failed to create compute krnl_output_stage_rtl!\n");
        printf("Test failed\n");
        return EXIT_FAILURE;
    }

    xclbin_uuid(kernelbinary, xclbinId);

    handle = xclOpen(0, nullptr, XCL_INFO);

    // Get kernel_drm_controller info

    // checking cu based address for testing purpose: not needed
    size_t drm_cuaddr;
    xclGetComputeUnitInfo(drm_ctrl_kernel,0,XCL_COMPUTE_UNIT_BASE_ADDRESS,sizeof(drm_cuaddr),&drm_cuaddr,nullptr);
    printf("\n DRM Ctrl CU addr = %zx",drm_cuaddr);

    cl_uint drm_cuidx = 0;
    xclGetComputeUnitInfo(drm_ctrl_kernel,0,XCL_COMPUTE_UNIT_INDEX,sizeof(drm_cuidx),&drm_cuidx,nullptr);
    printf("\n DRM Ctrl CU index = %d\n\n",drm_cuidx);

    // Get krnl_input_stage_rtl info

    // checking cu based address for testing purpose: not needed
    size_t in_cuaddr;
    xclGetComputeUnitInfo(input_kernel,0,XCL_COMPUTE_UNIT_BASE_ADDRESS,sizeof(in_cuaddr),&in_cuaddr,nullptr);
    printf("\n Input CU addr = %zx",in_cuaddr);
    // checked

    cl_uint in_cuidx = 0;
    xclGetComputeUnitInfo(input_kernel,0,XCL_COMPUTE_UNIT_INDEX,sizeof(in_cuidx),&in_cuidx,nullptr);
    printf("\n Input CU index = %d\n\n",in_cuidx);

    // Get krnl_adder_stage_rtl info

    // checking cu based address for testing purpose: not needed
    size_t add_cuaddr;
    xclGetComputeUnitInfo(adder_kernel,0,XCL_COMPUTE_UNIT_BASE_ADDRESS,sizeof(add_cuaddr),&add_cuaddr,nullptr);
    printf("\n Adder CU addr = %zx",add_cuaddr);
    // checked

    cl_uint add_cuidx = 0;
    xclGetComputeUnitInfo(adder_kernel,0,XCL_COMPUTE_UNIT_INDEX,sizeof(add_cuidx),&add_cuidx,nullptr);
    printf("\n Adder CU index = %d\n\n",add_cuidx);

    // Get krnl_output_stage_rtl info

    // checking cu based address for testing purpose: not needed
    size_t out_cuaddr;
    xclGetComputeUnitInfo(output_kernel,0,XCL_COMPUTE_UNIT_BASE_ADDRESS,sizeof(out_cuaddr),&out_cuaddr,nullptr);
    printf("\n Output CU addr = %zx",out_cuaddr);
    // checked

    cl_uint out_cuidx = 0;
    xclGetComputeUnitInfo(output_kernel,0,XCL_COMPUTE_UNIT_INDEX,sizeof(out_cuidx),&out_cuidx,nullptr);
    printf("\n Output CU index = %d\n\n",out_cuidx);

    uint32_t reg;

    // Get DRM Ctrl version
    read_register(drm_cuidx, 0x70, &reg);
    cout << "DRM Controller version: " << hex << reg << dec << endl;

    // Check DRM Activator existance
    read_register(add_cuidx, 0x40, &reg);
    cout << "DRM Activator existance: " << hex << reg << dec << endl;
    if (reg != 0x600DC0DE) {
        cout << "Error: DRM Activator existance should be 0x600DC0DE" << endl;
        return -1;
    }

    // Check DRM Activator status
    read_register(add_cuidx, 0x38, &reg);
    if (reg != 0) {
        cout << "Error: DRM Activator status should be 0, not " << reg << endl;
        return -1;
    }
    cout << "DRM Activator is locked" << endl;

    //ACCELIZE DRMLIB CODE AREA START
    DrmManager *pDrmManager = NULL;

    if (DRM_OK != DrmManager_alloc(&pDrmManager,
            "conf.json",
            "cred.json",
            drm_read_callback, drm_write_callback, drm_error_callback,
            &drm_cuidx)) {
        printf("Error allocating DRM Manager object: %s", pDrmManager->error_message);
        return -1;
    }
    cout << "[DRMLIB] Allocated" << endl;

    if (DRM_OK != DrmManager_activate(pDrmManager, false)) {
        printf("Error activating DRM Manager object: %s", pDrmManager->error_message);
        DrmManager_free(&pDrmManager);
        return -1;
    }
    cout << "[DRMLIB] Design unlocked" << endl;

    // Check a DRM Activator is detected
    read_register(add_cuidx, 0x40, &reg);
    if (reg != 0x600DC0DE) {
        cout << "Error: No DRM Activator is detected" << endl;
        DrmManager_free(&pDrmManager);
        return -1;
    }
    cout << "[DRMLIB] a DRM Activator is detected" << endl;

    // Check DRM Activator status
    read_register(add_cuidx, 0x38, &reg);
    if (reg != 3) {
        cout << "Error: DRM Activator status should be 3, not " << reg << endl;
        DrmManager_free(&pDrmManager);
        return -1;
    }
    cout << "[DRMLIB] Design unlocked" << endl;
    //ACCELIZE DRMLIB CODE AREA STOP

    // Perform some processing with the kernels

    // Allocate buffers
    int* source_input;
    int* source_hw_results;
    int* source_sw_results;

    // Aligning memory in 4K boundary
    err  = posix_memalign((void**)&source_input,4096,MAX_LENGTH*sizeof(int));
    err |= posix_memalign((void**)&source_hw_results,4096,MAX_LENGTH*sizeof(int));
    err |= posix_memalign((void**)&source_sw_results,4096,MAX_LENGTH*sizeof(int));
    if (err) {
        cout << "Fatal Error calling posix_memalign" << endl;
        exit(EXIT_FAILURE);
    }

    // Fill the buffers
    for (int i = 0; i < DATA_SIZE; i++) {
        source_input[i] = i;
        source_sw_results[i] = i + INCR_VALUE;
        source_hw_results[i] = 0;
    }

    auto vector_size_bytes = sizeof(int) * DATA_SIZE;

    cl_mem buffer_input = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                vector_size_bytes, source_input, NULL);

    cl_mem buffer_output = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
                vector_size_bytes, source_hw_results, NULL);

    // Set kernel arguments
    int size = DATA_SIZE;
    int inc = INCR_VALUE;
    clSetKernelArg(input_kernel, 0, sizeof(cl_mem), &buffer_input);
    clSetKernelArg(input_kernel, 1, sizeof(int), &size);
    clSetKernelArg(adder_kernel, 0, sizeof(int), &inc);
    clSetKernelArg(adder_kernel, 1, sizeof(int), &size);
    clSetKernelArg(output_kernel, 0, sizeof(cl_mem), &buffer_output);
    clSetKernelArg(output_kernel, 1, sizeof(int), &size);

    // Create a command queue
    cl_command_queue commands = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err);
    if (err || !commands) {
        printf("Error: Failed to create a command commands!\n");
        printf("Error: code %i\n",err);
        printf("Test failed\n");
        return EXIT_FAILURE;
    }

    // Copy input data to device global memory
    cl_event write_event;
    if (clEnqueueMigrateMemObjects(commands, 1, &buffer_input, 0, 0, nullptr, &write_event)) {
        cout << "Fatal Error calling clEnqueueMigrateMemObjects" << endl;
        exit(EXIT_FAILURE);
    }

    // Launch the kernels
    cl_event evts[1] = {write_event};
    err  = clEnqueueTask(commands, input_kernel, 1, evts, nullptr);
    err |= clEnqueueTask(commands, adder_kernel, 1, evts, nullptr);
    err |= clEnqueueTask(commands, output_kernel, 1, evts, nullptr);
    if (err) {
        cout << "Fatal Error calling clEnqueueTask" << endl;
        exit(EXIT_FAILURE);
    }

    // Wait for all kernels to finish their operations
    if (clFinish(commands)) {
        cout << "Fatal Error calling clFinish" << endl;
        exit(EXIT_FAILURE);
    }

    // Copy Result from Device Global Memory to Host Local Memory
    if (clEnqueueMigrateMemObjects(commands, 1, &buffer_output, CL_MIGRATE_MEM_OBJECT_HOST, 0, nullptr, nullptr)) {
        cout << "Fatal Error calling clEnqueueMigrateMemObjects" << endl;
        exit(EXIT_FAILURE);
    }
    // Wait for all kernels to finish their operations
    if (clFinish(commands)) {
        cout << "Fatal Error calling clFinish" << endl;
        exit(EXIT_FAILURE);
    }

    // Compare the results of the Device to the simulation
    int mismatch = 0;
    for (int i = 0; i < DATA_SIZE; i++) {
        if (source_hw_results[i] != source_sw_results[i]) {
            std::cout << "Error: Result mismatch" << std::endl;
            std::cout << "i = " << i << " CPU result = " << source_sw_results[i]
                      << " Device result = " << source_hw_results[i]
                      << std::endl;
            mismatch ++;
            if (mismatch > 5) break;
        }
    }

    if (DRM_OK != DrmManager_deactivate(pDrmManager, false))
        cout << "Error deactivating DRM Manager object: " << pDrmManager->error_message << endl;
    else
        cout << "[DRMLIB] Design locked" << endl;

    if (DRM_OK != DrmManager_free(&pDrmManager))
        cout << "Error deallocating DRM Manager object: " << pDrmManager->error_message << endl;
    //ACCELIZE DRMLIB CODE AREA STOP

/*
    // DOES NOT WORK BECAUSE THE ADDER KERNEL CANNOT BE ACCESSED ANYMORE BUT I DON'T KNOW WHY

    // Check DRM Activator status
    read_register(add_cuidx, 0x38, &reg);
    if (reg != 0) {
        cout << "Error: DRM Activator status should be 0, not " << reg << endl;
        DrmManager_free(&pDrmManager);
        return -1;
    }
  */

    clReleaseMemObject(buffer_input);
    clReleaseMemObject(buffer_output);
    free(source_sw_results);

    clReleaseKernel(drm_ctrl_kernel);
    clReleaseKernel(input_kernel);
    clReleaseKernel(adder_kernel);
    clReleaseKernel(output_kernel);
    clReleaseCommandQueue(commands);
    clReleaseContext(context);
    clReleaseProgram(program);

    cout << "TEST " << (mismatch ? "FAILED" : "PASSED") << endl;
    return (mismatch ? EXIT_FAILURE : EXIT_SUCCESS);
}
