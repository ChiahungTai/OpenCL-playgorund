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

   
#ifndef __PIPELINE_DECODE_H__
#define __PIPELINE_DECODE_H__

#include <tchar.h>
#include <windows.h>
#include "DXUT.h"

#include "mfxstructures.h"
#include "mfxmvc.h"
#include "mfxvideo.h"
#include "mfxvideo++.h"

#include "OCLStruct.h"
#include "sample_opencl_plugin.h"
#include "sample_defs.h"
#include "base_allocator.h"

#pragma warning(disable : 4201)
#include <d3d9.h>
#include <dxva2api.h>
#include "d3d_allocator.h"


// this class implemented the reaer raw data from file and used to load raw data to memory for further decoding
class CSmplBitstreamReader
{
public :

    CSmplBitstreamReader();
    virtual ~CSmplBitstreamReader();

    virtual void      Close();
    // initialize reader to read given file
    virtual mfxStatus Init(const TCHAR *strFileName);
    // read next part of data when requested
    virtual mfxStatus ReadNextFrame(mfxBitstream *pBS);

protected:
    // opened handle of read file
    FILE*     m_fSource;
};

// structure to transfer decoding pipeline parameters for initialization
struct sInputParams
{
    mfxU32 videoType; // type of input stream
    bool   bd3dAlloc; // true - frames in video memory (d3d surfaces),  false - in system memory

    TCHAR  strSrcFile[MSDK_MAX_FILENAME_LEN]; // name of source file
    bool   bOCLFlag_FastRelaxedMath;// set true to use fast relaxed math to compile OCL program
};

// class that implement whole processing pipeline
// 0. allocate buffers for decoder and OCL plugin
// 1. read data from file
// 2. feed decoder by readed data
// 3. feed OCL plugin by decoded frame
class CDecodingPipeline
{
public:

    IDirect3DDeviceManager9* m_pd3dDeviceManager;
    UINT                     m_resetToken;

    CDecodingPipeline(IDirect3DDevice9* pd3dDevice);
    virtual ~CDecodingPipeline();
    // initialize pipline
    virtual mfxStatus Init(sInputParams *pParams);
    // close pipeline
    virtual void Close();
    // print information about initialized pipeline and input file
    virtual void PrintInfo();
    // decode next frame and put result to output surface
    mfxStatus DecodeOneFrame(int width, int height,IDirect3DSurface9 *pDstSurface,IDirect3DDevice9* pd3dDevice);


    // number of CPU tics in one millisecond. it used to calculate execution time by ticks
    double              m_TicksPerMiliSecond;
    // Average time of OCL plugin execution
    double              m_AverTimeOCL;
    // flag that indicates that MEdiaSDK decoder should decode new frame 
    bool                m_DECODEFlag;

    OCLStruct           m_OCLStruct; //OCL data
    // pointer to OCL MediaSDK plugin. this pointer is public to be able direct control from the CSample.
    OCLPlugin*          m_pOCLPlugin;
    // parameter of curretn processed video frame (size, type etc.)
    mfxVideoParam       m_mfxDecodeVideoParams; 

protected:
    CSmplBitstreamReader    m_FileReader;// file reader to mfxBitstream
    mfxBitstream            m_mfxBS; // contains data for decoding

    MFXVideoSession         m_mfxSession; // MediaSDK session
    MFXVideoDECODE*         m_pmfxDEC;// pointer to MediaSDK decoder

    D3DFrameAllocator*      m_pMFXAllocator;// frame allocator
    D3DAllocatorParams      m_MFXAllocatorParams;// allocator parameters

    mfxFrameAllocResponse   m_mfxDecoderResponse;  // memory allocation response for decoder  
    mfxFrameSurface1_OCL*   m_pmfxDecodeSurfaces; // frames array for decoder output
    mfxFrameSurface1_OCL*   m_pmfxDecodeSurfaceLast;// current decoded frame. this frame is reused if m_DECODEFlag is false
    
    mfxFrameAllocResponse   m_mfxOCLResponse;  // memory allocation response for OCL plugin  
    mfxFrameSurface1_OCL*   m_pmfxOCLSurfaces; // frames array for OCL plugin output

// define depth of async pipeline. this is maximum number of frames that could be fetched before waiting on total sync
#define SYNC_BUF_SIZE   1 
    struct 
    {
        mfxSyncPoint            m_DecodeSync;   // Decoder sync point
        mfxSyncPoint            m_OCLSync;      // OCL sync point
        mfxFrameSurface1_OCL*   m_pDecodeOutSurface;// pointer to frame that containes decoded frame
        mfxFrameSurface1_OCL*   m_pOCLOutSurface;// pointer to frame that contains OCL processed frame
    } m_Tasks[SYNC_BUF_SIZE]; // array of tasks. Each task is set of all processing stages(decode and OCL processing)
    int m_TaskIndex;    // index of current task
    
    // read file header and recognize video and decoder parameters
    virtual mfxStatus InitMfxParams();
    // allocate pools of frames fro decoder and OCL plugin
    virtual mfxStatus AllocFrames();
    // Release allocate frames
    virtual void ReleaseFrames();         
};

#endif // __PIPELINE_DECODE_H__ 