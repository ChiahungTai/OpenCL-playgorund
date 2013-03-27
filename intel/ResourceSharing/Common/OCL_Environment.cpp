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


#pragma once 

#include "OCL_Environment.h"



cl_int OCL_DeviceAndQueue::init(cl_context ctx, cl_device_id id)
{
	cl_int err;
	mID = id;

	mContext = ctx;

	OCL_RETURN_ON_ERR((
		mCmdQueue = clCreateCommandQueue( ctx, mID, 0, &err),err 
	));

	size_t extStringSize;
	OCL_RETURN_ON_ERR( clGetDeviceInfo(mID,CL_DEVICE_IMAGE_SUPPORT,sizeof(mbImageSupport),&mbImageSupport,NULL) );
	OCL_RETURN_ON_ERR( clGetDeviceInfo(mID,CL_DEVICE_EXTENSIONS,0,NULL,&extStringSize) );

	sDeviceExtensions = new char[extStringSize];
	OCL_RETURN_ON_ERR( clGetDeviceInfo(mID,CL_DEVICE_EXTENSIONS,extStringSize,sDeviceExtensions,NULL) );

	return CL_SUCCESS;
}

cl_int OCL_DeviceAndQueue::init(cl_context ctx, cl_device_id id, cl_command_queue_properties cmdProps)
{
	cl_int err;
	mID = id;

	mContext = ctx;

	OCL_RETURN_ON_ERR((
		mCmdQueue = clCreateCommandQueue( ctx, mID, cmdProps, &err),err 
	));

	size_t nameSize;
	OCL_RETURN_ON_ERR( clGetDeviceInfo( mID, CL_DEVICE_NAME, 0, NULL, &nameSize ) );

	sDeviceName = new char[nameSize];
	OCL_RETURN_ON_ERR( clGetDeviceInfo( mID, CL_DEVICE_NAME, nameSize, sDeviceName, NULL ) );

	size_t extStringSize;
	OCL_RETURN_ON_ERR( clGetDeviceInfo(mID,CL_DEVICE_IMAGE_SUPPORT,sizeof(mbImageSupport),&mbImageSupport,NULL) );
	OCL_RETURN_ON_ERR( clGetDeviceInfo(mID,CL_DEVICE_EXTENSIONS,0,NULL,&extStringSize) );

	sDeviceExtensions = new char[extStringSize];
	OCL_RETURN_ON_ERR( clGetDeviceInfo(mID,CL_DEVICE_EXTENSIONS,extStringSize,sDeviceExtensions,NULL) );
	
	return CL_SUCCESS;
}

cl_int OCL_DeviceAndQueue::destroy()
{
	if( mCmdQueue )
	{
		OCL_RETURN_ON_ERR(clFinish(mCmdQueue));
		OCL_RETURN_ON_ERR( clReleaseCommandQueue(mCmdQueue) );
	}

	delete [] sDeviceExtensions;
	delete [] sDeviceName;

	return CL_SUCCESS;
}

cl_int OCL_Platform::init(cl_platform_id id)
{
	cl_int err;
	mID = id;

	// store name
	size_t nameSize;
	OCL_RETURN_ON_ERR( clGetPlatformInfo( mID, CL_PLATFORM_NAME, 0, NULL, &nameSize ) );

	sPlatformName = new char[nameSize];
	OCL_RETURN_ON_ERR( clGetPlatformInfo( mID, CL_PLATFORM_NAME, nameSize, sPlatformName, NULL ) );

	// store extensions
	size_t extensionsSize;
	OCL_RETURN_ON_ERR( clGetPlatformInfo( mID, CL_PLATFORM_EXTENSIONS, 0, NULL, &extensionsSize ) );

	sPlatformExtensions = new char[extensionsSize];
	OCL_RETURN_ON_ERR( clGetPlatformInfo( mID, CL_PLATFORM_EXTENSIONS, extensionsSize, sPlatformExtensions, NULL ) );

	// default device type 
	cl_device_type deviceType = CL_DEVICE_TYPE_ALL;

	cl_context_properties ctxProps[] = 
	{
		CL_CONTEXT_PLATFORM, 
		(cl_context_properties)mID, 
		0 
	};

	OCL_RETURN_ON_ERR( ( mContext = clCreateContextFromType( ctxProps, deviceType, NULL, NULL, &err ), err));

	// Create array of devices
	size_t dev_ids_size;
	OCL_RETURN_ON_ERR( clGetContextInfo( mContext, CL_CONTEXT_DEVICES, 0, NULL, &dev_ids_size ) );

	uiNumDevices = (cl_uint)dev_ids_size/sizeof(cl_device_id);
	cl_device_id* pDeviceIDs = (cl_device_id*)calloc(1,dev_ids_size);

	mpDevices = new OCL_DeviceAndQueue[uiNumDevices];

	OCL_RETURN_ON_ERR( clGetContextInfo( mContext, CL_CONTEXT_DEVICES, dev_ids_size, pDeviceIDs, NULL ) );

	for( cl_uint j=0; j<uiNumDevices; j++ )
	{
		OCL_RETURN_ON_ERR( mpDevices[j].init( mContext, pDeviceIDs[j] ) );
		OCL_RETURN_ON_ERR( 
			(mpDevices[j].mContext = clCreateContextFromType( ctxProps, deviceType, NULL, NULL, &err )
			,err ));
	}

	delete pDeviceIDs;

	return CL_SUCCESS;
}

