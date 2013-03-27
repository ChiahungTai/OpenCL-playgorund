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

#include <iostream>
#include <fstream>
#include <windows.h>
#include "sample_defs.h"
#include "DXUT.h"
#include "DXUTgui.h"
#include "sample_opencl_plugin.h"

using namespace std;

#define OCL_SAFE_CALL(FUNC) OCL_SAFE_CALL_ACT(FUNC,return MFX_ERR_DEVICE_FAILED)

// Constructor for openCL plugin
OCLPlugin::OCLPlugin(bool FastRelaxedMath)
{
    MSDK_TRACE(__FUNCTION__);

    m_OCLFlag = true;
    m_pOCLStruct = NULL;
    m_pMFXFrameAllocator = NULL;

    m_FlagFastRelaxedMath = FastRelaxedMath;
    m_Param1 = 50;
    m_Param2 = 50;
    m_MouseX = -1;
    m_MouseY = -1;
    m_ButtonFlag = 0;
    m_TimeAver = 0;
    m_OCLBuffer = NULL;
    m_OCLBufferSize = 0;

    // MFX
    m_bInited = false;
    m_pmfxCore = NULL;
    m_pTasks = NULL;
    m_MaxNumTasks = 0;
    memset(&m_mfxVideoParam, 0, sizeof(m_mfxVideoParam));
    memset(&m_mfxPluginParam, 0, sizeof(m_mfxPluginParam));
    // fill mfxPluginParam
    m_mfxPluginParam.ThreadPolicy = MFX_THREADPOLICY_SERIAL;
    m_mfxPluginParam.MaxThreadNum = 1;


    // OpenCL
    m_ProgramNum = 0;
    for(int i = 0; i<MAX_PROGRAM_NUM; ++i)
    {
        m_pProgramSource[i] = 0;
        m_pProgramBuildLOG[i] = 0;
        m_clProgram[i] = 0;
        m_clKernelProcessY[i] = 0;
        m_clKernelMouse[i] = 0;
        m_clKernelProcessUV[i] = 0;
    }
    m_GlobalWorkSizeY[0] = m_GlobalWorkSizeY[1] = 0;
    m_GlobalWorkSizeUV[0] = m_GlobalWorkSizeUV[1] = 0;

    m_hDevice = NULL;

    m_PictureH = 0;
    m_PictureW = 0;
    m_PictureBytesUV = 0;
    m_PictureBytesY = 0;

}

// destruct OpenCL Plugin
OCLPlugin::~OCLPlugin()
{
    MSDK_TRACE(__FUNCTION__);
    Close();
    mfxPluginClose();
}

// set current programm that will be executed for video frame
void    OCLPlugin::SetProgram(int index)
{
    MSDK_TRACE(__FUNCTION__);
    if(m_pOCLStruct==NULL) return;
    if(index < 0) return;
    if(index >=m_ProgramNum) return;
    if(m_ProgramIndex != index)
    {// clear internal OCL buffer
        void* pData;
        cl_int err;
        pData = clEnqueueMapBuffer(m_pOCLStruct->m_clCommandQueue,m_OCLBuffer,true,CL_MAP_READ,0,m_OCLBufferSize,0,NULL,NULL,&err);
        memset(pData,0,m_OCLBufferSize);
        err = clEnqueueUnmapMemObject(m_pOCLStruct->m_clCommandQueue,m_OCLBuffer,pData,0,NULL,NULL);
    }
    m_ProgramIndex = index;

}

// return name of currently executed or given program
WCHAR*   OCLPlugin::GetProgramName(int index)
{
    MSDK_TRACE(__FUNCTION__);
    if(index<0) index = m_ProgramIndex;
    if(index < 0 || index >=m_ProgramNum) return NULL;
    return m_ProgramName[index];
};

// return source code of currently executed or given program
char*   OCLPlugin::GetProgramSRC(int index)
{
    MSDK_TRACE(__FUNCTION__);
    if(index<0) index = m_ProgramIndex;
    if(index < 0 || index >=m_ProgramNum) return NULL;
    return m_pProgramSource[index];
};

// return build log of currently executed or given program
char*   OCLPlugin::GetProgramBuildLOG(int index)
{
    MSDK_TRACE(__FUNCTION__);
    if(index<0) index = m_ProgramIndex;
    if(index < 0 || index >=m_ProgramNum) return NULL;
    return m_pProgramBuildLOG[index];
};


