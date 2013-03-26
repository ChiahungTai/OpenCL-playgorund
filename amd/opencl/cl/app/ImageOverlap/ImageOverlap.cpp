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


#include "ImageOverlap.hpp"
#include <cmath>


int
ImageOverlap::readImage(std::string mapImageName,std::string verifyImageName)
{

    // load input bitmap image 
    mapBitmap.load(mapImageName.c_str());
	verifyBitmap.load(verifyImageName.c_str());
    // error if image did not load 
    if(!mapBitmap.isLoaded())
    {
        sampleCommon->error("Failed to load input image!");
        return SDK_FAILURE;
    }

    // get width and height of input image
    height = mapBitmap.getHeight();
    width = mapBitmap.getWidth();
	image_desc.image_width=width;
	image_desc.image_height=height;
    // allocate memory for map image data 
    mapImageData = (cl_uchar4*)malloc(width * height * sizeof(cl_uchar4));
    CHECK_ALLOCATION(mapImageData,"Failed to allocate memory! (mapImageData)");

    // allocate memory for fill image data 
    fillImageData = (cl_uchar4*)malloc(width * height * sizeof(cl_uchar4));
    CHECK_ALLOCATION(fillImageData,"Failed to allocate memory! (fillImageData)");

    // initializa the Image data to NULL 
    memset(fillImageData, 0, width * height * pixelSize);

    // get the pointer to pixel data 
    pixelData = mapBitmap.getPixels();
    CHECK_ALLOCATION(pixelData,"Failed to read mapBitmap pixel Data!");

    // Copy pixel data into mapImageData
    memcpy(mapImageData, pixelData, width * height * pixelSize);

    // allocate memory for verification output
    verificationImageData = (cl_uchar4*)malloc(width * height * pixelSize);
    CHECK_ALLOCATION(pixelData,"verificationOutput heap allocation failed!");

	pixelData = verifyBitmap.getPixels();
	CHECK_ALLOCATION(pixelData,"Failed to read verifyBitmap pixel Data!");

	// Copy pixel data into verificationOutput
	memcpy(verificationImageData, pixelData, width * height * pixelSize);
	
    return SDK_SUCCESS;
}


int 
ImageOverlap::genBinaryImage()
{
    streamsdk::bifData binaryData;
    binaryData.kernelName = std::string("ImageOverlap_Kernels.cl");
    binaryData.flagsStr = std::string("");
    if(isComplierFlagsSpecified())
        binaryData.flagsFileName = std::string(flags.c_str());

    binaryData.binaryName = std::string(dumpBinary.c_str());
    int status = sampleCommon->generateBinaryImage(binaryData);
    return status;
    }


int 
ImageOverlap::setupCL()
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
    CHECK_OPENCL_ERROR(status, "clCreateContextFromType failed.");

    // getting device on which to run the sample
    status = sampleCommon->getDevices(context, &devices, deviceId, isDeviceIdEnabled());
    CHECK_ERROR(status, SDK_SUCCESS, "sampleCommon::getDevices() failed");

    status = deviceInfo.setDeviceInfo(devices[deviceId]);
    CHECK_OPENCL_ERROR(status, "deviceInfo.setDeviceInfo failed");

    if(!deviceInfo.imageSupport)
    {
        OPENCL_EXPECTED_ERROR(" Expected Error: Device does not support Images");
    }
	 
	blockSizeX = deviceInfo.maxWorkGroupSize<GROUP_SIZE?deviceInfo.maxWorkGroupSize:GROUP_SIZE;

    // Create command queue
	cl_command_queue_properties prop = 0;
	for(int i=0;i<3;i++)
	{
		commandQueue[i] = clCreateCommandQueue(
			context,
			devices[deviceId],
			prop,
			&status);
		 CHECK_OPENCL_ERROR(status,"clCreateCommandQueuefailed.");
	}

    // Create and initialize image objects

	// Create map image
	mapImage = clCreateImage(context,
		CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
		&imageFormat,
		&image_desc,
		mapImageData,
		&status);
	CHECK_OPENCL_ERROR(status,"clCreateBuffer failed. (mapImage)");
	int color[4] = {0,0,80,255};
	size_t origin[3] = {300,300,0};
	size_t region[3] = {100,100,1};
	status = clEnqueueFillImage(commandQueue[0], mapImage, color, origin, region, NULL, NULL, &eventlist[0]);

    // Create fill image
	fillImage = clCreateImage(context,
		CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
		&imageFormat,
		&image_desc,
		fillImageData,
		&status);
	CHECK_OPENCL_ERROR(status,"clCreateBuffer failed. (fillImage)");

	color[0] = 80;
	color[1] = 0;
	color[2] = 0;
	color[3] = 0;
	origin[0] = 50;
	origin[1] = 50;
	status = clEnqueueFillImage(commandQueue[1], fillImage, color, origin, region, NULL, NULL, &eventlist[1]);
	
	//Create output image
	outputImage = clCreateImage(context,
		CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
		&imageFormat,
		&image_desc,
		NULL,
		&status);
	CHECK_OPENCL_ERROR(status,"clCreateBuffer failed. (outputImage)");

    // create a CL program using the kernel source 
    streamsdk::buildProgramData buildData;
    buildData.kernelName = std::string("ImageOverlap_Kernels.cl");
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
	kernelOverLap = clCreateKernel(program, "OverLap", &status);
	CHECK_OPENCL_ERROR(status,"clCreateKernel failed.(OverLap)");

    return SDK_SUCCESS;
}


