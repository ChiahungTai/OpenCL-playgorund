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


#ifndef MERSENENNETWISTER_H_
#define MERSENENNETWISTER_H_

/**
 * Headers Files
 */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <SDKCommon.hpp>
#include <SDKApplication.hpp>
#include <SDKFile.hpp>

#include <malloc.h>

#define GROUP_SIZE 256

/**
 * MersenneTwister 
 * Class implements SIMD-oriented Fast mersenne Twister and Box-Muller Transformation
 * Derived from SDKSample base class
 */

class MersenneTwister : public SDKSample
{

    cl_uint numRands;               /**< Number of random number to be generated*/
    cl_uint mulFactor;              /**< Number of generated random numbers for each seed */
    cl_uint *seeds;                 /**< Array of seeds */
    cl_float *deviceResult;         /**< Array of Generated random numbersby specified device */

    cl_double setupTime;            /**< time taken to setup OpenCL resources and building kernel */
    cl_double kernelTime;           /**< time taken to run kernel and read result back */

    cl_context context;             /**< CL context */
    cl_device_id *devices;          /**< CL device list */
    
    cl_mem seedsBuf;                /**< CL memory buffer for seeds */
    cl_mem resultBuf;               /**< CL memory buffer for random numbers */

    cl_command_queue commandQueue;  /**< CL command queue */
    cl_program program;             /**< CL program  */
    cl_kernel kernel;               /**< CL kernel */

    cl_int width;                   /**< width of the execution domain */
    cl_int height;                  /**< height of the execution domain */
    //size_t kernelWorkGroupSize;     /** Group Size returned by kernel */
    size_t blockSizeX;              /**< block size in x-direction*/
    size_t blockSizeY;              /**< block size in y-direction*/
    int iterations;                 /**< Number of iterations for kernel execution */
    streamsdk::SDKDeviceInfo deviceInfo;            /**< Structure to store device information*/
    streamsdk::KernelWorkGroupInfo kernelInfo;          /**< Structure to store kernel related info */

public:
    /** 
     * Constructor 
     * Initialize member variables
     * @param name name of sample (string)
     */
    MersenneTwister(std::string name)
        : SDKSample(name),
        blockSizeX(1),
        blockSizeY(1),
        setupTime(0),
        kernelTime(0)
    {
        numRands = 256 * 256 * 4;
        mulFactor = 2;
        seeds = NULL;
        deviceResult = NULL;
        devices = NULL;
        width = 64;
        height = 64;
        iterations = 1;
    }

    /** 
     * Constructor 
     * Initialize member variables
     * @param name name of sample (const char*)
     */
    MersenneTwister(const char* name)
        : SDKSample(name),
        blockSizeX(1),
        blockSizeY(1),
        setupTime(0),
        kernelTime(0)
    {

        numRands = 256 * 256 * 4;
        mulFactor = 2;
        seeds = NULL;
        deviceResult = NULL;
        devices = NULL;
        width = 64;
        height = 64;
        iterations = 1;
    }

    ~MersenneTwister()
    {
            FREE(deviceResult);

            if(seeds)
        {
            #ifdef _WIN32
                ALIGNED_FREE(seeds);
            #else
                FREE(seeds);
            #endif
            seeds = NULL;
        }

            FREE(devices);

    }

    /**
     * Allocate and initialize required host memory with necessary values
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int setupMersenneTwister();

    /**
     * Override from SDKSample, Generate binary image of given kernel 
     * and exit application
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int genBinaryImage();

    /**
     * OpenCL related initializations. 
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

    /**
     * Override from SDKSample. Print sample stats.
     */
    void printStats();

    /**
     * Override from SDKSample. Initialize 
     * command line parser, add custom options
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int initialize();

    /**
     * Validate command line options
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int validateCommandLineOptions();


    /**
     * Override from SDKSample, adjust width and height 
     * of execution domain, perform all sample setup
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int setup();

    /**
     * Override from SDKSample
     * Run OpenCL Bitonic Sort
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