#define EXT_DECLARE(_name) _name##_fn m_##_name
#define EXT_INIT(_name) m_##_name = (_name##_fn) clGetExtensionFunctionAddress(#_name);
cl_int OCL_Platform::init(cl_platform_id id, OCL_Environment_Desc desc)
{
	cl_int err;
	mID = id;

	// store name
	size_t nameSize;
	OCL_RETURN_ON_ERR(clGetPlatformInfo( mID, CL_PLATFORM_NAME, 0, NULL, &nameSize ) );

	sPlatformName = new char[nameSize];
	OCL_RETURN_ON_ERR(clGetPlatformInfo( mID, CL_PLATFORM_NAME, nameSize, sPlatformName, NULL ) );

	// store extensions
	size_t extensionsSize;
	OCL_RETURN_ON_ERR(clGetPlatformInfo( mID, CL_PLATFORM_EXTENSIONS, 0, NULL, &extensionsSize ) );

	sPlatformExtensions = new char[extensionsSize];
	OCL_RETURN_ON_ERR( clGetPlatformInfo( mID, CL_PLATFORM_EXTENSIONS, extensionsSize, sPlatformExtensions, NULL ) );

	if(!desc.deviceType)
		desc.deviceType = CL_DEVICE_TYPE_ALL;

	const int MAX_DEVICES = 16; //guessing there are no more than 16 devices per platform
	cl_device_id devices[MAX_DEVICES];
	
    {// detect if D3D9 sharing is able for platform.
        char    str[1024*4];
        size_t  len = 0;
        OCL_RETURN_ON_ERR(clGetPlatformInfo(mID, CL_PLATFORM_EXTENSIONS, sizeof(str),str,&len));
        printf("Platform extensions: %s\n",str);
        if(NULL==strstr(str,"cl_intel_dx9_media_sharing"))
        {
            desc.intel_dx9_media_sharing = false;
			printf("Can't find cl_intel_dx9_media_sharing extension\n");
			assert(0);
        }
    }

	if(desc.intel_dx9_media_sharing)
	{// get device and create context for DXVA sharing
		EXT_DECLARE(clGetDeviceIDsFromDX9INTEL);
		EXT_INIT(clGetDeviceIDsFromDX9INTEL);
		err =  m_clGetDeviceIDsFromDX9INTEL(
			id,
			CL_D3D9EX_DEVICE_INTEL,
			&desc.pD3DD9,
			CL_ALL_DEVICES_FOR_DX9_INTEL,
			sizeof(devices)/sizeof(devices[0]), devices, &uiNumDevices);
		if(err == CL_DEVICE_NOT_FOUND)
		{
			//no device with device type found, 
			mContext = NULL;
			uiNumDevices = 0;
			mpDevices = NULL;
			return CL_SUCCESS;
		}
		//else
		OCL_RETURN_ON_ERR(err);

		///////////////////////////
		cl_uint j = 0;
		for(cl_uint i=0;i<uiNumDevices;i++)
		{//keep only req type
			cl_device_type type;
			err = clGetDeviceInfo(devices[i],CL_DEVICE_TYPE,sizeof(cl_device_type),&type,NULL);
			if(type != desc.deviceType) continue;
			devices[j++] = devices[i];
		}
		uiNumDevices = j;
		////////////////////////////

		if(!desc.ctxProps)
			desc.ctxProps = new cl_context_properties(0);


		cl_context_properties ctxProps[] = {CL_CONTEXT_D3D9EX_DEVICE_INTEL,(cl_context_properties)desc.pD3DD9,NULL};

		if(!desc.devOnlyContext)
			OCL_RETURN_ON_ERR( ( mContext = clCreateContext( ctxProps, uiNumDevices, devices, NULL, NULL, &err ), err));


		// Create array of devices
		mpDevices = new OCL_DeviceAndQueue[uiNumDevices];


		for( cl_uint j=0; j<uiNumDevices; j++ )
		{
			cl_context devcontext = clCreateContext( ctxProps, 1, devices+j, NULL, NULL, &err);
			OCL_RETURN_ON_ERR(err);
			if(desc.devOnlyContext)
			{
				OCL_RETURN_ON_ERR( mpDevices[j].init( devcontext, devices[j], desc.cmdQueueProps ) );
			}
			else
			{
				OCL_RETURN_ON_ERR( mpDevices[j].init( mContext, devices[j], desc.cmdQueueProps ) );
				mpDevices[j].mContext=devcontext;
			}
		}

	}
	else
	{// create context without D3Device connection


		//get the number of devices with the proper deviceType
		err = clGetDeviceIDs(id, desc.deviceType, MAX_DEVICES, devices, &uiNumDevices);
		if(err == CL_DEVICE_NOT_FOUND)
		{
			//no device with device type found, 
			mContext = NULL;
			uiNumDevices = 0;
			mpDevices = NULL;
			return CL_SUCCESS;
		}
		//else
		OCL_RETURN_ON_ERR(err);

		if(!desc.ctxProps)
			desc.ctxProps = new cl_context_properties(0);

		// up to 4 context properties input
		cl_context_properties ctxProps[] = 
		{
			CL_CONTEXT_PLATFORM, 
			(cl_context_properties)mID,
			desc.ctxProps[0],
			desc.ctxProps[1],
			desc.ctxProps[2],
			desc.ctxProps[3],
			0
		};

		if(!desc.devOnlyContext)
			OCL_RETURN_ON_ERR( ( mContext = clCreateContextFromType( ctxProps, desc.deviceType, NULL, NULL, &err ), err));


		// Create array of devices
		mpDevices = new OCL_DeviceAndQueue[uiNumDevices];


		for( cl_uint j=0; j<uiNumDevices; j++ )
		{
			cl_context devcontext = clCreateContext( ctxProps, 1, devices+j, NULL, NULL, &err);
			OCL_RETURN_ON_ERR(err);
			if(desc.devOnlyContext)
			{
				OCL_RETURN_ON_ERR( mpDevices[j].init( devcontext, devices[j], desc.cmdQueueProps ) );
			}
			else
			{
				OCL_RETURN_ON_ERR( mpDevices[j].init( mContext, devices[j], desc.cmdQueueProps ) );
				mpDevices[j].mContext=devcontext;
			}
		}
	}

	
	return CL_SUCCESS;
}



