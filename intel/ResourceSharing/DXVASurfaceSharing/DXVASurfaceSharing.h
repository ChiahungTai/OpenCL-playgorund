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

#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <mmsystem.h>
#include <tchar.h>
#include <strsafe.h>
#include <initguid.h>
#include <d3d9.h>
#include <dxva2api.h>
#include <iostream>
#include <string>
#include <vector>
#include "string.h"

#define NO_API_TIMING

#include "../../common/utils.h"
#include "../Common/OCL_Environment.h"

#include <CL/cl_ext.h>


/********************************    OpenCL Global Variables     ****************************************/
// flags for OCL kernel compilation
const char* compileOptions = "-Werror";
cl_uint				num_entries;
cl_platform_id		*platforms;
cl_uint				num_platforms=1;
cl_uint				platform_to_use=0;
cl_uint				device_to_use=0;
cl_int				response;
cl_context			testContext;
cl_command_queue	commandQueue;
cl_mem				memobjs[5];
unsigned char		*a;
cl_kernel			simpleInvertShared;
cl_program			simpleInvertSharedProgram;
cl_kernel			simpleVideoGenKernelShared;
cl_program			simpleVideoGenKernelSharedProgram;
cl_kernel			simpleVideoGaussKernelShared;
cl_program			simpleVideoGaussKernelSharedProgram;

/*********************OpenCL Sharing Related variables ****************************/
HANDLE		nv12TextureSharing;
cl_mem		sharedSurfaceOCLObjects[2];

//New Extensions variables
static clCreateFromDX9MediaSurfaceINTEL_fn clCreateFromDX9MediaSurfaceINTEL;
static clEnqueueAcquireDX9ObjectsINTEL_fn clEnqueueAcquireDX9ObjectsINTEL;
static clEnqueueReleaseDX9ObjectsINTEL_fn clEnqueueReleaseDX9ObjectsINTEL;

//
/******************************** Internal data for DXVA and WINDOWING ************************************/
//

const TCHAR CLASS_NAME[]  = TEXT("DXVA OpenCL D3D9 Surface Sharing");
const TCHAR WINDOW_NAME[] = TEXT("DXVA OpenCL D3D9 Surface Sharing Sample Application");

const UINT VIDEO_MAIN_WIDTH  = 640;
const UINT VIDEO_MAIN_HEIGHT = 480;
const RECT VIDEO_MAIN_RECT   = {0, 0, VIDEO_MAIN_WIDTH, VIDEO_MAIN_HEIGHT};

const UINT VIDEO_FPS     = 60;
const UINT VIDEO_MSPF    = (1000 + VIDEO_FPS / 2) / VIDEO_FPS;
const UINT VIDEO_100NSPF = VIDEO_MSPF * 10000;

const BYTE DEFAULT_PLANAR_ALPHA_VALUE = 0xFF;
const BYTE DEFAULT_PIXEL_ALPHA_VALUE  = 0x80;

const UINT VIDEO_REQUIED_OP = DXVA2_VideoProcess_YUV2RGB |
                              DXVA2_VideoProcess_StretchX |
                              DXVA2_VideoProcess_StretchY;


const D3DFORMAT VIDEO_RENDER_TARGET_FORMAT = D3DFMT_X8R8G8B8;
const D3DFORMAT VIDEO_MAIN_FORMAT = (D3DFORMAT)MAKEFOURCC('N', 'V','1', '2');

const UINT BACK_BUFFER_COUNT = 1;
const UINT SUB_STREAM_COUNT  = 1;
const UINT DWM_BUFFER_COUNT  = 0;


const UINT EX_COLOR_INFO[][2] =
{
    // SDTV ITU-R BT.601 YCbCr to driver's optimal RGB range
    {DXVA2_VideoTransferMatrix_BT601, DXVA2_NominalRange_Unknown},
    // SDTV ITU-R BT.601 YCbCr to studio RGB [16...235]
    {DXVA2_VideoTransferMatrix_BT601, DXVA2_NominalRange_16_235},
    // SDTV ITU-R BT.601 YCbCr to computer RGB [0...255]
    {DXVA2_VideoTransferMatrix_BT601, DXVA2_NominalRange_0_255},
    // HDTV ITU-R BT.709 YCbCr to driver's optimal RGB range
    {DXVA2_VideoTransferMatrix_BT709, DXVA2_NominalRange_Unknown},
    // HDTV ITU-R BT.709 YCbCr to studio RGB [16...235]
    {DXVA2_VideoTransferMatrix_BT709, DXVA2_NominalRange_16_235},
    // HDTV ITU-R BT.709 YCbCr to computer RGB [0...255]
    {DXVA2_VideoTransferMatrix_BT709, DXVA2_NominalRange_0_255}
};

