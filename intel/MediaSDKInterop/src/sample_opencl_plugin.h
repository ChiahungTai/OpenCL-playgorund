
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
#include <mfxplugin++.h>
#include "mfxdefs.h"
#include "windows.h"
#include "OCLStruct.h"

#pragma warning(disable : 4201)
#include <d3d9.h>
#include <dxva2api.h>
#include "d3d_allocator.h"


/* Frame Surface that contain OCL objects that share the Frame */
typedef struct _mfxFrameSurface1_OCL:mfxFrameSurface1{
    cl_mem           OCL_Y;
    cl_mem           OCL_UV;
} mfxFrameSurface1_OCL;


// task structure for OCL plugin
typedef struct {
    mfxFrameSurface1_OCL*   In;         // input surface
    mfxFrameSurface1_OCL*   Out;        // output surface
    mfxFrameSurface1_OCL    InBuffer;   // intermidiate buffer between decoder and OCL plungin
    int                     BufferSrcPitch; // number of pixels in 1 line for src buffer
    int                     BufferDstPitch; // number of pixels in 1 line for src buffer
    cl_event                clEventStop;// event to control when all OCL kernels were finished.
    bool                    bBusy;          // flag that indicate that task is not free
    LARGE_INTEGER           ticks[2];       // start strop ticks to calc OCL execution time
} MFXTask;

//maximum number of program (*.cl files) that could be loaded by plugin 
#define MAX_PROGRAM_NUM 128

// OCLplugin that load OCL program from *.cl filed and execute it to process YUV frame
class OCLPlugin : public MFXPlugin
{
public:
    OCLPlugin(bool FastRelaxedMath=false);
    ~OCLPlugin();


    // === function that are called by MediaSDK
    mfxStatus mfxPluginInit(mfxCoreInterface *core);
    mfxStatus mfxPluginClose();
    mfxStatus mfxGetPluginParam(mfxPluginParam *par);
    // called to submit task but not execute it
    mfxStatus mfxSubmit(const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxThreadTask *task);
    // function that called to start task execution and check status of execution
    mfxStatus mfxExecute(mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a);
    // free task and releated resources
    mfxStatus mfxFreeResources(mfxThreadTask task, mfxStatus sts);

    // == some parameters for OCL program
    int     m_Param1; // first parameter for OCL program
    int     m_Param2;// second parameter for OCL program
    int     m_MouseX;// OCL program parameter that is X coordinate of current mouse position
    int     m_MouseY;// OCL program parameter that is Y coordinate of current mouse position
    int     m_ButtonFlag;// OCL program parameter that indecates mouse button status
    // execution times
    float   m_TimeAver;// average time of OCL program execution

    // pointer to OCL stuff like programms, devices, queue and so on
    OCLStruct* m_pOCLStruct;
    
    // some internal OCL buffer to store internal programm data.
    // this buffer is cleared when program is changed
    cl_mem  m_OCLBuffer;
    int     m_OCLBufferSize;
    bool    m_FlagFastRelaxedMath; // flag that indicate that OCL program have to be compiled with fast-relaxed-math flag
    
    BaseFrameAllocator* m_pMFXFrameAllocator; // frame allocator to lock unlock buffers, this is just reference to allocator, do not create or delete it

    // flag that indicates that OCL plugin should process decoded frame 
    bool                m_OCLFlag;

    // init OCL stuff for given platform
    mfxStatus InitOCL(OCLStruct* pOCLStruct=NULL);
    
    // check if program was changed on the disk int he *.cl file 
    // and if it is changed then reload and recompile programm
    mfxStatus UpdateProgram(int  index=-1);
    // check if curretn folder is changed by adding or deleting *.cl files and 
    // if it is changed then rebuiild list of program
    // return true if program was updated.
    bool UpdateProgramList();
    // remove all OCL program from list
    void ClearProgramList(); 

    // set current programm that will be executed for video frame
    void    SetProgram(int index);
    // return index of currently executed OCL programm
    int     GetProgram()
    {
        return m_ProgramIndex<m_ProgramNum?m_ProgramIndex:-1;
    };
    // return name of currently executed or given program
    WCHAR*   GetProgramName(int index=-1);
    // return source code of currently executed or given program
    char*   GetProgramSRC(int index=-1);
    // return build log of currently executed or given program
    char*   GetProgramBuildLOG(int index=-1);

    // initialize OCL plugin 
    // mfxParam contains input and output video parameters (resolution, etc).
    // auxParam contains additional parameters (optional)
    mfxStatus Init(mfxVideoParam *mfxParam, void* auxParam, int auxParamSize);
    // close OCL plugin
    mfxStatus Close();

protected:
    // working function that feeds OCL queue by tasks
    mfxStatus EnqueueOCLTasks(MFXTask *pTask);
    // return curretn status of task MFX_TASK_WORKING or MFX_TASK_DONE.
    mfxStatus QueryStatus(MFXTask *pTask);

    // === MediaSDK stuff
    bool                m_bInited; //flag that indicate that OCLplugin already initialized
    mfxCoreInterface*   m_pmfxCore;// pointer to mfxCoreInterface. used to increase-decrease reference for in-out frames
    mfxPluginParam      m_mfxPluginParam;
    mfxVideoParam       m_mfxVideoParam;// parameters of in/out video frames
    MFXTask             *m_pTasks; // task pool to run OCL programm.
    int                 m_MaxNumTasks;// task pool size
    int                 m_PictureW;// width of processed picture
    int                 m_PictureH;// height of processed picture
    size_t              m_PictureBytesY;// input\output buffer size for Y component
    size_t              m_PictureBytesUV;// input\output buffer size for color components
    mfxFrameAllocResponse m_mfxOCLINResponse;  // memory allocation response for OCL plugin input

    // === OCL stuff
    int                 m_ProgramNum; // number of loaded programs
    WCHAR               m_ProgramName[MAX_PROGRAM_NUM][MAX_PATH]; // names of CL programms
    int                 m_ProgramIndex; // current program
    char*               m_pProgramSource[MAX_PROGRAM_NUM];// test of programs
    char*               m_pProgramBuildLOG[MAX_PROGRAM_NUM];// text of build log for programms
    cl_program          m_clProgram[MAX_PROGRAM_NUM]; // program id
    
    cl_kernel           m_clKernelMouse[MAX_PROGRAM_NUM];//OCL kernel that executed once per frame to transfer mouse position
    cl_kernel           m_clKernelProcessY[MAX_PROGRAM_NUM];//OCL kernel that process each Y component for each pixel
    cl_kernel           m_clKernelProcessUV[MAX_PROGRAM_NUM];//OCL kernel that process UV components for each 4 neighborhood pixels
    
    size_t              m_GlobalWorkSizeY[2];// global sizes to process Y components
    size_t              m_LocalWorkSizeY[2];// local sizes to process Y components
    size_t              m_GlobalWorkSizeUV[2];// global sizes to process UV components
    size_t              m_LocalWorkSizeUV[2];// local sizes to process UV components

    HANDLE              m_hDevice; // D3D device handle

};
