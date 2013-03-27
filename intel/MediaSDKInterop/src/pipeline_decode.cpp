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

#include <tchar.h>
#include <windows.h>

#include "pipeline_decode.h"


#include <smmintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>

// return name of codec by given ID
static TCHAR* CodecIdToStr(mfxU32 nFourCC)
{
    switch(nFourCC)
    {
    case MFX_CODEC_AVC:
        return _T("AVC");
    case MFX_CODEC_VC1:
        return _T("VC1");
    case MFX_CODEC_MPEG2:
        return _T("MPEG2");
    default:
        return _T("NOT_SUPPORTED");
    }
}

// function that scan frame array and return not locked frame
static mfxU16 GetFreeSurfaceIndex(mfxFrameSurface1_OCL* pSurfacesPool, mfxU16 nPoolSize, mfxU16 start=0)
{    
    if (pSurfacesPool)
    {
        for (mfxU16 i = 0; i < nPoolSize; i++)
        {
            mfxU16 ii = (start+i)%nPoolSize;
            if (0 == pSurfacesPool[ii].Data.Locked)
            {       
                return ii;
            }
        }
    }   

    return MSDK_INVALID_SURF_IDX;
}


// reset bitstream
static void WipeMfxBitstream(mfxBitstream* pBitstream)
{
    MSDK_CHECK_POINTER_NO_RET(pBitstream);

    //free allocated memory
    MSDK_SAFE_DELETE_ARRAY(pBitstream->Data);    
}

// initialize betsream before file reading starts 
static mfxStatus InitMfxBitstream(mfxBitstream* pBitstream, mfxU32 nSize)
{
    //check input params
    MSDK_CHECK_POINTER(pBitstream, MFX_ERR_NULL_PTR);
    MSDK_CHECK_ERROR(nSize, 0, MFX_ERR_NOT_INITIALIZED);

    //prepare pBitstream  
    WipeMfxBitstream(pBitstream);

    //prepare buffer
    pBitstream->Data = new mfxU8[nSize];
    MSDK_CHECK_POINTER(pBitstream->Data, MFX_ERR_MEMORY_ALLOC);

    pBitstream->MaxLength = nSize;

    return MFX_ERR_NONE;
}
// extend size of data storage. this function is callaed if decoder request more data
static mfxStatus ExtendMfxBitstream(mfxBitstream* pBitstream, mfxU32 nSize)
{
    MSDK_CHECK_POINTER(pBitstream, MFX_ERR_NULL_PTR);

    MSDK_CHECK_ERROR(nSize <= pBitstream->MaxLength, true, MFX_ERR_UNSUPPORTED);

    mfxU8* pData = new mfxU8[nSize];  
    MSDK_CHECK_POINTER(pData, MFX_ERR_MEMORY_ALLOC);

    memmove(pData, pBitstream->Data + pBitstream->DataOffset, pBitstream->DataLength);

    WipeMfxBitstream(pBitstream);

    pBitstream->Data       = pData;
    pBitstream->DataOffset = 0;
    pBitstream->MaxLength  = nSize;

    return MFX_ERR_NONE;
}

//===== CSmplBitstreamReader iplementation ======
CSmplBitstreamReader::CSmplBitstreamReader()
{
    m_fSource = NULL;
}

CSmplBitstreamReader::~CSmplBitstreamReader()
{
    Close();
}

void CSmplBitstreamReader::Close()
{
    if (m_fSource)
    {
        fclose(m_fSource);
        m_fSource = NULL;
    }
}

// initialize reader to read given file
mfxStatus CSmplBitstreamReader::Init(const TCHAR *strFileName)
{
    MSDK_CHECK_POINTER(strFileName, MFX_ERR_NULL_PTR);
    MSDK_CHECK_ERROR(_tcslen(strFileName), 0, MFX_ERR_NOT_INITIALIZED);

    Close();

    //open file to read input stream
    _tfopen_s(&m_fSource, strFileName, _T("rb"));
    MSDK_CHECK_POINTER(m_fSource, MFX_ERR_NULL_PTR);

    return MFX_ERR_NONE;
}

