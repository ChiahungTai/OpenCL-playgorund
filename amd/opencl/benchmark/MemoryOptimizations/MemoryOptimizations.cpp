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


#include "MemoryOptimizations.hpp"

//Separator
std::string sep = "-----------------------------------------";
int Iterations = 1000;
int 
MemoryOptimizations::setupMemoryOptimizations()
{
    // Allocate memory required foratmost vec4 elements 
    cl_uint size = length * sizeof(cl_float4);
    input = (cl_float*)malloc(size);
    CHECK_ALLOCATION(input, "Failed to allocate host memory. (input)");

    output = (cl_float*)malloc(size);
    CHECK_ALLOCATION(output, "Failed to allocate host memory. (output)");

    // random initialisation of input 
    sampleCommon->fillRandom<cl_float>(input,  4 * length, 1, 0, 10);

    return SDK_SUCCESS;
}

int 
MemoryOptimizations::LDSKernel(cl_kernel& kernel)
{
	globalThreads[0] = 1024 * 1024;
    globalThreads[1] = 1;

    localThreads[0] = 64;
    localThreads[1] = 1;

	int status = clSetKernelArg(kernel,
                            0,
                            sizeof(cl_float) * (32 * 64 + 256), 
                            0);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.(inputBuffer)");

    status = clSetKernelArg(kernel,
                            1,
                            sizeof(cl_mem), 
                            (void *)&outputBuffer);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.(outputBuffer)");

	double nsec = 0;

	for(int i = 0; i < Iterations; i++)
    {
        // Enqueue a kernel run call 
        cl_event ndrEvt;
        status = clEnqueueNDRangeKernel(commandQueue,
                                       kernel,
                                        2,
                                        NULL,
                                        globalThreads,
                                        localThreads,
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
        nsec += (endTime - startTime);

        status = clReleaseEvent(ndrEvt);
        CHECK_OPENCL_ERROR(status, "clReleaseEvent failed.(endTime)");
    }

	int numThreads = (int)(globalThreads[0] * globalThreads[1]);
    double bytes = (double)(Iterations * 256 * sizeof(cl_float));
    double perf = (bytes / nsec) * numThreads;

    std::cout << ": " << perf << " GB/s" << std::endl;
	return 0;
}

int 
MemoryOptimizations::genBinaryImage()
{
    streamsdk::bifData binaryData;
    binaryData.kernelName = std::string("MemoryOptimizations_Kernels.cl");
    binaryData.flagsStr = std::string("");
    if(isComplierFlagsSpecified())
        binaryData.flagsFileName = std::string(flags.c_str());

    binaryData.binaryName = std::string(dumpBinary.c_str());
    int status = sampleCommon->generateBinaryImage(binaryData);
    return status;
}


int
MemoryOptimizations::setupCL(void)
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
    status = sampleCommon->getDevices(context,&devices,deviceId,isDeviceIdEnabled());
    CHECK_ERROR(status, SDK_SUCCESS, "sampleCommon::getDevices() failed");

    //Set device info of given cl_device_id
    retValue = deviceInfo.setDeviceInfo(devices[deviceId]);
    CHECK_ERROR(retValue, SDK_SUCCESS, "SDKDeviceInfo::setDeviceInfo() failed");

    if(!strstr(deviceInfo.extensions, "cl_khr_global_int32_base_atomics"))
    {
        OPENCL_EXPECTED_ERROR("Device does not support global_int32_base_atomics!");
    }

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

    // Input buffer 
    inputBuffer = clCreateBuffer(context, 
                                 CL_MEM_READ_ONLY,
                                 sizeof(cl_float4) * length,
                                 0, 
                                 &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (inputBuffer)");

    // Write data to buffer 
    status = clEnqueueWriteBuffer(commandQueue,
                                  inputBuffer,
                                  1,
                                  0,
                                  sizeof(cl_float4) * length,
                                  input,
                                  0,
                                  0,
                                  0);
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (inputBuffer)");

    outputBuffer = clCreateBuffer(context, 
                                  CL_MEM_WRITE_ONLY,
                                  sizeof(cl_float4) * length,
                                  0, 
                                  &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (outputBuffer)");

    // create a CL program using the kernel source 
    streamsdk::buildProgramData buildData;
    buildData.kernelName = std::string("MemoryOptimizations_Kernels.cl");
    buildData.devices = devices;
    buildData.deviceId = deviceId;
    buildData.flagsStr = std::string("");
    if(isLoadBinaryEnabled())
        buildData.binaryName = std::string(loadBinary.c_str());

    if(isComplierFlagsSpecified())
        buildData.flagsFileName = std::string(flags.c_str());

    retValue = sampleCommon->buildOpenCLProgram(program, context, buildData);
    CHECK_ERROR(retValue, SDK_SUCCESS, "sampleCommon::buildOpenCLProgram() failed");

    // Copy 1D Fast Path 
    kernel[0] = clCreateKernel(program, "copy1DFastPath", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.(copy1DFastPath)");

    // Copy 1D Complete Path 
    kernel[1] = clCreateKernel(program, "copy1DCompletePath", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed. (copy1DCompletePath)");

    // Copy 2D float 
    kernel[2] = clCreateKernel(program, "copy2Dfloat", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed. (copy2Dfloat)");

    // Copy 2D float4 
    kernel[3] = clCreateKernel(program, "copy2Dfloat4", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed. (copy2Dfloat4)");

    // Copy 1D float4 
    kernel[4] = clCreateKernel(program, "copy1Dfloat4", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed. (copy1Dfloat4)");

    // Copy No Coalesced 
    kernel[5] = clCreateKernel(program, "NoCoal", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed. (NoCoal)");

    // Copy Split 
    kernel[6] = clCreateKernel(program, "Split", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed. (Split)");

	kernel[7] = clCreateKernel(program, "localBankConflicts", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed. (Split)");

	kernel[8] = clCreateKernel(program, "noLocalBankConflicts", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed. (Split)");

    return SDK_SUCCESS;
}

int
MemoryOptimizations::copy(cl_kernel& kernel, int vectorSize)
{
    cl_int status;

    // Check group size against kernelWorkGroupSize 
    status = clGetKernelWorkGroupInfo(kernel,
                                      devices[deviceId],
                                      CL_KERNEL_WORK_GROUP_SIZE,
                                      sizeof(size_t),
                                      &kernelWorkGroupSize,
                                      0);
    CHECK_OPENCL_ERROR(status, "clGetKernelWorkGroupInfo failed.");

    if(localThreads[0] * localThreads[1] > kernelWorkGroupSize)
    {
        std::cout << "\nDevice doesn't support required work-group size!\n";
        return SDK_SUCCESS;
    }

    // Set appropriate arguments to the kernel 

    status = clSetKernelArg(kernel,
                            0,
                            sizeof(cl_mem), 
                            (void *)&inputBuffer);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.(inputBuffer)");

    status = clSetKernelArg(kernel,
                            1,
                            sizeof(cl_mem), 
                            (void *)&outputBuffer);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed.(outputBuffer)");

    double nsec = 0;

    // Reduce the iterations if verification is enabled.
    if(verify)
        Iterations = 1;

    // Run the kernel for a number of iterations 
    for(int i = 0; i < Iterations; i++)
    {
        // Enqueue a kernel run call 
        cl_event ndrEvt;
        status = clEnqueueNDRangeKernel(commandQueue,
                                        kernel,
                                        2,
                                        NULL,
                                        globalThreads,
                                        localThreads,
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
        nsec += (endTime - startTime);

        status = clReleaseEvent(ndrEvt);
        CHECK_OPENCL_ERROR(status, "clReleaseEvent failed.(endTime)");
    }


    // Copy bytes 
    int numThreads = (int)(globalThreads[0] * globalThreads[1]);
    double bytes = (double)(Iterations * 2 * vectorSize * sizeof(cl_float));
    double perf = (bytes / nsec) * numThreads;

    std::cout << ": " << perf << " GB/s" << std::endl;

    if(verify)
    {
        // Enqueue readBuffer
        status = clEnqueueReadBuffer(commandQueue,
                                     outputBuffer,
                                     CL_TRUE,
                                     0,
                                     length * sizeof(cl_float4),
                                     output,
                                     0,
                                     NULL,
                                     0);
        CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");
        
        // Verify data 
        if(!memcmp(input, output, vectorSize * sizeof(cl_float) * length))
        {
            std::cout << "Passed!\n" << std::endl;
            return SDK_SUCCESS;
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
MemoryOptimizations::runCLKernels(void)
{
    int status;

    globalThreads[0] = length;
    globalThreads[1] = 1;

    localThreads[0] = GROUP_SIZE;
    localThreads[1] = 1;

    std::cout << sep << "\nCopy 1D FastPath\t";

    status = copy(kernel[0], 1);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << sep << "\nCopy 1D CompletePath\t";

    status = copy(kernel[1], 1);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << sep << "\nCopy 2D 32-bit (64x2)\t";

    globalThreads[0] = 1024;
    globalThreads[1] = 1024;

    localThreads[0] = 64;
    localThreads[1] = 2;

    status = copy(kernel[2], 1);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << "Copy 2D 128-bit (64x2)\t";
    status = copy(kernel[3], 4);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << sep << "\nCopy 2D 32-bit (64x4)\t";

    localThreads[0] = 64;
    localThreads[1] = 4;

    status = copy(kernel[2], 1);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << "Copy 2D 128-bit (64x4)\t";

    status = copy(kernel[3], 4);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << sep << "\nCopy 2D 32-bit (8x8)\t";

    localThreads[0] = 8;
    localThreads[1] = 8;

    status = copy(kernel[2], 1);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << "Copy 2D 128-bit (8x8)\t";

    status = copy(kernel[3], 4);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << sep << "\nCopy 2D 32-bit (256x1)\t";

    localThreads[0] = 256;
    localThreads[1] = 1;

    status = copy(kernel[2], 1);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << "Copy 2D 128-bit (256x1)\t";
    copy(kernel[3], 4);

    std::cout << sep << "\nCopy 2D 32-bit (32x2)\t";

    localThreads[0] = 32;
    localThreads[1] = 2;

    status = copy(kernel[2], 1);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << "Copy 2D 128-bit (32x2)\t";

    status = copy(kernel[3], 4);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << sep << "\nCopy 2D 32-bit (64x1)\t";

    localThreads[0] = 64;
    localThreads[1] = 1;

    status = copy(kernel[2], 1);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << "Copy 2D 128-bit (64x1)\t";

    status = copy(kernel[3], 4);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << sep << "\nCopy 2D 32-bit (16x16)\t";

    localThreads[0] = 16;
    localThreads[1] = 16;

    status = copy(kernel[2], 1);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << "Copy 2D 128-bit (16x16)\t";
    status = copy(kernel[3], 4);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << sep << "\nCopy 2D 32-bit (16x4)\t";

    localThreads[0] = 16;
    localThreads[1] = 4;

    status = copy(kernel[2], 1);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << "Copy 2D 128-bit (16x4)\t";

    status = copy(kernel[3], 4);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << sep << "\nCopy 2D 32-bit (1x64)\t";

    localThreads[0] = 1;
    localThreads[1] = 64;

    status = copy(kernel[2], 1);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << "Copy 2D 128-bit (1x64)\t";

    status = copy(kernel[3], 4);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << sep << "\nCopy 1D 128-bit \t";

    globalThreads[0] = length;
    globalThreads[1] = 1;

    localThreads[0] = 64;
    localThreads[1] = 1;

    status = copy(kernel[4], 4);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << sep << "\nNoCoal Copy 1D 32-bit \t";

    status = copy(kernel[5], 1);
    if(status != SDK_SUCCESS)
        return status;

    std::cout << sep << "\nSplit Copy 1D 32-bit \t";
    status = copy(kernel[6], 1);
    if(status != SDK_SUCCESS)
        return status;

	localThreads[0] = 64;
	std::cout << sep << "\nHasLocalBankConflicts 32-bit \t";
	status = LDSKernel(kernel[7]);
    if(status != SDK_SUCCESS)
        return status;

	std::cout << sep << "\nNoLocalBankConflicts 32-bit  \t";
	status = LDSKernel(kernel[8]);
    if(status != SDK_SUCCESS)
        return status;

    return SDK_SUCCESS;
}


int 
MemoryOptimizations::initialize()
{
    // Call base class Initialize to get default configuration 
    CHECK_ERROR(this->SDKSample::initialize(), SDK_SUCCESS, "OpenCL resource initilization failed");

    return SDK_SUCCESS;
}

int 
MemoryOptimizations::setup()
{
    CHECK_ERROR(setupMemoryOptimizations(), SDK_SUCCESS, "OpenCL resource setupMemoryOptimizations failed");
	
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
MemoryOptimizations::run()
{
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    // Arguments are set and execution call is enqueued on command buffer 
    int kernelRun = runCLKernels();
    if(kernelRun != SDK_SUCCESS)
        return kernelRun;

    sampleCommon->stopTimer(timer);
    totalKernelTime = (double)(sampleCommon->readTimer(timer));

    return SDK_SUCCESS;
}

int 
MemoryOptimizations::verifyResults()
{
    return SDK_SUCCESS;
}


int 
MemoryOptimizations::cleanup()
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

    status = clReleaseMemObject(outputBuffer);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(outputBuffer)");

    status = clReleaseCommandQueue(commandQueue);
    CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed.(commandQueue)");

    status = clReleaseContext(context);
    CHECK_OPENCL_ERROR(status, "clReleaseContext failed.(context)");

    // release program resources (input memory etc.) 
    FREE(input);
    FREE(output);
    FREE(devices);

    return SDK_SUCCESS;
}

int 
main(int argc, char * argv[])
{
    MemoryOptimizations clMemoryOptimizations("OpenCL Matrix Multiplication");

    // Initialize
    if(clMemoryOptimizations.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    if(clMemoryOptimizations.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    if(clMemoryOptimizations.isDumpBinaryEnabled())
    {
        return clMemoryOptimizations.genBinaryImage();
    }

    // Setup
    int state = clMemoryOptimizations.setup();
    if(state != SDK_SUCCESS)
    {
        if(state == SDK_EXPECTED_FAILURE)
            return SDK_SUCCESS;
        else
            return SDK_FAILURE;
    }

    // Run
    if (clMemoryOptimizations.run() != SDK_SUCCESS)
        return SDK_FAILURE;

    // VerifyResults
    if(clMemoryOptimizations.verifyResults() != SDK_SUCCESS)
        return SDK_FAILURE;

    // cleanup
    if(clMemoryOptimizations.cleanup() != SDK_SUCCESS)
        return SDK_FAILURE;

    return SDK_SUCCESS;
}
