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



#include <WindowsX.h>

#include <D3D10.h>
#include <D3DX10.h>

#include "../../common/utils.h"
#include "../Common/OCL_Environment.h"

#include "FreeCamera.h"
#include "resource.h"

#define WINDOW_WIDTH		512
#define WINDOW_HEIGHT		512

const cl_uint NUM_THREADS		= 1024;
const cl_uint NUM_POINTS		= 1024;
const cl_uint NUM_DIMENSIONS	= 4;
const cl_uint NUM_NEIGHBORS		= 1;
const cl_uint GROUP_SIZE		= NUM_POINTS / NUM_THREADS;
const cl_uint distance_matrix_size = (const cl_uint)((float)(NUM_POINTS * (NUM_POINTS-1)) / 2.0f);

// flags for OCL kernel compilation
const char* sOCLCompileOptions = 
"-Werror"
;

// OCL kernels file name
const char* sProjectionKernelsFileName = "Projection.cl";
const char* sRelativeDistanceErrorKernelsFileName = "RelativeDistanceError.cl";

// Projection kernel function name
const char* sProjectionKernelName = "project_all_points";
const char* sDistancesKernelName = "relative_distances";
const char* sFloatDifferenceKernelName = "float_difference";

#define D3D_SAFE_RELEASE(x)\
{\
	if( x )\
		x->Release();\
}

#define D3D_SAFE_RUN(x) \
{\
	HRESULT hr = x;\
	if(FAILED(hr))\
	{\
		return hr;\
	}\
}

// **************************** OpenCL Globals ****************************
cl_int						g_cl_err;
OCL_Environment				g_cl_Env;
cl_mem						g_cl_SHARED_buf3DPoints;
cl_kernel					g_cl_kernProjection, g_cl_kernDistances, g_clkernFloatDifference;
cl_program					g_cl_progProjection, g_cl_progDistances, g_cl_progFloatDifference;
cl_mem						g_cl_bufNDPoints, g_cl_SHARED_buf3DProjectionAxes, g_cl_bufNDDistances, g_cl_buf3DDistances, g_cl_bufDifference;

OCL_Platform*				g_pcl_default_platform;
OCL_DeviceAndQueue*			g_pcl_default_device;
// **************************** /OpenCL Globals ****************************


// **************************** DirectX Globals ****************************
struct Simple2DVertex
{
	cl_float4	pos;
};

D3D10_DRIVER_TYPE					g_d3dDriverType;

IDXGISwapChain*						g_pd3d_SwapChain = NULL;
ID3D10RenderTargetView*				g_pd3d_RenderTargetView = NULL;
ID3D10Device*						g_pd3d_Device = NULL;

ID3D10Effect*                       g_pd3d_Effect = NULL;
ID3D10EffectTechnique*              g_pd3d_3DParticlesTechnique = NULL;
ID3D10EffectTechnique*              g_pd3d_3DAxesTechnique = NULL;
ID3D10InputLayout*                  g_pd3d_VertexLayout = NULL;
ID3D10Buffer*                       g_pd3d_SHARED_buf3DPoints = NULL;
ID3D10Buffer*						g_pd3d_SHARED_buf3DProjectionAxes = NULL;
ID3D10ShaderResourceView*			g_pd3d_VertexSRV = NULL;
ID3D10Buffer*                       g_pd3d_IndexBuffer = NULL;
ID3D10Resource*						g_pd3d_ResTexture = NULL;
ID3D10ShaderResourceView*           g_pd3d_SRVTexture = NULL;
ID3D10EffectMatrixVariable*         g_pd3d_InvViewVariable = NULL;
ID3D10EffectMatrixVariable*         g_pd3d_WorldViewProjVariable = NULL;
ID3D10EffectShaderResourceVariable* g_pd3d_SRVVariable = NULL;

D3DXMATRIX							g_md3d_World;

CFreeCamera*						g_pd3d_Camera;

