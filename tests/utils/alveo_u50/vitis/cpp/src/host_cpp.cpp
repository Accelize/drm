/*******************************************************************************
Description: Vitis DRM Controller Register access
*******************************************************************************/

/*
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
*/
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <uuid/uuid.h>
#include <vector>
#include "xclhal2.h"
#include "experimental/xclbin-util.h"

#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#include <CL/cl2.hpp>
#include <CL/cl_ext_xilinx.h>

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

#define OCL_CHECK(error,call)                                       \
    call;                                                           \
    if (error != CL_SUCCESS) {                                      \
      printf("%s:%d Error calling " #call ", error code is: %d\n",  \
              __FILE__,__LINE__, error);                            \
      exit(EXIT_FAILURE);                                           \
    } 
    
     
using namespace std;


xclDeviceHandle handle;
uuid_t xclbinId;


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


template <typename T>
struct aligned_allocator
{
  using value_type = T;
  T* allocate(std::size_t num)
  {
    void* ptr = nullptr;
    if (posix_memalign(&ptr,4096,num*sizeof(T)))
      throw std::bad_alloc();
    return reinterpret_cast<T*>(ptr);
  }
  void deallocate(T* p, std::size_t num)
  {
    free(p);
  }
};

std::vector<unsigned char> read_binary_file(const std::string &xclbin_file_name) {
    cout << "INFO: Reading " << xclbin_file_name << endl;

    if (access(xclbin_file_name.c_str(), R_OK) != 0) {
        printf("ERROR: %s xclbin not available please build\n",
               xclbin_file_name.c_str());
        exit(EXIT_FAILURE);
    }
    //Loading XCL Bin into char buffer
    cout << "Loading: '" << xclbin_file_name.c_str() << "'\n";
    std::ifstream bin_file(xclbin_file_name.c_str(), std::ifstream::binary);
    bin_file.seekg(0, bin_file.end);
    auto nb = bin_file.tellg();
    bin_file.seekg(0, bin_file.beg);
    std::vector<unsigned char> buf;
    buf.resize(nb);
    bin_file.read(reinterpret_cast<char *>(buf.data()), nb);
    return buf;
}

std::vector<cl::Device> get_devices() {
    size_t i;
    cl_int err;
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    cl::Platform platform;
    for (i = 0; i < platforms.size(); i++) {
        platform = platforms[i];
        std::string platformName = platform.getInfo<CL_PLATFORM_NAME>(&err);
        if (platformName == "Xilinx") {
            cout << "Found Platform" << endl;
            cout << "Platform Name: " << platformName.c_str() << endl;
            break;
        }
    }
    if (i == platforms.size()) {
        cout << "Error: Failed to find Xilinx platform" << endl;
        exit(EXIT_FAILURE);
    }
    //Getting ACCELERATOR Devices and selecting 1st such device
    std::vector<cl::Device> devices;
    err = platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices);
    return devices;
}

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


/**
 * Entry point
 */
