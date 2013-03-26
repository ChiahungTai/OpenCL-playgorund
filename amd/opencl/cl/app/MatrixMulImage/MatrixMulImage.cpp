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


#include "MatrixMulImage.hpp"

int 
MatrixMulImage::setupMatrixMulImage()
{
    // allocate and init memory used by host  input0[width0][height0]
    cl_uint inputSizeBytes0 = width0 * height0 * sizeof(cl_float);

    input0 = (cl_float *) malloc(inputSizeBytes0);
    CHECK_ALLOCATION(input0, "Failed to allocate host memory. (input0)");

    // allocate and init memory used by host input1[width1][height1]
    cl_uint inputSizeBytes1 = width1 * height1 * sizeof(cl_float);

    input1 = (cl_float *) malloc(inputSizeBytes1);
    CHECK_ALLOCATION(input1, "Failed to allocate host memory. (input1)");
    
    // random initialisation of input
    sampleCommon->fillRandom<cl_float>(input0, width0, height0, 0, 10);
    sampleCommon->fillRandom<cl_float>(input1, width1, height1, 0, 10);

    // allocate memory for output[width1][height0]
    cl_uint outputSizeBytes = height0 * width1 * sizeof(cl_float);

    output = (cl_float *) malloc(outputSizeBytes);
    CHECK_ALLOCATION(output, "Failed to allocate host memory. (output)");

    // allocate memory for output[width1][height0] of reference implemenation
    if(verify)
    {
        verificationOutput = (cl_float *) malloc(outputSizeBytes);
        CHECK_ALLOCATION(verificationOutput, "Failed to allocate host memory. (verificationOutput)");
        memset(verificationOutput, 0, outputSizeBytes);
    }

    /* 
     * Unless quiet mode has been enabled, print the INPUT arrays
     */
    if(!quiet) 
    {
        sampleCommon->printArray<cl_float>(
            "Input0", 
            input0, 
            width0, 
            1);
        sampleCommon->printArray<cl_float>(
            "Input1", 
            input1, 
            width1, 
            1);
    }
    return SDK_SUCCESS;
}

int 
MatrixMulImage::genBinaryImage()
{
    streamsdk::bifData binaryData;
    binaryData.kernelName = std::string("MatrixMulImage_Kernels.cl");
    binaryData.flagsStr = std::string("");
    if(isComplierFlagsSpecified())
        binaryData.flagsFileName = std::string(flags.c_str());

    binaryData.binaryName = std::string(dumpBinary.c_str());
    int status = sampleCommon->generateBinaryImage(binaryData);
    return status;
}


int
MatrixMulImage::setupCL(void)
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

    context = clCreateContextFromType(
                  cps,
                  dType,
                  NULL,
                  NULL,
                  &status);
    CHECK_OPENCL_ERROR(status, "clCreateContextFromType failed.");
    
    // getting device on which to run the sample
    status = sampleCommon->getDevices(context, &devices, deviceId, isDeviceIdEnabled());
    CHECK_ERROR(status, 0, "sampleCommon::getDevices() failed");

    //Set device info of given cl_device_id
    retValue = deviceInfo.setDeviceInfo(devices[deviceId]);
    CHECK_ERROR(retValue, SDK_SUCCESS, "deviceInfo.setDeviceInfo. failed");

    {
        // The block is to move the declaration of prop closer to its use
        cl_command_queue_properties prop = 0;
        prop |= CL_QUEUE_PROFILING_ENABLE;

        commandQueue = clCreateCommandQueue(
                           context, 
                           devices[deviceId], 
                           prop, 
                           &status);
        CHECK_ERROR(retValue, SDK_SUCCESS, "clCreateCommandQueue. failed");
    }

    cl_image_format imageFormat;
    imageFormat.image_channel_data_type = CL_FLOAT;
    imageFormat.image_channel_order = CL_RGBA;

    if(!deviceInfo.imageSupport)
    {
        std::cout << "Expected Error: Image is not supported on the Device" << std::endl;
        return SDK_EXPECTED_FAILURE;
    }

    cl_image_desc imageDesc;
    memset(&imageDesc, '\0', sizeof(cl_image_desc));
    imageDesc.image_type = CL_MEM_OBJECT_IMAGE2D;

    // Create image for matrix A
    imageDesc.image_width = width0 / 4;
    imageDesc.image_height = height0;
    inputBuffer0 = clCreateImage(context,
                                 CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                 &imageFormat,
                                 &imageDesc,
                                 input0,
                                 &status);
    CHECK_OPENCL_ERROR(status, "clCreateImage failed. (inputBuffer0)");
   
    // Create image for matrix B
    imageDesc.image_width = width1 / 4;
    imageDesc.image_height = height1;
    inputBuffer1 = clCreateImage(context,
                                 CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                 &imageFormat,
                                 &imageDesc,
                                 input1,
                                 &status);
    CHECK_OPENCL_ERROR(status, "clCreateImage failed. (inputBuffer1)");
    
    // Create image for matrix C
    imageDesc.image_width = width1 / 4;
    imageDesc.image_height = height0;
    outputBuffer = clCreateImage(context,
                                 CL_MEM_WRITE_ONLY,
                                 &imageFormat,
                                 &imageDesc,
                                 0,
                                 &status);
    CHECK_OPENCL_ERROR(status, "clCreateImage failed. (outputBuffer)");

    // create a CL program using the kernel source 
    streamsdk::buildProgramData buildData;
    buildData.kernelName = std::string("MatrixMulImage_Kernels.cl");
    buildData.devices = devices;
    buildData.deviceId = deviceId;
    buildData.flagsStr = std::string("");
    if(isLoadBinaryEnabled())
        buildData.binaryName = std::string(loadBinary.c_str());

    if(isComplierFlagsSpecified())
        buildData.flagsFileName = std::string(flags.c_str());

    retValue = sampleCommon->buildOpenCLProgram(program, context, buildData);
    CHECK_ERROR(retValue, SDK_SUCCESS, "sampleCommon::buildOpenCLProgram() failed");
    
    kernel = clCreateKernel(program, "mmmKernel3", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.(kernel)");
   
    return SDK_SUCCESS;
}


