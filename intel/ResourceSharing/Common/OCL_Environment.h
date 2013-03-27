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
 * OpenCL Environment (OCL_Environment.h)
 *
 * A class and set of functions for initialization and destruction of 
 * openCL contexts, platforms, and devices. 
 * 
 */

#pragma once

#include <wtypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <assert.h>

#include "CL\cl.h"
#include "CL\cl.hpp"
#include "CL\cl_gl_ext.h"
#include "CL\cl_ext.h"
#include "CL\cl_gl.h"
#include "CL\cl_platform.h"
#include "CL\cl_d3d10.h"
#include "CL\opencl.h"
#include "..\..\common\utils.h"

// Disable deprecation (sprintf,fprintf,etc)
#pragma warning(disable : 4995)
#pragma warning(disable : 4996)


// Loads extension functions and creates a platform
class OCL_Environment
{
private:
	cl_int	err;

	int OCL_LoadLibrary();
	void OCL_UnloadLibrary();

public:
	//initialize everything to 0/NULL
	OCL_Environment(){memset(this,0,sizeof(OCL_Environment));}

	cl_uint					uiNumPlatforms;
	OCL_Platform*			mpPlatforms;

	cl_int init();
	cl_int init(OCL_Environment_Desc desc);

	cl_int destroy();
};

//wrapper functions
// OCL D3D10 interop functions
extern CL_API_ENTRY cl_int CL_API_CALL clGetDeviceIDsFromD3D10KHR(
    cl_platform_id             platform,
    cl_d3d10_device_source_khr d3d_device_source,
    void *                     d3d_object,
    cl_d3d10_device_set_khr    d3d_device_set,
    cl_uint                    num_entries,
    cl_device_id *             devices,
    cl_uint *                  num_devices) 

	;


extern CL_API_ENTRY cl_mem CL_API_CALL clCreateFromD3D10BufferKHR(
    cl_context     context,
    cl_mem_flags   flags,
    ID3D10Buffer * resource,
    cl_int *       errcode_ret) 

	;


extern CL_API_ENTRY cl_mem CL_API_CALL clCreateFromD3D10Texture2DKHR(
    cl_context        context,
    cl_mem_flags      flags,
    ID3D10Texture2D * resource,
    UINT              subresource,
    cl_int *          errcode_ret) 

	;


extern CL_API_ENTRY cl_mem CL_API_CALL clCreateFromD3D10Texture3DKHR(
    cl_context        context,
    cl_mem_flags      flags,
    ID3D10Texture3D * resource,
    UINT              subresource,
    cl_int *          errcode_ret)

	;


extern CL_API_ENTRY cl_int CL_API_CALL clEnqueueAcquireD3D10ObjectsKHR(
    cl_command_queue command_queue,
    cl_uint          num_objects,
    const cl_mem *   mem_objects,
    cl_uint          num_events_in_wait_list,
    const cl_event * event_wait_list,
    cl_event *       event)

	;


extern CL_API_ENTRY cl_int CL_API_CALL clEnqueueReleaseD3D10ObjectsKHR(
    cl_command_queue command_queue,
    cl_uint          num_objects,
    cl_mem *         mem_objects,
    cl_uint          num_events_in_wait_list,
    const cl_event * event_wait_list,
    cl_event *       event) 

	;
