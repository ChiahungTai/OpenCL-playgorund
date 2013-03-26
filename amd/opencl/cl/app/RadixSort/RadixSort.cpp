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


#include "RadixSort.hpp"

#include <math.h>


int 
RadixSort::hostRadixSort()
{
    cl_uint *histogram = (cl_uint*)malloc(RADICES * sizeof(cl_uint));
    CHECK_ALLOCATION(histogram, "Failed to allocate host memory. (histogram)");

    cl_uint *tempData = (cl_uint*)malloc(elementCount * sizeof(cl_uint));
    CHECK_ALLOCATION(tempData, "Failed to allocate host memory. (tempData)");

    if(histogram != NULL && tempData != NULL)
    {
        memcpy(tempData, unsortedData, elementCount * sizeof(cl_uint));
        for(int bits = 0; bits < sizeof(cl_uint) * RADIX ; bits += RADIX)
        {
            // Initialize histogram bucket to zeros
            memset(histogram, 0, RADICES * sizeof(cl_uint));

            // Calculate 256 histogram for all element
            for(int i = 0; i < elementCount; ++i)
            {
                cl_uint element = tempData[i];
                cl_uint value = (element >> bits) & RADIX_MASK;
                histogram[value]++;
            }

            // Prescan the histogram bucket
            cl_uint sum = 0;
            for(int i = 0; i < RADICES; ++i)
            {
                cl_uint val = histogram[i];
                histogram[i] = sum;
                sum += val;
            }

            // Rearrange  the elements based on prescaned histogram
            for(int i = 0; i < elementCount; ++i)
            {
                cl_uint element = tempData[i];
                cl_uint value = (element >> bits) & RADIX_MASK;
                cl_uint index = histogram[value];
                hSortedData[index] = tempData[i];
                histogram[value] = index + 1;
            }

            // Copy to tempData for further use
            if(bits != RADIX * 3)
                memcpy(tempData, hSortedData, elementCount * sizeof(cl_uint));
        }
    }

    FREE(tempData);
    FREE(histogram);
    return SDK_SUCCESS;
}

int
RadixSort::setupRadixSort()
{
	int i = 0;

	groupSize = GROUP_SIZE;
	elementCount = sampleCommon->roundToPowerOf2<cl_uint>(elementCount);

	//element count must be multiple of GROUP_SIZE * RADICES 
	int mulFactor = groupSize * RADICES;

	if(elementCount < mulFactor)
		elementCount = mulFactor;
	else
		elementCount = (elementCount / mulFactor) * mulFactor;
	if(elementCount > 65536)
	{
		
		printf("\nBecause the local memory is limited to 32K,So the input elementCount is limited to 65536!\n");
			
		elementCount = 65536;
	}

	numGroups = elementCount / mulFactor;

	// Allocate and init memory used by host
	unsortedData = (cl_uint*)malloc(elementCount * sizeof(cl_uint));
	CHECK_ALLOCATION(unsortedData, "Failed to allocate host memory. (unsortedData)");

	for(i = 0; i < elementCount; i++)
	{
		unsortedData[i] = (cl_uint)rand();
	}

	dSortedData = (cl_uint*)malloc(elementCount * sizeof(cl_uint));
	CHECK_ALLOCATION(dSortedData, "Failed to allocate host memory. (dSortedData)");

	memset(dSortedData, 0, elementCount * sizeof(cl_uint));

	hSortedData = (cl_uint*)malloc(elementCount * sizeof(cl_uint));
	CHECK_ALLOCATION(hSortedData, "Failed to allocate host memory. (hSortedData)");

	memset(hSortedData, 0, elementCount * sizeof(cl_uint));

	size_t tempSize = numGroups * groupSize * RADICES * sizeof(cl_uint);
	histogramBins = (cl_uint*)malloc(tempSize);
	CHECK_ALLOCATION(histogramBins, "Failed to allocate host memory. (histogramBins)");

	return SDK_SUCCESS;
}

int 
RadixSort::genBinaryImage()
{
    streamsdk::bifData binaryData;
    binaryData.kernelName = std::string("RadixSort_Kernels.cl");
    binaryData.flagsStr = std::string("");
    if(isComplierFlagsSpecified())
        binaryData.flagsFileName = std::string(flags.c_str());

    binaryData.binaryName = std::string(dumpBinary.c_str());
    int status = sampleCommon->generateBinaryImage(binaryData);
    return status;

}


