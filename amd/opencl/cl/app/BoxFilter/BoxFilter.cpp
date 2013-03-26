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


#include "BoxFilterSAT.hpp"
#include "BoxFilterSeparable.hpp"
#include <cmath>

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

int 
main(int argc, char * argv[])
{

    BoxFilterSAT clBoxFilterSAT("OpenCL BoxFilterSAT");
    BoxFilterSeparable clBoxFilterSeparable("OpenCL BoxFilterSeparable");

    std::cout << "Running SAT version.. " << std::endl;
    
    if(clBoxFilterSAT.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;
    
    if(clBoxFilterSAT.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    if(clBoxFilterSAT.isDumpBinaryEnabled())
    {
        return clBoxFilterSAT.genBinaryImage();
    }
    else
    {
        if(clBoxFilterSAT.setup() != SDK_SUCCESS)
            return SDK_FAILURE;

        if(clBoxFilterSAT.run() != SDK_SUCCESS)
            return SDK_FAILURE;

        if(clBoxFilterSAT.verifyResults() != SDK_SUCCESS)
            return SDK_FAILURE;

        if(clBoxFilterSAT.cleanup() != SDK_SUCCESS)
            return SDK_FAILURE;
       
        clBoxFilterSAT.printStats();
    }

    std::cout << std::endl << std::endl;
    std::cout << "Running Separable version.. " << std::endl;

    if(clBoxFilterSeparable.initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    if(clBoxFilterSeparable.parseCommandLine(argc, argv))
        return SDK_FAILURE;

    if(clBoxFilterSeparable.isDumpBinaryEnabled())
    {
        return clBoxFilterSeparable.genBinaryImage();
    }
    else
    {
        if(clBoxFilterSeparable.setup() != SDK_SUCCESS)
            return SDK_FAILURE;

        if(clBoxFilterSeparable.run() != SDK_SUCCESS)
            return SDK_FAILURE;

        if(clBoxFilterSeparable.verifyResults() != SDK_SUCCESS)
            return SDK_FAILURE;

        if(clBoxFilterSeparable.cleanup() != SDK_SUCCESS)
            return SDK_FAILURE;

        clBoxFilterSeparable.printStats();
    }

    return SDK_SUCCESS;
}



