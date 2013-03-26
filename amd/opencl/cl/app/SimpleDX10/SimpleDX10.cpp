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

#include "SimpleDX10.hpp"
#include <cmath>
#include<malloc.h>
#include <DXGI.h>


void* me;           /**< Pointing to SimpleDX10 class */

// D3D10 global vars
D3D10_DRIVER_TYPE                   driverType = D3D10_DRIVER_TYPE_NULL;
ID3D10Device*                       d3d10DevicePtr = NULL;
IDXGISwapChain*                     swapChain = NULL;
ID3D10RenderTargetView*             randerTargetViewPtr = NULL;
ID3D10DepthStencilView*             depthStencilViewPtr = NULL;
ID3D10Effect*                       effectPtr = NULL;
ID3D10EffectTechnique*              techniquePtr = NULL;
ID3D10EffectMatrixVariable*         matXformPtr;
ID3D10EffectShaderResourceVariable* texSRVarPtr = NULL;
ID3D10Texture2D*                    texturePtr = NULL;
ID3D10ShaderResourceView*           texSRViewPtr = NULL;
ID3D10Buffer*                       vetexBufPtr = NULL;
ID3D10Buffer*                       vetexStagingBufPtr = NULL;
ID3D10InputLayout*                  inputLayoutPtr = NULL;
#ifdef WIN8
XMMATRIX                            matXform;
#else
D3DXMATRIX                          matXform;
#endif

HWND    wndHandle;
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
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

typedef CL_API_ENTRY cl_mem (CL_API_CALL *PFN_clCreateFromD3D10BufferKHR)(
    cl_context context, cl_mem_flags flags, ID3D10Buffer*  buffer,
    cl_int* errcode_ret);
typedef CL_API_ENTRY cl_int (CL_API_CALL *PFN_clEnqueueAcquireD3D10ObjectsKHR)(
    cl_command_queue command_queue, cl_uint num_objects,
    const cl_mem* mem_objects, cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event);
typedef CL_API_ENTRY cl_int (CL_API_CALL *PFN_clEnqueueReleaseD3D10ObjectsKHR)(
    cl_command_queue command_queue, cl_uint num_objects,
    const cl_mem* mem_objects,  cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list, cl_event* event);

typedef CL_API_ENTRY cl_int (CL_API_CALL *PFN_clGetDeviceIDsFromD3D10KHR)(
    cl_platform_id platform,
    cl_d3d10_device_source_khr d3d_device_source,
    void *d3d_object,
    cl_d3d10_device_set_khr d3d_device_set,
    cl_uint num_entries,
    cl_device_id *devices,
    cl_uint *num_devices);


static PFN_clCreateFromD3D10BufferKHR pfn_clCreateFromD3D10BufferKHR = NULL;;
static PFN_clEnqueueAcquireD3D10ObjectsKHR pfn_clEnqueueAcquireD3D10ObjectsKHR = NULL;
static PFN_clEnqueueReleaseD3D10ObjectsKHR pfn_clEnqueueReleaseD3D10ObjectsKHR = NULL;
static PFN_clGetDeviceIDsFromD3D10KHR pfn_clGetDeviceIDsFromD3D10KHR = NULL;

// Mouse controls
int mouseOldX;
int mouseOldY;
int mouseButtons = 0;
float rotateX    = 90.0;
float rotateY    = 0.0;
float translateZ = 0.5;
float animate = 0.0f;

clock_t t1, t2;
int frameCount = 0;
int frameRefCount = 90;
double totalElapsedTime = 0.0;

//os
bool isWin8 = false;

