/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CVisual3DBottom.h
 *
 * @brief Contains declarations for bottom visualization routines  
 */


#pragma once

#include "Flux/CFlux2DGrid.h"
#include "Utils/CDXMeshBase.h"
#include "CVisual3DCommonData.h"
#include "CVisual3DInterface.h"

class CVisual3DBottom : 
    public CDXMeshBase
{
public:
    /**
     * @fn CVisual3DBottom()
     *
     * @brief Class constructor
     *
     * @param rGrid - The grid
     *
     * @param x - Coordinate
     *
     * @param z - Coordinate
     *
     * @param bVisBC - Border visualization flag
     */
    CVisual3DBottom(CFlux2DGrid &rGrid, float x, float z, bool bVisBC);
    
    /**
     * @fn UpdateSize()
     *
     * @brief Updates the grid size 
     *
     * @param rGrid - The grid
     */
    void UpdateSize(CFlux2DGrid &rGrid);

    /**
     * @class Material
     *
     * @brief Vertext buffer, shader macro, effect and effect name
     */
    class Material : 
        public CDXMaterialWVP
    {
    public:
        Material(CVisual3DCommonData &data);
    };
protected:

    /**
     * @var m_dimW
     *
     * @brief Width of the bottom
     */
    int     m_dimW;

    /**
     * @var m_dimH
     *
     * @brief Height of the bottom
     */
    int     m_dimH;

    /**
     * @var m_x
     *
     * @brief An X coordinate
     */
    float   m_x;

    /**
     * @var m_z
     *
     * @brief A Y coordinate
     */
    float   m_z;

    /**
     * @var m_bVisBC
     *
     * @brief A boolean flag indicating the visibility of the borders
     */
    bool    m_bVisBC;
};