int 
MatrixMulImage::runCLKernels(void)
{
    cl_int   status;

    /* 
     * Kernel runs over complete output matrix with blocks of blockSize x blockSize 
     * running concurrently
     */
    size_t globalThreads[2]= {width1 / 4, height0 / 8};
    size_t localThreads[2] = {blockSize, blockSize};

    status = kernelInfo.setKernelWorkGroupInfo(kernel, devices[deviceId]);
    CHECK_OPENCL_ERROR(status, "kernelInfo.setKernelWorkGroupInfo failed");
    
    availableLocalMemory = deviceInfo.localMemSize - kernelInfo.localMemoryUsed;
    neededLocalMemory = 2 * blockSize * blockSize * sizeof(cl_float); 
    if(neededLocalMemory > availableLocalMemory)
    {
        std::cout << "Unsupported: Insufficient local memory on device." << std::endl;
        return SDK_SUCCESS;
    }

    if((cl_uint)(localThreads[0]*localThreads[1]) > kernelInfo.kernelWorkGroupSize)
    {
       if(kernelInfo.kernelWorkGroupSize >= 64)
        {
            blockSize = 8; 
            localThreads[0] = blockSize;
            localThreads[1] = blockSize;
        }
        else if(kernelInfo.kernelWorkGroupSize >= 32)
        {
            blockSize = 4; 
            localThreads[0] = blockSize;
            localThreads[1] = blockSize;
        }
        else
        {
            std::cout << "Out of Resources!" << std::endl;
            std::cout << "Group Size specified : " << localThreads[0] * localThreads[1] << std::endl;
            std::cout << "Max Group Size supported on the kernel : " 
                      << kernelInfo.kernelWorkGroupSize<<std::endl;
            return SDK_FAILURE;
        }
    }

    if(localThreads[0] > deviceInfo.maxWorkItemSizes[0] ||
       localThreads[1] > deviceInfo.maxWorkItemSizes[1] ||
       localThreads[0]*localThreads[1] > deviceInfo.maxWorkGroupSize)
    {
        std::cout << "Unsupported: Device does not support requested number of work items." << std::endl;
        return SDK_FAILURE;
    }

    //For small matrix sizes
    while(globalThreads[0] % localThreads[0])
        localThreads[0] /= 2;

    while(globalThreads[1] % localThreads[1])
        localThreads[1] /= 2;

    // Set appropriate arguments to the kernel
    
    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&inputBuffer0);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (outputBuffer)");
   
    status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&inputBuffer1);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (inputBuffer0)");
    
    status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&outputBuffer);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (inputBuffer1)");
   
    status = clSetKernelArg(kernel, 3, sizeof(cl_int),(void*)&width0);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (width0)");
    
    status = clSetKernelArg(kernel, 4, sizeof(cl_int), &width1);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (width1)");
   
    // Enqueue a kernel run call
    cl_event ndrEvt;
    status = clEnqueueNDRangeKernel(
                 commandQueue,
                 kernel,
                 2,
                 NULL,
                 globalThreads,
                 localThreads,
                 0,
                 NULL,
                 &ndrEvt);
    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");
   
    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");
   
    cl_int eventStatus = CL_QUEUED;
    while(eventStatus != CL_COMPLETE)
    {
        status = clGetEventInfo(
                        ndrEvt, 
                        CL_EVENT_COMMAND_EXECUTION_STATUS, 
                        sizeof(cl_int),
                        &eventStatus,
                        NULL);
        CHECK_OPENCL_ERROR(status, "clGetEventInfo failed.");
        
    }

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

    status = clReleaseEvent(ndrEvt);
    CHECK_OPENCL_ERROR(status, "clReleaseEvent failed.(ndrEvt)");
    
    // Print performance numbers
    double sec = 1e-9 * (endTime - startTime);
    std::cout << "KernelTime (ms) : " << sec * 1000 << std::endl;

    double flops = 2 * width0 * width1;
    double perf = (flops / sec) * height0 * 1e-9;
    
    std::cout << "GFlops achieved : " << perf << std::endl << std::endl;

    size_t origin[] = {0, 0, 0};
    size_t region[] = {width1 / 4, height0, 1};
    cl_event readEvt;
    status = clEnqueueReadImage(commandQueue,
                                outputBuffer,
                                CL_FALSE,
                                origin,
                                region,
                                0,
                                0,
                                output,
                                0,
                                NULL,
                                &readEvt);
    CHECK_OPENCL_ERROR(status, "outputBuffer failed.(clEnqueueReadImage)");
    
    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.(commandQueue)");
    
    status = sampleCommon->waitForEventAndRelease(&readEvt);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(readEvt) Failed");

    return SDK_SUCCESS;
}