// read next part of data when requested
mfxStatus CSmplBitstreamReader::ReadNextFrame(mfxBitstream *pBS)
{
    MSDK_CHECK_POINTER(pBS, MFX_ERR_NULL_PTR);
    MSDK_CHECK_ERROR(m_fSource, NULL, MFX_ERR_NOT_INITIALIZED);

    mfxU32 nBytesRead = 0;

    memcpy(pBS->Data, pBS->Data + pBS->DataOffset, pBS->DataLength);
    pBS->DataOffset = 0;
    nBytesRead = (mfxU32)fread(pBS->Data + pBS->DataLength, 1, pBS->MaxLength - pBS->DataLength, m_fSource);

    if (0 == nBytesRead)
    {
        fseek(m_fSource,0,SEEK_SET);
        nBytesRead = (mfxU32)fread(pBS->Data + pBS->DataLength, 1, pBS->MaxLength - pBS->DataLength, m_fSource);
        if (0 == nBytesRead)
        {
            return MFX_ERR_MORE_DATA;
        }     
    }

    pBS->DataLength += nBytesRead;    

    return MFX_ERR_NONE;
}

//===== CDecodingPipeline iplementation ======
mfxStatus CDecodingPipeline::InitMfxParams()
{
    MSDK_CHECK_POINTER(m_pmfxDEC, MFX_ERR_NULL_PTR);
    mfxStatus sts = MFX_ERR_NONE;

    // try to find a sequence header in the stream
    // if header is not found this function exits with error (e.g. if device was lost and there's no header in the remaining stream)
    for(;;)
    {     
        // parse bit stream and fill mfx params
        sts = m_pmfxDEC->DecodeHeader(&m_mfxBS, &m_mfxDecodeVideoParams);

        if (MFX_ERR_MORE_DATA != sts)
            break;

        if (m_mfxBS.MaxLength == m_mfxBS.DataLength)
        {
            sts = ExtendMfxBitstream(&m_mfxBS, m_mfxBS.MaxLength * 2); 
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
        }
        // read a portion of data             
        sts = m_FileReader.ReadNextFrame(&m_mfxBS);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    }

    // check DecodeHeader status
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // specify memory type 
    m_mfxDecodeVideoParams.IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;

    return MFX_ERR_NONE;
}

