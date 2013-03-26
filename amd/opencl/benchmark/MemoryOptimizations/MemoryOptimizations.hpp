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

#ifndef MEMORY_OPTIMIZATIONS_H_
#define MEMORY_OPTIMIZATIONS_H_

#define GROUP_SIZE 256
#define NUM_KERNELS 9

//Header Files
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <SDKApplication.hpp>
#include <SDKCommandArgs.hpp>
#include <SDKCommon.hpp>
#include <SDKFile.hpp>

/**
 * MemoryOptimizations 
 * Class implements OpenCL Global Memory Optimizations sample
 * Derived from SDKSample base class
 */

class MemoryOptimizations : public SDKSample
{
    cl_double           setupTime;      /**< Time for setting up OpenCL */
    cl_double     totalKernelTime;      /**< Time for kernel execution */
    cl_double    totalProgramTime;      /**< Time for program execution */
    cl_uint                length;      /**< Length of the input data */
    cl_float              *input;       /**< Input array */
    cl_float              *output;      /**< Output Array */
    cl_context            context;      /**< CL context */
    cl_device_id         *devices;      /**< CL device list */
    cl_mem            inputBuffer;      /**< CL memory buffer*/
    cl_mem           outputBuffer;      /**< CL memory buffer*/
    cl_command_queue commandQueue;      /**< CL command queue */
    cl_program            program;      /**< CL program */
    cl_kernel kernel[NUM_KERNELS];      /**< CL kernel */
    size_t        globalThreads[2];      
    size_t         localThreads[2];       

    size_t       maxWorkGroupSize;      /**< Device Specific Information */
    size_t    kernelWorkGroupSize;      /**< Group Size returned by kernel */
    cl_uint         maxDimensions;
    size_t *     maxWorkItemSizes;
    cl_ulong     totalLocalMemory; 
    cl_ulong      usedLocalMemory; 
    cl_ulong availableLocalMemory; 
    cl_ulong    neededLocalMemory;
    streamsdk::SDKDeviceInfo deviceInfo;  /**< Structure to store device information*/
 
public:
    /** 
     * Constructor 
     * Initialize member variables
     * @param name name of sample (string)
     */
    MemoryOptimizations(std::string name): SDKSample(name)
    {
            input = NULL;
            output = NULL;
            setupTime = 0;
            totalKernelTime = 0;
            length = 1024 * 1024;
        }

    /** 
     * Constructor 
     * Initialize member variables
     * @param name name of sample (const char*)
     */
    MemoryOptimizations(const char* name): SDKSample(name)
    {
            input = NULL;
            output = NULL;
            setupTime = 0;
            totalKernelTime = 0;
            length = 1024 * 1024;
        }

    /**
     * Allocate and initialize host memory array with random values
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int setupMemoryOptimizations();

    /**
     * OpenCL related initialisations. 
     * Set up Context, Device list, Command Queue, Memory buffers
     * Build CL kernel program executable
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int setupCL();

    /**
     * Set values for kernels' arguments, enqueue calls to the kernels
     * on to the command queue, wait till end of kernel execution.
     * Get kernel start and end time if timing is enabled
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int runCLKernels();

    int copy(cl_kernel& kernel, int vectorSize);


	int LDSKernel(cl_kernel& kernel);

    /**
     * Override from SDKSample. Initialize 
     * command line parser, add custom options
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int initialize();

    /**
     * Override from SDKSample, Generate binary image of given kernel 
     * and exit application
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int genBinaryImage();

    /**
     * Override from SDKSample, adjust width and height 
     * of execution domain, perform all sample setup
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int setup();

    /**
     * Override from SDKSample
     * Run OpenCL Matrix Multiplication
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
};


#endif
