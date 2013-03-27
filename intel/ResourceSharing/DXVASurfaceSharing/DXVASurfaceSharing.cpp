// Copyright (c) 2009-2011 Intel Corporation
// All rights reserved.
// 
// WARRANTY DISCLAIMER
// 
// THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
// MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Intel Corporation is the author of the Materials, and requests that all
// problem reports or change requests be submitted to it directly


/*

Module Name:

    OpenCL_DXVA_Surface_Sharing.cpp

Abstract:

    This sample code demonstrates OpenCL D3D9 NV12 surface sharing with DXVA

Keyboard assignment:

    F1:Invert controls
	F2:Gauss controls
	F7:enable/disable FPS

*/
#pragma warning(disable : 4995)

#include "DXVASurfaceSharing.h"

//Used for console output
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>
//
// Helper inline functions.
//
inline BOOL operator != (const DXVA2_ValueRange& x, const DXVA2_ValueRange& y)
{
    return memcmp(&x, &y, sizeof(x)) ? TRUE : FALSE;
}


OCL_Environment cl_env;

//
// Debug Macro
//
#if defined(DBG) || defined(DEBUG) || defined(_DEBUG)

#define DBGMSG(x)  {DbgPrint(TEXT("%s(%u) : "), TEXT(__FILE__), __LINE__); DbgPrint x;}


VOID DbgPrint(PCTSTR format, ...)
{
    va_list args;
    va_start(args, format);

    TCHAR string[MAX_PATH];

    if (SUCCEEDED(StringCbVPrintf(string, sizeof(string), format, args)))
    {
        OutputDebugString(string);
    }
    else
    {
        DebugBreak();
    }
}

#else   // DBG || DEBUG || _DEBUG

#define DBGMSG(x)

#endif  // DBG || DEBUG || _DEBUG

bool InitializeD3D9()
{
    HRESULT hr;

    hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &ig_pD3D9);

    if (FAILED(hr))
    {
        DBGMSG((TEXT("Direct3DCreate9 failed.\n")));
        return FALSE;
    }

	//Device properties
    ig_D3DPP.BackBufferWidth				= 0;
    ig_D3DPP.BackBufferHeight			= 0;
    ig_D3DPP.BackBufferFormat           = VIDEO_RENDER_TARGET_FORMAT;
    ig_D3DPP.BackBufferCount            = BACK_BUFFER_COUNT;
    ig_D3DPP.SwapEffect                 = D3DSWAPEFFECT_DISCARD;
    ig_D3DPP.hDeviceWindow              = ig_Hwnd;
    ig_D3DPP.Windowed                   = ig_bWindowed;
    ig_D3DPP.Flags                      = D3DPRESENTFLAG_VIDEO;
    ig_D3DPP.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    ig_D3DPP.PresentationInterval       = D3DPRESENT_INTERVAL_ONE;

	//Try to create a hardware D3D9 device.
    hr = ig_pD3D9->CreateDeviceEx(D3DADAPTER_DEFAULT,
                                D3DDEVTYPE_HAL,
                                ig_Hwnd,
                                D3DCREATE_FPU_PRESERVE |
                                D3DCREATE_MULTITHREADED |
                                D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                &ig_D3DPP,
                                NULL,
                                &ig_pD3DD9);

    if (FAILED(hr))
    {
        DBGMSG((TEXT("CreateDevice(HAL) failed with error 0x%x.\n"), hr));
    }

    if (!ig_pD3DD9)
    {
        return FALSE;
    }

    return TRUE;
}

void DestroyD3D9()
{
    if (ig_pD3DD9)
    {
        ig_pD3DD9->Release();
        ig_pD3DD9 = NULL;
    }

    if (ig_pD3D9)
    {
        ig_pD3D9->Release();
        ig_pD3D9 = NULL;
    }
}