int
RadixSort::setupCL(void)
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
    CHECK_ERROR(retValue, SDK_SUCCESS, "SDKDeviceInfo::setDeviceInfo() failed");

    // Check particular extension
    if(!strstr(deviceInfo.extensions, "cl_khr_byte_addressable_store"))
    {
        byteRWSupport = false;
        OPENCL_EXPECTED_ERROR("Device does not support cl_khr_byte_addressable_store extension!");
    }

	if (RADICES > deviceInfo.maxWorkItemSizes[0]) 
	{
		OPENCL_EXPECTED_ERROR("Device does not support requested number of work items.");
	}

    // Set Presistent memory only for AMD platform
    cl_mem_flags inMemFlags = CL_MEM_READ_ONLY;
    if(isAmdPlatform())
        inMemFlags |= CL_MEM_USE_PERSISTENT_MEM_AMD;

    // Input buffer
   

	unsortedDataBuf = clCreateBuffer(
		context,
		CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR,
		sizeof(cl_uint) * elementCount,
		this->unsortedData,
		&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (unsortedDataBuf)");



    // Output for histogram kernel
    histogramBinsBuf = clCreateBuffer(
        context, 
        CL_MEM_READ_WRITE,
        numGroups * groupSize * RADICES * sizeof(cl_uint),
        NULL, 
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (histogramBinsBuf)");

    // Input for permute kernel
    scanedHistogramBinsBuf = clCreateBuffer(
        context, 
        CL_MEM_READ_WRITE,
        numGroups * groupSize * RADICES * sizeof(cl_uint),
        NULL, 
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (scanedHistogramBinsBuf)");

    // Final output
    sortedDataBuf = clCreateBuffer(
        context, 
        CL_MEM_WRITE_ONLY,
        elementCount * sizeof(cl_uint),
        NULL,
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (sortedDataBuf)");

	//add four buffers
	sumBufferin = clCreateBuffer(
		context,
		CL_MEM_READ_WRITE,
		(elementCount/GROUP_SIZE)*sizeof(cl_uint),
		NULL,
		&status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (sumBufferin)");

	sumBufferout = clCreateBuffer(
		context,
		CL_MEM_READ_WRITE,
		(elementCount/GROUP_SIZE)*sizeof(cl_uint),
		NULL,
		&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (sumBufferout)");

	summaryBUfferin = clCreateBuffer(
		context,
		CL_MEM_READ_WRITE,
		RADICES*sizeof(cl_uint),
		NULL,
		&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (summaryBUfferin)");

	summaryBUfferout = clCreateBuffer(
		context,
		CL_MEM_READ_WRITE,
		RADICES*sizeof(cl_uint),
		NULL,
		&status);
	CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (summaryBUfferout)");


    // create a CL program using the kernel source 
    streamsdk::buildProgramData buildData;
    buildData.kernelName = std::string("RadixSort_Kernels.cl");
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
    histogramKernel = clCreateKernel(program, "histogram", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

    status = kernelInfoHistogram.setKernelWorkGroupInfo(histogramKernel,devices[deviceId]);
    CHECK_ERROR(status, SDK_SUCCESS, "kernelInfoHistogram.setKernelWorkGroupInfo() failed");

    permuteKernel = clCreateKernel(program, "permute", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

    status = kernelInfoPermute.setKernelWorkGroupInfo(permuteKernel,devices[deviceId]);
    CHECK_ERROR(status, SDK_SUCCESS, "kernelInfoPermute.setKernelWorkGroupInfo() failed");

	//add five kernels
	scanArrayKerneldim1 = clCreateKernel(program,"ScanArraysdim1",&status);
	CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

	scanArrayKerneldim2 = clCreateKernel(program,"ScanArraysdim2",&status);
	CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

	prefixSumKernel = clCreateKernel(program,"prefixSum",&status);
	CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");
 
	blockAdditionKernel = clCreateKernel(program,"blockAddition",&status);
	CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

	FixOffsetkernel = clCreateKernel(program,"FixOffset",&status);
	CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");



    // Find minimum of all kernel's work-group sizes
    size_t temp = min(kernelInfoHistogram.kernelWorkGroupSize, kernelInfoPermute.kernelWorkGroupSize);

    // If groupSize exceeds the minimum
    if((size_t)groupSize > temp)
    {
        if(!quiet)
        {
            std::cout << "Out of Resources!" << std::endl;
            std::cout << "Group Size specified : " << groupSize << std::endl;
            std::cout << "Max Group Size supported on the kernel : "
                      << temp << std::endl;
            std::cout << "Falling back to " << temp << std::endl;
        }
        groupSize = (cl_int)temp;
    }

    return SDK_SUCCESS;
}

int 
RadixSort::runHistogramKernel(int bits)
{
    cl_int status;
    cl_int eventStatus = CL_QUEUED;
    cl_event ndrEvt;

    size_t globalThreads = elementCount;
    size_t localThreads = 256;

    if(localThreads > deviceInfo.maxWorkItemSizes[0] || 
       localThreads > deviceInfo.maxWorkGroupSize)
    {
        std::cout << "Unsupported: Device does not"
            "support requested number of work items.";
        return SDK_FAILURE;
    }

  
    // Setup kernel arguments
    status = clSetKernelArg(histogramKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&unsortedDataBuf); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (unsortedDataBuf)");

    status = clSetKernelArg(histogramKernel, 
                            1, 
                            sizeof(cl_mem), 
                            (void *)&histogramBinsBuf); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (histogramBinsBuf)");

    status = clSetKernelArg(histogramKernel, 
                            2, 
                            sizeof(cl_int), 
                            (void *)&bits); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (bits)");

    status = clSetKernelArg(histogramKernel, 
                            3, 
                            (256 * 1 * sizeof(cl_uint)), 
                            NULL); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (local memory)");

    if(kernelInfoHistogram.localMemoryUsed > deviceInfo.localMemSize)
    {
        std::cout << "Unsupported: Insufficient"
            "local memory on device." << std::endl;
        return SDK_FAILURE;
    }

    /* 
    * Enqueue a kernel run call.
    */
    status = clEnqueueNDRangeKernel(
        commandQueue,
        histogramKernel,
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
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(ndrEvt) Failed");

   

    return SDK_SUCCESS;
}


int
RadixSort::runPermuteKernel(int bits)
{
    cl_int status;
    cl_int eventStatus = CL_QUEUED;
    cl_event ndrEvt;

    size_t bufferSize = numGroups * groupSize * RADICES * sizeof(cl_uint);

    size_t globalThreads = elementCount/RADICES;
    size_t localThreads = groupSize;

    if(localThreads > deviceInfo.maxWorkItemSizes[0] || 
       localThreads > deviceInfo.maxWorkGroupSize)
    {
        std::cout<<"Unsupported: Device does not"
            "support requested number of work items.";
        return SDK_FAILURE;
    }

    // Whether sort is to be in increasing order. CL_TRUE implies increasing
    status = clSetKernelArg(permuteKernel, 
                            0, 
                            sizeof(cl_mem), 
                            (void *)&unsortedDataBuf); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (unsortedDataBuf)");

    status = clSetKernelArg(permuteKernel, 
                            1, 
                            sizeof(cl_mem), 
                            (void *)&scanedHistogramBinsBuf); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (scanedHistogramBinsBuf)");

    status = clSetKernelArg(permuteKernel, 
                            2, 
                            sizeof(cl_int), 
                            (void *)&bits); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (bits)");

    status = clSetKernelArg(permuteKernel, 
                            3, 
                            (groupSize * RADICES * sizeof(cl_ushort)),
                            NULL); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (local memory)");

    status = clSetKernelArg(permuteKernel, 
                            4, 
                            sizeof(cl_mem), 
                            (void *)&sortedDataBuf); 
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (sortedDataBuf)");

    if(kernelInfoPermute.localMemoryUsed > deviceInfo.localMemSize)
    {
        std::cout << "Unsupported: Insufficient"
            "local memory on device." << std::endl;
        return SDK_FAILURE;
    }

    /* 
    * Enqueue a kernel run call.
    */
    status = clEnqueueNDRangeKernel(
        commandQueue,
        permuteKernel,
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
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(ndrEvt) Failed");

    // Enqueue the results to application pointe


	cl_event copyEvt;
	status = clEnqueueCopyBuffer(
		commandQueue, 
		sortedDataBuf,
        unsortedDataBuf,
		0,
        0,
		elementCount * sizeof(cl_uint),
		0,
		NULL,
		&copyEvt);
	CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

	status = clFlush(commandQueue);
	CHECK_OPENCL_ERROR(status, "clFlush failed.");

	status = sampleCommon->waitForEventAndRelease(&copyEvt);
	CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(readEvt) Failed");


	


    return SDK_SUCCESS;
}



int RadixSort::runFixOffsetKernel()
{
	
	cl_int status;
	cl_int eventStatus = CL_QUEUED;
	cl_event ndrEvt,ndrEvt1,ndrEvt2,ndrEvt3,ndrEvt4;
    
	size_t numGroups = elementCount/256;
	size_t globalThreadsScan[2] = {numGroups,RADICES};
	size_t localThreadsScan[2] = {GROUP_SIZE,1};

/* There are five kernels to be run ,but when numGroups/GROUP_SIZE is equal to 1,there are only 3 kernels
   to be run*/

	/*The first kernel: we can issue the scanedHistogramBinsBuf buffer is a 2-dimention buffer,and the range
	of the 2nd dimention is 0-255,we need scan this buffer in a workgroup .*/
	cl_uint arg = 0;

	 // Setup kernel arguments
    status = clSetKernelArg(scanArrayKerneldim2,
		                    arg++,
							sizeof(cl_mem),
							(void *)&scanedHistogramBinsBuf);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (scanedHistogramBinsBuf)");


	status = clSetKernelArg(scanArrayKerneldim2,
		arg++,
		sizeof(cl_mem),
		(void *)&histogramBinsBuf);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (histogramBinsBuf)");

	status = clSetKernelArg(scanArrayKerneldim2,
		arg++,
		GROUP_SIZE * sizeof(cl_uint),
		NULL);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (localBUffer)");
	cl_uint groupS = GROUP_SIZE;

	status = clSetKernelArg(scanArrayKerneldim2,
		arg++,
		sizeof(cl_uint),
		(void *)&groupS);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (GROUP_SIZE)");

	status = clSetKernelArg(scanArrayKerneldim2,
		arg++,
		sizeof(cl_uint),
		(void *)&numGroups);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (GROUP_SIZE)");

	
	status = clSetKernelArg(scanArrayKerneldim2,
		arg++,
		sizeof(cl_mem),
		(void *)&sumBufferin);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (sumBufferin)");
	/* 
	* Enqueue a kernel run call.
	*/

	status = clEnqueueNDRangeKernel(
		commandQueue,
		scanArrayKerneldim2,
		2,
		NULL,
		globalThreadsScan,
		localThreadsScan,
		0,
		NULL,
		&ndrEvt);
	CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

	status = clFlush(commandQueue);
	CHECK_OPENCL_ERROR(status, "clFlush failed.");

	status = sampleCommon->waitForEventAndRelease(&ndrEvt);
	CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(ndrEvt) Failed");

	/*If there is only one workgroup in the 1st dimention we needn't run the prefix kernel
	  and the blockadition kernel*/
	if(numGroups/GROUP_SIZE != 1)
	{
    /*run prefix kernel:if there are more than workgroups in the 1st dimention , do an accumulation 
	 of the each group summary.*/
	size_t globalThredsPrefixSum[2] = {numGroups/GROUP_SIZE,RADICES};

	arg = 0;

	 // Setup kernel arguments
	status = clSetKernelArg(prefixSumKernel,
		arg++,
		sizeof(cl_mem),
		(void *)&sumBufferout);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (sumBufferout)");

	status = clSetKernelArg(prefixSumKernel,
		arg++,
		sizeof(cl_mem),
		(void *)&sumBufferin);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (sumBufferout)");

	status = clSetKernelArg(prefixSumKernel,
		arg++,
		sizeof(cl_mem),
		(void *)&summaryBUfferin);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (summaryBUfferin)");

	cl_uint pstride = (cl_uint)numGroups/GROUP_SIZE;

	status = clSetKernelArg(prefixSumKernel,
		arg++,
		sizeof(cl_uint),
		(void *)&pstride);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (summaryBUfferin)");


	/* 
	* Enqueue a kernel run call.
	*/
	status = clEnqueueNDRangeKernel(
		commandQueue,
		prefixSumKernel,
		2,
		NULL,
		globalThredsPrefixSum,
		NULL,
		0,
		NULL,
		&ndrEvt1);
	CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

	status = clFlush(commandQueue);
	CHECK_OPENCL_ERROR(status, "clFlush failed.");

	status = sampleCommon->waitForEventAndRelease(&ndrEvt1);
	CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(ndrEvt) Failed");

	/*run blockAddition kernel: for each element of the current group adds the accumulation of the previous 
	 group coputed by the prefix kernel*/
	size_t globalThreadsAdd[2] = {numGroups,RADICES};
	size_t localThreadsAdd[2] = {GROUP_SIZE,1};

	arg = 0;

	 // Setup kernel arguments
	status = clSetKernelArg(blockAdditionKernel,
		arg++,
		sizeof(cl_mem),
		(void *)&sumBufferout);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (sumBufferout)");

	status = clSetKernelArg(blockAdditionKernel,
		arg++,
		sizeof(cl_mem),
		(void *)&scanedHistogramBinsBuf);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (sumBufferout)");

	status = clSetKernelArg(blockAdditionKernel,
		arg++,
		sizeof(cl_uint),
		(void *)&pstride);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (summaryBUfferin)");

	/* 
	* Enqueue a kernel run call.
	*/
	status = clEnqueueNDRangeKernel(
		commandQueue,
		blockAdditionKernel,
		2,
		NULL,
		globalThreadsAdd,
		localThreadsAdd,
		0,
		NULL,
		&ndrEvt2);
	CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

	status = clFlush(commandQueue);
	CHECK_OPENCL_ERROR(status, "clFlush failed.");

	status = sampleCommon->waitForEventAndRelease(&ndrEvt2);
	CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(ndrEvt) Failed");
	}

	/*run ScanArraysdim1 kernel:now we have 256 values which are the summary of each row.
	  we shoule do a accumulation of this 256 value*/
	size_t globalThreadsScan1[1] = {RADICES};
	size_t localThreadsScan1[1] = {RADICES};

	arg = 0;

	 // Setup kernel arguments
	status = clSetKernelArg(scanArrayKerneldim1,
		arg++,
		sizeof(cl_mem),
		(void *)&summaryBUfferout);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (summaryBUfferout)");

	if(numGroups/GROUP_SIZE != 1)
	{
		status = clSetKernelArg(scanArrayKerneldim1,
			arg++,
			sizeof(cl_mem),
			(void *)&summaryBUfferin);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (summaryBUfferin)");

	}
	else
	{
		status = clSetKernelArg(scanArrayKerneldim1,
			arg++,
			sizeof(cl_mem),
			(void *)&sumBufferin);
		CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (summaryBUfferin)");
	}
	

	status = clSetKernelArg(scanArrayKerneldim1,
		arg++,
		RADICES * sizeof(cl_uint),
		NULL);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (localmemory)");
	groupS = RADICES;

	status = clSetKernelArg(scanArrayKerneldim1,
		arg++,
		sizeof(cl_uint),
		(void *)&groupS);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (block_size)");

	/* 
	* Enqueue a kernel run call.
	*/
	status = clEnqueueNDRangeKernel(
		commandQueue,
		scanArrayKerneldim1,
		1,
		NULL,
		globalThreadsScan1,
		localThreadsScan1,
		0,
		NULL,
		&ndrEvt3);
	CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

	status = clFlush(commandQueue);
	CHECK_OPENCL_ERROR(status, "clFlush failed.");

	status = sampleCommon->waitForEventAndRelease(&ndrEvt3);
	CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(ndrEvt) Failed");


	/*run fixoffset kernel: for each row of the 2nd dimention ,add the summary of the previous 
	row computed by the ScanArraysdim1 kenel ,(the 1st row needn't be changed!), now the correct 
	position has been computed out*/
	size_t globalThreadsFixOffset[2]= {numGroups,RADICES};

	arg = 0;

	 // Setup kernel arguments
	status = clSetKernelArg(FixOffsetkernel,
		arg++,
		sizeof(cl_mem),
		(void *)&summaryBUfferout);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (summaryBUfferout)");

	status = clSetKernelArg(FixOffsetkernel,
		arg++,
		sizeof(cl_mem),
		(void *)&scanedHistogramBinsBuf);
	CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (scanedHistogramBinsBuf)");


	/* 
	* Enqueue a kernel run call.
	*/
	status = clEnqueueNDRangeKernel(
		commandQueue,
		FixOffsetkernel,
		2,
		NULL,
		globalThreadsFixOffset,
		NULL,
		0,
		NULL,
		&ndrEvt4);
	CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

	status = clFlush(commandQueue);
	CHECK_OPENCL_ERROR(status, "clFlush failed.");

	status = sampleCommon->waitForEventAndRelease(&ndrEvt4);
	CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(ndrEvt) Failed");  
   
	return SDK_SUCCESS;

}



int 
RadixSort::runCLKernels(void)
{
    for(int bits = 0; bits < sizeof(cl_uint) * RADIX; bits += RADIX)
    {
        // Calculate thread-histograms
        runHistogramKernel(bits);
        
		// Scan the histogram
		runFixOffsetKernel();

        // Permute the element to appropriate place
        runPermuteKernel(bits);

        // Current output now becomes the next input

    }

    //readout the sortedData to Host buffer
	cl_int status;
	cl_uint *data = (cl_uint *)clEnqueueMapBuffer(commandQueue,sortedDataBuf,true,CL_MAP_READ,0,
		elementCount * sizeof(cl_uint),0,NULL,NULL,&status);
	
	CHECK_OPENCL_ERROR(status, "clMapBuffer failed.");
	memcpy(dSortedData,data,elementCount * sizeof(cl_uint));
	status = clEnqueueUnmapMemObject(commandQueue,sortedDataBuf,data,0,NULL,NULL);


    return SDK_SUCCESS;
}

int 
RadixSort::initialize()
{
    // Call base class Initialize to get default configuration
    if (this->SDKSample::initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    // Now add customized options
    streamsdk::Option* array_length = new streamsdk::Option;
    if(!array_length)
    {
        std::cout << "Memory allocation error.\n";
        return SDK_FAILURE;
    }

    array_length->_sVersion = "x";
    array_length->_lVersion = "count";
    array_length->_description = "Element count";
    array_length->_type = streamsdk::CA_ARG_INT;
    array_length->_value = &elementCount;
    sampleArgs->AddOption(array_length);
    delete array_length;


    streamsdk::Option* iteration_option = new streamsdk::Option;
    if(!iteration_option)
    {
        std::cout << "Memory Allocation error.\n";
        return SDK_FAILURE;
    }
    iteration_option->_sVersion = "i";
    iteration_option->_lVersion = "iterations";
    iteration_option->_description = "Number of iterations to execute kernel";
    iteration_option->_type = streamsdk::CA_ARG_INT;
    iteration_option->_value = &iterations;

    sampleArgs->AddOption(iteration_option);
    delete iteration_option;

    return SDK_SUCCESS;
}

int RadixSort::setup()
{
	if(iterations < 1)
	{
		std::cout<<"Error, iterations cannot be 0 or negative. Exiting..\n";
		exit(0);
	}
    int status = setupRadixSort();
    CHECK_ERROR(status, SDK_SUCCESS, "Sample SetUp Resources Failed");

    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    status = setupCL();
    if(status != SDK_SUCCESS)
    {
        if(status == SDK_EXPECTED_FAILURE) 
            return SDK_EXPECTED_FAILURE;

        return SDK_FAILURE;
    }

    sampleCommon->stopTimer(timer);
    // Compute setup time
    setupTime = (double)(sampleCommon->readTimer(timer));

    return SDK_SUCCESS;
}


int RadixSort::run()
{
    int status = 0;
    if(!byteRWSupport)
        return SDK_SUCCESS;

    for(int i = 0; i < 2 && iterations != 1; i++)
    {
        // Arguments are set and execution call is enqueued on command buffer
        if (runCLKernels() != SDK_SUCCESS)
            return SDK_FAILURE;
    }

    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    std::cout << "Executing kernel for " <<
        iterations<<" iterations" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    for(int i = 0; i < iterations; i++)
    {
        // Arguments are set and execution call is enqueued on command buffer
        if (runCLKernels() != SDK_SUCCESS)
            return SDK_FAILURE;

    }

    sampleCommon->stopTimer(timer);
    // Compute kernel time
    kernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;

    if(!quiet)
        sampleCommon->printArray<cl_uint>("dSortedData", dSortedData, 512, 1);

    return SDK_SUCCESS;
}

int 
RadixSort::verifyResults()
{
    if(!byteRWSupport)
        return SDK_SUCCESS;

    if(verify)
    {
        /* Rreference implementation on host device
        * Sorted by radix sort
        */
        int status = hostRadixSort();
        CHECK_ERROR(status, SDK_SUCCESS, "Host Implementation Failed");

        // compare the results and see if they match
        bool result = true;
        int failedCount = 0;
        for(int i = 0; i < elementCount; ++i)
        {
            if(dSortedData[i] != hSortedData[i])
            {
                result = false;
                failedCount++;
                printf("Element(%d) -  %u = %u \n", 
                       i,
                       dSortedData[i], hSortedData[i]);
            }
        }

        if(result)
        {
            std::cout << "Passed!\n" << std::endl;
            return SDK_SUCCESS;
        }
        else
        {
            std::cout <<" Failed\n" << std::endl;
            return SDK_FAILURE;
        }
    }
    return SDK_SUCCESS;
}

void
RadixSort::printStats()
{
    if(!byteRWSupport)
        return;

    std::string strArray[3] = {"Elements", "Time(sec)", "[Transfer+Kernel]Time(sec)"};
    std::string stats[3];

    totalTime = setupTime + kernelTime;
    stats[0]  = sampleCommon->toString(elementCount, std::dec);
    stats[1]  = sampleCommon->toString(totalTime, std::dec);
    stats[2]  = sampleCommon->toString(kernelTime, std::dec);

    this->SDKSample::printStats(strArray, stats, 3);
}


int
RadixSort::cleanup()
{
    if(!byteRWSupport)
        return SDK_SUCCESS;

    // Releases OpenCL resources (Context, Memory etc.)
    cl_int status;

    status = clReleaseMemObject(unsortedDataBuf);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(unsortedDataBuf)");

    status = clReleaseMemObject(sortedDataBuf);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(sortedDataBuf)");

    status = clReleaseMemObject(histogramBinsBuf);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(histogramBinsBuf)");

    status = clReleaseMemObject(scanedHistogramBinsBuf);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(scanedHistogramBinsBuf)");

	status = clReleaseMemObject(sumBufferin);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(scanedHistogramBinsBuf)");

	status = clReleaseMemObject(sumBufferout);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(scanedHistogramBinsBuf)");

	status = clReleaseMemObject(summaryBUfferin);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(scanedHistogramBinsBuf)");

	status = clReleaseMemObject(summaryBUfferout);
	CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(scanedHistogramBinsBuf)");

    status = clReleaseKernel(histogramKernel);
    CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.(histogramKernel)");

    status = clReleaseKernel(permuteKernel);
    CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.(permuteKernel)");

	status = clReleaseKernel(scanArrayKerneldim1);
	CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.(permuteKernel)");

	status = clReleaseKernel(scanArrayKerneldim2);
	CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.(permuteKernel)");

	status = clReleaseKernel(prefixSumKernel);
	CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.(permuteKernel)");

	status = clReleaseKernel(blockAdditionKernel);
	CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.(permuteKernel)");

	status = clReleaseKernel(FixOffsetkernel);
	CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.(permuteKernel)");


    status = clReleaseProgram(program);
    CHECK_OPENCL_ERROR(status, "clReleaseProgram failed.(program)");

    status = clReleaseCommandQueue(commandQueue);
    CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed.(commandQueue)");

    status = clReleaseContext(context);
    CHECK_OPENCL_ERROR(status, "clReleaseContext failed.");

    // Release program resources (input memory etc.) 
    FREE(unsortedData);
    FREE(dSortedData);
    FREE(hSortedData);
    FREE(histogramBins);
    FREE(devices);


    return SDK_SUCCESS;
}

int 
main(int argc, char * argv[])
{
    cl_int status = 0;
    RadixSort clRadixSort("RadixSort Sample");

    if (clRadixSort.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    if (clRadixSort.parseCommandLine(argc, argv) != SDK_SUCCESS)
        return SDK_FAILURE;

    if(clRadixSort.isDumpBinaryEnabled())
    {
        return clRadixSort.genBinaryImage();
    }

    status = clRadixSort.setup();
    if(status != SDK_SUCCESS)
        return (status == SDK_EXPECTED_FAILURE) ? SDK_SUCCESS : SDK_FAILURE;

    if (clRadixSort.run() != SDK_SUCCESS)
        return SDK_FAILURE;

    if (clRadixSort.verifyResults() != SDK_SUCCESS)
        return SDK_FAILURE;

    if (clRadixSort.cleanup() != SDK_SUCCESS)
        return SDK_FAILURE;

    clRadixSort.printStats();

    return SDK_SUCCESS;
}
