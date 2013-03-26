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


#ifndef BLACK_SCHOLES_H_
#define BLACK_SCHOLES_H_

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <SDKCommon.hpp>
#include <SDKApplication.hpp>
#include <SDKFile.hpp>


#define GROUP_SIZE 256
/**
 * BlackScholesDP 
 * Class implements Black-Scholes implementation for European Options
 * Derived from SDKSample base class
 */

class BlackScholesDP : public SDKSample
{
    cl_int samples;                 /**< Number of samples */
    cl_int width;                   /**< width of the execution domain */
    cl_int height;                  /**< height of the execution domain */
    cl_double *randArray;            /**< Array of random numbers */
    cl_double setupTime;            /**< time taken to setup OpenCL resources and building kernel */
    cl_double kernelTime;           /**< time taken to run kernel and read result back */
    cl_double *deviceCallPrice;      /**< Array of call price values */
    cl_double *devicePutPrice;       /**< Array of put price values */
    cl_double *hostCallPrice;        /**< Array of call price values */
    cl_double *hostPutPrice;         /**< Array of put price values */
    cl_context context;             /**< CL context */
    cl_device_id *devices;          /**< CL device list */
    cl_mem randBuf;                 /**< CL memory buffer for randArray */
    cl_mem callPriceBuf;            /**< CL memory buffer for callPrice */
    cl_mem putPriceBuf;             /**< CL memroy buffer for putPrice */
    cl_command_queue commandQueue;  /**< CL command queue */
    cl_program program;             /**< CL program  */
    cl_kernel kernel;               /**< CL kernel */
    size_t blockSizeX;              /**< block size in x-direction*/
    size_t blockSizeY;              /**< block size in y-direction*/
    int iterations;
    streamsdk::SDKDeviceInfo deviceInfo;     /**< Structure to store device information*/
    streamsdk::KernelWorkGroupInfo kernelInfo; /**< KernelWorkGroupInfo class Object */

public:
    /** 
     * Constructor 
     * Initialize member variables
     * @param name name of sample (string)
     */
    BlackScholesDP(std::string name)
        : SDKSample(name),
        samples(256 * 256 * 4),
        blockSizeX(1),
        blockSizeY(1),
        setupTime(0),
        kernelTime(0),
        randArray(NULL),
        deviceCallPrice(NULL),
        devicePutPrice(NULL),
        hostCallPrice(NULL),
        hostPutPrice(NULL),
        devices(NULL),
        iterations(1)
    {
        width = 64; 
        height = 64;
    }

    /** 
     * Constructor 
     * Initialize member variables
     * @param name name of sample (const char*)
     */
    BlackScholesDP(const char* name)
        : SDKSample(name),
        samples(256 * 256 * 4),
        blockSizeX(1),
        blockSizeY(1),
        setupTime(0),
        kernelTime(0),
        randArray(NULL),
        deviceCallPrice(NULL),
        devicePutPrice(NULL),
        hostCallPrice(NULL),
        hostPutPrice(NULL),
        devices(NULL),
        iterations(1)
    {
        width = 64; 
        height = 64;
    }

    ~BlackScholesDP()
    {
        if(randArray) 
        {
            #ifdef _WIN32
                ALIGNED_FREE(randArray);
            #else
                FREE(randArray);
            #endif

           FREE(deviceCallPrice);
           FREE(devicePutPrice);
           FREE(hostCallPrice);
           FREE(hostPutPrice);
           FREE(devices);
        }
    }

    /**
     * Allocate and initialize required host memory with appropriate values
     * @return SDK_SUCCESS on success and SDK_FAILURE on failure
     */
    int setupBlackScholesDP();

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
     * Run OpenCL Black-Scholes
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

private:
    
    /**
     *  Abromowitz Stegun approxmimation for PHI on the CPU(Cumulative Normal Distribution Function)
     */
    double phi(double X);

    /**
     *  CPU version of black scholes
     */
    void blackScholesDPCPU();

};
#endif