bool CreateDXVA2VPDevice(REFGUID guid)
{

	HRESULT hr;

    //
    // Query the supported render target format.
    //
    UINT i, count;
    D3DFORMAT* formats = NULL;

    hr = ig_pDXVAVPS->GetVideoProcessorRenderTargets(guid,
                                                    &ig_VideoDesc,
                                                    &count,
                                                    &formats);

    if (FAILED(hr))
    {
        DBGMSG((TEXT("GetVideoProcessorRenderTargets failed with error 0x%x.\n"), hr));
        return FALSE;
    }

    for (i = 0; i < count; i++)
    {
        if (formats[i] == VIDEO_RENDER_TARGET_FORMAT)
        {
            break;
        }
    }

    CoTaskMemFree(formats);

    if (i >= count)
    {
        DBGMSG((TEXT("GetVideoProcessorRenderTargets doesn't support that format.\n")));
        return FALSE;
    }

    //
    // Query video processor capabilities.
    //
    hr = ig_pDXVAVPS->GetVideoProcessorCaps(guid,
                                           &ig_VideoDesc,
                                           VIDEO_RENDER_TARGET_FORMAT,
                                           &ig_VPCaps);

    if (FAILED(hr))
    {
        DBGMSG((TEXT("GetVideoProcessorCaps failed with error 0x%x.\n"), hr));
        return FALSE;
    }

    //
    // This is a progressive device and we cannot provide any reference sample.
    //
    if (ig_VPCaps.NumForwardRefSamples > 0 || ig_VPCaps.NumBackwardRefSamples > 0)
    {
        DBGMSG((TEXT("NumForwardRefSamples or NumBackwardRefSamples is greater than 0.\n")));
        return FALSE;
    }

    //
    // Check to see if the device supports all the VP operations we want.
    //
    if ((ig_VPCaps.VideoProcessorOperations & VIDEO_REQUIED_OP) != VIDEO_REQUIED_OP)
    {
        DBGMSG((TEXT("The DXVA2 device doesn't support the VP operations.\n")));
        return FALSE;
    }

    //
    // Create a main stream surface. This is a shared surface 
    //
    hr = ig_pDXVAVPS->CreateSurface(VIDEO_MAIN_WIDTH,
                                   VIDEO_MAIN_HEIGHT,
                                   0,
                                   VIDEO_MAIN_FORMAT,
                                   D3DPOOL_DEFAULT,
                                   0,
                                   DXVA2_VideoProcessorRenderTarget,
                                   &ig_pMainStream,
                                   &nv12TextureSharing);

    if (FAILED(hr))
    {
        DBGMSG((TEXT("CreateSurface(MainStream) failed with error 0x%x.\n"), hr));
        return FALSE;
    }

    //
    // Finally create a video processor device.
    //
    hr = ig_pDXVAVPS->CreateVideoProcessor(guid,
                                          &ig_VideoDesc,
                                          VIDEO_RENDER_TARGET_FORMAT,
                                          SUB_STREAM_COUNT,
                                          &ig_pDXVAVPD);

    if (FAILED(hr))
    {
        DBGMSG((TEXT("CreateVideoProcessor failed with error 0x%x.\n"), hr));
        return FALSE;
    }

    ig_GuidVP = guid;

    return TRUE;
}

//initializes video stream
bool InitializeVideo()
{
    
	//Generates a stream of video frames
	generateVideoStream();

    return TRUE;
}

