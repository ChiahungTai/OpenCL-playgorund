/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CSky.cpp
 *
 * @brief Defines a class to handle the parameters for and the rendering of the sky
 */

#include "Utils/CVBGridGenerator.h"
#include "CSky.h"

namespace
{

    /**
     * @struct SphereGenerator
     *
     * @brief Stores parameters for generating a sphere
     */
    struct SphereGenerator
    {
        typedef D3DXVECTOR3 VERTEX;

        /**
         * @fn SphereGenerator()
         *
         * @brief Constructor of the structure
         *
         * @param dimAz - Azimuth co-efficient
         *
         * @param dimEl - Elevation dimensions
         *
         * @param radius - Radius of the sphere
         *
         * @param maxElevationAngle - The maximum elevation angle
         */
        SphereGenerator(int dimAz, int dimEl, float radius, float maxElevationAngle)
            : m_azimuthCoeff(2.0f * M_PI / float(dimAz-1)),
            m_elevationCoeff(maxElevationAngle / float(dimEl)),
            m_radius(radius)
        {

        }
        void operator()(int x, int y, D3DXVECTOR3 &rVtx) const
        {
            float az = m_azimuthCoeff * float(x);
            float el = m_elevationCoeff * float(y);
            rVtx.x = m_radius * cosf(az) * sinf(el);
            rVtx.y = m_radius * cosf(el);
            rVtx.z = m_radius * sinf(az) * sinf(el);
        }

    private:
        float m_azimuthCoeff;
        float m_elevationCoeff;
        float m_radius;
    };

    // Create vertex inputs for the bottom & the surface
    D3D10_INPUT_ELEMENT_DESC vtxLayoutDesc[] = 
    {
        { 
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 
        },
    };

}

/**
 * @fn CSky - Class constructor
 * 
 * @brief Instantiates an instance of the class
 *
 * @param pSkyCube - The sky shader resource view
 */
CSky::CSky(ID3D10ShaderResourceView *pSkyCube)
{
    m_material.m_cTex.Set(pSkyCube);

    // Create sphere
    int const dimAz = 20;
    int const dimEl = 10;

    SphereGenerator sphereFunc(dimAz, dimEl, 2000.0f, M_PI * 0.6f);
    CVBGridGenerator<SphereGenerator> sphereGen(dimAz, dimEl, sphereFunc);
    m_pVBuff = CreateVertexBufferImmutable(sphereGen);
    m_pIBuff = CreateGridIndexBuffer(dimAz, dimEl, &m_numIndices);
    m_vertexSize = sizeof(SphereGenerator::VERTEX);
}

/**
 * @fn RenderSky
 *
 * @brief Renders the sky
 *
 * @param rWorldViewProj - The world view projection matrix
 */
void CSky::RenderSky(D3DXMATRIX const &rWorldViewProj)
{
    m_material.m_cWVP.Set(rWorldViewProj);
    m_material.Apply();
    Render();
}

/**
 * @fn Material()
 *
 * @brief Class constructor
 */
CSky::Material::Material()
: CDXMaterialWVP(L"sky.fx", DIM(vtxLayoutDesc), vtxLayoutDesc)
, m_cTex(m_pEffect, "g_tex")
{}
// End of File