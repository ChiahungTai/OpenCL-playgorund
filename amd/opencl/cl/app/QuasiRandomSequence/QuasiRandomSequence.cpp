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

#include "QuasiRandomSequence.hpp"
#include <malloc.h>


/* Generate direction numbers
   v[j][32] : j dimensions each having 32 direction numbers
*/
void 
QuasiRandomSequence::generateDirectionNumbers(cl_uint nDimensions, 
                                              cl_uint* directionNumbers)
{
    cl_uint *v = directionNumbers;

    for (int dim = 0 ; dim < (int)(nDimensions); dim++)
    {
        // First dimension is a special case
        if (dim == 0)
        {
            for (int i = 0 ; i < N_DIRECTIONS ; i++)
            {
                // All m's are 1
                v[i] = 1 << (31 - i);
            }
        }
        else
        {
            int d = sobolPrimitives[dim].degree;
            /* The first direction numbers (up to the degree of the polynomial)
             are simply   v[i] = m[i] / 2^i  (stored in Q0.32 format) */
            for (int i = 0 ; i < d ; i++)
            {
                v[i] = sobolPrimitives[dim].m[i] << (31 - i);
            }

            for (int i = d ; i < N_DIRECTIONS ; i++) 
            {

                v[i] = v[i - d] ^ (v[i - d] >> d);
                /*                                
                Note that the coefficients a[] are zero or one and for compactness in
                 the input tables they are stored as bits of a single integer. To extract
                 the relevant bit we use right shift and mask with 1.
                 For example, for a 10 degree polynomial there are ten useful bits in a,
                 so to get a[2] we need to right shift 7 times (to get the 8th bit into
                 the LSB) and then mask with 1.*/
                for (int j = 1 ; j < d ; j++)
                {
                    v[i] ^= (((sobolPrimitives[dim].a >> (d - 1 - j)) & 1) * v[i - j]);
                }
            }
        }
        v += N_DIRECTIONS;
    }
}


/*
*  Host Initialization 
*    Allocate and initialize memory on the host.
*    Print input array. 
*/

int 
QuasiRandomSequence::setupQuasiRandomSequence()
{
    // allocate input array 
#ifdef _WIN32
    input = (cl_uint*)_aligned_malloc(nDimensions * N_DIRECTIONS * 
                                      sizeof(cl_uint),
                                      16);
#else
    input = (cl_uint*)memalign(16, 
                               nDimensions * N_DIRECTIONS * 
                               sizeof(cl_uint));
#endif
    CHECK_ALLOCATION(input, "Failed to allocate host memory. (input)");

    // Check for dimensions 
    if(nDimensions > MAX_DIMENSIONS)
    {
        std::cout << "Max allowed dimension is 10200!\n";
        return SDK_FAILURE;
    }

    // initializa sobol direction numbers
    generateDirectionNumbers(nDimensions, input);

    if(!quiet) 
    {
        sampleCommon->printArray<cl_uint>(
            "Input", 
            input, 
            N_DIRECTIONS, 
            nDimensions);

    }

    // allocate output array 
#ifdef _WIN32
    output = (cl_float*)_aligned_malloc(nVectors * nDimensions * 
                                        sizeof(cl_float), 
                                        16);
#else
    output = (cl_float*)memalign(16, 
                                 nVectors * nDimensions * 
                                 sizeof(cl_float));
#endif
    CHECK_ALLOCATION(output, "Failed to allocate host memory(output)");

    // if verification is enabled
    if(verify)
    {
        // allocate memory for verification output array 
        verificationOutput = (cl_float*)malloc(nVectors * nDimensions * 
                                               sizeof(cl_float));
        CHECK_ALLOCATION(verificationOutput,"Failed to allocate host memory. (verify)");

        memset(verificationOutput, 
               0, 
               nVectors * nDimensions * sizeof(cl_float));
    }
    return SDK_SUCCESS;
}