// for input
POINT g_PrevPos;
bool g_bDragging = false;
// **************************** /DirectX Globals ****************************


// **************************** Windows Globals ****************************
HINSTANCE					g_hInst;
HWND						g_hWnd;
// **************************** /Windows Globals ****************************



void OCL_RunKernel( cl_command_queue queue, cl_kernel kernel, size_t* globalWorkSize )
{
	cl_event ev;
	OCL_ABORT_ON_ERR( 
		clEnqueueNDRangeKernel(	queue,				// valid device command queue
								kernel,				// compiled kernel
								1,					// work dimensions
								NULL,				// global work offset
								globalWorkSize,		// global work size
								NULL,				// local work size
								0,					// number of events to wait for
								NULL,				// list of events to wait for
								&ev)				// this event
	);           

	OCL_ABORT_ON_ERR( clWaitForEvents(1, &ev) );
	
	//timing
	/* UNCOMMENT FOR TIMING INFORMATION
	cl_ulong end, start;
	OCL_ABORT_ON_ERR( clGetEventProfilingInfo(ev, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, 0) );
	OCL_ABORT_ON_ERR( clGetEventProfilingInfo(ev, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, 0) );

	double timeDiffMilliSec = (double)(end - start) * 1e-6;

	printf("Kernel elapsed time : %ems\n", (double)timeDiffMilliSec );
	*/
	clReleaseEvent(ev);
}

void OCL_InitBuffers()
{
	srand(clock());

	// create input/output buffers
	OCL_ABORT_ON_ERR(( 
		(g_cl_bufNDPoints = createRandomFloatVecBuffer( &g_pcl_default_platform->mContext, CL_MEM_READ_ONLY, sizeof(cl_float)*NUM_DIMENSIONS, NUM_POINTS, &g_cl_err, 1.0f )), 
		g_cl_err ));

	OCL_ABORT_ON_ERR(( 
		(g_cl_bufNDDistances = clCreateBuffer(	g_pcl_default_platform->mContext, CL_MEM_READ_WRITE, sizeof(cl_float)*distance_matrix_size, NULL, &g_cl_err)), 
		g_cl_err ));

	OCL_ABORT_ON_ERR(( 
		(g_cl_buf3DDistances = clCreateBuffer(	g_pcl_default_platform->mContext, CL_MEM_READ_WRITE, sizeof(cl_float)*distance_matrix_size, NULL, &g_cl_err)), 
		g_cl_err ));

	OCL_ABORT_ON_ERR(( 
		(g_cl_bufDifference = clCreateBuffer(	g_pcl_default_platform->mContext, CL_MEM_READ_WRITE, sizeof(cl_float)*distance_matrix_size, NULL, &g_cl_err)), 
		g_cl_err ));
}

void OCL_InitKernels()
{
	OCL_ABORT_ON_ERR((
		g_cl_kernProjection = createKernelFromFile(	&g_pcl_default_platform->mContext,
													g_pcl_default_device,
													sProjectionKernelsFileName, 
													sProjectionKernelName, 
													sOCLCompileOptions, 
													&g_cl_progProjection, 
													&g_cl_err ),
		g_cl_err
	));

	OCL_ABORT_ON_ERR((
		g_cl_kernDistances = createKernelFromFile(	&g_pcl_default_platform->mContext,
													g_pcl_default_device,
													sRelativeDistanceErrorKernelsFileName, 
													sDistancesKernelName, 
													sOCLCompileOptions, 
													&g_cl_progDistances, 
													&g_cl_err ),
		g_cl_err
	));

	OCL_ABORT_ON_ERR((
		g_clkernFloatDifference = createKernelFromFile(	&g_pcl_default_platform->mContext,
													g_pcl_default_device,
													sRelativeDistanceErrorKernelsFileName, 
													sFloatDifferenceKernelName, 
													sOCLCompileOptions, 
													&g_cl_progFloatDifference, 
													&g_cl_err ),
		g_cl_err
	));

}

