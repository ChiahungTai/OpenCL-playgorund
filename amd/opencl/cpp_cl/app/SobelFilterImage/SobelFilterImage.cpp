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


#include "SobelFilterImage.hpp"
#include <cmath>


int
SobelFilterImage::readInputImage(std::string inputImageName)
{
    // load input bitmap image 
    inputBitmap.load(inputImageName.c_str());

    // error if image did not load 
    if(!inputBitmap.isLoaded())
    {
        std::cout << "Failed to load input image!" << std::endl;
        return SDK_FAILURE;
    }

    // get width and height of input image 
    height = inputBitmap.getHeight();
    width = inputBitmap.getWidth();

    // allocate memory for input & output image data  */
    inputImageData  = (cl_uchar4*)malloc(width * height * sizeof(cl_uchar4));

    // error check 
    CHECK_ALLOCATION(inputImageData, "Failed to allocate memory! (inputImageData)");


    // allocate memory for output image data 
    outputImageData = (cl_uchar4*)malloc(width * height * sizeof(cl_uchar4));

    // error check 
    CHECK_ALLOCATION(outputImageData, "Failed to allocate memory! (outputImageData)");


    // initializa the Image data to NULL 
    memset(outputImageData, 0, width * height * pixelSize);

    // get the pointer to pixel data 
    pixelData = inputBitmap.getPixels();

    // error check 
    CHECK_ALLOCATION(pixelData, "Failed to read pixel Data!");

    // Copy pixel data into inputImageData 
    memcpy(inputImageData, pixelData, width * height * pixelSize);

    // allocate memory for verification output 
    verificationOutput = (cl_uchar*)malloc(width * height * pixelSize);

    // error check 
    CHECK_ALLOCATION(verificationOutput, "verificationOutput heap allocation failed!");

    // initialize the data to NULL 
    memset(verificationOutput, 0, width * height * pixelSize);

    return SDK_SUCCESS;

}


int
SobelFilterImage::writeOutputImage(std::string outputImageName)
{
    // copy output image data back to original pixel data 
    memcpy(pixelData, outputImageData, width * height * pixelSize);

    // write the output bmp file 
    if(!inputBitmap.write(outputImageName.c_str()))
    {
        std::cout << "Failed to write output image!" << std::endl;
        return SDK_FAILURE;
    }

    return SDK_SUCCESS;
}

int 
SobelFilterImage::genBinaryImage()
{
    streamsdk::bifData binaryData;
    binaryData.kernelName = std::string("SobelFilterImage_Kernels.cl");
    binaryData.flagsStr = std::string("");
    if(isComplierFlagsSpecified())
        binaryData.flagsFileName = std::string(flags.c_str());

    binaryData.binaryName = std::string(dumpBinary.c_str());
    int status = sampleCommon->generateBinaryImage(binaryData);
    return status;
    }


