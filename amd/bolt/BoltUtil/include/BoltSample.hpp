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


#ifndef BOLTSAMPLE_H_
#define BOLTSAMPLE_H_

/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include <BoltCommandArgs.hpp>
#include <bolt/boltVersion.h>
#include <bolt/cl/bolt.h>
#include "bolt/statisticalTimer.h"


/******************************************************************************
* namespace boltsdk                                                           *
******************************************************************************/
namespace boltsdk
{

/******************************************************************************
* BoltVersionStr                                                              *
* @brief struct definition that contains Bolt library version information     *
******************************************************************************/
struct BoltVersionStr
{
	int major;		/**< Bolt major release number */
	int minor;		/**< Bolt minor release number */
	int patch;		/**< Bolt build number */

	/**
    ***************************************************************************
	* @brief Constructor of BoltVersionStr. These version numbers come
    *        directly from the Bolt header files, and represent the version
    *        of header that the app is compiled against 
    **************************************************************************/
	BoltVersionStr()
	{
		major = BoltVersionMajor;
        minor = BoltVersionMinor;
        patch = BoltVersionPatch;
	}
};


/******************************************************************************
* BoltTimer                                                                   *
* Class implements an interface to use the underlying timer in Bolt library   *
******************************************************************************/
class BoltTimer
{
    bolt::statTimer& boltTimer;
    size_t timerId;
    
public:
    /** 
    ***************************************************************************
    * @brief Constructor of boltTimer to initialize member variables
    **************************************************************************/
    BoltTimer(): boltTimer(bolt::statTimer::getInstance())
    {
        //boltTimer = bolt::statTimer::getInstance( );
        timerId = -1;
    }

    /**
    ***************************************************************************
    * @brief Destructor of BoltTimer
    **************************************************************************/
    ~BoltTimer()
    {}

    /**
    ***************************************************************************
    * @fn setup
    * @brief Generate a unique Bolt timer id & reserve memory for iterations 
    * @return SDK_SUCCESS on success, SDK_FAILURE otherwise
    **************************************************************************/
    void setup(const std::string& strSampleName, unsigned iterations)
    {
        bolt::tstring tmp(strSampleName.length(), L' ');
        std::copy(strSampleName.begin(), strSampleName.end(), tmp.begin());

        boltTimer.Reserve(1, iterations);
        timerId	= boltTimer.getUniqueID(tmp, 0);
    }

    /**
    ***************************************************************************
    * @fn startTimer
    * @brief start the timer 
    * @return SDK_SUCCESS on success, SDK_FAILURE otherwise
    **************************************************************************/
   int startTimer()
    {
        if (timerId == -1)
            return SDK_FAILURE;
        boltTimer.Start(timerId);
        return SDK_SUCCESS;
    }

    /**
    ***************************************************************************
    * @fn stopTimer
    * @brief stop the timer 
    * @return SDK_SUCCESS on success, SDK_FAILURE otherwise
    **************************************************************************/
    int stopTimer()
    {
        if (timerId == -1)
            return SDK_FAILURE;
        boltTimer.Stop(timerId);
        return SDK_SUCCESS;
    }

    /**
    ***************************************************************************
    * @fn getAverageTime
    * @brief return the average execution time over a pruned set of iterations
    * @return double average execution time in seconds
    **************************************************************************/
    double getAverageTime()
    {
        size_t pruned = boltTimer.pruneOutliers(1.0);
        return (boltTimer.getAverageTime(timerId));
    }
};

}


/******************************************************************************
* BoltSample                                                                  *
* Class implements various resources required by the test & initializes the   *
* resources used by tests                                                     *
******************************************************************************/
class BoltSample
{
protected:
    boltsdk::BoltCommandArgs *sampleArgs;   /**< BoltCommandArgs class object to handle comd line options */
    boltsdk::BoltVersionStr boltVerStr;     /**< Bolt version string */
    boltsdk::BoltTimer sampleTimer;         /**< Bolt timer object */
    bool version;                           /**< Cmd Line Option- if version */
    std::string sampleName;                 /**< Name of the Sample */
    double totalTime;                       /**< Total Time taken by the Sample */
    bool quiet;                             /**< Cmd Line Option- if Quiet */
    bool verify;                            /**< Cmd Line Option- if verify */
    bool timing;                            /**< Cmd Line Option- if Timing */
    int samples;                            /**< Cmd Line Option- num samples */
    int iterations;                         /**< Cmd Line Option- iterations */
    
    /**
    ***************************************************************************
    * @fn initialize
    * @brief Initialize the resources used by tests
    * @return SDK_SUCCESS on success, SDK_FAILURE otherwise
    **************************************************************************/
    int initialize();

public:
    /**
    ***************************************************************************
    * @brief Constructor of BoltSample to initialize member variables
    * @param strSampleName Name of the Sample
    * @param numSamples Number of sample input values
    **************************************************************************/
    BoltSample(std::string strSampleName, unsigned numSamples);
    
    /**
    ***************************************************************************
    * @brief Destructor of BoltSample
    **************************************************************************/
    ~BoltSample();

    /**
    ***************************************************************************
    * @fn parseCommandLine
    * @brief parses the command line options given by user
    * @param argc Number of elements in cmd line input
    * @param argv Array of char* storing the CmdLine Options
    * @return SDK_SUCCESS on success, SDK_FAILURE otherwise
    **************************************************************************/
    int parseCommandLine(int argc, char **argv);
    
    /**
    ***************************************************************************
    * @fn usage
    * @brief Displays the various options available for any sample
    * @return void
    **************************************************************************/
    void usage();

    /**
    ***************************************************************************
    * @fn getBoltVerStr
    * @brief Returns Bolt Version string
    * @return std::string containing Bolt lib & runtime version
    **************************************************************************/
    std::string getBoltVerStr()
    {
        char str[1024];
        unsigned libMajor = 0, libMinor = 0, libPatch = 0;

        bolt::cl::getVersion( libMajor, libMinor, libPatch );

#if defined (_WIN32) && !defined(__MINGW32__)
        sprintf_s(str, 256, "Application compiled with Bolt: v%d.%d.%d\nBolt library compiled with Bolt: v%d.%d.%d",
            boltVerStr.major,
            boltVerStr.minor,
            boltVerStr.patch,
            libMajor,
            libMinor,
            libPatch);
#else 
        sprintf(str, "Application compiled with Bolt: v%d.%d.%d\nBolt library compiled with Bolt: v%d.%d.%d",
            boltVerStr.major,
            boltVerStr.minor,
            boltVerStr.patch,
            libMajor,
            libMinor,
            libPatch);
#endif

        return std::string(str);
    }

};

#endif


