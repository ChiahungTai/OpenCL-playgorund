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


#include "FFT.hpp"
#include <malloc.h>


int FFT::setupFFT()
{
    cl_uint inputSizeBytes;

    // allocate and init memory used by host 
    inputSizeBytes = length * sizeof(cl_float);

#if defined (_WIN32)
    input_i = (cl_float*)_aligned_malloc(inputSizeBytes, 16);
    input_r = (cl_float*)_aligned_malloc(inputSizeBytes, 16);
    output_i = (cl_float*)_aligned_malloc(inputSizeBytes, 16);
    output_r = (cl_float*)_aligned_malloc(inputSizeBytes, 16);
#else
    input_i = (cl_float*)memalign(16, inputSizeBytes);
    input_r = (cl_float*)memalign(16, inputSizeBytes);
    output_i = (cl_float*)memalign(16, inputSizeBytes);
    output_r = (cl_float*)memalign(16, inputSizeBytes);
#endif
    CHECK_ALLOCATION(input_i, "Failed to allocate host memory. (input_i)");
    CHECK_ALLOCATION(input_r, "Failed to allocate host memory. (input_r)");
    CHECK_ALLOCATION(output_i, "Failed to allocate host memory. (output_i)");
    CHECK_ALLOCATION(output_r, "Failed to allocate host memory. (output_r)");

    /* 
     * random initialisation of input 
     */
    sampleCommon->fillRandom<cl_float >(input_r, length, 1, 0, 255);
    sampleCommon->fillRandom<cl_float >(input_i, length, 1, 0, 0);

    memcpy(output_i, input_i, inputSizeBytes);
    memcpy(output_r, input_r, inputSizeBytes);

    /* 
     * Unless quiet mode has been enabled, print the INPUT array.
     * No more than 256 values are printed because it clutters the screen
     * and it is not practical to manually compare a large set of numbers
     */
    if(!quiet) 
    {
        sampleCommon->printArray<cl_float >("Original Input Real", 
                                            input_r, 
                                            10, 
                                            1);
        sampleCommon->printArray<cl_float >("Original Input Img", 
                                            input_i, 
                                            10, 
                                            1);
    }

    return SDK_SUCCESS;
}

int 
FFT::genBinaryImage()
{
    streamsdk::bifData binaryData;
    binaryData.kernelName = std::string("FFT_Kernels.cl");
    binaryData.flagsStr = std::string("-x clc++ ");
    if(isComplierFlagsSpecified())
        binaryData.flagsFileName = std::string(flags.c_str());

    binaryData.binaryName = std::string(dumpBinary.c_str());
    int status = sampleCommon->generateBinaryImage(binaryData);
    return status;
}


