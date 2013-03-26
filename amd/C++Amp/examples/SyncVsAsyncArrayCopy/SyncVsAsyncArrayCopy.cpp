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
* @file <SyncVsAsyncArrayCopy.cpp>
* 
* @brief Implements GPU based RGB to YUV 4:4:4 conversion depicting differences   
*        in using AMP Array copy() & AMP Array copy_aysnc() to do the same
******************************************************************************/

/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include "SyncVsAsyncArrayCopy.hpp"
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
        std::cout << "Completed setup() of SyncVsAsyncArrayCopy example\n";

    return AMP_SUCCESS;
}


/******************************************************************************
* Implementation of RgbToYuv::run()                                           *
******************************************************************************/
int RgbToYuv::run(std::vector<DATA_TYPE>& inputRgbData,
                  std::vector<DATA_TYPE>& outputFromSync,
                  std::vector<DATA_TYPE>& outputFromAsync)
{
    /**************************************************************************
    * Create a C++ AMP extent 1/3 the size of the input data.  This will make *
    * the loop execute 3 times over the input data                            *
    **************************************************************************/
    unsigned numThreads = inpDataLen/3;

    /**************************************************************************
    * Create & init timer objects to track the execution time of various runs *
    **************************************************************************/
    int syncTimer = sampleCommon->createTimer();
    int asyncTimer = sampleCommon->createTimer();

    /**************************************************************************
    * Warmup code with the sole purpose of avoiding JIT (just-in-time)        *
    * compilation overhead in affecting performance measurements. Create C++  *
    * AMP arrays to be used for warmup only                                   *
    **************************************************************************/
    extent<1> inpExt(numThreads);
    array<DATA_TYPE, 1> rgbSyncArray(inpExt);
    array<DATA_TYPE, 1> yuvArray(inpExt);
    rgbToYuvAmp(rgbSyncArray, yuvArray);
    if(!quiet)
        std::cout << "Completed Warm up run of C++AMP code" << std::endl;

    sampleCommon->resetTimer(syncTimer);
    sampleCommon->resetTimer(asyncTimer);

    /**************************************************************************
    * Execute RGB -> YUV 4:4:4 using Synchronous Array copy                   *
    **************************************************************************/
    if(!quiet)
        std::cout << "Executing AMP SynchronousCopy version of RGB to YUV(4:4:4)...\n";

    sampleCommon->startTimer(syncTimer);
    syncArrayCopy(inputRgbData, outputFromSync, numThreads);
    sampleCommon->stopTimer(syncTimer);
    syncExecTime = (double)(sampleCommon->readTimer(syncTimer));

    /**************************************************************************
    * Execute RGB -> YUV 4:4:4 using Asynchronous Array copy                  *
    **************************************************************************/
    if(!quiet)
        std::cout << "Executing AMP AsynchronousCopy version of RGB to YUV(4:4:4)...\n";

    sampleCommon->startTimer(asyncTimer);
    asyncArrayCopy(inputRgbData, outputFromAsync, numThreads);
    sampleCommon->stopTimer(asyncTimer);
    asyncExecTime = (double)(sampleCommon->readTimer(asyncTimer));

    if(!quiet)
        std::cout << "Completed run() of SyncVsAsyncArrayCopy example\n";

    return AMP_SUCCESS;
}


