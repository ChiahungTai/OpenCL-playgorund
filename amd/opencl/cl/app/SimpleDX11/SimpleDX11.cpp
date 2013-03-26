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

/**
*******************************************************************************
* @file <SimpleDX11.cpp>
* 
* @brief demonstrates the usage of the the DX11 - OCL interop 
******************************************************************************/

#include "SimpleDX11.hpp"
#include "BasicReader.hpp"
#include <cmath>
#include <malloc.h>


/**************************************************************************
* D3D11 global vars
**************************************************************************/ 
ID3D11Device                    *pD3D11Device         = NULL;
IDXGISwapChain                  *pSwapChain           = NULL;
ID3D11RenderTargetView          *pRenderTargetView    = NULL;
ID3D11Buffer                    *pVetexBufPtr         = NULL;
ID3D11Buffer                    *pVetexStagingBuf     = NULL;
ID3D11InputLayout               *pInputLayout         = NULL;

ID3D11VertexShader              *pSolidColorVS        = NULL;
ID3D11PixelShader               *pSolidColorPS        = NULL;
ID3D11DeviceContext             *pD3DContext          = NULL;      
D3D_FEATURE_LEVEL               featureLevel          = D3D_FEATURE_LEVEL_11_0;
ID3D11ShaderResourceView        *pColorMap            = NULL;
ID3D11SamplerState              *pColorMapSampler     = NULL;
ID3D11Texture2D                 *pTexture             = NULL;

HWND    wndHandle;
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

static PFN_clCreateFromD3D11BufferKHR       pfn_clCreateFromD3D11BufferKHR      = NULL;;
static PFN_clEnqueueAcquireD3D11ObjectsKHR  pfn_clEnqueueAcquireD3D11ObjectsKHR = NULL;
static PFN_clEnqueueReleaseD3D11ObjectsKHR  pfn_clEnqueueReleaseD3D11ObjectsKHR = NULL;
static PFN_clGetDeviceIDsFromD3D11KHR       pfn_clGetDeviceIDsFromD3D11KHR      = NULL;

/**************************************************************************
* Animate controls
**************************************************************************/ 
float animate           = 0.0f;

clock_t t1, t2;
int frameCount          = 0;
int frameRefCount       = 90;
double totalElapsedTime = 0.0;

/**************************************************************************
* Clear render target and the depth stencil
**************************************************************************/ 
const float ClearColor[] = { 0.0f, 0.15f, 0.0f, 0.0f };

/**
*******************************************************************************
* @fn DX11Cleanup
* @brief release DirectX resources
*
* @return void
******************************************************************************/
void DX11Cleanup()
{
    if(NULL !=  pColorMapSampler)
    {
        pColorMapSampler->Release();
        pColorMapSampler = NULL;
    }

    if(NULL != pColorMap)
    {
        pColorMap->Release();
        pColorMap = NULL;
    }
    
    if(NULL != pSolidColorVS)
    {
        pSolidColorVS->Release();
        pSolidColorVS = NULL;
    }
    
    if(NULL != pSolidColorPS)
    {
        pSolidColorPS->Release();
        pSolidColorPS = NULL;
    }
    
    if(NULL != pInputLayout)
    {
        pInputLayout->Release();
        pInputLayout = NULL;
    }

    if(NULL != pD3DContext)
    {
        pD3DContext->Release();
        pD3DContext = NULL;
    }

    if(NULL != pVetexBufPtr)
    {
        pVetexBufPtr->Release();
        pVetexBufPtr = NULL;
    }

    if(NULL != pVetexStagingBuf)
    {
        pVetexStagingBuf->Release();
        pVetexStagingBuf = NULL;
    }

    if(NULL != pRenderTargetView)
    {
        pRenderTargetView->Release();
        pRenderTargetView = NULL;
    }

    if(NULL != pSwapChain)
    {
        pSwapChain->Release();
        pSwapChain = NULL;
    }

    if(NULL != pD3D11Device)
    {
        pD3D11Device->Release();
        pD3D11Device = NULL;
    }

    if(NULL != pTexture)
    {
        pTexture->Release();
        pTexture = NULL;
    }
}

