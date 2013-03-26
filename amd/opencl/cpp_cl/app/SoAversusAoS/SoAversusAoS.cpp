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


#include "SoAversusAoS.hpp"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define SNPRINTF sprintf_s
#else
#define SNPRINTF snprintf
#endif


static const float VALID_DELTA = 2;

int
SoAvsAoS::setupCL()
{
	cl_int status = CL_SUCCESS;

	cl_device_type dType;

	if(deviceType.compare("cpu") == 0)
		dType = CL_DEVICE_TYPE_CPU;
	else //deviceType = "gpu" 
	{
		dType = CL_DEVICE_TYPE_GPU;
		if(isThereGPU() == false)
		{
			std::cout << "GPU not found. Falling back to CPU device" << std::endl;
			dType = CL_DEVICE_TYPE_CPU;	
		}
	}

	/*
	* Have a look at the available platforms and pick either
	* the AMD one if available or a reasonable default.
	*/
	cl_platform_id platform = NULL;
	int retValue = sampleCommon->getPlatform(platform, platformId, isPlatformEnabled());
	CHECK_ERROR(retValue, SDK_SUCCESS, "sampleCommon::getPlatform() failed");

	// Display available devices.
	retValue = sampleCommon->displayDevices(platform, dType);
	CHECK_ERROR(retValue, SDK_SUCCESS, "sampleCommon::displayDevices() failed");

	// Creating Context for the found platform
	cl_context_properties cps[3] = 
	{
		CL_CONTEXT_PLATFORM, 
		(cl_context_properties)platform, 
		0
	};

	context = clCreateContextFromType(cps,
		dType,
		NULL,
		NULL,
		&status);
	CHECK_OPENCL_ERROR(status, "clCreateContextFromType failed.");

	// getting device on which to run the sample
	status = sampleCommon->getDevices(context, &devices, deviceId, isDeviceIdEnabled());
	CHECK_ERROR(status, SDK_SUCCESS, "sampleCommon::getDevices() failed");

	// Create command queue

	commandQueue = clCreateCommandQueue(context,
		devices[deviceId],
		0,
		&status);
	CHECK_OPENCL_ERROR(status, "clCreateCommandQueue failed.");

	//Set device info of given cl_device_id
	retValue = deviceInfo.setDeviceInfo(devices[deviceId]);
	CHECK_ERROR(retValue, SDK_SUCCESS, "SDKDeviceInfo::setDeviceInfo() failed");
	//create buffer
	inputSize = elements * sizeof(int);
	if(inputSize > deviceInfo.maxMemAllocSize)
	{  
		printf("inputSize > deviceInfo.maxMemAllocSize,continue ...\n" );
		return SDK_FAILURE;
	}
	inputBuffer = clCreateBuffer(context, 
		CL_MEM_READ_ONLY, 
		inputSize, 
		NULL, 
		&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer(inBuffer) failed");

	// Create the output
	outputSize = treeNumber * sizeof(int);
	if(outputSize > deviceInfo.maxMemAllocSize)
	{
		printf("outputSize > deviceInfo.maxMemAllocSize,continue ...\n" );
		return SDK_FAILURE;
	}
	outputBuffer = clCreateBuffer(context, 
		CL_MEM_WRITE_ONLY, 
		outputSize, 
		NULL, 
		&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer(outBuffer) failed");

	// create a CL program using the kernel source
	streamsdk::buildProgramData buildData;
	buildData.kernelName = std::string("SoAversusAoS_Kernels.cl");
	buildData.devices = devices;
	buildData.deviceId = deviceId;
	buildData.flagsStr = makeOption();
	if(isLoadBinaryEnabled())
		buildData.binaryName = std::string(loadBinary.c_str());

	if(isComplierFlagsSpecified())
		buildData.flagsFileName = std::string(flags.c_str());

	retValue = sampleCommon->buildOpenCLProgram(program, context, buildData);
	CHECK_ERROR(retValue, SDK_SUCCESS, "sampleCommon::buildOpenCLProgram() failed");

	//create kernel
	kernel[0] = clCreateKernel(program,"AoSKernel", &status);
	CHECK_OPENCL_ERROR(status, "clCreateKernel failed");
	status = clSetKernelArg(kernel[0], 0, sizeof(cl_mem), (void *) &inputBuffer);
	status = clSetKernelArg(kernel[0], 1, sizeof(cl_mem), (void *) &outputBuffer);
	status = clSetKernelArg(kernel[0], 2, sizeof(cl_uint), (void *)&treeSize);
	kernel[1] = clCreateKernel(program,"SoAKernel", &status);
	CHECK_OPENCL_ERROR(status, "clCreateKernel failed");
	status = clSetKernelArg(kernel[1], 0, sizeof(cl_mem), (void *) &inputBuffer);
	status = clSetKernelArg(kernel[1], 1, sizeof(cl_mem), (void *) &outputBuffer);
	status = clSetKernelArg(kernel[1], 2, sizeof(cl_uint), (void *)&treeSize);

	return SDK_SUCCESS;
}

int 
SoAvsAoS::initialize()
{
	// Call base class Initialize to get default configuration
	if (this->SDKSample::initialize() != SDK_SUCCESS)
		return SDK_FAILURE;

	// Now add customized options
	streamsdk::Option* iteration_option = new streamsdk::Option;
	CHECK_ALLOCATION(iteration_option, "Memory Allocation error.\n");

	iteration_option->_sVersion = "i";
	iteration_option->_lVersion = "iterations";
	iteration_option->_description = "Number of iterations to execute kernel";
	iteration_option->_type = streamsdk::CA_ARG_INT;
	iteration_option->_value = &iterations;

	sampleArgs->AddOption(iteration_option);
	delete iteration_option;

	streamsdk::Option* tree_size_option = new streamsdk::Option;
	CHECK_ALLOCATION(tree_size_option , "Memory Allocation error.\n");

	tree_size_option->_sVersion = "s";
	tree_size_option->_lVersion = "size";
	tree_size_option->_description = "the size of the trees";
	tree_size_option->_type = streamsdk::CA_ARG_INT;
	tree_size_option->_value = &treeSize;

	sampleArgs->AddOption(tree_size_option);
	delete tree_size_option;

	streamsdk::Option* tree_number_option = new streamsdk::Option;
	CHECK_ALLOCATION(tree_number_option, "Memory Allocation error.\n");

	tree_number_option->_sVersion = "n";
	tree_number_option->_lVersion = "number";
	tree_number_option->_description = "The number of trees";
	tree_number_option->_type = streamsdk::CA_ARG_INT;
	tree_number_option->_value = &treeNumber;

	sampleArgs->AddOption(tree_number_option);
	delete tree_number_option;


	return SDK_SUCCESS;
}

int 
SoAvsAoS::genBinaryImage()
{
	streamsdk::bifData binaryData;
	binaryData.kernelName = std::string("SoAversusAoS_Kernels.cl");
	binaryData.flagsStr = makeOption();
	if(isComplierFlagsSpecified())
		binaryData.flagsFileName = std::string(flags.c_str());

	binaryData.binaryName = std::string(dumpBinary.c_str());
	int status = sampleCommon->generateBinaryImage(binaryData);
	return status;
}

int 
SoAvsAoS::setup()
{
	if(iterations < 1)
	{
		std::cout<<"Error, iterations cannot be 0 or negative. Exiting..\n";
		exit(0);
	}
	//check input size
	elements = treeSize * treeNumber;
	if(checkInput() != SDK_SUCCESS)
	{
		return SDK_FAILURE;
	}
	int timer = sampleCommon->createTimer();
	sampleCommon->resetTimer(timer);
	sampleCommon->startTimer(timer);

	if(setupCL() != SDK_SUCCESS)
		return SDK_FAILURE;

	sampleCommon->stopTimer(timer);

	// Compute setup time
	setupTime = (double)(sampleCommon->readTimer(timer));

	return SDK_SUCCESS;
}


void 
SoAvsAoS::fillData(cl_mem buffer)
{
	cl_int status = CL_SUCCESS;

	int* data = (int *)clEnqueueMapBuffer(commandQueue, buffer, true, CL_MAP_WRITE, 0, outputSize, 0, NULL, NULL, &status);
	memset(data, 0, outputSize);
	status = clEnqueueUnmapMemObject(commandQueue, buffer, data, 0, NULL, NULL);
}


int 
SoAvsAoS::runCLKernels()
{
	cl_int status = CL_SUCCESS;
	int timer = sampleCommon->createTimer();
	size_t global_work_size[2] = {treeNumber, 1};
	size_t local_work_size[2] = {GROUP_SIZE, 1};
	if(local_work_size[0] > deviceInfo.maxWorkGroupSize)
	{
		local_work_size[0] = deviceInfo.maxWorkGroupSize;
	}
	//run aos kernel
	//initialize aos data
	int* data = (int *)clEnqueueMapBuffer(commandQueue,inputBuffer, true, CL_MAP_WRITE, 0, inputSize, 0, NULL, NULL, &status);
	for (int i = 0; i < treeNumber; i++)
	{
		for(int j = 0; j < treeSize; j++)
		{
			data[j + i* treeSize] = j + i* treeSize;
		}
	}
	status = clEnqueueUnmapMemObject(commandQueue, inputBuffer, data, 0, NULL, NULL);
	// Warm-up.
	status = clEnqueueNDRangeKernel(commandQueue,
		kernel[0], 2,
		NULL,
		(const size_t*)global_work_size,
		(const size_t*)local_work_size,
		0,
		NULL,
		NULL);
	CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed");
	clFinish(commandQueue);

	status = CL_SUCCESS;
	sampleCommon->resetTimer(timer);
	sampleCommon->startTimer(timer);
	for (int i = 0; i < iterations; i++)
	{
		status |= clEnqueueNDRangeKernel(commandQueue,
			kernel[0], 2,
			NULL,
			(const size_t*)global_work_size,
			(const size_t*)local_work_size,
			0,
			NULL,
			NULL);
		CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed");
		clFinish(commandQueue);
	}



	sampleCommon->stopTimer(timer);
	aos_time= sampleCommon->readTimer(timer);
	totalTime += aos_time;

	// Collect the performance statistics.
	aos_perf = ((double)inputSize*(double)(iterations)*(double)(1e-09)) / aos_time;
	if(verify)
	{
		int* data = (int*)clEnqueueMapBuffer(commandQueue, outputBuffer, true, CL_MAP_READ, 0, outputSize, 0, NULL, NULL, &status);
		if(!checkResult(data))
		{
			passed = false;
		}
		status = clEnqueueUnmapMemObject(commandQueue, outputBuffer, data, 0, NULL, NULL);
	}

	//run SOA kernel

	//initialize soa data
	data = (int *)clEnqueueMapBuffer(commandQueue,inputBuffer, true, CL_MAP_WRITE, 0, inputSize, 0, NULL, NULL, &status);
	for (int i = 0; i < treeNumber; i++)
	{
		for(int j = 0; j < treeSize; j++)
		{
			data[i + j* treeNumber] = j + i* treeSize;
		}
	}
	status = clEnqueueUnmapMemObject(commandQueue, inputBuffer, data, 0, NULL, NULL);

	// Warm-up.
	status = clEnqueueNDRangeKernel(commandQueue,
		kernel[1], 2,
		NULL,
		(const size_t*)global_work_size,
		(const size_t*)local_work_size,
		0,
		NULL,
		NULL);
	CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed");
	clFinish(commandQueue);
	sampleCommon->resetTimer(timer);
	sampleCommon->startTimer(timer);
	for (int i = 0; i < iterations; i++)
	{
		status |= clEnqueueNDRangeKernel(commandQueue,
			kernel[1], 2,
			NULL,
			(const size_t*)global_work_size,
			(const size_t*)local_work_size,
			0,
			NULL,
			NULL);
	}

	CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed");
	clFinish(commandQueue);

	sampleCommon->stopTimer(timer);
	soa_time = sampleCommon->readTimer(timer);
	totalTime += soa_time;
	// Collect the performance statistics.
	soa_perf = ((double)inputSize*(double)(iterations)*(double)(1e-09)) / soa_time;

	if(verify)
	{
		int* data = (int*)clEnqueueMapBuffer(commandQueue, outputBuffer, true, CL_MAP_READ, 0, outputSize, 0, NULL, NULL, &status);
		if(!checkResult(data))
		{
			passed = false;
		}
		status = clEnqueueUnmapMemObject(commandQueue, outputBuffer, data, 0, NULL, NULL);
	}
	
	return SDK_SUCCESS;
}


int 
SoAvsAoS::run()
{
	calResultByCPU();
	return runCLKernels();
}

void
SoAvsAoS::calResultByCPU()
{
	CPUResult = (int *)malloc(outputSize);
	memset(CPUResult,0,outputSize);
	for(int i=0;i < treeNumber;i++)
	{
		for(int j=0;j < treeSize;j++)
		{
			CPUResult[i] += i * treeSize + j;
		}
	}
}

bool
SoAvsAoS::checkResult(int *data)
{
	for(int i=0;i< treeNumber; i++)
	{
		if(data[i] != CPUResult[i])
		{
			return false;
		}
	}
	return true;
}

int 
SoAvsAoS::verifyResults()
{
	if(verify)
	{

		if(passed)
		{
			std::cout<<"Passed!\n" << std::endl;
			return SDK_SUCCESS;
		}
		else
		{
			std::cout<<"Failed\n" << std::endl;
			return SDK_FAILURE;
		}
	}
	return SDK_SUCCESS;
}

void 
SoAvsAoS::printStats()
{
	std::cout << std::endl << std::endl;
	std::cout << "Type\t\tElements\tSecond\t\tGB/s" << std::endl;
	std::cout << "--------------------------------------------------------------------------" << std::endl;

	printf("AOS\t\t%d\t%f\t%5.2f\n", 
		elements, 
		aos_time,
		aos_perf);
	printf("SOA\t\t%d\t%f\t%5.2f\n", 
		elements, 
		soa_time,
		soa_perf);
	if(timing)
	{
		std::cout<<"\n"<<"kernel run time:"<<"\n"<<totalTime<<std::endl;
	}
}

int
SoAvsAoS::cleanup()
{


	// Releases OpenCL resources (Context, Memory etc.)
	cl_int status;
	status = clReleaseProgram(program);
	CHECK_OPENCL_ERROR(status, "clReleaseProgram failed.(program)");

	status = clReleaseCommandQueue(commandQueue);
	CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed.(commandQueue)");

	status = clReleaseContext(context);
	CHECK_OPENCL_ERROR(status, "clReleaseContext failed.(context)");


	status = clReleaseKernel(kernel[0]);
	CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.(kernel)");

	status = clReleaseKernel(kernel[1]);
	CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.(kernel)");

	status = clReleaseMemObject(inputBuffer);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(inputBuffer)");

	status = clReleaseMemObject(outputBuffer);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(outputBuffer)");

	FREE(devices);
	FREE(CPUResult);



	return SDK_SUCCESS;
}


int
SoAvsAoS::checkInput()
{
	if(treeNumber < GROUP_SIZE)
	{
		std::cout<<"Because of the work group size ,this treeNumber is small"<<std::endl;
		return SDK_FAILURE;
	}
	if(treeNumber % 256 !=0)
	{
		std::cout<<"Because of the work group size ,should be a multiple of 256"<<std::endl;
		return SDK_FAILURE;
	}

	return SDK_SUCCESS;
}

std::string 
SoAvsAoS::makeOption()
{
	char optionStr1[32] = {"\0"};
	char optionStr2[32] = {"\0"};
	std::sprintf(optionStr1,"%d",treeSize);
	std::sprintf(optionStr2,"%d",treeNumber);
	std::string opStr = "-x clc++ -D num1="+std::string(optionStr1)+" -D num2="+std::string(optionStr2)+" ";
	return opStr;
}

int
main(int argc, char * argv[])
{
	SoAvsAoS clSoAvsAos("OpenCL SoAvsAoS");

	if (clSoAvsAos.initialize() != SDK_SUCCESS)
		return SDK_FAILURE;


	if (clSoAvsAos.parseCommandLine(argc, argv) != SDK_SUCCESS)
		return SDK_FAILURE;




	if(clSoAvsAos.isDumpBinaryEnabled())
	{
		return clSoAvsAos.genBinaryImage();
	}
	else
	{
		if(clSoAvsAos.setup() != SDK_SUCCESS)
			return SDK_FAILURE;

		if (clSoAvsAos.run() != SDK_SUCCESS)
			return SDK_FAILURE;

		clSoAvsAos.printStats();
		if (clSoAvsAos.verifyResults() != SDK_SUCCESS)
			return SDK_FAILURE;

		if (clSoAvsAos.cleanup() != SDK_SUCCESS)
			return SDK_FAILURE;


	}

	return SDK_SUCCESS;
}
