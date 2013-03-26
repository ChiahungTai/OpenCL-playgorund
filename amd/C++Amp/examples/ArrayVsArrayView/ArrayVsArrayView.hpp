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

#ifndef _ArrayVsArrayView_H_
#define _ArrayVsArrayView_H_

/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include <AmpCommon.h>
#include <AmpSample.h>
#include <AmpCommandArgs.h>
#include <amp.h>

/******************************************************************************
* Defined macros                                                              *
******************************************************************************/
#define DATA_TYPE       unsigned
#define NUM_ELEMENTS    (1024*768*3)

using namespace concurrency;


/******************************************************************************
* RgbToYuv                                                                    *
* Class implements RGB to YUV 4:4:4 conversion                                *
******************************************************************************/
class RgbToYuv : public AmpSample
{
private:
    int           inpDataLen;      /**< Number of RGB input data (3 channel) */
    double        arrExecTime;     /**< Time to execute Array version   */
    double        arrViewExecTime; /**< Time to execute ArrayView version   */
    double        cpuExecTime;     /**< Time to execute CPU version   */

public:
    /** 
    ***************************************************************************
    * @brief Constructor of RgbToYuv to initialize member variables
    **************************************************************************/
    RgbToYuv(std::string sampleName) : AmpSample(sampleName)
    {
        inpDataLen = NUM_ELEMENTS;
        arrExecTime = arrViewExecTime = cpuExecTime = 0;
    }

    /**
    ***************************************************************************
    * @brief Destructor of RgbToYuv
    **************************************************************************/
    ~RgbToYuv()
    {}

    /**
    ***************************************************************************
    * @fn initialize
    * @brief Initialize command line parser, adds custom command line options
    * @return AMP_SUCCESS on success and nonzero on failure
    **************************************************************************/
    int initialize();

    /**
    ***************************************************************************
    * @fn setup
    * @brief Initialize the random array of input data
    * @param[out] inputRgbData : vector to be populated with random values
    * @return AMP_SUCCESS on success and nonzero on failure
    **************************************************************************/
    int setup(std::vector<DATA_TYPE>& inputRgbData);

    /**
    ***************************************************************************
    * @fn run
    * @brief Run RGB to YUV 4:4:4 implementation using AMP array & array_view
    * @param[in] inputRgbData : vector containing generated RGB data
    * @param[out] outputFromArrayView : vector that will contain output data
    *                                   calculated by AMP array_view version
    * @param[out] outputFromArray : vector that will contain output data
    *                               calculated by AMP array version
    * @return AMP_SUCCESS on success and nonzero on failure
    **************************************************************************/
    int run(std::vector<DATA_TYPE>& inputRgbData,
            std::vector<DATA_TYPE>& outputFromArrayView,
            std::vector<DATA_TYPE>& outputFromArray);

    /**
    ***************************************************************************
    * @fn verifyResults
    * @brief Verify generated results against reference implementation
    * @param[in] inputRgbData : vector containing generated RGB data
    * @param[in] outputFromArrayView : vector that contains output data
    *                                  calculated by AMP array_view version
    * @param[in] outputFromArray : vector that contains output data calculated
    *                              by AMP array version
    * @param[out] refVector : vector that will contain output data 
    *                         calculated by CPU single core version
    * @return AMP_SUCCESS on success and nonzero on failure
    **************************************************************************/
    int verifyResults(std::vector<DATA_TYPE>& inputRgbData,
                      std::vector<DATA_TYPE>& outputFromArrayView,
                      std::vector<DATA_TYPE>& outputFromArray,
                      std::vector<DATA_TYPE>& refVector);

    /**
    ***************************************************************************
    * @fn printStats
    * @brief Print timer statistics
    * @return void
    **************************************************************************/
    void printStats();

    /**
    ***************************************************************************
    * @fn outputSummary
    * @brief Writes a standard benchmark summary to stdout
    * @return void
    **************************************************************************/
    void outputSummary();

    /**
    ***************************************************************************
    * @fn rgbToYuvSingleCpu
    * @brief This function converts interleaved RGB data to YUV 4:4:4 data on 
    *        CPU.  Output YUV data generated by is function will be used as  
    *        reference data for checking correctness of other implementations
    * @param[in] pRGBData : Pointer to array containing interleaved RGB data
    * @param[out]    vYUV : Vector that will contain the output YUV 4:4:4 data
    * @return AMP_SUCCESS on success and nonzero on failure
    **************************************************************************/
    int RgbToYuv::rgbToYuvSingleCpu(DATA_TYPE *pRGBData, 
                           std::vector<DATA_TYPE>& vYUV);

    /**
    ***************************************************************************
    * @fn rgbToYuvAmpArrayView
    * @brief This function converts interleaved RGB data to YUV 4:4:4 data on 
    *        GPU using C++ AMP array_views to tranfer data to and fro between
    *        CPU & GPU
    * @param[in]  inpExt : extent<1> specifying the number input data
    * @param[in]  inputRgbData : vector containing interleaved RGB data
    * @param[out] outputFromArrayView : vector that will contain output YUV data
    * @param[in]  arrViewTimer : timer ID of AMP array_view version
    * @return AMP_SUCCESS on success and nonzero on failure
    **************************************************************************/
    int RgbToYuv::rgbToYuvAmpArrayView(extent<1> inpExt,
                                  std::vector<DATA_TYPE>& inputRgbData,
                                  std::vector<DATA_TYPE>& outputFromArrayView,
                                  int arrViewTimer);

    /**
    ***************************************************************************
    * @fn rgbToYuvAmpArray
    * @brief This function converts interleaved RGB data to YUV 4:4:4 data on
    *        GPU using C++ AMP arrays to tranfer data to and fro between CPU &
    *        GPU
    * @param[in]  inpExt : extent<1> specifying the number input data
    * @param[in]  inputRgbData : vector containing interleaved RGB data
    * @param[out] outputFromArray : vector that will contain output YUV data
    * @param[in]  arrTimer : timer ID of AMP array version
    * @return AMP_SUCCESS on success and nonzero on failure
    **************************************************************************/
    int RgbToYuv::rgbToYuvAmpArray(extent<1> inpExt,
                                   std::vector<DATA_TYPE>& inputRgbData,
                                   std::vector<DATA_TYPE>& outputFromArray,
                                   int arrTimer);

    /**
    ***************************************************************************
    * @fn compare
    * @brief This function verifies that the values in vTest vector match with
    *        the corresponding values in vRef vector.  On failure it prints the 
    *        first non matching value along with their respective indices and 
    *        the total number of mismatches
    * @param[in] vTest : Vector containing input data 
    * @param[in]  vRef : Vector containing reference data
    * @return AMP_SUCCESS if all values match, AMP_FAILURE otherwise
    **************************************************************************/
    int compare(std::vector<unsigned>& vTest,
                std::vector<unsigned>& vRef);

    /**
    ***************************************************************************
    * @fn getInputDataLenth
    * @brief Returns the size of input data that the example will be run on
    * @return size of RGB interleaved data held in inpDataLen
    **************************************************************************/
    DATA_TYPE getInputDataLenth()
    {
        return inpDataLen;
    }
};


#endif 
