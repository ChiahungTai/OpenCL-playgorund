/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */


/**
 * @file CDXRenderTarget.cpp
 *
 * @brief Implements classes for rendering targets
 */

#include <DXUT.h>
#include "CDXRenderTarget.h"

/**
 * @fn CDXRTRestorer()
 *
 * @brief Class constructor
 *
 * @note Gets the current render targets and viewports from the output device
 */
CDXRTRestorer::CDXRTRestorer()
{
    DXUTGetD3D10Device()->OMGetRenderTargets(MAX_RTVS, m_ppRTVs, &m_pDSV);
    m_numViewports = MAX_RTVS;
    DXUTGetD3D10Device()->RSGetViewports(&m_numViewports, m_viewports);
}

/**
 * @fn ~CDXRTRestorer()
 *
 * @brief Class destructor
 *
 * @note Sets the current render targets and viewports on the output device,
 *       releases all render target views and the depth stencil view
 */
CDXRTRestorer::~CDXRTRestorer()
{
    DXUTGetD3D10Device()->OMSetRenderTargets(MAX_RTVS, m_ppRTVs, m_pDSV);
    DXUTGetD3D10Device()->RSSetViewports(m_numViewports, m_viewports);
    for (int i = 0; i < MAX_RTVS; i++)
    {
        SAFE_RELEASE(m_ppRTVs[i]);
    }
    SAFE_RELEASE(m_pDSV);
}


/**
 * @fn CDXRenderTarget(int resW, int resH, DXGI_FORMAT fmt)
 *
 * @brief Class constructor, sizes the target, initializes textures and views to NULL
 *
 * @param resW - Width of the resource
 *
 * @param resH - Height of the resource
 *
 * @param fmt - Buffer format
 */
CDXRenderTarget::CDXRenderTarget(int resW, int resH, DXGI_FORMAT fmt)
{
    m_fmt = fmt;
    m_pSRV = NULL;
    m_pRTV = NULL;
    m_pTexture = NULL;
    m_pTextureCPU = NULL;

    Resize(resW,resH);
}

/**
 * @fn SetIt()
 *
 * @brief Sets the device render target views and the viewport on the output device
 */
void CDXRenderTarget::SetIt()
{
    // Set the device render targets
    DXUTGetD3D10Device()->OMSetRenderTargets(1, &m_pRTV, NULL);
    // Define the viewport
    D3D10_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width  = m_width;
    viewport.Height = m_height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    // Then set it.
    DXUTGetD3D10Device()->RSSetViewports(1, &viewport);
}

/**
 * @fn ~CDXRenderTarget()
 *
 * @brief Class destructor, releases buffers and textures
 */
CDXRenderTarget::~CDXRenderTarget()
{
    SAFE_RELEASE(m_pSRV);
    SAFE_RELEASE(m_pRTV);
    SAFE_RELEASE(m_pTexture);
    SAFE_RELEASE(m_pTextureCPU);
}

/**
 * @fn Resize (int w, int h)
 *
 * @brief Resizes the render target, creates new textures and views 
 *
 * @param w - New resource width
 *
 * @param h - New resource height
 */
void CDXRenderTarget::Resize(int resW, int resH)
{
    HRESULT hr;

    SAFE_RELEASE(m_pSRV);
    SAFE_RELEASE(m_pRTV);
    SAFE_RELEASE(m_pTexture);
    SAFE_RELEASE(m_pTextureCPU); 
    
    // Save the new values
    m_width = resW;
    m_height = resH;

    // Get the device
    ID3D10Device *pDev = DXUTGetD3D10Device();

    // Create the texture here
    CD3D10_TEXTURE2D_DESC desc(m_fmt, resW, resH, 1, 1);
    desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
    desc.Usage     = D3D10_USAGE_DEFAULT;
    hr = pDev->CreateTexture2D(&desc, NULL, &m_pTexture);
    assert(hr == S_OK);

    // Create the CPU version of the texture here
    CD3D10_TEXTURE2D_DESC descCPU(m_fmt, resW, resH, 1, 1);
    descCPU.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
    descCPU.Usage          = D3D10_USAGE_DYNAMIC;
    hr = pDev->CreateTexture2D(&descCPU, NULL, &m_pTextureCPU);
    assert(hr == S_OK);

    // Create the shader resource view
    D3D10_SHADER_RESOURCE_VIEW_DESC resDesc;
    resDesc.Format = m_fmt;
    resDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
    resDesc.Texture2D.MipLevels = 1;
    resDesc.Texture2D.MostDetailedMip = 0;
    hr = pDev->CreateShaderResourceView(m_pTexture, &resDesc, &m_pSRV);
    assert(hr == S_OK);

    // Create the render target view
    D3D10_RENDER_TARGET_VIEW_DESC rtDesc;
    rtDesc.Format = m_fmt;
    rtDesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
    rtDesc.Texture2D.MipSlice = 0;
    
    hr = pDev->CreateRenderTargetView(m_pTexture, &rtDesc, &m_pRTV);
    assert(hr == S_OK);

} // End of resize()

