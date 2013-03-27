/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CVisual3DSimple.cpp
 *
 * @brief Defines a class to manage the parameters for simple rendering of grids
 */

#include <DXUT.h>
#include "Flux/CFlux2DScene.h"
#include "Utils/vismodes.h"
#include "Utils/CVBGridGenerator.h"
#include "CVisual3DSimple.h"

namespace
{

    /**
     * @struct ScnVertex
     *
     * @brief A single vertex in a scene
     */
    struct ScnVertex
    {

        /**
         * @var pos
         *
         * @brief The position as a 3D vector
         */
        D3DXVECTOR3 pos;

        /**
         * @var color
         *
         * @brief The color of the vertex
         */
        D3DCOLOR    color;

        /**
         * @fn Set(float x, float y, float z, D3DCOLOR color_)
         *
         * @brief 
         *
         * @param x - The X coordinate for the vector
         *
         * @param y - The Y coordinate for the vector
         *
         * @param z - The Z coordinate for the vector
         *
         * @param color_ - The color of the specified point in 3D space
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
     * @var s_vtxLayoutDesc[]
     *
     * @brief Creates vertex inputs for the bottom surface
     */
    D3D10_INPUT_ELEMENT_DESC s_vtxLayoutDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 }
    };


    /**
     * @fn ConvertColor(unsigned int color)
     *
     * @brief Converts a grid color into a mesh color
     *
     * @param color - The color from the grid to convert
     */
    unsigned int ConvertColor(unsigned int color)
    {
        return (color & 0x0000FF00) | ((color << 16) & 0x00FF0000) | ((color >> 16) & 0x000000FF) | 0xFF000000;
    }

    /**
     * @fn ConvertHeightMapToMesh(pVertices, rVis, pG, xOffs, yOffs, bVisBC)
     *
     * @brief Converts a height map into a mesh
     *
     * @param pVertices -buffer of screen vertices, output
     *
     * @param rVis - Visualization mode
     *
     * @param pG - The grid to convert
     *
     * @param xOffs - X offset
     *
     * @param yOffs - Y offset
     *
     * @param bVisBC - Boolean variable indicates the visibility of borders
     */
    void ConvertHeightMapToMesh(ScnVertex *pVertices, VisMode const &rVis, CFlux2DGrid* pG, float xOffs, float yOffs, bool bVisBC)
    {
        int x,y;

        // Get the surface and bottom height maps
        DynamicMatrix<float> &surf = pG->GetCurrentSurface().m_heightMap;
        DynamicMatrix<float> &bott = pG->GetBottom().m_heightMap;

        // Get the horizontal and vertical step values
        float const gridStepX = pG->GetGridStepW();
        float const gridStepY = pG->GetGridStepH();
        // Determine the width of the border
        int         bs = bVisBC?surf.Border():0;

        // For every line in the surface
        for(y = -bs; y < surf.Height()+bs; y++)
        {
            // Get the Y position value based on the line number and the vertical step 
            float yPos = yOffs+gridStepY*0.5f + y*gridStepY;

            // For every column in the line
            for(x = -bs; x < (surf.Width()+bs); x++, pVertices++)
            {
                // Get the X position value based on the column and horizontal step
                float xPos = xOffs+gridStepX*0.5f + x*gridStepX;

                float H = surf.At(x,y) - bott.At(x,y) - 0.05f;

                // Store the information, construction the mesh
                pVertices->Set(xPos, H, yPos, ConvertColor( rVis.GetColor(*pG, x, y) ) );
            }
        }
    }
}

// ----------------------------------------------------------------------------------------------------------

/**
 * @fn GridData(CFlux2DGrid* pG, CRegion const &rReg, bool bVisBC)
 *
 * @brief Class constructor
 *
 * @param pG - The grid
 *
 * @param rReg - The region
 *
 * @param bVisBC - Boolean variable indicating the visiblity of borders
 */
CVisual3DSimple::GridData::GridData(CFlux2DGrid* pG, CRegion const &rReg, bool bVisBC)
: m_bottom(*pG,rReg.x1, rReg.y1, bVisBC)
{
    m_dimW = 0;
    m_dimH = 0;
    m_pSurfaceVBuff = NULL;
    m_bVisBC = bVisBC;
    UpdateSize(pG);
}

/**
 * @fn UpdateSize(CFlux2DGrid* pG)
 *
 * @brief Resizes the buffers if the size of the grid has changed
 *
 * @param The grid to be used in the resize operation
 */