int main(int argc, char **argv) {

    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <XCLBIN File>" << endl;
        return EXIT_FAILURE;
    }

    // OpenCL Setup
    // OpenCL objects
    cl::Device device;
    cl::Context context;
    cl::Program program;

    // Error Status variable
    cl_int err;

    // Create Program Objects
    // Load binary from disk
    string xclbin = string(argv[1]);
    cout << "INFO: loading xclbin " << xclbin << endl;

    // read_binary_file() is a utility API which will load the xclbin
    // and will return the pointer to file buffer.
    auto binary = read_binary_file(xclbin);
    cl::Program::Binaries bins{{binary.data(), binary.size()}};

    // get_devices() is a utility API which will find the xilinx
    // platforms and will return a list of devices connected to Xilinx platform
    auto devices = get_devices();
    int valid_device = 0;
    for (unsigned int i = 0; i < devices.size(); i++) {
        device = devices[i];
        // Creating Context and Command Queue for selected Device
        context = cl::Context(device, NULL, NULL, NULL, &err);
        cout << "Trying to program device[" << i
                  << "]: " << device.getInfo<CL_DEVICE_NAME>() << endl;
        program = cl::Program(context, {device}, bins, NULL, &err);
        if (err) {
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

    auto platform_id = device.getInfo<CL_DEVICE_PLATFORM>(&err);
    if (err) {
        printf("Error: Failed to get device info!\n");
        printf("Test failed\n");
        return EXIT_FAILURE;
    }

    xclbin_uuid(binary.data(), xclbinId);

    xclProbe();

    clGetDeviceInfo(device.get(),CL_DEVICE_HANDLE,sizeof(handle),&handle,nullptr);

    // Creating Kernels
    cl::Kernel drm_ctrl_kernel = cl::Kernel(program, "kernel_drm_controller", &err);
    if (err) {
        printf("Error: Failed to create compute kernel_drm_controller!\n");
        printf("Test failed\n");
        return EXIT_FAILURE;
    }

    cl::Kernel input_kernel = cl::Kernel(program, "krnl_input_stage_rtl", &err);
    if (err) {
        printf("Error: Failed to create compute krnl_input_stage_rtl!\n");
        printf("Test failed\n");
        return EXIT_FAILURE;
    }

    cl::Kernel adder_kernel = cl::Kernel(program, "krnl_adder_stage_rtl", &err);
    if (err) {
        printf("Error: Failed to create compute krnl_adder_stage_rtl!\n");
        printf("Test failed\n");
        return EXIT_FAILURE;
    }

    cl::Kernel output_kernel = cl::Kernel(program, "krnl_output_stage_rtl", &err);
    if (err) {
        printf("Error: Failed to create compute krnl_output_stage_rtl!\n");
        printf("Test failed\n");
        return EXIT_FAILURE;
    }

    // Get kernel_drm_controller info

    // checking cu based address for testing purpose: not needed
    size_t drm_cuaddr;
    xclGetComputeUnitInfo(drm_ctrl_kernel.get(),0,XCL_COMPUTE_UNIT_BASE_ADDRESS,sizeof(drm_cuaddr),&drm_cuaddr,nullptr);
    cout << endl << "DRM Ctrl CU addr = " << hex << drm_cuaddr << dec << endl;
    // checked
    cl_uint drm_cuidx;
    xclGetComputeUnitInfo(drm_ctrl_kernel.get(),0,XCL_COMPUTE_UNIT_INDEX,sizeof(drm_cuidx),&drm_cuidx,nullptr);
    cout << "DRM Ctrl CU index = " << drm_cuidx << endl;

    // Get krnl_input_stage_rtl info

    // checking cu based address for testing purpose: not needed
    size_t in_cuaddr;
    xclGetComputeUnitInfo(input_kernel.get(),0,XCL_COMPUTE_UNIT_BASE_ADDRESS,sizeof(in_cuaddr),&in_cuaddr,nullptr);
    cout << endl << "Input CU addr = " << hex << in_cuaddr << dec << endl;
    // checked
    cl_uint in_cuidx = 0;
    xclGetComputeUnitInfo(input_kernel.get(),0,XCL_COMPUTE_UNIT_INDEX,sizeof(in_cuidx),&in_cuidx,nullptr);
    cout << endl << "Input CU index = " << in_cuidx << endl;

    // Get krnl_adder_stage_rtl info

    // checking cu based address for testing purpose: not needed
    size_t add_cuaddr;
    xclGetComputeUnitInfo(adder_kernel.get(),0,XCL_COMPUTE_UNIT_BASE_ADDRESS,sizeof(add_cuaddr),&add_cuaddr,nullptr);
    cout << endl << "Adder CU addr = " << hex << add_cuaddr << dec << endl;
    // checked
    cl_uint add_cuidx = 0;
    xclGetComputeUnitInfo(adder_kernel.get(),0,XCL_COMPUTE_UNIT_INDEX,sizeof(add_cuidx),&add_cuidx,nullptr);
    cout << endl << "Adder CU index = " << add_cuidx << endl;

    // Get krnl_output_stage_rtl info

    // checking cu based address for testing purpose: not needed
    size_t out_cuaddr;
    xclGetComputeUnitInfo(output_kernel.get(),0,XCL_COMPUTE_UNIT_BASE_ADDRESS,sizeof(out_cuaddr),&out_cuaddr,nullptr);
    cout << endl << "Output CU addr = " << hex << out_cuaddr << dec << endl;
    // checked
    cl_uint out_cuidx = 0;
    xclGetComputeUnitInfo(output_kernel.get(),0,XCL_COMPUTE_UNIT_INDEX,sizeof(out_cuidx),&out_cuidx,nullptr);
    cout << endl << "Output CU index = " << out_cuidx << endl << endl;

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
/*
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
*/
    cout << "TEST " << (mismatch ? "FAILED" : "PASSED") << endl;
    return (mismatch ? EXIT_FAILURE : EXIT_SUCCESS);
}
