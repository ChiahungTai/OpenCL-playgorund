/**********************************************************************
Copyright ?012 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

?Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
?Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

#ifndef SIMPLE_DX11_H_
#define SIMPLE_DX11_H_

#include <Windows.h>
#include <mmsystem.h>

#pragma warning(disable : 4995) // disable deprecated warning 
#pragma warning(disable : 4996) // disable deprecated warning 
#include <strsafe.h>

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define D3D11_NO_HELPERS
#define D3D10_NO_HELPERS

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif


#include <cl\cl_d3d11.h>
#include <CL/cl_ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <SDKCommon.hpp>
#include <SDKApplication.hpp>
#include <SDKCommandArgs.hpp>
#include <SDKFile.hpp>
#include <SDKBitMap.hpp>

#define GROUP_SIZE 256
#define WINDOW_WIDTH 512
#define WINDOW_HEIGHT 512

#define WINDOW_CLASS_NAME   L"OpenCL DX11-Interop"
#define CAPTION_NAME        L"OpenCL SimpleDX11"

#define INIT_CL_EXT_FCN_PTR(name) \
    if(!pfn_##name) { \
    pfn_##name = (PFN_##name) \
    clGetExtensionFunctionAddress(#name); \
    if(!pfn_##name) { \
    std::cout << "Cannot get pointer to ext. fcn. " #name << std::endl; \
    return SDK_FAILURE; \
    } \
    }

typedef CL_API_ENTRY cl_mem (CL_API_CALL *PFN_clCreateFromD3D11BufferKHR)(
    cl_context context, cl_mem_flags flags, ID3D11Buffer*  buffer,
    cl_int* errcode_ret);

typedef CL_API_ENTRY cl_int (CL_API_CALL *PFN_clEnqueueAcquireD3D11ObjectsKHR)(
    cl_command_queue command_queue, cl_uint num_objects,
    const cl_mem* mem_objects, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event);

typedef CL_API_ENTRY cl_int (CL_API_CALL *PFN_clEnqueueReleaseD3D11ObjectsKHR)(
    cl_command_queue command_queue, cl_uint num_objects,
    const cl_mem* mem_objects,  cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event);

typedef CL_API_ENTRY cl_int (CL_API_CALL *PFN_clGetDeviceIDsFromD3D11KHR)(
    cl_platform_id platform,
    cl_d3d11_device_source_khr d3d_device_source,
    void *d3d_object,
    cl_d3d11_device_set_khr d3d_device_set,
    cl_uint num_entries,
    cl_device_id *devices,
    cl_uint *num_devices);


/**
* SimpleDX11 
* Class implements OpenCL  SimpleDX11 sample
* Derived from SDKSample base class
*/
class SimpleDX11 : public SDKSample
{

    cl_uint meshWidth;                  /**< mesh width */
    cl_uint meshHeight;                 /**< mesh height */
    cl_float* posFromCl;                /**< position vector */
    cl_float* refPos;                   /**< reference position vector */
    cl_context context;                 /**< CL context */
    cl_device_id *devices;              /**< CL device list */
    cl_command_queue commandQueue;      /**< CL command queue */
    cl_mem posBuf;                      /**< CL Buffer for position vector */
    cl_program program;                 /**< CL program */
    cl_kernel kernel;                   /**< CL kernel */
    size_t groupSize;                   /**< Work-Group size */
    cl_device_type dType;               /**< Device type */
    cl_device_id interopDeviceId;       /**< Interoperability device ID*/
    streamsdk::SDKDeviceInfo deviceInfo;                    /**< Structure to store device information*/
    streamsdk::KernelWorkGroupInfo kernelInfo;              /**< Structure to store kernel related info */

public:
    /** 
    * Constructor 
    * Initialize member variables
    * @param name name of sample (string)
    */
    explicit SimpleDX11(std::string name)
        : SDKSample(name),
        meshWidth(WINDOW_WIDTH),
        meshHeight(WINDOW_HEIGHT),
        posFromCl(NULL),
        refPos(NULL),
        devices(NULL),
        groupSize(GROUP_SIZE),
        dType(CL_DEVICE_TYPE_GPU)
    {
    }

    /** 
    * Constructor 
    * Initialize member variables
    * @param name name of sample (const char*)
    */
    explicit SimpleDX11(const char* name)
        : SDKSample(name),
        meshWidth(WINDOW_WIDTH),
        meshHeight(WINDOW_HEIGHT),
        posFromCl(NULL),
        refPos(NULL),
        devices(NULL),
        groupSize(GROUP_SIZE),
        dType(CL_DEVICE_TYPE_GPU)
    {
    }

    ~SimpleDX11();

    /**
    * Allocate and initialize host memory array with random values
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int setupSimpleDX11();

    /**
     * Override from SDKSample, Generate binary image of given kernel 
     * and exit application
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int genBinaryImage();

    /**
    * OpenCL related initialisations. 
    * Set up Context, Device list, Command Queue, Memory buffers
    * Build CL kernel program executable
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int setupCL();

    /**
    * Set values for kernels' arguments
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int setupCLKernels();

    /**
    * Enqueue calls to the kernels
    * on to the command queue, wait till end of kernel execution.
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int executeKernel();

    /**
    * Reference CPU implementation of Binomial Option
    * for performance comparison
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    void SimpleDX11CPUReference();

    /**
    * Override from SDKSample. 
    * Initialize command line parser, add custom options
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int initialize();

    /**
    * Override from SDKSample, adjust width and height 
    * of execution domain, perform all sample setup
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int setup();

    /**
    * Override from SDKSample
    * Run OpenCL SimpleDX11
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int run();

    /**
    * Override from SDKSample
    * Cleanup memory allocations
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int cleanup();

    /**
     * verifyResults
     * Pure Virtual Function
     * Verifies the results obtained with reference implementation
     * @return 0 on success Positive if expected and Non-zero on failure
     */
    virtual int verifyResults();    

    /**
     * Initialize DX11 device and object
     * @param platform CL Platform
     * @param context CL Context
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int initializeDX11(cl_platform_id platform, cl_context &context);

    /**
     * Renders the sineware
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int render();

    /**
     * Get error code
     */
    const char* getOpenCLErrorCodeStr(int errorCode) const;
    
};

#endif // SIMPLE_DX11_H_
