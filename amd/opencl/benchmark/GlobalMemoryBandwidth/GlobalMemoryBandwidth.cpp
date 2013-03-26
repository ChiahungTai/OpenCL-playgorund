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


#include "GlobalMemoryBandwidth.hpp"


//Separator
std::string sep = "-----------------------------------------";

int 
GlobalMemoryBandwidth::setupGlobalMemoryBandwidth()
{
    //Make vectorSize as 4 if -v option is 3. 

    //This memeory alignment is required as per OpenCL for type3 vectors 
    if(vectorSize == 3)
    {
        vec3 = true;
        vectorSize = 4;
    }
	else if((1 != vectorSize) && (2 != vectorSize) && (4 != vectorSize) && (8 != vectorSize) && (16 != vectorSize))
	{
		std::cout << "The vectorsize can only be one of 1,2,3(4),4,8,16!" << std::endl;
		return SDK_FAILURE;
	}

    /**
     * Allocate memory required for global buffer 
     * This includes both single and linear(cached and uncached) reads 
     */
    cl_uint sizeElement = vectorSize * sizeof(cl_float);
    cl_uint readLength = length + (NUM_READS * 1024 / sizeElement) + EXTRA_BYTES;
    cl_uint size = readLength * vectorSize * sizeof(cl_float);

    input = (cl_float*)malloc(size);
    CHECK_ALLOCATION(input, "Failed to allocate host memory. (input)");

    outputReadSingle = (cl_float*)malloc(length * vectorSize * sizeof(cl_float));
    CHECK_ALLOCATION(outputReadSingle, "Failed to allocate host memory. (outputReadSingle)");

    memset(outputReadSingle, 0, length * vectorSize * sizeof(cl_float));

    outputReadLinear = (cl_float*)malloc(length * vectorSize * sizeof(cl_float));
    CHECK_ALLOCATION(outputReadLinear, "Failed to allocate host memory. (outputReadLinear)");

    memset(outputReadLinear, 0, length * vectorSize * sizeof(cl_float));

    outputReadLU = (cl_float*)malloc(length * vectorSize * sizeof(cl_float));
    CHECK_ALLOCATION(outputReadLU, "Failed to allocate host memory. (outputReadLU)");

    memset(outputReadLU, 0, length * vectorSize * sizeof(cl_float));

    outputWriteLinear = (cl_float*)malloc(size);
    CHECK_ALLOCATION(outputWriteLinear, "Failed to allocate host memory. (outputWriteLinear)");

    memset(outputWriteLinear, 0, size);


	cl_uint lengthExtra = length * NUM_READS;
	cl_uint sizeE = lengthExtra * vectorSize *sizeof(cl_float);

	inputExtra = (cl_float*)malloc(sizeE);
	CHECK_ALLOCATION(inputExtra, "Failed to allocate host memory. (inputExtra)");

	sampleCommon->fillRandom<cl_float>(inputExtra,  
		lengthExtra * vectorSize, 
		1, 
		0, 
		10);


	outputReadRandom = (cl_float*)malloc(length * vectorSize * sizeof(cl_float));
	CHECK_ALLOCATION(outputReadRandom, "Failed to allocate host memory. (outputReadSingle)");

	memset(outputReadRandom, 0, length * vectorSize * sizeof(cl_float));

	outputReadunCombine = (cl_float*)malloc(length * vectorSize * sizeof(cl_float));
	CHECK_ALLOCATION(outputReadunCombine, "Failed to allocate host memory. (outputReadSingle)");

	memset(outputReadunCombine, 0, length * vectorSize * sizeof(cl_float));



    // random initialisation of input 
    sampleCommon->fillRandom<cl_float>(input,  
                                       readLength * vectorSize, 
                                       1, 
                                       0, 
                                    ( cl_float)(readLength-1));

	//sampleCommon->printArray("input",input,readLength * vectorSize,1);

    return SDK_SUCCESS;
}

