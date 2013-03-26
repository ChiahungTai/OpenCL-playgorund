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


/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include <BoltSample.hpp>

/******************************************************************************
* Implementation of BoltSample::initialize()                                  *
******************************************************************************/
int BoltSample::initialize()
{
    int defaultOptions = 6;

    boltsdk::Option *optionList = new boltsdk::Option[defaultOptions];
    CHECK_ALLOCATION(optionList, "Error. Failed to allocate memory (optionList)\n");

    optionList[0]._sVersion = "q";
    optionList[0]._lVersion = "quiet";
    optionList[0]._description = "Quiet mode. Suppress most text output.";
    optionList[0]._type = boltsdk::CA_NO_ARGUMENT;
    optionList[0]._value = &quiet;

    optionList[1]._sVersion = "e";
    optionList[1]._lVersion = "verify";
    optionList[1]._description = "Verify results against reference implementation.";
    optionList[1]._type = boltsdk::CA_NO_ARGUMENT;
    optionList[1]._value = &verify;

    optionList[2]._sVersion = "t";
    optionList[2]._lVersion = "timing";
    optionList[2]._description = "Print timing related statistics.";
    optionList[2]._type = boltsdk::CA_NO_ARGUMENT;
    optionList[2]._value = &timing;

    optionList[3]._sVersion = "v";
    optionList[3]._lVersion = "version";
    optionList[3]._description = "Bolt lib & runtime version string.";
    optionList[3]._type = boltsdk::CA_NO_ARGUMENT;
    optionList[3]._value = &version;

    optionList[4]._sVersion = "x";
    optionList[4]._lVersion = "samples";
    optionList[4]._description = "Number of sample input values.";
    optionList[4]._type = boltsdk::CA_ARG_INT;
    optionList[4]._value = &samples;

    optionList[5]._sVersion = "i";
    optionList[5]._lVersion = "iterations";
    optionList[5]._description = "Number of iterations.";
    optionList[5]._type = boltsdk::CA_ARG_INT;
    optionList[5]._value = &iterations;

    sampleArgs = new boltsdk::BoltCommandArgs(defaultOptions, optionList);
    CHECK_ALLOCATION(sampleArgs, "Failed to allocate memory. (sampleArgs)\n");
                
    return SDK_SUCCESS;
}

/******************************************************************************
* Implementation of BoltSample::parseCommandLine()                            *
******************************************************************************/
int BoltSample::parseCommandLine(int argc, char**argv)
{
    if(sampleArgs==NULL)
    {
        std::cout<<"Error. Command line parser not initialized.\n";
        return SDK_FAILURE;
    }
    else
    {
        if(!sampleArgs->parse(argv,argc))
        {
            usage();
            return SDK_FAILURE;
        }
        if(sampleArgs->isArgSet("h",true) != SDK_SUCCESS)
        {
            usage();
            return SDK_FAILURE;
        }
        if(sampleArgs->isArgSet("v", true) || sampleArgs->isArgSet("version", false))
        {
            std::cout << "Bolt version : " << std::endl 
                << getBoltVerStr().c_str() << std::endl;
            exit(0);
        }
        if(samples <= 0)
        {
            std::cout << "Number input samples should be more than Zero"
                      << std::endl << "Exiting..." << std::endl;
            return SDK_FAILURE;
        }
        if(iterations <= 0)
        {
            std::cout << "Number iterations should be more than Zero"
                      << std::endl << "Exiting..." << std::endl;
            return SDK_FAILURE;
        }
    }

    return SDK_SUCCESS;
}

/******************************************************************************
* Implementation of BoltSample::usage()                                       *
******************************************************************************/
void BoltSample::usage()
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
* Implementation of BoltSample::BoltSample()                                  *
******************************************************************************/
BoltSample::BoltSample(std::string strSampleName, unsigned numSamples)
{
    sampleName = strSampleName;
    sampleArgs = NULL;
    quiet = 0;
    verify = 0;
    timing = 0;
    samples = numSamples;
    iterations = 1;
    initialize();
}

/******************************************************************************
* Implementation of BoltSample::~BoltSample()                                 *
******************************************************************************/
BoltSample::~BoltSample()
{
    delete sampleArgs;
}
