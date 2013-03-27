/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

#pragma once

#include <DXUT.h>
#include "Utils/CVBGridGenerator.h"
#include "CVisual3DBottom.h"

/**
 * #def HIGH_CONTRAST_NORMAL
 *
 * @brief Defines a normlization factor for high contrast displays
 *
 * @note Choose value between 0.4 and 0.8 for best contrast 
 */
#define HIGH_CONTRAST_NORMAL 0.4f    

namespace
{

    /**
     * @struct ScnVertex
     *
     * @brief Definition of a scene vertex
     */
    struct ScnVertex
    {
        /**
         * @var pos
         *
         * @brief The position (X,Y,Z) of the vertext
         */
        D3DXVECTOR3 pos;

        /**
         * @var color
         *
         *@ brief The color of the vertex
         */
        D3DCOLOR    color;

        /**
         * @fn Set(float x, float y, float z, D3DCOLOR color_)
         *
         * @brief Sets the values of the vertext, position and color
         *
         * @param x - The X coordinate of the vertex
         *
         * @param y - The Y coordinate of the vertex
         *
         * @param z - The Z coordinate of the vertex
         *
         * @param color - The color of the vertex
         */
        void Set(float x, float y, float z, D3DCOLOR color_)
        {
            pos.x = x;
            pos.y = y;
            pos.z = z;
            color = color_;
        }
    };

    /**
     * @var scnVtxLayoutDesc[]
     *
     * @brief Creates vertex inputs for the bottom and the surface
     */
    D3D10_INPUT_ELEMENT_DESC scnVtxLayoutDesc[] = 
    {
        { 
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 
        },
        { 
            "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 
        }
    };

    /**
     * @fn CalculateBottomColor(pBottom, x, y, rcpStepW, rcpStepH)
     *
     * @brief Calculates the bottom color at the specified coordinates
     *
     * @param pBottom - Dynamic floating point matrix representing the bottom
     *
     * @param x - The X coordinate
     *
     * @param y - The Y coordinate
     *
     * @param rcpStepW - Recpricol of the width step
     *
     * @param rcpStepH - Recpricol of the height step
     *
     * @return - The color at the specified point
     */
    D3DCOLOR CalculateBottomColor(DynamicMatrix<float>* pBottom, int x, int y, float rcpStepW, float rcpStepH)
    {
        // Declare and initialize working variables
        int dimW = pBottom->Width();
        int dimH = pBottom->Height(); 

        // Find the value of the left and right hand neighbors
        D3DXVECTOR3 normal;
        float left  = -pBottom->At(__max(0,x-1), y);
        float right = -pBottom->At(__min(dimW-1,x+1), y);
        // Normalize
        normal.x = (left - right) * 0.5f * rcpStepW;

        // Now, find the values of the upper and lower neighbors
        float top   = -pBottom->At(x, __max(0,y-1));
        float bottom = -pBottom->At(x, __min(dimH-1,y+1));
        // Normalize
        normal.z = (top - bottom) * 0.5f * rcpStepH;

        normal.y = HIGH_CONTRAST_NORMAL; 

        // Now, normlize the generated vector
        D3DXVec3Normalize(&normal, &normal);

        int intensity = int(normal.y * 255.0f);
        // Compute the color and return it
        return 0xFF000000 | (intensity << 16) | (intensity << 8) | intensity;
    }