/**
*******************************************************************************
* @fn render
* @brief Draws the scene
*
* @return int : SDK_SUCCESS on success and SDK_FAILURE on failure
******************************************************************************/
int SimpleDX11::render()
{
    t1 = clock() * CLOCKS_PER_SEC;
    frameCount++;
    if(executeKernel() == SDK_FAILURE)
    {
        return SDK_FAILURE;
    }

    if(pD3DContext == 0)
    {
        return SDK_FAILURE;
    }

    pD3DContext->ClearRenderTargetView(pRenderTargetView, ClearColor);
    
    pD3DContext->PSSetSamplers(0, 1, &pColorMapSampler);

    pD3DContext->Draw(meshWidth * meshHeight, 0);

    /*************************************************************************
    * Present the information rendered to the back buffer 
    * to the front buffer (the screen)
    **************************************************************************/ 
    pSwapChain->Present(0, 0);
    animate += 0.01f;

    t2 = clock() * CLOCKS_PER_SEC;
    totalElapsedTime += (double)(t2 - t1);
    if(frameCount && frameCount > frameRefCount)
    {
        double fMs = (double)((totalElapsedTime / (double)CLOCKS_PER_SEC) / (double) frameCount);
        int framesPerSec = (int)(1.0 / (fMs / CLOCKS_PER_SEC));
        char str[128];
        sprintf_s(str, 128, "OpenCL SimpleDX11 | %d FPS", framesPerSec);
        SetWindowText(wndHandle, (str));
        frameCount = 0;
        totalElapsedTime = 0.0;
    }
    return SDK_SUCCESS;
}

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
            {
                return 0;
            }

        case WM_KEYDOWN:
            switch(wParam)
            {
                case 'q':
                case 'Q':
                case VK_ESCAPE:
                    DX11Cleanup();
                    PostQuitMessage(0);
                    return 0;
            }

        default:
            {
                ;
            }
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