int 
SobelFilterImage::setupCL()
{
    cl_int err = CL_SUCCESS;
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
    err = cl::Platform::get(&platforms);
    CHECK_OPENCL_ERROR(err, "Platform::get() failed.");

    std::vector<cl::Platform>::iterator i;
    if(platforms.size() > 0)
    {
        if(isPlatformEnabled())
        {
            i = platforms.begin() + platformId;
        }
        else
        {
            for(i = platforms.begin(); i != platforms.end(); ++i)
            {
                if(!strcmp((*i).getInfo<CL_PLATFORM_VENDOR>().c_str(), 
                    "Advanced Micro Devices, Inc."))
                {
                    break;
                }
            }
        }
    }

    cl_context_properties cps[3] = 
    {
        CL_CONTEXT_PLATFORM, 
        (cl_context_properties)(*i)(),
        0
    };

    context = cl::Context(dType, cps, NULL, NULL, &err);
    CHECK_OPENCL_ERROR(err, "Context::Context() failed.");

    devices = context.getInfo<CL_CONTEXT_DEVICES>(&err);
    CHECK_OPENCL_ERROR(err, "Context::getInfo() failed.");

    std::cout << "Platform :" << (*i).getInfo<CL_PLATFORM_VENDOR>().c_str() << "\n";
    int deviceCount = (int)devices.size();
    int j = 0;
    for (std::vector<cl::Device>::iterator i = devices.begin(); i != devices.end(); ++i, ++j)
    {
        std::cout << "Device " << j << " : ";
        std::string deviceName = (*i).getInfo<CL_DEVICE_NAME>();
        std::cout << deviceName.c_str() << "\n";
    }
    std::cout << "\n";

    if (deviceCount == 0) 
    {
        std::cout << "No device available\n";
        return SDK_FAILURE;
    }

    if(sampleCommon->validateDeviceId(deviceId, deviceCount))
    {
        std::cout << "sampleCommon::validateDeviceId() failed" << std::endl;
        return SDK_FAILURE;
    }


    // Check for image support 
    imageSupport = devices[deviceId].getInfo<CL_DEVICE_IMAGE_SUPPORT>(&err);
    CHECK_OPENCL_ERROR(err, "Device::getInfo() failed.");

    // If images are not supported then return 
    if(!imageSupport)
    {
        OPENCL_EXPECTED_ERROR("Images are not supported on this device!");
    }

    commandQueue = cl::CommandQueue(context, devices[deviceId], 0, &err);
    CHECK_OPENCL_ERROR(err, "CommandQueue::CommandQueue() failed.");
    /*
    * Create and initialize memory objects
    */
    inputImage2D = cl::Image2D(context,
                               CL_MEM_READ_ONLY,
                               cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8),
                               width,
                               height,
                               0,
                               NULL,
                               &err);
    CHECK_OPENCL_ERROR(err, "Image2D::Image2D() failed. (inputImage2D)");
    
 
    // Create memory objects for output Image 
    outputImage2D = cl::Image2D(context,
                               CL_MEM_WRITE_ONLY,
                               cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8),
                               width,
                               height,
                               0,
                               0,
                               &err);
    CHECK_OPENCL_ERROR(err, "Image2D::Image2D() failed. (outputImage2D)");

    device.push_back(devices[deviceId]);

    // create a CL program using the kernel source 
    streamsdk::SDKFile kernelFile;
    std::string kernelPath = sampleCommon->getPath();

    if(isLoadBinaryEnabled())
    {
        kernelPath.append(loadBinary.c_str());
        if(!kernelFile.readBinaryFromFile(kernelPath.c_str()))
        {
            std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
            return SDK_FAILURE;
        }
        cl::Program::Binaries programBinary(1,std::make_pair(
                                              (const void*)kernelFile.source().data(), 
                                              kernelFile.source().size()));

        program = cl::Program(context, device, programBinary, NULL, &err);
        CHECK_OPENCL_ERROR(err, "Program::Program(Binary) failed.");

    }
    else
    {
        kernelPath.append("SobelFilterImage_Kernels.cl");
        if(!kernelFile.open(kernelPath.c_str()))
        {
            std::cout << "Failed to load kernel file : " << kernelPath << std::endl;
            return SDK_FAILURE;
        }

        // create program source 
        cl::Program::Sources programSource(1, 
            std::make_pair(kernelFile.source().data(), 
            kernelFile.source().size()));

        // Create program object 
        program = cl::Program(context, programSource, &err);
        CHECK_OPENCL_ERROR(err, "Program::Program() failed.");

    }

    std::string flagsStr = std::string("");

    // Get additional options
    if(isComplierFlagsSpecified())
    {
        streamsdk::SDKFile flagsFile;
        std::string flagsPath = sampleCommon->getPath();
        flagsPath.append(flags.c_str());
        if(!flagsFile.open(flagsPath.c_str()))
        {
            std::cout << "Failed to load flags file: " << flagsPath << std::endl;
            return SDK_FAILURE;
        }
        flagsFile.replaceNewlineWithSpaces();
        const char * flags = flagsFile.source().c_str();
        flagsStr.append(flags);
    }

    if(flagsStr.size() != 0)
        std::cout << "Build Options are : " << flagsStr.c_str() << std::endl;

    err = program.build(device, flagsStr.c_str());

    if(err != CL_SUCCESS)
    {
        if(err == CL_BUILD_PROGRAM_FAILURE)
        {
            std::string str = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[deviceId]);

            std::cout << " \n\t\t\tBUILD LOG\n";
            std::cout << " ************************************************\n";
            std::cout << str << std::endl;
            std::cout << " ************************************************\n";
        }
    }
    CHECK_OPENCL_ERROR(err, "Program::build() failed.");

    // Create kernel 
    kernel = cl::Kernel(program, "sobel_filter",  &err);
    CHECK_OPENCL_ERROR(err, "Kernel::Kernel() failed.");

    // Check group size against group size returned by kernel 
    kernelWorkGroupSize = kernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(devices[deviceId], &err);
    CHECK_OPENCL_ERROR(err, "Kernel::getWorkGroupInfo()  failed.");

    if((blockSizeX * blockSizeY) > kernelWorkGroupSize)
    {
        if(!quiet)
        {
            std::cout << "Out of Resources!" << std::endl;
            std::cout << "Group Size specified : "
                      << blockSizeX * blockSizeY << std::endl;
            std::cout << "Max Group Size supported on the kernel : "
                      << kernelWorkGroupSize << std::endl;
            std::cout << "Falling back to " << kernelWorkGroupSize << std::endl;
        }

        if(blockSizeX > kernelWorkGroupSize)
        {
            blockSizeX = kernelWorkGroupSize;
            blockSizeY = 1;
        }
    }

    return SDK_SUCCESS;
}

