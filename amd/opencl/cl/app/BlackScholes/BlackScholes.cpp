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


#include "BlackScholes.hpp"

#include <math.h>
#include <malloc.h>


//  Constants 
#define S_LOWER_LIMIT 10.0f
#define S_UPPER_LIMIT 100.0f
#define K_LOWER_LIMIT 10.0f
#define K_UPPER_LIMIT 100.0f
#define T_LOWER_LIMIT 1.0f
#define T_UPPER_LIMIT 10.0f
#define R_LOWER_LIMIT 0.01f
#define R_UPPER_LIMIT 0.05f
#define SIGMA_LOWER_LIMIT 0.01f
#define SIGMA_UPPER_LIMIT 0.10f

float 
BlackScholes::phi(float X)
{
    float y, absX, t;

    // the coeffs
    const float c1 =  0.319381530f;
    const float c2 = -0.356563782f;
    const float c3 =  1.781477937f;
    const float c4 = -1.821255978f;
    const float c5 =  1.330274429f;

    const float oneBySqrt2pi = 0.398942280f;

    absX = fabs(X);
    t = 1.0f / (1.0f + 0.2316419f * absX);

    y = 1.0f - oneBySqrt2pi * exp(-X * X / 2.0f) *
        t * (c1 +
                t * (c2 +
                    t * (c3 +
                        t * (c4 + t * c5))));

    return (X < 0) ? (1.0f - y) : y;
}

void 
BlackScholes::blackScholesCPU()
{
    int y;
    for (y = 0; y < width * height * 4; ++y)
    {
        float d1, d2;
        float sigmaSqrtT;
        float KexpMinusRT;
        float s = S_LOWER_LIMIT * randArray[y] + S_UPPER_LIMIT * (1.0f - randArray[y]);
        float k = K_LOWER_LIMIT * randArray[y] + K_UPPER_LIMIT * (1.0f - randArray[y]);
        float t = T_LOWER_LIMIT * randArray[y] + T_UPPER_LIMIT * (1.0f - randArray[y]);
        float r = R_LOWER_LIMIT * randArray[y] + R_UPPER_LIMIT * (1.0f - randArray[y]);
        float sigma = SIGMA_LOWER_LIMIT * randArray[y] + SIGMA_UPPER_LIMIT * (1.0f - randArray[y]);

        sigmaSqrtT = sigma * sqrt(t);

        d1 = (log(s / k) + (r + sigma * sigma / 2.0f) * t) / sigmaSqrtT;
        d2 = d1 - sigmaSqrtT;

        KexpMinusRT = k * exp(-r * t);
        hostCallPrice[y] = s * phi(d1) - KexpMinusRT * phi(d2);
        hostPutPrice[y]  = KexpMinusRT * phi(-d2) - s * phi(-d1);
    }
}