int
FFT::setupCL(void)
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
    CHECK_ERROR(status, 0, "sampleCommon::getDevices() failed");

    status = deviceInfo.setDeviceInfo(devices[deviceId]);
    CHECK_OPENCL_ERROR(status, "deviceInfo.setDeviceInfo failed");

    {
        // The block is to move the declaration of prop closer to its use 
        cl_command_queue_properties prop = 0;
        commandQueue = clCreateCommandQueue(context, 
                                            devices[deviceId], 
                                            prop, 
                                            &status);
        CHECK_OPENCL_ERROR(status, "clCreateCommandQueue failed.");
    }


    // Create input real buffer 
    buffer_r = clCreateBuffer(context, 
                              CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                              sizeof(cl_float) * length,
                              0, 
                              &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (buffer_r)");

    // Create input imaginary buffer 
    buffer_i = clCreateBuffer(
            context, 
            CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
            sizeof(cl_float)*length,
            0, 
            &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (buffer_i)");

    // create a CL program using the kernel source 
    streamsdk::buildProgramData buildData;
    buildData.kernelName = std::string("FFT_Kernels.cl");
    buildData.devices = devices;
    buildData.deviceId = deviceId;
    buildData.flagsStr = std::string("-x clc++ ");
    if(isLoadBinaryEnabled())
        buildData.binaryName = std::string(loadBinary.c_str());

    if(isComplierFlagsSpecified())
        buildData.flagsFileName = std::string(flags.c_str());

    retValue = sampleCommon->buildOpenCLProgram(program, context, buildData);
    CHECK_ERROR(retValue, SDK_SUCCESS, "sampleCommon::buildOpenCLProgram() failed");

    // get a kernel object handle for a kernel with the given name 
    kernel = clCreateKernel(program, "kfft", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

    return SDK_SUCCESS;
}


int 
FFT::runCLKernels(void)
{
    cl_int   status;

    // Kernel Does an inplace FFT 
    cl_event writeEvt1;
    cl_event writeEvt2;
    status = clEnqueueWriteBuffer(
                commandQueue,
                buffer_r,
                CL_FALSE,
                0,
                sizeof(cl_float)*length,
                input_r,
                0,
                NULL,
                &writeEvt1);
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (buffer_r)");

    // Write input imaginary data to input_r buffer 
    status = clEnqueueWriteBuffer(
                commandQueue,
                buffer_i,
                CL_FALSE,
                0,
                sizeof(cl_float)*length,
                input_i,
                0,
                NULL,
                &writeEvt2);
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (buffer_i)");

    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    status = sampleCommon->waitForEventAndRelease(&writeEvt1);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(writeEvt1) Failed");

    status = sampleCommon->waitForEventAndRelease(&writeEvt2);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(writeEvt2) Failed");


    size_t globalThreads[1];
    size_t localThreads[1];

    globalThreads[0] = 64;
    localThreads[0]  = 64;

    status = kernelInfo.setKernelWorkGroupInfo(kernel, devices[deviceId]);
    CHECK_OPENCL_ERROR(status, "kernelInfo.setKernelWorkGroupInfo failed");

    if(kernelInfo.kernelWorkGroupSize < localThreads[0])
    {
        if(!quiet)
        {
            std::cout << "Error : Out of resources!\n";
            std::cout << "Work-group size required by kernel : 64\n";
            std::cout << "Maximum size supported on GPU for this kernel : "
                      << kernelInfo.kernelWorkGroupSize << std::endl;
        }
        return SDK_EXPECTED_FAILURE;
    }

    // 1st argument
    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&buffer_r);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (buffer_r)");

    // 2nd argument
    status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&buffer_i);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (buffer_i)");

    cl_event ndrEvt;
    status = clEnqueueNDRangeKernel(
                commandQueue,
                kernel,
                1,
                NULL,
                globalThreads,
                localThreads,
                0,
                NULL,
                &ndrEvt);
    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    status = sampleCommon->waitForEventAndRelease(&ndrEvt);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(ndrEvt) Failed");

    // Read real output data 
    cl_event readEvt1;
    cl_event readEvt2;
    status = clEnqueueReadBuffer(
                commandQueue,
                buffer_i,
                CL_FALSE,
                0,
                length * sizeof(cl_float),
                output_i,
                0,
                NULL,
                &readEvt1);
    CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.(buffer_i)");

    // Read imaginary output data 
    status = clEnqueueReadBuffer(
                commandQueue,
                buffer_r,
                CL_FALSE,
                0,
                length * sizeof(cl_float),
                output_r,
                0,
                NULL,
                &readEvt2);
    CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.(buffer_r)");

    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.");

    status = sampleCommon->waitForEventAndRelease(&readEvt1);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(readEvt1) Failed");

    status = sampleCommon->waitForEventAndRelease(&readEvt2);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(readEvt2) Failed");

    return SDK_SUCCESS;
}