//Read in a background image to use for video stream
void readBMPImage()
{
	HANDLE hBitMap = LoadImage(0, TEXT("test.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    
    BITMAP bitmap;
    GetObject(hBitMap,sizeof(BITMAP),&bitmap);
  
    ImageSize = bitmap.bmHeight*bitmap.bmWidth*bitmap.bmBitsPixel/8;

	imageWidth = bitmap.bmWidth;
	imageHeight = bitmap.bmHeight;
	imageBPP = bitmap.bmBitsPixel;  

	std::cout << "Image Width:" << imageWidth << std::endl << "Image Height:" << imageHeight << std::endl << "Image BPP:" << imageBPP << std::endl;

	imageBytes = NULL;
    imageBytes = new BYTE[ ImageSize ];
    
    GetBitmapBits((HBITMAP)hBitMap,ImageSize,imageBytes );
	DeleteObject(hBitMap);
}

cl_uint generateVideoStream()
{
	/* Setup openCL, creates context, kernels, command queue */
	if(openCLFirstLoop)
		openCLStartup();

	//Reads in the background image the first time through the application
	if(backGround)
	{
		readBMPImage();
		backGround=false;
	}

	UINT dx = VIDEO_MAIN_WIDTH/2;
	UINT dy = VIDEO_MAIN_HEIGHT/2;


	//Simulates panning over a larger image to generate a video stream
	if(moveRight)
	{
		originX++;
		if(originX >= 1920-320)
			moveRight=false;
	}
	else
	{
		originX--;
		if(originX <= 0)
			moveRight=true;
	}

	if(moveDown)
	{
		originY++;
		if(originY >= 1080-240)
			moveDown=false;
	}
	else
	{
		originY--;
		if(originY <= 0)
			moveDown=true;
	}

	//Uses loaded test image
	cl_uint err=0;
	
	int *orX = new int;
	int *orY = new int;
	*orX = originX;
	*orY = originY;

	cl_int errCode=0;
		
	//Register extensions if not done yet, only necessary on first pass
	if(!extensionsRegistered)
		registerOpenCLSharedSurfaceExtensions();
		

	if(!sharedSurfaceCreated)
		createFromSharedSurface();

	//Aquire D3D9 Object for OpenCL
	OCL_RETURN_ON_ERR(clEnqueueAcquireDX9ObjectsINTEL(commandQueue,2,sharedSurfaceOCLObjects,0,NULL,NULL));

	//Create openCL buffers
	OCL_RETURN_ON_ERR((memobjs[0] = clCreateBuffer(testContext,CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,sizeof(char)*ImageSize,imageBytes,NULL),err));	
	OCL_RETURN_ON_ERR((memobjs[2] = clCreateBuffer(testContext,CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,sizeof(int),orX,NULL),err));
	OCL_RETURN_ON_ERR((memobjs[3] = clCreateBuffer(testContext,CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,sizeof(int),orY,NULL),err));

	//Set kernel arguments
	OCL_RETURN_ON_ERR(clSetKernelArg(simpleVideoGenKernelShared,0,sizeof(cl_mem), (void *) &memobjs[0]));	
	OCL_RETURN_ON_ERR(clSetKernelArg(simpleVideoGenKernelShared,1,sizeof(cl_mem), (void *) &sharedSurfaceOCLObjects[0]));
	OCL_RETURN_ON_ERR(clSetKernelArg(simpleVideoGenKernelShared,2,sizeof(cl_mem), (void *) &sharedSurfaceOCLObjects[1]));
	OCL_RETURN_ON_ERR(clSetKernelArg(simpleVideoGenKernelShared,3,sizeof(cl_mem), (void *) &memobjs[2]));
	OCL_RETURN_ON_ERR(clSetKernelArg(simpleVideoGenKernelShared,4,sizeof(cl_mem), (void *) &memobjs[3]));

	// set work-item dimensions
	size_t				szGlobalWorkSize[2];
	size_t				szLocalWorkSize[2];
    
	szLocalWorkSize[0] = 16;
	szLocalWorkSize[1] = 16;
	szGlobalWorkSize[0] = 640;
	szGlobalWorkSize[1] = 480;

	// execute kernel
	OCL_RETURN_ON_ERR(clEnqueueNDRangeKernel(commandQueue, simpleVideoGenKernelShared, 2, NULL, szGlobalWorkSize, szLocalWorkSize, 0, NULL, NULL));

	//Release D3D9 Objects from openCL
	OCL_RETURN_ON_ERR(clEnqueueReleaseDX9ObjectsINTEL(commandQueue,2,sharedSurfaceOCLObjects,0,NULL,NULL));
	OCL_RETURN_ON_ERR(clFinish(commandQueue));
	//Free openCL memory objects
	OCL_RETURN_ON_ERR(clReleaseMemObject(memobjs[0]));
	OCL_RETURN_ON_ERR(clReleaseMemObject(memobjs[2]));
	OCL_RETURN_ON_ERR(clReleaseMemObject(memobjs[3]));
	
	delete orX;
	delete orY;
	return CL_SUCCESS;
}

bool InitializeDXVA2()
{
    HRESULT hr;

    //
    // Retrieve a back buffer as the video render target.
    //
    hr = ig_pD3DD9->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &ig_pD3DRT);

    if (FAILED(hr))
    {
        DBGMSG((TEXT("GetBackBuffer failed with error 0x%x.\n"), hr));
        return FALSE;
    }

    //
    // Create DXVA2 Video Processor Service.
    //
    hr = DXVA2CreateVideoService(ig_pD3DD9,
                                 IID_IDirectXVideoProcessorService,
                                 (VOID**)&ig_pDXVAVPS);

    if (FAILED(hr))
    {
        DBGMSG((TEXT("DXVA2CreateVideoService failed with error 0x%x.\n"), hr));
        return FALSE;
    }

    //
    // Initialize the video descriptor.
    //
    ig_VideoDesc.SampleWidth                         = VIDEO_MAIN_WIDTH;
    ig_VideoDesc.SampleHeight                        = VIDEO_MAIN_HEIGHT;
    ig_VideoDesc.SampleFormat.VideoChromaSubsampling = DXVA2_VideoChromaSubsampling_MPEG2;
    ig_VideoDesc.SampleFormat.NominalRange           = DXVA2_NominalRange_16_235;
    ig_VideoDesc.SampleFormat.VideoTransferMatrix    = EX_COLOR_INFO[ig_ExColorInfo][0];
    ig_VideoDesc.SampleFormat.VideoLighting          = DXVA2_VideoLighting_dim;
    ig_VideoDesc.SampleFormat.VideoPrimaries         = DXVA2_VideoPrimaries_BT709;
    ig_VideoDesc.SampleFormat.VideoTransferFunction  = DXVA2_VideoTransFunc_709;
    ig_VideoDesc.SampleFormat.SampleFormat           = DXVA2_SampleProgressiveFrame;
    ig_VideoDesc.Format                              = VIDEO_MAIN_FORMAT;
    ig_VideoDesc.InputSampleFreq.Numerator           = VIDEO_FPS;
    ig_VideoDesc.InputSampleFreq.Denominator         = 1;
    ig_VideoDesc.OutputFrameFreq.Numerator           = VIDEO_FPS;
    ig_VideoDesc.OutputFrameFreq.Denominator         = 1;

    //
    // Query the video processor GUID.
    //
    UINT count;
    GUID* guids = NULL;

    hr = ig_pDXVAVPS->GetVideoProcessorDeviceGuids(&ig_VideoDesc, &count, &guids);

    if (FAILED(hr))
    {
        DBGMSG((TEXT("GetVideoProcessorDeviceGuids failed with error 0x%x.\n"), hr));
        return FALSE;
    }

    //
    // Create a DXVA2 device.
    //
    for (UINT i = 0; i < count; i++)
    {
        if (CreateDXVA2VPDevice(guids[i]))
        {
            break;
        }
    }

    CoTaskMemFree(guids);

    if (!ig_pDXVAVPD)
    {
        DBGMSG((TEXT("Failed to create a DXVA2 device.\n")));
        return FALSE;
    }

    if (!InitializeVideo())
    {
        return FALSE;
    }

    return TRUE;
}

void DestroyDXVA2()
{
    if (ig_pMainStream)
    {
        ig_pMainStream->Release();
        ig_pMainStream = NULL;
    }

    if (ig_pDXVAVPD)
    {
        ig_pDXVAVPD->Release();
        ig_pDXVAVPD = NULL;
    }

    if (ig_pDXVAVPS)
    {
        ig_pDXVAVPS->Release();
        ig_pDXVAVPS = NULL;
    }

    if (ig_pD3DRT)
    {
        ig_pD3DRT->Release();
        ig_pD3DRT = NULL;
    }
	if(imageBytes)
	{
		delete [] imageBytes;
	}
	if(sharedSurfaceOCLObjects[0])
	{
		clReleaseMemObject(sharedSurfaceOCLObjects[0]);
	}
	if(sharedSurfaceOCLObjects[1])
	{
		clReleaseMemObject(sharedSurfaceOCLObjects[1]);
	}


}

bool ResetDevice(bool bChangeWindowMode)
{
    HRESULT hr;

    if (ig_pD3DD9)
    {
        //
        // Destroy DXVA2 device because it may be holding any D3D9 resources.
        //
        DestroyDXVA2();

        ig_D3DPP.BackBufferWidth  = 0;
        ig_D3DPP.BackBufferHeight = 0;
        ig_D3DPP.Windowed = ig_bWindowed;

        //
        // Reset will change the parameters, so use a copy instead.
        //
        D3DPRESENT_PARAMETERS d3dpp = ig_D3DPP;

        hr = ig_pD3DD9->Reset(&d3dpp);

        if (FAILED(hr))
        {
            DBGMSG((TEXT("Reset failed with error 0x%x.\n"), hr));
        }

        if (SUCCEEDED(hr) && InitializeDXVA2())
        {
            return TRUE;
        }

        //
        // If either Reset didn't work or failed to initialize DXVA2 device,
        // try to recover by recreating the devices from the scratch.
        //
        DestroyDXVA2();
        DestroyD3D9();
    }

    if (InitializeD3D9() && InitializeDXVA2())
    {
        return TRUE;
    }

    //
    // Fallback to Window mode, if failed to initialize Fullscreen mode.
    //
    if (ig_bWindowed)
    {
        return FALSE;
    }

    DestroyDXVA2();
    DestroyD3D9();

    ig_bWindowed = TRUE;

    if (InitializeD3D9() && InitializeDXVA2())
    {
        return TRUE;
    }

    return FALSE;
}

DWORD GetFrameNumber()
{
    DWORD currentTime;
    DWORD currentSysTime = timeGetTime();

    if (ig_StartSysTime > currentSysTime)
    {
        currentTime = currentSysTime + (0xFFFFFFFF - ig_StartSysTime);
    }
    else
    {
        currentTime = currentSysTime - ig_StartSysTime;
    }

    DWORD frame = currentTime / VIDEO_MSPF;
    DWORD delta = (currentTime - ig_PreviousTime) / VIDEO_MSPF;

    if (delta > 1)
    {
        if (ig_bDspFrameDrop)
        {
            DBGMSG((TEXT("Frame dropped %u frame(s).\n"), delta - 1));
        }
    }

    if (delta > 0)
    {
        ig_PreviousTime = currentTime;
    }

    return frame;
}

DXVA2_AYUVSample16 GetBackgroundColor()
{
    DXVA2_AYUVSample16 color;
	color.Cr    = 128 * 0x100;
    color.Cb    = 128 * 0x100;
    color.Y     = 0 * 0x100;
    color.Alpha = 0xFFFF;

    return color;
}

RECT ScaleRectangle(const RECT& input, const RECT& src, const RECT& dst)
{
    RECT rect;

    UINT src_dx = src.right - src.left;
    UINT src_dy = src.bottom - src.top;

    UINT dst_dx = dst.right - dst.left;
    UINT dst_dy = dst.bottom - dst.top;

    //
    // Scale input rectangle within src rectangle to dst rectangle.
    //
    rect.left   = input.left   * dst_dx / src_dx;
    rect.right  = input.right  * dst_dx / src_dx;
    rect.top    = input.top    * dst_dy / src_dy;
    rect.bottom = input.bottom * dst_dy / src_dy;

    return rect;
}

bool ProcessVideo()
{
	//Generate Next Video Frame;
	InitializeVideo();

    HRESULT hr;

    if (!ig_pD3DD9)
    {
        return FALSE;
    }

    RECT rect;
    GetClientRect(ig_Hwnd, &rect);

    if (IsRectEmpty(&rect))
    {
        return TRUE;
    }

	//Apply filter(s) 
	if (hardwareInvertShared)
	{
		openCLInvertShared();
	}
		
	if (hardwareGaussShared)
	{
		openCLGaussShared();
	}
		
    //
    // Check the current status of D3D9 device.
    //
    hr = ig_pD3DD9->TestCooperativeLevel();

    switch (hr)
    {
    case D3D_OK :
        break;

    case D3DERR_DEVICELOST :
        DBGMSG((TEXT("TestCooperativeLevel returned D3DERR_DEVICELOST.\n")));
        return TRUE;

    case D3DERR_DEVICENOTRESET :
        DBGMSG((TEXT("TestCooperativeLevel returned D3DERR_DEVICENOTRESET.\n")));

        if (!ResetDevice())
        {
            return FALSE;
        }

        break;

    default :
        DBGMSG((TEXT("TestCooperativeLevel failed with error 0x%x.\n"), hr));
        return FALSE;
    }

    DXVA2_VideoProcessBltParams blt = {0};
    DXVA2_VideoSample samples[1] = {0};

    DWORD frame = GetFrameNumber();

    LONGLONG start_100ns = frame * LONGLONG(VIDEO_100NSPF);
    LONGLONG end_100ns   = start_100ns + LONGLONG(VIDEO_100NSPF);

    RECT client;
    GetClientRect(ig_Hwnd, &client);

    RECT target;

    target.left   = client.left   + (client.right  - client.left) / 2 * (100 - ig_TargetWidthPercent)  / 100;
    target.right  = client.right  - (client.right  - client.left) / 2 * (100 - ig_TargetWidthPercent)  / 100;
    target.top    = client.top    + (client.bottom - client.top)  / 2 * (100 - ig_TargetHeightPercent) / 100;
    target.bottom = client.bottom - (client.bottom - client.top)  / 2 * (100 - ig_TargetHeightPercent) / 100;

    //
    // Initialize VPBlt parameters.
    //
    blt.TargetFrame = start_100ns;
    blt.TargetRect  = target;

    // DXVA2_VideoProcess_Constriction
    blt.ConstrictionSize.cx = target.right - target.left;
    blt.ConstrictionSize.cy = target.bottom - target.top;

    blt.BackgroundColor = GetBackgroundColor();

    //
    // Initialize main stream video sample.
    //
    samples[0].Start = start_100ns;
    samples[0].End   = end_100ns;

    samples[0].SampleFormat.SampleFormat = DXVA2_SampleProgressiveFrame;

    samples[0].SrcSurface = ig_pMainStream;

    // DXVA2_VideoProcess_SubRects
    samples[0].SrcRect = ig_SrcRect;

    // DXVA2_VideoProcess_StretchX, Y
    samples[0].DstRect = ScaleRectangle(ig_DstRect, VIDEO_MAIN_RECT, client);

    // DXVA2_VideoProcess_PlanarAlpha
    samples[0].PlanarAlpha = DXVA2FloatToFixed(float(ig_PlanarAlphaValue) / 0xFF);

    hr = ig_pDXVAVPD->VideoProcessBlt(ig_pD3DRT,
                                     &blt,
                                     samples,
                                     SUB_STREAM_COUNT,
                                     NULL);

    if (FAILED(hr))
    {
        DBGMSG((TEXT("VideoProcessBlt failed with error 0x%x.\n"), hr));
    }

    hr = ig_pD3DD9->Present(NULL, NULL, NULL, NULL);

    if (FAILED(hr))
    {
        DBGMSG((TEXT("Present failed with error 0x%x.\n"), hr));
    }

	//Calculate frame Rate
	num_of_frames++;
	currentTime = timeGetTime();
	if(currentTime - previousTime >= 3000)
	{
		previousTime = currentTime;
		if(fps_reporting)
			std::cout << "Current FPS: " << num_of_frames/3 << std::endl;
		num_of_frames=0;
	}

    return TRUE;
}

void OnDestroy(HWND hwnd)
{
    PostQuitMessage(0);
}


void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    if (!fDown)
    {
        return;
    }

    if (vk == VK_ESCAPE)
    {
        DestroyWindow(hwnd);
        return;
    }

    if (!ig_pD3DD9)
    {
        return;
    }

    switch (vk)
    {
    case VK_F1 :
		{
			if(hardwareInvertShared)
			{
				hardwareInvertShared=false;
				std::cout << "Invert disabled" << std::endl;
			}
			else
			{
				hardwareInvertShared=true;
				std::cout << "Invert enabled" << std::endl;
			}

			break;
		}
    case VK_F2 :
		{
			if(hardwareGaussShared)
			{
				hardwareGaussShared=false;
				std::cout << "Gaussian Filter disabled" << std::endl;
			}
			else
			{
				hardwareGaussShared=true;
				std::cout << "Gaussian Filter enabled" << std::endl;
			}
			break;
		}
    case VK_F3 :
		{

			break;
		}
    case VK_F4 :
		{
			break;

		}
    case VK_F5 :
		{

			break;		
		}
    case VK_F6 :
		{
			break;
		}
    case VK_F7 :
		{
			fps_reporting = !fps_reporting;
			break;
		}
    case VK_F8 :
		{

			break;
		}
    case VK_F9 :

        break;

    case VK_HOME :
        
        break;

    case VK_END :

        break;

    case VK_UP :
    case VK_RIGHT :
        
        break;

    case VK_DOWN :
    case VK_LEFT :
        
        break;
    }
}

