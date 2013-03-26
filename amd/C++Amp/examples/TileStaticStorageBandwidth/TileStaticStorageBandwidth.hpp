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
* @file <Tile_staticStorageBandWidth.hpp>
* 
* @Define a class named TileStaticBandwidth,and we will implement the bandwidth
******************************************************************************/

#ifndef TILESTATIC_BANDWIDTH_H_
#define TILESTATIC_BANDWIDTH_H_

/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include <amp.h> 
#include <AmpCommandArgs.h>
#include <AmpSample.h>
#include <vector>
#include <iostream>

/******************************************************************************
* namespace concurrency                                                       *
* namespace std                                                               *
******************************************************************************/
using namespace Concurrency;
using namespace std;

/******************************************************************************
* Defined macros                                                              *
******************************************************************************/
#define TILE_SIZE 256
#define NUM_ACCESS 256
#define LENGTH 1024 * 1024

/******************************************************************************
* TileStaticBandwidth                                                         *
* class implements various resources required by the test                     *
* Publically Inherited from AmpSample                                         *
******************************************************************************/
template<class T>
class TileStaticBandwidth : public AmpSample
{
    vector<T> output;                    /**< the output vector */
    unsigned int outputLength;                /**< the length of output vector */
    double kernelRunTime;                   /**< Kernel run time */
    unsigned int iter;                        /**< Number of iteratons for kernel excution */
    bool isPassed;                            /**< the verify result */

public:
    /** 
    **************************************************************************
    * Constructor 
    * Initialize member variables
    *************************************************************************/
    TileStaticBandwidth(string name):AmpSample(name)
    {
        outputLength = LENGTH;
        kernelRunTime = 0;
        iter = 100;
        isPassed = true;
    }

    /**
    ************************************************************************** 
    * @brief command line parser, add custom options
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    *************************************************************************/
     int initialize();

    /** 
    **************************************************************************
    * @breif Allocate and initialize the input vector with random values and 
    *  set zero to the output vectors
    * @return AMP_SUCCESS on success and AMP_FAILURE on failed
    *************************************************************************/
    int setup();

    /**
    **************************************************************************
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    *************************************************************************/
    int run();

    /**
    **************************************************************************
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    *************************************************************************/
    int verifyResults();
private:

    /**
    **************************************************************************
    * @brief calculate bandwidth function 
    * @return AMP_SUCCESS on success and AMP_FAILURE on failure
    *************************************************************************/
    double calBandwidth(double);

    /**
    **************************************************************************
    * @fn readSingle   
    * @brief mode: Read single test 
    *************************************************************************/
	void readSingle(array<T> &);
    void measureReadSingle(array<T> &);

    /**
    **************************************************************************
    * @fn readLinear   
    * @brief mode: Read linear test 
    *************************************************************************/
	void readLinear(array<T> &);
    void measureReadLinear(array<T> &);

    /**
    **************************************************************************
    * @fn writeLinear 
    * @brief mode: Write linear test
    *************************************************************************/
    void writeLinear(array<T> &);
	void measureWriteLinear(array<T> &);
};


#endif