/**
*******************************************************************************
* @fn initializeDX11
* @brief Initialize DX11 device and object
*
* @param[in] platform               : CL Platform
* @param[in,out] context            : CL Context
*
* @return int                       : SDK_SUCCESS on success and SDK_FAILURE on failure
******************************************************************************/
int SimpleDX11::initializeDX11(cl_platform_id platform, cl_context &context)
{
    HRESULT hrStatus;
    cl_int status;
    /**************************************************************************
    * Get all adapters
    **************************************************************************/ 
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

    /**************************************************************************
    * Push NULL to vAdapters and assign zero to deviceId if adapter is not found
    **************************************************************************/ 
    if(i == 0 && deviceId > 0)
    {
        deviceId = 0;
        vAdapters.push_back(NULL);
    }

    numAdapters = i;

    DISPLAY_DEVICE dispDevice;
    DWORD deviceNum;
    dispDevice.cb = sizeof(DISPLAY_DEVICE);

    DWORD connectedDisplays = 0;

    int xCoordinate = 0;
    int yCoordinate = 0;
    int xCoordinate1 = 0;

    /**************************************************************************
    * Used to store the number of OpenCL devices corresponding to 
    * the Direct3D 11 object
    **************************************************************************/ 
    cl_uint numDevices;

    for (deviceNum = 0; EnumDisplayDevices(NULL, deviceNum, &dispDevice, 0); deviceNum++) 
    {
        if (dispDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) 
        {
                continue;
        }

        if(!(dispDevice.StateFlags & DISPLAY_DEVICE_ACTIVE))
        {
            std::cout <<"Display device " << deviceNum << " is not connected!!" << std::endl;
            continue;
        }

        /**************************************************************************
        * initialize the DEVMODE structure
        **************************************************************************/
        DEVMODE deviceMode;
        ZeroMemory(&deviceMode, sizeof(deviceMode));
        deviceMode.dmSize = sizeof(deviceMode);
        deviceMode.dmDriverExtra = 0;

        EnumDisplaySettingsEx(dispDevice.DeviceName, ENUM_CURRENT_SETTINGS, &deviceMode, EDS_ROTATEDMODE);

        xCoordinate = deviceMode.dmPosition.x;
        yCoordinate = deviceMode.dmPosition.y;

        /**************************************************************************
        * Register the window class
        **************************************************************************/ 
        RegisterClassEx(&wc);

        /**************************************************************************
        * Create the application's window
        **************************************************************************/ 
        wndHandle = CreateWindow(reinterpret_cast<LPCSTR>(WINDOW_CLASS_NAME), 
                              reinterpret_cast<LPCSTR>(CAPTION_NAME), 
                              WS_CAPTION | WS_POPUPWINDOW, 
                              (isDeviceIdEnabled() && (deviceId < numAdapters))? xCoordinate1 : xCoordinate, 
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

        /**************************************************************************
        * Create a D3D Swap Chain and Device
        **************************************************************************/ 
        DXGI_SWAP_CHAIN_DESC pSwapChainDesc;
        ZeroMemory(&pSwapChainDesc, sizeof(pSwapChainDesc));
        pSwapChainDesc.BufferCount = 1;
        pSwapChainDesc.BufferDesc.Width = meshWidth;
        pSwapChainDesc.BufferDesc.Height = meshHeight;
        pSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        pSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
        pSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        pSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        pSwapChainDesc.OutputWindow = wndHandle;
        pSwapChainDesc.Windowed = true;
        pSwapChainDesc.SampleDesc.Count = 1;
        pSwapChainDesc.SampleDesc.Quality = 0;
        
        std::cout<<"Value of connected displays "<<connectedDisplays<<std::endl;
        
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
                                                &pSwapChainDesc,
                                                &pSwapChain,
                                                &pD3D11Device,
                                                &featureLevel, 
                                                &pD3DContext
                                               );

        if(hrStatus != S_OK)
        {
            std::cout << "\nERROR : Failed to create D3D device" << std::endl;
            return SDK_FAILURE;
        }
        
        status = pfn_clGetDeviceIDsFromD3D11KHR(platform, 
                    CL_D3D11_DEVICE_KHR, 
                    (void *)pD3D11Device, 
                    CL_PREFERRED_DEVICES_FOR_D3D11_KHR,
                    0,
                    NULL,
                    &numDevices);
        if (status != CL_SUCCESS)
        {
            if (status == CL_DEVICE_NOT_FOUND)
            {
                DestroyWindow(wndHandle);
                connectedDisplays++;
                continue;
            }
            else
            {
                CHECK_OPENCL_ERROR(status, "clGetDeviceIDsFromD3D11KHR failed.");
            }
        }

        std::cout<<"Number of OpenCL devices corresponding to the Direct3D 11 object  "<<numDevices<<std::endl;

        if (numDevices == 0)
        {
            DestroyWindow(wndHandle);
            connectedDisplays++;
            continue;
        }
        else 
        {
            if (deviceId == 0)
            {
                break;
            }
            else if(deviceId != connectedDisplays)
            {
                connectedDisplays++;
                DestroyWindow(wndHandle);
                if (xCoordinate >= 0)
                {
                    xCoordinate1 += deviceMode.dmPelsWidth;
                }
                else 
                {
                    xCoordinate1 -= deviceMode.dmPelsWidth;
                }
                continue;
            }
            else 
            {
                ShowWindow(wndHandle, SW_SHOW);
                break;
            }
        }
    }

    cl_device_id *openCLInteropDevices = new cl_device_id[numDevices];
    CHECK_ALLOCATION(openCLInteropDevices,"Allocation failed(openCLInteropDevices)");
    
    /**************************************************************************
    * To get all the OpenCL devices corresponding to the Direct3D object.
    **************************************************************************/ 
    status = pfn_clGetDeviceIDsFromD3D11KHR(platform, 
                CL_D3D11_DEVICE_KHR, 
                (void *)pD3D11Device, 
                CL_PREFERRED_DEVICES_FOR_D3D11_KHR,
                numDevices,
                openCLInteropDevices,
                NULL);

    CHECK_OPENCL_ERROR(status, "clGetDeviceIDsFromD3D11KHR failed.");

    interopDeviceId = openCLInteropDevices[0];

    std::cout<<"Interoperable deviceID "<<interopDeviceId<<std::endl;
    
    /**************************************************************************
    * Create context from ID3D11Device
    **************************************************************************/ 
    cl_context_properties cps[] = 
    { 
        CL_CONTEXT_PLATFORM, 
        (cl_context_properties)platform, 
        CL_CONTEXT_D3D11_DEVICE_KHR, 
        (intptr_t)pD3D11Device, 
        0 
    };

    context = clCreateContext(cps, 
                1,
                &interopDeviceId,
                0,
                0,
                &status);
    CHECK_OPENCL_ERROR(status, "clCreateContext failed.");

    /**************************************************************************
    * Create a render target view
    **************************************************************************/ 
    ID3D11Texture2D *pBackBuffer;
    hrStatus = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if(FAILED(hrStatus))
    {
        std::cout << "GetBuffer failed." << std::endl;
        return SDK_FAILURE;
    }

    hrStatus = pD3D11Device->CreateRenderTargetView(pBackBuffer, NULL, &pRenderTargetView);
    if(FAILED(hrStatus))
    {
        std::cout << "CreateRenderTargetView() failed" << std::endl;
        return SDK_FAILURE;
    }

    pBackBuffer->Release();

    pD3DContext->OMSetRenderTargets(1, &pRenderTargetView, NULL);

    /**************************************************************************
    * Create the viewport
    **************************************************************************/
    D3D11_VIEWPORT vp;
    vp.Width = static_cast<float>(meshWidth);
    vp.Height = static_cast<float>(meshHeight);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    pD3DContext->RSSetViewports(1, &vp);

    /**************************************************************************
    * Read vertex shader resource
    **************************************************************************/
    BasicReader Reader;
    std::vector<unsigned char> &vertexShaderByteCode = Reader.readBinaryFromFile("SimpleDX11VertexShader.cso");

    status = pD3D11Device->CreateVertexShader(
        &vertexShaderByteCode[0],
        vertexShaderByteCode.size(),
        NULL,
        &pSolidColorVS
        );
    if(FAILED(status))
    {
        std::cout << "Create Vertex Shader failed" << std::endl;
        return SDK_FAILURE;
    }

    /**************************************************************************
    * Create and set IAInputLayout
    **************************************************************************/ 
    D3D11_INPUT_ELEMENT_DESC vertexLayout[] = 
    {
        {
            "pos",
            0,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            0,
            0,
            D3D11_INPUT_PER_VERTEX_DATA,
            0
        }
    };

    status = pD3D11Device->CreateInputLayout(
        vertexLayout,
        sizeof(vertexLayout) / sizeof(D3D11_INPUT_ELEMENT_DESC),
        &vertexShaderByteCode[0],
        vertexShaderByteCode.size(),
        &pInputLayout 
       );
    if(FAILED(status))
    {
        std::cout << "Create Input Layout failed" << std::endl;
        return SDK_FAILURE;
    }

    pD3DContext->IASetInputLayout(pInputLayout);

    /******************************************************************************
    * Read pixel shader resource
    ******************************************************************************/
    std::vector<unsigned char> &pixelShaderBytecode = Reader.readBinaryFromFile("SimpleDX11PixelShader.cso");

    status = pD3D11Device->CreatePixelShader(
        &pixelShaderBytecode[0], 
        pixelShaderBytecode.size(), 
        NULL, 
        &pSolidColorPS
        );
    if(FAILED(status))
    {
        std::cout << "Create Pixel Shader failed" << std::endl;
        return SDK_FAILURE;
    }


    /******************************************************************************
    * create shader resource view from file
    ******************************************************************************/
    std::string imagePath("SimpleDX11.bmp");

    streamsdk::SDKBitMap image(imagePath.c_str());
    if (!image.isLoaded())
    {
        std::cout << "\nERROR: could not load bitmap " << imagePath.c_str() << std::endl;
        return SDK_FAILURE;
    }

    // Create shared texture for all duplication threads to draw into
    D3D11_TEXTURE2D_DESC tex2dDesc;
    tex2dDesc.Width = image.getWidth();
    tex2dDesc.Height = image.getHeight();
    tex2dDesc.MipLevels = 1;
    tex2dDesc.ArraySize = 1;
    tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex2dDesc.SampleDesc.Quality = 0;
    tex2dDesc.SampleDesc.Count = 1;
    tex2dDesc.Usage = D3D11_USAGE_IMMUTABLE;
    tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    tex2dDesc.CPUAccessFlags = 0;
    tex2dDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA srInitData;
    srInitData.pSysMem = image.getPixels();
    srInitData.SysMemPitch = image.getWidth() * sizeof(cl_uchar4);

    hrStatus = pD3D11Device->CreateTexture2D(&tex2dDesc, &srInitData, &pTexture);
    if(FAILED(hrStatus))
    {
        std::cout << "\nERROR: Unable to create ID3D11Texture2D " << std::endl;
        return SDK_FAILURE;
    }

    // Create 2D texture resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    HRESULT hr = pD3D11Device->CreateShaderResourceView(pTexture, &srvDesc, &pColorMap);
    if(FAILED(hr))
    {
        std::cout << "Cannot create shader resource view for texture" << std::endl;
        return SDK_FAILURE;
    }

    /**************************************************************************
    * set shader resource and bind sampler for rendering
    **************************************************************************/ 
    pD3DContext->PSSetShaderResources(0, 1, &pColorMap);
    pD3DContext->VSSetShader(pSolidColorVS, 0, 0);
    pD3DContext->PSSetShader(pSolidColorPS, 0, 0);

    D3D11_SAMPLER_DESC colorMapDesc;
    ZeroMemory(&colorMapDesc, sizeof(colorMapDesc));
    colorMapDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
    colorMapDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
    colorMapDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
    colorMapDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    colorMapDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    colorMapDesc.MinLOD = 0;
    colorMapDesc.MaxLOD = D3D11_FLOAT32_MAX;

    status = pD3D11Device->CreateSamplerState(&colorMapDesc, &pColorMapSampler);
    if(FAILED(status))
    {
        std::cout << "Create Color Sampler failed" << std::endl;
        return false;
    }

    return SDK_SUCCESS;
}

