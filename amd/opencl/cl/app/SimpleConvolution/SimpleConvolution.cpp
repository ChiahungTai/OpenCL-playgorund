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


#include "SimpleConvolution.hpp"

int SimpleConvolution::setupSimpleConvolution()
{
    cl_uint inputSizeBytes;

    if(width * height < 256)
    {
        width = 64;
        height = 64;
    }

    // allocate and init memory used by host
    inputSizeBytes = width * height * sizeof(cl_uint);
    input  = (cl_uint *) malloc(inputSizeBytes);
    
    CHECK_ALLOCATION(input, "Failed to allocate host memory. (input)");

    output = (cl_uint  *) malloc(inputSizeBytes);
    CHECK_ALLOCATION(output, "Failed to allocate host memory. (output)");

    cl_uint maskSizeBytes = maskWidth * maskHeight * sizeof(cl_float);
    mask = (cl_float  *) malloc(maskSizeBytes);
    CHECK_ALLOCATION(mask, "Failed to allocate host memory (mask)");

    // random initialisation of input 
    sampleCommon->fillRandom<cl_uint >(input, width, height, 0, 255);

    // Fill a blurr filter or some other filter of your choice
    for(cl_uint i = 0; i < maskWidth*maskHeight; i++)
        mask[i] = 0;

    cl_float val = 1.0f / (maskWidth * 2.0f - 1.0f);

    for(cl_uint i = 0; i < maskWidth; i++) 
    {
        cl_uint y = maskHeight / 2;
        mask[y * maskWidth + i] = val;
    }

    for(cl_uint i = 0; i < maskHeight; i++)
    {
        cl_uint x = maskWidth / 2;
        mask[i * maskWidth + x] = val;
    }

    // Unless quiet mode has been enabled, print the INPUT array.
    if(!quiet) 
    {
        sampleCommon->printArray<cl_uint >(
                "Original Input", 
                input, 
                width, 
                1);
        sampleCommon->printArray<cl_float >(
                "mask", 
                mask, 
                maskWidth, 
                maskHeight);
    }

    return SDK_SUCCESS;
}


int 
SimpleConvolution::genBinaryImage()
{
    streamsdk::bifData binaryData;
    binaryData.kernelName = std::string("SimpleConvolution_Kernels.cl");
    binaryData.flagsStr = std::string("");
    if(isComplierFlagsSpecified())
        binaryData.flagsFileName = std::string(flags.c_str());

    binaryData.binaryName = std::string(dumpBinary.c_str());
    int status = sampleCommon->generateBinaryImage(binaryData);
    return status;
}


