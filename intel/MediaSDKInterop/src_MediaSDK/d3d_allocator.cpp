/* ****************************************************************************** *\

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2008-2011 Intel Corporation. All Rights Reserved.

\* ****************************************************************************** */

#include <objbase.h>
#include <initguid.h>
#include <assert.h>
#include <d3d9.h>
#include "DXUT.h"

#include "d3d_allocator.h"

#define D3DFMT_NV12 (D3DFORMAT)MAKEFOURCC('N','V','1','2')
#define D3DFMT_YV12 (D3DFORMAT)MAKEFOURCC('Y','V','1','2')

D3DFORMAT ConvertMfxFourccToD3dFormat(mfxU32 fourcc)
{
    switch (fourcc)
    {
    case MFX_FOURCC_NV12:
        return D3DFMT_NV12;
    case MFX_FOURCC_YV12:
        return D3DFMT_YV12;
    case MFX_FOURCC_YUY2:
        return D3DFMT_YUY2;
    case MFX_FOURCC_RGB3:
        return D3DFMT_R8G8B8;
    case MFX_FOURCC_RGB4:
        return D3DFMT_A8R8G8B8;
    case mfxU32(D3DFMT_P8):
        return D3DFMT_P8;
    default:
        return D3DFMT_UNKNOWN;
    }
}

class DeviceHandle
{
public:
    DeviceHandle(IDirect3DDeviceManager9* manager)
        : m_manager(manager)
        , m_handle(0)
    {
        if (manager)
        {
            HRESULT hr = manager->OpenDeviceHandle(&m_handle);
            if (FAILED(hr))
                m_manager = 0;
        }        
    }

    ~DeviceHandle()
    {
        if (m_manager && m_handle)
            m_manager->CloseDeviceHandle(m_handle);
    }

    HANDLE Detach()
    {
        HANDLE tmp = m_handle;
        m_manager = 0;
        m_handle = 0;
        return tmp;
    }

    operator HANDLE()
    {
        return m_handle;
    }

    bool operator !() const
    {
        return m_handle == 0;
    }

protected:
    CComPtr<IDirect3DDeviceManager9> m_manager;
    HANDLE m_handle;
};

D3DFrameAllocator::D3DFrameAllocator()
: m_decoderService(0), m_processorService(0), m_hDecoder(0), m_hProcessor(0), m_manager(0)
{    
}

D3DFrameAllocator::~D3DFrameAllocator()
{
    Close();
}

mfxStatus D3DFrameAllocator::Init(mfxAllocatorParams *pParams)
{   
    D3DAllocatorParams *pd3dParams = 0;
    pd3dParams = dynamic_cast<D3DAllocatorParams *>(pParams);
    if (!pd3dParams)
        return MFX_ERR_NOT_INITIALIZED;
   
    m_manager = pd3dParams->pManager;

    return MFX_ERR_NONE;    
}

mfxStatus D3DFrameAllocator::Close()
{   
    if (m_manager && m_hDecoder)
    {
        m_manager->CloseDeviceHandle(m_hDecoder);
        m_manager = 0;
        m_hDecoder = 0;
    }

    if (m_manager && m_hProcessor)
    {
        m_manager->CloseDeviceHandle(m_hProcessor);
        m_manager = 0;
        m_hProcessor = 0;
    }

    return BaseFrameAllocator::Close();
}

