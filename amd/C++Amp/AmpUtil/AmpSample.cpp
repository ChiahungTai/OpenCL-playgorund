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
/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include <AmpCommon.h>
#include <AmpSample.h>

/******************************************************************************
* Implementation of AmpSample::initialize()                                   *
******************************************************************************/
int AmpSample::initialize()
{
    sampleCommon = new ampsdk::AmpCommon();
    int defaultOptions = 5;
   
    ampsdk::Option *optionList = new ampsdk::Option[defaultOptions];
    CHECK_ALLOCATION(optionList, "Error. Failed to allocate memory (optionList)\n");

    optionList[0]._sVersion = "q";
    optionList[0]._lVersion = "quiet";
    optionList[0]._description = "Quiet mode. Suppress all text output.";
    optionList[0]._type = ampsdk::CA_NO_ARGUMENT;
    optionList[0]._value = &quiet;

    optionList[1]._sVersion = "e";
    optionList[1]._lVersion = "verify";
    optionList[1]._description = "Verify results against reference implementation.";
    optionList[1]._type = ampsdk::CA_NO_ARGUMENT;
    optionList[1]._value = &verify;

    optionList[2]._sVersion = "t";
    optionList[2]._lVersion = "timing";
    optionList[2]._description = "Print timing.";
    optionList[2]._type = ampsdk::CA_NO_ARGUMENT;
    optionList[2]._value = &timing;

    optionList[3]._sVersion = "v";
    optionList[3]._lVersion = "version";
    optionList[3]._description = "AMD APP SDK version string.";
    optionList[3]._type = ampsdk::CA_NO_ARGUMENT;
    optionList[3]._value = &version;

    optionList[4]._sVersion = "d";
    optionList[4]._lVersion = "deviceId";
    optionList[4]._description = "Select deviceId to be used[0 to N-1 where N is number devices available].";
    optionList[4]._type = ampsdk::CA_ARG_INT;
    optionList[4]._value = &deviceId;

    sampleArgs = new ampsdk::AmpCommandArgs(defaultOptions, optionList);
    CHECK_ALLOCATION(sampleArgs, "Failed to allocate memory. (sampleArgs)\n");
                
    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of AmpSample::printStats()                                   *
******************************************************************************/
void AmpSample::printStats(std::string *statsStr, std::string * stats, int n)
{
    if(timing)
    {
        ampsdk::Table sampleStats;

        sampleStats._numColumns = n;
        sampleStats._numRows = 1;
        sampleStats._columnWidth = 35;
        sampleStats._delim = '$';
        
        sampleStats._dataItems = "";
        for(int i=0; i < n; ++i)
        {
            sampleStats._dataItems.append( statsStr[i] + "$");
        }
        sampleStats._dataItems.append("$");

        for(int i=0; i < n; ++i)
        {
            sampleStats._dataItems.append( stats[i] + "$");
        }

        sampleCommon->printTable(&sampleStats);
    }
}

/******************************************************************************
* Implementation of AmpSample::parseCommandLine()                             *
******************************************************************************/
int AmpSample::parseCommandLine(int argc, char**argv)
{
    if(sampleArgs==NULL)
    {
        std::cout<<"Error. Command line parser not initialized.\n";
        return AMP_FAILURE;
    }
    else
    {
        if(!sampleArgs->parse(argv,argc))
        {
            usage();
            return AMP_FAILURE;
        }

        if(sampleArgs->isArgSet("h",true) != AMP_SUCCESS)
        {
            usage();
            return AMP_FAILURE;
        }

        if(sampleArgs->isArgSet("v", true) 
            || sampleArgs->isArgSet("version", false))
        {
            std::cout << "SDK version : " << getSdkVerStr().c_str() 
                      << std::endl;
            exit(0);
        }
        if(sampleArgs->isArgSet("d",true) 
              || sampleArgs->isArgSet("deviceId",false))
        {
            enableDeviceId = true;

            if(validateDeviceOptions() != AMP_SUCCESS)
            {
                std::cout << "validateDeviceOptions failed.\n ";
                return AMP_FAILURE;
            }
        }
    }
    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of AmpSample::usage()                                        *
******************************************************************************/
void AmpSample::usage()
{
    if(sampleArgs==NULL)
        std::cout<<"Error. Command line parser not initialized.\n";
    else
    {
        std::cout<<"Usage\n";
        sampleArgs->help();
    }
}

/******************************************************************************
* Implementation of AmpSample::printDeviceList()                              *
******************************************************************************/
int AmpSample::printDeviceList()
{
    std::vector<accelerator> allAccl = accelerator::get_all();
    unsigned int numAccelerator = (unsigned int)allAccl.size();

    std::cout << "Available Accelerators:" << std::endl;
    for (unsigned i = 0; i < numAccelerator; ++i) 
    {
        std::wcout<<"Accelerator " <<i;
        std::wcout<< " : " << allAccl[i].get_description() << std::endl;
    }
    std::cout << "\n";

    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of AmpSample::validateDeviceOptions()                        *
******************************************************************************/
int AmpSample::validateDeviceOptions()
{
    std::vector<accelerator> allAccl = accelerator::get_all();
    unsigned int numAccelerator = (unsigned int)allAccl.size();


    if(deviceId >= numAccelerator)
    {
        if(deviceId - 1 == 0)
            std::cout << "deviceId should be 0" << std::endl;
        else
        {
            std::cout << "deviceId should be 0 to " << numAccelerator - 1 
                      << std::endl;
        }
        usage();
        return AMP_FAILURE;
    }
    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of AmpSample::setDefaultAccelerator()                        *
******************************************************************************/
int AmpSample::setDefaultAccelerator()
{
    std::vector<accelerator> allAccl = accelerator::get_all(); 
    /**************************************************************************
    * if deviceID is not set, the default accelerator is AMD Readon           *
    **************************************************************************/
    if(!enableDeviceId)
    {
        for (unsigned i = 0; i < allAccl.size(); ++i) 
        {
            if (allAccl[i].get_description().find(L"AMD Radeon") != std::wstring::npos || 
                allAccl[i].get_description().find(L"ATI Radeon") != std::wstring::npos )
            {
                deviceAccl = allAccl[i];
                break;
            }
        }
    }
    else
    {
        deviceAccl = allAccl[deviceId];
    }
    accelerator::set_default(deviceAccl.device_path);
    std::wcout << L"Selected accelerator : " << deviceAccl.get_description() 
               << std::endl;
    if (deviceAccl == accelerator(accelerator::direct3d_ref))
        std::cout << "WARNING!! Running on very slow emulator!" << std::endl;
    if(deviceAccl == accelerator(accelerator::cpu_accelerator))
    {
        std::cout << "There is no need to run on single CPU !"<<std::endl;
        return AMP_FAILURE;
    }
    return AMP_SUCCESS;
}

/******************************************************************************
* Implementation of AmpSample::AmpSample()                                    *
******************************************************************************/
AmpSample::AmpSample(std::string sampleName)
{
    name = sampleName;
    sampleCommon = NULL;
    sampleArgs = NULL;
    quiet = 0;
    verify = 0;
    timing = 0;
    deviceId = 0;
    enableDeviceId = false;
}

/******************************************************************************
* Implementation of AmpSample::~AmpSample()                                   *
******************************************************************************/
AmpSample::~AmpSample()
{
    delete sampleCommon;
    delete sampleArgs;
}