void OCL_RunKernelProjection( )
{

	// acquire buffers
	OCL_ABORT_ON_ERR( clEnqueueAcquireD3D10ObjectsKHR( g_pcl_default_device->mCmdQueue, 1, &g_cl_SHARED_buf3DProjectionAxes, 0, NULL, NULL ) );
	OCL_ABORT_ON_ERR( clEnqueueAcquireD3D10ObjectsKHR( g_pcl_default_device->mCmdQueue, 1, &g_cl_SHARED_buf3DPoints, 0, NULL, NULL ) );

	//Set up buffer parameters
	OCL_ABORT_ON_ERR( clSetKernelArg(	g_cl_kernProjection, 0, sizeof(cl_mem),	(void *) &g_cl_bufNDPoints			) );
	OCL_ABORT_ON_ERR( clSetKernelArg(	g_cl_kernProjection, 1, sizeof(cl_mem),	(void *) &g_cl_SHARED_buf3DProjectionAxes	) );
	OCL_ABORT_ON_ERR( clSetKernelArg(	g_cl_kernProjection, 2, sizeof(cl_mem),	(void *) &g_cl_SHARED_buf3DPoints	) );
	OCL_ABORT_ON_ERR( clSetKernelArg(	g_cl_kernProjection, 3, sizeof(cl_uint),(void *) &NUM_DIMENSIONS			) );

	size_t global_work_size[] = {NUM_POINTS};
	OCL_RunKernel( g_pcl_default_device->mCmdQueue, g_cl_kernProjection, global_work_size );

	// release buffers
	OCL_ABORT_ON_ERR( clEnqueueReleaseD3D10ObjectsKHR( g_pcl_default_device->mCmdQueue, 1, &g_cl_SHARED_buf3DProjectionAxes, 0, NULL, NULL ) );
	OCL_ABORT_ON_ERR( clEnqueueReleaseD3D10ObjectsKHR( g_pcl_default_device->mCmdQueue, 1, &g_cl_SHARED_buf3DPoints, 0, NULL, NULL ) );
}

void OCL_RunKernelNDDistances( )
{

	//Set up buffer parameters
	OCL_ABORT_ON_ERR( clSetKernelArg(	g_cl_kernDistances, 0, sizeof(cl_mem),		(void *) &g_cl_bufNDPoints	) );
	OCL_ABORT_ON_ERR( clSetKernelArg(	g_cl_kernDistances, 1, sizeof(cl_mem),		(void *) &g_cl_bufNDDistances	) );

	OCL_ABORT_ON_ERR( clSetKernelArg(	g_cl_kernDistances, 2, sizeof(cl_float)*GROUP_SIZE*NUM_DIMENSIONS,	(void *) NULL	) );
		
	OCL_ABORT_ON_ERR( clSetKernelArg(	g_cl_kernDistances, 3, sizeof(cl_uint),	(void *) &NUM_DIMENSIONS		) );
	OCL_ABORT_ON_ERR( clSetKernelArg(	g_cl_kernDistances, 4, sizeof(cl_uint),	(void *) &NUM_POINTS			) );

	size_t global_work_size[] = {NUM_POINTS};
	OCL_RunKernel( g_pcl_default_device->mCmdQueue, g_cl_kernDistances, global_work_size );
}

