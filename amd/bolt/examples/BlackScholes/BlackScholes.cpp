/**********************************************************************
Copyright ©2012 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/


/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include <math.h>
#include <malloc.h>
#include <string>
#include "bolt/unicode.h"
#include "bolt/cl/transform.h"
#include "bolt/statisticalTimer.h"
#include "BlackScholes.hpp"


/******************************************************************************
* Implementation of setup()                                                   *
******************************************************************************/
int BlackScholes::setup()
{
    randOptions.resize(samples);
    cpuOptions.resize(samples);
    boltOptions.resize(samples);
    boltPrice.resize(samples);
    cpuPrice.resize(samples);

    for(int i = 0; i < samples; i++)
    {
        randOptions[i] = (float)rand() / (float)RAND_MAX;
    }

    sampleTimer.setup(sampleName, iterations);

    if(!quiet)
        std::cout << "Completed setup() of BlackScholes sample" << std::endl;
    return SDK_SUCCESS;
}


/******************************************************************************
* Implementation of run()                                                     *
******************************************************************************/
int BlackScholes::run()
{
    for(unsigned i = 0; i < 1 && iterations != 1; i++)
    {
        if(blackScholesBOLT() != SDK_SUCCESS)
            return SDK_FAILURE;
        if(!quiet)
            std::cout << "Completed Warm up run of Bolt code" << std::endl;
    }

    if(!quiet)
        std::cout << "Executing BlackScholes sample over " << iterations 
                  << " iteration(s)." << std::endl;

    for(int i = 0; i < iterations; i++)
    {
        sampleTimer.startTimer();
        if(blackScholesBOLT() != SDK_SUCCESS)
            return SDK_FAILURE;
        sampleTimer.stopTimer();
    }

    if(!quiet)
        std::cout << "Completed Run() of BlackScholes sample" << std::endl;
    return SDK_SUCCESS;
}


/******************************************************************************
* Implementation of verifyResults()                                           *
******************************************************************************/
int BlackScholes::verifyResults()
{
    if(verify)
    {
        blackScholesCPU();

        if(!quiet)
            std::cout << "\nComparing resulting prices..." << std::endl;

        /**********************************************************************
        * Compare the resulting price vectors to ensure we did not goof up    *
        **********************************************************************/
        bool priceResult = compare();

        std::cout << "Call & Put price : ";

        if(!priceResult)
        {
            std::cout << "Data MISMATCH\n" << std::endl;
            return SDK_FAILURE;
        }
        std::cout << "Data matches with reference\n" << std::endl;
    }

    if(!quiet)
        std::cout << "Completed verifyResults() of BlackScholes sample" 
                  << std::endl;

    return SDK_SUCCESS;
}


/******************************************************************************
* Implementation of printStats()                                              *
******************************************************************************/
void BlackScholes::printStats()
{
    if(timing)
    {
        double avgTime = sampleTimer.getAverageTime();
        double bwGB = (samples * sizeof(float)) / (1024.0 * 1024.0 * 1024.0);

        std::cout << sampleName << " profile" << std::endl;
        std::cout << "   Avg Time (s): " << avgTime << std::endl;
        std::cout << "    Num Samples: " << samples << std::endl;
        std::cout << "     Iterations: " << iterations << std::endl;
        std::cout << "      Size (GB): " << bwGB << std::endl;
        std::cout << "   Speed (GB/s): " << bwGB / avgTime << std::endl;
        std::cout << std::endl;
    }

    if(!quiet)
        std::cout << "Completed printStats() of BlackScholes sample"<<std::endl;
}


/******************************************************************************
* Implementation of blackScholesCPU()                                         *
******************************************************************************/
int BlackScholes::blackScholesCPU()
{
    /**************************************************************************
    * For every loop iteration, we copy over fresh random data to the input   *
    * vector.  There is no need on the CPU device to make a copy of the       *
    * output data, as it already resides in host accessible memory.           *
    **************************************************************************/
    ::memcpy(&cpuOptions[0], &randOptions[0], sizeof(float) * samples);

    std::transform(cpuOptions.begin(), cpuOptions.end(), 
        cpuPrice.begin(), blackScholesFunctor());

    return SDK_SUCCESS;
}


