/**********************************************************************
Copyright ©2012 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

/**
*******************************************************************************
* @file <SimpleMultiGPU.cpp>
* 
* @brief Implements Multi GPU based RGB to YUV 4:4:4 conversion. The input data
*        is split into two halves with each half executed on a different GPU
******************************************************************************/

/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include "SimpleMultiGPU.hpp"
#include <iostream>


/******************************************************************************
* Implementation of RgbToYuv::initialize()                                    *
******************************************************************************/
int RgbToYuv::initialize()
{
    if(this->AmpSample::initialize())
        return AMP_FAILURE;

    ampsdk::Option* numInpValOpt = new ampsdk::Option;
    CHECK_ALLOCATION(numInpValOpt, "Memory Allocation error. (numInpValOpt)");

    numInpValOpt->_sVersion = "x";
    numInpValOpt->_lVersion = "samples";
    numInpValOpt->_description = "Number of example input values (multiples of 3)";
    numInpValOpt->_type = ampsdk::CA_ARG_INT;
    numInpValOpt->_value = &inpDataLen;
    sampleArgs->AddOption(numInpValOpt);

    numInpValOpt->_sVersion = "d";
    numInpValOpt->_lVersion = "deviceId";
    numInpValOpt->_description = "";
    sampleArgs->DeleteOption(numInpValOpt);

    delete numInpValOpt;

    return AMP_SUCCESS;
}


/******************************************************************************
* Implementation of RgbToYuv::setup()                                         *
******************************************************************************/
int RgbToYuv::setup(std::vector<DATA_TYPE>& inputRgbData)
{
    /**************************************************************************
    * Ensure that input data size is a multiple of 3. Resize the input vector *
    * to reflect the update to the number of input data elements              *
    **************************************************************************/
    unsigned remainder = (inpDataLen % 3);
    inpDataLen = (remainder) ? inpDataLen+(3-remainder) : inpDataLen;
    inputRgbData.resize(inpDataLen);

    /**************************************************************************
    * Initialize random number generator and generate random values for input.*
    * In a practical usecase the input/output data would be read/written      *
    * from/to image files                                                     *
    **************************************************************************/
    srand(2012);

    DATA_TYPE *pData = inputRgbData.data();
    for (int i = 0; i < inpDataLen; ++i)
    {
        pData[i] = (unsigned char)((int)rand() * 10 
                                           / (int)((RAND_MAX + 1)/(rand()+1)));
    }

    /**************************************************************************
    * Create objects of two device accelerators which are not emulated and    *
    * have dedicated memory.  Since this is a Multi GPU examaple we need to   *
    * error out in case they are not found                                    *
    **************************************************************************/
    printDeviceList();
    std::vector<accelerator> allAccl = accelerator::get_all();
    unsigned j = 0;
    for (unsigned i = 0; i < allAccl.size() && j < 2; ++i) 
    {
        if (!allAccl[i].is_emulated && allAccl[i].dedicated_memory > 0)
            devAccl[j++] = allAccl[i];
    }
    if (j != 2)
    {
        std::cout << "\nNeed 2 accelerators to run this example.  Found ";
        std::cout << j << " device accelerator(s) only." << std::endl;
        return AMP_FAILURE;
    }
    std::wcout << L"\nAccelerators used : " << std::endl 
        << devAccl[0].get_description() << std::endl << L"\t AND"
        << std::endl << devAccl[1].get_description();
    std::cout << std::endl << std::endl;

    if(!quiet)
        std::cout << "Completed setup() of SimpleMultiGPU example" << std::endl;

    return AMP_SUCCESS;
}


