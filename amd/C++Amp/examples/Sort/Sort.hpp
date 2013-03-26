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

#ifndef _Sort_H_
#define _Sort_H_
/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include <AmpCommon.h>
#include <AmpSample.h>
#include <AmpCommandArgs.h>

#include <amp.h>
#include <amp_short_vectors.h>
#include <iostream>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdlib.h>
#include <vector>

/******************************************************************************
* Defined macros                                                              *
******************************************************************************/
#define NOMINMAX

/******************************************************************************
* Macro Definitions                                                           *
* Number of threads per AMP tile in reduction and bottom scan.                *
* (Top scan must have cNTiles threads per tile.)                              *
******************************************************************************/
#define cTileSize 256

/******************************************************************************
* Macro Definitions                                                           *
* Number of AMP tiles to launch for reduction and bottom scan.                *
* (Top scan launch consists of a single tile only.)                           *
******************************************************************************/
#define cNTiles 64

/******************************************************************************
* namespace concurrency                                                       *
******************************************************************************/
using namespace concurrency;
using namespace concurrency::graphics;

/******************************************************************************
* Sort                                                                        *
* class implements various resources required by the test                     *
* Publically Inherited from AmpSample                                         *
* Initialize the resources used by tests                                      *
******************************************************************************/
class Sort : public AmpSample 
{
    public:
        std::vector<unsigned int> integers;        /**< Input vector */
        std::vector<unsigned int> sortedIntegers;  /**< Cpu vector */
        int                  nIntegerElems;        /**< Input vector length*/

        int                         length;        /**< Benchmark length*/
        int                           iter;        /**< Number of iterations for kernel execution */
        bool                  doValidation;        /**< Validation about input data */
        double             totalKernelTime;        /**< Kernel time */
        double              cpToDeviceTime;        /**< The transfer time between gpu and host */
        double                cpToHostTime;        /**< The transfer time between gpu and host */
        double                   totalTime;        /**< Total execution time */

    public:
        /**
        ***********************************************************************
        * @brief Constructor
        * @Initialize member variables
        * @param name name of sample (string)
        **********************************************************************/
        Sort(std::string name)
            : AmpSample(name)
        {
            nIntegerElems = 0;
            length = 8;
            iter = 10;
            totalKernelTime = 0;
            cpToDeviceTime = 0;
            cpToHostTime = 0;
            totalTime = 0;
            doValidation = true;
        }
 
        /**
        ***********************************************************************
        * @fn initialize
        * @brief Initialize command line parser, add custom options
        * @return 0 on success Positive if expected and Non-zero on failure
        ***********************************************************************/
        int initialize();

        /**
        ***********************************************************************
        * @fn setup
        * @brief Adjust member datas
        * @return 0 on success Positive if expected and Non-zero on failure
        ***********************************************************************/
        int setup();

        /**
        ************************************************************************
        * @fn run
        * @brief Run C++AMP
        * @return 0 on success Positive if expected and Non-zero on failure
        ***********************************************************************/
        int run();
    
        /**
        ************************************************************************
        * @fn verifyResults
        * @brief Verify against reference implementation,overload from verifyResults
        * @return 0 on success Positive if expected and Non-zero on failure
        *************************************************************************/
        int verifyResults();

        /**
        ************************************************************************
        * @fn printStats
        * @brief Print sample stats,overload from printStats
        ***********************************************************************/
        void printStats();

        /**
        ************************************************************************
        * @fn outputSummary
        * @brief Writes a standard benchmark summary to stdout
        ***********************************************************************/
        void outputSummary();

        /**
        ************************************************************************
        * @fn outputSummary
        * @brief Writes one line of mode timing/throughput data to stdout
        ***********************************************************************/
        void outputData(const std::string &modeText,
                double workDone,
                const std::string &workRateUnit);

        /**
        ************************************************************************
        * @fn toString
        * @brief convert a T type to string
        ***********************************************************************/
        template<typename T>
        std::string toString(T t, std::ios_base & (*r)(std::ios_base&)); 

        /**
        ************************************************************************
        * @fn checkCommand
        * @brief check the commands
        ***********************************************************************/
        int checkCommand(); 

        /**
        ************************************************************************
        * @fn RadixSort
        * @brief Start sort
        ***********************************************************************/
        int RadixSort(array<unsigned int> &integers,
                array<unsigned int> &tmpIntegers,
                array<unsigned int> &tmpHistograms);

        /**
        ************************************************************************
        * @fn BuildHistograms
        * @brief Computes an unmerged set of histograms of the low 4-bit values
        * of a given set of integers, following a parameterized bitwise
        * right shift of each element
        ***********************************************************************/
        void BuildHistograms(array<unsigned int> &integers,
                     array<unsigned int> &histograms,
                     int shift);

        /**
        ************************************************************************
        * @fn ScanHistograms
        * @brief Performs a scan of each digit in a given set of histograms
        * (0-15 counts).Scanned histograms are not merged, but structured
        * the same as the source array
        ***********************************************************************/
        void ScanHistograms(array<unsigned int> &histograms);

        /**
        ************************************************************************
        * @fn SortIntegerKeys
        * @brief Sort the integer keys
        ***********************************************************************/
        void SortIntegerKeys(array<unsigned int> &integers,
                     array<unsigned int> &sortedIntegers,
                     array<unsigned int> &scannedHistograms,
                     int shift);

};
#endif 
