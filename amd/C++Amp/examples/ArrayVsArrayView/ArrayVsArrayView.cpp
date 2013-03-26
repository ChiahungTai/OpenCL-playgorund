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
* @file <ArrayVsArrayView.cpp>
* 
* @brief Implements GPU based RGB to YUV 4:4:4 conversion depicting differences   
*        in using AMP Array & AMP ArrayView to do the same
******************************************************************************/

/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include "ArrayVsArrayView.hpp"
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
    * Create an object of the first AMD accelerator (if & when available). If *
    * not available then ensure that the default accl is not a slow emulator  *
    **************************************************************************/
    printDeviceList();
    if(setDefaultAccelerator() != AMP_SUCCESS)
        return AMP_FAILURE;
    std::cout << std::endl << std::endl;

    if(!quiet)
        std::cout << "Completed setup() of ArrayVsArrayView example" << std::endl;

    return AMP_SUCCESS;
}


/******************************************************************************
* Implementation of RgbToYuv::run()                                           *
******************************************************************************/
int RgbToYuv::run(std::vector<DATA_TYPE>& inputRgbData,
                  std::vector<DATA_TYPE>& outputFromArrayView,
                  std::vector<DATA_TYPE>& outputFromArray)
{
    /**************************************************************************
    * Create a C++ AMP extent of the required size and dimension              *
    **************************************************************************/
    extent<1> inpExt(inpDataLen);

    /**************************************************************************
    * Create & init timer objects to track the execution time of various runs *
    **************************************************************************/
    int arrTimer = sampleCommon->createTimer();
    int arrVwTimer = sampleCommon->createTimer();

    /**************************************************************************
    * Warmup code with the sole purpose of avoiding JIT (just-in-time)        *
    * compilation overhead in affecting performance measurements              *
    **************************************************************************/
    rgbToYuvAmpArrayView(inpExt, inputRgbData, outputFromArrayView, arrVwTimer);
    rgbToYuvAmpArray(inpExt, inputRgbData, outputFromArray, arrTimer);
    if(!quiet)
        std::cout << "Completed Warm up run of C++AMP code" << std::endl;

    sampleCommon->resetTimer(arrVwTimer);
    sampleCommon->resetTimer(arrTimer);

    /**************************************************************************
    * AMP ArrayView version of RGB to YUV(4:4:4)                              *
    **************************************************************************/
    if(!quiet)
        std::cout << "Executing AMP ArrayView version of RGB to YUV(4:4:4)...\n";
    rgbToYuvAmpArrayView(inpExt, inputRgbData, outputFromArrayView, arrVwTimer);
    arrViewExecTime = (double)(sampleCommon->readTimer(arrVwTimer));

    /**************************************************************************
    * AMP Array version of RGB to YUV(4:4:4)                                  *
    **************************************************************************/
    if(!quiet)
        std::cout << "Executing AMP Array version of RGB to YUV(4:4:4)...\n";
    rgbToYuvAmpArray(inpExt, inputRgbData, outputFromArray, arrTimer);
    arrExecTime = (double)(sampleCommon->readTimer(arrTimer));

    if(!quiet)
        std::cout << "Completed run() of ArrayVsArrayView example" << std::endl;

    return AMP_SUCCESS;
}


