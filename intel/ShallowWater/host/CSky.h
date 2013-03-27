/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CSky.h
 *
 * @brief Defines a class to handle the parameters for and the rendering of the sky
 */

#pragma once

#include "Utils/CDXMeshBase.h"

/**
 * @class CSky
 *
 * @brief Contains a function to render the sky and a Material class object that contains the resource
 */
class CSky : 
    public CDXMeshBase
{
public:

    /**
     * @fn CSky - Class constructor
     * 
     * @brief Instantiates an instance of the class
     *
     * @param pSkyCube - The sky shader resource view
     */
    CSky(ID3D10ShaderResourceView *pSkyCube);

    /**
     * @fn RenderSky
     *
     * @brief Renders the sky
     *
     * @param rWorldViewProj - The world view projection matrix
     */
    void RenderSky(D3DXMATRIX const &rWorldViewProj);

private:

    /**
     * @class Material
     *
     * @brief The resources used in rendering the sky
     */
    class Material : 
        public CDXMaterialWVP
    {
    public:

        /**
         * @fn Material()
         *
         * @brief Class constructor
         */
        Material();

        CFXResource m_cTex;
    };

    /**
     * @var m_material
     *
     * @brief The material class member instantiated
     */
    Material m_material;
};