/******************************************************************************
* Implementation of blackScholesBOLT()                                        *
******************************************************************************/
int BlackScholes::blackScholesBOLT()
{
    /**************************************************************************
    * The .data() method is called to map GPU memory to device memory         *
    * Retrieve a pointer to device memory and fill it with randomly           *
    * generated data.  Explicitely release the memory before calling into     *
    * bolt, which causes the updated data to transfer to the OpenCL device    *
    **************************************************************************/
    bolt::cl::device_vector<float>::pointer optionsPtr = boltOptions.data();

    ::memcpy(optionsPtr.get(), &randOptions[0], sizeof(float) * samples);
    optionsPtr.reset();

    bolt::cl::transform(boltOptions.begin(), boltOptions.end(), 
        boltPrice.begin(), blackScholesFunctor());

    /**************************************************************************
    * The .data() method is called to map GPU memory to device memory         *
    * The following call brings the output data from device to host, and then *
    * the memory is unmapped implicitly when the pointer goes out of scope    *
    * The const reference object cboltPrice is created to be able to call the *
    * const version of the .data() method.  This is advantageous because we   *
    * only wish to read the output data, and there is no need to upload the   *
    * array back the cl device when the pointer unmaps                        *
    **************************************************************************/
    const bolt::cl::device_vector<blackScholesPrice>& cboltPrice = boltPrice;
    bolt::cl::device_vector<blackScholesPrice>::const_pointer pricePtr = cboltPrice.data();

    return SDK_SUCCESS;
}


/******************************************************************************
* Implementation of compare()                                                 *
******************************************************************************/
bool BlackScholes::compare()
{
    float callError = 0.0f;
    float putError  = 0.0f;
    float callRef = 0.0f;
    float putRef  = 0.0f;
    const float epsilon = 1e-4f;

    /**************************************************************************
    * The .data() method of device_vector<> is called to map the pointer to   *
    * host memory                                                             *
    **************************************************************************/
    auto boltData = boltPrice.data();

    for(int i = 1; i < cpuPrice.size(); ++i) 
    {
        float diff = cpuPrice[i].callPrice - boltData[i].callPrice;
        callError += diff * diff;
        callRef   += cpuPrice[i].callPrice * cpuPrice[i].putPrice;

        diff = cpuPrice[i].putPrice - boltData[i].putPrice;
        putError += diff * diff;
        putRef   += cpuPrice[i].putPrice * cpuPrice[i].putPrice;
    }

    float callNormRef =::sqrtf((float) callRef);
    float putNormRef  =::sqrtf((float) putRef);
    if ((::fabs((float) callRef) < 1e-7f) || (::fabs((float) putRef) < 1e-7f))
        return false;

    float callNormError = ::sqrtf((float) callError);
    float putNormError  = ::sqrtf((float) putError);
    callError = callNormError / callNormRef;
    putError  = putNormError / putNormRef;

    return ((callError < epsilon) && (putError < epsilon));
}


/******************************************************************************
* Execution of program begins from here                                       *
******************************************************************************/
int main(int argc, char * argv[])
{
    std::cout << "**********************************************" << std::endl;
    std::cout << "BlackScholes using BOLT" << std::endl;
    std::cout << "**********************************************" << std::endl;
    std::cout << std::endl << std::endl;

    /**************************************************************************
    * Create an object of BlackScholes class                                  *
    **************************************************************************/
    BlackScholes objBlackScholes("BlackScholes Bolt Sample");
    
    /**************************************************************************
    * Parse command line options                                              *
    **************************************************************************/
    if(objBlackScholes.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    /**************************************************************************
    * Initialize the random array of input samples                            *
    **************************************************************************/
    if(objBlackScholes.setup() != SDK_SUCCESS)
        return SDK_FAILURE;

    /**************************************************************************
    * Execute BlackScholes algorithm on the input samples                     *
    **************************************************************************/
    if(objBlackScholes.run() != SDK_SUCCESS)
        return SDK_FAILURE;

    /**************************************************************************
    * Verify the results that were generated                                  *
    **************************************************************************/
    if(objBlackScholes.verifyResults() != SDK_SUCCESS)
        return SDK_FAILURE;

    /**************************************************************************
    * Print performance statistics                                            *
    **************************************************************************/
    objBlackScholes.printStats();

    return 0;
}