/******************************************************************************
* Implementation of RgbToYuv::run()                                           *
******************************************************************************/
int RgbToYuv::run(std::vector<DATA_TYPE>& inputRgbData,
                  std::vector<DATA_TYPE>& outputFromDev0,
                  std::vector<DATA_TYPE>& outputFromDev1)
{
    /**************************************************************************
    * Create a C++ AMP extent to represent the split input data               *
    **************************************************************************/
    unsigned numThreads = inpDataLen/2;
    extent<1> inpExt(numThreads);

    /**************************************************************************
    * Create C++ AMP array_view objects to be used.  The input data to be     *
    * used is split across Dev0 & Dev1 array_views                            *
    **************************************************************************/
    array_view<DATA_TYPE, 1> rgbInputToDev0(inpExt, inputRgbData.data());
    array_view<DATA_TYPE, 1> rgbInputToDev1(inpExt, inputRgbData.data()+numThreads);
    array_view<DATA_TYPE, 1> yuvOutputFromDev0(inpExt, outputFromDev0);
    array_view<DATA_TYPE, 1> yuvOutputFromDev1(inpExt, outputFromDev1);

    /**************************************************************************
    * Create & init timer objects to track the execution time of various runs *
    **************************************************************************/
    int ampMultiGpu = sampleCommon->createTimer();

    /**************************************************************************
    * Warmup code with the sole purpose of avoiding JIT (just-in-time)        *
    * compilation overhead in affecting performance measurements              *
    **************************************************************************/
    rgbToYuvAmp(rgbInputToDev0, yuvOutputFromDev0, devAccl[0].default_view);
	devAccl[0].default_view.flush();

	rgbToYuvAmp(rgbInputToDev1, yuvOutputFromDev1, devAccl[1].default_view);
	devAccl[1].default_view.flush();

    /**************************************************************************
    * Wait for the warmup run to finish                                       *
    **************************************************************************/
	devAccl[0].default_view.wait();
	devAccl[1].default_view.wait();

    if(!quiet)
        std::cout << "Completed Warm up run of C++AMP code" << std::endl;

    sampleCommon->resetTimer(ampMultiGpu);

    /**************************************************************************
    * Execute first half of the input on the first GPU device and the second  *
    * half on the second GPU device.  This is accomplished by specifying the  *
    * accelerator_view to be used as an argument to the lambda.  Array_views  *
    * used in this sample can be replaced with AMP arrays (staging arrays &   *
    * async_copy) to obtain a boost in performance                            *
    **************************************************************************/
    if(!quiet)
        std::cout << "Executing RGB to YUV(4:4:4) on multiple GPU's...\n";

    sampleCommon->startTimer(ampMultiGpu);
    
    rgbToYuvAmp(rgbInputToDev0, yuvOutputFromDev0, devAccl[0].default_view);
    rgbToYuvAmp(rgbInputToDev1, yuvOutputFromDev1, devAccl[1].default_view);

    /**************************************************************************
    * Copy the data back to the CPU.  Using synchronize_async to allow both   *
	* data copies to happen in parallel                                       *
    **************************************************************************/
	completion_future future0 = yuvOutputFromDev0.synchronize_async();
	completion_future future1 = yuvOutputFromDev1.synchronize_async();
	future0.wait();
	future1.wait();

    sampleCommon->stopTimer(ampMultiGpu);
    multiGpuExecTime = (double)(sampleCommon->readTimer(ampMultiGpu));

    if(!quiet)
        std::cout << "Completed run() of SimpleMultiGPU example" << std::endl;

    return AMP_SUCCESS;
}


/******************************************************************************
* Implementation of RgbToYuv::verifyResults()                                 *
******************************************************************************/
int RgbToYuv::verifyResults(std::vector<DATA_TYPE>& inputRgbData,
                            std::vector<DATA_TYPE>& outputFromDev0,
                            std::vector<DATA_TYPE>& outputFromDev1,
                            std::vector<DATA_TYPE>& refVector)
{
    if(verify) 
    {
        std::cout << std::endl << "Verifying..." << std::endl;
        int cpuTimer = sampleCommon->createTimer();
        sampleCommon->resetTimer(cpuTimer);

        /**********************************************************************
        * Execute RGB -> YUV 4:4:4 on CPU and generate the reference output   *
        **********************************************************************/
        if(!quiet)
            std::cout << "Executing CPU(single core) version of RGB to YUV(4:4:4)...\n";

        sampleCommon->startTimer(cpuTimer);
        rgbToYuvSingleCpu(inputRgbData.data(), refVector);
        sampleCommon->stopTimer(cpuTimer);
        cpuExecTime = (double)(sampleCommon->readTimer(cpuTimer));

        std::cout << "First half : ";
        if (compare(outputFromDev0, refVector, NUM_ELEMENTS/2, 0) != AMP_SUCCESS)
            std::cout << "Data MISMATCH\n";
        else
            std::cout << "matches the reference output of CPU\n";

        std::cout << "Second half : ";
        if (compare(outputFromDev1, refVector, NUM_ELEMENTS/2, NUM_ELEMENTS/2)
            != AMP_SUCCESS)
        {
            std::cout << "Data MISMATCH\n\n";
            return AMP_FAILURE;
        }
        std::cout << "matches the reference output of CPU\n\n";
    }

    if(!quiet)
        std::cout << "Completed verifyResults() of SimpleMultiGPU example\n";

    return AMP_SUCCESS;
}