void CVisual3DSimple::GridData::UpdateSize(CFlux2DGrid* pG)
{
    // Get the height and width of the grid
    int dimW = pG->GetDomainW();
    int dimH = pG->GetDomainH();
    // Adjust if borders are visible
    if(m_bVisBC)
    {
        dimW += 2*pG->m_bottom.m_heightMap.Border();
        dimH += 2*pG->m_bottom.m_heightMap.Border();
    }

    // IF the size has changed in either direction
    if(dimW!=m_dimW || dimH != m_dimH)
    {
        // Release the surface V buffer
        SAFE_RELEASE(m_pSurfaceVBuff);
        // Create a new surface V buffer
        m_pSurfaceVBuff = CreateVertexBuffer(dimW * dimH * sizeof(ScnVertex), D3D10_USAGE_DYNAMIC);
        // Compute the new number of indices
        m_numIndices = 2 * 3 * (dimW - 1) * (dimH - 1);
        // Update the size
        m_dimW = dimW;
        m_dimH = dimH;
        // Then update the bottom
        m_bottom.UpdateSize(*pG);
    }
}

/**
 * @fn ~GridData()
 *
 * @brief Class destructor
 */
CVisual3DSimple::GridData::~GridData()
{
    SAFE_RELEASE(m_pSurfaceVBuff);
}

/**
 * @fn Update(pG, rReg, pVM, bVisBC)
 *
 * @brief Updates the surface buffer, creates a mesh
 *
 * @param pG - The grid to update to
 *
 * @param rReg - The region for the update
 *
 * @param pVM - The visualization mode
 *
 * @param bVisBC - Boolean variable indicating the visiblity of borders
 */
void CVisual3DSimple::GridData::Update(CFlux2DGrid *pG, CRegion const &rReg, VisMode const *pVM, bool bVisBC)
{
    // BVS Possible place to improvement. Use double buffering approach. Test nowait flag, etc.
    void*   pData;

    // Update buffers to the grid
    UpdateSize(pG);

    // Map the pData buffer
    HRESULT hr = m_pSurfaceVBuff->Map(D3D10_MAP_WRITE_DISCARD, 0, &pData);
    assert(hr == S_OK);
    hr;

    // Convert the grids to a mesh
    ConvertHeightMapToMesh((ScnVertex*)pData, *pVM, pG, rReg.x1, rReg.y1, bVisBC);

    // Then un-map the buffer
    m_pSurfaceVBuff->Unmap();
}

/**
 * @fn RenderSurface()
 *
 * @brief Renderes the water surface
 */
void CVisual3DSimple::GridData::RenderSurface()
{
    ID3D10Device *pDev = DXUTGetD3D10Device();
    // Set the index buffer
    pDev->IASetIndexBuffer(m_bottom.GetIB(), DXGI_FORMAT_R32_UINT, 0);
    // Set the vertex buffer, input layout, then render
    UINT stride = sizeof(ScnVertex);
    UINT offset = 0;
    pDev->IASetVertexBuffers(0, 1, &m_pSurfaceVBuff, &stride, &offset);
    pDev->DrawIndexed(m_numIndices, 0, 0);
}

/**
 * @fn RenderBottom()
 *
 * @brief Renders the bottom scene
 */
void CVisual3DSimple::GridData::RenderBottom()
{
    m_bottom.Render();
}


// ----------------------------------------------------------------------------------------------------------

/**
 * @fn CVisual3DSimple(CFlux2DScene &rScene, CVisual3DCommonData &rData)
 *
 * @brief Class constructor, creates buffers
 *
 * @param rScene - The scene for this instance of the class
 *
 * @param rData - Common data, shader resource views
 */