// Initialization
// mfxParam contains input and output video parameters (resolution, etc).
// auxParam contains additional parameters (optional)
mfxStatus OCLPlugin::Init(mfxVideoParam *mfxParam, void* auxParam, int auxParamSize)
{
    MSDK_TRACE(__FUNCTION__);

    // set frame allocator
    m_pMFXFrameAllocator = *(BaseFrameAllocator**)auxParam;

    // check validity of parameters
    MSDK_CHECK_POINTER(mfxParam, MFX_ERR_NULL_PTR);     
    // only NV12 color format is supported
    if (MFX_FOURCC_NV12 != mfxParam->vpp.In.FourCC || MFX_FOURCC_NV12 != mfxParam->vpp.Out.FourCC)
    {
        return MFX_ERR_UNSUPPORTED;
    }    

    // save mfxVideoParam
    m_mfxVideoParam = *mfxParam;

    // set maximum nuber of internal task
    m_MaxNumTasks = m_mfxVideoParam.AsyncDepth + 1;
    // Create internal task pool
    m_pTasks = new MFXTask[m_MaxNumTasks];
    MSDK_CHECK_POINTER(m_pTasks, MFX_ERR_MEMORY_ALLOC);
    memset(m_pTasks, 0, sizeof(MFXTask) * m_MaxNumTasks);


    // Picture size
    m_PictureW = mfxParam->vpp.In.Width;
    m_PictureH = mfxParam->vpp.In.Height;
    m_PictureBytesY = m_PictureH * m_PictureW;
    m_PictureBytesUV = m_PictureH * m_PictureW / 2;


    // Global and loacal work sizes to process each Y component
    // Each workitem process 1 pixel
    m_GlobalWorkSizeY[0] = m_PictureW;
    m_GlobalWorkSizeY[1] = m_PictureH;
    m_LocalWorkSizeY[0] = 8;
    m_LocalWorkSizeY[1] = 8;
    m_GlobalWorkSizeY[0] = m_LocalWorkSizeY[0]*(m_GlobalWorkSizeY[0]/m_LocalWorkSizeY[0]);
    m_GlobalWorkSizeY[1] = m_LocalWorkSizeY[1]*(m_GlobalWorkSizeY[1]/m_LocalWorkSizeY[1]);


    // Global and loacal work sizes to process each UV component
    // Each workitem process 4 neirghborhood pixels
    m_GlobalWorkSizeUV[0] = m_PictureW/2;
    m_GlobalWorkSizeUV[1] = m_PictureH/2;
    m_LocalWorkSizeUV[0] = 8;
    m_LocalWorkSizeUV[1] = 8;
    m_GlobalWorkSizeUV[0] = m_LocalWorkSizeUV[0]*(m_GlobalWorkSizeUV[0]/m_LocalWorkSizeUV[0]);
    m_GlobalWorkSizeUV[1] = m_LocalWorkSizeUV[1]*(m_GlobalWorkSizeUV[1]/m_LocalWorkSizeUV[1]);

    printf("Image size %dx%d\n",m_PictureW,m_PictureH);
    printf("Y global size %dx%d\n",m_GlobalWorkSizeY[0],m_GlobalWorkSizeY[1]);
    printf("Y local size %dx%d\n",m_LocalWorkSizeY[0],m_LocalWorkSizeY[1]);
    printf("UV global size %dx%d\n",m_GlobalWorkSizeUV[0],m_GlobalWorkSizeUV[1]);
    printf("UV local size %dx%d\n",m_LocalWorkSizeUV[0],m_LocalWorkSizeUV[1]);

    if(m_pOCLStruct->m_intel_dx9_media_sharing)
    {// create intermidiate buffers between Decoder and OCL code. 
        // it is needed becasue decoder uses        DXVA2_VideoDecoderRenderTarget surfaces
        // and dx9_media_sharing extention requires DXVA2_VideoProcessorRenderTarget surfaces
        int i;
        mfxStatus sts = MFX_ERR_NONE;
        mfxFrameAllocRequest OCLRequest;
        OCLRequest.NumFrameSuggested = OCLRequest.NumFrameMin = (mfxU16)m_MaxNumTasks;
        OCLRequest.Type = MFX_MEMTYPE_INTERNAL_FRAME | MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET | MFX_MEMTYPE_FROM_VPPOUT;
        OCLRequest.Info = mfxParam->vpp.In;
        
        // alloc intermidiate frames
        sts = m_pMFXFrameAllocator->Alloc(m_pMFXFrameAllocator->pthis, &OCLRequest, &m_mfxOCLINResponse);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        if(m_mfxOCLINResponse.NumFrameActual<m_MaxNumTasks)
            return MFX_ERR_MEMORY_ALLOC;

        for (i = 0; i < m_MaxNumTasks; i++)
        {       
            IDirect3DSurface9* pD3DS = (IDirect3DSurface9*)(m_mfxOCLINResponse.mids[i]);
            HANDLE phNV12 = (HANDLE)(m_mfxOCLINResponse.mids[OCLRequest.NumFrameSuggested+i]);
            mfxFrameSurface1_OCL* pS = &(m_pTasks[i].InBuffer);
            memset(pS, 0, sizeof(mfxFrameSurface1_OCL));
            pS->Info=OCLRequest.Info;
            pS->Data.MemId = m_mfxOCLINResponse.mids[i];
            // create OCL objects that share intermidiate DXVA surfaces
            OCL_SAFE_CALL_NORET(( pS->OCL_Y = m_pOCLStruct->m_clCreateFromDX9MediaSurfaceINTEL(m_pOCLStruct->m_clContext,0,pD3DS,phNV12,0,&RET_STS),RET_STS));
            OCL_SAFE_CALL_NORET(( pS->OCL_UV = m_pOCLStruct->m_clCreateFromDX9MediaSurfaceINTEL(m_pOCLStruct->m_clContext,0,pD3DS,phNV12,1,&RET_STS),RET_STS));
        }  
    }


    // recreate internal buffer according new image size
    SAFE_FREE(m_OCLBuffer,clReleaseMemObject);
    m_OCLBufferSize = 0;
    if(m_pOCLStruct && (m_PictureW*m_PictureH)>0)
    {// create internal buffer for OCL programs initialized by 0
        assert(m_pOCLStruct->m_clContext);
        m_OCLBufferSize = sizeof(float)*m_PictureW*m_PictureH;
        OCL_SAFE_CALL(m_OCLBuffer = clCreateBuffer(m_pOCLStruct->m_clContext, CL_MEM_READ_WRITE, m_OCLBufferSize, NULL, &RET_STS));
        {// clear internal OCL buffer
            void* pData = NULL;
            OCL_SAFE_CALL_NORET(pData = clEnqueueMapBuffer(m_pOCLStruct->m_clCommandQueue,m_OCLBuffer,true,CL_MAP_READ,0,m_OCLBufferSize,0,NULL,NULL,&RET_STS));
            memset(pData,0,m_OCLBufferSize);
            OCL_SAFE_CALL_NORET(RET_STS = clEnqueueUnmapMemObject(m_pOCLStruct->m_clCommandQueue,m_OCLBuffer,pData,0,NULL,NULL));
        }
    }


    // Initialization completed
    m_bInited = true;

    return MFX_ERR_NONE;
}