// allocate pools of frames fro decoder and OCL plugin
mfxStatus CDecodingPipeline::AllocFrames()
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameAllocRequest DecoderRequest;
    mfxFrameAllocRequest OCLRequest;

    MSDK_CHECK_POINTER(m_pmfxDEC, MFX_ERR_NULL_PTR);
    MSDK_ZERO_MEMORY(DecoderRequest);

    // --- allocate frames for decoder
    // calculate number of surfaces required for decoder
    sts = m_pmfxDEC->QueryIOSurf(&m_mfxDecodeVideoParams, &DecoderRequest);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // prepare allocation request
    DecoderRequest.NumFrameMin = MSDK_MAX(DecoderRequest.NumFrameSuggested, 1);
    DecoderRequest.NumFrameSuggested = DecoderRequest.NumFrameMin+SYNC_BUF_SIZE;
    DecoderRequest.Info = m_mfxDecodeVideoParams.mfx.FrameInfo;
    //DecoderRequest.Type = MFX_MEMTYPE_INTERNAL_FRAME;
    DecoderRequest.Type = MFX_MEMTYPE_EXTERNAL_FRAME;
    DecoderRequest.Type |= MFX_MEMTYPE_FROM_DECODE; 
    DecoderRequest.Type |= MFX_MEMTYPE_FROM_VPPIN; 
    // add info about memory type to request 
    DecoderRequest.Type |= MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET; 

    // alloc frames for decoder
    sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, &DecoderRequest, &m_mfxDecoderResponse);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // prepare mfxFrameSurface1_OCL array for decoder
    m_pmfxDecodeSurfaces = new mfxFrameSurface1_OCL[m_mfxDecoderResponse.NumFrameActual];    
    for (int i = 0; i < m_mfxDecoderResponse.NumFrameActual; i++)
    {       
        memset(&(m_pmfxDecodeSurfaces[i]), 0, sizeof(mfxFrameSurface1_OCL));
        memcpy(&(m_pmfxDecodeSurfaces[i].Info), &(m_mfxDecodeVideoParams.mfx.FrameInfo), sizeof(mfxFrameInfo));
        m_pmfxDecodeSurfaces[i].Data.MemId = m_mfxDecoderResponse.mids[i];
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    }  

    // --- allocate frames for OCL
    // The number of surfaces for plugin output is SYNC_BUF_SIZE +1 for case
    OCLRequest.NumFrameMin = SYNC_BUF_SIZE;
    OCLRequest.NumFrameSuggested = OCLRequest.NumFrameMin+1;
    OCLRequest.Type = MFX_MEMTYPE_INTERNAL_FRAME;
    //OCLRequest.Type = MFX_MEMTYPE_EXTERNAL_FRAME;
    OCLRequest.Type |= MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET; 
    OCLRequest.Type |= MFX_MEMTYPE_FROM_VPPOUT; // THIS IS A WORKAROUND, NEED TO ADJUST ALLOCATOR
    OCLRequest.Info = m_mfxDecodeVideoParams.mfx.FrameInfo;
    // alloc frames for decoder
    sts = m_pMFXAllocator->Alloc(m_pMFXAllocator->pthis, &OCLRequest, &m_mfxOCLResponse);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // prepare mfxFrameSurface1_OCL array for OCL plugin
    m_pmfxOCLSurfaces = new mfxFrameSurface1_OCL[m_mfxOCLResponse.NumFrameActual];
    for (int i = 0; i < m_mfxOCLResponse.NumFrameActual; i++)
    {       
        memset(&(m_pmfxOCLSurfaces[i]), 0, sizeof(mfxFrameSurface1_OCL));
        memcpy(&(m_pmfxOCLSurfaces[i].Info), &(m_mfxDecodeVideoParams.mfx.FrameInfo), sizeof(mfxFrameInfo));
        m_pmfxOCLSurfaces[i].Data.MemId = m_mfxOCLResponse.mids[i];
        if(m_OCLStruct.m_intel_dx9_media_sharing)
        {
            IDirect3DSurface9* pS = (IDirect3DSurface9*)m_mfxOCLResponse.mids[i];
            HANDLE hNV12 = (HANDLE)m_mfxOCLResponse.mids[OCLRequest.NumFrameSuggested+i];
            OCL_SAFE_CALL_NORET(( m_pmfxOCLSurfaces[i].OCL_Y = m_OCLStruct.m_clCreateFromDX9MediaSurfaceINTEL(m_OCLStruct.m_clContext,0,pS,hNV12,0,&RET_STS),RET_STS));
            OCL_SAFE_CALL_NORET(( m_pmfxOCLSurfaces[i].OCL_UV = m_OCLStruct.m_clCreateFromDX9MediaSurfaceINTEL(m_OCLStruct.m_clContext,0,pS,hNV12,1,&RET_STS),RET_STS));
        }
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    }  

    return MFX_ERR_NONE;
}//CDecodingPipeline::AllocFrames



void CDecodingPipeline::ReleaseFrames()
{
    if(m_OCLStruct.m_intel_dx9_media_sharing)
    {
        for (int i = 0; i < m_mfxOCLResponse.NumFrameActual; i++)
        {
            m_OCLStruct.m_clEnqueueReleaseDX9ObjectsINTEL(m_OCLStruct.m_clCommandQueue,1,&(m_pmfxOCLSurfaces[i].OCL_Y),0,NULL,NULL);
            m_OCLStruct.m_clEnqueueReleaseDX9ObjectsINTEL(m_OCLStruct.m_clCommandQueue,1,&(m_pmfxOCLSurfaces[i].OCL_UV),0,NULL,NULL);
            OCL_SAFE_CALL_NORET(RET_STS = clFinish(m_OCLStruct.m_clCommandQueue));
            SAFE_FREE(m_pmfxOCLSurfaces[i].OCL_Y,clReleaseMemObject);
            SAFE_FREE(m_pmfxOCLSurfaces[i].OCL_UV,clReleaseMemObject);
        }
    }

    // delete surfaces 
    MSDK_SAFE_DELETE_ARRAY(m_pmfxDecodeSurfaces);    
    MSDK_SAFE_DELETE_ARRAY(m_pmfxOCLSurfaces);    
    if (m_pMFXAllocator)
    {        
        m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_mfxDecoderResponse);
        m_pMFXAllocator->Free(m_pMFXAllocator->pthis, &m_mfxOCLResponse);
    }

    return;
}//CDecodingPipeline::ReleaseFrames


