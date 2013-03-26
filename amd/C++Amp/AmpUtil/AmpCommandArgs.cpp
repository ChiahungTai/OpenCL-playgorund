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
#include "AmpCommandArgs.h"
#include <stdio.h>

/******************************************************************************
* namespace ampsdk                                                            *
******************************************************************************/
namespace ampsdk
{
    /**************************************************************************
    * Implementation of AmpCommandArgs::~AmpCommandArgs()                     *
    **************************************************************************/
    AmpCommandArgs::~AmpCommandArgs()
    {
        if(_options)
            delete[] _options;
    }

    /**************************************************************************
    * Implementation of AmpCommandArgs::match()                               *
    **************************************************************************/
    int AmpCommandArgs::match(char ** argv, int argc)
    {
        int matched  = 0;
        int shortVer = true;
        char * arg = *argv;

        if ( *arg == '-' && *(arg+1) == '-') 
        { 
            shortVer = false;
            arg++;
        }
        else if (*arg != '-') 
        {
            return matched;
        }

        arg++;  

        for (int count = 0; count < _numArgs; count++) 
        {    
            if (shortVer) 
            {
                matched = _options[count]._sVersion.compare(arg) == 0 ? 1 : 0;
            }
            else 
            {
                matched = _options[count]._lVersion.compare(arg) == 0 ? 1 : 0;
            }

            if (matched == 1) 
            {
                if (_options[count]._type == CA_NO_ARGUMENT) 
                {
                    *((bool *)_options[count]._value) = true;
                }
                else if (_options[count]._type == CA_ARG_FLOAT) 
                {
                    if(argc > 1)
                    {
                        sscanf(*(argv+1), "%f", (float *)_options[count]._value);
                        matched++; 
                    }
                    else
                    {
                        std::cout << "Error. Missing argument for \""
                                  << (*argv)<<"\"\n";
                        return AMP_FAILURE;
                    }
                }
                else if (_options[count]._type == CA_ARG_DOUBLE) 
                {
                    if(argc > 1)
                    {
                        sscanf(*(argv+1), "%lf", (double *)_options[count]._value);
                        matched++; 
                    }
                    else
                    {
                        std::cout << "Error. Missing argument for \""
                                  << (*argv) << "\"\n";
                        return AMP_FAILURE;
                    }
                }
                else if (_options[count]._type == CA_ARG_INT) 
                {
                    if(argc > 1)
                    {
                        sscanf(*(argv+1), "%d", (int *)_options[count]._value);
                        matched++; 
                    }
                    else
                    {
                        std::cout << "Error. Missing argument for \""
                                  << (*argv) << "\"\n";
                        return AMP_FAILURE;
                    }
                }
                else 
                { 
                    if(argc > 1)
                    {
                        std::string * str = (std::string *)_options[count]._value;
                        str->clear();
                        str->append(*(argv+1));
                        matched++;
                    }
                    else
                    {
                        std::cout << "Error. Missing argument for \""
                                  << (*argv) << "\"\n";
                        return AMP_FAILURE;
                    }
                }   
                break;
            }
        }
        return matched;
    }

    /**************************************************************************
    * Implementation of AmpCommandArgs::parse()                               *
    **************************************************************************/
    int  AmpCommandArgs::parse(char ** p_argv, int p_argc)
    {
        int matched = 0;
        int argc;
        char **argv;

        _argc = p_argc;
        _argv = p_argv;

        argc = p_argc;
        argv = p_argv;

        if(argc == 1)
            return AMP_FAILURE;

        while (argc > 0) 
        {
            matched = match(argv,argc);
            argc -= (matched > 0 ? matched : 1);
            argv += (matched > 0 ? matched : 1);
        }

        return matched;
    }

    /**************************************************************************
    * Implementation of AmpCommandArgs::isArgSet()                            *
    **************************************************************************/
    bool AmpCommandArgs::isArgSet(std::string str, bool shortVer)
    {
        bool enabled = false;

        for (int count = 0; count < _argc; count++) 
        {
            char * arg = _argv[count];

            if (*arg != '-') 
            {
                continue;
            }
            else if (*arg == '-' && *(arg+1) == '-' && !shortVer) 
            { 
                arg++;
            }

            arg++;  

            if (str.compare(arg) == 0) 
            {
                enabled = true;
                break;
            }
        }

        return enabled;
    } 
  
    /**************************************************************************
    * Implementation of AmpCommandArgs::AddOption()                           *
    **************************************************************************/
    int AmpCommandArgs::AddOption(Option* op)
    {
        if(!op)
        {
            std::cout<<"Error. Cannot add option, NULL input";
            return -1;
        }
        else
        {
            Option* save;
            if(_options != NULL)
                save = _options;
            _options = new Option[_numArgs + 1];
            if(!_options)
            {
                std::cout<<"Error. Cannot add option ";
                std::cout<<op->_sVersion<<". Memory allocation error\n";
                return AMP_FAILURE;
            }
            if(_options != NULL)
            {
                for(int i=0; i< _numArgs; ++i)
                {
                    _options[i] = save[i];
                }
            }
            _options[_numArgs] = *op;
            _numArgs++;
            delete []save;
        }

        return AMP_SUCCESS;
    }

    /**************************************************************************
    * Implementation of AmpCommandArgs::DeleteOption()                        *
    **************************************************************************/
    int AmpCommandArgs::DeleteOption(Option* op)
    {
        if(!op || _numArgs <= 0)
        {
            std::cout<<"Error. Cannot delete option." 
                <<"Null pointer or empty option list\n";
            return AMP_FAILURE;
        }

        for(int i = 0; i < _numArgs; i++)
        {
            if(op->_sVersion == _options[i]._sVersion || 
                op->_lVersion == _options[i]._lVersion )
            {
                for(int j = i; j < _numArgs-1; j++)
                {
                    _options[j] = _options[j+1];
                }
                _numArgs--;
            }
        }

        return AMP_SUCCESS;
    }

    /**************************************************************************
    * Implementation of AmpCommandArgs::help()                                *
    **************************************************************************/
    void AmpCommandArgs::help(void)
    {
        std::string result = "-h, --help\tDisplay this information\n";

        for (int count = 0; count < _numArgs; count++) 
        {
            std::string line;

            if (_options[count]._sVersion.length() > 0) 
            {
                line = "-" + _options[count]._sVersion + ", ";
            }

            line += "--" + _options[count]._lVersion
            + "\t" 
            + _options[count]._description + "\n";

            result += line;
        }

        std::cout<< result;
    }
}