// clear list of OCL programm 
void OCLPlugin::ClearProgramList()
{
    MSDK_TRACE(__FUNCTION__);
    for(int i=0;i<m_ProgramNum;++i)
    {
        SAFE_FREE(m_clKernelProcessUV[i], clReleaseKernel);
        SAFE_FREE(m_clKernelProcessY[i], clReleaseKernel);
        SAFE_FREE(m_clKernelMouse[i], clReleaseKernel);
        SAFE_FREE(m_clProgram[i], clReleaseProgram);
        MSDK_SAFE_DELETE_ARRAY(m_pProgramSource[i]);
        MSDK_SAFE_DELETE_ARRAY(m_pProgramBuildLOG[i]);
    }
    m_ProgramNum = 0;
}

// init OCL stuff for given platform
mfxStatus OCLPlugin::InitOCL(OCLStruct* pOCLStruct)
{
    MSDK_TRACE(__FUNCTION__);
    if(m_pOCLStruct)
    {
        for(int i=0;i<m_MaxNumTasks;++i)
        {
            if(m_pTasks[i].bBusy)
            {
                printf("ERROR!!! could not release OCL buffers because OCL filter is wroking NOW!\n");
            }
        }

        ClearProgramList();

        SAFE_FREE(m_OCLBuffer,clReleaseMemObject);
        m_pOCLStruct = NULL;
    }
    m_pOCLStruct = pOCLStruct;
    if(m_pOCLStruct==NULL) return MFX_ERR_NONE;

    // fill list of programs
    UpdateProgramList();

    // recreate internal buffer according new image size
    SAFE_FREE(m_OCLBuffer,clReleaseMemObject);
    m_OCLBufferSize = 0;
    if(m_pOCLStruct && (m_PictureW*m_PictureH)>0)
    {// create internal buffer for OCL programs initialized by 0
        assert(m_pOCLStruct->m_clContext);
        m_OCLBufferSize = sizeof(float)*m_PictureW*m_PictureH;
        OCL_SAFE_CALL(m_OCLBuffer = clCreateBuffer(m_pOCLStruct->m_clContext, CL_MEM_READ_WRITE, m_OCLBufferSize, NULL, &RET_STS));
        {// clear internal OCL buffer
            void* pData = NULL;
            OCL_SAFE_CALL_NORET(pData = clEnqueueMapBuffer(m_pOCLStruct->m_clCommandQueue,m_OCLBuffer,true,CL_MAP_READ,0,m_OCLBufferSize,0,NULL,NULL,&RET_STS));
            memset(pData,0,m_OCLBufferSize);
            OCL_SAFE_CALL_NORET(RET_STS = clEnqueueUnmapMemObject(m_pOCLStruct->m_clCommandQueue,m_OCLBuffer,pData,0,NULL,NULL));
        }
    }


    return MFX_ERR_NONE;
}// InitOCL