int
SimpleConvolution::setupCL(void)
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

    inputBuffer = clCreateBuffer(
            context, 
            CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
            sizeof(cl_uint ) * width * height,
            input, 
            &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (inputBuffer)");

    outputBuffer = clCreateBuffer(
            context, 
            CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
            sizeof(cl_uint ) * width * height,
            output, 
            &status);
    CHECK_OPENCL_ERROR( status,  "clCreateBuffer failed. (outputBuffer)");

    maskBuffer = clCreateBuffer(
            context, 
            CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
            sizeof(cl_float ) * maskWidth * maskHeight,
            mask, 
            &status);
    CHECK_OPENCL_ERROR( status, "clCreateBuffer failed. (maskBuffer)");

    // create a CL program using the kernel source 
    streamsdk::buildProgramData buildData;
    buildData.kernelName = std::string("SimpleConvolution_Kernels.cl");
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
    kernel = clCreateKernel(program, "simpleConvolution", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

    return SDK_SUCCESS;
}

int SimpleConvolution::setWorkGroupSize()
{
    cl_int status = 0;
    status = kernelInfo.setKernelWorkGroupInfo(kernel,devices[deviceId]);
    CHECK_ERROR(status, SDK_SUCCESS, " setKernelWorkGroupInfo() failed");

     globalThreads[0] = width*height;
     localThreads[0]  = 256;

    if((cl_uint)(localThreads[0]) > kernelInfo.kernelWorkGroupSize)
    {
        if(!quiet)
        {
            std::cout << "Out of Resources!" << std::endl;
            std::cout << "Group Size specified : "<< localThreads[0] 
                << std::endl;
            std::cout << "Max Group Size supported on the kernel : " 
                << kernelInfo.kernelWorkGroupSize <<std::endl;
            std::cout <<"Changing the group size to " << kernelInfo.kernelWorkGroupSize 
                << std::endl;
        }

        localThreads[0] = kernelInfo.kernelWorkGroupSize;
    }
    return SDK_SUCCESS;
}

int 
SimpleConvolution::runCLKernels(void)
{
    cl_int   status;
    cl_event events[2];

    status = this->setWorkGroupSize();
    CHECK_ERROR(status, SDK_SUCCESS, "setWorkGroupSize() failed");

    // Set appropriate arguments to the kernel
    status = clSetKernelArg(
                    kernel, 
                    0, 
                    sizeof(cl_mem), 
                    (void *)&outputBuffer);
    CHECK_OPENCL_ERROR( status, "clSetKernelArg failed. (outputBuffer)");

    status = clSetKernelArg(
                    kernel, 
                    1, 
                    sizeof(cl_mem), 
                    (void *)&inputBuffer);
    CHECK_OPENCL_ERROR( status, "clSetKernelArg failed. (inputBuffer)");

    status = clSetKernelArg(
                    kernel, 
                    2, 
                    sizeof(cl_mem), 
                    (void *)&maskBuffer);
    CHECK_OPENCL_ERROR( status, "clSetKernelArg failed. (maskBuffer)");

    cl_uint2 inputDimensions = {width, height};
    cl_uint2 maskDimensions  = {maskWidth, maskHeight};

    status = clSetKernelArg(
                    kernel, 
                    3, 
                    sizeof(cl_uint2), 
                    (void *)&inputDimensions);
    CHECK_OPENCL_ERROR( status, "clSetKernelArg failed. (inputDimensions)");

    status = clSetKernelArg(
                    kernel, 
                    4, 
                    sizeof(cl_uint2), 
                    (void *)&maskDimensions);
    CHECK_OPENCL_ERROR( status, "clSetKernelArg failed. (maskDimensions)");

    // Enqueue a kernel run call.
    status = clEnqueueNDRangeKernel(
            commandQueue,
            kernel,
            1,
            NULL,
            globalThreads,
            localThreads,
            0,
            NULL,
            &events[0]);
    CHECK_OPENCL_ERROR( status, "clEnqueueNDRangeKernel failed.");

    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status,"clFlush() failed"); 

    status = sampleCommon->waitForEventAndRelease(&events[0]);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(events[0]) Failed");
    
    // Enqueue readBuffer
    status = clEnqueueReadBuffer(
                commandQueue,
                outputBuffer,
                CL_TRUE,
                0,
                width * height * sizeof(cl_uint),
                output,
                0,
                NULL,
                &events[1]);
    CHECK_OPENCL_ERROR( status, "clEnqueueReadBuffer failed.");

    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status,"clFlush() failed"); 

    status = sampleCommon->waitForEventAndRelease(&events[1]);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(events[1]) Failed");
    return SDK_SUCCESS;
}

/**
 * Reference CPU implementation of Simple Convolution 
 * for performance comparison
 */