int 
BlackScholes::setupBlackScholes()
{
    int i = 0;

    // Calculate width and height from samples
    samples = samples / 4;
    samples = (samples / GROUP_SIZE)? (samples / GROUP_SIZE) * GROUP_SIZE: GROUP_SIZE;

    unsigned int tempVar1 = (unsigned int)sqrt((double)samples);
    tempVar1 = (tempVar1 / GROUP_SIZE)? (tempVar1 / GROUP_SIZE) * GROUP_SIZE: GROUP_SIZE;
    samples = tempVar1 * tempVar1;

    width = tempVar1;
    height = width;



#if defined (_WIN32)
    randArray = (cl_float*)_aligned_malloc(width * height * sizeof(cl_float4), 16);
#else
    randArray = (cl_float*)memalign(16, width * height * sizeof(cl_float4));
#endif
    CHECK_ALLOCATION(randArray, "Failed to allocate host memory. (randArray)");
    
    for(i = 0; i < width * height * 4; i++)
    {
        randArray[i] = (float)rand() / (float)RAND_MAX;
    }

    deviceCallPrice = (cl_float*)malloc(width * height * sizeof(cl_float4));
    CHECK_ALLOCATION(deviceCallPrice, "Failed to allocate host memory. (deviceCallPrice)");
    memset(deviceCallPrice, 0, width * height * sizeof(cl_float4));

    devicePutPrice = (cl_float*)malloc(width * height * sizeof(cl_float4));
    CHECK_ALLOCATION(devicePutPrice, "Failed to allocate host memory. (devicePutPrice)");   
    memset(devicePutPrice, 0, width * height * sizeof(cl_float4));

    hostCallPrice = (cl_float*)malloc(width * height * sizeof(cl_float4));
    CHECK_ALLOCATION(hostCallPrice, "Failed to allocate host memory. (hostCallPrice)");   
    memset(hostCallPrice, 0, width * height * sizeof(cl_float4));

    hostPutPrice = (cl_float*)malloc(width * height * sizeof(cl_float4));
    CHECK_ALLOCATION(hostPutPrice, "Failed to allocate host memory. (hostPutPrice)");   
    memset(hostPutPrice, 0, width * height * sizeof(cl_float4));

    return SDK_SUCCESS;
}

int 
BlackScholes::genBinaryImage()
{
    /*
     * Have a look at the available platforms and pick either
     * the AMD one if available or a reasonable default.
     */
    
    streamsdk::bifData binaryData;
    binaryData.kernelName = std::string("BlackScholes_Kernels.cl");
    binaryData.flagsStr = std::string("");
    if(isComplierFlagsSpecified())
        binaryData.flagsFileName = std::string(flags.c_str());

    binaryData.binaryName = std::string(dumpBinary.c_str());
    int status = sampleCommon->generateBinaryImage(binaryData);
    return status;
}