int 
ImageOverlap::runCLKernels()
{
    cl_int status;
	
	//wait for fill end
	status=clEnqueueMarkerWithWaitList(commandQueue[2],2,eventlist,&enqueueEvent);
	CHECK_OPENCL_ERROR(status,"clEnqueueMarkerWithWaitList failed.(commandQueue[2])");

    // Set appropriate arguments to the kernelOverLap

    // map buffer image
	status = clSetKernelArg(
		kernelOverLap,
		0,
		sizeof(cl_mem),
		&mapImage);
    CHECK_OPENCL_ERROR(status,"clSetKernelArg failed. (mapImage)");

    // fill Buffer image
    status = clSetKernelArg(
        kernelOverLap,
        1,
        sizeof(cl_mem),
        &fillImage);
    CHECK_OPENCL_ERROR(status,"clSetKernelArg failed. (fillImage)");

	// fill Buffer image
	status = clSetKernelArg(
		kernelOverLap,
		2,
		sizeof(cl_mem),
		&outputImage);
	CHECK_OPENCL_ERROR(status,"clSetKernelArg failed. (outputImage)");

    // Enqueue a kernel run call.
    size_t globalThreads[] = {width, height};
    size_t localThreads[] = {blockSizeX, blockSizeY};

    status = clEnqueueNDRangeKernel(
        commandQueue[2],
        kernelOverLap,
        2,
        NULL,
        globalThreads,
        localThreads,
        1,
        &enqueueEvent,
        NULL);
    CHECK_OPENCL_ERROR(status,"clEnqueueNDRangeKernel failed.");

    // Enqueue Read Image
    size_t origin[] = {0, 0, 0};
    size_t region[] = {width, height, 1};
	size_t  rowPitch; 
	size_t  slicePitch;
    // Read copy
	outputImageData = (cl_uchar4*)clEnqueueMapImage( commandQueue[2],
		outputImage, 
		CL_FALSE, 
		mapFlag,
		origin, region,
		&rowPitch, &slicePitch,
		0, NULL,
		NULL,
		&status );
     CHECK_OPENCL_ERROR(status,"clEnqueueMapImage failed.(commandQueue[2])");

	clFlush(commandQueue[0]);
	clFlush(commandQueue[1]);
    
	status = clEnqueueUnmapMemObject(commandQueue[2],outputImage,(void*)outputImageData,NULL,0,NULL);
	CHECK_OPENCL_ERROR(status,"clEnqueueUnmapMemObject failed.(outputImage)");

	// Wait for the read buffer to finish execution
    status = clFinish(commandQueue[2]);
    CHECK_OPENCL_ERROR(status,"clFinish failed.(commandQueue[2])");


	return SDK_SUCCESS;
}


