/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CDXRenderTarget.h
 *
 * @brief Declares classes for rendering targets
 */

#pragma once

#include <d3d10.h>
#include "Flux/CFlux2DSurface.h"

/**
 * @class CDXRTRestorer
 *
 * @brief Render Target View Restoration
 */
class CDXRTRestorer
{
public:
    
    /**
     * @fn CDXRTRestorer()
     *
     * @brief Class constructor
     *
     * @note Gets the current render targets and viewports from the output device
     */
    CDXRTRestorer();

    /**
     * @fn ~CDXRTRestorer()
     *
     * @brief Class destructor
     *
     * @note Sets the current render targets and viewports on the output device,
     *       releases all render target views and the depth stencil view
     */
    ~CDXRTRestorer();
private:

    /**
     * @enum MAX_RTVS
     *
     * @brief Defines the maximum number of render target views
     */
    enum 
    { 
        MAX_RTVS = 4 
    };

    /**
     * @var m_ppRTVs[MAX_RTVS]
     *
     * @brief Array of render target views
     */
    ID3D10RenderTargetView *m_ppRTVs[MAX_RTVS];

    /**
     * @var m_numViewports
     *
     * @brief The number of viewports
     */
    UINT                   m_numViewports;

    /**
     * @var m_viewports[MAX_RTVS]
     *
     * @brief An array of viewports
     */
    D3D10_VIEWPORT         m_viewports[MAX_RTVS];

    /**
     * @var m_pDSV
     *
     * @brief A depth stencil view
     */
    ID3D10DepthStencilView *m_pDSV;
};

/**
 * @class CDXRenderTarget
 *
 * @brief Maintains render target view, shader resource view and textures
 */
class CDXRenderTarget
{
public:

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
    CDXRenderTarget(int resW, int resH, DXGI_FORMAT fmt);

    /**
     * @fn ~CDXRenderTarget()
     *
     * @brief Class destructor, releases buffers and textures
     */
    ~CDXRenderTarget();

    /**
     * @fn GetRTV()
     *
     * @brief Used to obtain a pointer to the render target view
     *
     * @return - the Render Target View
     */
    ID3D10RenderTargetView* GetRTV()
    { 
        return m_pRTV; 
    }

        /* ****************************** DEPRECATED CODE *************************** *\
             //ID3D10RenderTargetView const * GetRTV() const { return m_pRTV; }
        \* ****************************** END DEPRECATED CODE *********************** */

    /**
     * @fn GetSRV()
     *
     * @brief Used to obtain a reference to the shader resource view
     *
     * @return - The shader resource view
     */
    ID3D10ShaderResourceView* GetSRV() 
    { 
        return m_pSRV; 
    }

    /**
     * @fn GetTexture()
     *
     * @brief Retrieves the texture
     *
     * @return - The texture
     */
    ID3D10Texture2D* GetTexture()
    {
        return m_pTexture;
    }

    /**
     * @fn GetTextureCPU()
     *
     * @brief Retrieves the CPU version of the texture
     *
     * @return - The CPU texture
     */
    ID3D10Texture2D* GetTextureCPU()
    {
        return m_pTextureCPU;
    }

    /**
     * @fn GetWidth()
     *
     * @brief Used to find the width of the resource
     *
     * @return - The width
     */
    int GetWidth() const 
    { 
        return m_width; 
    }

    /**
     * @fn GetHeight()
     *
     * @brief Used to find the height of the resource
     *
     * @return - The height
     */
    int GetHeight() const 
    {
        return m_height; 
    }

    /**
     * @fn SetIt()
     *
     * @brief Sets the device render target views and the viewport on the output device
     */
    void SetIt();

    /**
     * @fn Resize (int w, int h)
     *
     * @brief Resizes the render target, creates new textures and views 
     *
     * @param w - New resource width
     *
     * @param h - New resourece height
     */
    void Resize(int w, int h);

protected:

    /**
     * @var m_fmt
     *
     * @brief Buffer content format
     */
    DXGI_FORMAT                 m_fmt;

    /**
     * @var m_width
     *
     * @brief Width of the target
     */
    int                         m_width;

    /**
     * @var m_height
     *
     * @brief Height of the target
     */
    int                         m_height;

    /**
     * @var m_pTexture
     *
     * @brief The target texture
     */
    ID3D10Texture2D*            m_pTexture;

    /**
     * @var m_pTextureCPU
     *
     * @brief CPU copy of the texture
     *
     */
    ID3D10Texture2D*            m_pTextureCPU;

    /**
     * @var m_pSRV
     *
     * @brief The shader resource view
     */
    ID3D10ShaderResourceView*   m_pSRV;

    /**
     * @var m_pRTV
     *
     * @brief The render target view
     */
    ID3D10RenderTargetView*     m_pRTV;
};