/******************************************************************************
* Implementation of RgbToYuv::verifyResults()                                 *
******************************************************************************/
int RgbToYuv::verifyResults(std::vector<DATA_TYPE>& inputRgbData,
                            std::vector<DATA_TYPE>& outputFromSync,
                            std::vector<DATA_TYPE>& outputFromAsync,
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

        std::cout << "Output of AMP Synchronous Copy ";
        if (compare(outputFromSync, refVector) != AMP_SUCCESS)
            std::cout << "Data MISMATCH\n";
        else
            std::cout << "matches the reference output of CPU\n";

        std::cout << "Output of AMP Asynchronous Copy ";
        if (compare(outputFromAsync, refVector) != AMP_SUCCESS)
        {
            std::cout << "Data MISMATCH\n\n";
            return AMP_FAILURE;
        }
        std::cout << "matches the reference output of CPU\n\n";
    }

    if(!quiet)
        std::cout << "Completed verifyResults() of SyncVsAsyncArrayCopy example\n";

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
        std::cout << "Asynchronous Copy Exec Time : " << syncExecTime << "s\n";
        std::cout << "Synchronous Copy Exec Time  : " << asyncExecTime << "s\n";
        std::cout << "CPU Exec Time               : " << cpuTimeStr.str() << "\n";
    }

    if(!quiet)
        std::cout << "Completed printStats() of SyncVsAsyncArrayCopy example\n";
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
* Implementation of RgbToYuv::syncArrayCopy()                                 *
******************************************************************************/
int RgbToYuv::syncArrayCopy(std::vector<DATA_TYPE>& inputRgbData,
                            std::vector<DATA_TYPE>& outputFromSync,
                            unsigned numThreads)
{
    /**************************************************************************
    * Create a C++ AMP extent of the required size and dimension              *
    **************************************************************************/
    extent<1> inpExt(numThreads);
    std::vector<DATA_TYPE>::iterator inpIter;

    /**************************************************************************
    * Create C++ AMP arrays to be used                                        *
    **************************************************************************/
    array<DATA_TYPE, 1> rgbSyncArray(inpExt);
    array<DATA_TYPE, 1> yuvArray(inpExt);

    /**************************************************************************
    * Execute RGB -> YUV 4:4:4 on the GPU using copy().  The data to be used  *
    * in that iteration is copied to GPU, utilized by GPU algo & and the      *
    * resulting output is copied over to the host synchronously.              *
    **************************************************************************/
    for (unsigned i = 0; i < inpDataLen/numThreads; i++)
    {
        inpIter = inputRgbData.begin() + (i * numThreads);
        copy(inpIter, inpIter + numThreads, rgbSyncArray);
        rgbToYuvAmp(rgbSyncArray, yuvArray);
        copy(yuvArray, outputFromSync.begin() + (i * numThreads));
    }

    if(!quiet)
        std::cout << "Completed syncArrayCopy() of SyncVsAsyncArrayCopy example\n";

    return AMP_SUCCESS;
}


/******************************************************************************
* Implementation of RgbToYuv::asyncArrayCopy()                                *
******************************************************************************/
int RgbToYuv::asyncArrayCopy(std::vector<DATA_TYPE>& inputRgbData,
                             std::vector<DATA_TYPE>& outputFromAsync,
                             unsigned numThreads)
{
    /**************************************************************************
    * Create a C++ AMP extent of the required size and dimension              *
    **************************************************************************/
    extent<1> inpExt(numThreads);

    /**************************************************************************
    * Create C++ AMP arrays & completion_future object to be used             *
    **************************************************************************/
    array<DATA_TYPE, 1> rgbAsyncInp1(inpExt);
    array<DATA_TYPE, 1> rgbAsyncInp2(inpExt);
    array<DATA_TYPE, 1> rgbAsyncInp3(inpExt);
    array<DATA_TYPE, 1> yuvArray1(inpExt);
    array<DATA_TYPE, 1> yuvArray2(inpExt);
    array<DATA_TYPE, 1> yuvArray3(inpExt);
    completion_future cfCopy[3];

    /**************************************************************************
    * An asynchronous copy is only scheduled and will need to be tracked by   *
    * using the completion_future objects that are returned. Here we schedule *
    * all the async copies together                                           *
    **************************************************************************/
    cfCopy[0] = copy_async(inputRgbData.begin(), 
                         inputRgbData.begin() + numThreads, rgbAsyncInp1);
    cfCopy[1] = copy_async(inputRgbData.begin() + numThreads, 
                         inputRgbData.begin() + (2*numThreads), rgbAsyncInp2);
    cfCopy[2] = copy_async(inputRgbData.begin() + (2*numThreads), 
                         inputRgbData.begin() + (3*numThreads), rgbAsyncInp3);

    /**************************************************************************
    * Execute RGB -> YUV 4:4:4 on the GPU using copy_async(). Wait for the    *
    * async copy to complete before launching the kernel. After completion of *
    * RGB -> YUV 4:4:4 start another copy_async() to the required destination *
    * tracked by completion_future object to ensure the completion of copy    *
    **************************************************************************/
    cfCopy[0].wait();
    rgbToYuvAmp(rgbAsyncInp1, yuvArray1);
    cfCopy[0] = copy_async(yuvArray1, outputFromAsync.begin());

    cfCopy[1].wait();
    rgbToYuvAmp(rgbAsyncInp2, yuvArray2);
    cfCopy[1] = copy_async(yuvArray2, outputFromAsync.begin() + numThreads);

    cfCopy[2].wait();
    rgbToYuvAmp(rgbAsyncInp3, yuvArray3);
    cfCopy[2] = copy_async(yuvArray3, outputFromAsync.begin() + (2*numThreads));

    cfCopy[0].wait();
    cfCopy[1].wait();
    cfCopy[2].wait();

    if(!quiet)
        std::cout << "Completed asyncArrayCopy() of SyncVsAsyncArrayCopy example\n";

    return AMP_SUCCESS;
}


