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

#pragma once


/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include "AmpEngine.hpp"
#include "DX11Engine.hpp"
#include <memory>
#include <malloc.h>


/**************************************************************************
* AMPInteropDX11 
* The main class of this project
**************************************************************************/ 
class AMPInteropDX11
{    
public:
    /*
    * Constructor 
    * Initialize member variables
    */ 
    AMPInteropDX11()
        : meshWidth(WINDOW_WIDTH),
          meshHeight(WINDOW_HEIGHT)
    {
    }

    ~AMPInteropDX11(){};

    /*
    * Initialize 
    * @return NOERROR on success and E_FAIL on failure
    */ 
    int init(void);

    /*
    * Run AMPInteropDX11
    * @return NOERROR on success and E_FAIL on failure
    */ 
    int run(void);
        
private:
    unsigned int meshWidth;                  /**< mesh width */
    unsigned int meshHeight;                 /**< mesh height */
};