// scan folder and recreate the list of programs if there is any change in folder
bool OCLPlugin::UpdateProgramList()
{
    MSDK_TRACE(__FUNCTION__);
    bool flagUpdate = false;
    
    WIN32_FIND_DATA FindFileData;    
    HANDLE hFind;     
    hFind = FindFirstFile(L"*.cl", &FindFileData);

    if(hFind == INVALID_HANDLE_VALUE)
    {
        if(m_ProgramNum>0) flagUpdate = true;
    }
    else if(m_ProgramNum<=0)
    {
        flagUpdate = true;
    }
    else
    {
        int index=0;

        for(;;)
        {
            if( wcsncmp(m_ProgramName[index],FindFileData.cFileName,sizeof(WCHAR)*MAX_PATH) !=0 )
            {
                flagUpdate = true;
                break;
            }

            index++;
            if(!FindNextFile(hFind,&FindFileData))
            {
                if(index<m_ProgramNum) 
                    flagUpdate = true;
                break;
            }

            if(index>=m_ProgramNum)
            {
                flagUpdate = true;
                break;
            }
        }
        FindClose(hFind);    
    }

    if(flagUpdate)
    {// new list of programms will be created
        WCHAR ProgramName[MAX_PATH];
        int     index = m_ProgramIndex;
        if(index >=0 && index < m_ProgramNum)
        {
            memcpy(ProgramName,m_ProgramName[index],sizeof(WCHAR)*MAX_PATH);
            index = -1;
        }
        else
        {
            index = 0;
        }

        ClearProgramList();
        hFind = FindFirstFile(L"_*.cl", &FindFileData);    

        for(;hFind != INVALID_HANDLE_VALUE && m_ProgramNum < MAX_PROGRAM_NUM;)
        {
            memcpy(m_ProgramName[m_ProgramNum],FindFileData.cFileName,sizeof(WCHAR)*MAX_PATH);
            if( index<0 && wcsncmp(m_ProgramName[m_ProgramNum],ProgramName,sizeof(WCHAR)*MAX_PATH) ==0 )
            {
                index = m_ProgramNum;
            }
            m_ProgramNum++;
            // load kernel code and compile it
            UpdateProgram(m_ProgramNum-1);
            if(!FindNextFile(hFind,&FindFileData)) break;
        }
        if (hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);    
        }

        if(index<0 && m_ProgramIndex>=m_ProgramNum)
            index = m_ProgramNum-1;
        SetProgram(index);
    }

    // return is list updated or not
    return flagUpdate;
}

