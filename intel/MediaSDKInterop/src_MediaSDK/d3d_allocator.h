/* ****************************************************************************** *\

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2008-2011 Intel Corporation. All Rights Reserved.

\* ****************************************************************************** */

#ifndef __D3D_ALLOCATOR_H__
#define __D3D_ALLOCATOR_H__

#include <atlbase.h>
#include <dxva2api.h>
#include "base_allocator.h"

enum eTypeHandle
{
    DXVA2_PROCESSOR     = 0x00,
    DXVA2_DECODER       = 0x01
};

struct D3DAllocatorParams : mfxAllocatorParams
{
    IDirect3DDeviceManager9 *pManager;
};

class D3DFrameAllocator: public BaseFrameAllocator
{
public:
    D3DFrameAllocator();
    virtual ~D3DFrameAllocator();    

    virtual mfxStatus Init(mfxAllocatorParams *pParams);
    virtual mfxStatus Close();

    virtual IDirect3DDeviceManager9* GetDeviceManager()
    {
        return m_manager;
    };

protected:
    virtual mfxStatus LockFrame(mfxMemId mid, mfxFrameData *ptr);
    virtual mfxStatus UnlockFrame(mfxMemId mid, mfxFrameData *ptr);
    virtual mfxStatus GetFrameHDL(mfxMemId mid, mfxHDL *handle);       

    virtual mfxStatus CheckRequestType(mfxFrameAllocRequest *request);
    virtual mfxStatus ReleaseResponse(mfxFrameAllocResponse *response);
    virtual mfxStatus AllocImpl(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response);
   
    CComPtr<IDirect3DDeviceManager9> m_manager;
    CComPtr<IDirectXVideoDecoderService> m_decoderService;
    CComPtr<IDirectXVideoProcessorService> m_processorService;
    HANDLE m_hDecoder;
    HANDLE m_hProcessor;
};

#endif // __D3D_ALLOCATOR_H__