int SimpleDX10::loadTexture(ID3D10Texture2D** ppTex2D)
{
    std::string imagePath = sampleCommon->getPath();
    imagePath.append("SimpleDX10.bmp");

    streamsdk::SDKBitMap image(imagePath.c_str());
    if (!image.isLoaded())
    {
        std::cout << "\nERROR: could not load bitmap " << imagePath.c_str() << std::endl;
        return SDK_FAILURE;
    }

    HRESULT hrStatus;
    D3D10_TEXTURE2D_DESC tex2dDesc;
    tex2dDesc.Width = image.getWidth();
    tex2dDesc.Height = image.getHeight();
    tex2dDesc.MipLevels = 1;
    tex2dDesc.ArraySize = 1;
    tex2dDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex2dDesc.SampleDesc.Quality = 0;
    tex2dDesc.SampleDesc.Count = 1;
    tex2dDesc.Usage = D3D10_USAGE_IMMUTABLE;
    tex2dDesc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    tex2dDesc.CPUAccessFlags = 0;
    tex2dDesc.MiscFlags = 0;

    D3D10_SUBRESOURCE_DATA srInitData;
    srInitData.pSysMem = image.getPixels();
    srInitData.SysMemPitch = image.getWidth() * sizeof(cl_uchar4);

    hrStatus = d3d10DevicePtr->CreateTexture2D(&tex2dDesc, &srInitData, ppTex2D);
    if(FAILED(hrStatus))
    {
        std::cout << "\nERROR: Unable to create ID3D10Texture2D " << std::endl;
        return SDK_FAILURE;
    }

    return SDK_SUCCESS;
}


/**
 * Name: Cleanup()
 * Desc: Releases all previously initialized objects
 */
void DX10Cleanup()
{
    if( texturePtr != NULL )
        texturePtr->Release();

    if( vetexBufPtr != NULL )
        vetexBufPtr->Release();

    if( vetexStagingBufPtr != NULL )
        vetexStagingBufPtr->Release();

    if( d3d10DevicePtr != NULL )
        d3d10DevicePtr->Release();

}

/**
 * Name: Render()
 * Desc: Draws the scene
 */
int SimpleDX10::render()
{
    t1 = clock() * CLOCKS_PER_SEC;
    frameCount++;
    if(executeKernel() == SDK_FAILURE)
        return SDK_FAILURE;

	
     // Clear render target and the depth stencil 
    float ClearColor[] = { 0.0f, 0.15f, 0.0f, 0.0f };
    d3d10DevicePtr->ClearRenderTargetView(randerTargetViewPtr, ClearColor );
    d3d10DevicePtr->ClearDepthStencilView(depthStencilViewPtr, D3D10_CLEAR_DEPTH, 1.0, 0 );


#ifdef WIN8
	// Update effect x-form matrix variable
	matXform = XMMatrixTranslation(0.0, translateZ, 0.0);
    XMMATRIX mTemp;
    mTemp = XMMatrixRotationX((float)XMConvertToRadians(rotateX));
    matXform = XMMatrixMultiply(matXform, mTemp);
    mTemp = XMMatrixRotationY((float)XMConvertToRadians(rotateY));
    matXform = XMMatrixMultiply(matXform, mTemp);

    // Scale the image
    mTemp = XMMatrixScaling(0.75, 0.75, 0.25);
    matXform = XMMatrixMultiply(matXform, mTemp);
#else
	// Update effect x-form matrix variable
    D3DXMatrixTranslation(&matXform, 0.0, translateZ, 0.0);
    D3DXMATRIX mTemp;
    D3DXMatrixRotationX(&mTemp, (float)D3DXToRadian(rotateX));
    D3DXMatrixMultiply(&matXform, &matXform, &mTemp);
    D3DXMatrixRotationY(&mTemp, (float)D3DXToRadian(rotateY));
    D3DXMatrixMultiply(&matXform, &matXform, &mTemp);

    // Scale the image
    D3DXMatrixScaling(&mTemp, 0.75, 0.75, 0.25);
    D3DXMatrixMultiply(&matXform, &matXform, &mTemp);

#endif
    // Set transformation matrix to effect
    matXformPtr->SetMatrix((float*) &matXform);
    techniquePtr->GetPassByName("p0")->Apply(0);
    d3d10DevicePtr->Draw(meshWidth * meshHeight, 0);

    // Present the information rendered to the back buffer to the front buffer (the screen)
    swapChain->Present(0, 0);
    animate += 0.01f;

    t2 = clock() * CLOCKS_PER_SEC;
    totalElapsedTime += (double)(t2 - t1);
    if(frameCount && frameCount > frameRefCount)
    {
        double fMs = (double)((totalElapsedTime / (double)CLOCKS_PER_SEC) / (double) frameCount);
        int framesPerSec = (int)(1.0 / (fMs / CLOCKS_PER_SEC));
        char str[128];
        sprintf_s(str, 128, "OpenCL SimpleDX10 | %d FPS", framesPerSec);
        SetWindowText(wndHandle, (str));
        frameCount = 0;
        totalElapsedTime = 0.0;
    }
    return SDK_SUCCESS;
}