void OCL_RunKernel3DDistances( )
{

	cl_uint dims3d = 3;

	OCL_ABORT_ON_ERR( clEnqueueAcquireD3D10ObjectsKHR( g_pcl_default_device->mCmdQueue, 1, &g_cl_SHARED_buf3DPoints, 0, NULL, NULL ) );

	//Set up buffer parameters
	OCL_ABORT_ON_ERR( clSetKernelArg(	g_cl_kernDistances, 0, sizeof(cl_mem),		(void *) &g_cl_SHARED_buf3DPoints	) );
	OCL_ABORT_ON_ERR( clSetKernelArg(	g_cl_kernDistances, 1, sizeof(cl_mem),		(void *) &g_cl_buf3DDistances	) );

	OCL_ABORT_ON_ERR( clSetKernelArg(	g_cl_kernDistances, 2, sizeof(cl_float)*GROUP_SIZE*NUM_DIMENSIONS,	(void *) NULL	) );
		
	OCL_ABORT_ON_ERR( clSetKernelArg(	g_cl_kernDistances, 3, sizeof(cl_uint),	(void *) &dims3d		) );
	OCL_ABORT_ON_ERR( clSetKernelArg(	g_cl_kernDistances, 4, sizeof(cl_uint),	(void *) &NUM_POINTS			) );

	size_t global_work_size[] = {NUM_POINTS};
	OCL_RunKernel( g_pcl_default_device->mCmdQueue, g_cl_kernDistances, global_work_size );

	OCL_ABORT_ON_ERR( clEnqueueReleaseD3D10ObjectsKHR( g_pcl_default_device->mCmdQueue, 1, &g_cl_SHARED_buf3DPoints, 0, NULL, NULL ) );
}

void OCL_RunKernelDifference( )
{

	//Set up buffer parameters
	OCL_ABORT_ON_ERR( clSetKernelArg(	g_clkernFloatDifference, 0, sizeof(cl_mem),		(void *) &g_cl_bufNDDistances	) );
	OCL_ABORT_ON_ERR( clSetKernelArg(	g_clkernFloatDifference, 1, sizeof(cl_mem),		(void *) &g_cl_buf3DDistances	) );
	OCL_ABORT_ON_ERR( clSetKernelArg(	g_clkernFloatDifference, 2, sizeof(cl_mem),		(void *) &g_cl_bufDifference	) );
		

	size_t global_work_size[] = {distance_matrix_size};
	OCL_RunKernel( g_pcl_default_device->mCmdQueue, g_clkernFloatDifference, global_work_size );
}

cl_float OCL_ComputeProjectionError_Relative_Distances_Method( )
{
	OCL_RunKernelNDDistances();
	OCL_RunKernel3DDistances();
	OCL_RunKernelDifference();

	cl_float *projected_distance_difference = new cl_float[distance_matrix_size];
	clEnqueueReadBuffer( g_pcl_default_device->mCmdQueue, g_cl_bufDifference, CL_TRUE, 0, sizeof(cl_float)*distance_matrix_size, projected_distance_difference, 0, NULL, NULL );
	clFinish(g_pcl_default_device->mCmdQueue);

	cl_float g_cl_error = 0;
	for(UINT i=0; i<distance_matrix_size; i++)
	{
		g_cl_error += projected_distance_difference[i];
	}
	
	delete projected_distance_difference;

	return g_cl_error;
}

void OCL_Init()
{
	// Initialize opencl environment
	cl_context_properties cps[] =
	{ 
        CL_CONTEXT_D3D10_DEVICE_KHR, 
        (intptr_t)g_pd3d_Device, 
        0 
    };

	OCL_Environment_Desc desc;
	ZeroMemory(&desc,sizeof(desc));

	desc.sPlatformName = "Intel(R) OpenCL";
	desc.ctxProps = cps;
	desc.cmdQueueProps = CL_QUEUE_PROFILING_ENABLE;
	desc.deviceType = CL_DEVICE_TYPE_GPU;

	g_cl_err = g_cl_Env.init(desc);
	
	if(g_cl_err == CL_DEVICE_NOT_FOUND)
	{
		printf("Intel OpenCL-capable GPU not found (or OpenCL drivers are not installed for GPU), exiting!");
		Sleep(5000);
		exit(EXIT_FAILURE);
	}

	// find a platform/device which allows d3d sharing
	bool found = false;
	for(cl_uint p=0; p<g_cl_Env.uiNumPlatforms && !found; p++)
	{
		for(cl_uint d=0; d<g_cl_Env.mpPlatforms[p].uiNumDevices && !found; d++)
		{
			if(strstr(g_cl_Env.mpPlatforms[p].mpDevices[d].sDeviceExtensions,"cl_khr_d3d10_sharing"))
			{
				g_pcl_default_platform = &g_cl_Env.mpPlatforms[p];
				g_pcl_default_device = &g_pcl_default_platform->mpDevices[d];
				found = true;
			}
		}
	}

	if(!found)
	{
		printf("Direct3D 10 buffer sharing support not found, exiting!");
		Sleep(5000);
		exit(EXIT_FAILURE);
	}

	OCL_InitBuffers();
	OCL_InitKernels();
}