void OnPaint(HWND hwnd)
{
    if (!ig_pD3D9) return;
    ValidateRect(hwnd , NULL);
    ProcessVideo();
}

void OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    if (!ig_pD3DD9)
    {
        return;
    }

    RECT rect;
    GetClientRect(hwnd, &rect);

    if (IsRectEmpty(&rect))
    {
        return;
    }

    //
    // Do not reset the device while the mode change is in progress.
    //
    if (ig_bInModeChange)
    {
        return;
    }

    if (!ResetDevice())
    {
        DestroyWindow(hwnd);
        return;
    }

    InvalidateRect(hwnd , NULL , FALSE);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
        HANDLE_MSG(hwnd, WM_PAINT,   OnPaint);
        HANDLE_MSG(hwnd, WM_SIZE,    OnSize);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

//initializes window
bool InitializeWindow()
{
    WNDCLASS wc = {0};

    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClass(&wc))
    {
        DBGMSG((TEXT("RegisterClass failed with error %d.\n"), GetLastError()));
        return FALSE;
    }

    //
    // Start in Window mode regardless of the initial mode.
    //
    ig_Hwnd = CreateWindow(CLASS_NAME,
                          WINDOW_NAME,
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          NULL,
                          NULL,
                          GetModuleHandle(NULL),
                          NULL);

    if (!ig_Hwnd)
    {
        DBGMSG((TEXT("CreateWindow failed with error %d.\n"), GetLastError()));
        return FALSE;
    }

    ShowWindow(ig_Hwnd, SW_SHOWDEFAULT);
    UpdateWindow(ig_Hwnd);

    return TRUE;
}