/******************************************************************************
* Implementation of RgbToYuv::rgbToYuvAmp()                                   *
******************************************************************************/
int RgbToYuv::rgbToYuvAmp(array<DATA_TYPE, 1>& arrRGB, 
                          array<DATA_TYPE, 1>& arrYUV)
{
    /**************************************************************************
    * Note that Arrays can ONLY be captured "by reference" in any overload    *
    * of parallel_for_each()                                                  *
    **************************************************************************/
    parallel_for_each(arrYUV.extent/3, [&](index<1> idx) restrict(amp)
        {
            DATA_TYPE i = idx[0] * 3;
            float R = (float)arrRGB[i];
            float G = (float)arrRGB[i+1];
            float B = (float)arrRGB[i+2];

            float Y = (0.257f * R) + (0.504f * G) + (0.098f * B) + 16.f;
            float V = (0.439f * R) - (0.368f * G) - (0.071f * B) + 128.f;
            float U = -(0.148f * R) - (0.291f * G) + (0.439f * B) + 128.f;

	        arrYUV[i] = (DATA_TYPE)(Y > 255 ? 255 : Y);
	        arrYUV[i+1] = (DATA_TYPE)(U > 255 ? 255 : U);
	        arrYUV[i+2] = (DATA_TYPE)(V > 255 ? 255 : V);
        });

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
* Execution of program begins from here                                       *
******************************************************************************/
int main(int argc, char *argv[])
{
    std::cout << "**********************************************" << std::endl;
    std::cout << "C++ AMP Synchronous Vs Asynchronous Array Copy" << std::endl;
    std::cout << "**********************************************" << std::endl;

    /**************************************************************************
    * Create an object of RgbToYuv class                                      *
    **************************************************************************/
    RgbToYuv objRgbToYuv("C++Amp Sync Vs Async Array Copy example");

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
    std::vector<DATA_TYPE> outputFromSync(numElements);
    std::vector<DATA_TYPE> outputFromAsync(numElements);
    std::vector<DATA_TYPE> refVector(numElements);

    /**************************************************************************
    * Execute RgbToYuv 4:4:4 color conversion on the the input RGB data using *
    * AMP array and AMP array_view                                            *
    **************************************************************************/
    if(objRgbToYuv.run(inputRgbData, outputFromSync,
                       outputFromAsync) != AMP_SUCCESS)
        return AMP_FAILURE;

    /**************************************************************************
    * Verify the results that were generated                                  *
    **************************************************************************/
    if(objRgbToYuv.verifyResults(inputRgbData, outputFromSync,
                                 outputFromAsync, refVector) != AMP_SUCCESS)
        return AMP_FAILURE;

    /**************************************************************************
    * Print performance statistics                                            *
    **************************************************************************/
    objRgbToYuv.printStats();

    return 0;
}
