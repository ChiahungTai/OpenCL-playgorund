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

#include "DeviceFission.hpp"



int
DeviceFission::setupDeviceFission()
{
    // Make sure length is multiple of group size * numSubDevices
    unsigned int mulFactor = (unsigned int)groupSize * numSubDevices;
    length = (length < mulFactor) ? mulFactor : length;
    length = (length / mulFactor) * mulFactor;

	// Calculate half length
	half_length = length >> 1;

	// Get allocate memory for input buffer
    input = (cl_int*)malloc(half_length * sizeof(cl_int));
    CHECK_ALLOCATION(input, "Failed to allocate host memory. (input)");

    // Random initialisation of input
    sampleCommon->fillRandom<cl_int>(input, half_length, 1, 1, 8);

    // Unless quiet mode has been enabled, print the INPUT array
    if(!quiet) 
		sampleCommon->printArray<cl_int>("Input:", input, half_length, 1);

	// Get allocate memory for subOutput buffer
    subOutput = (cl_int*)malloc(length * sizeof(cl_int));
    CHECK_ALLOCATION(subOutput, "Failed to allocate host memory. (subOutput)");

    return SDK_SUCCESS;
}

int
DeviceFission::setupCLPlatform()
{
    cl_int status = CL_SUCCESS;

    /*
     * Have a look at the available platforms and pick either
     * the AMD one if available or a reasonable default.
     */
    cl_platform_id platform = NULL;
    int retValue = sampleCommon->getPlatform(platform, platformId, isPlatformEnabled());
    CHECK_ERROR(retValue, SDK_SUCCESS, "sampleCommon::getPlatform(rootplatform) failed");

    // Display available devices.
    retValue = sampleCommon->displayDevices(platform, CL_DEVICE_TYPE_ALL);
    CHECK_ERROR(retValue, SDK_SUCCESS, "sampleCommon::displayDevices(rootplatform) failed");

    /*
     * If we could find our platform, use it. Otherwise use just available platform.
     */
    cl_context_properties cps[3] = 
    {
        CL_CONTEXT_PLATFORM, 
        (cl_context_properties)platform, 
        0
    };

    rContext = clCreateContextFromType(platform ? cps : NULL,
									   CL_DEVICE_TYPE_ALL,
									   NULL,
									   NULL,
									   &status);
    CHECK_OPENCL_ERROR( status, "clCreateContextFromType failed.");

    // getting devices on which to run the sample
    status = sampleCommon->getDevices(rContext, &Devices, 0, isDeviceIdEnabled());
    CHECK_ERROR(status, SDK_SUCCESS, "sampleCommon::getDevices() failed");

    // Set deviceListSize from clGetContextInfo
	status = clGetContextInfo(rContext, CL_CONTEXT_DEVICES, 0, 0, &deviceListSize);
	CHECK_ERROR(status, SDK_SUCCESS, "clGetContextInfo failed. (deviceListSize)");

	// Get GPU device and CPU devices by the deviceInfo.
	for (cl_uint i = 0 ; i < deviceListSize / sizeof(cl_device_id) ; i++)
	{
		retValue = deviceInfo.setDeviceInfo(Devices[i]);
		CHECK_ERROR(retValue, 0, "SDKDeviceInfo::setDeviceInfo() failed");
		if (deviceInfo.dType == CL_DEVICE_TYPE_CPU)
		{
			cpuDevice = Devices[i];
		}
	}

	// Get allocate memory for subDevices
    subDevices = (cl_device_id*)malloc(numSubDevices * sizeof(cl_device_id));
    CHECK_ALLOCATION(subDevices, "Failed to allocate memory. (subDevices)");

	// Get allocate memory for subKernel
	subKernel = (cl_kernel*)malloc(numSubDevices * sizeof(cl_kernel));
	CHECK_ALLOCATION(subKernel, "Failed to allocate memory. (subKernel)");

	// Get maxSubDevices from clGetDeviceInfo
	cl_uint maxSubDevices;
	status = clGetDeviceInfo(cpuDevice, CL_DEVICE_PARTITION_MAX_SUB_DEVICES, sizeof(maxSubDevices), &maxSubDevices, NULL);
	CHECK_OPENCL_ERROR(status, "clGetDeviceInfo failed. (maxSubDevices)")

	if(maxSubDevices <= 1)
	{
		std::cout<<"Error: The CPU should have than one core to run this sample."<<std::endl;
		return SDK_FAILURE;
	}

	// Initialize required partition property
	cl_device_partition_property partitionPrty[5] =
	{
		CL_DEVICE_PARTITION_BY_COUNTS,  
		maxSubDevices / 2, maxSubDevices / 2,
		CL_DEVICE_PARTITION_BY_COUNTS_LIST_END, 
		0 };

	// Create sub-devices
	status = clCreateSubDevices(cpuDevice, partitionPrty, numSubDevices, subDevices, NULL);
	CHECK_OPENCL_ERROR( status, "clCreateSubDevices failed.");

    return SDK_SUCCESS;
}