/******************************************************************************
* Implementation of RgbToYuv::verifyResults()                                 *
******************************************************************************/
int RgbToYuv::verifyResults(std::vector<DATA_TYPE>& inputRgbData,
                            std::vector<DATA_TYPE>& outputFromArrayView,
                            std::vector<DATA_TYPE>& outputFromArray,
                            std::vector<DATA_TYPE>& refVector)
{
    if(verify) 
    {
        std::cout << std::endl << "Verifying..." << std::endl;
        int cpuTimer = sampleCommon->createTimer();
        sampleCommon->resetTimer(cpuTimer);

        if(!quiet)
            std::cout << "Executing CPU(single core) version of RGB to YUV(4:4:4)...\n";

        sampleCommon->startTimer(cpuTimer);
        rgbToYuvSingleCpu(inputRgbData.data(), refVector);
        sampleCommon->stopTimer(cpuTimer);
        cpuExecTime = (double)(sampleCommon->readTimer(cpuTimer));

        std::cout << "Output of AMP ArrayView ";
        if (compare(outputFromArrayView, refVector) != AMP_SUCCESS)
            std::cout << "Data MISMATCH\n";
        else
            std::cout << "matches the reference output of CPU\n";

        std::cout << "Output of AMP Array ";
        if (compare(outputFromArray, refVector) != AMP_SUCCESS)
        {
            std::cout << "Data MISMATCH\n\n";
            return AMP_FAILURE;
        }
        std::cout << "matches the reference output of CPU\n\n";
    }

    if(!quiet)
        std::cout << "Completed verifyResults() of ArrayVsArrayView example\n";

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
        std::cout << "Array Exec Time     : " << arrExecTime << "s\n";
        std::cout << "ArrayView Exec Time : " << arrViewExecTime << "s\n";
        std::cout << "CPU Exec Time       : " << cpuTimeStr.str() << "\n";
    }

    if(!quiet)
        std::cout << "Completed printStats() of ArrayVsArrayView example" << std::endl;
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
* Implementation of RgbToYuv::rgbToYuvAmpArrayView()                          *
******************************************************************************/
int RgbToYuv::rgbToYuvAmpArrayView(extent<1> inpExt,
                                   std::vector<DATA_TYPE>& inputRgbData,
                                   std::vector<DATA_TYPE>& outputFromArrayView,
                                   int arrViewTimer)
{
    sampleCommon->startTimer(arrViewTimer);

    /**************************************************************************
    * Create C++ AMP array views to be used.                                  *
    * Array views are wrappers to containers of data and hence require the    *
    * data during creation. The actual container can reside either on the CPU *
    * (vector) or in GPU (array, array view). Data of an array view is copied *
    * on demand to the accelerator                                            *
    **************************************************************************/
    const array_view<DATA_TYPE, 1> rgbDeviceArrayView(inpExt, inputRgbData); 
    array_view<DATA_TYPE, 1> yuvArrayView(inpExt, outputFromArrayView); 

    /**************************************************************************
    * Usage of 'const' in the array_view declaration is an optimization to    *
    * prevent the data being copied back to CPU and discard_data() is used to *
    * avoid copying initial data of yuvArrayView (output array view) to GPU   *
    **************************************************************************/
    yuvArrayView.discard_data();

    /**************************************************************************
    * Note that ArrayViews can ONLY be captured "by value" in any overload    *
    * of parallel_for_each().                                                 *
    **************************************************************************/
    parallel_for_each(rgbDeviceArrayView.extent/3, [=](index<1> idx) restrict(amp)
        {
            DATA_TYPE i = idx[0] * 3;
            DATA_TYPE j = i;
            float R = (float)rgbDeviceArrayView[j];
            j = j + 1;
            float G = (float)rgbDeviceArrayView[j];
            j = j + 1;
            float B = (float)rgbDeviceArrayView[j];

            float Y = (0.257f * R) + (0.504f * G) + (0.098f * B) + 16.f;
            float V = (0.439f * R) - (0.368f * G) - (0.071f * B) + 128.f;
            float U = -(0.148f * R) - (0.291f * G) + (0.439f * B) + 128.f;

	        yuvArrayView[i] = (DATA_TYPE)(Y > 255 ? 255 : Y);
	        yuvArrayView[i+1] = (DATA_TYPE)(U > 255 ? 255 : U);
	        yuvArrayView[i+2] = (DATA_TYPE)(V > 255 ? 255 : V);
        });

    /**************************************************************************
    * Synchronize the ArrayView data between host & accl manually             *
    **************************************************************************/
    yuvArrayView.synchronize();

    sampleCommon->stopTimer(arrViewTimer);

    return AMP_SUCCESS;
}