bool InitializeTimer()
{
    ig_hTimer = CreateWaitableTimer(NULL, FALSE, NULL);

    if (!ig_hTimer)
    {
        DBGMSG((TEXT("CreateWaitableTimer failed with error %d.\n"), GetLastError()));
        return FALSE;
    }

    LARGE_INTEGER li = {0};

    if (!SetWaitableTimer(ig_hTimer,
                          &li,
                          VIDEO_MSPF,
                          NULL,
                          NULL,
                          FALSE))
    {
        DBGMSG((TEXT("SetWaitableTimer failed with error %d.\n"), GetLastError()));
        return FALSE;
    }

    ig_bTimerSet = (timeBeginPeriod(1) == TIMERR_NOERROR);

    ig_StartSysTime = timeGetTime();

    return TRUE;
}

void DestroyTimer()
{
    if (ig_bTimerSet)
    {
        timeEndPeriod(1);
        ig_bTimerSet = FALSE;
    }

    if (ig_hTimer)
    {
        CloseHandle(ig_hTimer);
        ig_hTimer = NULL;
    }
}

int MessageLoop()
{
    MSG msg = {0};

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {

            TranslateMessage(&msg);
            DispatchMessage(&msg);

            continue;
        }

        //
        // Wait until the timer expires or any message is posted.
        //
        if (MsgWaitForMultipleObjects(1,
                                      &ig_hTimer,
                                      FALSE,
                                      INFINITE,
                                      QS_ALLINPUT) == WAIT_OBJECT_0)
        {
            if (!ProcessVideo())
            {
                DestroyWindow(ig_Hwnd);
            }
        }
    }

    return INT(msg.wParam);
}