cl_int OCL_Platform::destroy()
{
	for( cl_uint i=0; i<uiNumDevices; i++ )
	{
		OCL_RETURN_ON_ERR( mpDevices[i].destroy() );
	}

	if(mpDevices) delete [] mpDevices;

	if(mContext)OCL_RETURN_ON_ERR(clReleaseContext( mContext ));

	delete [] sPlatformName;
	delete [] sPlatformExtensions;

	return CL_SUCCESS;
}

cl_int OCL_Environment::init()
{
	OCL_LoadLibrary();

	// Get number of  available platforms
	OCL_RETURN_ON_ERR( clGetPlatformIDs( 0, NULL, &uiNumPlatforms ) );

	if( !(uiNumPlatforms > 0) )
	{
		printf("No available platform!");
		abort();
	}

	// Create platforms
	cl_platform_id*	pPlatformIDs = (cl_platform_id*)calloc( sizeof(cl_platform_id), uiNumPlatforms );
	
	OCL_RETURN_ON_ERR( clGetPlatformIDs( uiNumPlatforms, pPlatformIDs, NULL ) );

	// Create one OCL_Platform for each advertised platform
	mpPlatforms = new OCL_Platform[uiNumPlatforms];
	for( cl_uint i=0; i<uiNumPlatforms; i++ )
	{
		OCL_RETURN_ON_ERR( mpPlatforms[i].init(pPlatformIDs[i]) );
	}

	delete pPlatformIDs;
	
	return CL_SUCCESS;
}

