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

#ifndef SOA_VERSUS_AOS_H_
#define SOA_VERSUS_AOS_H_

/**
* Header Files
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <SDKCommon.hpp>
#include <SDKApplication.hpp>
#include <SDKFile.hpp>
#include <vector>

#define GROUP_SIZE 256
#define TREE_SIZE 1024 * 4
#define TREE_NUMBER 1024 * 4

/**
* SoAvsAoS
* Class to show the difference in performance
* between Structure of Arrays (SOA) and 
* Array of Structures (AOS) on GPUs.
* Derived from SDKSample base class
*/

class SoAvsAoS : public SDKSample
{
	cl_double setupTime;            /**< time taken to setup OpenCL resources and building kernel */
	cl_double kernelTime;           /**< time taken to run kernel and read result back */

	cl_context context;             /**< CL context */
	cl_device_id *devices;          /**< CL device list */
	cl_mem inputBuffer;             /**< CL memory buffer */
	cl_mem outputBuffer;             /**< CL memory buffer */
	cl_command_queue commandQueue;  /**< CL command queue */
	cl_program program;             /**< CL program  */
	cl_kernel kernel[2];               /**< CL kernel */

	size_t groupSize;               /**< Work-group size */
	int iterations;                 /**< Number of iterations for kernel execution*/
	streamsdk::SDKDeviceInfo deviceInfo;            /**< Structure to store device information*/
	streamsdk::KernelWorkGroupInfo kernelInfo;      /**< Structure to store kernel related info */
	size_t inputSize;               /** Input size */
	size_t outputSize;              /** Output size */
	int elements;                   /** Number of elements */
	int treeNumber;					    /** The number of trees */
	int treeSize;					   /** The size of the trees */
	double aos_time;
	double soa_time;
	double aos_perf;
	double soa_perf;
	bool passed;
	double totalTime;
	int *CPUResult;
	/** Private helper methods to setup the input/output and run the test. */
	void fillData(cl_mem buffer);



public:
	/** 
	* Constructor 
	* Initialize member variables
	* @param name name of sample (string)
	*/
	explicit SoAvsAoS(std::string name)
		: SDKSample(name), 
		devices(NULL),
		inputBuffer(NULL),
		outputBuffer(NULL)
	{
		groupSize = GROUP_SIZE;
		iterations = 2;
		passed = true;
		totalTime = 0.0;
		treeSize = TREE_SIZE;
		treeNumber = TREE_NUMBER;
	}

	/** 
	* Constructor 
	* Initialize member variables
	* @param name name of sample (const char*)
	*/
	explicit SoAvsAoS(const char* name)
		: SDKSample(name), 
		devices(NULL),
		inputBuffer(NULL),
		outputBuffer(NULL)
	{
		groupSize = GROUP_SIZE;
		iterations = 2;
		passed = true;
		totalTime = 0.0;
		treeSize = TREE_SIZE;
		treeNumber = TREE_NUMBER;
	}

	~SoAvsAoS()
	{

	}
	
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
	* Override from SDKSample, Generate binary image of given kernel 
	* and exit application
	* @return SDK_SUCCESS on success and SDK_FAILURE on failure
	*/
	int genBinaryImage();

	/**
	* Override from SDKSample
	* Run OpenCL Reduction
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
	*check user's input
	*/
	int checkInput();

	/**
	*make the option string for build openCL program
	*/
	std::string makeOption();

	/**
	*calculate result with CPU
	*/
	void calResultByCPU();

	bool checkResult(int *);
};

#endif //SOA_VERSUS_AOS_H

