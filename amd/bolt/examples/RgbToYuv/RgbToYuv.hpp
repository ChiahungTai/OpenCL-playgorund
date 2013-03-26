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


#ifndef RGB_TO_YUV_H_
#define RGB_TO_YUV_H_

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
* rgbValue                                                                    *
* struct definition that contains RGB values to be used in the next functor   *
******************************************************************************/
BOLT_FUNCTOR(rgbValue,
    struct rgbValue
    {
        char r;
        char g;
        char b;
    };
);

/******************************************************************************
* yuvValue                                                                    *
* struct definition that contains YUV values to be used in the next functor   *
******************************************************************************/
BOLT_FUNCTOR(yuvValue,
    struct yuvValue
    {
        char y;
        char u;
        char v;
    };
);

/******************************************************************************
* rgbToYuvFunctor                                                             *
* functor definition that performs the color conversion from RGB to YUV 4:4:4 *
******************************************************************************/
BOLT_FUNCTOR(rgbToYuvFunctor,
    struct rgbToYuvFunctor
    {
        /**
        ***********************************************************************
        * @fn operator override used as functor to perfrom RgbToYuv conversion
        * @brief Converts RGB color space to YUV color space
        * @return a yuvValue struct variable converted YUV value
        **********************************************************************/
        yuvValue operator() (const rgbValue& rgb)
        {
            yuvValue yuv;
            float R = (float)rgb.r;
            float G = (float)rgb.g;
            float B = (float)rgb.b;

            float Y = (0.257f * R) + (0.504f * G) + (0.098f * B) + 16.f;
            float V = (0.439f * R) - (0.368f * G) - (0.071f * B) + 128.f;
            float U = -(0.148f * R) - (0.291f * G) + (0.439f * B) + 128.f;

            yuv.y = (char)(Y > 255 ? 255 : Y);
            yuv.u = (char)(U > 255 ? 255 : U);
	        yuv.v = (char)(V > 255 ? 255 : V);

            return (yuv);
        };
    };
);

/******************************************************************************
* RgbToYuv                                                                    *
* Class implements RGB to YUV 4:4:4 conversion                                *
******************************************************************************/
class RgbToYuv : public BoltSample
{
    
    std::vector<rgbValue> rgbData;  /** Random generated RGB values */
    std::vector<yuvValue> boltYuv;  /** BOLT calculated YUV values  */
    std::vector<yuvValue> cpuYuv;   /** CPU calculated YUV values   */

public:
    /** 
    ***************************************************************************
    * @brief Constructor of RgbToYuv to initialize member variables
    **************************************************************************/
    RgbToYuv(std::string sampleName) : BoltSample(sampleName, 1024 * 768)
    {}

    /**
    ***************************************************************************
    * @brief Destructor of RgbToYuv
    **************************************************************************/
    ~RgbToYuv()
    {
        rgbData.clear();
        boltYuv.clear();
        cpuYuv.clear();
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
    * @brief Run RGB to YUV 4:4:4 implementation in Bolt
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
    * @fn rgbToYuvCPU
    * @brief CPU version of RGB to YUV 4:4:4 conversion
    * @return SDK_SUCCESS on success and nonzero on failure
    **************************************************************************/
    int rgbToYuvCPU();

    /**
    ***************************************************************************
    * @fn rgbToYuvBOLT
    * @brief BOLT version of RGB to YUV 4:4:4 conversion
    * @return SDK_SUCCESS on success and nonzero on failure
    **************************************************************************/
    int rgbToYuvBOLT();

    /**
    ***************************************************************************
    * @fn compare
    * @brief compares cpuYuv with boltYuv and errors out if the values do not
    *        match lists the number of mis-matching values
    * @return true if all values match, false otherwise 
    **************************************************************************/
    bool compare();

};
#endif 