//
/*************************** DXVA/WINDOWING Global variables ****************************************/
//

BOOL ig_bWindowed        = TRUE;
BOOL ig_bTimerSet        = FALSE;
BOOL ig_bInModeChange    = FALSE;
BOOL ig_bDspFrameDrop    = FALSE;

HWND    ig_Hwnd			= NULL;
HANDLE  ig_hTimer		= NULL;

RECT ig_RectWindow		= {0};
DWORD ig_StartSysTime	= 0;
DWORD ig_PreviousTime	= 0;

//Direct3D9 devices
IDirect3D9Ex*        ig_pD3D9		= NULL;
IDirect3DDevice9Ex*  ig_pD3DD9		= NULL;
IDirect3DSurface9* ig_pD3DRT			= NULL;
IDirect3DSurface9* ig_pMainStream	= NULL;	//Shared surface

D3DPRESENT_PARAMETERS ig_D3DPP = {0};

IDirectXVideoProcessorService* ig_pDXVAVPS = NULL;
IDirectXVideoProcessor*        ig_pDXVAVPD = NULL;

GUID                     ig_GuidVP    = {0};
DXVA2_VideoDesc          ig_VideoDesc = {0};
DXVA2_VideoProcessorCaps ig_VPCaps    = {0};

INT ig_BackgroundColor = 0;
INT ig_ExColorInfo     = 0;
INT ig_ProcAmpSteps[4] = {0};

WORD ig_PlanarAlphaValue = DEFAULT_PLANAR_ALPHA_VALUE;
BYTE ig_PixelAlphaValue  = DEFAULT_PIXEL_ALPHA_VALUE;

RECT ig_SrcRect = VIDEO_MAIN_RECT;
RECT ig_DstRect = VIDEO_MAIN_RECT;

UINT ig_TargetWidthPercent  = 100;
UINT ig_TargetHeightPercent = 100;

/*********************************Application control variables ********************************************/
unsigned int scrollOffset = 0;
bool		openCLFirstLoop					= true;
bool		openCLStreamGenerationShared	= false;
bool		hardwareInvertShared			= false;
bool		hardwareGaussShared				= false;
bool		extensionsRegistered			= false;
bool		sharedSurfaceCreated			= false;
const int			widthAmount				= 640;
const int			amount					= widthAmount*480;

//Variables to hold image properties
int			ImageSize;
int			imageWidth;
int			imageHeight;
int			imageBPP;
BYTE*		imageBytes;

//Variables for fps calculations and image generation
bool				backGround		= true;
unsigned long		currentTime		= 0;
unsigned long		previousTime	= 0;
int					num_of_frames;
bool				fps_reporting	= false;
int					originX			= 800;
int					originY			= 800;
bool				moveRight		= true;
bool				moveDown		= true;


/************************Function definitions**********************************************/
	/***********************OpenCL Related ***********************/
	cl_uint				generateVideoStream();
	cl_uint				openCLInvertShared();
	cl_uint				openCLGaussShared();
	cl_uint				createFromSharedSurface();
	cl_uint				openCLStartup();
	cl_uint				openCLDestroy();
	cl_uint				registerOpenCLSharedSurfaceExtensions();

	/***********************DXVA Functions*************************/
	bool				InitializeVideo();
	bool				InitializeDXVA2();
	bool				InitializeD3D9();
	VOID				DestroyD3D9();
	bool				CreateDXVA2VPDevice(REFGUID guid);
	VOID				DestroyDXVA2();
	bool				ResetDevice(bool bChangeWindowMode = FALSE);
	DWORD				GetFrameNumber();
	DXVA2_AYUVSample16	GetBackgroundColor();
	RECT				ScaleRectangle(const RECT& input, const RECT& src, const RECT& dst);
	bool				ProcessVideo();

	/***********************Windowing Related**********************/
	VOID				OnDestroy(HWND hwnd);
	VOID				OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
	VOID				OnPaint(HWND hwnd);
	VOID				OnSize(HWND hwnd, UINT state, int cx, int cy);
	LRESULT CALLBACK	WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	bool				InitializeWindow();
	bool				InitializeTimer();
	VOID				DestroyTimer();
	INT					MessageLoop();
	bool				InitializeModule();
	INT WINAPI			_tWinMain(__in HINSTANCE hInstance,__in_opt HINSTANCE hPrevInstance,__in LPTSTR lpCmdLine,__in INT nCmdShow);

	/*****************Convenience functions ******************/
	void				readBMPImage();
	void				RedirectIOToConsole();
	char*				ReadSources(const char *fileName);