/**
 * Name: MsgProc()
 * Desc: The window's message handler
 */
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {

        case WM_DESTROY:
            return 0;

        case WM_KEYDOWN:
            switch(wParam)
            {
                case 'q':
                case 'Q':
                case VK_ESCAPE:
                    DX10Cleanup();
                    PostQuitMessage(0);
                    return 0;
            }
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}


int SimpleDX10::initializeDX10(cl_platform_id platform, 
                    cl_context &context, 
                    cl_device_id &interopDevice)
{
    HRESULT hrStatus;
    cl_int status;
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

    //Push NULL to vAdapters and assign zero to deviceId if adapter is not found
    if(i == 0 && deviceId >= 0)
    {
        deviceId = 0;
        vAdapters.push_back(NULL);
    }

    numAdapters = i;

    DISPLAY_DEVICE dispDevice;
    DWORD deviceNum;
    dispDevice.cb = sizeof(DISPLAY_DEVICE);

    DWORD displayDevices = 0;
    DWORD connectedDisplays = 0;

    int xCoordinate = 0;
    int yCoordinate = 0;
    int xCoordinate1 = 0;

    cl_uint numDevices;//Used to store the number of OpenCL devices corresponding to the Direct3D 10 object

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
                              (isDeviceIdEnabled() && (deviceId < numAdapters))? xCoordinate1 : xCoordinate, 
                              yCoordinate, 
                              WINDOW_WIDTH, 
                              WINDOW_HEIGHT, 
                              NULL, 
                              NULL, 
                              wc.hInstance, 
                              NULL);


        // Create a D3D Swap Chain and Device
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 1;
        sd.BufferDesc.Width = meshWidth;
        sd.BufferDesc.Height = meshHeight;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = wndHandle;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        
		std::cout<<"Value of connected displays "<<connectedDisplays<<std::endl;

		hrStatus =  D3D10CreateDeviceAndSwapChain((connectedDisplays < numAdapters) ? vAdapters.at(connectedDisplays) : vAdapters.at(numAdapters - 1) , 
                                                D3D10_DRIVER_TYPE_HARDWARE, 
                                                NULL,
                                                0,
                                                D3D10_SDK_VERSION, 
                                                &sd,
                                                &swapChain,
                                                &d3d10DevicePtr );

        if(hrStatus != S_OK)
        {
            std::cout << "\nERROR : Failed to create D3D device" << std::endl;
            return SDK_FAILURE;
        }
        
        

        status = pfn_clGetDeviceIDsFromD3D10KHR(platform, 
                    CL_D3D10_DEVICE_KHR, 
                    (void *)d3d10DevicePtr, 
                    CL_PREFERRED_DEVICES_FOR_D3D10_KHR,
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
                CHECK_OPENCL_ERROR(status, "clGetDeviceIDsFromD3D10KHR failed.");
            }
        }
        std::cout<<"Number of OpenCL devices corresponding to the Direct3D 10 object  "<<numDevices<<std::endl;

        if (numDevices == 0)
        {
            DestroyWindow(wndHandle);
            connectedDisplays++;
            continue;
        }
        else 
        {
            if (deviceId == 0)
                break;
            else if(deviceId != connectedDisplays)
            {
                connectedDisplays++;
                DestroyWindow(wndHandle);
                if (xCoordinate >= 0)
                {
                    xCoordinate1 += deviceMode.dmPelsWidth;
                    // try the next display
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

    /**
    * To get all the OpenCL devices corresponding to the Direct3D object.
    */

    status = pfn_clGetDeviceIDsFromD3D10KHR(platform, 
                CL_D3D10_DEVICE_KHR, 
                (void *)d3d10DevicePtr, 
                CL_PREFERRED_DEVICES_FOR_D3D10_KHR,
                numDevices,
                openCLInteropDevices,
                NULL);

    CHECK_OPENCL_ERROR(status, "clGetDeviceIDsFromD3D10KHR failed.");

    interopDeviceId = openCLInteropDevices[0];

    std::cout<<"Interoperable deviceID "<<interopDeviceId<<std::endl;
    /*
     * If we could find our platform, use it. Otherwise use just available platform.
     */

    // Create context from ID3D10Device
    cl_context_properties cps[] = 
    { 
        CL_CONTEXT_PLATFORM, 
        (cl_context_properties)platform, 
        CL_CONTEXT_D3D10_DEVICE_KHR, 
        (intptr_t)d3d10DevicePtr, 
        0 
    };

    context = clCreateContext(cps, 
                1,
                &interopDeviceId,
                0,
                0,
                &status);
    CHECK_OPENCL_ERROR(status, "clCreateContext failed.");

    // Create a render target view
    ID3D10Texture2D *pBackBuffer;
    hrStatus = swapChain->GetBuffer(0, __uuidof( ID3D10Texture2D ), (LPVOID*)&pBackBuffer );
    if(FAILED(hrStatus))
    {
        std::cout << "GetBuffer failed." << std::endl;
        return SDK_FAILURE;
    }

    hrStatus = d3d10DevicePtr->CreateRenderTargetView(pBackBuffer, NULL, &randerTargetViewPtr );
    pBackBuffer->Release();
    if(FAILED(hrStatus))
    {
        std::cout << "CreateRenderTargetView() failed" << std::endl;
        return SDK_FAILURE;
    }

    ID3D10Texture2D *stenciltext;
    {
        D3D10_TEXTURE2D_DESC d;
        d.CPUAccessFlags = 0;
        d.Usage = D3D10_USAGE_DEFAULT;
        d.Width = meshWidth;
        d.Height = meshHeight;
        d.MipLevels = 1;
        d.Format = DXGI_FORMAT_D32_FLOAT;
        d.ArraySize = 1;
        d.SampleDesc.Count = 1;
        d.SampleDesc.Quality = 0;
        d.BindFlags = D3D10_BIND_DEPTH_STENCIL;
        d.MiscFlags = 0;

        hrStatus = d3d10DevicePtr->CreateTexture2D(&d,0,&stenciltext);
        if(FAILED(hrStatus))
        {
            std::cout << "CreateTexture2D() failed" << std::endl;
            return SDK_FAILURE;
        }
    }

    {
        D3D10_DEPTH_STENCIL_VIEW_DESC p;
        p.Format = DXGI_FORMAT_D32_FLOAT;
        p.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
        p.Texture2DArray.MipSlice = 0;

        hrStatus = d3d10DevicePtr->CreateDepthStencilView(stenciltext,&p,&depthStencilViewPtr);
        stenciltext->Release();
        if(FAILED(hrStatus))
        {
            std::cout << "CreateDepthStencilView() failed" << std::endl;
            return SDK_FAILURE;
        }
    }
    d3d10DevicePtr->OMSetRenderTargets(1, &randerTargetViewPtr, NULL);

    // Create the viewport
    D3D10_VIEWPORT vp;
    vp.Width = meshWidth;
    vp.Height = meshHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    d3d10DevicePtr->RSSetViewports(1, &vp);
    return SDK_SUCCESS;
}


int
SimpleDX10::setupSimpleDX10()
{
 
    pos = (cl_float*)malloc(meshWidth * meshHeight * sizeof(cl_float4));
    CHECK_ALLOCATION(pos, "Failed to allocate host memory. (pos)");

    memset(pos, 0, 4 * meshWidth * meshHeight  * sizeof(cl_float));

    return SDK_SUCCESS;
}

int 
SimpleDX10::genBinaryImage()
{
    streamsdk::bifData binaryData;
    binaryData.kernelName = std::string("SimpleDX10_Kernels.cl");
    binaryData.flagsStr = std::string("");
    if(isComplierFlagsSpecified())
        binaryData.flagsFileName = std::string(flags.c_str());

    binaryData.binaryName = std::string(dumpBinary.c_str());
    int status = sampleCommon->generateBinaryImage(binaryData);
    return status;
}


int
SimpleDX10::setupCL()
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


    // Init extension function pointers
#define INIT_CL_EXT_FCN_PTR(name) \
    if(!pfn_##name) { \
        pfn_##name = (PFN_##name) \
        clGetExtensionFunctionAddress(#name); \
        if(!pfn_##name) { \
            std::cout << "Cannot get pointer to ext. fcn. " #name << std::endl; \
            return SDK_FAILURE; \
        } \
    }

    INIT_CL_EXT_FCN_PTR(clCreateFromD3D10BufferKHR);
    INIT_CL_EXT_FCN_PTR(clEnqueueAcquireD3D10ObjectsKHR);
    INIT_CL_EXT_FCN_PTR(clEnqueueReleaseD3D10ObjectsKHR);
    INIT_CL_EXT_FCN_PTR(clGetDeviceIDsFromD3D10KHR);

    /*
     * Have a look at the available platforms and pick either
     * the AMD one if available or a reasonable default.
     */
    cl_platform_id platform = NULL;
    int retValue = sampleCommon->getPlatform(platform, platformId, isPlatformEnabled());
    CHECK_ERROR(retValue, SDK_SUCCESS, "sampleCommon::getPlatform() failed");


    // Display available devices.
    retValue = sampleCommon->displayDevices(platform, dType);
    CHECK_ERROR(retValue, SDK_SUCCESS, "sampleCommon::displayDevices() failed");

    status = initializeDX10(platform, context, interopDeviceId);
    if(status != SDK_SUCCESS)
        return status;

    retValue = deviceInfo.setDeviceInfo(interopDeviceId);
    CHECK_ERROR(retValue, 0, "SDKDeviceInfo::setDeviceInfo() failed");

    // Check particular extension
    if(!strstr(deviceInfo.extensions, "cl_khr_d3d10_sharing"))
    CHECK_OPENCL_ERROR(status, "Device does not support cl_khr_d3d10_sharing extension!");

    // Create command queue
    commandQueue = clCreateCommandQueue(
        context,
        interopDeviceId,
        0,
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateCommandQueue failed.");

    // Create Vertex buffer object
    D3D10_BUFFER_DESC bufDesc;
    bufDesc.ByteWidth = meshWidth * meshHeight * sizeof(cl_float4);
    bufDesc.Usage = D3D10_USAGE_STAGING;
    bufDesc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
    bufDesc.BindFlags = 0;
    bufDesc.MiscFlags = 0;
    HRESULT hrStatus = d3d10DevicePtr->CreateBuffer(&bufDesc, NULL, &vetexStagingBufPtr);
    if(FAILED(hrStatus))
    {
        std::cout << "CreateBuffer failed. (vetexStagingBufPtr)" << std::endl;
        return SDK_FAILURE;
    }

    bufDesc.ByteWidth = meshWidth * meshHeight * sizeof(cl_float4);
    if(dType == CL_DEVICE_TYPE_GPU)
    {
		bufDesc.CPUAccessFlags = 0;
		bufDesc.Usage = D3D10_USAGE_DEFAULT;
		bufDesc.MiscFlags = D3D10_RESOURCE_MISC_SHARED;
    }
    else
	{
		bufDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
        bufDesc.Usage = D3D10_USAGE_DYNAMIC;

		if(isWin8)
		{
			bufDesc.MiscFlags = D3D10_RESOURCE_MISC_TEXTURECUBE;
		}
		else
		{
			bufDesc.MiscFlags = D3D10_RESOURCE_MISC_SHARED;
		}
    }
	
    bufDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER | D3D10_BIND_SHADER_RESOURCE;

    hrStatus = d3d10DevicePtr->CreateBuffer(&bufDesc, NULL, &vetexBufPtr);
    if(FAILED(hrStatus))
    {
        std::cout << "CreateBuffer failed. (vetexBufPtr)" << std::endl;
        return SDK_FAILURE;
    }

    // Set vertex bufer
    if(vetexBufPtr)
    {
        const UINT strides[] = {4 * sizeof(float)};
        const UINT offsets[] = {0};
        d3d10DevicePtr->IASetVertexBuffers(0, 1, &vetexBufPtr, strides, offsets);
    }

    // Set primitive topology
    d3d10DevicePtr->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);

    posBuf = pfn_clCreateFromD3D10BufferKHR(context, CL_MEM_WRITE_ONLY, vetexBufPtr, &status);
    CHECK_OPENCL_ERROR(status, "pfn_clCreateFromD3D10BufferKHR failed. (posBuf)");

    // create a CL program using the kernel source 
    streamsdk::buildProgramData buildData;
    buildData.kernelName = std::string("SimpleDX10_Kernels.cl");
    buildData.devices = &interopDeviceId;
    buildData.deviceId = 0;
    buildData.flagsStr = std::string(""); 
    if(isLoadBinaryEnabled())
        buildData.binaryName = std::string(loadBinary.c_str());

    if(isComplierFlagsSpecified())
        buildData.flagsFileName = std::string(flags.c_str());

    retValue = sampleCommon->buildOpenCLProgram(program, context, buildData);
    CHECK_ERROR(retValue, 0, "sampleCommon::buildOpenCLProgram() failed");

    // get a kernel object handle for a kernel with the given name
    kernel = clCreateKernel(
        program,
        "sineWave",
        &status);
    CHECK_OPENCL_ERROR(status, "clCreateKernel failed.");

    if(loadTexture(&texturePtr) == SDK_FAILURE)
    {
        std::cout << "\n ERROR : Failed to load texture" << std::endl;
        return SDK_FAILURE;
    }

    return SDK_SUCCESS;
}


int 
SimpleDX10::setupCLKernels()
{
    cl_int status;

    // Set appropriate arguments to the kernel

    // Position buffer
    status = clSetKernelArg(
        kernel,
        0,
        sizeof(cl_mem),
        (void*)&posBuf);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (posBuf)");

    // Mesh width
    status = clSetKernelArg(
        kernel,
        1,
        sizeof(cl_uint),
        (void *)&meshWidth);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (meshWidth)");

    // Mesh Height
    status = clSetKernelArg(
        kernel,
        2,
        sizeof(cl_uint),
        (void *)&meshHeight);
    CHECK_OPENCL_ERROR(status, "clSetKernelArg failed. (meshHeight)");

    // Check group size against group size returned by kernel
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
 * sine_wave on cpu
 */
void 
SimpleDX10::SimpleDX10CPUReference()
{
    for(unsigned int i = 0; i < meshHeight; ++i)
    {
        for(unsigned int j = 0; j < meshWidth; ++j)
        {
            unsigned int x = j;
            unsigned int y = i;

            // calculate uv coordinates
            float u = x / (float)meshWidth;
            float v = y / (float)meshHeight;
            u = u * 2.0f - 1.0f;
            v = v * 2.0f - 1.0f;

            // calculate simple sine wave pattern
            float freq = 4.0f;
            float w = sin(u * freq + animate) * cos(v * freq + animate) * 0.5f;

            // write output vertex
            refPos[i * meshWidth * 4 + j * 4 + 0] = u;
            refPos[i * meshWidth * 4 + j * 4 + 1] = w;
            refPos[i * meshWidth * 4 + j * 4 + 2] = v;
            refPos[i * meshWidth * 4 + j * 4 + 3] = 1.0f;
        }
    }
}

int
SimpleDX10::initialize()
{
    // Call base class Initialize to get default configuration
    if (this->SDKSample::initialize() != SDK_SUCCESS)
        return SDK_FAILURE;

    return SDK_SUCCESS;
}

int
SimpleDX10::setup()
{
    if (setupSimpleDX10() != SDK_SUCCESS)
        return SDK_FAILURE;

    if (setupCL() != SDK_SUCCESS)
        return SDK_FAILURE;

    return SDK_SUCCESS;
}

int SimpleDX10::executeKernel()
{

    cl_int status = CL_SUCCESS;

    // Set local and global work group sizes
    size_t localX = (size_t)sqrt((double)groupSize);
    size_t globalWorkSize[2] = {meshWidth, meshHeight};
    size_t localWorkSize[2] = {groupSize, 1};
 
     static ULONG rcprev = 2;
     ULONG rc;
     if(vetexBufPtr && (rc = vetexBufPtr->AddRef()))
     {
         rc = vetexBufPtr->Release();
         if(rc != rcprev)
         {
             std::cout << "ERROR: Ref.count(g_pVB) increased" << std::endl;
             return SDK_FAILURE;
         }
         rcprev = rc;
     }

     status = pfn_clEnqueueAcquireD3D10ObjectsKHR(
         commandQueue,
         1,
         &posBuf,
         0,
         NULL,
         NULL);
     CHECK_OPENCL_ERROR(status, "pfn_clEnqueueAcquireD3D10ObjectsKHR failed(posBuf).");

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
    
    status = pfn_clEnqueueReleaseD3D10ObjectsKHR(
         commandQueue,
         1,
         &posBuf,
         0,
         NULL,
         NULL);
    CHECK_OPENCL_ERROR(status, "pfn_clEnqueueReleaseD3D10ObjectsKHR failed(posBuf).");

    return SDK_SUCCESS;
}


int 
SimpleDX10::run()
{
    // Arguments are set and execution call is enqueued on command buffer
    if(setupCLKernels() != SDK_SUCCESS)
        return SDK_FAILURE;

    if(!quiet && !verify)
    {
        executeKernel();

        HRESULT hr = S_OK;

        // Create effect from file simpleDX10.fx
        DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
        ID3D10Blob* pErrMsgs = NULL;

    /**************************************************************************
    * Read Complied effect file
    **************************************************************************/
    ifstream is("SimpleDX10.fxo", ios::binary); 
    is.seekg(0,ios_base::end); 
    streampos pos = is.tellg(); 
    is.seekg(0,ios_base::beg); 
    char * effectBuffer = new char[(unsigned int)pos]; 
    is.read(effectBuffer,pos); 
    hr = D3D10CreateEffectFromMemory((void *)effectBuffer,(size_t)pos,dwShaderFlags,d3d10DevicePtr,NULL,&effectPtr); 

		  if(FAILED(hr))
        {
           
            std::cout << "\nError: Unable to load FX file : " 
                      << "Please make sure it is located in working directory or in path" 
                      << std::endl;
            return SDK_FAILURE;
        }


        // Get the technique
        techniquePtr = effectPtr->GetTechniqueByName("t0");

        // Get effect matrix variable
        matXformPtr = effectPtr->GetVariableByName("matXform")->AsMatrix();

        // Create 2D texture resource view
        D3D10_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        hr = d3d10DevicePtr->CreateShaderResourceView(texturePtr, &srvDesc, &texSRViewPtr);
        if(FAILED(hr))
        {
            std::cout << "Cannot create shader resource view for texture" << std::endl;
            return SDK_FAILURE;
        }
        texSRVarPtr = effectPtr->GetVariableByName("tex")->AsShaderResource();
        texSRVarPtr->SetResource(texSRViewPtr);

        // Create and set IAInputLayout
        D3D10_INPUT_ELEMENT_DESC vertexLayout[] = {
            {
                "pos",
                0,
                DXGI_FORMAT_R32G32B32A32_FLOAT,
                0,
                0,
                D3D10_INPUT_PER_VERTEX_DATA,
                0
            }
        };
        D3D10_PASS_DESC passDesc;
        hr = techniquePtr->GetPassByName("p0")->GetDesc(&passDesc);
        if(FAILED(hr))
        {
            std::cout << "\nFailed get pass description\n" << std::endl;
            return SDK_FAILURE;
        }
        hr = d3d10DevicePtr->CreateInputLayout(
            vertexLayout,
            sizeof(vertexLayout) / sizeof(D3D10_INPUT_ELEMENT_DESC),
            passDesc.pIAInputSignature,
            passDesc.IAInputSignatureSize,
            &inputLayoutPtr);
        if(FAILED(hr))
        {
            std::cout << "\nFailed created IA input layout\n" << std::endl;
            return SDK_FAILURE;
        }
        d3d10DevicePtr->IASetInputLayout(inputLayoutPtr);

        // Show the window
        ShowWindow(wndHandle, SW_SHOWDEFAULT);
        UpdateWindow(wndHandle);

        // Enter the message loop
        MSG msg;
        ZeroMemory(&msg, sizeof(msg));

        while( msg.message != WM_QUIT )
        {
            if( PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
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
        if(verify || timing)
        {
            executeKernel();
            d3d10DevicePtr->CopyResource(vetexStagingBufPtr, vetexBufPtr);
            d3d10DevicePtr->Flush();

            void* ptr = NULL;
            HRESULT hrStatus = vetexStagingBufPtr->Map(D3D10_MAP_READ, 0, &ptr);
            if(FAILED(hrStatus))
            {
                std::cout << "Map failed.(vetexStagingBufPtr)" << std::endl;
                return SDK_FAILURE;
            }

            memcpy(pos, ptr, meshWidth * meshHeight * sizeof(cl_float4));
            vetexStagingBufPtr->Unmap();
        }
        sampleCommon->printArray<cl_float>("Output", pos, 256, 1);
    }

    return SDK_SUCCESS;
}

int
SimpleDX10::verifyResults()
{
    if(verify)
    {
        /* reference implementation
         * it overwrites the input array with the output
         */
        refPos = (cl_float*)malloc(meshWidth * meshHeight * sizeof(cl_float4));
        CHECK_ALLOCATION(refPos, "Failed to allocate host memory. (refPos)");

        memset(refPos, 0, meshWidth * meshHeight * sizeof(cl_float4));
        SimpleDX10CPUReference();
        sampleCommon->printArray<cl_float>("refOutput", refPos, 256, 1);

        // compare the results and see if they match
        if(!sampleCommon->compare(pos, refPos, meshWidth * meshHeight * 4))
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

int
SimpleDX10::cleanup()
{
    // Releases OpenCL resources (Context, Memory etc.)
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

SimpleDX10::~SimpleDX10()
{
    // release program resources
    FREE(pos);
    FREE(refPos);
    FREE(devices);
}


int 
main(int argc, char * argv[])
{
    int status = 0;
    bool isVistaOrWin7OrWin8	= false;

    // Find the version of Windows
    OSVERSIONINFO vInfo;
    memset(&vInfo, 0, sizeof(vInfo));
    vInfo.dwOSVersionInfoSize = sizeof(vInfo);
    if(!GetVersionEx(&vInfo))
    {
        DWORD dwErr = GetLastError();
        std::cout << "\nERROR : Unable to get Windows version information.\n" << std::endl;
        return SDK_FAILURE;
    }

    if(vInfo.dwMajorVersion >= 6)
    {
		isVistaOrWin7OrWin8 = true;

		if( (6 == vInfo.dwMajorVersion) && (2 == vInfo.dwMinorVersion) )
		{
			isWin8 = true;
		}
	}

    if(!isVistaOrWin7OrWin8)
    {
        std::cout << "Unsupported OS!\n";
        return SDK_SUCCESS;
    }
    SimpleDX10 clSimpleDX10("OpenCL SimpleDX10");
    me = &clSimpleDX10;

    status = clSimpleDX10.initialize();
    if(status != SDK_SUCCESS)
        return status == SDK_EXPECTED_FAILURE ? SDK_SUCCESS :SDK_FAILURE;

    if (clSimpleDX10.parseCommandLine(argc, argv) != SDK_SUCCESS)
        return SDK_FAILURE;


    if(clSimpleDX10.isDumpBinaryEnabled())
    {
        return clSimpleDX10.genBinaryImage();
    }

    status = clSimpleDX10.setup();
    if(status != SDK_SUCCESS)
    {
        return SDK_FAILURE;
    }

    if (clSimpleDX10.run() != SDK_SUCCESS)
        return SDK_FAILURE;

    if (clSimpleDX10.verifyResults() != SDK_SUCCESS)
        return SDK_FAILURE;

    if (clSimpleDX10.cleanup() != SDK_SUCCESS)
        return SDK_FAILURE;

    UnregisterClass(reinterpret_cast<LPCSTR>(WINDOW_CLASS_NAME), wc.hInstance);
    return SDK_SUCCESS;
}