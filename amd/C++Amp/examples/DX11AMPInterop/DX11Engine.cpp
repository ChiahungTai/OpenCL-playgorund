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
#include "DX11Engine.hpp"
#include "AmpEngine.hpp"

#include "BasicReader.hpp"

extern const unsigned int nVerticesNum;

HWND wndHandle;
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDCLASSEX wc =
{
    sizeof(WNDCLASSEX),
    CS_CLASSDC,
    MsgProc,
    0L,
    0L,
    GetModuleHandle(NULL),
    NULL,
    NULL,
    NULL,
    NULL,
    reinterpret_cast<LPCSTR>(WINDOW_CLASS_NAME),
    NULL
};


long t1, t2;
int frameCount            = 0;
int frameRefCount         = 90;
double totalElapsedTime   = 0.0;

// Clear render target and the depth stencil 
float ClearColor[]        = { 0.0f, 0.15f, 0.0f, 0.0f };


/**
*******************************************************************************
* @fn MsgProc
* @brief The window's message handler
*
* @return LRESULT
******************************************************************************/
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_DESTROY:
            return 0;

        case WM_KEYDOWN:
            switch(wParam)
            {
                case 'q':
                case 'Q':
                case VK_ESCAPE:
                    DX11Engine::getInstance()->DX11Cleanup();
                    PostQuitMessage(0);
                    return 0;
            }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}


DX11Engine* DX11Engine::pDX11Engine = NULL;

DX11Engine::~DX11Engine()
{
    // release program resources
    DX11Cleanup();
}

/**
*******************************************************************************
* @fn getInstance
* @brief Get instance of DX11 Engine
*
* @return Pointer to DX11Engine obj
******************************************************************************/
DX11Engine* DX11Engine::getInstance()
{
    return pDX11Engine;
}

/**
*******************************************************************************
* @fn DX11EngineFactory
* @brief Creation factory of DX11 Engine
*
* @return NOERROR on success Positive if expected and E_FAIL on failure
******************************************************************************/
int DX11Engine::DX11EngineFactory()
{    
    if(NULL == pDX11Engine)
    {
        pDX11Engine = new DX11Engine( );
        if(NULL == pDX11Engine)
        {
            return E_FAIL;
        }
    }

    return NOERROR;
}

