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


#include "HistogramAtomics.hpp"

#include <math.h>

void 
Histogram::calculateHostBin()
{
    // compute CPU histogram
    cl_int *p = (cl_int*)input;

    memset(cpuhist, 0, sizeof(cl_uint) * NBINS);

    for(unsigned int i = 0; i < inputNBytes / sizeof(cl_uint); i++) 
    {
        cpuhist[ (p[i] >> 24) & 0xff ]++;
        cpuhist[ (p[i] >> 16) & 0xff ]++;
        cpuhist[ (p[i] >> 8) & 0xff ]++;
        cpuhist[ (p[i] >> 0) & 0xff ]++;
    }
}

int
Histogram::setupHistogram()
{
	
    cl_int status = 0;

    nThreads =          64 * 1024;//64*1024
    n4Vectors =          2048 * 2048;
    n4VectorsPerThread = n4Vectors / nThreads;
    inputNBytes =        n4Vectors * sizeof(cl_uint4);

    input =  (cl_uint*)malloc(inputNBytes);
    CHECK_ALLOCATION(input, "Failed to allocate host memory. (input)");

    // random initialization of input 
    time_t ltime;
    time(&ltime);
    cl_uint a = (cl_uint) ltime, b = (cl_uint) ltime;
    cl_uint *p = (cl_uint *) input;

    for(unsigned int i = 0; i < inputNBytes / sizeof(cl_uint); i++)
        p[i] = ( b = ( a * ( b & 65535 )) + ( b >> 16 ));
	

    return SDK_SUCCESS;
}

int 
Histogram::genBinaryImage()
{
    streamsdk::bifData binaryData;
    binaryData.kernelName = std::string("HistogramAtomics_Kernels.cl");
    binaryData.flagsStr = std::string("");
    if(isComplierFlagsSpecified())
        binaryData.flagsFileName = std::string(flags.c_str());

    binaryData.binaryName = std::string(dumpBinary.c_str());
    int status = sampleCommon->generateBinaryImage(binaryData);
    return status;
}

