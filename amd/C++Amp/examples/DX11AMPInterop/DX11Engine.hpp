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

#include <Windows.h>
#include <d3d11.h>
//#include <D3Dcompiler.h>
#include <time.h>
#include <memory>
#include <malloc.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string.h>
#include <string>
#include "Definition.hpp"

#include <ppltasks.h>

/**************************************************************************
* DX11Engine 
* Class implements DX11
**************************************************************************/ 
class DX11Engine
{    
public:
    /*
    * Constructor 
    * Initialize member variables
    */ 
    explicit DX11Engine()
        : meshWidth(WINDOW_WIDTH),
          meshHeight(WINDOW_HEIGHT),
          featureLevel(D3D_FEATURE_LEVEL_11_0)
    {
        pD3D11Device         = NULL;
        pD3DContext          = NULL; 
        pSwapChain           = NULL;
        pRenderTargetView    = NULL;
        pVertexBuffer        = NULL;
        pVertexPosBuffer     = NULL;
        pInputLayout         = NULL;
        pVsBuffer            = NULL;
        pPsBuffer            = NULL;
        pSolidColorVS        = NULL;
        pSolidColorPS        = NULL;     
        pColorMap            = NULL;
    }

    ~DX11Engine();
    
    /*
    * Creation factory of DX11 Engine
    */ 
    static int DX11EngineFactory();

    /*
    * Get instance of DX11 Engine
    */ 
    static DX11Engine* getInstance();


    /*
    * Initialize DX11 device and object
    * @interopDevice 
    * @return NOERROR on success and E_FAIL on failure
    */ 
    int initializeDX11(void);
    
    /*
    * Create  shader resource view from AMP array buffer
    */ 
    int createSRVFromAMPBuffer(void);

    /**
     * Renders the sineware
     * @return NOERROR on success and E_FAIL on failure
     */
    int render(void);

    /**
    * Run DX11Engine
    * @return NOERROR on success and E_FAIL on failure
    */
    int run(void);
    
    /**
    * Cleanup DX11 resources
    */
    void DX11Cleanup();
    
    /**
    * unregister window class when exit.
    */
    void exitWindow(void);

    /**
    * Get vertex position buffer 
    */
    ID3D11Buffer **getVertexPosBuffer() {return &pVertexPosBuffer;}
    
    /**
    * Get DX11 device instance
    */
    ID3D11Device **getD3D11Device() {return &pD3D11Device;}    
    
private:
    unsigned int meshWidth;                  /**< mesh width */
    unsigned int meshHeight;                 /**< mesh height */
        
    /*
    * D3D11 vars
    */ 
    D3D_FEATURE_LEVEL               featureLevel;
    ID3D11Device                    *pD3D11Device;
    ID3D11DeviceContext             *pD3DContext; 
    IDXGISwapChain                  *pSwapChain;
    ID3D11RenderTargetView          *pRenderTargetView;
    ID3D11Buffer                    *pVertexBuffer;
    ID3D11Buffer                    *pVertexPosBuffer;
    ID3D11InputLayout               *pInputLayout;
    ID3DBlob                        *pVsBuffer;
    ID3DBlob                        *pPsBuffer;
    ID3D11VertexShader              *pSolidColorVS;
    ID3D11PixelShader               *pSolidColorPS;     
    ID3D11ShaderResourceView        *pColorMap;

    // Define a callback function pointer
    static DX11Engine*              pDX11Engine;
};