HRESULT D3D10_CreateStructuredBuffer( ID3D10Device* pDevice, UINT uElementSize, UINT uCount, VOID* pInitData, ID3D10Buffer** ppBufOut )
{
	*ppBufOut = NULL;

	D3D10_BUFFER_DESC desc;
	ZeroMemory( &desc, sizeof(desc) );
	desc.BindFlags =  D3D10_BIND_VERTEX_BUFFER;
	desc.ByteWidth = uElementSize * uCount;
	//check if this flag required using clGetContextInfo with CL_CONTEXT_D3D10_PREFER_SHARED_RESOURCES_KHR 
	desc.MiscFlags = D3D10_RESOURCE_MISC_SHARED;

	if ( pInitData )
	{
		D3D10_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = pInitData;
		return pDevice->CreateBuffer( &desc, &InitData, ppBufOut );
	} else
		return pDevice->CreateBuffer( &desc, NULL, ppBufOut );
}

HRESULT D3D10_Init()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
    createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

    D3D10_DRIVER_TYPE driverTypes[] =
    {
        D3D10_DRIVER_TYPE_HARDWARE,
        D3D10_DRIVER_TYPE_WARP,
        D3D10_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_d3dDriverType = driverTypes[driverTypeIndex];
        hr = D3D10CreateDeviceAndSwapChain( NULL, g_d3dDriverType, NULL, createDeviceFlags, D3D10_SDK_VERSION, &sd,
                                            &g_pd3d_SwapChain, &g_pd3d_Device );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Create a render target view
    ID3D10Texture2D* pBackBuffer = NULL;
    D3D_SAFE_RUN( g_pd3d_SwapChain->GetBuffer( 0, __uuidof( ID3D10Texture2D ), ( LPVOID* )&pBackBuffer ) );

	D3D_SAFE_RUN( g_pd3d_Device->CreateRenderTargetView( pBackBuffer, NULL, &g_pd3d_RenderTargetView ) );
    pBackBuffer->Release();
	g_pd3d_Device->OMSetRenderTargets( 1, &g_pd3d_RenderTargetView, NULL );

    // Setup the viewport
    D3D10_VIEWPORT vp;
    vp.Width = width;
    vp.Height = height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pd3d_Device->RSSetViewports( 1, &vp );

	// Create the effect
    DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    // Set the D3D10_SHADER_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3D10_SHADER_DEBUG;
#endif

    hr = D3DX10CreateEffectFromFile( L"ParticleDraw.fx", NULL, NULL, "fx_4_0", dwShaderFlags, 0, g_pd3d_Device, NULL, NULL, &g_pd3d_Effect, NULL, NULL );
    if( FAILED( hr ) )
    {
        MessageBox( NULL,
					L"Error loading/compiling shader", 
					L"Shader Load Error", MB_OK );
        return hr;
    }

    // Obtain the techniques
    g_pd3d_3DParticlesTechnique = g_pd3d_Effect->GetTechniqueByName( "RenderParticles" );
	g_pd3d_3DAxesTechnique = g_pd3d_Effect->GetTechniqueByName( "RenderAxes" );

    // Obtain the variables
    g_pd3d_WorldViewProjVariable		= g_pd3d_Effect->GetVariableByName( "g_mWorldViewProj" )->AsMatrix();
	g_pd3d_InvViewVariable				= g_pd3d_Effect->GetVariableByName( "g_mInvView" )->AsMatrix();
	g_pd3d_SRVVariable					= g_pd3d_Effect->GetVariableByName( "g_txDiffuse" )->AsShaderResource();

    // Define the input layout
    D3D10_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION",	0,	DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	0,	D3D10_INPUT_PER_VERTEX_DATA, 0 }
    };
    UINT numElements = sizeof( layout ) / sizeof( layout[0] );

    // Create the input layout
    D3D10_PASS_DESC PassDesc;
    g_pd3d_3DParticlesTechnique->GetPassByIndex( 0 )->GetDesc( &PassDesc );
    D3D_SAFE_RUN( g_pd3d_Device->CreateInputLayout( layout, numElements, PassDesc.pIAInputSignature,
                                          PassDesc.IAInputSignatureSize, &g_pd3d_VertexLayout ) );

    // Set the input layout
    g_pd3d_Device->IASetInputLayout( g_pd3d_VertexLayout );

	// 
	D3D_SAFE_RUN( D3D10_CreateStructuredBuffer( g_pd3d_Device, sizeof( Simple2DVertex ), NUM_POINTS, NULL, &g_pd3d_SHARED_buf3DPoints ) );
	D3D_SAFE_RUN( D3D10_CreateStructuredBuffer( g_pd3d_Device, sizeof( Simple2DVertex ), NUM_DIMENSIONS, NULL, &g_pd3d_SHARED_buf3DProjectionAxes ) );

    // Set primitive topology
    g_pd3d_Device->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_POINTLIST );

    // Load the Texture
	D3D_SAFE_RUN( D3DX10CreateTextureFromFile( g_pd3d_Device, L"particle.dds", NULL, NULL, &g_pd3d_ResTexture, NULL ) );
	D3D_SAFE_RUN( g_pd3d_Device->CreateShaderResourceView( g_pd3d_ResTexture, NULL, &g_pd3d_SRVTexture ) );

	// Set texture
	g_pd3d_SRVVariable->SetResource( g_pd3d_SRVTexture );

	D3DXVECTOR3 mInitEye(0,0,-500);
	g_pd3d_Camera = new CFreeCamera(mInitEye);

    // Initialize the world matrix
    D3DXMatrixIdentity( &g_md3d_World );

    return S_OK;
}