/**
*******************************************************************************
* @fn initializeDX11
* @brief Initialize DX11 device and object
*
* @return NOERROR on success Positive if expected and E_FAIL on failure
******************************************************************************/
int DX11Engine::initializeDX11(void)
{
    int hrStatus = S_OK;
    // Get all adapters
    std::vector<IDXGIAdapter*> vAdapters;
    unsigned int numAdapters;
    IDXGIFactory* factory;
    CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
    IDXGIAdapter * pAdapter = 0;
    UINT i = 0;
    while(factory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND) 
    { 
        vAdapters.push_back(pAdapter); 
        ++i; 
    }

    // Push NULL to vAdapters and assign zero to deviceId if adapter is not found
    if(i == 0)
    {
        vAdapters.push_back(NULL);
    }

    numAdapters = i;

    DISPLAY_DEVICE dispDevice;
    DWORD deviceNum;
    dispDevice.cb = sizeof(DISPLAY_DEVICE);

    int xCoordinate = 0;
    int yCoordinate = 0;

    for (deviceNum = 0; EnumDisplayDevices(NULL, deviceNum, &dispDevice, 0); deviceNum++) 
    {
        if (dispDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) 
        {
                continue;
        }

        if(!(dispDevice.StateFlags & DISPLAY_DEVICE_ACTIVE))
        {
            continue;
        }

        DEVMODE deviceMode;

        // initialize the DEVMODE structure
        ZeroMemory(&deviceMode, sizeof(deviceMode));
        deviceMode.dmSize = sizeof(deviceMode);
        deviceMode.dmDriverExtra = 0;

        EnumDisplaySettingsEx(dispDevice.DeviceName, ENUM_CURRENT_SETTINGS, &deviceMode, EDS_ROTATEDMODE);

        xCoordinate = deviceMode.dmPosition.x;
        yCoordinate = deviceMode.dmPosition.y;

        // Register the window class
        RegisterClassEx(&wc);

        // Create the application's window
        wndHandle = CreateWindow(reinterpret_cast<LPCSTR>(WINDOW_CLASS_NAME), 
                              reinterpret_cast<LPCSTR>(CAPTION_NAME), 
                              WS_CAPTION | WS_POPUPWINDOW, 
                              false, 
                              yCoordinate, 
                              WINDOW_WIDTH, 
                              WINDOW_HEIGHT, 
                              NULL, 
                              NULL, 
                              wc.hInstance, 
                              NULL);
        
        D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0
        };
        unsigned int totalFeatureLevels = ARRAYSIZE(featureLevels);

        // Create a D3D Swap Chain and Device
        DXGI_SWAP_CHAIN_DESC swapChainDesc;
        ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
        swapChainDesc.BufferCount       = 1;
        swapChainDesc.BufferDesc.Width  = meshWidth;
        swapChainDesc.BufferDesc.Height = meshHeight;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferDesc.RefreshRate.Numerator   = 60;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow       = wndHandle;
        swapChainDesc.Windowed           = true;
        swapChainDesc.SampleDesc.Count   = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        
        unsigned int creationFlags = 0;

#ifdef _DEBUG
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        hrStatus =  D3D11CreateDeviceAndSwapChain(0,
                                                D3D_DRIVER_TYPE_HARDWARE, 
                                                NULL,
                                                creationFlags,
                                                featureLevels, 
                                                totalFeatureLevels,
                                                D3D11_SDK_VERSION, 
                                                &swapChainDesc,
                                                &pSwapChain,
                                                &pD3D11Device,
                                                &featureLevel, 
                                                &pD3DContext
                                               );

        if(hrStatus != S_OK)
        {
            std::cout << "\nERROR : Failed to create D3D device" << std::endl;
            return E_FAIL;
        }                
    }
        
    // Create a render target view
    ID3D11Texture2D *pBackBuffer;
    hrStatus = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if(FAILED(hrStatus))
    {
        std::cout << "GetBuffer failed." << std::endl;
        return E_FAIL;
    }

    hrStatus = pD3D11Device->CreateRenderTargetView(pBackBuffer, NULL, &pRenderTargetView);
    pBackBuffer->Release();
    if(FAILED(hrStatus))
    {
        std::cout << "CreateRenderTargetView() failed" << std::endl;
        return E_FAIL;
    }


    pD3DContext->OMSetRenderTargets(1, &pRenderTargetView, NULL);

    // Create the viewport
    D3D11_VIEWPORT vp;
    vp.Width = static_cast<float>(meshWidth);
    vp.Height = static_cast<float>(meshHeight);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    pD3DContext->RSSetViewports(1, &vp);

	// Read vertex shader resource
	BasicReader Reader;
	auto vertexShaderByteCode = Reader.readBinaryFromFile("SimpleVertexShader.cso");

	hrStatus = pD3D11Device->CreateVertexShader(
		(void *)vertexShaderByteCode.data(),
	    (SIZE_T)vertexShaderByteCode.size(),
        nullptr,
        &pSolidColorVS
       );
    if(FAILED(hrStatus))
    {
        std::cout << "Create Vertex Shader failed" << std::endl;

        return E_FAIL;
    }
    
    //Create and set IAInputLayout
    D3D11_INPUT_ELEMENT_DESC vertexLayout[] = 
    {
        {
            "pos",
            0,
            DXGI_FORMAT_R32G32_FLOAT,
            0,
            0,
            D3D11_INPUT_PER_VERTEX_DATA,
            0
        }
    };

    hrStatus = pD3D11Device->CreateInputLayout(
        vertexLayout,
        sizeof(vertexLayout) / sizeof(D3D11_INPUT_ELEMENT_DESC),
		vertexShaderByteCode.data(), 
		vertexShaderByteCode.size(), 
        &pInputLayout 
       );
    if(FAILED(hrStatus))
    {
        std::cout << "Create Input Layout failed" << std::endl;
        return E_FAIL;
    }


    pD3DContext->IASetInputLayout(pInputLayout);

	// Read pixel shader resource
	auto pixelShaderBytecode = Reader.readBinaryFromFile("SimplePixelShader.cso");

	hrStatus = pD3D11Device->CreatePixelShader(
		pixelShaderBytecode.data(), 
		pixelShaderBytecode.size(), 
        NULL, 
        &pSolidColorPS
       );
    if(FAILED(hrStatus))
    {
        std::cout << "Create Pixel Shader failed" << std::endl;
        return E_FAIL;
    }
	        
    // Create Vertex buffer object
    D3D11_BUFFER_DESC bufDesc;
    bufDesc.ByteWidth        = nVerticesNum * sizeof(XMFLOAT2);
    bufDesc.CPUAccessFlags   = 0;
    bufDesc.Usage            = D3D11_USAGE_DEFAULT;
    bufDesc.MiscFlags        = D3D11_RESOURCE_MISC_SHARED;
    bufDesc.BindFlags        = D3D11_BIND_VERTEX_BUFFER;
    hrStatus = pD3D11Device->CreateBuffer(&bufDesc, NULL, &pVertexBuffer);
    if(FAILED(hrStatus))
    {
        std::cout << "CreateBuffer failed. (pVertexBuffer)" << std::endl;
        return E_FAIL;
    }
    
    // Set vertex bufer
    if(pVertexBuffer)
    {
        const UINT strides[] = {sizeof(XMFLOAT2)};
        const UINT offsets[] = {0};
        pD3DContext->IASetVertexBuffers(0, 1, &pVertexBuffer, strides, offsets);
    }

    // Set primitive topology
    pD3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    return NOERROR;
}

