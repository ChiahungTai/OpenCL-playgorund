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
#include <amp.h>
#include <amp_math.h>
#include <amp_graphics.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <iterator>
#include <vector>
#include <iostream>
#include "Definition.hpp"


using namespace concurrency;
using namespace concurrency::graphics;
using namespace concurrency::fast_math;
using namespace concurrency::direct3d;
using namespace std;
using namespace DirectX;

// Vertex Number
const unsigned int nVerticesNum = 4;

/**************************************************************************
* AMPEngine 
* Class implements C++ AMP sample
**************************************************************************/ 
class AMPEngine
{
public:
    AMPEngine(ID3D11Device* pD3D11Device) 
        : acclView(create_accelerator_view(pD3D11Device)),
          nDataNum(nVerticesNum),          
          deviceId(0)
    {
        pData = NULL;
    }
    
    /*
    * Creation factory of AMP Engine
    * @param  Pointer of D3D11 Device
    */ 
    static int ampEngineFactory(ID3D11Device **pD3D11Device);
    
    /*
    * Get instance of AMP Engine
    */ 
    static AMPEngine* getInstance();

    /*
    * Set accelerator and create array buffer,
    * also get D3D11 buffer pointer from array buffer
    * @param  address of D3D11 buffer pointer
    */ 
    int setup(ID3D11Buffer **ppVertexBuffer);
    
    /*
    * Do AMP calculation by GPU
    */ 
    void calculateDataByGPU(void);


    /*
     * verifyResults
     * Verifies the results obtained with reference implementation
     * @return NOERROR on success Positive if expected and E_FAIL on failure
     */ 
    int verifyResults(void);


private:
    
    /*
    * set default accelarator
    * @param void
    */ 
    int setDefaultAccelarator(void);

    /*
    * Create array buffer
    */ 
    int createArrayBuffer(void);

    
    /*
    * Get D3D buffer from D3D accelerator_view interface of array data .
    * @param address of a pointer,it's for output
    */ 
    int getDxBuffer( void** d3dbuffer) const;

private:
    accelerator         deviceAccl;
    accelerator_view    acclView;
    unsigned int        deviceId;
    array<XMFLOAT2, 1>  *pData;
    int                 nDataNum;
    static AMPEngine    *pAMPEngine;
};


