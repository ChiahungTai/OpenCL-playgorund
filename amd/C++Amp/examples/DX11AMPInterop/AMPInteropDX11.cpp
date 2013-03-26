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

/**
*******************************************************************************
* @file <AMPInteropDX11.cpp>
* 
* @brief demonstrates the usage of the the DX11 - C++ AMP interop 
******************************************************************************/


/******************************************************************************
* Included header files                                                       *
******************************************************************************/
#include "AMPInteropDX11.hpp"


/**
*******************************************************************************
* @fn init
* @brief Initialize DX and AMP enovironment
*
* @return NOERROR on success and E_FAIL on failure
******************************************************************************/
int AMPInteropDX11::init()
{
    RETURN_IF_FAIL(DX11Engine::DX11EngineFactory());
    RETURN_IF_FAIL(DX11Engine::getInstance()->initializeDX11());
    
    RETURN_IF_FAIL(AMPEngine::ampEngineFactory(DX11Engine::getInstance()->getD3D11Device()));    
    RETURN_IF_FAIL(AMPEngine::getInstance()->setup(DX11Engine::getInstance()->getVertexPosBuffer()));
    RETURN_IF_FAIL(DX11Engine::getInstance()->createSRVFromAMPBuffer());
    
    return NOERROR;
}

/**
*******************************************************************************
* @fn Run
* @brief verify AMP result first and render window 
*
* @return NOERROR on success and E_FAIL on failure
******************************************************************************/
int AMPInteropDX11::run()
{
    // warm up and verify
    RETURN_IF_FAIL(AMPEngine::getInstance()->verifyResults());
    RETURN_IF_FAIL(DX11Engine::getInstance()->run());

    return NOERROR;
}

/********************************************************************************
* Execution of program begins from here                                         *
********************************************************************************/
int main()
{
    bool isVistaOrWin7 = false;

    std::cout << "**********************************************" << std::endl;
    std::cout << "C++ AMP DirectX 11 Interop" << std::endl;
    std::cout << "**********************************************" << std::endl;

    // Find the version of Windows
    OSVERSIONINFO vInfo;
    memset(&vInfo, 0, sizeof(vInfo));
    vInfo.dwOSVersionInfoSize = sizeof(vInfo);
    if(!GetVersionEx(&vInfo))
    {
        std::cout << "\nERROR : Unable to get Windows version information.\n" << std::endl;
        return E_FAIL;
    }

    if(vInfo.dwMajorVersion >= 6)
        isVistaOrWin7 = true;

    if(!isVistaOrWin7)
    {
        std::cout << "Unsupported OS!\n";
        return NOERROR;
    }

    AMPInteropDX11 ampSimpleDX11;
    RETURN_IF_FAIL(ampSimpleDX11.init());
    RETURN_IF_FAIL(ampSimpleDX11.run());

    DX11Engine::getInstance()->exitWindow();
    return NOERROR;
}