/******************************************************************************
* Implementation of RgbToYuv::rgbToYuvAmpArray()                              *
******************************************************************************/
int RgbToYuv::rgbToYuvAmpArray(extent<1> inpExt,
                               std::vector<DATA_TYPE>& inputRgbData,
                               std::vector<DATA_TYPE>& outputFromArray,
                               int arrTimer)
{
    accelerator cpuAccl = accelerator(accelerator::cpu_accelerator);

    /**************************************************************************
    * Create C++ AMP arrays to be used.                                       *
    * Arrays are containers of data and they reside in accelerators.  Arrays  *
    * can be created with OR without data.  Staging Arrays are created only   *
    * when both the accelerator view and its associated view are specified    *
    **************************************************************************/
    array<DATA_TYPE, 1> rgbStagingArray(inpExt, cpuAccl.default_view, 
        deviceAccl.default_view);
    array<DATA_TYPE, 1> rgbDeviceArray(inpExt, deviceAccl.default_view);
    array<DATA_TYPE, 1> yuvArray(inpExt, deviceAccl.default_view);

    /**************************************************************************
    * Copy input data to ensure that arrays & array_views work on same input  *
    **************************************************************************/
    copy(inputRgbData.begin(), rgbStagingArray);

    sampleCommon->startTimer(arrTimer);

    /**************************************************************************
    * Data of an AMP Array must be explicitly copied from host to an accl     *
    * unless specified during AMP array creation. However with AMP arrays, an *
    * AMP runtime can be informed to optimize data transfers between staging  *
    * array & destination array without extra copy and buffer creation/release*
    **************************************************************************/
    copy(rgbStagingArray, rgbDeviceArray);

    /**************************************************************************
    * Note that Arrays can ONLY be captured "by reference" in any overload    *
    * of parallel_for_each()                                                  *
    **************************************************************************/
    parallel_for_each(rgbDeviceArray.extent/3, [&](index<1> idx) restrict(amp)
        {
            DATA_TYPE i = idx[0] * 3;
            float R = (float)rgbDeviceArray[i];
            float G = (float)rgbDeviceArray[i+1];
            float B = (float)rgbDeviceArray[i+2];

            float Y = (0.257f * R) + (0.504f * G) + (0.098f * B) + 16.f;
            float V = (0.439f * R) - (0.368f * G) - (0.071f * B) + 128.f;
            float U = -(0.148f * R) - (0.291f * G) + (0.439f * B) + 128.f;

	        yuvArray[i] = (DATA_TYPE)(Y > 255 ? 255 : Y);
	        yuvArray[i+1] = (DATA_TYPE)(U > 255 ? 255 : U);
	        yuvArray[i+2] = (DATA_TYPE)(V > 255 ? 255 : V);
        });

    /**************************************************************************
    * Copy the data on accelerator back on the host                           *
    **************************************************************************/
    copy(yuvArray, outputFromArray.begin());
 
    sampleCommon->stopTimer(arrTimer);

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of RgbToYuv::compare()                                       *
******************************************************************************/
int RgbToYuv::compare(std::vector<unsigned>& vTest,
                      std::vector<unsigned>& vRef)
{
    int passed = AMP_SUCCESS;
    unsigned totalMismatches = 0;

    for (int i = 0; i < inpDataLen; ++i)
    {
        if (vTest[i] != vRef[i])
        {
            if (!totalMismatches)
            {
                std::cout << " mismatch found " << std::endl << "vTest[" 
                    << i << "] = " << vTest[i] << ", vRef[" << i << "] = " 
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
* Execution of example begins from here                                       *
******************************************************************************/
int main(int argc, char *argv[])
{
    std::cout << "**********************************************" << std::endl;
    std::cout << "C++ AMP Array Vs ArrayView" << std::endl;
    std::cout << "**********************************************" << std::endl;

    /**************************************************************************
    * Create an object of RgbToYuv class                                      *
    **************************************************************************/
    RgbToYuv objRgbToYuv("C++Amp Array Vs ArrayView example");

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
    std::vector<DATA_TYPE> outputFromArrayView(numElements);
    std::vector<DATA_TYPE> outputFromArray(numElements);
    std::vector<DATA_TYPE> refVector(numElements);

    /**************************************************************************
    * Execute RgbToYuv 4:4:4 color conversion on the the input RGB data using *
    * AMP array and AMP array_view                                            *
    **************************************************************************/
    if(objRgbToYuv.run(inputRgbData, outputFromArrayView,
                       outputFromArray) != AMP_SUCCESS)
        return AMP_FAILURE;

    /**************************************************************************
    * Verify the results that were generated                                  *
    **************************************************************************/
    if(objRgbToYuv.verifyResults(inputRgbData, outputFromArrayView,
                                 outputFromArray, refVector) != AMP_SUCCESS)
        return AMP_FAILURE;

    /**************************************************************************
    * Print performance statistics                                            *
    **************************************************************************/
    objRgbToYuv.printStats();

    return 0;
}