int 
SobelFilterImage::runCLKernels()
{
    cl_int status;

    cl::size_t<3> origin;
    origin[0] = 0; 
    origin[1] = 0; 
    origin[2] = 0;

    cl::size_t<3> region;
    region[0] = width;
    region[1] = height;
    region[2] = 1;

    cl::Event writeEvt;
    status = commandQueue.enqueueWriteImage(
                inputImage2D,
                CL_TRUE,
                origin,
                region,
                0,
                0,
                inputImageData,
                NULL,
                &writeEvt);
    CHECK_OPENCL_ERROR(status, "CommandQueue::enqueueWriteImage failed. (inputImage2D)");

    status = commandQueue.flush();
    CHECK_OPENCL_ERROR(status, "cl::CommandQueue.flush failed.");

    cl_int eventStatus = CL_QUEUED;
    while(eventStatus != CL_COMPLETE)
    {
        status = writeEvt.getInfo<cl_int>(
                    CL_EVENT_COMMAND_EXECUTION_STATUS, 
                    &eventStatus);
        CHECK_OPENCL_ERROR(status, "cl:Event.getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS) failed.");

    }

    // Set appropriate arguments to the kernel 
    // input buffer image 
    status = kernel.setArg(0, inputImage2D);
    CHECK_OPENCL_ERROR(status, "Kernel::setArg() failed. (inputImageBuffer)");

    // outBuffer imager 
    status = kernel.setArg(1, outputImage2D);
    CHECK_OPENCL_ERROR(status, "Kernel::setArg() failed. (outputImageBuffer)");

    /* 
    * Enqueue a kernel run call.
    */
    cl::NDRange globalThreads(width, height);
    cl::NDRange localThreads(blockSizeX, blockSizeY);

    cl::Event ndrEvt;
    status = commandQueue.enqueueNDRangeKernel(
                kernel,
                cl::NullRange,
                globalThreads,
                localThreads,
                0, 
                &ndrEvt);
    CHECK_OPENCL_ERROR(status, "CommandQueue::enqueueNDRangeKernel() failed.");

    status = commandQueue.flush();
    CHECK_OPENCL_ERROR(status, "cl::CommandQueue.flush failed.");

    eventStatus = CL_QUEUED;
    while(eventStatus != CL_COMPLETE)
    {
        status = ndrEvt.getInfo<cl_int>(
                    CL_EVENT_COMMAND_EXECUTION_STATUS, 
                    &eventStatus);
        CHECK_OPENCL_ERROR(status, "cl:Event.getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS) failed.");
         }

    // Enqueue Read Image 
    origin[0] = 0; 
    origin[1] = 0; 
    origin[2] = 0;

    region[0] = width;
    region[1] = height;
    region[2] = 1;

    // Enqueue readBuffer
    cl::Event readEvt;
    status = commandQueue.enqueueReadImage(
                outputImage2D,
                CL_FALSE,
                origin,
                region,
                0,
                0,
                outputImageData,
                NULL,
                &readEvt);
    CHECK_OPENCL_ERROR(status, "CommandQueue::enqueueReadImage failed.");

    status = commandQueue.flush();
    CHECK_OPENCL_ERROR(status, "cl::CommandQueue.flush failed.");

    eventStatus = CL_QUEUED;
    while(eventStatus != CL_COMPLETE)
    {
        status = readEvt.getInfo<cl_int>(
                    CL_EVENT_COMMAND_EXECUTION_STATUS, 
                    &eventStatus);
        CHECK_OPENCL_ERROR(status, "cl:Event.getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS) failed.");

    }

    return SDK_SUCCESS;
}