void RedirectIOToConsole()
{
	int hConHandle;

	long lStdHandle;

	CONSOLE_SCREEN_BUFFER_INFO coninfo;

	FILE *fp;

	// allocate a console for this app

	AllocConsole();

	// set the screen buffer to be big enough to let us scroll text

	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),

	&coninfo);

	coninfo.dwSize.Y = 1000;

	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE),

	coninfo.dwSize);

	// redirect unbuffered STDOUT to the console

	lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);

	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);

	fp = _fdopen( hConHandle, "w" );

	*stdout = *fp;

	setvbuf( stdout, NULL, _IONBF, 0 );

	// redirect unbuffered STDIN to the console

	lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);

	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);

	fp = _fdopen( hConHandle, "r" );

	*stdin = *fp;

	setvbuf( stdin, NULL, _IONBF, 0 );

	// redirect unbuffered STDERR to the console

	lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);

	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);

	fp = _fdopen( hConHandle, "w" );

	*stderr = *fp;

	setvbuf( stderr, NULL, _IONBF, 0 );

	// make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog

	// point to console as well

	//ios::sync_with_stdio();

}

cl_uint openCLStartup()
{
	//Gets platform information and displays it
	cl_int err;

	// initialize opencl environment (platforms, devices, command queues)

	OCL_Environment_Desc desc;
	ZeroMemory(&desc,sizeof(desc));

	//Gets GPU devices
	desc.sPlatformName = "Intel(R) OpenCL";
	desc.deviceType = CL_DEVICE_TYPE_GPU; 
	desc.cmdQueueProps = CL_QUEUE_PROFILING_ENABLE;
	
	//Set context properties telling cl we will be sharing with a D3D9EX device
	cl_context_properties properties[10];
	properties[0] = CL_CONTEXT_D3D9EX_DEVICE_INTEL;
	properties[1] = (cl_context_properties)ig_pD3DD9;
	properties[2] = NULL;

	desc.ctxProps = properties;

	desc.intel_dx9_media_sharing = true;
	desc.pD3DD9= ig_pD3DD9; 
	cl_env.init(desc);
	num_platforms = cl_env.uiNumPlatforms;

	//Display platform/device
	std::cout << "Platform:" << cl_env.mpPlatforms[platform_to_use].sPlatformName << std::endl << cl_env.mpPlatforms[platform_to_use].sPlatformExtensions << std::endl;
	std::cout << "Platform Devices:" << cl_env.mpPlatforms[platform_to_use].mpDevices[device_to_use].sDeviceName << std::endl << cl_env.mpPlatforms[platform_to_use].mpDevices[device_to_use].sDeviceExtensions << std::endl;

	// initialize timing variables
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	commandQueue = cl_env.mpPlatforms[platform_to_use].mpDevices[device_to_use].mCmdQueue;
	testContext = cl_env.mpPlatforms[platform_to_use].mContext;

	//Creates kernel from file
	OCL_RETURN_ON_ERR((
				simpleInvertShared = createKernelFromFile(&cl_env.mpPlatforms[platform_to_use].mContext, 
													&cl_env.mpPlatforms[platform_to_use].mpDevices[device_to_use], 
													"simple_invert_shared.cl", 
													"simple_invert_shared", 
													compileOptions, 
													&simpleInvertSharedProgram, 
													&err ),
			err
			));

	OCL_RETURN_ON_ERR((
				simpleVideoGenKernelShared = createKernelFromFile(&cl_env.mpPlatforms[platform_to_use].mContext, 
													&cl_env.mpPlatforms[platform_to_use].mpDevices[device_to_use], 
													"simple_write_shared.cl", 
													"simple_write_shared", 
													compileOptions, 
													&simpleVideoGenKernelSharedProgram, 
													&err ),
			err
			));

	OCL_RETURN_ON_ERR((
				simpleVideoGaussKernelShared = createKernelFromFile(&cl_env.mpPlatforms[platform_to_use].mContext, 
													&cl_env.mpPlatforms[platform_to_use].mpDevices[device_to_use], 
													"simple_gauss_shared.cl", 
													"simple_gauss_shared", 
													compileOptions, 
													&simpleVideoGaussKernelSharedProgram, 
													&err ),
			err
			));

	openCLFirstLoop=false;
	return CL_SUCCESS;
}