/*
 * This is a naive O(N^3) CPU implementatio of matrix multiplication
 */
void 
MatrixMulImage::MatrixMulImageCPUReference(
    cl_float * output,
    cl_float * input0,
    cl_float * input1,
    const cl_uint y,
    const cl_uint x,
    const cl_uint z)
{
    for(cl_uint i = 0; i < y; i++)
    {
        for(cl_uint j = 0; j < z; j++)
        {
            for(cl_uint k = 0; k < x; k++)
            {
                output[i*z + j] += (input0[i * x + k] * input1[k * z + j]);
            }
        }
    }
}

int 
MatrixMulImage::initialize()
{
    // Call base class Initialize to get default configuration
    if(this->SDKSample::initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    // add an option for getting blockSize from commandline
    streamsdk::Option* xParam = new streamsdk::Option;
    CHECK_ALLOCATION(xParam, "Memory allocation error. (xParam)");
   
    xParam->_sVersion = "x";
    xParam->_lVersion = "height0";
    xParam->_description = "height of matrix A";
    xParam->_type     = streamsdk::CA_ARG_INT;
    xParam->_value    = &n;

    sampleArgs->AddOption(xParam);
    delete xParam;

    streamsdk::Option* yParam = new streamsdk::Option;
    CHECK_ALLOCATION(yParam, "Memory allocation error. (yParam)");
    

    yParam->_sVersion = "y";
    yParam->_lVersion = "width0";
    yParam->_description = "width of matrix A and Height of matrix B";
    yParam->_type     = streamsdk::CA_ARG_INT;
    yParam->_value    = &m;

    sampleArgs->AddOption(yParam);
    delete yParam;

    streamsdk::Option* zParam = new streamsdk::Option;
    CHECK_ALLOCATION(zParam, "Memory allocation error. (zParam)");
    

    zParam->_sVersion = "z";
    zParam->_lVersion = "width1";
    zParam->_description = "width of matrix B";
    zParam->_type     = streamsdk::CA_ARG_INT;
    zParam->_value    = &k;

    sampleArgs->AddOption(zParam);
    delete zParam;

    streamsdk::Option* num_iterations = new streamsdk::Option;
    CHECK_ALLOCATION(num_iterations, "Memory allocation error. (num_iterations)");
    

    num_iterations->_sVersion = "i";
    num_iterations->_lVersion = "iterations";
    num_iterations->_description = "Number of iterations for kernel execution";
    num_iterations->_type = streamsdk::CA_ARG_INT;
    num_iterations->_value = &iterations;

    sampleArgs->AddOption(num_iterations);
    delete num_iterations;

    return SDK_SUCCESS;
}

int 
MatrixMulImage::setup()
{  
    // Make sure the dimensions are multiples of blockSize
    const int vectorSize = 4;
    if(n % (blockSize * vectorSize) != 0)
    {
        n = (n / (blockSize * vectorSize) + 1) * (blockSize * vectorSize);
    }

    if(m % blockSize != 0)
    {
        m = (m / blockSize + 1) * blockSize;
    }

    if(k % (blockSize * vectorSize) != 0)
    {
       k = (k / (blockSize * vectorSize) + 1) * (blockSize * vectorSize);
    }

    width0  = m;
    height0 = n;
    
    width1  = k;
    height1 = m;

    if(setupMatrixMulImage() != SDK_SUCCESS)
        return SDK_FAILURE;

    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    int status = setupCL();
    if(status != SDK_SUCCESS)
    {
        if(status == SDK_EXPECTED_FAILURE)
            return SDK_EXPECTED_FAILURE;
        
        return SDK_FAILURE;
    }

    sampleCommon->stopTimer(timer);

    setupTime = (cl_double)sampleCommon->readTimer(timer);

    return SDK_SUCCESS;
}


int 
MatrixMulImage::run()
{
    std::cout << "Executing kernel for " << iterations 
              << " iterations" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);   

    for(int i = 0; i < iterations; i++)
    {
        // Arguments are set and execution call is enqueued on command buffer
        if(runCLKernels() != SDK_SUCCESS)
            return SDK_FAILURE;
    }

    sampleCommon->stopTimer(timer);
    totalKernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;

    if(!quiet) {
        sampleCommon->printArray<cl_float>("Output", output, width1, 1);
    }

    return SDK_SUCCESS;
}

