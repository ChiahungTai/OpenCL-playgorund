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


#ifndef BINARYSEARCH_H_
#define BINARYSEARCH_H_

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <SDKCommon.hpp>
#include <SDKApplication.hpp>
#include <SDKCommandArgs.hpp>
#include <SDKFile.hpp>



/**
 * BinarySearch 
 * Class implements OpenCL Binary Search sample
 * Derived from SDKSample base class
 */

class BinarySearch : public SDKSample
{
    cl_uint                  seed;      /**< Seed value for random number generation */
    cl_double           setupTime;      /**< Time for setting up Opencl */
    cl_double     totalKernelTime;      /**< Time for kernel execution */
    cl_double    totalProgramTime;      /**< Time for program execution */
    cl_double referenceKernelTime;      /**< Time for reference implementation */
    cl_uint                findMe;      /**< Search for this number in the input array */
    cl_uint                *input;      /**< Input array */
    cl_uint                 length;     /**< Length of the input array */
    cl_uint               *output;      /**< Output array */
    cl_uint    *verificationInput;      /**< Input array for reference implementation */
    cl_context            context;      /**< CL context */
    cl_device_id         *devices;      /**< CL device list */
    cl_mem            inputBuffer;      /**< CL memory buffer */
    cl_mem           outputBuffer;      /**< CL output memory buffer */
    cl_command_queue commandQueue;      /**< CL command queue */
    cl_program            program;      /**< CL program  */
    cl_kernel              kernel;      /**< CL kernel */
    cl_uint       numSubdivisions;      /**< number of subdivisions to do N'ary Search */
	cl_uint  globalLowerBound;
    cl_uint  globalUpperBound;
    int                iterations;      /**< Number of iterations for kernel execution */
    streamsdk::KernelWorkGroupInfo kernelInfo;      /**< Structure to store kernel related info */

public:
    /** 
     * Constructor 
     * Initialize member variables
     * @param name name of sample (string)
     */
    BinarySearch(std::string name)
        : SDKSample(name)    {
            seed = 123;
            input = NULL;
            output = NULL;
            verificationInput = NULL;
            findMe = 5;
            numSubdivisions = 8;        /**< 8-ary Search by Default */
            length = 512;
            setupTime = 0;
            totalKernelTime = 0;
            iterations = 1;
			globalLowerBound = 0;
			globalUpperBound = 0;
        }

    /** 
     * Constructor 
     * Initialize member variables
     * @param name name of sample (const char*)
     */
    BinarySearch(const char* name)
        : SDKSample(name)    {
            seed = 123;
            input = NULL;
            output = NULL;
            verificationInput = NULL;
            findMe = 5;
            numSubdivisions = 8;      /**< Binary Search by Default */
            length = 512;
            setupTime = 0;
            totalKernelTime = 0;
            iterations = 1;
			globalLowerBound =0;
			globalUpperBound = 0;
        }

    /**
     * Allocate and initialize host memory array with random values
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int setupBinarySearch();

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
     * Set values for kernels' arguments, enqueue calls to the kernels
     * on to the command queue, wait till end of kernel execution.
     * Get kernel start and end time if timing is enabled
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int runCLKernels();

    /**
    * CPU verification for the BinarySearch algorithm
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int binarySearchCPUReference();

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
     * Override from SDKSample, adjust width and height 
     * of execution domain, perform all sample setup
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int setup();

    /**
     * Override from SDKSample
     * Run OpenCL BinarySearch 
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
