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

#include "BasicReader.hpp"
#include <windows.h>


#define MAX_PATH       260

std::string getPath()
{
#ifdef _WIN32
    char buffer[MAX_PATH];
#ifdef UNICODE
    if(!GetModuleFileName(NULL, (LPWCH)buffer, sizeof(buffer)))
        throw std::string("GetModuleFileName() failed!");
#else
    if(!GetModuleFileName(NULL, buffer, sizeof(buffer)))
        throw std::string("GetModuleFileName() failed!");
#endif
    std::string str(buffer);
    /* '\' == 92 */
    int last = (int)str.find_last_of((char)92);
#else
    char buffer[PATH_MAX + 1];
    ssize_t len;
    if((len = readlink("/proc/self/exe",buffer, sizeof(buffer) - 1)) == -1)
        throw std::string("readlink() failed!");
    buffer[len] = '\0';
    std::string str(buffer);
    /* '/' == 47 */
    int last = (int)str.find_last_of((char)47);
#endif
    return str.substr(0, last + 1);
}


std::vector<unsigned char> BasicReader::readBinaryFromFile(const char* fileName)
{
	std::string path = getPath();
	path.append(fileName);
    FILE * input = NULL;
    size_t size = 0;
    unsigned char* binary = NULL;

	if((fopen_s(&input, path.c_str(), "rb"))!= 0)
    {
		exit(1);
    }

    fseek(input, 0L, SEEK_END); 
    size = ftell(input);
    rewind(input);
    binary = (unsigned char*)malloc(size);
    if(binary == NULL)
    {
        exit(1);
    }
    fread(binary, sizeof(unsigned char), size, input);
    fclose(input);
	source_ = std::vector<unsigned char>(size);
	for(size_t i = 0; i < size; i++)
		source_[i] = binary[i];
    free(binary);

    return source_;
}

