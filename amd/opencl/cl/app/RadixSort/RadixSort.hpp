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


#ifndef RADIXSORT_H_
#define RADIXSORT_H_

/**
 * Heaer Files
 */

#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <SDKCommon.hpp>
#include <SDKApplication.hpp>
#include <SDKCommandArgs.hpp>
#include <SDKFile.hpp>

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define ELEMENT_COUNT (8192)
#define RADIX 8
#define RADICES (1 << RADIX)	//Values handeled by each work-item?
#define RADIX_MASK (RADICES - 1)
#define GROUP_SIZE 64
#define NUM_GROUPS (ELEMENT_COUNT / (GROUP_SIZE * RADICES))	
	

/**
* RadixSort 
* Class implements 256 RadixSort bin implementation 
* Derived from SDKSample base class
*/

class RadixSort : public SDKSample
{
    cl_int  elementCount;           /**< Size of RadixSort bin */
    cl_int  groupSize;              /**< Number of threads in a Group */
    cl_int  numGroups;              /**< Number of groups */
    cl_ulong neededLocalMemory;     /**< Local memory need by application which set from host */
    cl_bool byteRWSupport;          /**< Flag for byte-addressable store */
    int iterations;                 /**< Number of iterations for kernel execution */

    //Host buffers
    cl_uint *unsortedData;          /**< unsorted elements */
    cl_uint *dSortedData;           /**< device sorted elements */
    cl_uint *hSortedData;           /**< host sorted elements */
    cl_uint *histogramBins;         /**< Histogram output */
	

    //Device buffers
    cl_mem  unsortedDataBuf;        /**< CL memory buffer for unsored data */
    cl_mem  histogramBinsBuf;//in       /**< CL memory buffer for prescaneduckets */
    cl_mem  scanedHistogramBinsBuf;//out     /**< CL memory buffer for prescaneduckets */
    cl_mem  sortedDataBuf;          /**< CL memory buffer for sorted data */
	//add four buffers
	cl_mem  sumBufferin;
	cl_mem  sumBufferout;
	cl_mem  summaryBUfferin;
	cl_mem  summaryBUfferout;
	//cl_mem midoutBuffer;
	//end
	

    cl_double kernelTime;           /**< Time for host implementation */
    cl_double setupTime;            /**< Time for host implementation */

    //CL objects
    cl_context context;             /**< CL context */
    cl_device_id *devices;          /**< CL device list */
    cl_command_queue commandQueue;  /**< CL command queue */
    cl_program program;             /**< CL program  */
    cl_kernel histogramKernel;      /**< CL kernel for histogram */
    cl_kernel permuteKernel;        /**< CL kernel for permute */
	//add four kernels
	cl_kernel scanArrayKerneldim2;
    cl_kernel scanArrayKerneldim1;
    cl_kernel prefixSumKernel;
    cl_kernel blockAdditionKernel;
	cl_kernel FixOffsetkernel;
	//end
    streamsdk::SDKDeviceInfo deviceInfo;/**< Structure to store device information*/
    streamsdk::KernelWorkGroupInfo kernelInfoHistogram,kernelInfoPermute;/**< Structure to store kernel related info */


public:
    /** 
    * Constructor 
    * Initialize member variables
    * @param name name of sample (string)
    */
    RadixSort(std::string name)
        : SDKSample(name),
        elementCount(ELEMENT_COUNT),
        groupSize(GROUP_SIZE),
        numGroups(NUM_GROUPS),
        kernelTime(0),
        setupTime(0),
        unsortedData(NULL),
        dSortedData(NULL),
        hSortedData(NULL),
        histogramBins(NULL),
        devices(NULL),
        byteRWSupport(true),
        iterations(1)
    {
    }

    /** 
    * Constructor 
    * Initialize member variables
    * @param name name of sample (const char*)
    */
    RadixSort(const char* name)
        : SDKSample(name),
        elementCount(ELEMENT_COUNT),
        groupSize(GROUP_SIZE),
        numGroups(NUM_GROUPS),
        kernelTime(0),
        setupTime(0),
        unsortedData(NULL),
        dSortedData(NULL),
        hSortedData(NULL),
        histogramBins(NULL),
        devices(NULL),
        byteRWSupport(true),
        iterations(1)
    {
    }

    ~RadixSort()
    {}

    /**
    * Allocate and initialize required host memory with appropriate values
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int setupRadixSort();

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
    *  Host Radix sort 
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int hostRadixSort();

    /**
    *  Runs Histogram Kernel
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int runHistogramKernel(int bits);

    /**
    *  Runs Permute Kernel
    * @return SDK_SUCCESS on success and SDK_FAILURE on failure
    */
    int runPermuteKernel(int bits);

	int runStaticKernel();

	int runFixOffsetKernel();

	
};



#endif  //RADIXSORT_H_