cl_uint openCLDestroy()
{

    if( simpleVideoGaussKernelShared ) {OCL_RETURN_ON_ERR(clReleaseKernel( simpleVideoGaussKernelShared ));  simpleVideoGaussKernelShared = NULL;}
    if( simpleVideoGaussKernelSharedProgram ) {OCL_RETURN_ON_ERR(clReleaseProgram( simpleVideoGaussKernelSharedProgram ));  simpleVideoGaussKernelSharedProgram = NULL;}
    if( simpleVideoGenKernelShared ) {OCL_RETURN_ON_ERR(clReleaseKernel( simpleVideoGenKernelShared ));  simpleVideoGenKernelShared = NULL;}
    if( simpleVideoGenKernelSharedProgram ) {OCL_RETURN_ON_ERR(clReleaseProgram( simpleVideoGenKernelSharedProgram ));  simpleVideoGenKernelSharedProgram = NULL;}
    if( simpleInvertShared ) {OCL_RETURN_ON_ERR(clReleaseKernel(simpleInvertShared ));  simpleInvertShared = NULL;}
    if( simpleInvertSharedProgram ) {OCL_RETURN_ON_ERR(clReleaseProgram( simpleInvertSharedProgram ));  simpleInvertSharedProgram = NULL;}
    ///if( commandQueue ) {clReleaseCommandQueue( commandQueue );  commandQueue = NULL;}
    ///if( testContext ) {clReleaseContext( testContext );  testContext = NULL;}
	
	return CL_SUCCESS;
}