int 
ImageOverlap::initialize()
{

    // Call base class Initialize to get default configuration
    if (this->SDKSample::initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    streamsdk::Option* iteration_option = new streamsdk::Option;
    CHECK_ALLOCATION(iteration_option,"Memory Allocation error. (iteration_option)");

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
ImageOverlap::setup()
{
    int status = 0;
    // Allocate host memoryF and read input image
    std::string filePath = sampleCommon->getPath() + std::string(MAP_IMAGE);
	std::string  verifyfilePath = sampleCommon->getPath() + std::string(MAP_VERIFY_IMAGE);
    status = readImage(filePath,verifyfilePath);
    CHECK_ERROR(status, SDK_SUCCESS, "Read Map Image failed");

	// create and initialize timers
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    status = setupCL();
    if(status != SDK_SUCCESS)
        return status;
    
    sampleCommon->stopTimer(timer);
    // Compute setup time
    setupTime = (double)(sampleCommon->readTimer(timer));

    return SDK_SUCCESS;
}


int 
ImageOverlap::run()
{
    int status;

    // create and initialize timers
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    std::cout << "Executing kernel for " << iterations << 
        " iterations" <<std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    for(int i = 0; i < iterations; i++)
    {
        // Set kernel arguments and run kernel
        status = runCLKernels();
        CHECK_ERROR(status, SDK_SUCCESS, "OpenCL run Kernel failed");
    }

    sampleCommon->stopTimer(timer);
    // Compute kernel time
    kernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;

    return SDK_SUCCESS;
}


int 
ImageOverlap::cleanup()
{
    // Releases OpenCL resources (Context, Memory etc.)
    cl_int status;
	status = clReleaseEvent(eventlist[0]);
	CHECK_OPENCL_ERROR(status,"clReleaseEvent failed.(eventlist[0])");

	status = clReleaseEvent(eventlist[1]);
	CHECK_OPENCL_ERROR(status,"clReleaseEvent failed.(eventlist[1])");

	status = clReleaseEvent(enqueueEvent);
	CHECK_OPENCL_ERROR(status,"clReleaseEvent failed.(enqueueEvent)");

    status = clReleaseKernel(kernelOverLap);
    CHECK_OPENCL_ERROR(status,"clReleaseKernel failed.(kernelOverLap)");

    status = clReleaseProgram(program);
    CHECK_OPENCL_ERROR(status,"clReleaseProgram failed.(program)");

    status = clReleaseMemObject(mapImage);
    CHECK_OPENCL_ERROR(status,"clReleaseMemObject failed.(mapImage)");

    status = clReleaseMemObject(fillImage);
    CHECK_OPENCL_ERROR(status,"clReleaseMemObject failed.(fillImage)");

	status = clReleaseMemObject(outputImage);
	CHECK_OPENCL_ERROR(status,"clReleaseMemObject failed.(outputImage)");

	for (int i=0;i<3;i++)
	{
		status = clReleaseCommandQueue(commandQueue[i]);
		CHECK_OPENCL_ERROR(status,"clReleaseCommandQueue failed.(commandQueue)");
	}
    
    status = clReleaseContext(context);
    CHECK_OPENCL_ERROR(status,"clReleaseContext failed.(context)");

    // release program resources (input memory etc.)

    FREE(mapImageData);
    FREE(fillImageData);
    FREE(verificationImageData);
    FREE(devices);

    return SDK_SUCCESS;
}


void 
ImageOverlap::ImageOverlapCPUReference()
{

}


int 
ImageOverlap::verifyResults()
{
    if(verify)
    {
        std::cout << "Verifying result - ";
        // compare the results and see if they match
        if(!memcmp(outputImageData, verificationImageData, width * height * 4))
        {
            std::cout << "Passed!\n" << std::endl;
        }
        else
        {
            std::cout << "Failed\n" << std::endl;
            return SDK_FAILURE;
        }
       
    }
    return SDK_SUCCESS;
}


void 
ImageOverlap::printStats()
{
    std::string strArray[4] = 
    {
        "Width", 
        "Height", 
        "Time(sec)", 
        "kernelTime(sec)"
    };
    std::string stats[4];

    totalTime = setupTime + kernelTime;

    stats[0] = sampleCommon->toString(width, std::dec);
    stats[1] = sampleCommon->toString(height, std::dec);
    stats[2] = sampleCommon->toString(totalTime, std::dec);
    stats[3] = sampleCommon->toString(kernelTime, std::dec);

    this->SDKSample::printStats(strArray, stats, 4);
}


int 
main(int argc, char * argv[])
{
    int status = 0;
    ImageOverlap clImageOverlap("OpenCL ImageOverlap");

    if (clImageOverlap.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    if (clImageOverlap.parseCommandLine(argc, argv) != SDK_SUCCESS)
        return SDK_FAILURE;

    if(clImageOverlap.isDumpBinaryEnabled())
    {
        return clImageOverlap.genBinaryImage();
    }

    status = clImageOverlap.setup();
    if(status != SDK_SUCCESS)
        return (status == SDK_EXPECTED_FAILURE) ? SDK_SUCCESS : SDK_FAILURE;

    if (clImageOverlap.run() !=SDK_SUCCESS)
        return SDK_FAILURE;

    if (clImageOverlap.verifyResults() != SDK_SUCCESS)
        return SDK_FAILURE;

    if (clImageOverlap.cleanup() != SDK_SUCCESS)
        return SDK_FAILURE;

    clImageOverlap.printStats();
    return SDK_SUCCESS;
}