// check if curretn program source file is changed and rebuild it
mfxStatus OCLPlugin::UpdateProgram(int  index)
{
    MSDK_TRACE(__FUNCTION__);
    int RebuildFlag = 0;
    if(index<0) index = m_ProgramIndex;
    if(index<0 || index >= m_ProgramNum) return MFX_ERR_UNSUPPORTED;
    char* pSrc = NULL;
    size_t SrcSize = 0;

    {// read file from disk 
        WCHAR* pSrcFile = m_ProgramName[index];
        ifstream is;
        is.open(pSrcFile, ios::binary);
        if(!is.good())
        {
            printf("could not open cl kernel to read\n");
            return MFX_ERR_UNSUPPORTED;
        }

        is.seekg (0, ios::end);
        SrcSize = (size_t)is.tellg();
        is.seekg (0, ios::beg);

        if(SrcSize == 0)
        {
            return MFX_ERR_UNSUPPORTED;
        }

        pSrc = new char[SrcSize+1];
        if(!pSrc)
        {
            return MFX_ERR_UNSUPPORTED;
        }

        is.read(pSrc, SrcSize);
        is.close();
        pSrc[SrcSize]=0;

        if(m_pProgramSource[index]==NULL)RebuildFlag=1;

        int i;
        for(i=0;!RebuildFlag && pSrc[i] && m_pProgramSource[index][i];i++)
        {
            if(pSrc[i] != m_pProgramSource[index][i])
            {
                RebuildFlag=1;
                break;
            }
        }
        if(pSrc && m_pProgramSource[index] && pSrc[i] != m_pProgramSource[index][i])
        {
            RebuildFlag=1;
        }
    }// end read code;


    if(!RebuildFlag)
    {
        MSDK_SAFE_DELETE_ARRAY(pSrc);
        return MFX_ERR_NONE;
    }
        
    // delete old kernel
    SAFE_FREE(m_clKernelProcessUV[index], clReleaseKernel);
    SAFE_FREE(m_clKernelProcessY[index], clReleaseKernel);
    SAFE_FREE(m_clKernelMouse[index], clReleaseKernel);
    SAFE_FREE(m_clProgram[index], clReleaseProgram);
    MSDK_SAFE_DELETE_ARRAY(m_pProgramBuildLOG[index]);
    MSDK_SAFE_DELETE_ARRAY(m_pProgramSource[index]);
    m_pProgramSource[index] = pSrc;

    if(m_pOCLStruct==NULL) return MFX_ERR_NULL_PTR;

    // Create the program object
    OCL_SAFE_CALL(m_clProgram[index] = clCreateProgramWithSource(m_pOCLStruct->m_clContext, 1, (const char **)&m_pProgramSource[index], &SrcSize, &RET_STS));

    // Build program
    cl_int err = clBuildProgram(m_clProgram[index], 0, NULL, m_FlagFastRelaxedMath?"-cl-fast-relaxed-math":NULL, NULL, NULL);
    {
        size_t paramValueSizeRet = 0;
        clGetProgramBuildInfo(m_clProgram[index], m_pOCLStruct->GetDevice(), CL_PROGRAM_BUILD_LOG, 0, NULL, &paramValueSizeRet);
        //printf("clGetProgramBuildInfo return %lu size\n",(unsigned long)paramValueSizeRet);
        m_pProgramBuildLOG[index] = new char[paramValueSizeRet+1];
        clGetProgramBuildInfo(m_clProgram[index], m_pOCLStruct->GetDevice(), CL_PROGRAM_BUILD_LOG, paramValueSizeRet, m_pProgramBuildLOG[index], &paramValueSizeRet);
        m_pProgramBuildLOG[index][paramValueSizeRet] = 0;
        if(err!=CL_SUCCESS)printf("Build Log:\n%s\n",m_pProgramBuildLOG[index]);
        fflush(NULL);
    }

    if(err==CL_SUCCESS)
    {
        cl_int err;
        // Create kernels if kernel exist
        // if kernel does not exist then keep id==NULL. it is not error
        if(strstr(m_pProgramSource[index],"Mouse"))
            m_clKernelMouse[index] = clCreateKernel(m_clProgram[index], "Mouse", &err);
        if(strstr(m_pProgramSource[index],"ProcessY"))
            m_clKernelProcessY[index] = clCreateKernel(m_clProgram[index], "ProcessY", &err);
        if(strstr(m_pProgramSource[index],"ProcessUV"))
            m_clKernelProcessUV[index] = clCreateKernel(m_clProgram[index], "ProcessUV", &err);
    }
    return MFX_ERR_NONE;
}// UpdateKrenel


mfxStatus OCLPlugin::Close()
{
    MSDK_TRACE(__FUNCTION__);

    if(m_pOCLStruct && m_pOCLStruct->m_intel_dx9_media_sharing)
    {
        mfxStatus sts = MFX_ERR_NONE;
        for (int i = 0; i < m_mfxOCLINResponse.NumFrameActual; i++)
        {
            m_pOCLStruct->m_clEnqueueReleaseDX9ObjectsINTEL(m_pOCLStruct->m_clCommandQueue,1,&(m_pTasks[i].InBuffer.OCL_Y),0,NULL,NULL);
            m_pOCLStruct->m_clEnqueueReleaseDX9ObjectsINTEL(m_pOCLStruct->m_clCommandQueue,1,&(m_pTasks[i].InBuffer.OCL_UV),0,NULL,NULL);
            OCL_SAFE_CALL_NORET(RET_STS = clFinish(m_pOCLStruct->m_clCommandQueue));
            SAFE_FREE(m_pTasks[i].InBuffer.OCL_Y,clReleaseMemObject);
            SAFE_FREE(m_pTasks[i].InBuffer.OCL_UV,clReleaseMemObject);
        }
        sts = m_pMFXFrameAllocator->Free(m_pMFXFrameAllocator->pthis,&m_mfxOCLINResponse);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    }

    // Release OpenCL Objects
    InitOCL(NULL);
    // Release MFXTask array
    MSDK_SAFE_DELETE_ARRAY(m_pTasks);   
    memset(&m_mfxPluginParam, 0, sizeof(m_mfxPluginParam));

    if(m_hDevice && m_pMFXFrameAllocator)
    {
        HRESULT hr;
        IDirect3DDeviceManager9* pDM = ((D3DFrameAllocator*)m_pMFXFrameAllocator)->GetDeviceManager();
        V(pDM->CloseDeviceHandle(m_hDevice));
        m_hDevice = NULL;
    }

    m_bInited = false;

    return MFX_ERR_NONE;
}