int 
QuasiRandomSequence::genBinaryImage()
{
    streamsdk::bifData binaryData;
    binaryData.kernelName = std::string("QuasiRandomSequence_Kernels.cl");
    binaryData.flagsStr = std::string("");
    if(isComplierFlagsSpecified())
        binaryData.flagsFileName = std::string(flags.c_str());

    binaryData.binaryName = std::string(dumpBinary.c_str());
    int status = sampleCommon->generateBinaryImage(binaryData);
    return status;
}


int
QuasiRandomSequence::setupCL(void)
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
        //The block is to move the declaration of prop closer to its use
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

    // Set Persistent memory only for AMD platform
    cl_mem_flags inMemFlags = CL_MEM_READ_ONLY;
    if(isAmdPlatform())
        inMemFlags |= CL_MEM_USE_PERSISTENT_MEM_AMD;

    inputBuffer = clCreateBuffer(
        context, 
        inMemFlags,
        sizeof(cl_uint) * nDimensions * N_DIRECTIONS,
        0, 
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (inputBuffer)");

    outputBuffer = clCreateBuffer(
        context, 
        CL_MEM_WRITE_ONLY,
        sizeof(cl_float) * nVectors * nDimensions,
        0, 
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateBuffer failed. (outputBuffer)");

    // create a CL program using the kernel source 
    streamsdk::buildProgramData buildData;
    buildData.kernelName = std::string("QuasiRandomSequence_Kernels.cl");
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
    kernel = clCreateKernel(program, "QuasiRandomSequence", &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

    status = kernelInfo.setKernelWorkGroupInfo(kernel, devices[deviceId]);
    CHECK_ERROR(status, SDK_SUCCESS, "KernelInfo.setKernelWorkGroupInfo() failed");

    if( nVectors/4 > kernelInfo.kernelWorkGroupSize)
    {
        if(!quiet)
        {
            std::cout << "Out of Resources!" << std::endl;
            std::cout << "Group Size specified (nVectors/4) : "
                      << nVectors / 4 << std::endl;
            std::cout << "Max Group Size supported on the kernel : " 
                      << kernelInfo.kernelWorkGroupSize<<std::endl;
            std::cout << "Falling back to " 
                      << kernelInfo.kernelWorkGroupSize << std::endl;
        }
        nVectors = (cl_uint)kernelInfo.kernelWorkGroupSize * 4;
    }
    return SDK_SUCCESS;
}



int 
QuasiRandomSequence::runCLKernels(void)
{
    cl_int   status;

    // set total threads and block size 
    size_t globalThreads[1]= {nDimensions * nVectors / 4};
    size_t localThreads[1] = {nVectors / 4};

    if(localThreads[0] > deviceInfo.maxWorkItemSizes[0] || 
       localThreads[0] > deviceInfo.maxWorkGroupSize)
    {
        std::cout << "Unsupported: Device does not support"
                  << "requested number of work items." << std::endl;
        return SDK_FAILURE;
    }

    // Enqueue write to inputBuffer 
    cl_event writeEvt;
    status = clEnqueueWriteBuffer(
                    commandQueue,
                    inputBuffer,
                    CL_FALSE,
                    0,
                    sizeof(cl_uint) * nDimensions * N_DIRECTIONS,
                    input,
                    0, 
                    NULL,
                    &writeEvt);
    CHECK_OPENCL_ERROR(status, "clEnqueueWriteBuffer failed. (inputBuffer)");

    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.(commandQueue)");

    status = sampleCommon->waitForEventAndRelease(&writeEvt);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(writeEvt) Failed");

    // Set appropriate arguments to the kernel 

    // 1st argument to the kernel - outputBuffer 
    status = clSetKernelArg(
        kernel, 
        0, 
        sizeof(cl_mem), 
        (void *)&outputBuffer);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (outputBuffer)");

    // 2nd argument to the kernel - inputBuffer
    status = clSetKernelArg(
        kernel, 
        1, 
        sizeof(cl_mem), 
        (void *)&inputBuffer);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (inputBuffer)");

    // 3rd argument to the kernel - localBuffer(shared memory)
    status = clSetKernelArg(
        kernel, 
        2, 
        32 * sizeof(cl_uint), 
        NULL);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (localBuffer)");

    availableLocalMemory = deviceInfo.localMemSize - kernelInfo.localMemoryUsed;

    neededLocalMemory = 32 * sizeof(cl_uint); 

    if(neededLocalMemory > availableLocalMemory)
    {
        std::cout << "Unsupported: Insufficient"
            "local memory on device." << std::endl;
        return SDK_FAILURE;
    }

    // Enqueue a kernel run call
    cl_event ndrEvt;
    status = clEnqueueNDRangeKernel(
        commandQueue,
        kernel,
        1, // work_dim
        NULL, // global_work_offset
        globalThreads, // global_work_size
        localThreads, // local_work_size
        0, // num_events in wait list
        NULL, // event_wait_list
        &ndrEvt); // event

    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.(commandQueue)");

    status = sampleCommon->waitForEventAndRelease(&ndrEvt);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(ndrEvt) Failed");

    // Enqueue readBuffer
    cl_event readEvt;
    status = clEnqueueReadBuffer(
        commandQueue,
        outputBuffer,
        CL_TRUE,
        0,
        nDimensions * nVectors * sizeof(cl_float),
        output,
        0,
        NULL,
        &readEvt);
    CHECK_OPENCL_ERROR(status, "clEnqueueReadBuffer failed.");

    status = clFlush(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFlush failed.(commandQueue)");

    status = sampleCommon->waitForEventAndRelease(&readEvt);
    CHECK_ERROR(status, SDK_SUCCESS, "WaitForEventAndRelease(readEvt) Failed");

    return SDK_SUCCESS;
}


int 
QuasiRandomSequence::initialize()
{
    // Call base class Initialize to get default configuration
    if (this->SDKSample::initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    streamsdk::Option* array_length = new streamsdk::Option;
    CHECK_ALLOCATION(array_length, "Memory allocation error.\n");

    array_length->_sVersion = "y";
    array_length->_lVersion = "dimension";
    array_length->_description = "Number of dimensions";
    array_length->_type = streamsdk::CA_ARG_INT;
    array_length->_value = &nDimensions;
    sampleArgs->AddOption(array_length);


    array_length->_sVersion = "x";
    array_length->_lVersion = "vector";
    array_length->_description = "Number of vectors";
    array_length->_type = streamsdk::CA_ARG_INT;
    array_length->_value = &nVectors;
    sampleArgs->AddOption(array_length);

    delete array_length;

    streamsdk::Option* iteration_option = new streamsdk::Option;
    CHECK_ALLOCATION(iteration_option, "Memory Allocation error.\n");

    iteration_option->_sVersion = "i";
    iteration_option->_lVersion = "iterations";
    iteration_option->_description = "Number of iterations";
    iteration_option->_type = streamsdk::CA_ARG_INT;
    iteration_option->_value = &iterations;

    sampleArgs->AddOption(iteration_option);
    delete iteration_option;

    return SDK_SUCCESS;
}

int 
QuasiRandomSequence::setup()
{
	if(iterations < 1)
	{
		std::cout<<"Error, iterations cannot be 0 or negative. Exiting..\n";
		exit(0);
	}
    if (setupQuasiRandomSequence() != SDK_SUCCESS)
        return SDK_FAILURE;

    // create and initialize timers 
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

    if (setupCL() != SDK_SUCCESS)
        return SDK_FAILURE;

    sampleCommon->stopTimer(timer);
    // Compute setup time 
    setupTime = (double)(sampleCommon->readTimer(timer));

    return SDK_SUCCESS;
}


int QuasiRandomSequence::run()
{
    // Warm up
    int status = 0;
    for(int i = 0; i < 2 && iterations != 1; i++)
    {
        // Arguments are set and execution call is enqueued on command buffer 
        if (runCLKernels() != SDK_SUCCESS)
            return SDK_FAILURE;
    }

    std::cout << "Executing kernel for " << 
        iterations<<" iterations" << std::endl;
    std::cout<<"-------------------------------------------"<<std::endl;

    // create and initialize timers 
    int timer = sampleCommon->createTimer();
    sampleCommon->resetTimer(timer);
    sampleCommon->startTimer(timer);

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
    {
        sampleCommon->printArray<cl_float>("Output", 
                                           output, 
                                           nVectors, nDimensions);
    }

    return SDK_SUCCESS;
}


void 
QuasiRandomSequence::quasiRandomSequenceCPUReference()
{

    for(int j=0; j < (int)nDimensions; j++)
    {
        for(int i=0; i < (int)nVectors; i++)
        {
            unsigned int temp = 0;
            for(int k=0; k < 32; k++)
            {
                int mask = (int)(pow(2, (double)k));

                temp ^= ((i & mask) >> k) * input[j * 32 + k];
            }

            if(i==0 && j==0)
            {
                verificationOutput[j * nVectors + i] = 0;
            }
            else
            {
                verificationOutput[j * nVectors + i] = (cl_float)(temp / 
                    pow(2, (double)32));
            }
        }
    }
}


int 
QuasiRandomSequence::verifyResults()
{

    if(verify)
    {
        // reference implementation 
        quasiRandomSequenceCPUReference();

        // compare the results and see if they match 
        if(sampleCommon->compare(output, 
                                 verificationOutput, nDimensions * nVectors))
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
QuasiRandomSequence::printStats()
{
    std::string strArray[3] = {"Elements", "Time(sec)", "[Transfer+Kernel]Time(sec)"};
    std::string stats[3];

    int length = nDimensions * nVectors;

    totalTime = setupTime + kernelTime;
    stats[0]  = sampleCommon->toString(length, std::dec);
    stats[1]  = sampleCommon->toString(totalTime, std::dec);
    stats[2]  = sampleCommon->toString(kernelTime, std::dec);

    this->SDKSample::printStats(strArray, stats, 3);

}

int 
QuasiRandomSequence::cleanup()
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

    status = clReleaseCommandQueue(commandQueue);
    CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed.(commandQueue)");

    status = clReleaseContext(context);
    CHECK_OPENCL_ERROR(status, "clReleaseContext failed.(context)");

#ifdef _WIN32
        ALIGNED_FREE(input);

        ALIGNED_FREE(output);
#else
    if(input)
        FREE(input);

    if(output)
        FREE(output);
#endif


    FREE(verificationOutput);
    FREE(devices);

    return SDK_SUCCESS;
}

/* Main routine */
int 
main(int argc, char * argv[])
{
    cl_int status = 0;
    QuasiRandomSequence clQuasiRandomSequence("OpenCL Reduction");

    if (clQuasiRandomSequence.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    if (clQuasiRandomSequence.parseCommandLine(argc, argv) != SDK_SUCCESS)
        return SDK_FAILURE;

    if(clQuasiRandomSequence.isDumpBinaryEnabled())
    {
        return clQuasiRandomSequence.genBinaryImage();
    }

    if (clQuasiRandomSequence.setup() != SDK_SUCCESS)
        return SDK_FAILURE;

    if (clQuasiRandomSequence.run() != SDK_SUCCESS)
        return SDK_FAILURE;

    if (clQuasiRandomSequence.verifyResults() != SDK_SUCCESS)
        return SDK_FAILURE;

    if (clQuasiRandomSequence.cleanup() != SDK_SUCCESS)
        return SDK_FAILURE;

    clQuasiRandomSequence.printStats();
    return SDK_SUCCESS;
}