// Constructor
CDecodingPipeline::CDecodingPipeline(IDirect3DDevice9* pd3dDevice):m_OCLStruct(pd3dDevice)
{
    m_pmfxDEC = NULL;
    m_pOCLPlugin = NULL;
    m_pMFXAllocator = NULL;
    m_pmfxDecodeSurfaces = NULL; 
    m_pmfxOCLSurfaces = NULL;



    if(m_OCLStruct.m_clContext==NULL)
    {
        printf("OCL fail\n"); 
        exit(0);
    }
    if(!m_OCLStruct.m_intel_dx9_media_sharing)
    {
        printf("OCL cl_intel_dx9_media_sharing is not working\n"); 
        exit(0);
    }

    MSDK_ZERO_MEMORY(m_mfxDecodeVideoParams);
    MSDK_ZERO_MEMORY(m_mfxDecoderResponse);
    MSDK_ZERO_MEMORY(m_mfxOCLResponse);
    MSDK_ZERO_MEMORY(m_mfxBS);

    // frequency of current CPU to calculate execution time
    LARGE_INTEGER       freq;
    QueryPerformanceFrequency(&freq);
    m_TicksPerMiliSecond = (double)freq.QuadPart / 1000.;

    // set that both decoder and OCL are working
    m_DECODEFlag = true;

    // initialize tasks pool
    memset(m_Tasks,0,sizeof(m_Tasks));
    m_TaskIndex = 0;

    HRESULT hr;
    UINT resetToken = 0;    
    m_pd3dDeviceManager = NULL;  
    m_resetToken        = 0;
    V(DXVA2CreateDirect3DDeviceManager9(&resetToken, &m_pd3dDeviceManager));
    V(m_pd3dDeviceManager->ResetDevice(pd3dDevice, resetToken));
    m_resetToken = resetToken;
}//CDecodingPipeline::CDecodingPipeline

CDecodingPipeline::~CDecodingPipeline()
{
    Close();
}

// initialize pipline
mfxStatus CDecodingPipeline::Init(sInputParams *pParams)
{
    mfxStatus sts = MFX_ERR_NONE;
    MSDK_CHECK_POINTER(pParams, MFX_ERR_NULL_PTR);

    // prepare input stream file reader
    sts = m_FileReader.Init(pParams->strSrcFile);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // init session
    mfxVersion version = {0, 1};// API version

    // try to init HW implementation if possible
    sts = m_mfxSession.Init(MFX_IMPL_AUTO, &version);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // create decoder
    m_pmfxDEC = new MFXVideoDECODE(m_mfxSession);    

    //create OCL plugin
    m_pOCLPlugin = new OCLPlugin(pParams->bOCLFlag_FastRelaxedMath);
    sts = m_pOCLPlugin->InitOCL(&m_OCLStruct);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // prepare bit stream
    sts = InitMfxBitstream(&m_mfxBS, 1024 * 1024);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);    

    // set video type in parameters
    m_mfxDecodeVideoParams.mfx.CodecId = pParams->videoType; 
    sts = InitMfxParams();
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // provide device manager to MediaSDK
    sts = m_mfxSession.SetHandle(MFX_HANDLE_D3D9_DEVICE_MANAGER, m_pd3dDeviceManager);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // create D3D allocator and set for session
    m_pMFXAllocator = new D3DFrameAllocator;        
    sts = m_mfxSession.SetFrameAllocator(m_pMFXAllocator);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // initialize memory allocator
    m_MFXAllocatorParams.pManager = m_pd3dDeviceManager;
    sts = m_pMFXAllocator->Init(&m_MFXAllocatorParams);

    // if allocator is provided to MediaSDK as external, frames must be allocated prior to decoder initialization
    // allocate frames for decoder and OCL plugin
    sts = AllocFrames();
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // init decoder
    sts = m_pmfxDEC->Init(&m_mfxDecodeVideoParams);
    if(sts == MFX_WRN_PARTIAL_ACCELERATION)
    {
        printf("Warning!!! Decoder returns MFX_WRN_PARTIAL_ACCELERATION\n");
    }
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    { // register plugin callbacks in Media SDK
        mfxVideoParam       OCLPluginVideoParams;
        MFXPluginAdapter    pluginAdapter(m_pOCLPlugin);

        sts = MFXVideoUSER_Register(m_mfxSession, 0, &pluginAdapter);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        // init OCL plugin
        MSDK_ZERO_MEMORY(OCLPluginVideoParams);
        OCLPluginVideoParams.AsyncDepth = SYNC_BUF_SIZE; // the maximum number of tasks that can be submitted before any task execution finishes
        OCLPluginVideoParams.vpp.In = m_mfxDecodeVideoParams.mfx.FrameInfo;        
        OCLPluginVideoParams.vpp.Out = m_mfxDecodeVideoParams.mfx.FrameInfo;        
        // need to call Init after registration because mfxCore interface is needed
        sts = m_pOCLPlugin->Init(&OCLPluginVideoParams, &m_pMFXAllocator, 0);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);     
    }

    PrintInfo(); 

    return MFX_ERR_NONE;
}//CDecodingPipeline::Init

