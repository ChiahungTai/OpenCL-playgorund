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
#ifndef AMPCOMMON_HPP_
#define AMPCOMMON_HPP_

/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <ctime>
#include <cmath>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <malloc.h>
#include <amp.h>
#include <math.h>
#include <numeric>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <linux/limits.h>
#endif

#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
#define _aligned_malloc __mingw_aligned_malloc 
#define _aligned_free  __mingw_aligned_free 
#endif 

#ifndef _WIN32
#if defined(__INTEL_COMPILER)
#pragma warning(disable : 1125)
#endif
#endif

/******************************************************************************
* Macro Definitions                                                           *
* AMP_SUCCESS                                                                 *
* AMP_FAILURE                                                                 *
* CHECK_ALLOCATION                                                            *
* CHECK_ERROR                                                                 *
* FREE                                                                        *
******************************************************************************/
#define AMP_SUCCESS 0
#define AMP_FAILURE 1

#define SDK_VERSION_MAJOR 2
#define SDK_VERSION_MINOR 8
#define SDK_VERSION_BUILD 1
#define SDK_VERSION_REVISION 1

#define CHECK_ALLOCATION(actual, msg) \
        if(actual == NULL) \
        { \
            ampsdk::AmpCommon::error(msg); \
            std::cout << "Location : " << __FILE__ << ":" << __LINE__<< std::endl; \
            return AMP_FAILURE; \
        }

#define CHECK_ERROR(actual, reference, msg) \
        if(actual != reference) \
        { \
            ampsdk::AmpCommon::error(msg); \
            std::cout << "Location : " << __FILE__ << ":" << __LINE__<< std::endl; \
            return AMP_FAILURE; \
        }

#define FREE(ptr) \
    { \
    if(ptr != NULL) \
        { \
            free(ptr); \
            ptr = NULL; \
        } \
    } 

#ifdef _WIN32
#define ALIGNED_FREE(ptr) \
    { \
        if(ptr != NULL) \
        { \
            _aligned_free(ptr); \
            ptr = NULL; \
        } \
    }
#endif

/******************************************************************************
* namespace ampsdk                                                            *
******************************************************************************/
namespace ampsdk
{
    /**************************************************************************
    * sdkVersionStr                                                           *
    * struct to form AMD APP SDK version                                      *
    **************************************************************************/
    struct sdkVersionStr
    {
        int major;        /**< SDK major release number */
        int minor;        /**< SDK minor release number */
        int build;        /**< SDK build number */
        int revision;     /**< SDK revision number */

        /**
        ***********************************************************************
        * @brief Constructor
        **********************************************************************/
        sdkVersionStr()
        {
            major = SDK_VERSION_MAJOR;
            minor = SDK_VERSION_MINOR;
            build = SDK_VERSION_BUILD;
            revision = SDK_VERSION_REVISION;
        }
    };

    /**************************************************************************
    * Timer                                                                   *
    * struct to handle time measuring functionality                           *
    **************************************************************************/
    struct Timer
    {
        std::string name;   /**< name name of time object*/
        long long _freq;    /**< _freq frequency*/
        long long _clocks;  /**< _clocks number of ticks at end*/
        long long _start;   /**< _start start point ticks*/
    };

    /**************************************************************************
    * Table                                                                   *
    * struct to create a table                                                *
    **************************************************************************/
    struct Table
    {
        int _numColumns;        /**< _numColumns  number of columns */
        int _numRows;           /**< _numRows  number of rows */
        int _columnWidth;       /**< _columnWidth  width of columns */
        std::string _delim;     /**< _delim  delimiter */
        std::string _dataItems; /**< _dataItems  string of data item */
    };

    /**************************************************************************
    * AmpCommon                                                               *
    * class implements the common steps which are done                        *
    * for most(if not all) samples                                            *
    **************************************************************************/
    class AmpCommon
    {
        private:

            std::vector<Timer*> _timers; /**< _timers vector to Timer objects */

        public:

            /**
            *******************************************************************
            * @brief Constructor
            ******************************************************************/
            AmpCommon()
            {};

            /**
            *******************************************************************
            * @brief Destructor
            ******************************************************************/
            ~AmpCommon();

            /**
            *******************************************************************
            * @fn error
            * @brief constant function, Prints error messages 
            * @param errorMsg char* message
            ******************************************************************/
            static void error(const char* errorMsg);	

            /**
            *******************************************************************
            * @fn error
            * @brief constant function, Prints error messages 
            * @param errorMsg std::string message
            ******************************************************************/
            static void error(std::string errorMsg);

            /**
            *******************************************************************
            * @brief compare template version
            * @brief compare data to check error
            * @param refData templated input
            * @param data templated input
            * @param length number of values to compare
            * @param epsilon errorWindow
            ******************************************************************/
            bool compare(const float *refData, const float *data, 
                            const int length, const float epsilon = 1e-6f); 
            bool compare(const double *refData, const double *data, 
                            const int length, const double epsilon = 1e-6f); 

            /**
            *******************************************************************
            * @fn printArray
            * @brief displays a array on std::out
            ******************************************************************/
            template<typename T> 
            void printArray(
                     const std::string header,
                     const T * data, 
                     const int width,
                     const int height) const;

            /**
            *******************************************************************
            * @fn printArray overload function
            * @brief displays a array on std::out
            ******************************************************************/
            template<typename T> 
            void printArray(
                     const std::string header,
                     const std::vector<T>& data, 
                     const int width,
                     const int height) const;

            /**
            *******************************************************************
            * @fn Timer Functions
            * @fn CreateTimer 
            * @fn resetTimer
            * @fn startTimer
            * @fn stopTimer
            * @fn readTimer
            ******************************************************************/
            int createTimer();
            int resetTimer(int handle);
            int startTimer(int handle);
            int stopTimer(int handle);
            double readTimer(int handle);

            /**
            *******************************************************************
            * @fn printTable
            * @brief displays a table of input/output 
            * @param t Table type object
            ******************************************************************/
            void printTable(Table* t);

    };
}

#endif