int 
SobelFilterImage::initialize()
{
    // Call base class Initialize to get default configuration
    CHECK_ERROR(this->SDKSample::initialize(), SDK_SUCCESS, "OpenCL resource initilization failed");

    streamsdk::Option* iteration_option = new streamsdk::Option;
    if(!iteration_option)
    {
        sampleCommon->error("Memory Allocation error.\n");
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

int 
SobelFilterImage::setup()
{
    // Allocate host memory and read input image 
    std::string filePath = sampleCommon->getPath() + std::string(INPUT_IMAGE);
    if(readInputImage(filePath) != SDK_SUCCESS)
        return SDK_FAILURE;

    // create and initialize timers 
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    int status = setupCL();
    if (status != SDK_SUCCESS)
    {
        return status;
    }

    sampleCommon->stopTimer(timer);
    // Compute setup time 
    setupTime = (double)(sampleCommon->readTimer(timer));

    return SDK_SUCCESS;

}

int 
SobelFilterImage::run()
{
    if(!byteRWSupport)
        return SDK_SUCCESS;

    // Warm up
    for(int i = 0; i < 2 && iterations != 1; i++)
    {
        // Set kernel arguments and run kernel 
        if(runCLKernels() != SDK_SUCCESS)
            return SDK_FAILURE;

    }

    std::cout << "Executing kernel for " << iterations 
              << " iterations" <<std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    // create and initialize timers 
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    for(int i = 0; i < iterations; i++)
    {
        // Set kernel arguments and run kernel 
        if(runCLKernels() != SDK_SUCCESS)
            return SDK_FAILURE;

    }

    sampleCommon->stopTimer(timer);
    // Compute kernel time 
    kernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;

    // write the output image to bitmap file 
    if(writeOutputImage(OUTPUT_IMAGE) != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    return SDK_SUCCESS;
}

int 
SobelFilterImage::cleanup()
{

    // release program resources (input memory etc.) 
        FREE(inputImageData);
        FREE(outputImageData);
        FREE(verificationOutput);

    return SDK_SUCCESS;
}


void 
SobelFilterImage::sobelFilterImageCPUReference()
{
    // x-axis gradient mask 
    const int kx[][3] = 
    {
        { 1, 2, 1},
        { 0, 0, 0},
        { -1, -2, -1}
    };

    // y-axis gradient mask 
    const int ky[][3] = 
    {
        { 1, 0, -1},
        { 2, 0, -2},
        { 1, 0, -1}
    };

    int gx = 0;
    int gy = 0;

    // pointer to input image data 
    cl_uchar *ptr = (cl_uchar*)malloc(width * height * pixelSize);
    memcpy(ptr, inputImageData, width * height * pixelSize);

    // each pixel has 4 uchar components 
    int w = width * 4;

    int k = 1;

    // apply filter on each pixel (except boundary pixels) 
    for(int i = 0; i < (int)(w * (height - 1)) ; i++) 
    {
        if(i < (k + 1) * w - 4 && i >= 4 + k * w)
        {
            gx =  kx[0][0] * *(ptr + i - 4 - w)  
                + kx[0][1] * *(ptr + i - w) 
                + kx[0][2] * *(ptr + i + 4 - w)
                + kx[1][0] * *(ptr + i - 4)     
                + kx[1][1] * *(ptr + i)      
                + kx[1][2] * *(ptr + i + 4)
                + kx[2][0] * *(ptr + i - 4 + w) 
                + kx[2][1] * *(ptr + i + w) 
                + kx[2][2] * *(ptr + i + 4 + w);

            gy =  ky[0][0] * *(ptr + i - 4 - w) 
                + ky[0][1] * *(ptr + i - w) 
                + ky[0][2] * *(ptr + i + 4 - w)
                + ky[1][0] * *(ptr + i - 4)     
                + ky[1][1] * *(ptr + i)      
                + ky[1][2] * *(ptr + i + 4)
                + ky[2][0] * *(ptr + i - 4 + w) 
                + ky[2][1] * *(ptr + i + w) 
                + ky[2][2] * *(ptr + i + 4 + w);

            float gx2 = pow((float)gx, 2);
            float gy2 = pow((float)gy, 2);

            double temp = sqrt(gx2 + gy2) / 2.0;

            // Saturate 
            if(temp > 255)
                temp = 255;
            if(temp < 0)
                temp = 0;

            *(verificationOutput + i) = (cl_uchar)(temp);
        }

        // if reached at the end of its row then incr k 
        if(i == (k + 1) * w - 5)
        {
            k++;
        }
    }
    FREE(ptr);
}


int 
SobelFilterImage::verifyResults()
{
    if(!byteRWSupport)
        return SDK_SUCCESS;

    if(verify)
    {
        // reference implementation 
        sobelFilterImageCPUReference();

        size_t size = width * height * sizeof(cl_uchar4);

        cl_uchar4 *verificationData = (cl_uchar4*)malloc(size);
        memcpy(verificationData, verificationOutput, size);

        cl_uint error = 0;
        for(cl_uint y = 0; y < height; y++)
        {
            for(cl_uint x = 0; x < width; x++)
            {
                int c = x + y * width;
                // Only verify pixels inside the boundary 
                if((x >= 1 && x < (width - 1) && y >= 1 && y < (height - 1)))
                {
                    error += outputImageData[c].s[0]-verificationData[c].s[0];
                }
            }
        }

        // compare the results and see if they match 
        if(!error)
        {
            std::cout << "Passed!\n" << std::endl;
            FREE(verificationData);
            return SDK_SUCCESS;
        }
        else
        {
            std::cout << "Failed\n" << std::endl;
            FREE(verificationData);
            return SDK_FAILURE;
        }
    }

    return SDK_SUCCESS;
}

void 
SobelFilterImage::printStats()
{
    std::string strArray[4] = 
    {
        "Width", 
        "Height", 
        "Time(sec)", 
        "[Transfer+Kernel]Time(sec)"
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
    SobelFilterImage clSobelFilterImage("OpenCL SobelFilterImage");

    if(clSobelFilterImage.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    if(clSobelFilterImage.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    if(clSobelFilterImage.isDumpBinaryEnabled())
    {
        return clSobelFilterImage.genBinaryImage();
    }
    else
    {
        // Setup
        int status = clSobelFilterImage.setup();
        if(status != SDK_SUCCESS)
            return (status == SDK_EXPECTED_FAILURE) ? SDK_SUCCESS : SDK_FAILURE;

        // Run
        if(clSobelFilterImage.run() != SDK_SUCCESS)
            return SDK_FAILURE;

        // VerifyResults
        if(clSobelFilterImage.verifyResults() != SDK_SUCCESS)
            return SDK_FAILURE;

        // Cleanup
        if(clSobelFilterImage.cleanup() != SDK_SUCCESS)
            return SDK_FAILURE;

        clSobelFilterImage.printStats();
    }

    return SDK_SUCCESS;
}