void 
SimpleConvolution::simpleConvolutionCPUReference(cl_uint  *output,
                                                 const cl_uint  *input,
                                                 const cl_float *mask,
                                                 const cl_uint  width,
                                                 const cl_uint height,
                                                 const cl_uint maskWidth,
                                                 const cl_uint maskHeight)
{
    cl_uint vstep = (maskWidth  - 1) / 2;
    cl_uint hstep = (maskHeight - 1) / 2;

    // for each pixel in the input
    for(cl_uint x = 0; x < width; x++)
        for(cl_uint y = 0; y < height; y++)
        {
            /*
             * find the left, right, top and bottom indices such that
             * the indices do not go beyond image boundaires
             */
            cl_uint left    = (x           <  vstep) ? 0         : (x - vstep);
            cl_uint right   = ((x + vstep) >= width) ? width - 1 : (x + vstep); 
            cl_uint top     = (y           <  hstep) ? 0         : (y - hstep);
            cl_uint bottom  = ((y + hstep) >= height)? height - 1: (y + hstep); 

            /*
             * initializing wighted sum value
             */
            cl_float sumFX = 0;

            for(cl_uint i = left; i <= right; ++i)
                for(cl_uint j = top ; j <= bottom; ++j)
                {
                    /*
                     * performing wighted sum within the mask boundaries
                     */
                    cl_uint maskIndex = (j - (y - hstep)) * maskWidth  + (i - (x - vstep));
                    cl_uint index     = j                 * width      + i;
                    
                    /*
                     * to round to the nearest integer
                     */
                    sumFX += ((float)input[index] * mask[maskIndex]);
                }
            sumFX += 0.5f;
            output[y*width + x] = cl_uint(sumFX);
        }
}

int SimpleConvolution::initialize()
{
   // Call base class Initialize to get default configuration
   if  (this->SDKSample::initialize() != SDK_SUCCESS)
       return SDK_FAILURE;

   // Now add customized options
    streamsdk::Option* width_option = new streamsdk::Option;
    CHECK_ALLOCATION(width_option, "Memory allocation error.\n");

    width_option->_sVersion = "x";
    width_option->_lVersion = "width";
    width_option->_description = "Width of the input matrix";
    width_option->_type = streamsdk::CA_ARG_INT;
    width_option->_value = &width;

    sampleArgs->AddOption(width_option);
    delete width_option;

    streamsdk::Option* height_option = new streamsdk::Option;
    CHECK_ALLOCATION(height_option, "Memory allocation error.\n");

    height_option->_sVersion = "y";
    height_option->_lVersion = "height";
    height_option->_description = "Height of the input matrix";
    height_option->_type = streamsdk::CA_ARG_INT;
    height_option->_value = &height;

    sampleArgs->AddOption(height_option);
    delete height_option;

    streamsdk::Option* mask_width = new streamsdk::Option;
    CHECK_ALLOCATION(mask_width, "Memory allocation error.\n");

    maskWidth = 3;
    mask_width->_sVersion = "m";
    mask_width->_lVersion = "masksize";
    mask_width->_description = "Width of the mask matrix";
    mask_width->_type = streamsdk::CA_ARG_INT;
    mask_width->_value = &maskWidth;
    sampleArgs->AddOption(mask_width);
    delete mask_width;

    streamsdk::Option* num_iterations = new streamsdk::Option;
    CHECK_ALLOCATION(num_iterations, "Memory allocation error.\n");

    num_iterations->_sVersion = "i";
    num_iterations->_lVersion = "iterations";
    num_iterations->_description = "Number of iterations for kernel execution";
    num_iterations->_type = streamsdk::CA_ARG_INT;
    num_iterations->_value = &iterations;

    sampleArgs->AddOption(num_iterations);
    delete num_iterations;

   return SDK_SUCCESS;
}

int SimpleConvolution::setup()
{
   if(!sampleCommon->isPowerOf2(width))
      width = sampleCommon->roundToPowerOf2(width);
   if(!sampleCommon->isPowerOf2(height))
      height = sampleCommon->roundToPowerOf2(height);

   maskHeight = maskWidth;

   if(!(maskWidth%2))
      maskWidth++;
   if(!(maskHeight%2))
      maskHeight++;

    if (setupSimpleConvolution() != SDK_SUCCESS)
        return SDK_FAILURE;

    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    if (setupCL() != SDK_SUCCESS)
        return SDK_FAILURE;

    sampleCommon->stopTimer(timer);

    setupTime = (cl_double)sampleCommon->readTimer(timer);

   return SDK_SUCCESS;
}