int
BlackScholes::setupCL(void)
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
    CHECK_ERROR(retValue, SDK_SUCCESS, "BlackScholes::getPlatform() failed");

    // Display available devices.
    retValue = sampleCommon->displayDevices(platform, dType);
    CHECK_ERROR(retValue, SDK_SUCCESS, "BlackScholes::::displayDevices() failed");

    // If we could find our platform, use it. Otherwise use just available platform.
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
    
    {
        // The block is to move the declaration of prop closer to its use
        cl_command_queue_properties prop = 0;
        commandQueue = clCreateCommandQueue(context, 
                                            devices[deviceId], 
                                            prop, 
                                            &status);
       CHECK_OPENCL_ERROR(status, "clCreateCommandQueue failed.");   
    }
    //Set device info of given cl_device_id
    retValue = deviceInfo.setDeviceInfo(devices[deviceId]);
    CHECK_ERROR(retValue, SDK_SUCCESS, "SDKDeviceInfo::setDeviceInfo() failed");

    
    // Set Presistent memory only for AMD platform
    cl_mem_flags inMemFlags = CL_MEM_READ_ONLY;
    if(isAmdPlatform())
        inMemFlags |= CL_MEM_USE_PERSISTENT_MEM_AMD;

    randBuf = clCreateBuffer(context, 
                             inMemFlags,
                             sizeof(cl_float4) * width  * height,
                             NULL, 
                             &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (randBuf)");   

    callPriceBuf = clCreateBuffer(context, 
                                  CL_MEM_WRITE_ONLY,
                                  sizeof(cl_float4) * width * height,
                                  NULL, 
                                  &status);
     CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (callPriceBuf)");

    putPriceBuf = clCreateBuffer(context, 
                                 CL_MEM_WRITE_ONLY,
                                 sizeof(cl_float4) * width * height,
                                 NULL, 
                                 &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (putPriceBuf)");

    // create a CL program using the kernel source 
    streamsdk::buildProgramData buildData;
    buildData.kernelName = std::string("BlackScholes_Kernels.cl");
    buildData.devices = devices;
    buildData.deviceId = deviceId;
    buildData.flagsStr = std::string("");
    if(isLoadBinaryEnabled())
        buildData.binaryName = std::string(loadBinary.c_str());

    if(isComplierFlagsSpecified())
        buildData.flagsFileName = std::string(flags.c_str());

    retValue = sampleCommon->buildOpenCLProgram(program, context, buildData);
    CHECK_ERROR(retValue, SDK_SUCCESS, "sampleCommon::buildOpenCLProgram() failed");


 
    // get a kernel object handle for a kernel with the given name
	if (useScalarKernel)
		kernel = clCreateKernel(program, "blackScholes_scalar", &status);
	else
		kernel = clCreateKernel(program, "blackScholes", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.(kernel)");
    
    status = kernelInfo.setKernelWorkGroupInfo(kernel, devices[deviceId]);
    CHECK_OPENCL_ERROR(status, "kernelInfo.setKernelWorkGroupInfo failed");

    // Calculte 2D block size according to required work-group size by kernel
    kernelInfo.kernelWorkGroupSize = kernelInfo.kernelWorkGroupSize > GROUP_SIZE ? GROUP_SIZE : kernelInfo.kernelWorkGroupSize;
    while((blockSizeX * blockSizeY) < kernelInfo.kernelWorkGroupSize)
    {
        bool next = false;
        if(2 * blockSizeX * blockSizeY <= kernelInfo.kernelWorkGroupSize)
        {
            blockSizeX <<= 1;
            next = true;
        }
        if(2 * blockSizeX * blockSizeY <= kernelInfo.kernelWorkGroupSize)
        {
            next = true;
            blockSizeY <<= 1;
        }

        // Break if no if statement is executed
        if(next == false)
            break;
    }

    return SDK_SUCCESS;
}


int 
BlackScholes::runCLKernels(void)
{
    cl_int   status;
    cl_event ndrEvt;
    size_t globalThreads[2] = {width, height};
	if (useScalarKernel) {
		globalThreads[0]*=4;
	}
    size_t localThreads[2] = {blockSizeX, blockSizeY};


    if(localThreads[0] > deviceInfo.maxWorkItemSizes[0] ||
       localThreads[1] > deviceInfo.maxWorkItemSizes[1] ||
       localThreads[0]*localThreads[1] > deviceInfo.maxWorkGroupSize) //(size_t)blockSizeX * blockSizeY
    {
        std::cout << "Unsupported: Device does not support" 
            "requested number of work items.";
        FREE(maxWorkItemSizes);
        return SDK_FAILURE;
    }
    
    cl_event inMapEvt;
    void* mapPtr = clEnqueueMapBuffer(
                        commandQueue,
                        randBuf,
                        CL_FALSE,
                        CL_MAP_WRITE,
                        0,
                        sizeof(cl_float4) * width  * height,
                        0,
                        NULL,
                        &inMapEvt,
                        &status);
    CHECK_OPENCL_ERROR(status, "clEnqueueMapBuffer failed. (mapPtr)");            
    
    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush(commandQueue) failed.");            
    
    status = sampleCommon->waitForEventAndRelease(&inMapEvt);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(inMapEvt) Failed");

    memcpy(mapPtr, randArray, sizeof(cl_float4) * width  * height);

    cl_event unmapEvent;
    status = clEnqueueUnmapMemObject(
                    commandQueue,
                    randBuf,
                    mapPtr,
                    0,
                    NULL,
                    &unmapEvent);
    CHECK_OPENCL_ERROR(status, "clEnqueueUnmapMemObject failed. (randBuf)");
   
    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed. (randBuf)");
    
    status = sampleCommon->waitForEventAndRelease(&unmapEvent);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(unmapEvent) Failed");

    // whether sort is to be in increasing order. CL_TRUE implies increasing
    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&randBuf); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (randBuf)");
   
	int rowStride = useScalarKernel?width*4:width;
    status = clSetKernelArg(kernel, 1, sizeof(rowStride), (const void *)&rowStride);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (width)");
    

    status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&callPriceBuf);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (callPriceBuf)");
    
    status = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&putPriceBuf);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (putPriceBuf)");

    // Enqueue a kernel run call.
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
    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");
  
    status = sampleCommon->waitForEventAndRelease(&ndrEvt);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(ndrEvt) Failed");

    cl_event callEvent;
    cl_event putEvent;

    // Enqueue the results to application pointer
    status = clEnqueueReadBuffer(commandQueue, 
                                 callPriceBuf, 
                                 CL_FALSE,
                                 0,
                                 width * height * sizeof(cl_float4),
                                 deviceCallPrice,
                                 0,
                                 NULL,
                                 &callEvent);
    CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

    // Enqueue the results to application pointer
    status = clEnqueueReadBuffer(commandQueue, 
                                 putPriceBuf, 
                                 CL_FALSE,
                                 0,
                                 width * height * sizeof(cl_float4),
                                 devicePutPrice,
                                 0,
                                 NULL,
                                 &putEvent);
    CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");
   
    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush(commanQueue) failed.");
   
    status = sampleCommon->waitForEventAndRelease(&callEvent);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(callEvent) Failed");

    status = sampleCommon->waitForEventAndRelease(&putEvent);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(putEvent) Failed");

    return SDK_SUCCESS;
}