void CDecodingPipeline::Close()
{
    WipeMfxBitstream(&m_mfxBS);
    MSDK_SAFE_DELETE(m_pmfxDEC);   

    MFXVideoUSER_Unregister(m_mfxSession, 0);        
    MSDK_SAFE_DELETE(m_pOCLPlugin);   

    ReleaseFrames();

    // delete allocator
    MSDK_SAFE_DELETE(m_pMFXAllocator);

    MSDK_SAFE_RELEASE(m_pd3dDeviceManager);

    m_FileReader.Close();
    m_mfxSession.Close();

    return;
}//CDecodingPipeline::Close

// decode next frame and put result to output surface
mfxStatus CDecodingPipeline::DecodeOneFrame(int Width, int Height, IDirect3DSurface9 *pDstSurface, IDirect3DDevice9* pd3dDevice)
{   

    // check if previouse submitted task exists
    if(m_Tasks[m_TaskIndex].m_DecodeSync || m_Tasks[m_TaskIndex].m_OCLSync)
    {// wait task is finished and copy result texture to back buffer
        mfxStatus   sts = MFX_ERR_NONE;
        mfxFrameSurface1_OCL*   pOutSurface = NULL; // output surface. 
        //wait the previous submitted tasks
        if(m_Tasks[m_TaskIndex].m_DecodeSync)
        {
            sts = m_mfxSession.SyncOperation(m_Tasks[m_TaskIndex].m_DecodeSync, MSDK_DEC_WAIT_INTERVAL);
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
            pOutSurface = m_Tasks[m_TaskIndex].m_pDecodeOutSurface;
        }
        if(m_Tasks[m_TaskIndex].m_OCLSync)
        {
            sts = m_mfxSession.SyncOperation(m_Tasks[m_TaskIndex].m_OCLSync, MSDK_VPP_WAIT_INTERVAL);
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
            pOutSurface = m_Tasks[m_TaskIndex].m_pOCLOutSurface;
        }
        if(pOutSurface)
        {/* copy YUV texture to screen */

            HRESULT hr;

            IDirect3DSurface9* pSrcSurface = (IDirect3DSurface9*)(pOutSurface->Data.MemId);
            assert(pDstSurface && pSrcSurface);
            if(pSrcSurface && pDstSurface)
            {
                RECT    r;
                r.left = 0;
                r.top = 0;
                r.right = min(Width,m_mfxDecodeVideoParams.vpp.In.Width);
                r.bottom = min(Height,m_mfxDecodeVideoParams.vpp.In.Height);

                r.right -= r.right&1;
                r.bottom -= r.bottom&1;

                V(pd3dDevice->StretchRect(pSrcSurface, &r, pDstSurface, &r,D3DTEXF_POINT));
            }

        }
#if (defined( __INTEL_COMPILER ) && !defined(WIN64))
        if(m_Tasks[m_TaskIndex].m_pDecodeOutSurface && m_Tasks[m_TaskIndex].m_pDecodeOutSurface->Data.Locked)
          _InterlockedDecrement16((void*)&m_Tasks[m_TaskIndex].m_pDecodeOutSurface->Data.Locked);
        if(m_Tasks[m_TaskIndex].m_pOCLOutSurface && m_Tasks[m_TaskIndex].m_pOCLOutSurface->Data.Locked)
            _InterlockedDecrement16((void*)&m_Tasks[m_TaskIndex].m_pOCLOutSurface->Data.Locked);
#else
        if(m_Tasks[m_TaskIndex].m_pDecodeOutSurface && m_Tasks[m_TaskIndex].m_pDecodeOutSurface->Data.Locked)
          _InterlockedDecrement16((volatile short*)&m_Tasks[m_TaskIndex].m_pDecodeOutSurface->Data.Locked);
        if(m_Tasks[m_TaskIndex].m_pOCLOutSurface && m_Tasks[m_TaskIndex].m_pOCLOutSurface->Data.Locked)
            _InterlockedDecrement16((volatile short*)&m_Tasks[m_TaskIndex].m_pOCLOutSurface->Data.Locked);
#endif

    }

    // clear sync task for further using
    m_Tasks[m_TaskIndex].m_OCLSync = 0;
    m_Tasks[m_TaskIndex].m_pOCLOutSurface = 0;
    m_Tasks[m_TaskIndex].m_DecodeSync = 0;
    m_Tasks[m_TaskIndex].m_pDecodeOutSurface = 0;



    if(m_DECODEFlag)
    {// feed decoder 
        mfxSyncPoint        DecodeSyncPoint = 0;
        static mfxU16      nDecoderSurfIndex = 0; // index of free surface   
        mfxStatus   sts = MFX_ERR_NONE;
        m_pmfxDecodeSurfaceLast = NULL; // reset curretn decoder surface to get new one from Decoder
        while(MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts || MFX_ERR_MORE_SURFACE == sts || MFX_WRN_DEVICE_BUSY == sts)          
        {// loop until decoder report that it get request for new frame
            if (MFX_WRN_DEVICE_BUSY == sts)
            {
                Sleep(1); // just wait and then repeat the same call to DecodeFrameAsync
            }
            else if (MFX_ERR_MORE_DATA == sts)
            { // read more data to input bit stream 
                sts = m_FileReader.ReadNextFrame(&m_mfxBS);
                MSDK_BREAK_ON_ERROR(sts);            
            }
            else if (MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE == sts)
            {// find new working-output surface in m_pmfxDecodeSurfaces
                //nDecoderSurfIndex = 0;
                nDecoderSurfIndex = GetFreeSurfaceIndex(m_pmfxDecodeSurfaces, m_mfxDecoderResponse.NumFrameActual,nDecoderSurfIndex); 
                if (MSDK_INVALID_SURF_IDX == nDecoderSurfIndex)
                {
                    return MFX_ERR_MEMORY_ALLOC;
                }
            }

            // send request to decoder
            sts = m_pmfxDEC->DecodeFrameAsync(
                &m_mfxBS, 
                (mfxFrameSurface1*)&(m_pmfxDecodeSurfaces[nDecoderSurfIndex]), 
                (mfxFrameSurface1**)&m_pmfxDecodeSurfaceLast, 
                &DecodeSyncPoint);
            // ignore warnings if output is available, 
            // if no output and no action required just repeat the same call
            if (MFX_ERR_NONE < sts && DecodeSyncPoint) 
            {
                sts = MFX_ERR_NONE;
            }

            if (MFX_ERR_NONE == sts)
            {// decoder return sync point then fill the curretn task nad switch to OCL Plugin feeding
                m_Tasks[m_TaskIndex].m_DecodeSync = DecodeSyncPoint;
                m_Tasks[m_TaskIndex].m_pDecodeOutSurface = m_pmfxDecodeSurfaceLast;
                // look for output process
#if (defined( __INTEL_COMPILER ) && !defined(WIN64))
                if(m_Tasks[m_TaskIndex].m_pDecodeOutSurface)
                    _InterlockedIncrement16((void*)&m_Tasks[m_TaskIndex].m_pDecodeOutSurface->Data.Locked);
#else
                if(m_Tasks[m_TaskIndex].m_pDecodeOutSurface)
                    _InterlockedIncrement16((volatile short*)&m_Tasks[m_TaskIndex].m_pDecodeOutSurface->Data.Locked);
#endif

                break;
            }
        }
        if(MFX_ERR_NONE != sts)
        {
            printf("Decoder return error %d!\n",sts);
        }
    }//if(m_DECODEFlag)

    if(m_pOCLPlugin && m_pOCLPlugin->m_OCLFlag)
    {// OPENCL part
        mfxSyncPoint        OCLSyncPoint = 0;
        mfxStatus   sts = MFX_ERR_NONE;
        // get index for output surface for OCL plugin
        mfxU16 nOCLSurfIndex = GetFreeSurfaceIndex(m_pmfxOCLSurfaces, m_mfxOCLResponse.NumFrameActual);
        MSDK_CHECK_ERROR(nOCLSurfIndex, MSDK_INVALID_SURF_IDX, MFX_ERR_MEMORY_ALLOC); 
        mfxHDL pOutSurf = &m_pmfxOCLSurfaces[nOCLSurfIndex];
        mfxHDL inp = m_pmfxDecodeSurfaceLast;

        // OCL filter
        for(;;)
        {
            sts = MFXVideoUSER_ProcessFrameAsync(m_mfxSession, &inp, 1, &pOutSurf, 1, &OCLSyncPoint); 

            if (MFX_WRN_DEVICE_BUSY == sts)
            {
                Sleep(1); // just wait and then repeat the same call
            }
            else
            {
                break;
            }
        }                           

        // ignore warnings if output is available, 
        if (MFX_ERR_NONE < sts && OCLSyncPoint) 
        {
            sts = MFX_ERR_NONE;
        }

        if(MFX_ERR_NONE!=sts) return sts;                 

        if (MFX_ERR_NONE == sts)
        {
            m_Tasks[m_TaskIndex].m_OCLSync = OCLSyncPoint;
            m_Tasks[m_TaskIndex].m_pOCLOutSurface = &m_pmfxOCLSurfaces[nOCLSurfIndex];
            // look for output process
#if (defined( __INTEL_COMPILER ) && !defined(WIN64))
            _InterlockedIncrement16((void*)&m_Tasks[m_TaskIndex].m_pOCLOutSurface->Data.Locked);
#else
            _InterlockedIncrement16((volatile short*)&m_Tasks[m_TaskIndex].m_pOCLOutSurface->Data.Locked);
#endif

        }            
    }

    // increase task index to point to next task. 
    m_TaskIndex = (m_TaskIndex+1)%SYNC_BUF_SIZE;
    return MFX_ERR_NONE;
}//CDecodingPipeline::DecodeOneFrame