int SimpleConvolution::run()
{
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    std::cout << "Executing kernel for " << iterations << 
        " iterations" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    for(int i = 0; i < iterations; i++)
    {
        // Arguments are set and execution call is enqueued on command buffer
        if (runCLKernels() != SDK_SUCCESS)
            return SDK_FAILURE;
    }

    sampleCommon->stopTimer(timer);
    totalKernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;

    if(!quiet) {
        sampleCommon->printArray<cl_uint >("Output", output, width, 1);
    }

    return SDK_SUCCESS;
}

int SimpleConvolution::verifyResults()
{
    if(verify)
    {
        verificationOutput = (cl_uint *) malloc(width * height * sizeof(cl_uint ));
        CHECK_ALLOCATION(verificationOutput, "Failed to allocate host memory. (verificationOutput)");

        /* 
         * reference implementation
         */
        cl_uint2 inputDimensions = {width    , height};
        cl_uint2 maskDimensions  = {maskWidth, maskHeight};

        simpleConvolutionCPUReference(verificationOutput, input, mask, width, height,
                                        maskWidth, maskHeight);

        // compare the results and see if they match
        if(memcmp(output, verificationOutput, height*width*sizeof(cl_uint )) == 0)
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

void SimpleConvolution::printStats()
{
    std::string strArray[5] = {"Width", "Height", "mask Size", "Time(sec)", "KernelTime(sec)"};
    std::string stats[5];

    totalTime = setupTime + totalKernelTime;

    stats[0]  = sampleCommon->toString(width    , std::dec);
    stats[1]  = sampleCommon->toString(height   , std::dec);
    stats[2]  = sampleCommon->toString(maskWidth, std::dec);
    stats[3]  = sampleCommon->toString(totalTime, std::dec);
    stats[4]  = sampleCommon->toString(totalKernelTime, std::dec);

    this->SDKSample::printStats(strArray, stats, 5);
}

int SimpleConvolution::cleanup()
{
    // Releases OpenCL resources (Context, Memory etc.) 
    cl_int status;

    status = clReleaseKernel(kernel);
    CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.(kernel)");

    status = clReleaseProgram(program);
    CHECK_OPENCL_ERROR(status, "clReleaseProgram failed.(program)");

   status = clReleaseMemObject(inputBuffer);
   CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(inputBuffer)");

   status = clReleaseMemObject(outputBuffer);
   CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(outputBuffer)");

   status = clReleaseMemObject(maskBuffer);
   CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(maskBuffer)");

    status = clReleaseCommandQueue(commandQueue);
    CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed.(commandQueue)");

    status = clReleaseContext(context);
    CHECK_OPENCL_ERROR(status, "clReleaseContext failed.(context)");

    // release program resources (input memory etc.)
    FREE(input);
    FREE(output);
    FREE(mask);
    FREE(verificationOutput);
    FREE(devices);

   return SDK_SUCCESS;
}

int 
main(int argc, char * argv[])
{
    SimpleConvolution clSimpleConvolution("OpenCL Simple Convolution");

    if (clSimpleConvolution.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    if (clSimpleConvolution.parseCommandLine(argc, argv) != SDK_SUCCESS)
        return SDK_FAILURE;

    if(clSimpleConvolution.isDumpBinaryEnabled())
    {
        return clSimpleConvolution.genBinaryImage();
    }

    if (clSimpleConvolution.setup() != SDK_SUCCESS)
        return SDK_FAILURE;

    if (clSimpleConvolution.run() != SDK_SUCCESS)
        return SDK_FAILURE;

    if (clSimpleConvolution.verifyResults() != SDK_SUCCESS)
        return SDK_FAILURE;

    if (clSimpleConvolution.cleanup() != SDK_SUCCESS)
        return SDK_FAILURE;

    clSimpleConvolution.printStats();
    return SDK_SUCCESS;
}