/**
*******************************************************************************
* @fn setupSimpleDX11
* @brief Implementation of  SimpleDX11::setupSimpleDX11()   
*
* @return int    : SDK_SUCCESS on success and SDK_FAILURE on failure
******************************************************************************/
int SimpleDX11::setupSimpleDX11()
{ 
    posFromCl = (cl_float*)malloc(meshWidth * meshHeight * sizeof(cl_float4));
    CHECK_ALLOCATION(posFromCl, "Failed to allocate host memory. (posFromCl)");

    memset(posFromCl, 0, 4 * meshWidth * meshHeight  * sizeof(cl_float));

    return SDK_SUCCESS;
}

/**
*******************************************************************************
* @fn genBinaryImage
* @brief Override from SDKSample, Generate binary image of given kernel
*  and exit application
*
* @return int    : SDK_SUCCESS on success and SDK_FAILURE on failure
******************************************************************************/
int SimpleDX11::genBinaryImage()
{
    streamsdk::bifData binaryData;
    binaryData.kernelName = std::string("SimpleDX11_Kernels.cl");
    binaryData.flagsStr = std::string("");
    if(isComplierFlagsSpecified())
    {
        binaryData.flagsFileName = std::string(flags.c_str());
    }

    binaryData.binaryName = std::string(dumpBinary.c_str());
    int status = sampleCommon->generateBinaryImage(binaryData);
    return status;
}

