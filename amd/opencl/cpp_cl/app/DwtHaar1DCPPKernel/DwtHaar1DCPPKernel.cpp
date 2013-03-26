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


#include "DwtHaar1DCPPKernel.hpp"
#include <math.h>

int 
DwtHaar1D::calApproxFinalOnHost()
{
    // Copy inData to hOutData
    cl_float *tempOutData = (cl_float*)malloc(signalLength * sizeof(cl_float));
    CHECK_ALLOCATION(tempOutData, "Failed to allocate host memory. (tempOutData)");

    memcpy(tempOutData, inData, signalLength * sizeof(cl_float));

    for(cl_uint i = 0; i < signalLength; ++i)
    {
        tempOutData[i] = tempOutData[i] / sqrt((float)signalLength);
    }

    cl_uint length = signalLength;
    while(length > 1u)
    {
        for(cl_uint i = 0; i < length / 2; ++i)
        {
            cl_float data0 = tempOutData[2 * i];
            cl_float data1 = tempOutData[2 * i + 1];

            hOutData[i] = (data0 + data1) / sqrt((float)2);
            hOutData[length / 2 + i] = (data0 - data1) / sqrt((float)2);
        }
        // Copy inData to hOutData
        memcpy(tempOutData, hOutData, signalLength * sizeof(cl_float));

        length >>= 1;
    }

    FREE(tempOutData);
    return SDK_SUCCESS;
}

int 
DwtHaar1D::getLevels(unsigned int length, unsigned int* levels)
{
    cl_int returnVal = SDK_FAILURE;

    for(unsigned int i = 0; i < 24; ++i)
    {
        if(length == (1 << i))
        {
            *levels = i;
            returnVal = SDK_SUCCESS;
            break;
        }
    }

    return returnVal;
}

int DwtHaar1D::setupDwtHaar1D()
{
    // signal length must be power of 2
    signalLength = sampleCommon->roundToPowerOf2<cl_uint>(signalLength);

    unsigned int levels = 0;
    int result = getLevels(signalLength, &levels);
    CHECK_ERROR(result,SDK_SUCCESS, "signalLength > 2 ^ 23 not supported");

    // Allocate and init memory used by host
    inData = (cl_float*)malloc(signalLength * sizeof(cl_float));
    CHECK_ALLOCATION(inData, "Failed to allocate host memory. (inData)");

    for(unsigned int i = 0; i < signalLength; i++)
    {
        inData[i] = (cl_float)(rand() % 10);
    }

    dOutData = (cl_float*) malloc(signalLength * sizeof(cl_float));
    CHECK_ALLOCATION(dOutData, "Failed to allocate host memory. (dOutData)");

    memset(dOutData, 0, signalLength * sizeof(cl_float));

    dPartialOutData = (cl_float*) malloc(signalLength * sizeof(cl_float));
    CHECK_ALLOCATION(dPartialOutData, "Failed to allocate host memory.(dPartialOutData)");

    memset(dPartialOutData, 0, signalLength * sizeof(cl_float));

    hOutData = (cl_float*)malloc(signalLength * sizeof(cl_float));
    CHECK_ALLOCATION(hOutData, "Failed to allocate host memory. (hOutData)");

    memset(hOutData, 0, signalLength * sizeof(cl_float));

    if(!quiet)
    {
        sampleCommon->printArray<cl_float>("Input Signal", inData, 256, 1);
    }
	

    return SDK_SUCCESS;
}

int 
DwtHaar1D::genBinaryImage()
{
    streamsdk::bifData binaryData;
    binaryData.kernelName = std::string("DwtHaar1DCPPKernel_Kernels.cl");
    binaryData.flagsStr = std::string("-x clc++ ");
    if(isComplierFlagsSpecified())
        binaryData.flagsFileName = std::string(flags.c_str());

    binaryData.binaryName = std::string(dumpBinary.c_str());
    int status = sampleCommon->generateBinaryImage(binaryData);
    return status;
}