void D3D10_Render()
{
    // Just clear the backbuffer
    float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; //red,green,blue,alpha
    g_pd3d_Device->ClearRenderTargetView( g_pd3d_RenderTargetView, ClearColor );

	const D3DXMATRIX mView = g_pd3d_Camera->getViewMatrix();

	D3DXMATRIX mWorldViewProj = g_md3d_World * mView * g_pd3d_Camera->getProjMatrix();
    g_pd3d_WorldViewProjVariable->SetMatrix( ( float* ) &mWorldViewProj );

	D3DXMATRIX mInvView;
	D3DXMatrixInverse( &mInvView, NULL, &mView );
	g_pd3d_InvViewVariable->SetMatrix( ( float* ) &mInvView );

	UINT stride = sizeof( Simple2DVertex );
	UINT offset = 0;
	g_pd3d_Device->IASetVertexBuffers( 0, 1, &g_pd3d_SHARED_buf3DPoints, &stride, &offset ); 

	// Render the particles
	D3D10_TECHNIQUE_DESC techDesc;
    g_pd3d_3DParticlesTechnique->GetDesc( &techDesc );

    for( UINT p = 0; p < techDesc.Passes; ++p )
    {
        g_pd3d_3DParticlesTechnique->GetPassByIndex( p )->Apply( 0 );
       g_pd3d_Device->Draw( NUM_POINTS, 0 );
    }

	g_pd3d_Device->IASetVertexBuffers( 0, 1, &g_pd3d_SHARED_buf3DProjectionAxes, &stride, &offset ); 

	// Render the axes
    g_pd3d_3DAxesTechnique->GetDesc( &techDesc );

    for( UINT p = 0; p < techDesc.Passes; ++p )
    {
        g_pd3d_3DAxesTechnique->GetPassByIndex( p )->Apply( 0 );
        g_pd3d_Device->Draw( NUM_DIMENSIONS, 0 );
    }
}