int 
BlackScholes::initialize()
{
    // Call base class Initialize to get default configuration
    CHECK_ERROR((this->SDKSample::initialize()), SDK_SUCCESS, "OpenCL resource initialization  failed");
    streamsdk::Option* num_samples = new streamsdk::Option;
    CHECK_ALLOCATION(num_samples, "Failed to intialization of class. (num_samples)");
    num_samples->_sVersion = "x";
    num_samples->_lVersion = "samples";
    num_samples->_description = "Number of samples to be calculated";
    num_samples->_type = streamsdk::CA_ARG_INT;
    num_samples->_value = &samples;

    sampleArgs->AddOption(num_samples);

    num_samples->_sVersion = "i";
    num_samples->_lVersion = "iterations";
    num_samples->_description = "Number of iterations";
    num_samples->_type = streamsdk::CA_ARG_INT;
    num_samples->_value = &iterations;
    
    sampleArgs->AddOption(num_samples);

    num_samples->_sVersion = "s";
    num_samples->_lVersion = "scalar";
    num_samples->_description = "Run the scalar kernel instead of the vectoroized kernel";
    num_samples->_type = streamsdk::CA_NO_ARGUMENT;
    num_samples->_value = &useScalarKernel;
    
    sampleArgs->AddOption(num_samples);

    delete num_samples;

    return SDK_SUCCESS;
}

