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
#include "bolt/cl/transform.h"
#include "RgbToYuv.hpp"


/******************************************************************************
* Implementation of setup()                                                   *
******************************************************************************/
int RgbToYuv::setup()
{
    rgbData.resize(samples);
    boltYuv.resize(samples);
    cpuYuv.resize(samples);

    for(int i = 0; i < samples; i++)
    {
        rgbData[i].r = (char)((float)rand() / (float)RAND_MAX);
        rgbData[i].g = (char)((float)rand() / (float)RAND_MAX);
        rgbData[i].b = (char)((float)rand() / (float)RAND_MAX);
    }

    sampleTimer.setup(sampleName, iterations);

    if(!quiet)
        std::cout << "Completed setup() of RgbToYuv sample" << std::endl;
    return SDK_SUCCESS;
}


/******************************************************************************
* Implementation of run()                                                     *
******************************************************************************/
int RgbToYuv::run()
{
    for(unsigned i = 0; i < 1 && iterations != 1; i++)
    {
        if(rgbToYuvBOLT() != SDK_SUCCESS)
            return SDK_FAILURE;
        if(!quiet)
            std::cout << "Completed Warm up run of Bolt code" << std::endl;
    }

    if(!quiet)
        std::cout << "Executing RgbToYuv sample over " << iterations 
                  << " iteration(s)." << std::endl;

    for(int i = 0; i < iterations; i++)
    {
        sampleTimer.startTimer();
        if(rgbToYuvBOLT() != SDK_SUCCESS)
            return SDK_FAILURE;
        sampleTimer.stopTimer();
    }

    if(!quiet)
        std::cout << "Completed Run() of RgbToYuv sample" << std::endl;
    return SDK_SUCCESS;
}


/******************************************************************************
* Implementation of verifyResults()                                           *
******************************************************************************/
int RgbToYuv::verifyResults()
{
    if(verify)
    {
        rgbToYuvCPU();

        if(!quiet)
            std::cout << "\nComparing resulting data..." << std::endl;

        /**********************************************************************
        * Compare the resulting data vectors to ensure we did not goof up     *
        **********************************************************************/
        bool priceResult = compare();

        std::cout << "YUV 4:4:4 using Bolt : ";

        if(!priceResult)
        {
            std::cout << "Data MISMATCH\n" << std::endl;
            return SDK_FAILURE;
        }
        std::cout << "Data matches with reference\n" << std::endl;
    }

    if(!quiet)
        std::cout << "Completed verifyResults() of RgbToYuv sample" 
                  << std::endl;

    return SDK_SUCCESS;
}


/******************************************************************************
* Implementation of printStats()                                              *
******************************************************************************/
void RgbToYuv::printStats()
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
        std::cout << "Completed printStats() of RgbToYuv sample"<<std::endl;
}


/******************************************************************************
* Implementation of rgbToYuvCPU()                                             *
******************************************************************************/
int RgbToYuv::rgbToYuvCPU()
{
    std::transform(rgbData.begin(), rgbData.end(), 
        cpuYuv.begin(), rgbToYuvFunctor());
    return SDK_SUCCESS;
}


/******************************************************************************
* Implementation of rgbToYuvBOLT()                                            *
******************************************************************************/
int RgbToYuv::rgbToYuvBOLT()
{
    bolt::cl::transform(rgbData.begin(), rgbData.end(), 
        boltYuv.begin(), rgbToYuvFunctor());
    return SDK_SUCCESS;
}


/******************************************************************************
* Implementation of compare()                                                 *
******************************************************************************/
bool RgbToYuv::compare()
{
    bool passed = true;
    unsigned totalMismatches = 0;

    for (int i = 0; i < samples; ++i)
    {
        if (cpuYuv[i].y != boltYuv[i].y || cpuYuv[i].u != boltYuv[i].u 
            || cpuYuv[i].v != boltYuv[i].v)
        {
            if (!totalMismatches)
            {
                std::cout << "First mismatch found"  << std::endl << "cpuYuv[" 
                  << i << "].y = " << cpuYuv[i].y << ", boltYuv[" << i 
                  << "].y = " << boltYuv[i].y << std::endl << "cpuYuv[" << i 
                  << "].u = " << cpuYuv[i].u << ", boltYuv[" << i << "] = " 
                  << boltYuv[i].u << std::endl << "cpuYuv[" << i << "].v = " 
                  << cpuYuv[i].v << ", boltYuv[" << i << "].v = " 
                  << boltYuv[i].v << std::endl;
                passed = false;
            }
            totalMismatches++;
        }
    }
    
    if (totalMismatches)
        std::cout << "Total number of mismatches found = " << totalMismatches
                  << std::endl;

    return (totalMismatches == 0);
}

/******************************************************************************
* Execution of program begins from here                                       *
******************************************************************************/
int main(int argc, char * argv[])
{
    std::cout << "**********************************************" << std::endl;
    std::cout << "RgbToYuv using BOLT" << std::endl;
    std::cout << "**********************************************" << std::endl;
    std::cout << std::endl << std::endl;

    /**************************************************************************
    * Create an object of RgbToYuv class                                      *
    **************************************************************************/
    RgbToYuv objRgbToYuv("RgbToYuv Bolt Sample");
    
    /**************************************************************************
    * Parse command line options                                              *
    **************************************************************************/
    if(objRgbToYuv.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    /**************************************************************************
    * Initialize the random array of input samples                            *
    **************************************************************************/
    if(objRgbToYuv.setup() != SDK_SUCCESS)
        return SDK_FAILURE;

    /**************************************************************************
    * Execute RgbToYuv 4:4:4 color conversion on the the input RGB data       *
    **************************************************************************/
    if(objRgbToYuv.run() != SDK_SUCCESS)
        return SDK_FAILURE;

    /**************************************************************************
    * Verify the results that were generated                                  *
    **************************************************************************/
    if(objRgbToYuv.verifyResults() != SDK_SUCCESS)
        return SDK_FAILURE;

    /**************************************************************************
    * Print performance statistics                                            *
    **************************************************************************/
    objRgbToYuv.printStats();

    return 0;
}