// working function that feeds OCL queue by tasks
mfxStatus OCLPlugin::EnqueueOCLTasks(MFXTask *pTask)
{
    MSDK_TRACE(__FUNCTION__);
    int Processed = 0;
    int KernelIndex = m_ProgramIndex;

    if(m_pOCLStruct==NULL) return MFX_ERR_NULL_PTR;

    //clear event
    pTask->clEventStop = NULL;

    if(m_OCLFlag && KernelIndex>=0 && KernelIndex<m_ProgramNum)
    {
        cl_mem	surfaces[4] = {pTask->In->OCL_Y,pTask->In->OCL_UV,pTask->Out->OCL_Y,pTask->Out->OCL_UV};
        if(m_pOCLStruct->m_intel_dx9_media_sharing)
        {// look d3d10 texture 
            OCL_SAFE_CALL(RET_STS = m_pOCLStruct->m_clEnqueueAcquireDX9ObjectsINTEL(m_pOCLStruct->m_clCommandQueue,4,surfaces,0,NULL,NULL));
        }

        if(m_clKernelMouse[KernelIndex])
        {// run Mouse preprocessing kernel
            cl_kernel kernel = m_clKernelMouse[KernelIndex];
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 0, sizeof(cl_int), (void*)&m_MouseX));
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 1, sizeof(cl_int), (void*)&m_MouseY));
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 2, sizeof(cl_int), (void*)&m_PictureW));
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 3, sizeof(cl_int), (void*)&m_PictureH));
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 4, sizeof(cl_int), (void*)&m_ButtonFlag));
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 5, sizeof(cl_int), (void*)&m_Param1));
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 6, sizeof(cl_int), (void*)&m_Param2));
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 7, sizeof(cl_mem), (void*)&m_OCLBuffer));
            OCL_SAFE_CALL(RET_STS = clEnqueueTask(m_pOCLStruct->m_clCommandQueue, kernel,0,0,NULL));
        }

        if(m_clKernelProcessY[KernelIndex])
        {// run kernel to process Y components
            cl_kernel kernel = m_clKernelProcessY[KernelIndex];
            Processed = 1;
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&pTask->In->OCL_Y));
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&pTask->In->OCL_UV));
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 2, sizeof(cl_int), (void*)&pTask->BufferSrcPitch));
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void*)&pTask->Out->OCL_Y));
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void*)&pTask->Out->OCL_UV));
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 5, sizeof(cl_int), (void*)&pTask->BufferDstPitch));
            clSetKernelArg(kernel, 6, sizeof(cl_int), (void*)&m_Param1);
            clSetKernelArg(kernel, 7, sizeof(cl_int), (void*)&m_Param2);
            clSetKernelArg(kernel, 8, sizeof(cl_mem), (void*)&m_OCLBuffer);

            // Enqueue KernelProcessY
            OCL_SAFE_CALL(RET_STS = clEnqueueNDRangeKernel(m_pOCLStruct->m_clCommandQueue, kernel, 2, NULL, m_GlobalWorkSizeY, m_LocalWorkSizeY,0, NULL,NULL));
        }

        if(m_clKernelProcessUV[KernelIndex])
        {// run kernel to process UV compoments
            cl_kernel kernel = m_clKernelProcessUV[KernelIndex];
            Processed = 1; 
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&pTask->In->OCL_Y));
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&pTask->In->OCL_UV));
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 2, sizeof(cl_int), (void*)&pTask->BufferSrcPitch));
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void*)&pTask->Out->OCL_Y));
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void*)&pTask->Out->OCL_UV));
            OCL_SAFE_CALL(RET_STS = clSetKernelArg(kernel, 5, sizeof(cl_int), (void*)&pTask->BufferDstPitch));
            clSetKernelArg(kernel, 6, sizeof(cl_int), (void*)&m_Param1);
            clSetKernelArg(kernel, 7, sizeof(cl_int), (void*)&m_Param2);
            clSetKernelArg(kernel, 8, sizeof(cl_mem), (void*)&m_OCLBuffer);

            // Enqueue KernelProcessUV
            OCL_SAFE_CALL(RET_STS = clEnqueueNDRangeKernel(m_pOCLStruct->m_clCommandQueue, kernel, 2, NULL, m_GlobalWorkSizeUV, m_LocalWorkSizeUV,0, NULL,NULL));
        }
        if(m_pOCLStruct->m_intel_dx9_media_sharing)
        {// unlook d3d9 texture 
            OCL_SAFE_CALL(RET_STS = m_pOCLStruct->m_clEnqueueReleaseDX9ObjectsINTEL(m_pOCLStruct->m_clCommandQueue,4,surfaces,0,NULL,NULL));
        }
    }

    OCL_SAFE_CALL(RET_STS = clEnqueueMarker(m_pOCLStruct->m_clCommandQueue, &pTask->clEventStop));
    // Flush queue
    OCL_SAFE_CALL(RET_STS = clFlush(m_pOCLStruct->m_clCommandQueue));

    return MFX_TASK_WORKING;
}