cl_uint registerOpenCLSharedSurfaceExtensions() //Function gets extension function addresses
{
	//Function to create shared surface from DXVA surface
	clCreateFromDX9MediaSurfaceINTEL = 
                                (clCreateFromDX9MediaSurfaceINTEL_fn) clGetExtensionFunctionAddress("clCreateFromDX9MediaSurfaceINTEL");

	//Aquires object for use by opencl
	clEnqueueAcquireDX9ObjectsINTEL = 
                                (clEnqueueAcquireDX9ObjectsINTEL_fn) clGetExtensionFunctionAddress("clEnqueueAcquireDX9ObjectsINTEL");

	//Releases the object back to dxva
	clEnqueueReleaseDX9ObjectsINTEL = 
                                (clEnqueueReleaseDX9ObjectsINTEL_fn) clGetExtensionFunctionAddress("clEnqueueReleaseDX9ObjectsINTEL");

	
	extensionsRegistered = true;
	return CL_SUCCESS;
}
cl_uint openCLInvertShared()
{		

	if(!sharedSurfaceCreated) //Creates the shared surfaces if they have not been created already
		createFromSharedSurface();

	//Aquire D3D9 Object for OpenCL
	OCL_RETURN_ON_ERR( clEnqueueAcquireDX9ObjectsINTEL(commandQueue,2,sharedSurfaceOCLObjects,0,NULL,NULL));

	//Set kernel arguments
	OCL_RETURN_ON_ERR(clSetKernelArg(simpleInvertShared,0,sizeof(cl_mem), (void *) &sharedSurfaceOCLObjects[0]));	
	OCL_RETURN_ON_ERR(clSetKernelArg(simpleInvertShared,1,sizeof(cl_mem), (void *) &sharedSurfaceOCLObjects[0]));
	OCL_RETURN_ON_ERR(clSetKernelArg(simpleInvertShared,2,sizeof(cl_mem), (void *) &sharedSurfaceOCLObjects[1]));
	OCL_RETURN_ON_ERR(clSetKernelArg(simpleInvertShared,3,sizeof(cl_mem), (void *) &sharedSurfaceOCLObjects[1]));

	// set work-item dimensions
    size_t				szGlobalWorkSize[2];
    size_t				szLocalWorkSize[2];
    
    szLocalWorkSize[0] = 16;
    szLocalWorkSize[1] = 16;
    szGlobalWorkSize[0] = 640;
    szGlobalWorkSize[1] = 480;

	// execute kernel
	OCL_RETURN_ON_ERR(clEnqueueNDRangeKernel(commandQueue, simpleInvertShared, 2, NULL, szGlobalWorkSize, szLocalWorkSize, 0, NULL, NULL));

	//Release D3D9 Objects from openCL
	OCL_RETURN_ON_ERR(clEnqueueReleaseDX9ObjectsINTEL(commandQueue,2,sharedSurfaceOCLObjects,0,NULL,NULL));
	OCL_RETURN_ON_ERR(clFinish(commandQueue));
	

	openCLFirstLoop=false;
	return CL_SUCCESS;
}
cl_uint openCLGaussShared()
{		

	if(!sharedSurfaceCreated) //Creates the shared surfaces if they have not been created already
		createFromSharedSurface();

	//Aquire D3D9 Object for OpenCL
	OCL_RETURN_ON_ERR(clEnqueueAcquireDX9ObjectsINTEL(commandQueue,2,sharedSurfaceOCLObjects,0,NULL,NULL));

	//Set kernel arguments
	OCL_RETURN_ON_ERR(clSetKernelArg(simpleVideoGaussKernelShared,0,sizeof(cl_mem), (void *) &sharedSurfaceOCLObjects[0]));	
	OCL_RETURN_ON_ERR(clSetKernelArg(simpleVideoGaussKernelShared,1,sizeof(cl_mem), (void *) &sharedSurfaceOCLObjects[0]));
	OCL_RETURN_ON_ERR(clSetKernelArg(simpleVideoGaussKernelShared,2,sizeof(cl_mem), (void *) &sharedSurfaceOCLObjects[1]));
	OCL_RETURN_ON_ERR(clSetKernelArg(simpleVideoGaussKernelShared,3,sizeof(cl_mem), (void *) &sharedSurfaceOCLObjects[1]));

	// set work-item dimensions
    size_t				szGlobalWorkSize[2];
    size_t				szLocalWorkSize[2];
    
    szLocalWorkSize[0] = 16;
    szLocalWorkSize[1] = 16;
    szGlobalWorkSize[0] = 640;
    szGlobalWorkSize[1] = 480;

	// execute kernel
	OCL_RETURN_ON_ERR(clEnqueueNDRangeKernel(commandQueue, simpleVideoGaussKernelShared, 2, NULL, szGlobalWorkSize, szLocalWorkSize, 0, NULL, NULL));

	//Release D3D9 Objects from openCL
	OCL_RETURN_ON_ERR(clEnqueueReleaseDX9ObjectsINTEL(commandQueue,2,sharedSurfaceOCLObjects,0,NULL,NULL));
	OCL_RETURN_ON_ERR(clFinish(commandQueue));
	
	openCLFirstLoop=false;
	return CL_SUCCESS;
}

cl_uint createFromSharedSurface()
{
	cl_int errCode;

	//Create cl_mem object from shared DXVA Surface, NV12 format is used so surface is split into 2 planes
	//Y plane
	OCL_RETURN_ON_ERR(( sharedSurfaceOCLObjects[0] = clCreateFromDX9MediaSurfaceINTEL(testContext,0,ig_pMainStream,nv12TextureSharing,0,&errCode),errCode));
	//UV plane
	OCL_RETURN_ON_ERR((sharedSurfaceOCLObjects[1] = clCreateFromDX9MediaSurfaceINTEL(testContext,0,ig_pMainStream,nv12TextureSharing,1,&errCode),errCode));

	sharedSurfaceCreated=true;
	return CL_SUCCESS;
}

bool InitializeModule()
{
	//Create Console Window;
	#ifdef _DEBUG
		RedirectIOToConsole();
	#endif

    return TRUE;
}


int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    INT wParam = 0;

    if (InitializeModule() &&
        InitializeWindow() &&
        InitializeD3D9() &&
        InitializeDXVA2() &&
        InitializeTimer())
    {
        wParam = MessageLoop();
    }

    DestroyTimer();
    DestroyDXVA2();
    DestroyD3D9();

	openCLDestroy();
	cl_env.destroy();

    return wParam;
}