/*
   This computes an in-place complex-to-complex FFT 
   x and y are the real and imaginary arrays of 2^m points.
   dir =  1 gives forward transform
   dir = -1 gives reverse transform 
*/
void fftCPU(short int dir,long m,cl_float *x,cl_float *y)
{
   long n, i, i1, j, k, i2, l, l1, l2;
   double c1, c2, tx, ty, t1, t2, u1, u2, z;

   // Calculate the number of points 
   n = 1;
   for (i = 0;i < m;i++) 
      n *= 2;

   // Do the bit reversal 
   i2 = n >> 1;
   j = 0;
   for (i = 0;i < n - 1;i++) 
   {
      if (i < j) 
      {
         tx = x[i];
         ty = y[i];
         x[i] = x[j];
         y[i] = y[j];
         x[j] = (cl_float)tx;
         y[j] = (cl_float)ty;
      }
      k = i2;
      while (k <= j)
      {
         j -= k;
         k >>= 1;
      }
      j += k;
   }

   // Compute the FFT 
   c1 = -1.0; 
   c2 = 0.0;
   l2 = 1;
   for (l = 0;l < m;l++) 
   {
      l1 = l2;
      l2 <<= 1;
      u1 = 1.0; 
      u2 = 0.0;
      for (j = 0;j < l1;j++)
      {
         for (i = j;i < n;i += l2) 
         {
            i1 = i + l1;
            t1 = u1 * x[i1] - u2 * y[i1];
            t2 = u1 * y[i1] + u2 * x[i1];
            x[i1] = (cl_float)(x[i] - t1); 
            y[i1] = (cl_float)(y[i] - t2);
            x[i] += (cl_float)t1;
            y[i] += (cl_float)t2;
         }
         z =  u1 * c1 - u2 * c2;
         u2 = u1 * c2 + u2 * c1;
         u1 = z;
      }
      c2 = sqrt((1.0 - c1) / 2.0);
      if (dir == 1) 
         c2 = -c2;
      c1 = sqrt((1.0 + c1) / 2.0);
   }

   /* Scaling for forward transform */
   /*if (dir == 1) {
      for (i=0;i<n;i++) {
         x[i] /= n;
         y[i] /= n;
      }
   }*/
   
}

/**
 * Reference CPU implementation of FFT Convolution 
 * for performance comparison
 */
void 
FFT::fftCPUReference(cl_float *referenceReal,
                     cl_float *referenceImaginary,
                     cl_float *input_r,
                     cl_float *input_i,
                     cl_uint  w)
{

    // Copy data from input to reference buffers 
    memcpy(referenceReal, input_r, w * sizeof(cl_float));
    memcpy(referenceImaginary, input_i, w * sizeof(cl_float));

    // Compute reference FFT 
    fftCPU(1, 10, referenceReal, referenceImaginary);
}



int FFT::initialize()
{
   // Call base class Initialize to get default configuration
   if(this->SDKSample::initialize())
      return SDK_FAILURE;

    streamsdk::Option* iteration_option = new streamsdk::Option;
    CHECK_ALLOCATION(iteration_option, "Memory Allocation error.\n");

    iteration_option->_sVersion = "i";
    iteration_option->_lVersion = "iterations";
    iteration_option->_description = "Number of iterations to execute kernel";
    iteration_option->_type = streamsdk::CA_ARG_INT;
    iteration_option->_value = &iterations;

    sampleArgs->AddOption(iteration_option);

    delete iteration_option;

   return SDK_SUCCESS;
}