mfxStatus D3DFrameAllocator::LockFrame(mfxMemId mid, mfxFrameData *ptr)
{
    IDirect3DSurface9 *pSurface = (IDirect3DSurface9 *)mid;
    if (pSurface == 0)
        return MFX_ERR_INVALID_HANDLE;

    if (ptr == 0)
        return MFX_ERR_LOCK_MEMORY;

    D3DSURFACE_DESC desc;
    HRESULT hr = pSurface->GetDesc(&desc);
    if (FAILED(hr))
        return MFX_ERR_LOCK_MEMORY;

    if (desc.Format != D3DFMT_NV12 &&
        desc.Format != D3DFMT_YV12 &&
        desc.Format != D3DFMT_YUY2 &&
        desc.Format != D3DFMT_R8G8B8 &&
        desc.Format != D3DFMT_A8R8G8B8 &&
        desc.Format != D3DFMT_P8)
        return MFX_ERR_LOCK_MEMORY;

    D3DLOCKED_RECT locked;

    hr = pSurface->LockRect(&locked, 0, D3DLOCK_NOSYSLOCK);
    if (FAILED(hr))
        return MFX_ERR_LOCK_MEMORY;

    switch ((DWORD)desc.Format)
    {
    case D3DFMT_NV12:
        ptr->Pitch = (mfxU16)locked.Pitch;
        ptr->Y = (mfxU8 *)locked.pBits;
        ptr->U = (mfxU8 *)locked.pBits + desc.Height * locked.Pitch;
        ptr->V = ptr->U + 1;
        break;
    case D3DFMT_YV12:
        ptr->Pitch = (mfxU16)locked.Pitch;
        ptr->Y = (mfxU8 *)locked.pBits;
        ptr->V = ptr->Y + desc.Height * locked.Pitch;
        ptr->U = ptr->V + (desc.Height * locked.Pitch) / 4;
        break;
    case D3DFMT_YUY2:
        ptr->Pitch = (mfxU16)locked.Pitch;
        ptr->Y = (mfxU8 *)locked.pBits;
        ptr->U = ptr->Y + 1;
        ptr->V = ptr->Y + 3;
        break;
    case D3DFMT_R8G8B8:
        ptr->Pitch = (mfxU16)locked.Pitch;
        ptr->B = (mfxU8 *)locked.pBits;
        ptr->G = ptr->B + 1;
        ptr->R = ptr->B + 2;
        break;
    case D3DFMT_A8R8G8B8:
        ptr->Pitch = (mfxU16)locked.Pitch;
        ptr->B = (mfxU8 *)locked.pBits;
        ptr->G = ptr->B + 1;
        ptr->R = ptr->B + 2;
        ptr->A = ptr->B + 3;
        break;
    case D3DFMT_P8:
        ptr->Pitch = (mfxU16)locked.Pitch;
        ptr->Y = (mfxU8 *)locked.pBits;
        ptr->U = 0;
        ptr->V = 0;
        break;
    }

    return MFX_ERR_NONE;
}

mfxStatus D3DFrameAllocator::UnlockFrame(mfxMemId mid, mfxFrameData *ptr)
{
    IDirect3DSurface9 *pSurface = (IDirect3DSurface9 *)mid;
    if (pSurface == 0)
        return MFX_ERR_INVALID_HANDLE;

    pSurface->UnlockRect();
    
    if (NULL != ptr)
    {
        ptr->Pitch = 0;
        ptr->Y     = 0;
        ptr->U     = 0;
        ptr->V     = 0;
    }

    return MFX_ERR_NONE;
}

mfxStatus D3DFrameAllocator::GetFrameHDL(mfxMemId mid, mfxHDL *handle)
{
    if (handle == 0)
        return MFX_ERR_INVALID_HANDLE;

    *handle = mid;
    return MFX_ERR_NONE;
}

mfxStatus D3DFrameAllocator::CheckRequestType(mfxFrameAllocRequest *request)
{    
    mfxStatus sts = BaseFrameAllocator::CheckRequestType(request);
    if (MFX_ERR_NONE != sts)
        return sts;

    if ((request->Type & (MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET | MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET)) != 0)
        return MFX_ERR_NONE;
    else
        return MFX_ERR_UNSUPPORTED;
}

mfxStatus D3DFrameAllocator::ReleaseResponse(mfxFrameAllocResponse *response)
{
    if (!response)
        return MFX_ERR_NULL_PTR;

    mfxStatus sts = MFX_ERR_NONE;

    if (response->mids)
    {
        for (mfxU32 i = 0; i < response->NumFrameActual; i++)
        {
            if (response->mids[i])
            {
                IDirect3DSurface9* handle = 0;
                sts = GetFrameHDL(response->mids[i], (mfxHDL *)&handle);
                if (MFX_ERR_NONE != sts)
                    return sts;
                handle->Release();
            }
        }        
    }

    delete [] response->mids;
    response->mids = 0;
    
    return sts;
}