cl_int OCL_Environment::init(OCL_Environment_Desc desc)
{

	OCL_LoadLibrary();

	// Get number of  available platforms
	OCL_RETURN_ON_ERR( clGetPlatformIDs( 0, NULL, &uiNumPlatforms ) );

	if( !(uiNumPlatforms > 0) )
	{
		printf("No available platform!");
		abort();
	}

	// Create platforms
	cl_platform_id*	pPlatformIDs = (cl_platform_id*)calloc( sizeof(cl_platform_id), uiNumPlatforms );
	
	OCL_RETURN_ON_ERR( clGetPlatformIDs( uiNumPlatforms, pPlatformIDs, NULL ) );

	// Alloc OCL_Platforms
	if( desc.sPlatformName )
		mpPlatforms = new OCL_Platform[1];
	else
		mpPlatforms = new OCL_Platform[uiNumPlatforms];
	
	bool founddev = false;

	for( cl_uint i=0; i<uiNumPlatforms; i++ )
	{
		char* sPlatname;
		size_t nameSize;
		OCL_RETURN_ON_ERR( clGetPlatformInfo( pPlatformIDs[i], CL_PLATFORM_NAME, 0, NULL, &nameSize ) );

		sPlatname = new char[nameSize];
		OCL_RETURN_ON_ERR( clGetPlatformInfo( pPlatformIDs[i], CL_PLATFORM_NAME, nameSize, sPlatname, NULL ) );

		if( desc.sPlatformName )
		{
			if( strcmp(sPlatname,desc.sPlatformName ) == 0 )
			{
				uiNumPlatforms = 1;
				OCL_RETURN_ON_ERR( mpPlatforms[0].init(pPlatformIDs[i],desc) );
                founddev = true;
                break;
			}
		}
		else
		{
			OCL_RETURN_ON_ERR( mpPlatforms[i].init(pPlatformIDs[i],desc) );
			if(mpPlatforms[i].uiNumDevices > 0)
				founddev = true;//at least 1 compatible device found
		}
	}

	delete pPlatformIDs;
	//if no device found return the error
	if(!founddev)
		OCL_RETURN_ON_ERR(CL_DEVICE_NOT_FOUND);

	return CL_SUCCESS;
}



// OCL D3D10 interop functions
typedef CL_API_ENTRY cl_int (CL_API_CALL *clGetDeviceIDsFromD3D10KHRFuncType)(
    cl_platform_id             platform,
    cl_d3d10_device_source_khr d3d_device_source,
    void *                     d3d_object,
    cl_d3d10_device_set_khr    d3d_device_set,
    cl_uint                    num_entries,
    cl_device_id *             devices,
    cl_uint *                  num_devices) CL_API_SUFFIX__VERSION_1_0;
clGetDeviceIDsFromD3D10KHRFuncType clGetDeviceIDsFromD3D10KHRFunc;

typedef CL_API_ENTRY cl_mem (CL_API_CALL *clCreateFromD3D10BufferKHRFuncType)(
    cl_context     context,
    cl_mem_flags   flags,
    ID3D10Buffer * resource,
    cl_int *       errcode_ret) CL_API_SUFFIX__VERSION_1_0;
clCreateFromD3D10BufferKHRFuncType clCreateFromD3D10BufferKHRFunc;

typedef CL_API_ENTRY cl_mem (CL_API_CALL *clCreateFromD3D10Texture2DKHRFuncType)(
    cl_context        context,
    cl_mem_flags      flags,
    ID3D10Texture2D * resource,
    UINT              subresource,
    cl_int *          errcode_ret) CL_API_SUFFIX__VERSION_1_0;
clCreateFromD3D10Texture2DKHRFuncType clCreateFromD3D10Texture2DKHRFunc;

typedef CL_API_ENTRY cl_mem (CL_API_CALL *clCreateFromD3D10Texture3DKHRFuncType)(
    cl_context        context,
    cl_mem_flags      flags,
    ID3D10Texture3D * resource,
    UINT              subresource,
    cl_int *          errcode_ret) CL_API_SUFFIX__VERSION_1_0;
clCreateFromD3D10Texture3DKHRFuncType clCreateFromD3D10Texture3DKHRFunc;

typedef CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueAcquireD3D10ObjectsKHRFuncType)(
    cl_command_queue command_queue,
    cl_uint          num_objects,
    const cl_mem *   mem_objects,
    cl_uint          num_events_in_wait_list,
    const cl_event * event_wait_list,
    cl_event *       event) CL_API_SUFFIX__VERSION_1_0;
clEnqueueAcquireD3D10ObjectsKHRFuncType clEnqueueAcquireD3D10ObjectsKHRFunc;

typedef CL_API_ENTRY cl_int (CL_API_CALL *clEnqueueReleaseD3D10ObjectsKHRFuncType)(
    cl_command_queue command_queue,
    cl_uint          num_objects,
    cl_mem *         mem_objects,
    cl_uint          num_events_in_wait_list,
    const cl_event * event_wait_list,
    cl_event *       event) CL_API_SUFFIX__VERSION_1_0;
