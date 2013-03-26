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


#ifndef BOLTCOMMANDARGS_HPP_
#define BOLTCOMMANDARGS_HPP_

/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <ctime>
#include <string.h>

/******************************************************************************
* Defined macros                                                              *
******************************************************************************/
#define SDK_SUCCESS 0
#define SDK_FAILURE 1

#define CHECK_ALLOCATION(actual, msg) \
        if(actual == NULL) \
        { \
			std::cout << msg << std::endl; \
            std::cout << "Location : " << __FILE__ << ":" << __LINE__<< std::endl; \
            return SDK_FAILURE; \
        }


/******************************************************************************
* namespace boltsdk                                                           *
******************************************************************************/
namespace boltsdk
{

    /**************************************************************************
	* CmdArgsEnum                                                             *
	* Enum for datatype of CmdArgs                                            *
    **************************************************************************/
	enum CmdArgsEnum
	{
	  CA_ARG_INT,
	  CA_ARG_FLOAT, 
	  CA_ARG_DOUBLE, 
	  CA_ARG_STRING,
	  CA_NO_ARGUMENT
	};

    /**************************************************************************
	* Option                                                                  *
	* struct implements the fields required to map cmd line args to option    *
    **************************************************************************/
	struct Option
	{
	  std::string  _sVersion;
	  std::string  _lVersion;
	  std::string  _description;
	  CmdArgsEnum  _type;
	  void *       _value;
	};

    /**************************************************************************
	* BoltCommandArgs                                                         *
	* class implements Support for Command Line Options for any sample        *
    **************************************************************************/
	class BoltCommandArgs
	{
		private:
			int _numArgs;				/**< number of arguments */
			int _argc;					/**< number of arguments */
			int _seed;					/**< seed value */
			char ** _argv;				/**< array of char* holding CmdLine Options */
			Option * _options;			/**< struct option object */

			/**
            *******************************************************************
			* @brief Constructor private and cannot be called directly
			******************************************************************/
			BoltCommandArgs(void)
			{
				_options = NULL; 
				_numArgs = 0; 
				_argc = 0; 
				_argv = NULL;
				_seed = 123;
			}

			/**
			*******************************************************************
			* @fn match
			* @param argv array of CMDLine Options
			* @param argc number of CMdLine Options
            * @return SDK_SUCCESS on success, SDK_FAILURE otherwise
			******************************************************************/
			int match(char ** argv, int argc);
	      
		public:

			/**
			*******************************************************************
			* @brief Constructor to initialize member variables
			* @param numArgs number of arguments
			* @param options array of option objects
			******************************************************************/
			BoltCommandArgs(int numArgs, Option * options) 
			: _numArgs(numArgs), _options(options)
			{}

            /**
            *******************************************************************
            * @brief Destructor of BoltCommandArgs
            ******************************************************************/
			~BoltCommandArgs();
			
			/**
            *******************************************************************
			* @fn AddOption
			* @brief Function to add a new CmdLine Option
			* @param op Option object
            * @return SDK_SUCCESS on success, SDK_FAILURE otherwise
            ******************************************************************/
			int AddOption(Option* op);

			/**
            *******************************************************************
			* @fn DeleteOption
			* @brief Function to a delete CmdLine Option
			* @param op Option object
            * @return SDK_SUCCESS on success, SDK_FAILURE otherwise
            ******************************************************************/
			int DeleteOption(Option* op);

			/**
            *******************************************************************
			* @fn parse
			* @brief Function to parse the Cmdline Options
			* @param argv array of strings of CmdLine Options
			* @param argc Number of CmdLine Options
			* @return 0 on success Positive if expected and Non-zero on failure
            ******************************************************************/
			int parse(char ** argv, int argc);

			/**
            *******************************************************************
			* @fn isArgSet
			* @brief Function to check if a argument is set
			* @param arg option string
			* @param shortVer if the option is short version. (Opt param default=false)
			* @return true if success else false
            ******************************************************************/
			bool isArgSet(std::string arg, bool shortVer = false);

			/**
            *******************************************************************
			* @fn help
			* @brief Function to write help text
            ******************************************************************/
			void help(void);
	};
}

#endif