void D3D10_DestroyDevice()
{
	g_pd3d_Device->OMSetRenderTargets( 0, NULL, NULL );

	D3D_SAFE_RELEASE( g_pd3d_Effect );

	D3D_SAFE_RELEASE( g_pd3d_VertexLayout );
	D3D_SAFE_RELEASE( g_pd3d_SHARED_buf3DPoints );
	D3D_SAFE_RELEASE( g_pd3d_SHARED_buf3DProjectionAxes );
	D3D_SAFE_RELEASE( g_pd3d_IndexBuffer );
	D3D_SAFE_RELEASE( g_pd3d_ResTexture );
	D3D_SAFE_RELEASE( g_pd3d_SRVTexture );

    D3D_SAFE_RELEASE( g_pd3d_RenderTargetView );
    D3D_SAFE_RELEASE( g_pd3d_SwapChain );
    D3D_SAFE_RELEASE( g_pd3d_Device );
}

void OCL_DestroyDevice()
{
	// destroy kernels/programs
	clReleaseKernel(g_cl_kernProjection);
	clReleaseProgram(g_cl_progProjection);
	clReleaseKernel(g_cl_kernDistances);
	clReleaseProgram(g_cl_progDistances);
	clReleaseKernel(g_clkernFloatDifference);
	clReleaseProgram(g_cl_progFloatDifference);

	// destroy buffers
	clReleaseMemObject(g_cl_bufNDPoints);
	clReleaseMemObject(g_cl_bufNDDistances);
	clReleaseMemObject(g_cl_buf3DDistances);
	clReleaseMemObject(g_cl_bufDifference);
	clReleaseMemObject(g_cl_SHARED_buf3DProjectionAxes);
	clReleaseMemObject(g_cl_SHARED_buf3DPoints);

	g_cl_Env.destroy();

	g_pcl_default_platform = NULL;
	g_pcl_default_device = NULL;
}

void D3D10_OCL_InitInterop()
{
	cl_int g_cl_err;

	OCL_ABORT_ON_ERR((
		g_cl_SHARED_buf3DPoints = clCreateFromD3D10BufferKHR(	g_pcl_default_platform->mContext,
																CL_MEM_READ_WRITE,
																g_pd3d_SHARED_buf3DPoints,
																&g_cl_err), g_cl_err
	));

	OCL_ABORT_ON_ERR((
		g_cl_SHARED_buf3DProjectionAxes = clCreateFromD3D10BufferKHR(	g_pcl_default_platform->mContext,
																		CL_MEM_READ_WRITE,
																		g_pd3d_SHARED_buf3DProjectionAxes,
																		&g_cl_err), g_cl_err
	));

	// acquire buffer
	OCL_ABORT_ON_ERR( clEnqueueAcquireD3D10ObjectsKHR( g_pcl_default_device->mCmdQueue, 1, &g_cl_SHARED_buf3DProjectionAxes, 0, NULL, NULL ) );

	fillRandomFloatVecBuffer( &g_pcl_default_device->mCmdQueue, &g_cl_SHARED_buf3DProjectionAxes, sizeof(cl_float4), NUM_DIMENSIONS, NULL, 50.0f );

	// release buffer
	OCL_ABORT_ON_ERR( clEnqueueReleaseD3D10ObjectsKHR( g_pcl_default_device->mCmdQueue, 1, &g_cl_SHARED_buf3DProjectionAxes, 0, NULL, NULL ) );
}

