/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CVisual3DCommonData.cpp
 *
 * @brief contains the implementation of common visualization routines 
 */

#include <DXUT.h>
#include "Utils/CVBGridGenerator.h"
#include "CVisual3DCommonData.h"

/**
 * @fn CVisual3DCommonData()
 *
 * @brief Class constructor
 *
 * @note This method loads the shader resource views for the sample
 */
CVisual3DCommonData::CVisual3DCommonData()
{
    m_pBottomTexture    = LoadShaderResourceView(L"Data/land_diffuse.bmp");
    ///m_pBottomTexture    = LoadShaderResourceView(L"Data/sand_tile.bmp");
    m_pSurfaceTexture   = LoadShaderResourceView(L"Data/waves.bmp");
    m_pSkyTexture       = LoadShaderResourceView(L"Data/sky2.dds");
}

/**
 * @fn ~CVisual3DCommonData()
 *
 * @brief Class destructor, releases resources
 */
CVisual3DCommonData::~CVisual3DCommonData()
{
    SAFE_RELEASE(m_pBottomTexture);
    SAFE_RELEASE(m_pSurfaceTexture);
    SAFE_RELEASE(m_pSkyTexture);
}