// Check status of OCL execution
mfxStatus OCLPlugin::QueryStatus(MFXTask *pTask)
{
    MSDK_TRACE(__FUNCTION__);
    // check event
    if(!pTask->clEventStop)
        return MFX_ERR_DEVICE_FAILED;
    cl_int sts = -1;    
    size_t size_ret = 0;    
    OCL_SAFE_CALL_ACT(RET_STS = clGetEventInfo(pTask->clEventStop, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(sts), &sts, &size_ret),return MFX_ERR_DEVICE_FAILED);\
    if (sts < 0)  
        return MFX_ERR_DEVICE_FAILED;
    if(sts != CL_COMPLETE)  
        return MFX_TASK_WORKING;
    SAFE_FREE(pTask->clEventStop,clReleaseEvent);
    return MFX_TASK_DONE;
}


// function that called to start task execution and check status of execution
mfxStatus OCLPlugin::mfxExecute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a)
{    
    MSDK_TRACE(__FUNCTION__);
    mfxStatus       sts;

    uid_p = uid_p;

    MFXTask*        pTask = (MFXTask *)task;

    MSDK_CHECK_ERROR(m_bInited, false, MFX_ERR_NOT_INITIALIZED);
    MSDK_CHECK_POINTER(m_pmfxCore, MFX_ERR_NOT_INITIALIZED);  
    MSDK_CHECK_POINTER(m_pOCLStruct, MFX_ERR_NOT_INITIALIZED);

    if (0 == uid_a)
    {// first call
        // get start counter
        QueryPerformanceCounter(&(pTask->ticks[0]));
        // feed OCL queue by tasks
        sts = EnqueueOCLTasks(pTask);
    }
    
    // check is OCL finish working or not
    sts = QueryStatus(pTask);

    if(MFX_TASK_DONE == sts)
    {// OCL finished. 
        //get clocks and calc execution time
        LARGE_INTEGER F;
        QueryPerformanceFrequency(&F);
        QueryPerformanceCounter(&(pTask->ticks[1]));
        float T = (float)(pTask->ticks[1].QuadPart-pTask->ticks[0].QuadPart);
        m_TimeAver = m_TimeAver*0.95f + 0.05f*(1000.0f*T)/(float)F.QuadPart;
    }


    return sts;        
}//OCLPlugin::mfxExecute

