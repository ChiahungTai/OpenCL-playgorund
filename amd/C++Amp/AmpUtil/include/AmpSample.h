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
//
// Copyright (c) 2008 Advanced Micro Devices, Inc. All rights reserved.
//

#ifndef AMPSAMPLE_H_
#define AMPSAMPLE_H_

/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include <AmpCommon.h>
#include <AmpCommandArgs.h>

/******************************************************************************
* namespace Concurrency                                                       *
******************************************************************************/
using namespace Concurrency;

/******************************************************************************
* AmpSample                                                                   *
* class implements various resources required by the test                     *
* Publically Inherited from AmpApplication                                    *
* Initialize the resources used by tests                                      *
******************************************************************************/
class AmpSample
{
    protected:
        ampsdk::AmpCommandArgs *sampleArgs;     /**< AmpCommandArgs class object to handle comd line options */
        ampsdk::AmpCommon * sampleCommon;       /**< AmpCommon class object */	
        ampsdk::sdkVersionStr sdkVerStr;        /**< SDK version string */
        bool version;                           /**< Cmd Line Option- if version */
        std::string name;                       /**< Name of the Sample */
        double totalTime;                       /**< Total Time taken by the Sample */
        bool quiet;                             /**< Cmd Line Option- if Quiet */
        bool verify;                            /**< Cmd Line Option- if verify */
        bool timing;                            /**< Cmd Line Option- if Timing */
        unsigned int deviceId;                  /**< Cmd Line Option- if deviceId */
        bool enableDeviceId;                    /**< Cmd Line Option- if enableDeviceId */
        accelerator deviceAccl;                 /**< Cmd Line Option- if enableDeviceId */

    protected:

        /**
        ***********************************************************************
        * @fn initialize
        * @brief Initialize the resources used by tests
        * @return 0 on success Positive if expected and Non-zero on failure
        **********************************************************************/
        int initialize();

        /**
        ***********************************************************************
        * @brief Print the results from the test
        * @param stdStr Parameter
        * @param stats Statistic value of parameter
        * @param n number
        **********************************************************************/
        void printStats(std::string *stdStr, std::string * stats, int n);

        /**
        ***********************************************************************
        * @brief Destroy the resources used by tests
        **********************************************************************/
        virtual ~AmpSample();

    public:
        /**
        ***********************************************************************
        * @brief Constructor, initialize the resources used by tests
        * @param sampleName Name of the Sample
        **********************************************************************/
        AmpSample(std::string sampleName);

        /**
        ***********************************************************************
        * @brief parseCommandLine parses the command line options given by user
        * @param argc Number of elements in cmd line input
        * @param argv array of char* storing the CmdLine Options
        * @return 0 on success Positive if expected and Non-zero on failure
        **********************************************************************/
        int parseCommandLine(int argc, char **argv);

        /**
        ***********************************************************************
        * @brief Displays the various options available for any sample
        **********************************************************************/
        void usage();

        /**
        ***********************************************************************
        * @brief Print all available devices 
        * @return 0 on success Positive if expected and Non-zero on failure
        **********************************************************************/
        int printDeviceList();

        /**
        ***********************************************************************
        * @brief Validates if the intended platform and device is used
        * @return 0 on success Positive if expected and Non-zero on failure
        **********************************************************************/
        int validateDeviceOptions();

        /**
        ********************************************************************************
        * @fn setDefaultAccelerator
        * @brief Set a default accelerator
        *******************************************************************************/
        int setDefaultAccelerator();

        /**
        ***********************************************************************
        * @brief Returns SDK Version string
        * @return std::string
        **********************************************************************/
        std::string getSdkVerStr()
        {
            char str[1024];
            std::string dbgStr("");
            std::string internal("");

    #ifdef _WIN32
    #ifdef _DEBUG
            dbgStr.append("-dbg");
    #endif
    #else
    #ifdef NDEBUG
            dbgStr.append("-dbg");
    #endif
    #endif

    #if defined (_WIN32) && !defined(__MINGW32__)
            sprintf_s(str, 256, "AMD-APP-SDK-v%d.%d%s%s (%d.%d)",
                sdkVerStr.major,
                sdkVerStr.minor,
                internal.c_str(),
                dbgStr.c_str(),
                sdkVerStr.build,
                sdkVerStr.revision);
    #else 
            sprintf(str, "AMD-APP-SDK-v%d.%d%s%s (%d.%d)",
                sdkVerStr.major,
                sdkVerStr.minor,
                internal.c_str(),
                dbgStr.c_str(),
                sdkVerStr.build,
                sdkVerStr.revision);
    #endif

                return std::string(str);
        }
};
#endif