clEnqueueReleaseD3D10ObjectsKHRFuncType clEnqueueReleaseD3D10ObjectsKHRFunc;

int OCL_Environment::OCL_LoadLibrary()
{

	// EXTENSIONS
	// OCL D3D10 interop functions
	clGetDeviceIDsFromD3D10KHRFunc = (clGetDeviceIDsFromD3D10KHRFuncType) clGetExtensionFunctionAddress("clGetDeviceIDsFromD3D10KHR");
	clCreateFromD3D10BufferKHRFunc = (clCreateFromD3D10BufferKHRFuncType) clGetExtensionFunctionAddress("clCreateFromD3D10BufferKHR");
	clCreateFromD3D10Texture2DKHRFunc = (clCreateFromD3D10Texture2DKHRFuncType) clGetExtensionFunctionAddress("clCreateFromD3D10Texture2DKHR");
	clCreateFromD3D10Texture3DKHRFunc = (clCreateFromD3D10Texture3DKHRFuncType) clGetExtensionFunctionAddress("clCreateFromD3D10Texture3DKHR");
	clEnqueueAcquireD3D10ObjectsKHRFunc = (clEnqueueAcquireD3D10ObjectsKHRFuncType) clGetExtensionFunctionAddress("clEnqueueAcquireD3D10ObjectsKHR");
	clEnqueueReleaseD3D10ObjectsKHRFunc = (clEnqueueReleaseD3D10ObjectsKHRFuncType) clGetExtensionFunctionAddress("clEnqueueReleaseD3D10ObjectsKHR");

	return 0;
}

void OCL_Environment::OCL_UnloadLibrary()
{
}

//wrapper functions

// OCL D3D10 interop functions
CL_API_ENTRY cl_int CL_API_CALL clGetDeviceIDsFromD3D10KHR(
    cl_platform_id             platform,
    cl_d3d10_device_source_khr d3d_device_source,
    void *                     d3d_object,
    cl_d3d10_device_set_khr    d3d_device_set,
    cl_uint                    num_entries,
    cl_device_id *             devices,
    cl_uint *                  num_devices) 
{
	return clGetDeviceIDsFromD3D10KHRFunc(platform,d3d_device_source,d3d_object,d3d_device_set,num_entries,devices,num_devices);
}

CL_API_ENTRY cl_mem CL_API_CALL clCreateFromD3D10BufferKHR(
    cl_context     context,
    cl_mem_flags   flags,
    ID3D10Buffer * resource,
    cl_int *       errcode_ret) 
{
	return clCreateFromD3D10BufferKHRFunc(context,flags,resource,errcode_ret);
}

CL_API_ENTRY cl_mem CL_API_CALL clCreateFromD3D10Texture2DKHR(
    cl_context        context,
    cl_mem_flags      flags,
    ID3D10Texture2D * resource,
    UINT              subresource,
    cl_int *          errcode_ret) 
{
	return clCreateFromD3D10Texture2DKHRFunc(context,flags,resource,subresource,errcode_ret);
}

CL_API_ENTRY cl_mem CL_API_CALL clCreateFromD3D10Texture3DKHR(
    cl_context        context,
    cl_mem_flags      flags,
    ID3D10Texture3D * resource,
    UINT              subresource,
    cl_int *          errcode_ret)
{
	return clCreateFromD3D10Texture3DKHRFunc(context,flags,resource,subresource,errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueAcquireD3D10ObjectsKHR(
    cl_command_queue command_queue,
    cl_uint          num_objects,
    const cl_mem *   mem_objects,
    cl_uint          num_events_in_wait_list,
    const cl_event * event_wait_list,
    cl_event *       event)
{
	return clEnqueueAcquireD3D10ObjectsKHRFunc(command_queue,num_objects,mem_objects,num_events_in_wait_list,event_wait_list,event);
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueReleaseD3D10ObjectsKHR(
    cl_command_queue command_queue,
    cl_uint          num_objects,
    cl_mem *         mem_objects,
    cl_uint          num_events_in_wait_list,
    const cl_event * event_wait_list,
    cl_event *       event) 
{
	return clEnqueueReleaseD3D10ObjectsKHRFunc(command_queue,num_objects,mem_objects,num_events_in_wait_list,event_wait_list,event);
}

cl_int OCL_Environment::destroy()
{
	for(cl_uint i=0; i<uiNumPlatforms; i++)
		OCL_RETURN_ON_ERR(mpPlatforms[i].destroy());

	delete [] mpPlatforms;

	OCL_UnloadLibrary();
	
	//reset everything to 0/NULL
	memset(this,0,sizeof(OCL_Environment));

	return CL_SUCCESS;
}