int 
BlackScholes::setup()
{
    if(setupBlackScholes() != SDK_SUCCESS)
        return SDK_FAILURE;

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


int
BlackScholes::run()
{

    // Warm up
    for(int i = 0; i < 2 && iterations != 1; i++)
    {
        // Arguments are set and execution call is enqueued on command buffer
        if(runCLKernels() != SDK_SUCCESS)
            return SDK_FAILURE;
      }
    
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    std::cout << "Executing kernel for " << iterations 
        << " iterations" << std::endl;
    std::cout <<"-------------------------------------------" << std::endl;

    for(int i = 0; i < iterations; i++)
    {
        // Arguments are set and execution call is enqueued on command buffer
        if(runCLKernels() != SDK_SUCCESS)
            return SDK_FAILURE;
    }
    
    sampleCommon->stopTimer(timer);
    // Compute kernel time
    kernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;

    if(!quiet)
    {
        sampleCommon->printArray<cl_float>("deviceCallPrice",
                                           deviceCallPrice, 
                                           width, 
                                           1);

        sampleCommon->printArray<cl_float>("devicePutPrice", 
                                           devicePutPrice, 
                                           width, 
                                           1);
    }
    return SDK_SUCCESS;
}

int 
BlackScholes::verifyResults()
{
    if(verify)
    {
        /* reference implementation
         * it overwrites the input array with the output
         */
        blackScholesCPU();

        if(!quiet)
        {
            sampleCommon->printArray<cl_float>("hostCallPrice",
                                               hostCallPrice, 
                                               width, 
                                               1);

            sampleCommon->printArray<cl_float>("hostPutPrice", 
                                               hostPutPrice, 
                                               width, 
                                               1);
        }

        
        // compare the call/put price results and see if they match
        bool callPriceResult = sampleCommon->compare(hostCallPrice, deviceCallPrice, width * height * 4);
        bool putPriceResult = sampleCommon->compare(hostPutPrice, devicePutPrice, width * height * 4, 1e-4f);

        if(!(callPriceResult ? (putPriceResult ? true : false) : false))
        {
            std::cout << "Failed\n" << std::endl;
            return SDK_FAILURE;
        }
        else
        {
            std::cout << "Passed!\n" << std::endl;
            return SDK_SUCCESS;
        }
    }

    return SDK_SUCCESS;
}

void 
BlackScholes::printStats()
{
    int actualSamples = width * height * 4;
    totalTime = setupTime + kernelTime;

    std::string strArray[4] = 
    {
        "Option Samples", 
        "Time(sec)", 
        "[Transfer+kernel]Time(sec)", 
        "Options/sec"
    };

    std::string stats[4];
    stats[0] = sampleCommon->toString(actualSamples, std::dec);
    stats[1] = sampleCommon->toString(totalTime, std::dec);
    stats[2] = sampleCommon->toString(kernelTime, std::dec);
    stats[3] = sampleCommon->toString(actualSamples / totalTime, std::dec);

    this->SDKSample::printStats(strArray, stats, 4);

}


int BlackScholes::cleanup()
{
    // Releases OpenCL resources (Context, Memory etc.) 
    cl_int status;
    status = clReleaseMemObject(randBuf);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject(randBuf) failed.");
    
    status = clReleaseMemObject(callPriceBuf);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject(callPriceBuf) failed."); 
    
    status = clReleaseMemObject(putPriceBuf);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject(callPriceBuf) failed.");

    status = clReleaseKernel(kernel);
    CHECK_OPENCL_ERROR(status, "clReleaseKernel(kernel) failed.");

    status = clReleaseProgram(program);
    CHECK_OPENCL_ERROR(status, "clReleaseProgram(program) failed.");

    status = clReleaseCommandQueue(commandQueue);
    CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue(commandQueue) failed.");

    status = clReleaseContext(context);
    CHECK_OPENCL_ERROR(status, "clReleaseContext(context) failed.");

    // Release program resources (input memory etc.)

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

    return SDK_SUCCESS;
}

int 
main(int argc, char * argv[])
{
    // Create MonteCalroAsian object
    BlackScholes clBlackScholes("Black Scholes OpenCL sample");
    
    // Initialization
    if(clBlackScholes.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    // Parse command line options
    if(clBlackScholes.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    if(clBlackScholes.isDumpBinaryEnabled())
    {
        return clBlackScholes.genBinaryImage();
    }
    else
    {
        // Setup
        if(clBlackScholes.setup() !=SDK_SUCCESS)
            return SDK_FAILURE;

        // Run 
        if(clBlackScholes.run() !=SDK_SUCCESS)
            return SDK_FAILURE;

        // Verifty
        if(clBlackScholes.verifyResults() !=SDK_SUCCESS)
            return SDK_FAILURE;

        // Cleanup resources created
        if(clBlackScholes.cleanup() !=SDK_SUCCESS)
            return SDK_FAILURE;

        // Print performance statistics 
        clBlackScholes.printStats();
    }

    return SDK_SUCCESS;
}
