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

   
#ifndef __OCLSTRUCT_H__
#define __OCLSTRUCT_H__

// define for <CL/cl_ext.h>
#define DX9_MEDIA_SHARING

#include <stdio.h>
#include <D3D9.h>
#include <CL/opencl.h>
#include <CL/cl_ext.h>

#define OCL_SAFE_CALL_NORET(FUNC)             \
{                                       \
    cl_int RET_STS = CL_SUCCESS;        \
    FUNC;                               \
    if (RET_STS != CL_SUCCESS)          \
    {                                   \
        printf("Failed %s, status %d\n", #FUNC, RET_STS);  \
        printf("%s: %d\n", __FILE__, __LINE__);  \
    }                                   \
}
#define OCL_SAFE_CALL_ACT(FUNC,act)             \
{                                       \
    cl_int RET_STS = CL_SUCCESS;        \
    FUNC;                               \
    if (RET_STS != CL_SUCCESS)          \
    {                                   \
        printf("Failed %s, status %d\n", #FUNC, RET_STS);  \
        printf("%s: %d\n", __FILE__, __LINE__);  act;\
    }                                   \
}

#ifndef SAFE_FREE
#define SAFE_FREE(P, FREE_FUNC)  { if (P) { FREE_FUNC(P); P = NULL; } }
#endif

#define EXT_DECLARE(_name) _name##_fn m_##_name
#define EXT_INIT(_name) m_##_name = (_name##_fn) clGetExtensionFunctionAddress(#_name);

// maximal number of OCL platforms
#define MAX_PLATFORM_NUM 16
#define MAX_NAME_SIZE 1024
class OCLStruct
{
public:
    int                 m_intel_dx9_media_sharing;//flag that show is dx9_media_sharing enable or not 
    cl_uint             m_NumDevices;// device number of curretn platform
    cl_device_id        m_Devices[2];// array of devices for current platform
    cl_context          m_clContext;// curretn OCL context
    cl_command_queue    m_clCommandQueue;// curent OCL queue

    int                 m_PlatformCurrent;  // index of curretn OCL platform
    cl_uint             m_PlatformNum; // actual number of existed OCL platfroms
    cl_platform_id      m_PlatformIDs[MAX_PLATFORM_NUM];// array of platfroms IDs
    char                m_PlatformNames[MAX_PLATFORM_NUM][MAX_NAME_SIZE];// array of OCL platforms names

    EXT_DECLARE(clGetDeviceIDsFromDX9INTEL);
    EXT_DECLARE(clCreateFromDX9MediaSurfaceINTEL);
    EXT_DECLARE(clEnqueueAcquireDX9ObjectsINTEL);
    EXT_DECLARE(clEnqueueReleaseDX9ObjectsINTEL);
    inline int InitDX9MediaFunctions()
    {
        // get DXVA sharing functions
        EXT_INIT(clGetDeviceIDsFromDX9INTEL);
        EXT_INIT(clCreateFromDX9MediaSurfaceINTEL);
        EXT_INIT(clEnqueueAcquireDX9ObjectsINTEL);
        EXT_INIT(clEnqueueReleaseDX9ObjectsINTEL);
        return 
            (m_clGetDeviceIDsFromDX9INTEL!=NULL) && 
            (m_clCreateFromDX9MediaSurfaceINTEL!=NULL) && 
            (m_clEnqueueAcquireDX9ObjectsINTEL!=NULL) && 
            (m_clEnqueueReleaseDX9ObjectsINTEL!=NULL);
    }

    OCLStruct(IDirect3DDevice9* pd3dDevice);
    ~OCLStruct();
    cl_device_id GetDevice()
    {
        return m_Devices[0];
    }

};


#endif // __OCLSTRUCT_H__