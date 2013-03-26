/**********************************************************************
Copyright ©2012 Advanced Micro Devices, Inc. All rights reserved.

C++ AMP kernels within this source tree are derivatives of kernels
from the SHOC project. Source or binary distribution of this project must
disclose derivation and include the SHOC license:

SHOC 1.1.2  license Copyright ©2011, UT-Battelle, LLC. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.
•	Neither the name of Oak Ridge National Laboratory, nor UT-Battelle, LLC, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

#ifndef _FFT_H_
#define _FFT_H_

/**********************************************************************
* Included header files                                               *
**********************************************************************/
#include <AmpCommon.h>
#include <AmpSample.h>
#include <AmpCommandArgs.h>

#include <amp_math.h>
#include <math.h>

#include <amp.h>
#include <amp_short_vectors.h>
#include <iostream>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdlib.h>
#include <vector>


/**********************************************************************
* namespace Concurrency                                               *
**********************************************************************/
using namespace concurrency;
using namespace concurrency::graphics;
using namespace concurrency::graphics::direct3d;


/**********************************************************************
* Defined macros                                                      *
**********************************************************************/
#define _USE_MATH_DEFINES

#define M_E            2.71828182845904523536
#define M_LOG2E        1.44269504088896340736
#define M_LOG10E      0.434294481903251827651
#define M_LN2         0.693147180559945309417
#define M_LN10         2.30258509299404568402
#define M_PI           3.14159265358979323846
#define M_PI_2         1.57079632679489661923
#define M_PI_4        0.785398163397448309616
#define M_1_PI        0.318309886183790671538
#define M_2_PI        0.636619772367581343076
#define M_2_SQRTPI     1.12837916709551257390
#define M_SQRT2        1.41421356237309504880
#define M_SQRT1_2     0.707106781186547524401
#define DIR            (INVERSE ? -1 : 1)

/**********************************************************************
* Defined macros,size of each 1D FFT in complex numbers.              *
**********************************************************************/
#define cNFFTPoints 512


/**********************************************************************
* FFT                                                                 *
* class implements various resources required by the test             *
* Publically Inherited from AmpSample                                 *
* Initialize the resources used by tests                              *
**********************************************************************/
template <typename FPTYPE, bool INVERSE, 
          typename FPTYPE2>
class FFT: public AmpSample 
{
    int                           iter;     /**< Number of iterations for kernel execution */
    int                         length;     /**< Benchmark length*/
    bool                     arrayview;     /**< use array_view or not */
    bool                  doValidation;     /**< Validation about input data */
    bool                     doDPTests;     /**< Double precision test */

    int                    vectorElems;     /**< Matrix element number*/
    std::vector<FPTYPE2>        vector;     /**< Input vector*/
    std::vector<FPTYPE2>  refDFTVector;     /**< Reference vector*/
    std::vector<FPTYPE2>     outVector;
    double             totalkernelTime;     /**< Kernel time */
    double                cpyToGpuTime;    /**< The transfer time between gpu and host */
    double               cpyToHostTime;    /**< The transfer time between gpu and host */
    double                   totalTime;    /**< Total execution time */

public:
    /******************************************************************
    * @brief Constructor, Initialize member variables
    * @param name name of sample (string)
    ******************************************************************/
    FFT(std::string name)
        : AmpSample(name)
    {
        vectorElems            = 1;
        length                = 12;
        iter                  = 10;
        doValidation        = true;
        arrayview          = false;
        totalkernelTime        = 0;
        cpyToGpuTime           = 0;
        cpyToHostTime          = 0;
        totalTime              = 0;
        doDPTests           = true;
    }

    
    /**
    *******************************************************************
    * @brief initialize
    * Initialize the resources used by tests
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    ******************************************************************/
    int initialize();

    /**
    *******************************************************************
    * @brief adjust member datas 
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    ******************************************************************/
    int setup();

    /**
    *******************************************************************
    * @brief Run C++AMP
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    ******************************************************************/
    int run();

    /**
    *******************************************************************
    * verifyResults
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    ******************************************************************/
    int verifyResults();

    /**
    *******************************************************************
    * @brief Override from AMPSample. Print sample stats.
    * @param stdStr Parameter
    * @param stats Statistic value of parameter
    * @param n number
    ******************************************************************/
    void printStats(std::string *stdStr, std::string * stats, int n){};

    /**
    *******************************************************************
    * @brief Overload from printStats
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    ******************************************************************/
    void printStats();

    /**
    *******************************************************************
    * @brief clear vectors
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    ******************************************************************/
    void cleanUp();

    /**
    *******************************************************************
    * @brief Writes a standard benchmark summary to stdout.
    * @return none
    ******************************************************************/
    int outputSummary();

    /**
    *******************************************************************
    * @brief Writes one line of mode timing/throughput data to stdout.
    * @param modeText 
    * @param workDone
    * @param workRateUnit
    * @return none
    ******************************************************************/
    void outputData(const std::string &modeText,
                    double workDone,
                    const std::string &workRateUnit);

    /**
    *******************************************************************
    * @brief Compute FFT
    * @param devVector
    * @param vectorElems
    * @return none
    ******************************************************************/
    void FFT_Amp(array<FPTYPE2, 1> &devVector,
                 array<FPTYPE2, 1> &devOutVector,
                 int vectorElems);

    /**
    *******************************************************************
    * @brief GPU computation using array_view
    * @param devVector
    * @param vectorElems
    * @return none
    ******************************************************************/
    void FFT_Amp_View(array_view<FPTYPE2, 1> &devVector,
                      array_view<FPTYPE2, 1> &devOutVector,
                      int vectorElems);

    /**
    *******************************************************************
    * @brief Compute FFT reference
    * @param vecTile
    * @param refDFTTile
    * @param nPoints
    * @param stride
    * @return none
    ******************************************************************/
    void ReferenceFFT(FPTYPE2 *vecTile,
                      FPTYPE2 *refDFTTile,
                      int nPoints,
                      int stride);

	  /**
    *******************************************************************
    * @brief Run FFT, using array
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    ******************************************************************/
    int runArray();

    /**
    *******************************************************************
    * @brief Run FFT, using array_view
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    ******************************************************************/
    int runArray_View();

};
#endif
