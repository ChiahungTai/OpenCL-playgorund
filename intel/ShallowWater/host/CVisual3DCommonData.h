/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CVisual3DCommonData.h
 *
 * @brief Contains declarations for common visualization routines 
 */

#pragma once

#include <d3d10.h>


#include "CL\cl.h"


/**
 * @class CVisual3DCommonData
 *
 * @brief Container for all shader resource views in the sample
 */
class CVisual3DCommonData
{
public:

    /**
     * @fn CVisual3DCommonData()
     *
     * @brief Class constructor
     *
     * @note This method loads the shader resource views for the sample
     */
    CVisual3DCommonData();

    /**
     * @fn ~CVisual3DCommonData()
     *
     * @brief Class destructor, releases resources
     */
    ~CVisual3DCommonData();

    // These are the textures used in this sample 
    // They are added for more realistic visualization

    /**
     * @var m_pBottomTexture
     *
     * @brief The bottom texture
     */
    ID3D10ShaderResourceView *m_pBottomTexture;

    /**
     * @var m_pSurfaceTexture
     *
     * @brief The water surface texture
     */
    ID3D10ShaderResourceView *m_pSurfaceTexture;

    /**
     * @var m_pSkyTexture
     *
     * @brief The texture of the sky
     */
    ID3D10ShaderResourceView *m_pSkyTexture;


};