void ProcessKeyboardInput()
{
	// Keyboard Translation
	SHORT sWkey = GetKeyState(0x57) >> 8;
	SHORT sAkey = GetKeyState(0x41) >> 8;
	SHORT sDkey = GetKeyState(0x44) >> 8;
	SHORT sSkey = GetKeyState(0x53) >> 8;

	SHORT sSHIFTkey = GetKeyState(0x10) >> 8;

	D3DXVECTOR3 vTrans( (float) (sAkey ? 0.1f : 0.0f) + (sDkey ? -0.1f : 0.0f), 0.0f, (float) (sWkey ? 0.1f : 0.0f) + (sSkey ? -0.1f : 0.0f) );
	vTrans = sSHIFTkey ? vTrans*10.0f : vTrans;

	g_pd3d_Camera->translateLocal( vTrans );
}

LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
        case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

		case WM_CHAR:
			if(wParam==VK_ESCAPE)
				PostQuitMessage( 0 );
            break;

		case WM_MOUSEMOVE:
			if( g_bDragging )
			{
				g_pd3d_Camera->rotate(	0.01f*(GET_X_LPARAM(lParam) - g_PrevPos.x),
									0.01f*(GET_Y_LPARAM(lParam) - g_PrevPos.y) );
				g_PrevPos.x = GET_X_LPARAM(lParam);
				g_PrevPos.y = GET_Y_LPARAM(lParam); 
			}
			break;

		case WM_LBUTTONDOWN:
			g_PrevPos.x = GET_X_LPARAM(lParam);
			g_PrevPos.y = GET_Y_LPARAM(lParam); 
			g_bDragging = true;
			break;

		case WM_MOUSELEAVE:
		case WM_LBUTTONUP:
			g_bDragging = false;
			break;

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}

HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"WindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"WindowClass", L"Direct3D10 OpenCL Interoperation Demonstration", WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
                           NULL );
    if( !g_hWnd )
        return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{

	if(AllocConsole())
	{
		freopen("CONOUT$", "wt", stdout);

		SetConsoleTitle(L"Debug Console");
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
	}

	// Open up a window
	D3D_SAFE_RUN(InitWindow( hInstance, nCmdShow ));

	// Initialize D3D10
	if( FAILED( D3D10_Init() ) )
    {
        D3D10_DestroyDevice();
        return 0;
    }

	// Create/compile kernels
	OCL_Init();

	// Initialize shared textures
	D3D10_OCL_InitInterop();

	// Static counter for output
	static bool firsttime = true;

	// Print directions
	printf("Direct3D 10 Buffer Sharing for Multidimensional Projection\n");
	printf("Direct3D 10 window displays particles which are transformed in OpenCL using a shared buffer. The transformation projects high-dimensional points into 3-dimensional space (default 4, can be changed by altering NUM_DIMENSIONS macro in code.\n");
	printf("Keys:\n");
	printf("\tMouse left button + drag for rotation\n");
	printf("\tW, A, D, S for movement\n");
	printf("\tHold Shift to move faster\n");

    // Main message loop
    MSG msg = {0};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
			ProcessKeyboardInput();
            OCL_RunKernelProjection();
			D3D10_Render();
			g_pd3d_SwapChain->Present( 0, 0 );

			if(firsttime)
			{			
				cl_float error = OCL_ComputeProjectionError_Relative_Distances_Method();
				printf("Projection Error (total squared distances): %f\n", error);
				firsttime = false;
			}
        }
    }

	// Destroy ocl
	printf("Destroy ocl\n");
	OCL_DestroyDevice();

	// Destroy d3d
	printf("Destroy d3d\n");
    D3D10_DestroyDevice();

	return ( int )msg.wParam;
}