int
Histogram::setupCL(void)
{
	
    cl_int status = CL_SUCCESS;
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

    // If we could find our platform, use it. Otherwise use just available platform.
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
    CHECK_OPENCL_ERROR( status, "clCreateContextFromType failed.");

    // getting device on which to run the sample
    status = sampleCommon->getDevices(context, &devices, deviceId, isDeviceIdEnabled());
    CHECK_ERROR(status, SDK_SUCCESS, "sampleCommon::getDevices() failed");

    {
        // The block is to move the declaration of prop closer to its use 
        cl_command_queue_properties prop = 0;
        commandQueue = clCreateCommandQueue(
                context, 
                devices[deviceId], 
                prop, 
                &status);
        CHECK_OPENCL_ERROR( status, "clCreateCommandQueue failed.");
    }

    //Set device info of given cl_device_id
    retValue = deviceInfo.setDeviceInfo(devices[deviceId]);
    CHECK_ERROR(retValue, 0, "SDKDeviceInfo::setDeviceInfo() failed");

    // Check if byte-addressable store is supported 
    if(!strstr(deviceInfo.extensions, "cl_khr_byte_addressable_store"))
    {
        reqdExtSupport = false;
        OPENCL_EXPECTED_ERROR("Device does not support cl_khr_byte_addressable_store extension!!");
    }

    // Check if byte-addressable store is supported 
    if(!strstr(deviceInfo.extensions, "cl_khr_global_int32_base_atomics"))
    {
        reqdExtSupport = false;
        OPENCL_EXPECTED_ERROR("Device does not support global_int32_base_atomics extension!");
    }
    // Check if byte-addressable store is supported 
    if(!strstr(deviceInfo.extensions, "cl_khr_local_int32_base_atomics"))
    {
        reqdExtSupport = false;
        OPENCL_EXPECTED_ERROR("Device does not support local_int32_base_atomics extension!");
    }

    // Create input buffer
    inputBuffer = clCreateBuffer(context, 
        CL_MEM_READ_ONLY,
        inputNBytes,
        NULL, 
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (inputBuffer)");

    // create a CL program using the kernel source 
    streamsdk::buildProgramData buildData;
    buildData.kernelName = std::string("HistogramAtomics_Kernels.cl");
    buildData.devices = devices;
    buildData.deviceId = deviceId;
    buildData.flagsStr = std::string("");
    if(isLoadBinaryEnabled())
        buildData.binaryName = std::string(loadBinary.c_str());

    if(isComplierFlagsSpecified())
        buildData.flagsFileName = std::string(flags.c_str());

    retValue = sampleCommon->buildOpenCLProgram(program, context, buildData);
    CHECK_ERROR(retValue, 0, "sampleCommon::buildOpenCLProgram() failed");

    // get a kernel object handle for a kernel with the given name 
    histogram = clCreateKernel(program, "histogramKernel", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

    // get a kernel object handle for a kernel with the given name 
    reduce = clCreateKernel(program, "reduceKernel", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

    status =  kernelInfoReduce.setKernelWorkGroupInfo(reduce,devices[deviceId]);
    CHECK_ERROR(status, SDK_SUCCESS, "setKErnelWorkGroupInfo() failed");

    status =  kernelInfoHistogram.setKernelWorkGroupInfo(histogram,devices[deviceId]);
    CHECK_ERROR(status, SDK_SUCCESS, "setKErnelWorkGroupInfo() failed");

    // histogramKernel work group size must be integer multiple of 256
    if(kernelInfoHistogram.kernelWorkGroupSize % 256 != 0)
    {
        OPENCL_EXPECTED_ERROR("Device does not support work-group size of 256 on histogram kernel. Exiting!");
    }

    nThreadsPerGroup =   (cl_uint) kernelInfoHistogram.compileWorkGroupSize[0];
    nGroups =            nThreads / nThreadsPerGroup;
    outputNBytes =       nGroups * NBINS * sizeof(cl_uint);

    output = (cl_uint*)malloc(outputNBytes);
    CHECK_ALLOCATION(output, "Failed to allocate host memory. (output)");

    // Create output Buffer
    outputBuffer = clCreateBuffer( context, 
        CL_MEM_READ_WRITE,
        outputNBytes,
        0, 
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (outputBuffer)");
	
    return SDK_SUCCESS;

	
}
int 
Histogram::runCLKernels(void)
{
	
    cl_int status;
    cl_int eventStatus = CL_QUEUED;
    size_t globalThreads[3] = {1};
    size_t localThreads[3] = {1};
	size_t globalThreadsReduce = NBINS;
	size_t localThreadsReduce = nThreadsPerGroup;

    globalThreads[0] = nThreads;
    localThreads[0]  = nThreadsPerGroup;

    cl_event writeEvt;
    status = clEnqueueWriteBuffer(commandQueue,
                                  inputBuffer,
                                  CL_FALSE,
                                  0,
                                  inputNBytes,
                                  input,
                                  0,
                                  NULL,
                                  &writeEvt);
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (inputBuffer)");

    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    status = sampleCommon->waitForEventAndRelease(&writeEvt);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(writeEvt) Failed");

    int Arg = 0;

    // __global input & output
    status = clSetKernelArg(histogram, 
                            Arg++, 
                            sizeof(cl_mem), 
                            (void *)&inputBuffer);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (inputBuffer)");

    status |= clSetKernelArg(histogram, 
                             Arg++, 
                             sizeof(cl_mem), 
                             (void *)&outputBuffer);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (outputBuffer)");

    status |= clSetKernelArg(histogram, 
                             Arg++, 
                             sizeof(n4VectorsPerThread), 
                             (void *)&n4VectorsPerThread);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (n4VectorsPerThread)");

    // reduceKernel

    Arg = 0;
    status |= clSetKernelArg(reduce, 
                             Arg++, 
                             sizeof(cl_mem), 
                             (void *)&outputBuffer);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (outputBuffer)");

    status |= clSetKernelArg(reduce, 
                             Arg++, 
                             sizeof(nGroups), 
                             (void *)&nGroups);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (nGroups)");

	
    /* 
    * Enqueue a kernel run call.
    */
    cl_event ndrEvt1;
    cl_event ndrEvt2;
	
    status = clEnqueueNDRangeKernel(commandQueue,
                                    histogram,
                                    1,
                                    NULL,
                                    globalThreads,
                                    localThreads,
                                    0,
                                    NULL,
                                    &ndrEvt1);
    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed. (histogram)");

    status = clEnqueueNDRangeKernel(commandQueue,
                                    reduce,
                                    1,
                                    NULL,
                                    &globalThreadsReduce,
                                    &localThreadsReduce,
                                    1,
                                    &ndrEvt1,
                                    &ndrEvt2);
    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed. (reduce)");

    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    status = sampleCommon->waitForEventAndRelease(&ndrEvt1);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(ndrEvt1) Failed");

    status = sampleCommon->waitForEventAndRelease(&ndrEvt2);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(ndrEvt2) Failed");

	

    // Enqueue the results to application pointer
    cl_event readEvt;
    status = clEnqueueReadBuffer(commandQueue,
                                 outputBuffer,
                                 CL_FALSE,
                                 0,
                                /*outputNBytes*/256 * sizeof(cl_uint),
                                 output,
                                 0,
                                 NULL,
                                 &readEvt);
    CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    status = sampleCommon->waitForEventAndRelease(&readEvt);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(readEvt) Failed");
	
    
    return SDK_SUCCESS;
}

int
Histogram::initialize()
{
    // Call base class Initialize to get default configuration
    if(this->SDKSample::initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    streamsdk::Option* iteration_option = new streamsdk::Option;
    CHECK_ALLOCATION(iteration_option, "Memory allocation error.\n");

    iteration_option->_sVersion = "i";
    iteration_option->_lVersion = "iterations";
    iteration_option->_description = "Number of iterations to execute kernel";
    iteration_option->_type = streamsdk::CA_ARG_INT;
    iteration_option->_value = &iterations;

    sampleArgs->AddOption(iteration_option);
    delete iteration_option;

    return SDK_SUCCESS;
}

int 
Histogram::setup()
{
	if(iterations < 1)
	{
		std::cout<<"Error, iterations cannot be 0 or negative. Exiting..\n";
		exit(0);
	}
    if(setupHistogram() != SDK_SUCCESS)
        return SDK_FAILURE;

    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    int status = setupCL();
    if(status != SDK_SUCCESS)
        return status;

    sampleCommon->stopTimer(timer);
    // Compute setup time 
    setupTime = (double)(sampleCommon->readTimer(timer));

    return SDK_SUCCESS;
}


int 
Histogram::run()
{
    if(!reqdExtSupport)
        return SDK_SUCCESS;

    for(int i = 0; i < 2 && iterations != 1; i++)
    {
        // Arguments are set and execution call is enqueued on command buffer 
        if(runCLKernels() != SDK_SUCCESS)
            return SDK_FAILURE;
    }

    if(!quiet)
    {
        std::cout << "Executing kernel for "
            << iterations << " iterations" << std::endl;
        std::cout << "-------------------------------------------" << std::endl;
    }

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
    // Compute average kernel time 
    kernelTimeGlobal = (double)(sampleCommon->readTimer(timer)) / iterations;

    if(!quiet)
        sampleCommon->printArray<cl_uint>("Output", output, NBINS, 1);

    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    return SDK_SUCCESS;
}

int
Histogram::verifyResults()
{
    if(!reqdExtSupport)
        return SDK_SUCCESS;

    if(verify)
    {
        /* reference implementation on host device
         * calculates the histogram bin on host
         */
        calculateHostBin();

        // compare the results and see if they match 
        bool flag = true;
        for(cl_uint i = 0; i < NBINS; ++i)
        {
            if(cpuhist[i] != output[i])
            {
                flag = false;
                break;
            }
        }

        if(flag)
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

void Histogram::printStats()
{
    if(!reqdExtSupport)
        return;

    // calculate total time 
    totalTime = setupTime + kernelTimeGlobal;

    std::string strArray[3] = 
    {
        "Input", 
        "Time(sec)", 
        "[Transfer+Kernel]Time(sec)"
    };
    std::string stats[3];

    cl_int elem = inputNBytes / sizeof(cl_uint);
    stats[0] = sampleCommon->toString(elem, std::dec);
    stats[1] = sampleCommon->toString(totalTime, std::dec);
    stats[2] = sampleCommon->toString(kernelTimeGlobal, std::dec);

    this->SDKSample::printStats(strArray, stats, 3);

    totalTime = setupTime + kernelTimeLocal;

}


int Histogram::cleanup()
{
    if(!reqdExtSupport)
        return SDK_SUCCESS;

    // Releases OpenCL resources (Context, Memory etc.) 
    cl_int status;

    status = clReleaseMemObject(inputBuffer);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(inputBuffer )");

    status = clReleaseMemObject(outputBuffer);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(outputBuffer)");

    status = clReleaseKernel(histogram);
    CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.(histogram)");

    status = clReleaseKernel(reduce);
    CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.(reduce)");

    status = clReleaseProgram(program);
    CHECK_OPENCL_ERROR(status, "clReleaseProgram failed.(program)");

    status = clReleaseCommandQueue(commandQueue);
    CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed.(commandQueue)");

    status = clReleaseContext(context);
    CHECK_OPENCL_ERROR(status, "clReleaseContext failed.(context)");

    return SDK_SUCCESS;
}

int 
main(int argc, char * argv[])
{
    // Create MonteCalroAsian object
    Histogram clHistogram("HistogramAtomics OpenCL sample");

    // Initialization
    if(clHistogram.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    // Parse command line options
    if(clHistogram.parseCommandLine(argc, argv) != SDK_SUCCESS)
        return SDK_FAILURE;

    if(clHistogram.isDumpBinaryEnabled())
    {
        return clHistogram.genBinaryImage();
    }
    
    // Setup
    int status = clHistogram.setup();
    if(status != SDK_SUCCESS)
    {
        if(status == SDK_EXPECTED_FAILURE)
            return SDK_SUCCESS;
        else
            return SDK_FAILURE;
    }

    // Run 
    if(clHistogram.run() != SDK_SUCCESS)
        return SDK_FAILURE;

    // Verify
    if(clHistogram.verifyResults() != SDK_SUCCESS)
        return SDK_FAILURE;

    // Cleanup resources created 
    if(clHistogram.cleanup() != SDK_SUCCESS)
        return SDK_FAILURE;

    // Print performance statistics 
    clHistogram.printStats();

    return SDK_SUCCESS;
}