int FFT::setup()
{
   if(setupFFT() != SDK_SUCCESS)
      return SDK_FAILURE;

    // create and initialize timers 
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


int FFT::run()
{
    // Warm up
    for(int i = 0; i < 2 && iterations != 1; i++)
    {
        int rValue = runCLKernels();
        if(rValue != SDK_SUCCESS)
        {
            if(rValue == SDK_EXPECTED_FAILURE)
                return SDK_EXPECTED_FAILURE;
            else
                return SDK_FAILURE;
        }
    }

    std::cout << "Executing kernel for " << iterations 
              << " iterations" << std::endl;
    std::cout << "-------------------------------------------" << std::endl;

    /* Arguments are set and execution call is enqueued on command buffer */
    /* create and initialize timers */
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    for(int i = 0; i < iterations; i++)
    {
        int rValue = runCLKernels();
        if(rValue != SDK_SUCCESS)
        {
            if(rValue == SDK_EXPECTED_FAILURE)
                return SDK_EXPECTED_FAILURE;
            else
                return SDK_FAILURE;
        }
    }

    sampleCommon->stopTimer(timer);
    // Compute kernel time 
    kernelTime = (double)(sampleCommon->readTimer(timer)) / iterations;


    if(!quiet)
    {
        sampleCommon->printArray<cl_float >("Output real", output_r, 10, 1);
        sampleCommon->printArray<cl_float >("Output img", output_i, 10, 1);
    }

    return SDK_SUCCESS;
}

int FFT::verifyResults()
{
    if(verify)
    {
        verificationOutput_i = (cl_float *)malloc(length * sizeof(cl_float));
        verificationOutput_r = (cl_float *)malloc(length * sizeof(cl_float));

        CHECK_ALLOCATION(verificationOutput_i, "Failed to allocate host memory"
                                "(verificationOutput)");

        CHECK_ALLOCATION(verificationOutput_r, "Failed to allocate host memory"
                                "(verificationOutput)");

        // Compute reference FFT on input 
        fftCPUReference(verificationOutput_r, 
                        verificationOutput_i, 
                        input_r, 
                        input_i, 
                        length);

        if(!quiet)
            sampleCommon->printArray<cl_float >("verification Output img", 
                                                verificationOutput_i, 
                                                10, 
                                                1);

        // Compare results 
        if(sampleCommon->compare(verificationOutput_i, output_i, length, 1e-4f))
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

void FFT::printStats()
{
    std::string strArray[3] = {"Length", "Time(sec)", "[Transfer+Kernel]Time(sec)"};
    std::string stats[3];

    totalTime = setupTime + kernelTime;

    stats[0] = sampleCommon->toString(length   , std::dec);
    stats[1] = sampleCommon->toString(totalTime, std::dec);
    stats[2] = sampleCommon->toString(kernelTime, std::dec);

    this->SDKSample::printStats(strArray, stats, 3);
}

int FFT::cleanup()
{
    // Releases OpenCL resources (Context, Memory etc.) 
    cl_int status;

    status = clReleaseMemObject(buffer_i);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(buffer_i)");

    status = clReleaseMemObject(buffer_r);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(buffer_r)");

    status = clReleaseKernel(kernel);
    CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.(kernel)");

    status = clReleaseProgram(program);
    CHECK_OPENCL_ERROR(status, "clReleaseProgram failed.(program)");

    status = clReleaseCommandQueue(commandQueue);
    CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed.(commandQueue)");

    status = clReleaseContext(context);
    CHECK_OPENCL_ERROR(status, "clReleaseContext failed.(context)");

    /* release program resources (input memory etc.) */
#if defined (_WIN32)
        ALIGNED_FREE(input_i);
        ALIGNED_FREE(input_r);
        ALIGNED_FREE(output_i);
        ALIGNED_FREE(output_r);
#else
        FREE(input_i);
        FREE(input_r);
        FREE(output_i);
        FREE(output_r);
#endif


    FREE(verificationOutput_i);
    FREE(verificationOutput_r);
    FREE(devices);

    return SDK_SUCCESS;
}

int 
main(int argc, char * argv[])
{
    FFT clFFT("OpenCL FFT ");

    // Initialize
    if(clFFT.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    if(clFFT.parseCommandLine(argc, argv) != SDK_SUCCESS)
        return SDK_FAILURE;

   if(clFFT.isDumpBinaryEnabled())
    {
        // GenBinaryImage
        return clFFT.genBinaryImage();
    }
   else
    {
        // Setup
        if(clFFT.setup() != SDK_SUCCESS)
            return SDK_FAILURE;

        // Run
        if(clFFT.run() != SDK_SUCCESS)
            return SDK_FAILURE;

            // VerifyResults
            if(clFFT.verifyResults() != SDK_SUCCESS)
                return SDK_FAILURE;

        // Cleanup
        if(clFFT.cleanup() != SDK_SUCCESS)
            return SDK_FAILURE;

        clFFT.printStats();
    }

    return SDK_SUCCESS;
}
