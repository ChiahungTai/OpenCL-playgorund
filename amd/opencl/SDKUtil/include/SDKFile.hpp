/**********************************************************************
Copyright ?012 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

?Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
?Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/
#ifndef SDKFILE_HPP_
#define SDKFILE_HPP_

/**
 * Header Files
 */
#include <vector>
#include <string>
#include <fstream>
#include <malloc.h>
#include <SDKCommon.hpp>

/**
 * namespace streamsdk
 */
namespace streamsdk
{
/**
 * getCurrentDir
 * Get current directory
 * @return string
 */
extern std::string getCurrentDir();

/**
 * class SDKFile
 * for the opencl program file processing
 */
class SDKFile
{
public:
    /**
	 *Default constructor
	 */
    SDKFile(): source_(""){}

    /**
	 * Destructor
	 */
    ~SDKFile(){};

    /**
	 * Opens the CL program file
	 * @return true if success else false
	 */
    bool open(const char* fileName);

	/**
	 * writeBinaryToFile
	 * @param fileName Name of the file
	 * @param binary char binary array
	 * @param numBytes number of bytes
	 * @return true if success else false
	 */
    int writeBinaryToFile(const char* fileName, const char* binary, size_t numBytes);

	/**
	 * readBinaryToFile
	 * @param fileName name of file
	 * @return true if success else false
	 */
    int readBinaryFromFile(const char* fileName);

    /**
	 * Replaces Newline with spaces
	 */
    void replaceNewlineWithSpaces()
    {
        size_t pos = source_.find_first_of('\n', 0);
        while(pos != -1)
        {
            source_.replace(pos, 1, " ");
            pos = source_.find_first_of('\n', pos + 1);
        }
        pos = source_.find_first_of('\r', 0);
        while(pos != -1)
        {
            source_.replace(pos, 1, " ");
            pos = source_.find_first_of('\r', pos + 1);
        }
    }

	/**
	 * source
	 * Returns a pointer to the string object with the source code
	 */
    const std::string&  source() const { return source_; }

private:
    /**
	 * Disable copy constructor
	 */
    SDKFile(const SDKFile&);

    /**
	 * Disable operator=
	 */
    SDKFile& operator=(const SDKFile&);

    std::string     source_;    //!< source code of the CL program
};

} // namespace streamsdk

#endif  // SDKFile_HPP_