// print information about initialized pipeline and input file
void CDecodingPipeline::PrintInfo()
{       
    //_tprintf(_T("Intel(R) Media SDK Version %s is used\n"), MSDK_SAMPLE_VERSION);
    mfxIMPL impl;
    m_mfxSession.QueryIMPL(&impl);
    TCHAR* sImpl = (MFX_IMPL_HARDWARE == impl) ? _T("hw") : _T("sw");
    _tprintf(_T("Decoder impl\t\t%s\n"), sImpl);

    _tprintf(_T("\nInput video\t%s\n"), CodecIdToStr(m_mfxDecodeVideoParams.mfx.CodecId));

    mfxFrameInfo Info = m_mfxDecodeVideoParams.mfx.FrameInfo;
    _tprintf(_T("Resolution\t%dx%d\n"), Info.Width, Info.Height);
    _tprintf(_T("Crop X,Y,W,H\t%d,%d,%d,%d\n"), Info.CropX, Info.CropY, Info.CropW, Info.CropH);

    mfxF64 dFrameRate = (Info.FrameRateExtN && Info.FrameRateExtD)?((mfxF64)Info.FrameRateExtN / Info.FrameRateExtD):0;
    _tprintf(_T("Frame rate\t%.2f\n"), dFrameRate);



    mfxVersion ver;
    m_mfxSession.QueryVersion(&ver);
    _tprintf(_T("MediaSDK version\t%d.%d\n"), ver.Major, ver.Minor);

    _tprintf(_T("\n"));

    return;
}