// called to submit MediaSDK task but not execute it
mfxStatus OCLPlugin::mfxSubmit(const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxThreadTask *task)
{
    MSDK_TRACE(__FUNCTION__);

    MSDK_CHECK_POINTER(in, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(out, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(*in, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(*out, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(task, MFX_ERR_NULL_PTR);    
    MSDK_CHECK_POINTER(m_pOCLStruct, MFX_ERR_NULL_PTR);    
    MSDK_CHECK_NOT_EQUAL(in_num, 1, MFX_ERR_UNSUPPORTED);
    MSDK_CHECK_NOT_EQUAL(out_num, 1, MFX_ERR_UNSUPPORTED);
    MSDK_CHECK_POINTER(m_pmfxCore, MFX_ERR_NOT_INITIALIZED);  
    MSDK_CHECK_ERROR(m_bInited, false, MFX_ERR_NOT_INITIALIZED);  

    mfxFrameSurface1_OCL *surface_in = (mfxFrameSurface1_OCL *)in[0];
    mfxFrameSurface1_OCL *surface_out = (mfxFrameSurface1_OCL *)out[0];          
    
    {// check validity of parameters
        mfxFrameInfo *pIn = &surface_in->Info; 
        mfxFrameInfo *pOut = &surface_out->Info;
        if ((pIn->CropW != m_mfxVideoParam.vpp.In.CropW || 
            pIn->CropH != m_mfxVideoParam.vpp.In.CropH || 
            pIn->FourCC != m_mfxVideoParam.vpp.In.FourCC ||
            pOut->CropW != m_mfxVideoParam.vpp.Out.CropW || 
            pOut->CropH != m_mfxVideoParam.vpp.Out.CropH || 
            pOut->FourCC != m_mfxVideoParam.vpp.Out.FourCC) && (pOut->FourCC != MFX_FOURCC_RGB4))
        {
            return MFX_ERR_INVALID_VIDEO_PARAM;
        }
    }

    // find free task in task array
    int ind;
    for (ind = 0; ind < m_MaxNumTasks; ind++)
    {
        if (!m_pTasks[ind].bBusy)
            break;
    }

    if (ind >= m_MaxNumTasks)
    {
        return MFX_WRN_DEVICE_BUSY; // currently there are no free tasks available
    }

    MFXTask* pTask = &m_pTasks[ind];
    
    if(m_pOCLStruct->m_intel_dx9_media_sharing)
    {// copy input data into internal buffer
        IDirect3DDevice9*   pd3dDevice = NULL;
        HRESULT hr;
        IDirect3DDeviceManager9* pDM = ((D3DFrameAllocator*)m_pMFXFrameAllocator)->GetDeviceManager();
        if(m_hDevice==NULL)
        {
            V(pDM->OpenDeviceHandle(&m_hDevice));
        }
        V(pDM->LockDevice(m_hDevice, &pd3dDevice, true));
        V(pd3dDevice->StretchRect(
            (IDirect3DSurface9*)surface_in->Data.MemId, NULL, 
            (IDirect3DSurface9*)pTask->InBuffer.Data.MemId, NULL, 
            D3DTEXF_POINT));
        pd3dDevice->Release();
        V(pDM->UnlockDevice(m_hDevice, true));
        pTask->In = &(pTask->InBuffer);
    }
    else
    {// store the reference to input surface for further processing
        pTask->In = surface_in;
        m_pmfxCore->IncreaseReference(m_pmfxCore->pthis, &(surface_in->Data));
    }

    pTask->Out = surface_out;
    m_pmfxCore->IncreaseReference(m_pmfxCore->pthis, &(surface_out->Data));

    pTask->bBusy = true;
    *task = (mfxThreadTask)pTask;    

    return MFX_ERR_NONE;
}// OCLPlugin::mfxSubmit


// free task and releated resources
mfxStatus OCLPlugin::mfxFreeResources(mfxThreadTask task, mfxStatus sts)
{
    MSDK_TRACE(__FUNCTION__);
    sts = sts;
    MSDK_CHECK_ERROR(m_bInited, false, MFX_ERR_NOT_INITIALIZED); 
    MSDK_CHECK_POINTER(m_pmfxCore, MFX_ERR_NOT_INITIALIZED);  

    MFXTask *pTask = (MFXTask *)task;
    
    if(!m_pOCLStruct->m_intel_dx9_media_sharing)
    {
        m_pmfxCore->DecreaseReference(m_pmfxCore->pthis, &(pTask->In->Data));
    }

    m_pmfxCore->DecreaseReference(m_pmfxCore->pthis, &(pTask->Out->Data));
    SAFE_FREE(pTask->clEventStop,clReleaseEvent);

    pTask->bBusy = false;

    return MFX_ERR_NONE;
}//OCLPlugin::mfxFreeResources

mfxStatus OCLPlugin::mfxPluginInit(mfxCoreInterface *core)
{
    MSDK_TRACE(__FUNCTION__);

    MSDK_CHECK_POINTER(core, MFX_ERR_NULL_PTR);       
        
    MSDK_SAFE_DELETE(m_pmfxCore);

    m_pmfxCore = new mfxCoreInterface; 
    MSDK_CHECK_POINTER(m_pmfxCore, MFX_ERR_MEMORY_ALLOC);
    *m_pmfxCore = *core;    

    return MFX_ERR_NONE;
}

mfxStatus OCLPlugin::mfxPluginClose()
{
    MSDK_TRACE(__FUNCTION__);

    MSDK_SAFE_DELETE(m_pmfxCore);

    return MFX_ERR_NONE;
}

mfxStatus OCLPlugin::mfxGetPluginParam(mfxPluginParam *par)
{
    MSDK_TRACE(__FUNCTION__);

    MSDK_CHECK_POINTER(par, MFX_ERR_NULL_PTR);    

    *par = *(mfxPluginParam *)&m_mfxPluginParam;

    return MFX_ERR_NONE;
}