mfxStatus D3DFrameAllocator::AllocImpl(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response)
{
    HRESULT hr = S_OK;

    D3DFORMAT format = ConvertMfxFourccToD3dFormat(request->Info.FourCC);

    if (format == D3DFMT_UNKNOWN)
        return MFX_ERR_UNSUPPORTED;
    
    safe_array<mfxMemId> mids(new mfxMemId[request->NumFrameSuggested*2]);
    if (!mids.get())
        return MFX_ERR_MEMORY_ALLOC;

    memset(mids.get(),0,sizeof(mfxMemId)*request->NumFrameSuggested*2);

    // VPP may require at input/output surfaces with color format other than NV12 (in case of color conversion)
    // VideoProcessorService must used to create such surfaces
    if (((request->Type & (MFX_MEMTYPE_FROM_VPPIN | MFX_MEMTYPE_FROM_VPPOUT)) && (MFX_FOURCC_NV12 != request->Info.FourCC))||
        (request->Type & (MFX_MEMTYPE_FROM_VPPIN | MFX_MEMTYPE_FROM_VPPOUT)) && (request->Type & MFX_MEMTYPE_INTERNAL_FRAME ))
    {
        if (!m_hProcessor)
        {
            DeviceHandle device = DeviceHandle(m_manager);

            if (!device)
                return MFX_ERR_MEMORY_ALLOC;

            CComPtr<IDirectXVideoProcessorService> service = 0;

            hr = m_manager->GetVideoService(device, IID_IDirectXVideoProcessorService, (void**)&service);

            if (FAILED(hr))
                return MFX_ERR_MEMORY_ALLOC;

            m_processorService = service;
            m_hProcessor = device.Detach();

        }

        for(int i=0; i<request->NumFrameSuggested;++i)
        {
            V(m_processorService->CreateSurface(
                request->Info.Width,
                request->Info.Height,
                0,//request->NumFrameSuggested - 1,
                format,
                D3DPOOL_DEFAULT,
                0,
                DXVA2_VideoProcessorRenderTarget,
                (IDirect3DSurface9 **)mids.get()+i,
                (HANDLE*)(mids.get() + i+request->NumFrameSuggested)));
        }
    }
    else
    {
        if (!m_hDecoder)
        {
            DeviceHandle device = DeviceHandle(m_manager);

            if (!device)
                return MFX_ERR_MEMORY_ALLOC;

            CComPtr<IDirectXVideoDecoderService> service = 0;

            hr = m_manager->GetVideoService(device, IID_IDirectXVideoDecoderService, (void**)&service);

            if (FAILED(hr))
                return MFX_ERR_MEMORY_ALLOC;

            m_decoderService = service;
            m_hDecoder = device.Detach();
        }
        V(m_decoderService->CreateSurface(
            request->Info.Width,
            request->Info.Height,
            request->NumFrameSuggested - 1,
            format,
            D3DPOOL_DEFAULT,
            0,
            DXVA2_VideoDecoderRenderTarget,
            (IDirect3DSurface9 **)mids.get(),
            //(HANDLE*)(mids.get() + request->NumFrameSuggested)
            NULL
            ));
    }


    if (FAILED(hr))
    {        
        return MFX_ERR_MEMORY_ALLOC;
    }

#if 0 // optional, may be done on application level if needed
    //zeroing created surfaces
    for (mfxU32 i = 0; i < request->NumFrameSuggested; i++)
    {
        mfxFrameData pData;
        if (MFX_ERR_NONE == LockFrame(mids.get()[i], &pData))
        {
            if (format == D3DFMT_NV12 ||
                format == D3DFMT_YV12)
            {
                memset(pData.Y, 0, (pData.Pitch * request->Info.Height * 3)/2 );
            }
            UnlockFrame(mids.get()[i], &pData);
        }
    }
#endif

    response->mids = mids.release();
    response->NumFrameActual = request->NumFrameSuggested;

    return MFX_ERR_NONE;
}