/**
*******************************************************************************
* @fn render
* @brief Draws the scene
*
* @return NOERROR on success and E_FAIL on failure
******************************************************************************/
int DX11Engine::render()
{
    t1 = clock() * CLOCKS_PER_SEC;
    frameCount++;

    // run AMP kernel
    AMPEngine::getInstance()->calculateDataByGPU();

    if(NULL == pD3DContext)
    {
        return E_FAIL;
    }
    
    ID3D11ShaderResourceView* aRViews[ 1 ] = { pColorMap };
    pD3DContext->VSSetShaderResources(0, 1, aRViews);

    pD3DContext->ClearRenderTargetView(pRenderTargetView, ClearColor);

    pD3DContext->VSSetShader(pSolidColorVS, 0, 0);
    pD3DContext->PSSetShader(pSolidColorPS, 0, 0);
    pD3DContext->Draw(nVerticesNum, 0);
    
    ID3D11ShaderResourceView* ppSRVNULL[1] = { NULL };
    pD3DContext->VSSetShaderResources(0, 1, ppSRVNULL);

    // Present the information rendered to the back buffer to the front buffer (the screen)
    pSwapChain->Present(0, 0);

    t2 = clock() * CLOCKS_PER_SEC;
    totalElapsedTime += (double)(t2 - t1);
    if(frameCount && (frameCount > frameRefCount))
    {
        double fMs = (double)((totalElapsedTime / (double)CLOCKS_PER_SEC) / (double) frameCount);
        int framesPerSec = (int)(1.0 / (fMs / CLOCKS_PER_SEC));
        char str[128];
        sprintf_s(str, 128, "C++AMPinteropDX11 | %d FPS", framesPerSec);
        SetWindowText(wndHandle, (str));
        frameCount = 0;
        totalElapsedTime = 0.0;
    }
    return NOERROR;
}

/**
*******************************************************************************
* @fn run
*
* @brief    Run AMP kernel and render
*
* @return NOERROR on success and E_FAIL on failure
******************************************************************************/
int DX11Engine::run()
{
    // Show the window
    ShowWindow(wndHandle, SW_SHOWDEFAULT);
    UpdateWindow(wndHandle);

    // Enter the message loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    while(msg.message != WM_QUIT)
    {
        if(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            if(msg.message == WM_NCLBUTTONDOWN)
            {
                break;
            }
            render();
        }
    }

    return NOERROR;
}


/**
*******************************************************************************
* @fn DX11Cleanup
* @brief release DirectX resources
*
* @return void
******************************************************************************/
void DX11Engine::DX11Cleanup()
{
    SAFE_RELEASE(pColorMap);
    SAFE_RELEASE(pSolidColorVS);
    SAFE_RELEASE(pSolidColorPS);
    SAFE_RELEASE(pInputLayout);
    SAFE_RELEASE(pD3DContext);
    SAFE_RELEASE(pVertexBuffer);
    SAFE_RELEASE(pVertexPosBuffer);
    SAFE_RELEASE(pRenderTargetView);
    SAFE_RELEASE(pSwapChain);
    SAFE_RELEASE(pD3D11Device);
}

/**
*******************************************************************************
* @fn DX11Cleanup
* @brief Create shader resource view from AMP array buffer
*
* @return void
******************************************************************************/
int DX11Engine::createSRVFromAMPBuffer()
{    
    // Bind a resource view to the CS buffer
    D3D11_BUFFER_DESC descBuf;
    ZeroMemory(&descBuf, sizeof(descBuf));
    if(NULL != pVertexPosBuffer)
    {
        pVertexPosBuffer->GetDesc(&descBuf);
    }
    else
    {
        return E_FAIL;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC DescRV;
    ZeroMemory(&DescRV, sizeof(DescRV));
    DescRV.Format = DXGI_FORMAT_R32_TYPELESS;
    DescRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    DescRV.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
    DescRV.Buffer.FirstElement = 0;
    DescRV.Buffer.NumElements = descBuf.ByteWidth / sizeof(int); 
    RETURN_IF_FAIL(pD3D11Device->CreateShaderResourceView(pVertexPosBuffer, &DescRV, &pColorMap));

    return NOERROR;
}

/**
*******************************************************************************
* @fn exitWindow
* @brief unregister window class when exit.
*
* @return void
******************************************************************************/
void DX11Engine::exitWindow(void)
{
    UnregisterClass(reinterpret_cast<LPCSTR>(WINDOW_CLASS_NAME), wc.hInstance);
}