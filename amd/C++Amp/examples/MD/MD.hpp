/**********************************************************************
Copyright ©2012 Advanced Micro Devices, Inc. All rights reserved.

C++ AMP kernels within this source tree are derivatives of kernels
from the SHOC project. Source or binary distribution of this project must
disclose derivation and include the SHOC license:

SHOC 1.1.2  license Copyright ©2011, Copyright ©2011, UT-Battelle, LLC. All rights reserved.

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

#ifndef _MD_H_
#define _MD_H_

/******************************************************************************
* Included header files                                                                                                          *
******************************************************************************/
#include <AmpCommon.h>
#include <AmpSample.h>
#include <AmpCommandArgs.h>

#include <iomanip>
#include <amp_short_vectors.h>
#include <functional>
#include <iostream>
#include <queue>
#include <random>
#include <sstream>
#include <stdlib.h>

/******************************************************************************
* Defined macros                                                                                                                  *
* Maximum number of (nearest) atom neighbors to consider.                                                  *
* Distance^2 beyond which atomic interactions are not considered.                                         *
* Edge length of the atomic system bounding cube.                                                                *
* Constants used in Lennard-Jones equation.                                                                          *
******************************************************************************/
#define cMaxAtomNeighbors 64
#define cCutOffDistanceSqrd 16.0
#define cDefaultLength 20480
#define cDomainEdge 20.0
#define cLJConst1 1.5
#define cLJConst2 2.0

/******************************************************************************
* namespace                                                                                                                         *
******************************************************************************/
using namespace concurrency;
using namespace concurrency::graphics;

/******************************************************************************
* MD class                                                                                                                           *
* MD class description                                                                                                          *
* implements various resources required                                                                                *
* Publically Inherited from AmpSamples                                                                                *
* Initialize the resources                                                                                                         *
******************************************************************************/
template <typename FPTYPE>
class MD : public AmpSample 
{
    public:
        std::vector<double_4>     dpPositions;      /**< Double4 input vector */
        std::vector<float_4>         spPositions;      /**< Float4 input vector */
        std::vector<int>                   neighbors;      /**< neighbors */
        std::vector<typename short_vector<FPTYPE, 4>::type> forces;    /**Allocate space for output potential force.*/

        int                                          nAtoms;      /**< neighbors */
        int                                                 iter;      /**< Number of iterations for kernel execution */
        int                                             length;      /**< Benchmark length*/
        bool                                doValidation;      /**< Validation about input data */
        bool                                  doDPTests;      /**< Double precision support validation */

        double                        totalKernelTime;      /**< Kernel time */
        double                         cpToGpuTime;      /**< The transfer time between gpu and host */
        double                        cpToHostTime;      /**< The transfer time between gpu and host */
        double                                 totalTime;      /**< Total execution time */

    public:

    /**
    ******************************************************************
    * @fn MD()
    * @brief Constructor 
    * @brief Initialize member variables
    * @param name name of sample (string)
    ******************************************************************/
    MD(std::string name)
        : AmpSample(name)
    {
        nAtoms = 0;
        length = cDefaultLength;
        iter = 10;
        totalKernelTime = 0;
        doValidation = true;
        doDPTests = true;
        cpToGpuTime = 0;
        cpToHostTime = 0;
        totalTime = 0;
    }

    /**
    *******************************************************************
    * @fn initialize()
    * @brief command line parser, add custom options
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    ******************************************************************/
    int initialize();

    /**
    *******************************************************************
    * @fn setup()
    * @brief setup member data
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    ******************************************************************/
    int setup();

    /**
    *******************************************************************
    * @fn run()
    * @brief  Execute MD
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    ******************************************************************/
    int run(std::vector<typename short_vector<FPTYPE, 4>::type> &positions);

    /**
    *******************************************************************
    * @fn verifyResults()
    * @brief verify the results
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    ******************************************************************/
    int verifyResults(std::vector<typename short_vector<FPTYPE, 4>::type> &positions);

    /**
    *******************************************************************
    * @fn printStats()
    * @brief print some timer information
    ******************************************************************/
    void printStats();

    /**
    *******************************************************************
    * @fn outputSummary()
    * @brief Writes a standard benchmark summary to stdout.
    ******************************************************************/
    void outputSummary();

    /**
    *******************************************************************
    * @fn outputData()
    * @brief Writes one line of mode timing/throughput data to stdout.
    ******************************************************************/
    void outputData(const std::string &modeText,
                double workDone,
                const std::string &workRateUnit);

    /**
    *******************************************************************
    * @fn LJ()
    * @brief Lennard-Jones computation .
    ******************************************************************/
    void LJ(array<typename short_vector<FPTYPE, 4>::type> &positions,
            array<int> &neighbors,
            array<typename short_vector<FPTYPE, 4>::type> &forces,
            const FPTYPE cutOffDistSqrd,
            const FPTYPE ljConst1,
            const FPTYPE ljConst2);

    /**
    *******************************************************************
    * @randomizeInputData()
    * @brief Writes data into input.
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    ******************************************************************/
    int randomizeInputData();

     /**
     ******************************************************************
     * @brief Print the input and output
     * Print the results on the screen according to command .
     * @return AMP_SUCCESS on success and AMP_FAILURE on failure
     ******************************************************************/
    int printArray();
};
#endif
