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

/*******************************************************************************
* @file <ArrayBandwidth.hpp>
* @Define a class named ArrayBandwidth,and we will implement the bandwidth
  @testing the bandwidth of Array_View and Array
******************************************************************************/

#ifndef ARRAYBANDWIDTH_HPP_
#define ARRAYBANDWIDTH_HPP_

/******************************************************************************
* Defined macros                                                              *
******************************************************************************/
#define NUM_READS 32
#define LENGTH 1024 * 256
#define OFFSET 4096 
#define INPUTLEHGTH NUM_READS * LENGTH
#define STRIDE 512

/******************************************************************************
* Included header files                                                       *
******************************************************************************/
//Header Files
#include <amp.h>
#include <iostream>
#include <string.h>
#include <AmpCommon.h>
#include <AmpSample.h>
#include <AmpCommandArgs.h>

/******************************************************************************
* namespace std Concurrency                                                   *
******************************************************************************/
using namespace std;
using namespace concurrency;

/**
 * ArrayBandwidth 
 * Class implements C++ AMP Array_View and Array Buffer Bandwidth sample
 */

template <class T>
class ArrayBandwidth : public AmpSample
{
    unsigned int inputLength;            /**< the length of input vector */
    unsigned int outputLength;           /**< the length of input vector */
    vector<T> input;                     /**< the  input vector */
    vector<T> outputReadSingle;          /**< Output Array for Read Single */
    vector<T> outputReadLinear;          /**< Output Array for Read Linear */
    vector<T> outputReadLU;              /**< Output Array for Read Linear(unCached) */
    vector<T> outputReadRandom;          /**< Output Array for Read Random */
    vector<T> outputReadStride;       /**< Output Array for Read Stride */
    vector<T> outputWriteLinear;         /**< Output Array for Write Linear */
    
    accelerator deviceAccl;

    double bytes;                       /**< Number of Bytes handled by each thread */
    unsigned int iter;                  /**< Number of iteratons for kernel excution */
    double setupTime;                   /**< Timer for setup */
    double totalkernelTime;             /**< Time for kernel execution */

public:
    /** 
     * @brief Constructor 
     * @param stdStr Parameter
     * Initialize member variables
     */
    ArrayBandwidth(std::string name):AmpSample(name)
    {
        
        inputLength = INPUTLEHGTH;
        outputLength = LENGTH;
        iter = 100;
        setupTime = 0;
        totalkernelTime = 0;

    }
    
    /**
     * @brief Create C++ AMP array to be used and set the output vectors to 0.  
     * and run the array bandwidth compute function of six modes.
     * @return AMP_SUCCESS on success and US_FAILURE on failed
     */
    int RunArrayBandwidthTesting();
     /**
     * @brief Override from AmpSample. Initialize 
     * command line parser, add custom options
     * @return AMAP_SUCCESS on success and SDK_FAILURE on failure
     */
    int initialize();
    /** 
     * @brief Allocate and initialize the input vector with random values and 
     * set zero to the output vectors
     * @return AMP_SUCCESS on success and US_FAILURE on failed
     */
    int setup();
     /**
     * @brief Run bandwidth testing
     * @return AMP_SUCCESS on success and SDK_FAILURE on failure
     */
    int run();
    
     /**
     * @brief Verify against reference implementation
     * @return AMP_SUCCESS on success and SDK_FAILURE on failure
     */
    int verifyResults();


private:

    
    /**
     * @brief Push 0 for the output vectors
     * @param the destination vector
     * @param the width of the data
     * @param the height of the data
     * @return AMP_SUCCESS on success and SDK_FAILURE on failed
     */
    int setZero(vector<T>& arry,const int width,const int height);
    
    /**
     * @brief Array Bandwidth function 
     * @param input array
     * @param outpur array
     * mode: Read  single 
     * @return AMP_SUCCESS on success and SDK_FAILURE on failed
     */
	void readSingleBandwidth(array<T,1>& in,array<T,1>& result);
    int RunReadSingleBandwidth(array<T,1>& in,array<T,1>& result);
    /**
     * @brief Array Bandwidth function 
     * @param input array
     * @param outpur array
     * mode: Read  Linear 
     * @return AMP_SUCCESS on success and SDK_FAILURE on failed
     */
	void readLinearBandwidth(array<T,1>& in,array<T,1>& result);
    int RunReadLinearBandwidth(array<T,1>& in,array<T,1>& result);
    /**
     * @brief Array Bandwidth function 
     * @param input array
     * @param outpur array
     * mode: Read  Linear(uncached) 
     * @return AMP_SUCCESS on success and SDK_FAILURE on failed
     */
	void readLUDBandwidth(array<T,1>& in,array<T,1>& result);
    int RunReadLUBandwidth(array<T,1>& in,array<T,1>& result);
    /**
     * @brief Array Bandwidth function 
     * @param input array
     * @param outpur array
     * mode: Read  Random 
     * @return AMP_SUCCESS on success and SDK_FAILURE on failed
     */
	void readRandomBandwidth(array<T,1>& in,array<T,1>& result);
    int RunReadRandomBandwidth(array<T,1>& in,array<T,1>& result);
    /**
     * @brief Array Bandwidth function 
     * @param input array
     * @param outpur array
     * mode: Read Stride
     * @return AMP_SUCCESS on success and SDK_FAILURE on failed
     */
	void readStrideBandwidth(array<T,1>& in,array<T,1>& result);
    int RunReadStrideBandwidth(array<T,1>& in,array<T,1>& result);
    /**
     * @brief Array Bandwidth function 
     * @param input array
     * @param outpur array
     * mode: Write Linear
     * @return AMP_SUCCESS on success and SDK_FAILURE on failed
     */
	void writeLinearBandwidth(array<T,1>& constValue,array<T,1>& result);
    int RunWriteLinearBandwidth(array<T,1>& constValue,array<T,1>& result);
    /*
    * Prints no more than 256 elements of the given array.
    * Prints full array if length is less than 256.
    * Prints Array name followed by elements.
    */
    int fillRandom(
         std::vector<T>& arrayPtr, 
         const int width,
         const int height,
         const T rangeMin,
         const T rangeMax,
         unsigned int seed);
};


#endif