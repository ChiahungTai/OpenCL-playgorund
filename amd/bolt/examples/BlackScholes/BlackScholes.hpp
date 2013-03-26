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


#ifndef BLACK_SCHOLES_H_
#define BLACK_SCHOLES_H_

/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include <iostream>
#include <vector>
#include <iostream>
#include <algorithm>
#include "BoltSample.hpp"
#include "bolt/cl/bolt.h"


/******************************************************************************
* blackScholesPrice                                                           *
* struct definition that contains call price & put price to be used in the    *
* next functor                                                                *
******************************************************************************/
BOLT_FUNCTOR(blackScholesPrice,
    struct blackScholesPrice
    {
        float callPrice;
        float putPrice;
    };
);

/******************************************************************************
* blackScholesFunctor                                                         *
* functor definition that performs the BlackScholes algorithm                 *
******************************************************************************/
BOLT_FUNCTOR(blackScholesFunctor,
    struct blackScholesFunctor
    {
        /**
        ***********************************************************************
        * @fn phi
        * @brief Abromowitz Stegun approxmimation for PHI on the 
        *        CPU(Cumulative Normal Distribution Function)
        * @return a float PHI value
        **********************************************************************/
        float phi(float X)
        {
            float y;
            float absX;
            float t;

            // the coeffs
            const float c1 =  0.319381530f;
            const float c2 = -0.356563782f;
            const float c3 =  1.781477937f;
            const float c4 = -1.821255978f;
            const float c5 =  1.330274429f;

            const float oneBySqrt2pi = 0.398942280f;

            absX = fabs(X);
            t = 1.0f / (1.0f + 0.2316419f * absX);

            y = 1.0f - oneBySqrt2pi * exp(-X * X / 2.0f) *
                t * (c1 +
                        t * (c2 +
                            t * (c3 +
                                t * (c4 + t * c5))));

            return ((X < 0) ? (1.0f - y) : y);
        };

        /**
        ***********************************************************************
        * @fn operator override used as functor to calculate BlackScholes 
        * @brief Calulates the call price and put price of a given option using
        *        BlackScholes formula as described in wikipedia
        *        http://en.wikipedia.org/wiki/Black%E2%80%93Scholes
        * @return a blackScholesPrice struct variable that contians the
        *         resultant floating point call price and put price
        **********************************************************************/
        blackScholesPrice operator() (const float& inpOption)
        {
            const float S_LOWER_LIMIT_ = 10.0f;
            const float S_UPPER_LIMIT_ = 100.0f;
            const float K_LOWER_LIMIT_ = 10.0f;
            const float K_UPPER_LIMIT_ = 100.0f;
            const float T_LOWER_LIMIT_ = 1.0f;
            const float T_UPPER_LIMIT_ = 10.0f;
            const float R_LOWER_LIMIT_ = 0.01f;
            const float R_UPPER_LIMIT_ = 0.05f;
            const float SIGMA_LOWER_LIMIT_ = 0.01f;
            const float SIGMA_UPPER_LIMIT_ = 0.10f;
            float d1, d2;
            float sigmaSqrtT;
            float KexpMinusRT;
            float s = S_LOWER_LIMIT_ * inpOption + S_UPPER_LIMIT_ * (1.0f - inpOption);
            float k = K_LOWER_LIMIT_ * inpOption + K_UPPER_LIMIT_ * (1.0f - inpOption);
            float t = T_LOWER_LIMIT_ * inpOption + T_UPPER_LIMIT_ * (1.0f - inpOption);
            float r = R_LOWER_LIMIT_ * inpOption + R_UPPER_LIMIT_ * (1.0f - inpOption);
            float sigma = SIGMA_LOWER_LIMIT_ * inpOption + SIGMA_UPPER_LIMIT_ * (1.0f - inpOption);
            blackScholesPrice price;

            sigmaSqrtT = sigma * sqrt(t);

            d1 = (log(s / k) + (r + sigma * sigma / 2.0f) * t) / sigmaSqrtT;
            d2 = d1 - sigmaSqrtT;

            KexpMinusRT = k * exp(-r * t);
            price.callPrice = s * phi(d1) - KexpMinusRT * phi(d2);
            price.putPrice  = KexpMinusRT * phi(-d2) - s * phi(-d1);
            return (price);
        };
    };
);

/******************************************************************************
* BlackScholes                                                                *
* Class implements Black-Scholes implementation for European Options          *
******************************************************************************/
class BlackScholes : public BoltSample
{
    std::vector<float> randOptions;             /**< Random generated options */
    std::vector<float> cpuOptions;             /**< Random generated options */
    std::vector<blackScholesPrice> cpuPrice;   /**< CPU call & put prices    */

    /**************************************************************************
    * Create input option and output prices on the device using device_vector *
    * to gain a huge performance boost. In this sample the input options used *
    * for cpu calculations are replicated to exist on the device              *
    **************************************************************************/
    bolt::cl::device_vector<float> boltOptions;             /**< Random generated options */
    bolt::cl::device_vector<blackScholesPrice> boltPrice;   /**< BOLT call & put prices   */

public:
    /** 
    ***************************************************************************
    * @brief Constructor of BlackScholes to initialize member variables
    * Allocating the input device_vector in host visible device device memory
    **************************************************************************/
    BlackScholes(std::string sampleName) : BoltSample(sampleName, 256*256*4), 
                                           boltOptions(0, 0.0f, CL_MEM_READ_ONLY | CL_MEM_USE_PERSISTENT_MEM_AMD),
                                           boltPrice(0, blackScholesPrice(), CL_MEM_WRITE_ONLY)
    {}

    /**
    ***************************************************************************
    * @brief Destructor of BlackScholes
    **************************************************************************/
    ~BlackScholes()
    {
        cpuOptions.clear();
        cpuPrice.clear();
        boltOptions.clear();
        boltPrice.clear();
    }

    /**
    ***************************************************************************
    * @fn setup
    * @brief Initialize the random array of input samples
    * @return SDK_SUCCESS on success and nonzero on failure
    **************************************************************************/
    int setup();

    /**
    ***************************************************************************
    * @fn run
    * @brief Run Black-Scholes implementation in Bolt
    * @return SDK_SUCCESS on success and nonzero on failure
    **************************************************************************/
    int run();

    /**
    ***************************************************************************
    * @fn verifyResults
    * @brief Verify against CPU reference implementation
    * @return SDK_SUCCESS on success and nonzero on failure
    **************************************************************************/
    int verifyResults();

    /**
    ***************************************************************************
    * @fn printStats
    * @brief Print timer statistics
    * @return void
    **************************************************************************/
    void printStats();

    /**
    ***************************************************************************
    * @fn blackScholesCPU
    * @brief CPU version of black scholes that calculates the call & put prices
    *        based on the input options
    * @return SDK_SUCCESS on success and nonzero on failure
    **************************************************************************/
    int blackScholesCPU();

    /**
    ***************************************************************************
    * @fn blackScholesBOLT
    * @brief BOLT version of black scholes that calculates the call & put 
    *        prices based on the input options
    * @return SDK_SUCCESS on success and nonzero on failure
    **************************************************************************/
    int blackScholesBOLT();

    /**
    ***************************************************************************
    * @fn compare
    * @brief compares cpuPrice with boltPrice and errors out if error is 
    *        greater than 1e-4f either in call price or put price
    * @return true if all values match, false otherwise 
    **************************************************************************/
    bool compare();
};

#endif 
