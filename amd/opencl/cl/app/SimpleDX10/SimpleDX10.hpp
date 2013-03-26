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


#ifndef SIMPLE_DX10_H_
#define SIMPLE_DX10_H_

#include <Windows.h>
#include <mmsystem.h>
#include <d3d10_1.h>
#include <d3d10.h>



#if ((_WIN32_WINNT < 0x0602) || (_MSC_VER < 1600))
// Application is targetting below Windows 8
#else
// Application is targetting Windows 8 or above
#define WIN8
#endif

#ifdef WIN8
#include <directXmath.h>
#else
#include <D3DX10.h>
#pragma comment(lib,"d3dx10.lib")
#endif
#include <D3D10effect.h>


#pragma warning( disable : 4995 ) // disable deprecated warning 
#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <CL/cl_d3d10.h>
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

#define WINDOW_CLASS_NAME   L"OpenCL DX10-Interop"
#define CAPTION_NAME        L"OpenCL SimpleDX10"
#ifdef WIN8
using namespace DirectX;
#endif
using namespace std;
/**
* SimpleDX10 
* Class implements OpenCL  SimpleDX10 sample
* Derived from SDKSample base class
*/
class SimpleDX10 : public SDKSample
{

    cl_uint meshWidth;                  /**< mesh width */
    cl_uint meshHeight;                 /**< mesh height */
    cl_float* pos;                      /**< position vector */
    cl_float* refPos;                   /**< reference position vector */
    cl_context context;                 /**< CL context */
    cl_device_id *devices;              /**< CL device list */
    cl_command_queue commandQueue;      /**< CL command queue */
    cl_mem posBuf;                      /**< CL Buffer for position vector */
    cl_program program;                 /**< CL program */
    cl_kernel kernel;                   /**< CL kernel */
    //size_t kernelWorkGroupSize;         /**< Group size returned by kernel */
    size_t groupSize;                   /**< Work-Group size */
    cl_device_type dType;               /**< Device type */
    cl_device_id interopDeviceId;       /**< Interoperability device ID*/
    streamsdk::SDKDeviceInfo deviceInfo;                    /**< Structure to store device information*/
    streamsdk::KernelWorkGroupInfo kernelInfo;              /**< Structure to store kernel related info */

private:


public:
    /** 
    * Constructor 
    * Initialize member variables
    * @param name name of sample (string)
    */
    explicit SimpleDX10(std::string name)
        : SDKSample(name),
        meshWidth(WINDOW_WIDTH),
        meshHeight(WINDOW_HEIGHT),
        pos(NULL),
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
    explicit SimpleDX10(const char* name)
        : SDKSample(name),
        meshWidth(WINDOW_WIDTH),
        meshHeight(WINDOW_HEIGHT),
        pos(NULL),
        refPos(NULL),
        devices(NULL),
        groupSize(GROUP_SIZE),
        dType(CL_DEVICE_TYPE_GPU)
    {
    }

    ~SimpleDX10();

    /**
    * Allocate and initialize host memory array with random values
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int setupSimpleDX10();

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
    void SimpleDX10CPUReference();

    /**
    * Override from SDKSample. Initialize 
    * command line parser, add custom options
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
    * Run OpenCL SimpleDX10
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
     * Override from SDKSample
     * Verify against reference implementation
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int verifyResults();

    /**
     * Initialize DX10 device and object
     * @param platform CL Platform
     * @param context CL Context
     * @interopDeevice 
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int initializeDX10(cl_platform_id platform, 
                    cl_context &context, 
                    cl_device_id &interopDevice);

    /**
     *  Loads given Texture.
     *  @texture texture object
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int loadTexture(ID3D10Texture2D** ppTex2D);

    /**
     * Renders the sineware
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int render();

    /**
     * Get error code
     */
    const char* getOpenCLErrorCodeStr(int errorCode) const;

    int checkVal(int input, 
                 int reference, 
                 std::string message) const;

    int getAdapterId(const char* devName);

};

#endif // SIMPLE_DX10_H_