int 
MatrixMulImage::verifyResults()
{
    if(verify)
    {
        // reference implementation
        int refTimer = sampleCommon->createTimer();
        sampleCommon->resetTimer(refTimer);
        sampleCommon->startTimer(refTimer);
        MatrixMulImageCPUReference(verificationOutput, 
                                              input0, 
                                              input1, 
                                              height0, 
                                              width0,  
                                              width1);
        sampleCommon->stopTimer(refTimer);
        referenceKernelTime = sampleCommon->readTimer(refTimer);

        // compare the results and see if they match
        if(sampleCommon->compare(output, verificationOutput, height0*width1))
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
MatrixMulImage::printStats()
{
    std::string strArray[4] = 
    {
        "MatrixA", 
        "MatrixB", 
        "Time(sec)", 
        "KernelTime(sec)"
    };
    std::string stats[4];

    totalTime = setupTime + totalKernelTime;

    stats[0]  = sampleCommon->toString(height0, std::dec)
                +"x"+sampleCommon->toString(width0, std::dec);
    stats[1]  = sampleCommon->toString(height1, std::dec)
                +"x"+sampleCommon->toString(width1, std::dec);
    stats[2]  = sampleCommon->toString(totalTime, std::dec);
    stats[3]  = sampleCommon->toString(totalKernelTime, std::dec);
    
    this->SDKSample::printStats(strArray, stats, 4);
}

int 
MatrixMulImage::cleanup()
{
    // Releases OpenCL resources (Context, Memory etc.
    cl_int status;

    status = clReleaseKernel(kernel);
    CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.(kernel)");
    
    status = clReleaseProgram(program);
    CHECK_OPENCL_ERROR(status, "clReleaseProgram failed.(program)");
   
    status = clReleaseMemObject(inputBuffer0);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(inputBuffer0)");
    
    status = clReleaseMemObject(inputBuffer1);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(inputBuffer1)");
    
    status = clReleaseMemObject(outputBuffer);
    CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed.(outputBuffer)");
    
    status = clReleaseCommandQueue(commandQueue);
    CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed.(commandQueue)");
    
    status = clReleaseContext(context);
    CHECK_OPENCL_ERROR(status, "clReleaseContext failed.(context)");
    
    // release program resources (input memory etc.)
    FREE(input0);

    FREE(input1);

    FREE(output);

    FREE(verificationOutput);

    // release device list
    FREE(devices);


    return SDK_SUCCESS;
}

int 
main(int argc, char * argv[])
{
    MatrixMulImage clMatrixMulImage("OpenCL Matrix Multiplication");

    if(clMatrixMulImage.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    if(clMatrixMulImage.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    if(clMatrixMulImage.isDumpBinaryEnabled())
    {
        return clMatrixMulImage.genBinaryImage();
    }
    else
    {
        int status = clMatrixMulImage.setup();
        if(status !=  SDK_SUCCESS)
            return (status == SDK_EXPECTED_FAILURE) ? SDK_SUCCESS : SDK_FAILURE;

        if(clMatrixMulImage.run() != SDK_SUCCESS)
            return SDK_FAILURE;

        if(clMatrixMulImage.verifyResults()==SDK_FAILURE)
                return SDK_FAILURE;

        if(clMatrixMulImage.cleanup()==SDK_FAILURE)
          return SDK_FAILURE;

        clMatrixMulImage.printStats();
    }
    return SDK_SUCCESS;
}