/******************************************************************************
* Implementation of RgbToYuv::printStats()                                    *
******************************************************************************/
void RgbToYuv::printStats()
{
    if (timing)
    {
        std::stringstream cpuTimeStr;
        if (cpuExecTime != 0)
            cpuTimeStr << std::setprecision(3) << cpuExecTime << "s\n";
        else
            cpuTimeStr << "Not run on CPU\n";

        std::cout << "\nRgbToYuv of given " << inpDataLen << " values\n" 
                  << std::setprecision(3) << std::fixed;
        std::cout << "Multi GPU Exec Time : " << multiGpuExecTime << "s\n";
        std::cout << "CPU Exec Time       : " << cpuTimeStr.str() << "\n";
    }

    if(!quiet)
        std::cout << "Completed printStats() of SimpleMultiGPU example" << std::endl;
}


/******************************************************************************
* Implementation of RgbToYuv::rgbToYuvSingleCpu()                             *
******************************************************************************/
int RgbToYuv::rgbToYuvSingleCpu(DATA_TYPE *pRGBData, 
                                std::vector<DATA_TYPE>& vYUV)
{
    for (int i = 0; i < inpDataLen; i+=3)
    {
        float R = (float)pRGBData[i];
        float G = (float)pRGBData[i+1];
        float B = (float)pRGBData[i+2];

        float Y = (0.257f * R) + (0.504f * G) + (0.098f * B) + 16.f;
        float V = (0.439f * R) - (0.368f * G) - (0.071f * B) + 128.f;
        float U = -(0.148f * R) - (0.291f * G) + (0.439f * B) + 128.f;

        vYUV[i] = (DATA_TYPE)(Y > 255 ? 255 : Y);
        vYUV[i+1] = (DATA_TYPE)(U > 255 ? 255 : U);
        vYUV[i+2] = (DATA_TYPE)(V > 255 ? 255 : V);
    }
    
    return AMP_SUCCESS;
}


/******************************************************************************
* Implementation of RgbToYuv::rgbToYuvAmp()                                   *
******************************************************************************/
int RgbToYuv::rgbToYuvAmp(array_view<DATA_TYPE, 1>& avRGB, 
                 array_view<DATA_TYPE, 1>& avYUV, accelerator_view devAcclView)
{
    /**************************************************************************
    * RGB to YUV 4:4:4 implementation in C++ AMP                              *
    **************************************************************************/
    parallel_for_each(devAcclView, avYUV.extent/3, [=](index<1> idx) restrict(amp)
        {
            DATA_TYPE i = idx[0] * 3;
            DATA_TYPE j = i;
            float R = (float)avRGB[j];
            j = j + 1;
            float G = (float)avRGB[j];
            j = j + 1;
            float B = (float)avRGB[j];

            float Y = (0.257f * R) + (0.504f * G) + (0.098f * B) + 16.f;
            float V = (0.439f * R) - (0.368f * G) - (0.071f * B) + 128.f;
            float U = -(0.148f * R) - (0.291f * G) + (0.439f * B) + 128.f;

            avYUV[i] = (DATA_TYPE)(Y > 255 ? 255 : Y);
            avYUV[i+1] = (DATA_TYPE)(U > 255 ? 255 : U);
            avYUV[i+2] = (DATA_TYPE)(V > 255 ? 255 : V);
        });
    
    return AMP_SUCCESS;
}


