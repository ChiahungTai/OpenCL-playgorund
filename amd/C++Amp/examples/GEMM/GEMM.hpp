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

#ifndef _GEMM_H_
#define _GEMM_H_

/**************************************************************************
* Included header files                                                   *
**************************************************************************/
#include <AmpCommon.h>
#include <AmpSample.h>
#include <AmpCommandArgs.h>
#include <iomanip>

#include <amp.h>
#include <iostream>
#include <random>
#include <stdlib.h>
#include <vector>

/**************************************************************************
* namespace concurrency                                                   *
**************************************************************************/
using namespace concurrency;

/**************************************************************************
* Class GEMM                                                              *
* Publically Inherited from AmpSample                                     *
**************************************************************************/
template <typename FPTYPE, bool TRANSA>
class GEMM : public AmpSample
{
    std::vector<FPTYPE>    matrixA;    /**< MatrixA vector*/
    std::vector<FPTYPE>    matrixB;    /**< MatrixB vector*/
    std::vector<FPTYPE>    matrixC;    /**< MatrixC vector*/
    std::vector<FPTYPE>  matrixOut;    /**< Matrix copy from matrixC*/

    int                 matrixEdge;    /**< Matrix edge*/
    int                matrixElems;    /**< Matrix element number*/
    int                     length;    /**< Benchmark length*/
    int                       iter;    /**< Number of iterations for kernel execution */
    bool                 arrayview;    /**< use array_view or not */

    FPTYPE                   alpha;
    FPTYPE                    beta;

    double         totalKernelTime;    /**< Kernel time */
    double          cpToDeviceTime;    /**< The transfer time between gpu and host */
    double            cpToHostTime;    /**< The transfer time between gpu and host */
    double               totalTime;    /**< Total execution time */

public:

    /**
    *******************************************************************
    * @brief Initialize member variables
    * @param name name of sample (string)
    ******************************************************************/
    GEMM(std::string name)
        : AmpSample(name)
    {
        beta = 0;
        alpha = 0;
        matrixEdge = 1;
        matrixElems = 1;
        length = 2048;
        iter = 10;
        arrayview = false;
        totalKernelTime = 0;
        cpToDeviceTime = 0;
        cpToHostTime = 0;
        totalTime = 0;
    }

    /**
    *******************************************************************
    * @brief Override from AMPSample. Initialize 
    * command line parser, add custom options
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    ******************************************************************/
    int initialize();

    /**
    *******************************************************************
    * Adjust member datas 
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    ******************************************************************/
    int setup();

    /**
    *******************************************************************
    * @brief Run C++Amp
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    ******************************************************************/
    int run();

    /**
    *******************************************************************
    * @fn verifyResults
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    ******************************************************************/
    int verifyResults();

    /**
    *******************************************************************
    * @fn printStats
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    ******************************************************************/
    void printStats();

    /**
    *******************************************************************
    * @brief Writes a standard benchmark summary to stdout.
    ******************************************************************/
    void outputSummary();

    /**
    *******************************************************************
    * @brief Writes one line of mode timing/throughput data to stdout.
    ******************************************************************/
    void outputData(double workDone,
        const std::string &workRateUnit);

    /**
    *******************************************************************
    * @brief Compute GEMM.
    ******************************************************************/
    void GEMM_Amp(array<FPTYPE, 2> &devMatrixA,
        array<FPTYPE, 2> &devMatrixB,
        array<FPTYPE, 2> &devMatrixC,
        array<FPTYPE, 2> &devMatrixOut,
        FPTYPE alpha,
        FPTYPE beta,
        bool transA,
        int matrixEdge);

    /**
    *******************************************************************
    * @brief Compute GEMM using array view.
    ******************************************************************/
    void GEMM_Amp_View(array_view<FPTYPE, 2> &devMatrixA,
        array_view<FPTYPE, 2> &devMatrixB,
        array_view<FPTYPE, 2> &devMatrixC,
        array_view<FPTYPE, 2> &devMatrixOut,
        FPTYPE alpha,
        FPTYPE beta,
        bool transA,
        int matrixEdge);

    /**
    *******************************************************************
    * @brief Run GEMM using array.
    ******************************************************************/
    int runArray();

    /**
    *******************************************************************
    * @brief Run GEMM using array_view.
    ******************************************************************/
    int runArrayView();

};
#endif 