    /**
     * @struct BottomGen
     *
     * @brief Used to generate the contents of the bottom
     */
    struct BottomGen : 
        public CVBContentGenerator
    {
    public:

        /**
         * @fn BottomGen(pBottom, xOffs, zOffs, xStep, zStep, bVisBC )
         *
         * @brief Structure constructor, initializes parameters and the scene
         *
         * @param pBottom - Dynamic matrix representing the bottom
         *
         * @param xOffs - The X offset
         *
         * @param zOffs - The Z offset
         *
         * @param xStep - The X step or delta
         *
         * @param zStep - The Z step or delta
         *
         * @param bVisBC - A flag indicating whether or not the borders are visible
         */
        BottomGen( DynamicMatrix<float> *pBottom,
            float xOffs, 
            float zOffs, 
            float xStep, 
            float zStep, 
            bool bVisBC ) 
            : m_pBottom(pBottom), 
            m_xOffs(xOffs), 
            m_zOffs(zOffs), 
            m_xStep(xStep), 
            m_zStep(zStep), 
            m_bVisBC(bVisBC) 
        {

        }

        /**
         * @fn GetSize()
         *
         * @brief Used to find the size of the bottom scene
         *
         * @note Tests to see if borders are to be included
         *
         * @return The size of the bottom scene
         */
        virtual int GetSize() const 
        { 
            int w = m_pBottom->Width();
            int h = m_pBottom->Height();
            if (m_bVisBC)
            {
                w+= 2*m_pBottom->Border(); 
                h+= 2*m_pBottom->Border(); 
            }
            return w * h * sizeof(ScnVertex); 
        }

        /**
         * @fn Generate(void *pData)
         *
         * @brief 
         *
         * @param pData - The vertice data
         */
        virtual void Generate(void *pData) const 
        {
            // Declare and initialize working variables
            int dimW = m_pBottom->Width();
            int dimH = m_pBottom->Height();
            float rcpStepX = 1.0f / m_xStep;
            float rcpStepZ = 1.0f / m_zStep;
            
            ScnVertex *pVertices = static_cast<ScnVertex*>(pData);

            // Determine the border size if it is to be displayed
            int bs = m_bVisBC?m_pBottom->Border():0;

            // For every line in the scene (including borders if they are to be displayed)
            for (int y = -bs; y < dimH+bs; y++)
            {
                // Compute the Z value based on the line and Z step
                float zPos = m_zOffs+0.5f*m_zStep+y*m_zStep;

                // For every column in the line (including borders if they are to be displayed)
                for (int x = -bs; x < dimW+bs; x++, pVertices++)
                {
                    // Compute the X value based on the pixel number and the X step
                    float       xPos = m_xOffs+0.5f*m_xStep+x*m_xStep;
                    D3DCOLOR    color;
                    D3DXVECTOR3 normal;
                    // Get the top, left, bottom, and right neighbor values
                    float       left  = -m_pBottom->At(__max(0,x-1), y);
                    float       right = -m_pBottom->At(__min(dimW-1,x+1), y);
                    float       top   = -m_pBottom->At(x, __max(0,y-1));
                    float       bottom = -m_pBottom->At(x, __min(dimH-1,y+1));
                    // Normalize
                    normal.x = (left - right) * 0.5f * rcpStepX;
                    normal.z = (top - bottom) * 0.5f * rcpStepZ;
                    normal.y = HIGH_CONTRAST_NORMAL; 

                    D3DXVec3Normalize(&normal, &normal);

                    // Compute the color
                    int intensity = int(normal.y * 255.0f);
                    color =  0xFF000000 | (intensity << 16) | (intensity << 8) | intensity;

                    // Save the results in the vertex buffer
                    pVertices->Set(xPos, -m_pBottom->At(x,y), zPos, color);
                } // Advance to the next column
            } // Advance to the next line
        } // End of Generate()

    private:

        /**
         * @var m_pBottom
         *
         * @brief A dynamic matrix representing the bottom
         */
        DynamicMatrix<float>* m_pBottom;

        /**
         * @var m_xOffs
         *
         * @brief - X offset
         */
        float m_xOffs;

        /**
         * @var m_zOffs
         *
         * @brief - Z offset
         */
        float m_zOffs;

        /**
         * @var m_xStep
         *
         * @brief X step or delta
         */
        float m_xStep;

        /**
         * @var m_zStep
         *
         * @brief Z step or delta
         */
        float m_zStep;

        /**
         * @var m_bVisBC
         *
         * @brief Boolean used to indicate the visiblity of borders
         */
        bool  m_bVisBC;

        NON_COPYABLE(BottomGen)
    };

}
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
CVisual3DBottom::CVisual3DBottom(CFlux2DGrid &rGrid, float x, float z, bool bVisBC)
{
    m_dimW = 0;
    m_dimH = 0;
    m_x = x;
    m_z = z;
    m_bVisBC = bVisBC;
    m_pVBuff = NULL;
    m_pIBuff = NULL;
    UpdateSize(rGrid);
}

/**
 * @fn UpdateSize(CFlux2DGrid &rGrid)
 *
 * @brief Checks the size of the bottom height map contained with the grid
 *        and re-intantiates buffers if a change in size is detected
 *
 * @param rGrid - The grid containint the bottom height map
 */
void CVisual3DBottom::UpdateSize(CFlux2DGrid &rGrid)
{
    DynamicMatrix<float>* pBottom = &(rGrid.GetBottom().m_heightMap);
    // Get the size of the height map
    int dimW = pBottom->Width();
    int dimH = pBottom->Height();
    // Adjust for borders if they are visible
    if (m_bVisBC)
    {
        dimW += 2*pBottom->Border();
        dimH += 2*pBottom->Border();
    }

    // Check the computed size against the current size
    if(dimW!=m_dimW || dimH != m_dimH)
    {
        // If different, release current buffers
        SAFE_RELEASE(m_pVBuff);
        SAFE_RELEASE(m_pIBuff);
        m_vertexSize = sizeof(ScnVertex);
        // Re-create the buffers
        m_pVBuff = CreateVertexBufferImmutable(BottomGen(pBottom, m_x, m_z, rGrid.GetGridStepW(), rGrid.GetGridStepH(), m_bVisBC));
        m_pIBuff = CreateGridIndexBuffer(dimW, dimH, &m_numIndices);
        // Store the size
        m_dimW = dimW;
        m_dimH = dimH;
    }
} // End of CVisual3DBottom::UpdateSize

/**
 * @fn Material(CVisual3DCommonData &data)
 *
 * @brief Class constructor, sets shader resources 
 *
 * @param data - Contains the bottom texture
 */
CVisual3DBottom::Material::Material(CVisual3DCommonData &data)
: CDXMaterialWVP(L"bottom.fx", DIM(scnVtxLayoutDesc), scnVtxLayoutDesc)
{
    m_pEffect->GetVariableByName( "g_bottomDiffuse" )->AsShaderResource()->SetResource( data.m_pBottomTexture );
}