/******************************************************************************
* Implementation of RgbToYuv::compare()                                       *
******************************************************************************/
int RgbToYuv::compare(std::vector<unsigned>& vTest,
                   std::vector<unsigned>& vRef, int len, unsigned startOffset)
{
    int passed = AMP_SUCCESS;
    unsigned totalMismatches = 0;

    if (vRef.size() < (len+startOffset))
    {
        std::cout << "Ref vector size (" << vRef.size() 
          << ") should be > start offset + len (" << len+startOffset << ")\n";
        return AMP_FAILURE;
    }

    for (unsigned i = startOffset, j = 0; i < len+startOffset; ++i, ++j)
    {
        if (vTest[j] != vRef[i])
        {
            if (!totalMismatches)
            {
                std::cout << " mismatch found " << std::endl << "vTest[" 
                    << j << "] = " << vTest[j] << ", vRef[" << i << "] = " 
                    << vRef[i] << std::endl;
                passed = AMP_FAILURE;
            }
            totalMismatches++;
        }
    }
    
    if (totalMismatches)
        std::cout << "Total number of mismatches found = " 
                  << totalMismatches << std::endl;

    return passed;
}


/******************************************************************************
* Execution of program begins from here                                       *
******************************************************************************/
int main(int argc, char *argv[])
{
    std::cout << "**********************************************" << std::endl;
    std::cout << "C++ AMP Simple multi GPU" << std::endl;
    std::cout << "**********************************************" << std::endl;

    /**************************************************************************
    * Create an object of RgbToYuv class                                      *
    **************************************************************************/
    RgbToYuv objRgbToYuv("C++Amp Simple multi GPU example");

    /**************************************************************************
    * Initialize the additional cmd line options of the example               *
    **************************************************************************/
    if(objRgbToYuv.initialize() != AMP_SUCCESS)
        return AMP_FAILURE;
    std::cout << std::endl << std::endl;

    /**************************************************************************
    * Parse command line options                                              *
    **************************************************************************/
    if(objRgbToYuv.parseCommandLine(argc, argv) != AMP_SUCCESS)
        return AMP_FAILURE;

    /**************************************************************************
    * Create input data vector                                                *
    **************************************************************************/
    int numElements = objRgbToYuv.getInputDataLenth();
    if (numElements <= 0)
    {
        std::cout << "Input data length should be greater than Zero" << std::endl;
        return AMP_FAILURE;
    }

    std::vector<DATA_TYPE> inputRgbData(numElements);

    /**************************************************************************
    * Initialize the random array of input samples                            *
    **************************************************************************/
    if(objRgbToYuv.setup(inputRgbData) != AMP_SUCCESS)
        return AMP_FAILURE;

    /**************************************************************************
    * Create vectors to be used as output with the updated size of input len  *
    **************************************************************************/
    numElements = objRgbToYuv.getInputDataLenth();
    std::vector<DATA_TYPE> outputFromDev0(numElements);
    std::vector<DATA_TYPE> outputFromDev1(numElements);
    std::vector<DATA_TYPE> refVector(numElements);

    /**************************************************************************
    * Execute RgbToYuv 4:4:4 color conversion on the the input RGB data on    *
    * multiple GPU's                                                          *
    **************************************************************************/
    if(objRgbToYuv.run(inputRgbData, outputFromDev0,
                       outputFromDev1) != AMP_SUCCESS)
        return AMP_FAILURE;

    /**************************************************************************
    * Verify the results that were generated                                  *
    **************************************************************************/
    if(objRgbToYuv.verifyResults(inputRgbData, outputFromDev0,
                                 outputFromDev1, refVector) != AMP_SUCCESS)
        return AMP_FAILURE;

    /**************************************************************************
    * Print performance statistics                                            *
    **************************************************************************/
    objRgbToYuv.printStats();

    return 0;
}