CVisual3DSimple::CVisual3DSimple(CFlux2DScene &rScene, CVisual3DCommonData &rData)
    : m_scene(rScene),
    m_rCommonData(rData),
    m_bottomMat(rData),
    m_surfaceMat(L"simple.fx", DIM(s_vtxLayoutDesc), 
    s_vtxLayoutDesc)
{
    m_pHeightBuffer = 0;
    m_pUBuffer = 0;
    m_pVBuffer = 0;
    // We set the Visible Border Condition only in this case

    // Set border visualization flag
    m_bVisBC = 1;

    // Create visuals
    for (int i = 0; i < rScene.GetNumGrids(); i++)
    {
        GridData* gd = new GridData(m_scene.GetGrid(i), m_scene.GetRegion(i), m_bVisBC);
        m_Grids.Append(gd);
    }


    // Create the u, v and height buffers 

    
    {
        // create buffers for the Height and velocity data 
        int             i;
        
        unsigned int size = 0;
        if(m_pHeightBuffer)
        {
            free(m_pHeightBuffer);
        }
        if(m_pUBuffer)
        {
            free(m_pUBuffer);
        }
        if(m_pVBuffer)
        {
            free(m_pVBuffer);
        }

        // Find the maximum grid size
        for(i=0;i<m_scene.GetNumGrids();++i)
        {
            uint32_t s = (uint32_t)m_scene.GetGrid(i)->GetBottom().m_heightMap.m_DataSize;
            if(size < s)size = s;
        }
        

        // Create U, V, and Height buffers for writing to OCL
        m_pHeightBuffer = malloc(size);
        m_pUBuffer = malloc(size);
        m_pVBuffer = malloc(size);

    }
} // End of CVisual3DSimple::CVisual3DSimple()

/**
 * @fn Update(struct VisMode const *pVM, float dt)
 *
 * @brief Updates the display, processes all buffers
 *
 * @param pVM - The visualization mode
 *
 * @param dt - Delta time
 */
int GetGrid(void* pHData, void* pUData, void* pVData, int i);
void CVisual3DSimple::Update(VisMode const *pVM, float)
{
    // Declare and initialize working variables
    int             i;
    int             gridNum = m_scene.GetNumGrids();

    // Get all grid data from the OCL side 
    for(i=0;i<gridNum;++i)
    {
        CFlux2DGrid*   pG = m_scene.GetGrid(i);
        int             size = pG->GetBottom().m_heightMap.m_DataSize;

        // Run the grid function
        GetGrid(m_pHeightBuffer, m_pUBuffer, m_pVBuffer, i);

        // Copy the OCL buffer into local memory
        memcpy(pG->GetCurrentSurface().m_heightMap.m_pPointer, m_pHeightBuffer, size);

        // Copy the water bars speed values
        memcpy(pG->GetCurrentSurface().m_uMap.m_pPointer, m_pUBuffer, size);

        memcpy(pG->GetCurrentSurface().m_vMap.m_pPointer, m_pVBuffer, size);

        // Update the grid
        m_Grids[i]->Update(pG, m_scene.GetRegion(i), pVM, m_bVisBC);
    }// Move to the next grid

}// END of CVisual3DSimple::Update()


/**
 * @fn RenderSurface(D3DXMATRIX const &rWorldViewProj, D3DXVECTOR3 const &rCamPos)
 *
 * @brief Renders the water surface
 *
 * @param rWorldViewProj - The world view projection matrix
 *
 * @param rCamPos - A vector representing the camera position
 */
void CVisual3DSimple::RenderSurface(D3DXMATRIX const &rWorldViewProj, D3DXVECTOR3 const &)
{
    m_surfaceMat.m_cWVP.Set(rWorldViewProj);
    m_surfaceMat.Apply();

    // Set topology
    DXUTGetD3D10Device()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    int const num = m_scene.GetNumGrids();

    // For every grid in the scene
    for (int i = 0; i < num; i++)
    {
        // Render it
        m_Grids[i]->RenderSurface();
    }
}

/**
 * @fn RenderBottom(D3DXMATRIX const &rWorldViewProj)
 *
 * @brief Renders the bottom scene
 *
 * @param rWorldViewProj - The world view projection matrix
 */
void CVisual3DSimple::RenderBottom(D3DXMATRIX const &rWorldViewProj)
{
    m_bottomMat.m_cWVP.Set(rWorldViewProj);
    m_bottomMat.Apply();

    int const num = m_scene.GetNumGrids();
    // For every grid in the scene
    for (int i = 0; i < num; i++)
    {
        // Render
        m_Grids[i]->RenderBottom();
    }
}

/**
 * @fn ~CVisual3DSimple()
 *
 * @brief Class Destructor, deletes grids and destroys all buffers
 */
CVisual3DSimple::~CVisual3DSimple()
{
    int i;
    // Delete all grids
    for(i = 0; i<m_Grids.Size(); i++)
    {
        GridData* gd = m_Grids[i];
        SAFE_DELETE(gd);
    }

    if(m_pHeightBuffer)
    {
        free(m_pHeightBuffer);
    }
    if(m_pUBuffer)
    {
        free(m_pUBuffer);
    }
    if(m_pVBuffer)
    {
        free(m_pVBuffer);
    }

}