int 
GlobalMemoryBandwidth::genBinaryImage()
{
    streamsdk::bifData binaryData;
    binaryData.kernelName = std::string("GlobalMemoryBandwidth_Kernels.cl");

    // Pass vectorSize as DATATYPE to kernel
    char buildOption[128];
	if(vectorSize == 1)
		sprintf(buildOption, "-D DATATYPE=float -D DATATYPE2=float4 -D OFFSET=%d ", OFFSET);
	else
		sprintf(buildOption, "-D DATATYPE=float%d -D DATATYPE2=float%d -D OFFSET=%d ", (vec3 == true) ? 3 : vectorSize,(vec3 == true) ? 3 : vectorSize, OFFSET);


    binaryData.flagsStr = std::string(buildOption);
    if(isComplierFlagsSpecified())
        binaryData.flagsFileName = std::string(flags.c_str());

    binaryData.binaryName = std::string(dumpBinary.c_str());
    int status = sampleCommon->generateBinaryImage(binaryData);
    CHECK_ERROR(status, SDK_SUCCESS, "OpenCL Generate Binary Image Failed");
    return status;
}


int
GlobalMemoryBandwidth::setupCL(void)
{
    cl_int status = 0;
    cl_device_type dType;
    if(deviceType.compare("cpu") == 0)
    {
        dType = CL_DEVICE_TYPE_CPU;
    }
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

    /*
     * If we could find our platform, use it. Otherwise use just available platform.
     */
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

    //Set device info of given cl_device_id
    retValue = deviceInfo.setDeviceInfo(devices[deviceId]);
    CHECK_ERROR(retValue, SDK_SUCCESS, "SDKDeviceInfo::setDeviceInfo() failed");

    std::string deviceStr(deviceInfo.deviceVersion);
    size_t vStart = deviceStr.find(" ", 0);
    size_t vEnd = deviceStr.find(" ", vStart + 1);
    std::string vStrVal = deviceStr.substr(vStart + 1, vEnd - vStart - 1);

#ifdef CL_VERSION_1_1
    if(vStrVal.compare("1.0") > 0)
    {
        char openclVersion[1024];
        status = clGetDeviceInfo(devices[deviceId],
                                 CL_DEVICE_OPENCL_C_VERSION,
                                 sizeof(openclVersion),
                                 openclVersion,
                                 0);
        CHECK_OPENCL_ERROR(status, "clGetDeviceInfo failed.");
        
        std::string tempStr(openclVersion);
        size_t dotPos = tempStr.find_first_of(".");
        size_t spacePos = tempStr.find_last_of(" ");
        tempStr = tempStr.substr(dotPos + 1, spacePos - dotPos);
        int minorVersion = atoi(tempStr.c_str());
        // OpenCL 1.1 has inbuilt support for vec3 data types
        if(minorVersion < 1 && vec3 == true)
        {
            OPENCL_EXPECTED_ERROR("Device doesn't support built-in 3 component vectors!");
        }
    }
    else
    {
        // OpenCL 1.1 has inbuilt support for vec3 data types
        if(vec3 == true)
        {
            OPENCL_EXPECTED_ERROR("Device doesn't support built-in 3 component vectors!");
        }
    }
#else
    // OpenCL 1.1 has inbuilt support for vec3 data types
    if(vec3 == true)
    {
        OPENCL_EXPECTED_ERROR("Device doesn't support built-in 3 component vectors!");
    }
#endif

    {
        // The block is to move the declaration of prop closer to its use 
        cl_command_queue_properties prop = 0;
        prop |= CL_QUEUE_PROFILING_ENABLE;

        commandQueue = clCreateCommandQueue(context, 
                                            devices[deviceId], 
                                            prop, 
                                            &status);
        CHECK_OPENCL_ERROR(status, "clCreateCommandQueue failed.");
    }

    cl_uint sizeElement = vectorSize * sizeof(cl_float);
    cl_uint readLength = length + (NUM_READS * 1024 / sizeElement) + EXTRA_BYTES;
	readRange = readLength;
    cl_uint size = readLength * vectorSize * sizeof(cl_float);


    // Create input buffer
    inputBuffer = clCreateBuffer(context, 
                                 CL_MEM_READ_ONLY,
                                 size,
                                 0, 
                                 &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (inputBuffer)");

    // Write data to buffer
    status = clEnqueueWriteBuffer(commandQueue,
                                  inputBuffer,
                                  1,
                                  0,
                                  size,
                                  input,
                                  0,
                                  0,
                                  0);
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (inputBuffer)");

    outputBufferReadSingle = clCreateBuffer(context, 
                                            CL_MEM_WRITE_ONLY,
                                            sizeof(cl_float) * vectorSize * length,
                                            0, 
                                            &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (outputBufferReadSingle)");

    // Write data to buffer
    status = clEnqueueWriteBuffer(commandQueue,
                                  outputBufferReadSingle,
                                  CL_TRUE,
                                  0,
                                  sizeof(cl_float) * vectorSize * length,
                                  outputReadSingle,
                                  0,
                                  NULL,
                                  NULL);
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (outputBufferReadSingle)");

    outputBufferReadLinear = clCreateBuffer(context, 
                                            CL_MEM_WRITE_ONLY,
                                            sizeof(cl_float) * vectorSize * length,
                                            0, 
                                            &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (outputBufferReadLinear)");

    // Write data to buffer
    status = clEnqueueWriteBuffer(commandQueue,
                                  outputBufferReadLinear,
                                  CL_TRUE,
                                  0,
                                  sizeof(cl_float) * vectorSize * length,
                                  outputReadLinear,
                                  0,
                                  NULL,
                                  NULL);
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (outputBufferReadLinear)");

    outputBufferReadLU = clCreateBuffer(context, 
                                        CL_MEM_WRITE_ONLY,
                                        sizeof(cl_float) * vectorSize * length,
                                        0, 
                                        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (outputBufferReadLU)");

    // Write data to buffer 
    status = clEnqueueWriteBuffer(commandQueue,
                                  outputBufferReadLU,
                                  CL_TRUE,
                                  0,
                                  sizeof(cl_float) * vectorSize * length,
                                  outputReadLU,
                                  0,
                                  NULL,
                                  NULL);
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (outputBufferReadLU)");

     outputBufferWriteLinear = clCreateBuffer(context, 
                                              CL_MEM_WRITE_ONLY,
                                              size,
                                              0, 
                                              &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (outputBufferWriteLinear)");

    // Write data to buffer 
    status = clEnqueueWriteBuffer(commandQueue,
                                  outputBufferWriteLinear,
                                  CL_TRUE,
                                  0,
                                  size,
                                  outputWriteLinear,
                                  0,
                                  NULL,
                                  NULL);
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (outputBufferWriteLinear)");


	inputBufferExtra = clCreateBuffer(context,
		 CL_MEM_READ_ONLY,
		 sizeof(cl_float) * vectorSize *length * NUM_READS,
		 0,
		 &status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed due to inavailability of memory on this platform. (inputBufferExtra)");

	status = clEnqueueWriteBuffer(commandQueue,
		inputBufferExtra,
		CL_TRUE,
		0,
		sizeof(cl_float) * vectorSize *length * NUM_READS,
		inputExtra,
		0,
		NULL,
		NULL);
	CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (inputBufferExtra)");

	outputBufferReadRandom = clCreateBuffer(context, 
		CL_MEM_WRITE_ONLY,
		sizeof(cl_float) * vectorSize * length,
		0, 
		&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (outputBufferReadCombine)");

	// Write data to buffer 
	status = clEnqueueWriteBuffer(commandQueue,
		outputBufferReadRandom,
		CL_TRUE,
		0,
		sizeof(cl_float) * vectorSize * length,
		outputReadRandom,
		0,
		NULL,
		NULL);
	CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (outputBufferReadCombine)");

	outputBufferReadunCombine = clCreateBuffer(context, 
		CL_MEM_WRITE_ONLY,
		sizeof(cl_float) * vectorSize * length,
		0, 
		&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (outputBufferReadunCombine)");

	// Write data to buffer 
	status = clEnqueueWriteBuffer(commandQueue,
		outputBufferReadunCombine,
		CL_TRUE,
		0,
		sizeof(cl_float) * vectorSize * length,
		outputReadunCombine,
		0,
		NULL,
		NULL);
	CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (outputBufferReadunCombine)");




    // create a CL program using the kernel source
    char buildOption[128];
    if(vectorSize == 1)
        sprintf(buildOption, "-D DATATYPE=float -D DATATYPE2=float4 -D OFFSET=%d ", OFFSET);
    else
        sprintf(buildOption, "-D DATATYPE=float%d -D DATATYPE2=float%d -D OFFSET=%d ", (vec3 == true) ? 3 : vectorSize,(vec3 == true) ? 3 : vectorSize, OFFSET);

    // create a CL program using the kernel source
    streamsdk::buildProgramData buildData;
    buildData.kernelName = std::string("GlobalMemoryBandwidth_Kernels.cl");
    buildData.devices = devices;
    buildData.deviceId = deviceId;
    buildData.flagsStr = std::string(buildOption);
    if(isLoadBinaryEnabled())
        buildData.binaryName = std::string(loadBinary.c_str());

    if(isComplierFlagsSpecified())
        buildData.flagsFileName = std::string(flags.c_str());

    retValue = sampleCommon->buildOpenCLProgram(program, context, buildData);
    CHECK_ERROR(retValue, SDK_SUCCESS, "sampleCommon::buildOpenCLProgram() failed");

    // Global memory bandwidth from read-single access
    kernel[0] = clCreateKernel(program, "read_single", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.(read_single)");

    // Global memory  bandwidth from read-linear access
    kernel[1] = clCreateKernel(program, "read_linear", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.(read_linear)");

    // Global memory  bandwidth from read-linear access
    kernel[2] = clCreateKernel(program, "read_linear_uncached", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.(read_linear_uncached)");

    // Global memory  bandwidth from write-linear access
    kernel[3] = clCreateKernel(program, "write_linear", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.(GlobalBandwidth_write_linear)");


	if(vectorSize != 1)
	{
	   kernel[4] = clCreateKernel(program, "read_random", &status);
	CHECK_OPENCL_ERROR(status, "clCreateKernel failed.(GlobalBandwidth_read_random)");
	}
	else
	{
		kernel[4] = clCreateKernel(program, "read_random1", &status);
		CHECK_OPENCL_ERROR(status, "clCreateKernel failed.(GlobalBandwidth_read_random1)");

	}

	kernel[5] = clCreateKernel(program, "read_uncoalescing", &status);
	CHECK_OPENCL_ERROR(status, "clCreateKernel failed.(GlobalBandwidth_read_random1)");


    return SDK_SUCCESS;
}

int
GlobalMemoryBandwidth::bandwidth(cl_kernel &kernel, 
                             cl_mem &outputBuffer,
                             cl_float *output)
{
    cl_int status;
    cl_mem constValue =NULL;

    // Check group size against kernelWorkGroupSize
    status = clGetKernelWorkGroupInfo(kernel,
                                      devices[deviceId],
                                      CL_KERNEL_WORK_GROUP_SIZE,
                                      sizeof(size_t),
                                      &kernelWorkGroupSize,
                                      0);
    CHECK_OPENCL_ERROR(status, "clGetKernelWorkGroupInfo failed.");

    if(localThreads > kernelWorkGroupSize)
        localThreads = kernelWorkGroupSize;

    //Set appropriate arguments to the kernel
    int argIndex = 0;
    if(!writeFlag && !extraFlag)
    {
		
        status = clSetKernelArg(kernel,
                                argIndex++,
                                sizeof(cl_mem), 
                                (void *)&inputBuffer);
        CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.(inputBuffer)");
		
    }
    else if(extraFlag)
	{
		status = clSetKernelArg(kernel,
			argIndex++,
			sizeof(cl_mem), 
			(void *)&inputBufferExtra);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.(inputBufferExtra)");
	}
	else
    {
        // Pass a single constant value to kernel of type - float<vectorSize>
        cl_float *temp = (cl_float*)malloc(vectorSize * sizeof(cl_float));
        memset(temp, 0, vectorSize * sizeof(cl_float));

        constValue = clCreateBuffer(context,
                                           CL_MEM_READ_ONLY,
                                           vectorSize * sizeof(cl_float),
                                           0,
                                           &status);
        CHECK_OPENCL_ERROR(status, "clCreateBuffer failed.(constValue)");

        status = clEnqueueWriteBuffer(commandQueue,
                                      constValue,
                                      1,
                                      0,
                                      vectorSize * sizeof(cl_float),
                                      temp,
                                      0,
                                      0,
                                      0);
        if(sampleCommon->checkVal(status,
                                   CL_SUCCESS,
                                   "clEnqueueWriteBuffer failed.(constValue)"))
        {
            //Deallocate temp memory
            FREE(temp);
            return SDK_FAILURE;
        }

        // Deallocate temp memory
        FREE(temp);
        status = clSetKernelArg(kernel,
                                argIndex++,
                                sizeof(cl_mem), 
                                (void *)&constValue);
        CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.(constValue)");
    }

    status = clSetKernelArg(kernel,
                            argIndex++,
                            sizeof(cl_mem), 
                            (void *)&outputBuffer);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.(outputBuffer)");


    double sec = 0;
    int iter = iterations;
    if(verify)
        iter = 1;

    // Run the kernel for a number of iterations
    for(int i = 0; i < iter; i++)
    {
        // Enqueue a kernel run call
        cl_event ndrEvt;
        status = clEnqueueNDRangeKernel(commandQueue,
                                        kernel,
                                        1,
                                        NULL,
                                        &globalThreads,
                                        &localThreads,
                                        0,
                                        NULL,
                                        &ndrEvt);
        CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

        // wait for the kernel call to finish execution 
        status = clWaitForEvents(1, &ndrEvt);
        CHECK_OPENCL_ERROR(status, "clWaitForEvents failed.");

        // Calculate performance 
        cl_ulong startTime;
        cl_ulong endTime;
        
        // Get kernel profiling info 
        status = clGetEventProfilingInfo(ndrEvt,
                                         CL_PROFILING_COMMAND_START,
                                         sizeof(cl_ulong),
                                         &startTime,
                                         0);
        CHECK_OPENCL_ERROR(status, "clGetEventProfilingInfo failed.(startTime)");

        status = clGetEventProfilingInfo(ndrEvt,
                                         CL_PROFILING_COMMAND_END,
                                         sizeof(cl_ulong),
                                         &endTime,
                                         0);
        CHECK_OPENCL_ERROR(status, "clGetEventProfilingInfo failed.(endTime)");

        // Cumulate time for each iteration 
        sec += 1e-9 * (endTime - startTime);

        status = clReleaseEvent(ndrEvt);
        CHECK_OPENCL_ERROR(status, "clGetEventProfilingInfo failed.(endTime)");
    }

    // Copy bytes 
    int bytesPerThread = 0;
    if(vec3 == true)
        bytesPerThread = NUM_READS * 3 * sizeof(cl_float);
    else
        bytesPerThread = NUM_READS * vectorSize * sizeof(cl_float);
    double bytes = (double)(iter * bytesPerThread);
    double perf = (bytes / sec) * 1e-9;
    perf *= globalThreads;
    
    std::cout << ": " << perf << " GB/s" << std::endl;

    // Enqueue readBuffer
    status = clEnqueueReadBuffer(commandQueue,
                                 outputBuffer,
                                 CL_TRUE,
                                 0,
                                 length * sizeof(cl_float) * vectorSize,
                                 output,
                                 0,
                                 NULL,
                                 0);
    CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

    if(constValue)
    {
        status = clReleaseMemObject(constValue);
        CHECK_OPENCL_ERROR(status, "clReleaseMemOnject failed.");
    }
    return SDK_SUCCESS;
}

int 
GlobalMemoryBandwidth::runCLKernels(void)
{
    int status;

    if(vec3 == true)
        std::cout << "\nGlobal Memory Read\nAccessType\t: single\nVectorElements\t: 3" << std::endl;
    else
        std::cout << "\nGlobal Memory Read\nAccessType\t: single\nVectorElements\t: " << vectorSize << std::endl;

    std::cout << "Bandwidth\t";

    // Measure bandwidth of single reads from global buffer
    status = bandwidth(kernel[0], outputBufferReadSingle, outputReadSingle);
    if(status != SDK_SUCCESS)
        return SDK_FAILURE;

    if(vec3 == true)
        std::cout << "\nGlobal Memory Read\nAccessType\t: linear\nVectorElements\t: 3" << std::endl;
    else
        std::cout << "\nGlobal Memory Read\nAccessType\t: linear\nVectorElements\t: " << vectorSize << std::endl;

    std::cout << "Bandwidth\t";

    // Measure bandwidth of linear reads from global buffer
    status = bandwidth(kernel[1], outputBufferReadLinear, outputReadLinear);
    if(status != SDK_SUCCESS)
        return SDK_FAILURE;

    if(vec3 == true)
        std::cout << "\nGlobal Memory Read\nAccessType\t: linear(uncached)\nVectorElements\t: 3" << std::endl;
    else
        std::cout << "\nGlobal Memory Read\nAccessType\t: linear(uncached)\nVectorElements\t: " << vectorSize << std::endl;

    std::cout << "Bandwidth\t";

    // Measure bandwidth of linear reads from global buffer 
    status = bandwidth(kernel[2], outputBufferReadLU, outputReadLU);
    if(status != SDK_SUCCESS)
        return SDK_FAILURE;

    if(vec3 == true)
        std::cout << "\nGlobal Memory Write\nAccessType\t: linear\nVectorElements\t: 3" << std::endl;
    else
        std::cout << "\nGlobal Memory Write\nAccessType\t: linear\nVectorElements\t: " << vectorSize << std::endl;

    std::cout << "Bandwidth\t";
    writeFlag = true;

    // Measure bandwidth of linear reads from global buffer 
    status = bandwidth(kernel[3], outputBufferWriteLinear, outputWriteLinear);
    if(status != SDK_SUCCESS)
        return SDK_FAILURE;
	writeFlag = false;


	

	if(vec3 == true)
		std::cout << "\nGlobal Memory Read\nAccessType\t: random\nVectorElements\t: 3" << std::endl;
	else
		std::cout << "\nGlobal Memory Read\nAccessType\t: random\nVectorElements\t: " << vectorSize << std::endl;

	std::cout << "Bandwidth\t";
	
	status = bandwidth(kernel[4], outputBufferReadRandom, outputReadRandom);
	if(status != SDK_SUCCESS)
		return SDK_FAILURE;
	

	extraFlag = true;
	if(vec3 == true)
		std::cout << "\nGlobal Memory Read\nAccessType\t: unCombine_unCache\nVectorElements\t: 3" << std::endl;
	else
		std::cout << "\nGlobal Memory Read\nAccessType\t: unCombine_unCache\nVectorElements\t: " << vectorSize << std::endl;

	std::cout << "Bandwidth\t";

	status = bandwidth(kernel[5], outputBufferReadunCombine, outputReadunCombine);
	if(status != SDK_SUCCESS)
		return SDK_FAILURE;

	


    return SDK_SUCCESS;
}


int 
GlobalMemoryBandwidth::initialize()
{
    // Call base class Initialize to get default configuration 
    if(this->SDKSample::initialize())
        return SDK_FAILURE;

    streamsdk::Option* num_iterations = new streamsdk::Option;
    CHECK_ALLOCATION(num_iterations,"num_iterators memory allocation failed");

    num_iterations->_sVersion = "i";
    num_iterations->_lVersion = "iterations";
    num_iterations->_description = "Number of iterations for kernel execution";
    num_iterations->_type = streamsdk::CA_ARG_INT;
    num_iterations->_value = &iterations;

    sampleArgs->AddOption(num_iterations);
    delete num_iterations;

    streamsdk::Option* num_arguments = new streamsdk::Option;
    CHECK_ALLOCATION(num_arguments,"num_arguments memory allocation failed");

    num_arguments->_sVersion = "c";
    num_arguments->_lVersion = "components";
    num_arguments->_description = "Number of vector components to be used. Can be either 1,2,3(4),4,8,16";
    num_arguments->_type = streamsdk::CA_ARG_INT;
    num_arguments->_value = &vectorSize;

    sampleArgs->AddOption(num_arguments);
    delete num_arguments;

    return SDK_SUCCESS;
}

int 
GlobalMemoryBandwidth::setup()
{
	if(iterations < 1)
	{
		std::cout<<"Error, iterations cannot be 0 or negative. Exiting..\n";
		exit(0);
	}
    if(setupGlobalMemoryBandwidth()!=SDK_SUCCESS)
        return SDK_FAILURE;

    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    int status = setupCL();
    if(status != SDK_SUCCESS)
        return status;

    sampleCommon->stopTimer(timer);

    setupTime = (cl_double)sampleCommon->readTimer(timer);

    return SDK_SUCCESS;
}


int 
GlobalMemoryBandwidth::run()
{
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);   

    // Arguments are set and execution call is enqueued on command buffer
    if(runCLKernels() != SDK_SUCCESS)
        return SDK_FAILURE;

    sampleCommon->stopTimer(timer);
    totalKernelTime = (double)(sampleCommon->readTimer(timer));

    return SDK_SUCCESS;
}

int 
GlobalMemoryBandwidth::verifyResults()
{
    if(verify)
    {
        int vecElements = (vec3 == true) ? 3 : vectorSize;
        // Verify result for single access
        verificationOutput = (cl_float*)malloc(length * vectorSize * sizeof(cl_float));
        CHECK_ALLOCATION(verificationOutput, "verificationOutput memory allocation failed");

        std::cout << "\nVerifying results for Read-Single : ";
        memset(verificationOutput, 0, length * vectorSize * sizeof(cl_float));

        int index = 0;
        for(int i = 0; i < (int)length; i++)
        {
            for(int j = 0; j < NUM_READS; j++)
            {
                for(int k = 0; k < vecElements; k++)
                    verificationOutput[i * vectorSize + k] += input[(index + j) * vectorSize + k];
            }
        }

        if(!memcmp(outputReadSingle, 
                   verificationOutput, 
                   length * vectorSize * sizeof(cl_float)))
        {
            std::cout << "Passed!\n" << std::endl;
        }
        else
        {
            std::cout << "Failed!\n" << std::endl;
            return SDK_FAILURE;
        }

        std::cout << "\nVerifying results for Read-Linear : ";
        memset(verificationOutput, 0, length * vectorSize * sizeof(cl_float));
        // Verify result for Linear access 
        for(int i = 0; i < (int)length; i++)
        {
            index = i;
            for(int j = 0; j < NUM_READS; j++)
            {
                for(int k = 0; k < vecElements; k++)
                    verificationOutput[i * vectorSize + k] += input[(index + j) * vectorSize + k];
            }
        }

        if(!memcmp(outputReadLinear, 
                   verificationOutput, 
                   length * vectorSize * sizeof(cl_float)))
        {
            std::cout << "Passed!\n" << std::endl;
        }
        else
        {
            std::cout << "Failed!\n" << std::endl;
            return SDK_FAILURE;
        }

        std::cout << "\nVerifying results for Read-Linear(uncached) : ";
        memset(verificationOutput, 0, length * vectorSize * sizeof(cl_float));
        // Verify result for Linear access
        int elementSize = sizeof(cl_float) * vectorSize;
        int stride = 512 / elementSize;
        for(int i = 0; i < (int)length; i++)
        {
            int readPos = i;
            for(int j = 0; j < NUM_READS; j++)
            {
                readPos += (4096 * vectorSize);
                for(int k = 0; k < vecElements; k++)
                {
                    verificationOutput[i * vectorSize + k] += input[readPos * vectorSize + k];
                }
            }
        }

        if(!memcmp(outputReadLU, 
                   verificationOutput, 
                   length * vectorSize * sizeof(cl_float)))
        {
           std::cout << "Passed!\n" << std::endl;
        }
        else
        {
            std::cout << "Failed!\n" << std::endl;
            return SDK_FAILURE;
        }

        std::cout << "\nVerifying results for Write-Linear : ";
        memset(verificationOutput, 0, length * vectorSize * sizeof(cl_float));
        if(!memcmp(outputWriteLinear, 
                   verificationOutput, 
                   length * vectorSize * sizeof(cl_float)))
        {
           std::cout << "Passed!\n" << std::endl;
          
        }
        else
        {
            std::cout << "Failed!\n" << std::endl;
            return SDK_FAILURE;
        }


		cl_uint index1,readpos;
		std::cout << "\nVerifying results for Read-Random : ";
		memset(verificationOutput, 0, length * vectorSize * sizeof(cl_float));
			
			
				// Verify result for Random access 
				for(int i = 0; i < (int)length; i++)
				{
					 index1 = i % localThreads;
					//cl_uint index = 0;
					for(int j = 0; j < NUM_READS; j++)
					{ 

						for(int k =0 ; k<vectorSize ;k++)
						{
							verificationOutput[i * vectorSize + k] += input[index1 * vectorSize + k];

						}
						index1 = (cl_uint)(input[index1 * vectorSize + 0]);
						
					}
				}


		if(!memcmp(outputReadRandom, 
			verificationOutput, 
			length * vectorSize * sizeof(cl_float)))
		{
			std::cout << "Passed!\n" << std::endl;
		}
		else
		{
			std::cout << "Failed!\n" << std::endl;
			return SDK_FAILURE;
		}

		std::cout << "\nVerifying results for Read-unCombine : ";
		memset(verificationOutput, 0, length * vectorSize * sizeof(cl_float));

		for(int i =0;i<(int)length;i++)
		{
			for(int j=0;j<NUM_READS;j++)
			{
				readpos = i*NUM_READS + j;
				for(int k=0;k<vectorSize;k++)
				{
					verificationOutput[i * vectorSize + k] += inputExtra[(readpos) * vectorSize + k];

				}
			}
		}
		if(!memcmp(outputReadunCombine, 
			verificationOutput, 
			length * vectorSize * sizeof(cl_float)))
		{
			std::cout << "Passed!\n" << std::endl;
		}
		else
		{
			std::cout << "Failed!\n" << std::endl;
			return SDK_FAILURE;
		}


    }


    return SDK_SUCCESS;
}


int 
GlobalMemoryBandwidth::cleanup()
{
    // Releases OpenCL resources (Context, Memory etc.)
    cl_int status;

    for(int i = 0; i < NUM_KERNELS; i++)
    {
        status = clReleaseKernel(kernel[i]);
        CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.");
    }

    status = clReleaseProgram(program);
    CHECK_OPENCL_ERROR(status, "clReleaseProgram failed.(program)");
 
    status = clReleaseMemObject(inputBuffer);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(inputBuffer)");

    status = clReleaseMemObject(outputBufferReadSingle);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(outputBufferReadSingle)");

    status = clReleaseMemObject(outputBufferReadLinear);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(outputBufferReadLinear)");

    status = clReleaseMemObject(outputBufferReadLU);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(outputBufferReadLU)");

    status = clReleaseMemObject(outputBufferWriteLinear);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(outputBufferWriteLinear)");


	status = clReleaseMemObject(outputBufferReadRandom);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(outputBufferWriteLinear)");

	status = clReleaseMemObject(outputBufferReadunCombine);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(outputBufferWriteLinear)");

	status = clReleaseMemObject(inputBufferExtra);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(inputBufferExtra)");




    status = clReleaseCommandQueue(commandQueue);
    CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed.(commandQueue)");

    status = clReleaseContext(context);
    CHECK_OPENCL_ERROR(status, "clReleaseContext failed. (context)");

    // release program resources (input memory etc.)

    FREE(input);
    FREE(outputReadSingle);
    FREE(outputReadLinear);
    FREE(outputReadLU);
    FREE(outputWriteLinear);
    FREE(verificationOutput);

	FREE(outputReadRandom);
	FREE(outputReadunCombine);
	FREE(inputExtra);

    FREE(devices);

    return SDK_SUCCESS;
}

int 
main(int argc, char * argv[])
{
    cl_int status = 0;
    GlobalMemoryBandwidth clGlobalMemoryBandwidth("OpenCL Reduction");

    if(clGlobalMemoryBandwidth.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    if(clGlobalMemoryBandwidth.parseCommandLine(argc, argv) != SDK_SUCCESS)
        return SDK_FAILURE;

    if(clGlobalMemoryBandwidth.isDumpBinaryEnabled())
    {
        return clGlobalMemoryBandwidth.genBinaryImage();
    }

    status = clGlobalMemoryBandwidth.setup();
    if(status != SDK_SUCCESS)
    {
        if(status == SDK_EXPECTED_FAILURE)
            return SDK_SUCCESS;

        return SDK_FAILURE;
    }

    if(clGlobalMemoryBandwidth.run() != SDK_SUCCESS)
        return SDK_FAILURE;

    if(clGlobalMemoryBandwidth.verifyResults() != SDK_SUCCESS)
        return SDK_FAILURE;
    
    if(clGlobalMemoryBandwidth.cleanup() != SDK_SUCCESS)
        return SDK_FAILURE;

    return SDK_SUCCESS;
}
