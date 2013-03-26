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


#ifndef PREFIXSUM_H_
#define PREFIXSUM_H_


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <SDKCommon.hpp>
#include <SDKApplication.hpp>
#include <SDKCommandArgs.hpp>
#include <SDKFile.hpp>

/**
 * PrefixSum 
 * Class implements OpenCL Prefix Sum sample
 * Derived from SDKSample base class
 */

class PrefixSum : public SDKSample
{
    cl_uint                  seed;      /**< Seed value for random number generation */
    cl_double           setupTime;      /**< Time for setting up OpenCL */
    cl_double     totalKernelTime;      /**< Time for kernel execution */
    cl_double    totalProgramTime;      /**< Time for program execution */
    cl_double referenceKernelTime;      /**< Time for reference implementation */
    cl_int                 length;      /**< length of the input array */
    cl_float               *input;      /**< Input array */
    cl_float              *output;      /**< Output Array */
    cl_float  *verificationOutput;      /**< Output array for reference implementation */
    cl_context            context;      /**< CL context */
    cl_device_id         *devices;      /**< CL device list */
    cl_mem            inputBuffer;      /**< CL memory buffer */
    cl_mem           outputBuffer;      /**< CL memory output Buffer */
    cl_command_queue commandQueue;      /**< CL command queue */
    cl_program            program;      /**< CL program  */
    cl_kernel              kernel;      /**< CL kernel */
    cl_ulong availableLocalMemory; 
    cl_ulong    neededLocalMemory;
    int                iterations;      /**< Number of iterations for kernel execution */
    streamsdk::SDKDeviceInfo deviceInfo;/**< Structure to store device information*/
    streamsdk::KernelWorkGroupInfo kernelInfo;/**< Structure to store kernel related info */

public:
    /** 
     * Constructor 
     * Initialize member variables
     * @param name name of sample (string)
     */
    PrefixSum(std::string name)
        : SDKSample(name)    {
            seed = 123;
            input = NULL;
            output = NULL;
            verificationOutput = NULL;
            length = 512;
            setupTime = 0;
            totalKernelTime = 0;
            iterations = 1;
        }

    /** 
     * Constructor 
     * Initialize member variables
     * @param name name of sample (const char*)
     */
    PrefixSum(const char* name)
        : SDKSample(name)    {
            seed = 123;
            input = NULL;
            output = NULL;
            verificationOutput = NULL;
            length = 512;
            setupTime = 0;
            totalKernelTime = 0;
            iterations = 1;
        }

    /**
     * Allocate and initialize host memory array with random values
     * @return SDK_SUCCESS on success and SDL_FAILURE on failure
     */
    int setupPrefixSum();

    /**
     * OpenCL related initialisations. 
     * Set up Context, Device list, Command Queue, Memory buffers
     * Build CL kernel program executable
     * @return SDK_SUCCESS on success and SDL_FAILURE on failure
     */
    int setupCL();

    /**
     * Set values for kernels' arguments, enqueue calls to the kernels
     * on to the command queue, wait till end of kernel execution.
     * Get kernel start and end time if timing is enabled
     * @return SDK_SUCCESS on success and SDL_FAILURE on failure
     */
    int runCLKernels();

    /**
     * Reference CPU implementation of Prefix Sum
     * @param output the array that stores the prefix sum
     * @param input the input array
     * @param length length of the input array
     */
    void prefixSumCPUReference(
    cl_float * output,
    cl_float * input,
    const cl_uint length);

    /**
     * Override from SDKSample. Print sample stats.
     */
    void printStats();

    /**
     * Override from SDKSample. Initialize 
     * command line parser, add custom options
     * @return SDK_SUCCESS on success and SDL_FAILURE on failure
     */
    int initialize();

    /**
     * Override from SDKSample, Generate binary image of given kernel 
     * and exit application
     * @return SDK_SUCCESS on success and SDL_FAILURE on failure
     */
    int genBinaryImage();

    /**
     * Override from SDKSample, adjust width and height 
     * of execution domain, perform all sample setup
     * @return SDK_SUCCESS on success and SDL_FAILURE on failure
     */
    int setup();

    /**
     * Override from SDKSample
     * Run OpenCL FastWalsh Transform 
     * @return SDK_SUCCESS on success and SDL_FAILURE on failure
     */
    int run();

    /**
     * Override from SDKSample
     * Cleanup memory allocations
     * @return SDK_SUCCESS on success and SDL_FAILURE on failure
     */
    int cleanup();

    /**
     * Override from SDKSample
     * Verify against reference implementation
     * @return SDK_SUCCESS on success and SDL_FAILURE on failure
     */
    int verifyResults();
};
#endif