int
DwtHaar1D::setupCL(void)
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


    commandQueue = clCreateCommandQueue(context, 
        devices[deviceId], 
        0, 
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateCommandQueue failed.");

    //Set device info of given cl_device_id
    retValue = deviceInfo.setDeviceInfo(devices[deviceId]);
    CHECK_ERROR(retValue, 0, "SDKDeviceInfo::setDeviceInfo() failed");

    // Set Presistent memory only for AMD platform
    cl_mem_flags inMemFlags = CL_MEM_READ_ONLY;
    if(isAmdPlatform())
        inMemFlags |= CL_MEM_USE_PERSISTENT_MEM_AMD;

    inDataBuf = clCreateBuffer(context,
        inMemFlags,
        sizeof(cl_float) * signalLength,
        NULL,
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (inDataBuf)");

    dOutDataBuf = clCreateBuffer(context, 
        CL_MEM_WRITE_ONLY,
        signalLength * sizeof(cl_float),
        NULL, 
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (dOutDataBuf)");

    dPartialOutDataBuf = clCreateBuffer(context, 
        CL_MEM_WRITE_ONLY,
        signalLength * sizeof(cl_float),
        NULL, 
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (dPartialOutDataBuf)");

    // create a CL program using the kernel source 
    streamsdk::buildProgramData buildData;
    buildData.kernelName = std::string("DwtHaar1DCPPKernel_Kernels.cl");
    buildData.devices = devices;
    buildData.deviceId = deviceId;
    buildData.flagsStr = std::string("-x clc++ ");
    if(isLoadBinaryEnabled())
        buildData.binaryName = std::string(loadBinary.c_str());

    if(isComplierFlagsSpecified())
        buildData.flagsFileName = std::string(flags.c_str());

    retValue = sampleCommon->buildOpenCLProgram(program, context, buildData);
    CHECK_ERROR(retValue, 0, "sampleCommon::buildOpenCLProgram() failed");

    // get a kernel object handle for a kernel with the given name 
    kernel = clCreateKernel(program, "dwtHaar1D", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

    status = kernelInfo.setKernelWorkGroupInfo(kernel,devices[deviceId]);
    CHECK_ERROR(status, SDK_SUCCESS, " setKernelWorkGroupInfo() failed");

    return SDK_SUCCESS;
}

int DwtHaar1D::setWorkGroupSize()
{
    cl_int status = 0;

    globalThreads = curSignalLength >> 1;
    localThreads = groupSize;

    if(localThreads > deviceInfo.maxWorkItemSizes[0] || localThreads > deviceInfo.maxWorkGroupSize)
    {
        std::cout << "Unsupported: Device does not support"
                     "requested number of work items.";
        return SDK_FAILURE;
    }

    if(kernelInfo.localMemoryUsed > deviceInfo.localMemSize)
    {
        std::cout << "Unsupported: Insufficient local memory on device." << 
            std::endl;
        return SDK_FAILURE;
    }
    return SDK_SUCCESS;
}
int DwtHaar1D::runDwtHaar1DKernel()
{
	
    cl_int status;
	
    status = this->setWorkGroupSize();
    CHECK_ERROR(status, SDK_SUCCESS, "setWorkGroupSize failed");

    // Force write to inData Buf to update its values 
    cl_event writeEvt;
    status = clEnqueueWriteBuffer(
                commandQueue,
                inDataBuf,
                CL_FALSE,
                0,
                curSignalLength * sizeof(cl_float),
                inData,
                0,
                NULL,
                &writeEvt);
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (inDataBuf)");

    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    status = sampleCommon->waitForEventAndRelease(&writeEvt);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(writeEvt1) Failed");

	ParaClass *paraClass = new ParaClass;//new a paraclass

	this->classObj = clCreateBuffer(context,CL_MEM_USE_HOST_PTR,sizeof(ParaClass),paraClass,&status);
	 CHECK_OPENCL_ERROR(status, "clclCreateBuffer failed. (inDataBuf)");

	cl_event mapEvt;

	paraClass=(ParaClass *)clEnqueueMapBuffer(commandQueue,this->classObj,CL_FALSE,CL_MAP_WRITE,0,sizeof(ParaClass),0,NULL,&mapEvt,&status);

	 CHECK_OPENCL_ERROR(status, "clEnqueueMapBuffer failed. (classObj)");

    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    status = sampleCommon->waitForEventAndRelease(&mapEvt);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(mapEvt1) Failed");

	paraClass->setValue(this->totalLevels,this->curSignalLength,this->levelsDone,this->maxLevelsOnDevice);
	
	 cl_event unmapEvt;
	 status=clEnqueueUnmapMemObject(commandQueue,this->classObj,paraClass,0,NULL,&unmapEvt);//class is passed to the Device

	  CHECK_OPENCL_ERROR(status, "clEnqueueunMapBuffer failed. (classObj)");

    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    status = sampleCommon->waitForEventAndRelease(&unmapEvt);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(mapEvt1) Failed");


    // Whether sort is to be in increasing order. CL_TRUE implies increasing 
    status = clSetKernelArg(kernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void*)&inDataBuf); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (inDataBuf)");

    status = clSetKernelArg(kernel, 
                            1, 
                            sizeof(cl_mem), 
                            (void*)&dOutDataBuf); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (dOutDataBuf)");

    status = clSetKernelArg(kernel, 
                            2, 
                            sizeof(cl_mem), 
                            (void*)&dPartialOutDataBuf); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (dPartialOutData)");

    status = clSetKernelArg(kernel, 
                            3, 
                            (localThreads * 2 * sizeof(cl_float)),
                            NULL); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (local memory)");

	status = clSetKernelArg(kernel, 
                            4, 
                            sizeof(cl_mem),
							(void*)&this->classObj); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (global memory)");

    /* 
    * Enqueue a kernel run call.
    */
    cl_event ndrEvt;
    status = clEnqueueNDRangeKernel(
                commandQueue,
                kernel,
                1,
                NULL,
                &globalThreads,
                &localThreads,
                0,
                NULL,
                &ndrEvt);
    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    status = sampleCommon->waitForEventAndRelease(&ndrEvt);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(ndrEvt1) Failed");

    // Enqueue the results to application pointer
    cl_event readEvt1;
    status = clEnqueueReadBuffer(
                commandQueue, 
                dOutDataBuf, 
                CL_FALSE,
                0,
                signalLength * sizeof(cl_float),
                dOutData,
                0,
                NULL,
                &readEvt1);
    CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

    // Enqueue the results to application pointer
    cl_event readEvt2;
    status = clEnqueueReadBuffer(
                commandQueue, 
                dPartialOutDataBuf, 
                CL_FALSE,
                0,
                signalLength * sizeof(cl_float),
                dPartialOutData,
                0,
                NULL,
                &readEvt2);
    CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    status = sampleCommon->waitForEventAndRelease(&readEvt1);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(readEvt1) Failed");

    status = sampleCommon->waitForEventAndRelease(&readEvt2);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(readEvt2) Failed");

	delete paraClass;
	 
	clReleaseMemObject(this->classObj);


    return SDK_SUCCESS;
}

int 
DwtHaar1D::runCLKernels(void)
{

    // Calculate thread-histograms 
    unsigned int levels = 0;
    unsigned int curLevels = 0;
    unsigned int actualLevels = 0;

    int result = getLevels(signalLength, &levels);
    CHECK_ERROR(result, SDK_SUCCESS, "getLevels() failed");

    actualLevels = levels;

    //max levels on device should be decided by kernelWorkGroupSize
    int tempVar = (int)(log((float)kernelInfo.kernelWorkGroupSize) / log((float)2));
    maxLevelsOnDevice = tempVar + 1;

    cl_float* temp = (cl_float*)malloc(signalLength * sizeof(cl_float));
    memcpy(temp, inData, signalLength * sizeof(cl_float));

    levelsDone = 0;
    int one = 1;
    while((unsigned int)levelsDone < actualLevels)
    {
        curLevels = (levels < maxLevelsOnDevice) ? levels : maxLevelsOnDevice;

        // Set the signal length for current iteration
        if(levelsDone == 0)
            curSignalLength = signalLength;
        else
            curSignalLength = (one << levels);

        // Set group size
        groupSize = (1 << curLevels) / 2;

        totalLevels = levels;
        runDwtHaar1DKernel();

        if(levels <= maxLevelsOnDevice)
        {
            dOutData[0] = dPartialOutData[0];
            memcpy(hOutData, dOutData, (one << curLevels) * sizeof(cl_float));
            memcpy(dOutData + (one << curLevels), hOutData + (one << curLevels), (signalLength  - (one << curLevels)) * sizeof(cl_float));
            break;
        }
        else
        {
            levels -= maxLevelsOnDevice;
            memcpy(hOutData, dOutData, curSignalLength * sizeof(cl_float));
            memcpy(inData, dPartialOutData, (one << levels) * sizeof(cl_float));
            levelsDone += (int)maxLevelsOnDevice;
        }

    }

    memcpy(inData, temp, signalLength * sizeof(cl_float));
    free(temp);

	 

    return SDK_SUCCESS;
}

int
DwtHaar1D::initialize()
{
    // Call base class Initialize to get default configuration
    if(this->SDKSample::initialize())
        return SDK_FAILURE;

    streamsdk::Option* length_option = new streamsdk::Option;
    CHECK_ALLOCATION(length_option, "Error. Failed to allocate memory (length_option)\n");

    length_option->_sVersion = "x";
    length_option->_lVersion = "signalLength";
    length_option->_description = "Length of the signal";
    length_option->_type = streamsdk::CA_ARG_INT;
    length_option->_value = &signalLength;

    sampleArgs->AddOption(length_option);
    delete length_option;

    streamsdk::Option* iteration_option = new streamsdk::Option;
    CHECK_ALLOCATION(iteration_option, "Error. Failed to allocate memory (iteration_option)\n");

    iteration_option->_sVersion = "i";
    iteration_option->_lVersion = "iterations";
    iteration_option->_description = "Number of iterations for kernel execution";
    iteration_option->_type = streamsdk::CA_ARG_INT;
    iteration_option->_value = &iterations;

    sampleArgs->AddOption(iteration_option);
    delete iteration_option;

    return SDK_SUCCESS;
}

int DwtHaar1D::setup()
{
	if(iterations < 1)
	{
		std::cout<<"Error, iterations cannot be 0 or negative. Exiting..\n";
		exit(0);
	}
    if(setupDwtHaar1D() != SDK_SUCCESS)
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


int DwtHaar1D::run()
{
    // Warm up
    for(int i = 0; i < 2 && iterations != 1; i++)
    {
        // Arguments are set and execution call is enqueued on command buffer
        if(runCLKernels() != SDK_SUCCESS)
            return SDK_FAILURE;
    }

    std::cout << "Executing kernel for " << 
        iterations << " iterations" << std::endl;
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
    
    // Compute kernel time
    kernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;

    if(!quiet)
    {
        sampleCommon->printArray<cl_float>("dOutData", dOutData, 256, 1);
    }

    return SDK_SUCCESS;
}

int 
DwtHaar1D::verifyResults()
{
    if(verify)
    {
        // Rreference implementation on host device 
        calApproxFinalOnHost();

        // Compare the results and see if they match 
        bool result = true;
        for(cl_uint i = 0; i < signalLength; ++i)
        {
            if(fabs(dOutData[i] - hOutData[i]) > 0.1f)
            {
                result = false;
                break;
            }
        }

        if(result)
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

void DwtHaar1D::printStats()
{
    std::string strArray[3] = {"SignalLength", "Time(sec)", "[Transfer+Kernel]Time(sec)"};
    totalTime = setupTime + kernelTime;

    std::string stats[3];
    stats[0] = sampleCommon->toString(signalLength, std::dec);
    stats[1] = sampleCommon->toString(totalTime, std::dec);
    stats[2] = sampleCommon->toString(kernelTime, std::dec);

    this->SDKSample::printStats(strArray, stats, 3);
}


int DwtHaar1D::cleanup()
{
    // Releases OpenCL resources (Context, Memory etc.)
    cl_int status;

    status = clReleaseMemObject(inDataBuf);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(inDataBuf)");

    status = clReleaseMemObject(dOutDataBuf);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(dOutDataBuf)");

    status = clReleaseMemObject(dPartialOutDataBuf);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(dPartialOutDataBuf)");


    status = clReleaseKernel(kernel);
    CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.(kernel)");

    status = clReleaseProgram(program);
    CHECK_OPENCL_ERROR(status, "clReleaseProgram failed.(program)");

    status = clReleaseCommandQueue(commandQueue);
    CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed.(commandQueue)");

    status = clReleaseContext(context);
    CHECK_OPENCL_ERROR(status, "clReleaseContext failed.(context)");

    // Release program resources (input memory etc.)
    FREE(inData);
    FREE(dOutData);
    FREE(dPartialOutData);
    FREE(hOutData);
    FREE(devices);

    return SDK_SUCCESS;
}

int 
main(int argc, char * argv[])
{
    // Create MonteCalroAsian object 
    DwtHaar1D clDwtHaar1D("DwtHaar1D sample");

    // Initialization 
    if(clDwtHaar1D.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    // Parse command line options 
    if(clDwtHaar1D.parseCommandLine(argc, argv) != SDK_SUCCESS)
        return SDK_FAILURE;

    if(clDwtHaar1D.isDumpBinaryEnabled())
    {
        return clDwtHaar1D.genBinaryImage();
    }
    
    // Setup 
    if(clDwtHaar1D.setup()!=SDK_SUCCESS)
        return SDK_FAILURE;

    // Run 
    if(clDwtHaar1D.run()!=SDK_SUCCESS)
        return SDK_FAILURE;

    // Verify 
    if(clDwtHaar1D.verifyResults()!=SDK_SUCCESS)
        return SDK_FAILURE;

    // Cleanup resources created 
    if(clDwtHaar1D.cleanup()!=SDK_SUCCESS)
        return SDK_FAILURE;

    // Print performance statistics 
    clDwtHaar1D.printStats();

    return SDK_SUCCESS;
}