int 
DeviceFission::genBinaryImage()
{
    streamsdk::bifData binaryData;
    binaryData.kernelName = std::string("DeviceFission_Kernels.cl");
    binaryData.flagsStr = std::string("");
    if(isComplierFlagsSpecified())
        binaryData.flagsFileName = std::string(flags.c_str());

    binaryData.binaryName = std::string(dumpBinary.c_str());
    int status = sampleCommon->generateBinaryImage(binaryData);
    return status;
}

int
DeviceFission::setupCLRuntime()
{
    cl_int status = CL_SUCCESS;

    // Create a CL program using the kernel source 
    streamsdk::buildProgramData buildData;
    buildData.kernelName = std::string("DeviceFission_Kernels.cl");
    buildData.devices = Devices;
    buildData.deviceId = deviceId;
    buildData.flagsStr = std::string("");
    if(isLoadBinaryEnabled())
        buildData.binaryName = std::string(loadBinary.c_str());

    if(isComplierFlagsSpecified())
        buildData.flagsFileName = std::string(flags.c_str());

	// Get allocate memory for subCmdQueue
	subCmdQueue = (cl_command_queue*)malloc(numSubDevices * sizeof(cl_command_queue));
	CHECK_ALLOCATION(subCmdQueue,"Failed to allocate memory. (subCmdQueue)");

	// Create command queue subCmdQueue
    for(cl_uint i = 0; i < numSubDevices; i++)
    {
        // Create command queue 
        subCmdQueue[i] = clCreateCommandQueue(rContext,
											  subDevices[i],
											  0,
											  &status);
        CHECK_OPENCL_ERROR(status, "clCreateCommandQueue failed. (subCmdQueue)");
    }

	// Create memory objects for input
    InBuf = clCreateBuffer(rContext,
                           CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                           length * sizeof(cl_int),
                           NULL,
                           &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (InBuf)");

	// Get allocate memory for sub devices output 
	subOutBuf = (cl_mem*)malloc(numSubDevices * sizeof(cl_mem));

	for(cl_uint i = 0; i < numSubDevices; i++)
	{
		// Create memory objects for sub devices output 
		subOutBuf[i] = clCreateBuffer(rContext,
								      CL_MEM_WRITE_ONLY,
								      half_length * sizeof(cl_int) ,
								      NULL,
								      &status);
		CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (subOutBuf)");
	}

    streamsdk::SDKFile kernelFile;
    std::string kernelPath = sampleCommon->getPath();

    char * source = NULL;
    size_t sourceSize[] = {0};
    char * binary = NULL;
    size_t binarySize = 0;

    if(isLoadBinaryEnabled())
    {
		kernelPath += loadBinary;

		if(kernelFile.readBinaryFromFile(kernelPath.c_str()))
		{
			std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
			return SDK_FAILURE;
		}

        // Get binaries and binary sizes for CPU devices
        char** subBinaries = (char**)malloc(numSubDevices * sizeof(char*));
        if(subBinaries == NULL)
        {
            sampleCommon->error("Failed to allocate memory(subBinaries)");
            return SDK_FAILURE;
        }

        size_t* subBinariesSize = (size_t*)malloc(numSubDevices * sizeof(size_t*));
        if(subBinariesSize == NULL)
        {
            sampleCommon->error("Failed to allocate memory(subBinariesSize)");
            return SDK_FAILURE;
        }

        for(cl_uint i = 0; i < numSubDevices; ++i)
        {
            subBinaries[i] = (char*)kernelFile.source().c_str();
            subBinariesSize[i] = kernelFile.source().size();
        }

        subProgram = clCreateProgramWithBinary(rContext,
                                               numSubDevices,
                                               subDevices, 
                                               (const size_t *)subBinariesSize,
                                               (const unsigned char**)subBinaries,
                                               NULL,
                                               &status);
		CHECK_OPENCL_ERROR(status, "clCreateProgramWithBinary failed.(subProgram)");

        free(subBinaries);
        free(subBinariesSize);
        subBinariesSize = NULL;
        subBinaries = NULL;
    }
    else
    {
        kernelPath.append("DeviceFission_Kernels.cl");
        if(!kernelFile.open(kernelPath.c_str()))//bool
        {
            std::cout << "Failed to load kernel file: " << kernelPath << std::endl;
            return SDK_FAILURE;
        }
        const char * source = kernelFile.source().c_str();
        size_t sourceSize[] = {strlen(source)};

        // Create a CL program for sub-devices using the kernel source
        subProgram = clCreateProgramWithSource(rContext,
                                               1,
                                               (const char**)&source,
                                               sourceSize,
                                               &status);
        CHECK_OPENCL_ERROR(status, "clCreateProgramWithSource failed.(subProgram)");

		 // Create a CL program for GPU device using the kernel source
		gpuProgram = clCreateProgramWithSource(rContext,
											   1,
											   (const char**)&source,
											   sourceSize,
											   &status);
		CHECK_OPENCL_ERROR(status, "clCreateProgramWithSource failed.(gpuProgram)");
    }

	// Get build options
	const char *flags;
	streamsdk::SDKFile flagsFile;
	std::string flagsPath = sampleCommon->getPath();
	if(buildData.flagsFileName.size() != 0)
	{
		flagsPath.append(buildData.flagsFileName.c_str());
		if(!flagsFile.open(flagsPath.c_str()))
		{
			std::cout << "Failed to load flags file: " << flagsPath << std::endl;
			return SDK_FAILURE;
		}
		flagsFile.replaceNewlineWithSpaces();
		flags = flagsFile.source().c_str();
		if(strlen(flags) != 0)
			std::cout << "Build Options are : " << flags << std::endl;
	}
	else
	{
		flags = NULL;
	}
	
    // Create a cl program executable for all sub-devices 
    status = clBuildProgram(subProgram,
                            numSubDevices,
                            subDevices,
                            flags,
                            NULL,
                            NULL);
	CHECK_OPENCL_ERROR(status, "clBuildProgram failed.(subProgram)");
    if(status != CL_SUCCESS)
    {
        if(status == CL_BUILD_PROGRAM_FAILURE)
        {
            cl_int logStatus;
            char * buildLog = NULL;
            size_t buildLogSize = 0;
            logStatus = clGetProgramBuildInfo(subProgram, 
                                              subDevices[0], 
                                              CL_PROGRAM_BUILD_LOG, 
                                              buildLogSize, 
                                              buildLog, 
                                              &buildLogSize);
            if(!sampleCommon->checkVal(logStatus,
                                       CL_SUCCESS,
                                       "clGetProgramBuildInfo failed."))
                return SDK_FAILURE;

            buildLog = (char*)malloc(buildLogSize);
            if(NULL == buildLog)
            {
                sampleCommon->error("Failed to allocate host memory.(buildLog)");
                return SDK_FAILURE;
            }
            memset(buildLog, 0, buildLogSize);

            logStatus = clGetProgramBuildInfo(subProgram, 
                                              subDevices[0], 
                                              CL_PROGRAM_BUILD_LOG, 
                                              buildLogSize, 
                                              buildLog, 
                                              NULL);
            if(!sampleCommon->checkVal(logStatus,
                                       CL_SUCCESS,
                                       "clGetProgramBuildInfo failed."))
            {
				free(buildLog);
				return SDK_FAILURE;
            }

            std::cout << " \n\t\t\tBUILD LOG(SUB-DEVICES)\n";
            std::cout << " ************************************************\n";
            std::cout << buildLog << std::endl;
            std::cout << " ************************************************\n";
            free(buildLog);
        }

        if(!sampleCommon->checkVal(status,
                                   CL_SUCCESS,
                                   "clBuildProgram failed. (SUB-DEVICES)"))
            return SDK_FAILURE;
	}

    // Get a kernel object handle for a kernel with the given name 
    subKernel[0] = clCreateKernel(subProgram,
                                  "Add",
                                  &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.(subKernel[0])");

	// Get a kernel object handle for a kernel with the given name 
	subKernel[1] = clCreateKernel(subProgram,
								  "Sub",
								  &status);
	CHECK_OPENCL_ERROR(status, "clCreateKernel failed.(subKernel[1])");

    return SDK_SUCCESS;
}

int
DeviceFission::runCLALLKerenls()
{
    cl_int status;
	cl_event writeEvent;
	cl_event rangeEvent[2];

    // Set global and local work items
    size_t globalThreads[] = {half_length};
    size_t localThreads[] = {groupSize};

	// Enqueue write Buffer to the first sub device queue
	status = clEnqueueWriteBuffer(subCmdQueue[0],
								  InBuf,
								  CL_FALSE,
								  0,
								  half_length* sizeof(cl_int),
								  input,
								  0,
								  NULL,
								  &writeEvent);
	CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed");

	cl_uint rangeEventNum = 0;

	rangeEvent[0] = rangeEvent[1] = writeEvent;
	rangeEventNum++;

    for(cl_uint i = 0; i < numSubDevices; ++i)
    {
		// Set subOutBuf as second argument 
		status = clSetKernelArg(subKernel[i], 1, sizeof(cl_mem), (void*)&subOutBuf[i]);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (subOutBuf)");

		// Set InBuf as first argument
		status = clSetKernelArg(subKernel[i], 0, sizeof(cl_mem),(void*)&InBuf);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (InBuf)");	

        // Enqueue kernel
		status = clEnqueueNDRangeKernel(subCmdQueue[i],
										subKernel[i],
										1,
										NULL,
										globalThreads,
										localThreads,
										rangeEventNum,
										&rangeEvent[i],
										NULL);
		CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.(subCmdQueue)");

		// Enqueue readBuffer
		status = clEnqueueReadBuffer(subCmdQueue[i],
									 subOutBuf[i],
									 CL_FALSE,
									 0,
									 half_length * sizeof(cl_int),
									 subOutput + half_length * i,
									 0,
									 NULL,
									 NULL);
		CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed. (subCmdQueue)");
	}

	// Flush all queues together

	for(cl_uint i = 0; i < numSubDevices; ++i)
	{
		status = clFlush(subCmdQueue[i]);
		CHECK_OPENCL_ERROR(status, "clFlush failed. (subCmdQueue)");
	}
	
	// Finish all queues
	status = clFinish(subCmdQueue[0]);
	CHECK_OPENCL_ERROR(status, "clFinish failed. (subCmdQueue[0])");

	status = clFinish(subCmdQueue[1]);
	CHECK_OPENCL_ERROR(status, "clFinish failed. (subCmdQueue[1])");

	status = clReleaseEvent(writeEvent);
	CHECK_OPENCL_ERROR(status, "clReleaseEvent failed. (writeEvent)");

    return SDK_SUCCESS;
}

int 
DeviceFission::runCLKernels()
{
	// create and initialize timers
	int timer = sampleCommon->createTimer();
	sampleCommon->resetTimer(timer);
	sampleCommon->startTimer(timer);

    // Run kernels on sub-devices and GPU device
    if(runCLALLKerenls() != CL_SUCCESS)
        return SDK_FAILURE;

	sampleCommon->stopTimer(timer);
	// Compute setup time
	totalTime = (double)(sampleCommon->readTimer(timer));

    return SDK_SUCCESS;
}

int DeviceFission::initialize()
{
    // Call base class Initialize to get default configuration
    if(this->SDKSample::initialize())
        return SDK_FAILURE;

    // Now add customized options
    streamsdk::Option* array_length = new streamsdk::Option;
    CHECK_ALLOCATION(array_length, "Memory allocation error.\n");

    array_length->_sVersion = "x";
    array_length->_lVersion = "length";
    array_length->_description = "Length of the Input array (Default value 1024)";
    array_length->_type = streamsdk::CA_ARG_INT;
    array_length->_value = &length;
    sampleArgs->AddOption(array_length);
    delete array_length;

    return SDK_SUCCESS;
}

int 
DeviceFission::setup()
{
    cl_int retValue = setupCLPlatform();
    if(retValue != SDK_SUCCESS)
        return (retValue == SDK_EXPECTED_FAILURE)? SDK_EXPECTED_FAILURE:SDK_FAILURE;

    if(setupDeviceFission() != SDK_SUCCESS)
        return SDK_FAILURE;

    if(setupCLRuntime() != SDK_SUCCESS)
        return SDK_FAILURE;

    return SDK_SUCCESS;
}


int DeviceFission::run()
{
    // Execute kernel on device
    if(runCLKernels() != SDK_SUCCESS)
        return SDK_FAILURE;

    if(!quiet)
	{
		sampleCommon->printArray<cl_int>("Sub devices first half output:", subOutput, half_length, 1);
		sampleCommon->printArray<cl_int>("Sub devices second half output:", subOutput + half_length, half_length, 1);
	}

    return SDK_SUCCESS;
}

int DeviceFission::verifyResults()
{
    if(verify)
    {
		bool subDevicesRlt = CL_TRUE;
		for(cl_uint i = 0; i < (cl_uint)(half_length); ++i)
		{
			if(input[i] - subOutput[i] != -1)
				subDevicesRlt = CL_FALSE;
			if(input[i] - subOutput[i + half_length] != 1)
				subDevicesRlt = CL_FALSE;
		}
        if(subDevicesRlt)
        {
            std::cout << "Passed!\n" << std::endl;
            return SDK_SUCCESS;
        }
        else
        {
            std::cout << "Failed\n" << std::endl;
            return SDK_FAILURE;
        }
    }
    return SDK_SUCCESS;
}

int
DeviceFission::cleanup()
{

    // Releases all OpenCL resources of root device 
    cl_int status;

    status = clReleaseProgram(subProgram);
    CHECK_OPENCL_ERROR(status, "clReleaseProgram failed. (subProgram)");

	status = clReleaseProgram(gpuProgram);
	CHECK_OPENCL_ERROR(status, "clReleaseProgram failed. (gpuProgram)");

	status = clReleaseMemObject(InBuf);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed. (InBuf)");

    for(cl_uint i = 0; i < numSubDevices; ++i)
    {
        status = clReleaseCommandQueue(subCmdQueue[i]);
        CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed. (subCmdQueue)");

		status = clReleaseDevice(subDevices[i]);
		CHECK_OPENCL_ERROR(status, "clReleaseDevice failed. (subDevices)");

		status = clReleaseKernel(subKernel[i]);
		CHECK_OPENCL_ERROR(status, "clReleaseKernel failed. (subKernel)");

		status = clReleaseMemObject(subOutBuf[i]);
		CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed. (subOutBuf)");

    }
	for(cl_uint i = 0; i < deviceListSize / sizeof(cl_device_id) ; ++i)
	{
		status = clReleaseDevice(Devices[i]);
		CHECK_OPENCL_ERROR(status, "clReleaseDevice failed. (Devices)");
	}
    status = clReleaseContext(rContext);
    CHECK_OPENCL_ERROR(status, "clReleaseContext failed. (rContext)");

		


    return SDK_SUCCESS;
}

DeviceFission::~DeviceFission()
{
    /* Release program resources (input memory etc.) */
    FREE(input);
    FREE(subOutput);
	FREE(subDevices);
	FREE(subKernel);
    FREE(subCmdQueue);
	FREE(subOutBuf);
	}

void 
DeviceFission::printStats()
{
	std::string strArray[2] = {"Input length", "RunTime(sec)"};

	std::string stats[2];

	stats[0] = sampleCommon->toString(length, std::dec);
	stats[1] = sampleCommon->toString(totalTime, std::dec);


	this->SDKSample::printStats(strArray, stats, 2);
}

int 
main(int argc, char * argv[])
{

    DeviceFission clDeviceFission("OpenCL DeviceFission");
	int temp = argc;
	int i = 0;
	while (temp > 0) 
    {
        std::string strLoad="--load";
		if(strLoad.compare(argv[i])==0)
		{
			std::cout << "Failed to load GPU kernel file, please assign it by '--loadgpu'. "<< std::endl;
			return SDK_FAILURE;
		}
        
        temp --;
        i ++;
    }

    // Initialize
    if(clDeviceFission.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    // ParseCommandLine
    if(clDeviceFission.parseCommandLine(argc, argv) != SDK_SUCCESS)
        return SDK_FAILURE;

    if(clDeviceFission.isDumpBinaryEnabled())
    {
        if(clDeviceFission.genBinaryImage() != SDK_SUCCESS)
            return SDK_FAILURE;
        return SDK_SUCCESS;
    }
    else
    {
        // Setup
        cl_int retValue = clDeviceFission.setup();
        if(retValue != SDK_SUCCESS)
            return (retValue == SDK_EXPECTED_FAILURE)? SDK_SUCCESS: SDK_FAILURE;

        // Run
        if(clDeviceFission.run() != SDK_SUCCESS)
            return SDK_FAILURE;
        // VerifyResults
        if(clDeviceFission.verifyResults() != SDK_SUCCESS)
            return SDK_FAILURE;
        // Cleanup
        if(clDeviceFission.cleanup() != SDK_SUCCESS)
            return SDK_FAILURE;
    }

	clDeviceFission.printStats();

    return SDK_SUCCESS;
}