/**
*******************************************************************************
* @fn setupCL
* @brief  OpenCL related initialisations. Set up Context, Device list, 
*    Command Queue, Memory buffers. Build CL kernel program executable
*
* @return int    : SDK_SUCCESS on success and SDK_FAILURE on failure
******************************************************************************/
int SimpleDX11::setupCL()
{
    cl_int status = CL_SUCCESS;
    cl_device_type dType = CL_DEVICE_TYPE_GPU;
    if(deviceType.compare("cpu") == 0)
    {
        dType = CL_DEVICE_TYPE_CPU;
    }
    else //deviceType = "gpu" 
    {
        dType = CL_DEVICE_TYPE_GPU;
        if(isThereGPU() == false)
        {
            std::cout << "GPU not found. Falling back to CPU device" << std::endl;
            dType = CL_DEVICE_TYPE_CPU;
        }
    }

    /******************************************************************************
    * Init extension function pointers     
    ******************************************************************************/
    INIT_CL_EXT_FCN_PTR(clCreateFromD3D11BufferKHR);
    INIT_CL_EXT_FCN_PTR(clEnqueueAcquireD3D11ObjectsKHR);
    INIT_CL_EXT_FCN_PTR(clEnqueueReleaseD3D11ObjectsKHR);
    INIT_CL_EXT_FCN_PTR(clGetDeviceIDsFromD3D11KHR);

    /******************************************************************************
    * Have a look at the available platforms and pick either
    * the AMD one if available or a reasonable default.   
    ******************************************************************************/
    cl_platform_id platform = NULL;
    int retValue = sampleCommon->getPlatform(platform, (int)platformId, isPlatformEnabled());
    CHECK_ERROR(retValue, SDK_SUCCESS, "sampleCommon::getPlatform() failed");

    /******************************************************************************
    * Display available devices.
    ******************************************************************************/
    retValue = sampleCommon->displayDevices(platform, dType);
    CHECK_ERROR(retValue, SDK_SUCCESS, "sampleCommon::displayDevices() failed");

    char pbuf[500];
    status = clGetPlatformInfo(platform,
        CL_PLATFORM_EXTENSIONS,
        sizeof(pbuf),
        pbuf,
        NULL);
    if(status != CL_SUCCESS)
    {
        std::cout<<"Error: clGetPlatformInfo failed. Error code : ";
        std::cout << streamsdk::getOpenCLErrorCodeStr(status) << std::endl;
        return SDK_FAILURE;
    }

    /******************************************************************************
    * Check particular extension
    ******************************************************************************/
    if(!strstr(pbuf, "cl_khr_d3d11_sharing"))
    {
        CHECK_OPENCL_ERROR(status, "Device does not support cl_khr_d3d11_sharing extension!");
    }

    status = initializeDX11(platform, context);
    if(status != SDK_SUCCESS)
    {
        return status;
    }

    retValue = deviceInfo.setDeviceInfo(interopDeviceId);
    CHECK_ERROR(retValue, 0, "SDKDeviceInfo::setDeviceInfo() failed");

    /******************************************************************************
    * Create command queue
    ******************************************************************************/
    commandQueue = clCreateCommandQueue(
        context,
        interopDeviceId,
        0,
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateCommandQueue failed.");

    /******************************************************************************
    * Create Vertex buffer object
    ******************************************************************************/
    D3D11_BUFFER_DESC bufDesc;
    bufDesc.ByteWidth = meshWidth * meshHeight * sizeof(cl_float4);
    bufDesc.Usage = D3D11_USAGE_STAGING;
    bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    bufDesc.BindFlags = 0;
    bufDesc.MiscFlags = 0;
    HRESULT hrStatus = pD3D11Device->CreateBuffer(&bufDesc, NULL, &pVetexStagingBuf);
    if(FAILED(hrStatus))
    {
        std::cout << "CreateBuffer failed. (pVetexStagingBuf)" << std::endl;
        return SDK_FAILURE;
    }

    if(dType == CL_DEVICE_TYPE_GPU)
    {
        bufDesc.CPUAccessFlags = 0;
        bufDesc.Usage = D3D11_USAGE_DEFAULT;
        bufDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
    }
    else
    {
        std::cout << "This sample must run with GPU" << std::endl;
        return SDK_FAILURE;
    }
    
    bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_SHADER_RESOURCE;

    hrStatus = pD3D11Device->CreateBuffer(&bufDesc, NULL, &pVetexBufPtr);
    if(FAILED(hrStatus))
    {
        std::cout << "CreateBuffer failed. (pVetexBufPtr)" << std::endl;
        return SDK_FAILURE;
    }

    /******************************************************************************
    * Bind ID3D11Buffer to the device
    ******************************************************************************/
    if(pVetexBufPtr)
    {
        const UINT strides[] = {4 * sizeof(float)};
        const UINT offsets[] = {0};
        pD3DContext->IASetVertexBuffers(0, 1, &pVetexBufPtr, strides, offsets);
    }

    /******************************************************************************
    * Set primitive topology
    ******************************************************************************/
    pD3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

    /******************************************************************************
    * Create opencl buffer based on D3D11 buffer,
    * then opencl can opreate this sharing buffer with DirectX11.
    ******************************************************************************/
    posBuf = pfn_clCreateFromD3D11BufferKHR(context, CL_MEM_WRITE_ONLY, pVetexBufPtr, &status);
    CHECK_OPENCL_ERROR(status, "pfn_clCreateFromD3D11BufferKHR failed. (posBuf)");

    /******************************************************************************
    * Create a CL program using the kernel source 
    ******************************************************************************/
    streamsdk::buildProgramData buildData;
    buildData.kernelName = std::string("SimpleDX11_Kernels.cl");
    buildData.devices = &interopDeviceId;
    buildData.deviceId = 0;
    buildData.flagsStr = std::string(""); 
    if(isLoadBinaryEnabled())
    {
        buildData.binaryName = std::string(loadBinary.c_str());
    }

    if(isComplierFlagsSpecified())
    {
        buildData.flagsFileName = std::string(flags.c_str());
    }

    retValue = sampleCommon->buildOpenCLProgram(program, context, buildData);
    CHECK_ERROR(retValue, 0, "sampleCommon::buildOpenCLProgram() failed");

    /******************************************************************************
    * Get a kernel object handle for a kernel with the given name
    ******************************************************************************/
    kernel = clCreateKernel(
        program,
        "sineWave",
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

    return SDK_SUCCESS;
}

/**
*******************************************************************************
* @fn setupCLKernels
* @brief Set appropriate arguments to the kernel
*
* @return int    : SDK_SUCCESS on success and SDK_FAILURE on failure
******************************************************************************/
int SimpleDX11::setupCLKernels()
{
    cl_int status;

    /******************************************************************************
    * Position buffer
    ******************************************************************************/
    status = clSetKernelArg(
        kernel,
        0,
        sizeof(cl_mem),
        (void*)&posBuf);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (posBuf)");

    /******************************************************************************
    * Mesh width
    ******************************************************************************/
    status = clSetKernelArg(
        kernel,
        1,
        sizeof(cl_uint),
        (void *)&meshWidth);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (meshWidth)");

    /******************************************************************************
    * Mesh Height
    ******************************************************************************/
    status = clSetKernelArg(
        kernel,
        2,
        sizeof(cl_uint),
        (void *)&meshHeight);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (meshHeight)");

    /******************************************************************************
    * Check group size against group size returned by kernel
    ******************************************************************************/
    status = kernelInfo.setKernelWorkGroupInfo(kernel , interopDeviceId);
    CHECK_OPENCL_ERROR(status, "kernelInfo.setKernelWorkGroupInfo failed.");

    if(groupSize > kernelInfo.kernelWorkGroupSize)
    {
        if(!quiet)
        {
            std::cout << "Out of Resources!" << std::endl;
            std::cout << "Group Size specified : " << groupSize << std::endl;
            std::cout << "Max Group Size supported on the kernel : " 
                << kernelInfo.kernelWorkGroupSize << std::endl;
            std::cout << "Falling back to " << kernelInfo.kernelWorkGroupSize << std::endl;
        }
        groupSize = kernelInfo.kernelWorkGroupSize;
    }

    return SDK_SUCCESS;
}

/**
*******************************************************************************
* @fn SimpleDX11CPUReference
*
* @brief  sine_wave on cpu
*
* @return int    : SDK_SUCCESS on success and SDK_FAILURE on failure
******************************************************************************/
void SimpleDX11::SimpleDX11CPUReference()
{
    for(unsigned int i = 0; i < meshHeight; ++i)
    {
        for(unsigned int j = 0; j < meshWidth; ++j)
        {
            unsigned int x = j;
            unsigned int y = i;

            /******************************************************************************
            * Calculate uv coordinates
            ******************************************************************************/
            float u = x / (float)meshWidth;
            float v = y / (float)meshHeight;
            u = u * 2.0f - 1.0f;
            v = v * 2.0f - 1.0f;

            /******************************************************************************
            * Calculate simple sine wave pattern
            ******************************************************************************/
            float freq = 4.0f;
            float w = sin(u * freq + animate) * cos(v * freq + animate) * 0.5f;
                        
            /******************************************************************************
            * Write output vertex
            ******************************************************************************/
            refPos[i * meshWidth * 4 + j * 4 + 0] = u;
            refPos[i * meshWidth * 4 + j * 4 + 1] = w;
            refPos[i * meshWidth * 4 + j * 4 + 2] = v;
            refPos[i * meshWidth * 4 + j * 4 + 3] = 1.0f;
        }
    }
}

/**
*******************************************************************************
* @fn verifyResults
*
* @brief  Verifies the results obtained with reference implementation
*
* @return int    : SDK_SUCCESS on success and SDK_FAILURE on failure
******************************************************************************/
int SimpleDX11::verifyResults()
{
    if(verify)
    {
        /******************************************************************************
        * Reference implementation, it overwrites the input array with the output
        ******************************************************************************/
        refPos = (cl_float*)malloc(meshWidth * meshHeight * sizeof(cl_float4));
        CHECK_ALLOCATION(refPos, "Failed to allocate host memory. (refPos)");

        memset(refPos, 0, meshWidth * meshHeight * sizeof(cl_float4));
        SimpleDX11CPUReference();
        sampleCommon->printArray<cl_float>("refOutput", refPos, 256, 1);

        /******************************************************************************
        * Compare the results and see if they match
        ******************************************************************************/
        if(!sampleCommon->compare(posFromCl, refPos, meshWidth * meshHeight * 4))
        {
            std::cout << "Failed\n" << std::endl;
            return SDK_FAILURE;
        }
        else
        {
            std::cout << "Passed!\n" << std::endl;
            return SDK_SUCCESS;
        }
    }

    return SDK_SUCCESS;
}

/**
*******************************************************************************
* @fn initialize
*
* @brief   Initialize command line parser, add custom options
*
* @return int    : SDK_SUCCESS on success and SDK_FAILURE on failure
******************************************************************************/
int SimpleDX11::initialize()
{
    /******************************************************************************
    * Call base class Initialize to get default configuration
    ******************************************************************************/
    if (this->SDKSample::initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    /******************************************************************************
    * Now add customized options
    ******************************************************************************/
    streamsdk::Option* array_length = new streamsdk::Option;
    CHECK_ALLOCATION(array_length, "Memory allocation error.\n");


    array_length->_sVersion = "delete device";
    array_length->_lVersion = "device";

    if(multiDevice)
    {
        array_length->_description = "Execute the openCL kernel on a device [cpu|gpu|all]";
    }
    else
    {
        array_length->_description = "Execute the openCL kernel on a device [cpu|gpu]";
    }

    array_length->_type = streamsdk::CA_ARG_STRING;
    array_length->_value = &deviceType;

    sampleArgs->DeleteOption(array_length);


    array_length->_sVersion = "t";
    array_length->_lVersion = "timing";
    array_length->_description = "Print timing.";
    array_length->_type = streamsdk::CA_NO_ARGUMENT;
    array_length->_value = &timing;
    
    sampleArgs->DeleteOption(array_length);
    
    return SDK_SUCCESS;
}

/**
*******************************************************************************
* @fn setup
*
* @brief   perform all sample setup
*
* @return int    : SDK_SUCCESS on success and SDK_FAILURE on failure
******************************************************************************/
int SimpleDX11::setup()
{
    if (setupSimpleDX11() != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    if (setupCL() != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    return SDK_SUCCESS;
}

/**
*******************************************************************************
* @fn executeKernel
*
* @brief   Enqueue calls to the kernels on to the command queue, 
*    wait till end of kernel execution.
*
* @return int    : SDK_SUCCESS on success and SDK_FAILURE on failure
******************************************************************************/
int SimpleDX11::executeKernel()
{
    cl_int status = SDK_SUCCESS;

    /******************************************************************************
    * Set local and global work group sizes
    ******************************************************************************/
    size_t globalWorkSize[2]    = {meshWidth, meshHeight};
    size_t localWorkSize[2]        = {groupSize, 1};

    /******************************************************************************
    * Opencl enqueue acquire D3D11 object, exit if not
    ******************************************************************************/
    status = pfn_clEnqueueAcquireD3D11ObjectsKHR(
         commandQueue,
         1,
         &posBuf,
         0,
         NULL,
         NULL);
     CHECK_OPENCL_ERROR(status, "pfn_clEnqueueAcquireD3D11ObjectsKHR failed(posBuf).");

    /******************************************************************************
    * Change the animated argument
    ******************************************************************************/
    status = clSetKernelArg(kernel, 3, sizeof(float), (void*)&animate);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (g_Anim)");

    status = clEnqueueNDRangeKernel(
         commandQueue,
         kernel,
         2,
         NULL,
         (const size_t*) globalWorkSize,
         (const size_t*) localWorkSize,
         0,
         NULL,
         NULL);
    CHECK_OPENCL_ERROR(status, "clEnqueueNDRangeKernel failed.");

    status = clFinish(commandQueue);
    CHECK_OPENCL_ERROR(status, "clFinish failed.");
    
    /******************************************************************************
    * Opencl enqueue release D3D11 object, exit if not
    ******************************************************************************/
    status = pfn_clEnqueueReleaseD3D11ObjectsKHR(
         commandQueue,
         1,
         &posBuf,
         0,
         NULL,
         NULL);
    CHECK_OPENCL_ERROR(status, "pfn_clEnqueueReleaseD3D11ObjectsKHR failed(posBuf).");

    return SDK_SUCCESS;
}

/**
*******************************************************************************
* @fn run
*
* @brief    Run OpenCL SimpleDX11
*
* @return int    : SDK_SUCCESS on success and SDK_FAILURE on failure
******************************************************************************/
int SimpleDX11::run()
{
    /******************************************************************************
    * Arguments are set and execution call is enqueued on command buffer
    ******************************************************************************/
    if(setupCLKernels() != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    if(!quiet && !verify)
    {
        executeKernel();
        
        /******************************************************************************
        * Show the window
        ******************************************************************************/
        ShowWindow(wndHandle, SW_SHOWDEFAULT);
        UpdateWindow(wndHandle);

        /******************************************************************************
        * Enter the message loop
        ******************************************************************************/
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
  
    }
    else 
    {
        if(verify)
        {
            executeKernel();

            /******************************************************************************
            * Duplicate D3D11 buffer to pVetexStagingBuf for verification
            ******************************************************************************/
            pD3DContext->CopyResource(pVetexStagingBuf, pVetexBufPtr);
            pD3DContext->Flush();

            /******************************************************************************
            * Get the data from pVetexStagingBuf by map and unmap functions of D3D11
            ******************************************************************************/
            void* ptr = NULL;
            D3D11_MAPPED_SUBRESOURCE MappedResource;
            HRESULT hrStatus = pD3DContext->Map(pVetexStagingBuf, 0, D3D11_MAP_READ, 0, &MappedResource);
            if(FAILED(hrStatus))
            {
                std::cout << "Map failed.(pVetexStagingBuf)" << std::endl;
                return SDK_FAILURE;
            }

            ptr = (void*)MappedResource.pData;
            memcpy(posFromCl, ptr, meshWidth * meshHeight * sizeof(cl_float4));
            pD3DContext->Unmap(pVetexStagingBuf, 0);
        }

        sampleCommon->printArray<cl_float>("Output", posFromCl, 256, 1);
    }

    return SDK_SUCCESS;
}

/**
*******************************************************************************
* @fn cleanup
*
* @brief    Cleanup memory allocations
*
* @return int    : SDK_SUCCESS on success and SDK_FAILURE on failure
******************************************************************************/
int SimpleDX11::cleanup()
{
    /******************************************************************************
    * Releases OpenCL resources (Context, Memory etc.)
    ******************************************************************************/
    cl_int status;

    status = clReleaseMemObject(posBuf);
    CHECK_OPENCL_ERROR(status, "clReleaseMemObject failed.(posBuf)");

    status = clReleaseKernel(kernel);
    CHECK_OPENCL_ERROR(status, "clReleaseKernel failed.(kernel)");

    status = clReleaseProgram(program);
    CHECK_OPENCL_ERROR(status, "clReleaseProgram failed.(program)");

    status = clReleaseCommandQueue(commandQueue);
    CHECK_OPENCL_ERROR(status, "clReleaseCommandQueue failed. (commandQueue)");

    status = clReleaseContext(context);
    CHECK_OPENCL_ERROR(status, "clReleaseContext failed.(context)");

    return SDK_SUCCESS;
}

SimpleDX11::~SimpleDX11()
{
    DX11Cleanup();

    FREE(posFromCl);
    FREE(refPos);
    FREE(devices);
}

/********************************************************************************
* Execution of program begins from here                                         *
********************************************************************************/
int main(int argc, char * argv[])
{
    int status = 0;
    bool isVistaOrWin7 = false;

    /******************************************************************************
    * Find the version of Windows
    ******************************************************************************/
    OSVERSIONINFO vInfo;
    memset(&vInfo, 0, sizeof(vInfo));
    vInfo.dwOSVersionInfoSize = sizeof(vInfo);
    if(!GetVersionEx(&vInfo))
    {
        std::cout << "\nERROR : Unable to get Windows version information.\n" << std::endl;
        return SDK_FAILURE;
    }

    if(vInfo.dwMajorVersion >= 6)
    {
        isVistaOrWin7 = true;
    }
    else
    {
        std::cout << "Unsupported OS!\n";
        return SDK_SUCCESS;
    }

    SimpleDX11 clSimpleDX11("OpenCL SimpleDX11");

    status = clSimpleDX11.initialize();
    if(status != SDK_SUCCESS)
    {
        return status == SDK_EXPECTED_FAILURE ? SDK_SUCCESS :SDK_FAILURE;
    }

    if (clSimpleDX11.parseCommandLine(argc, argv) != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    if(clSimpleDX11.isDumpBinaryEnabled())
    {
        return clSimpleDX11.genBinaryImage();
    }

    status = clSimpleDX11.setup();
    if(status != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    if (clSimpleDX11.run() != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }
    
    if (clSimpleDX11.verifyResults() != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    if (clSimpleDX11.cleanup() != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    UnregisterClass(reinterpret_cast<LPCSTR>(WINDOW_CLASS_NAME), wc.hInstance);

    return SDK_SUCCESS;
}