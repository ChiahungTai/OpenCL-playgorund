/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CVisual3DGathered.cpp
 *
 * @brief Defines a class to manage the parameters for gathered (advanced) rendering of grids
 */

#include <DXUT.h>

#include "macros.h"
#include "Utils/CVBGridGenerator.h"
#include "CVisual3DGathered.h"
#include "Util/memory.h"

#define DEFAULT_MAX_DEPTH 70.0f   // Select depth value from 10 to 100
#define DEFAULT_TILE_SIZE 256   
#define DEFAULT_GLOBAL_NORMAL_SCALE 1.0f   
#define DEFAULT_LOCAL_NORMAL_SCALE 2.0f   
#define DEFAULT_ADVECTED_TEXTURE_SCALE 0.03f   
#define DEFAULT_ADVECTED_TEXTURE_LATENCY 5.0f 

namespace
{
    D3D10_INPUT_ELEMENT_DESC s_vtxDesc = { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 };
};

D3D10_SHADER_MACRO   CVisual3DGathered::m_pMacro[2];

/**
 * @fn CVisual3DGathered()
 *
 * @brief Class constructor of the grid renderer
 *
 * @param scene - The scene to draw
 *
 * @param rData - Common data for all renderers
 *
 * @param dimW - The width of the resulting scene grid
 *
 * @param dimH - The height of the resulting scene grid
 */
CVisual3DGathered::CVisual3DGathered(CFlux2DScene &rScene, CVisual3DCommonData &rData, int dimW, int dimH)
: m_scene(rScene)
, m_rCommonData(rData)
, m_surface(dimW, dimH, DXGI_FORMAT_R32G32_FLOAT)
, m_material(rData)
, m_velocity(dimW, dimH, DXGI_FORMAT_R32G32_FLOAT)
, m_texAdvection(dimW, dimH)
{
    // Initialize parameters
    m_maxDepth =  DEFAULT_MAX_DEPTH;
    m_tileSize = DEFAULT_TILE_SIZE;

    m_globalNormalScale = DEFAULT_GLOBAL_NORMAL_SCALE;
    m_localNormalScale  = DEFAULT_LOCAL_NORMAL_SCALE;

    // Create index and vertex buffers
    CreateIndexAndVertexBufferAccordingToBottom(dimW, dimH);
    
    // Set width to dimW-1, height to dimH-1 and use the power of 2 for shaders
    m_material.m_cIndexXForm.Set(CVectorInt(dimW-1, dimH-1, NextPowOf2Log(dimW), 0));

    // Load the shader resource view for the bottom
    m_pBottomGrad = LoadShaderResourceView(L"Data/wb_tropic.bmp");
    m_material.m_cBottomGradient.Set(m_pBottomGrad);

    // Load the shader resource view for the surface
    m_pSurfaceGrad  = LoadShaderResourceView(L"Data/ww_tropic.dds");
    m_material.m_cSurfaceGradient.Set(m_pSurfaceGrad);

    // Get the width and height of the region
    float lenW = m_scene.GetRegion().x2 - m_scene.GetRegion().x1;
    float lenH = m_scene.GetRegion().y2 - m_scene.GetRegion().y1;
    // Set texture parameters
    m_texAdvection.SetSpatialDimensions(lenW, lenH, DEFAULT_ADVECTED_TEXTURE_SCALE);
    m_texAdvection.SetLatency(DEFAULT_ADVECTED_TEXTURE_LATENCY);
    m_texAdvection.InitTextureCoordinates();

    UpdateParameters();



    {
        /* *************************************************************************************************** *\
                                    Create buffers to get the whole scene surface
        \* *************************************************************************************************** */
        
        // Create a buffer description and initialize it
        int size = dimW*dimH*2*sizeof(float);
        m_pSceneHeightAndBottomBuffer = 0;
        m_pSceneVelocityBuffer = 0;

        // Create an  buffer for the scene height and bottom
        m_pSceneHeightAndBottomBuffer = malloc(size);

        // Create another  buffer for the scene velocity
        m_pSceneVelocityBuffer = malloc(size);

    }

}// End of CVisual3DGathered::CVisual3DGathered()

/**
 * @fn ~CVisual3DGathered()
 *
 * @brief Class destructor, releases resources
 */
CVisual3DGathered::~CVisual3DGathered()
{
    SAFE_RELEASE(m_pIBuff);
    SAFE_RELEASE(m_pVBuff);

    SAFE_RELEASE(m_pBottomGrad);
    SAFE_RELEASE(m_pSurfaceGrad);

    if(m_pSceneHeightAndBottomBuffer)
    {
        free(m_pSceneHeightAndBottomBuffer);
    }

    if(m_pSceneVelocityBuffer)
    {
        free(m_pSceneVelocityBuffer);
    }


}// End of CVisual3DGathered::~CVisual3DGathered()

/**
 * @fn UpdateParameters()
 *
 * @brief Transfers new parameter values to the shaders
 */
void CVisual3DGathered::UpdateParameters()
{
    // Get the region width and height
    float lenW = m_scene.GetRegion().x2 - m_scene.GetRegion().x1;
    float lenH = m_scene.GetRegion().y2 - m_scene.GetRegion().y1;

    // Normal scaling vector
    D3DXVECTOR4 vec;
    vec.x = 2.0f * float(m_surface.GetWidth()) / lenW;
    vec.y = m_globalNormalScale;
    vec.z = 2.0f * float(m_surface.GetHeight()) / lenH;
    vec.w = 0.0f;
    m_material.m_cNormalScale.Set(vec);

    // Vector for the bottom scales
    D3DXVECTOR4 v(1.0f / m_tileSize, 1.0f / m_maxDepth, 0.0f, 1.0f);
    m_material.m_cBottomScales.Set(v);

    // Vector for the surface is normal scale
    v.x = m_localNormalScale;
    m_material.m_cSurfaceLocalNormalScale.Set(v);
}

/**
 * @fn MacroDefinition()
 *
 * @brief This method is used to setup macros for the shaders as required
 *
 */
void CVisual3DGathered::MacroDefinition()
{
    m_pMacro[0].Name = NULL;
    m_pMacro[0].Definition = NULL;
}


/**
 * @fn CreateIndexAndVertexBufferAccordingToBottom()
 *
 * @brief Creates an index and corresponding vertice buffer to help render surfaces
 *
 * @param dimW - The width of the grid to render
 *
 @ @param dimH - The height of the grid to render
 */
void CVisual3DGathered::CreateIndexAndVertexBufferAccordingToBottom(int dimW, int dimH)
{
    // Get the region
    CRegion  &R = m_scene.GetRegion();
    // Determine the number of indices based on the height and width of the grid
    m_numIndices = 2 * 3 * (dimW - 1) * (dimH - 1);

    // Allocate memory for the indices
    int *pMem = (int*)_mm_malloc(m_numIndices * sizeof(int), 16);

    // Prepare a buffer
    int *pIB = pMem;
    int offs = 0;           // This variable tracks the total number of pixels

    // For every line in the grid
    for (int y = 0; y < (dimH-1); y++, offs++)
    {
        // For every pixel in the line
        for (int x = 0; x < (dimW-1); x++, offs++)
        {
            CSamplerBilinear sampler;
            CFlux2DGrid* pG = NULL;
            // Compute bounds
            float xf = R.x1+(R.x2-R.x1)*(float)x/(dimW-1);      // Left
            float xf1 = R.x1+(R.x2-R.x1)*(float)(x+1)/(dimW-1); // Right
            float yf = R.y1+(R.y2-R.y1)*(float)y/(dimH-1);      // Top
            float yf1 = R.y1+(R.y2-R.y1)*(float)(y+1)/(dimH-1); // Bottom
            float h00 = 0;
            float h10 = 0;
            float h01 = 0;
            // Get the left-upper grid, if possible
            pG = m_scene.FindGrid(xf,yf,&sampler);
            // If we found the left-upper grid, set h00 to the grid height
            if(pG) h00 = sampler(&pG->m_bottom.m_heightMap);
            // Get the right-upper grid, if possible
            pG = m_scene.FindGrid(xf1,yf,&sampler);
            // If we found the right-upper grid, set h10 to the grid height
            if(pG) h10 = sampler(&pG->m_bottom.m_heightMap);
            // Get the bottom-left grid, if possible
            pG = m_scene.FindGrid(xf,yf1,&sampler);
            // If we found the bottom-left grid, set h01 to the grid height
            if(pG) h01 = sampler(&pG->m_bottom.m_heightMap);
            
            // If we found a right-upper grid but not a left-upper grid or
            // if we found a bottom-left grid but not a left-upper grid then
            // the result of the following multiplication will be less than zero.
            // The grid is either on the right or the bottom edge

            if ((h10 - h00) * (h01 - h00) < 0.0f)
            {
                pIB[0] = offs;
                pIB[1] = offs + dimW + 1;
                pIB[2] = offs + 1;
                pIB[3] = offs;
                pIB[4] = offs + dimW;
                pIB[5] = offs + dimW + 1;
            }
            else
            {
                pIB[0] = offs;
                pIB[1] = offs + dimW;
                pIB[2] = offs + 1;
                pIB[3] = offs + dimW;
                pIB[4] = offs + dimW + 1;
                pIB[5] = offs + 1;
            }
            pIB += 6;
        }
    }

    // Create the index buffer
    m_pIBuff = CreateIndexBufferImmutable(m_numIndices, pMem);

    // Free the initialization data
    _mm_free(pMem);

    // Create the vertex buffer
    CVBRegionGenerator generator(dimW, dimH, m_scene.GetRegion());
    CVBGridGenerator<CVBRegionGenerator> gridGen(dimW, dimH, generator);
    m_pVBuff = CreateVertexBufferImmutable(gridGen);
}
// End of CreateIndexAndVertexBufferAccordingToBottom()

/**
 * @fn Update()
 *
 * @brief Updates the scene grid by getting data from the OCL side and preparing it for rendering
 *
 * @param VisMode - The visualization mode. (It does not make sense for CVisual3DGathered) 
 *
 * @param dt - The time step
 */
int GetScene(void* pHBData, void* pUVData, void* in_pData);

void CVisual3DGathered::Update(struct VisMode const *, float dt)
{
    // Get the size of the surface, width and height
    int sizes[2] = {m_surface.GetWidth(),m_surface.GetHeight()};

    // Declare working buffers and a buffer list
    void*            pBuffers[2];
    // Initialize the buffer list
    pBuffers[0] = m_pSceneHeightAndBottomBuffer;
    pBuffers[1] = m_pSceneVelocityBuffer;
    
    // Get the scene from OCL
    GetScene(m_pSceneHeightAndBottomBuffer, m_pSceneVelocityBuffer, (void*)sizes);

    int                 k;          // index of buffer. 0 = velocity buffer, 1 = Heigh and bottom buffer

    // Declare render targets for surface and velocity
    CDXRenderTarget*   pRT[2] = {&m_surface,&m_velocity};
    
    // For each target
    for(k=0;k<2;++k)
    {
        // Copy the buffer into the DX texture
        ID3D10Texture2D*        pTex = pRT[k]->GetTexture();
        void*                   pSrcData = NULL;


        // Map the buffer for reading
        pSrcData = pBuffers[k];

#if 0
        // Map the texture to fill with scene values
        D3D10_MAPPED_TEXTURE2D  mappedTex;
        int                     y;
        ID3D10Texture2D*        pTexCPU = pRT[k]->GetTextureCPU();
        if (pTexCPU->Map(0, D3D10_MAP_WRITE_DISCARD, 0, &mappedTex) != S_OK) return;

        // For every line in the scene
        for(y = 0; y < sizes[1]; y++)
        {
            // Prepare a destination
            float* pDst = (float*)((char*)mappedTex.pData + mappedTex.RowPitch*y);
            // Read the source data
            float* pSrc = (float*)pSrcData + y*sizes[0]*2;
            // And copy it to the destination
            memcpy(pDst,pSrc,sizeof(float)*sizes[0]*2);
        }

        // Un-map the first buffer
        pTexCPU->Unmap(0);

        // Then copy the resource
        DXUTGetD3D10Device()->CopyResource(pTex,pTexCPU);
#else
        DXUTGetD3D10Device()->UpdateSubresource(pTex, 0, NULL,pSrcData, sizeof(float)*sizes[0]*2, 0);
#endif

    }
    

    // Update texture advection
    DropResources();
    m_texAdvection.Update(m_velocity, dt);

} // End of Update

/**
 * @fn RenderTheBuffer()
 *
 * @brief Renders the vertex m_pVBuffand and the index m_pIBuff buffers to draw 
 *        either the water surface or the bottom surface
 */
void CVisual3DGathered::RenderTheBuffer()
{
    // Get the output device
    ID3D10Device *pDev = DXUTGetD3D10Device();
    // Set the topology
    pDev->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    // Set the index buffer
    pDev->IASetIndexBuffer(m_pIBuff, DXGI_FORMAT_R32_UINT, 0);
    // Set the vertex buffer, input layout, then render
    UINT offset = 0;
    UINT vsize  = sizeof(D3DXVECTOR2);
    pDev->IASetVertexBuffers(0, 1, &m_pVBuff, &vsize, &offset);
    pDev->DrawIndexed(m_numIndices, 0, 0);

} // End of RenderTheBuffer()

/**
 * @fn RenderSurface()
 *
 * @brief Renders the surface of the water
 *
 * @param rWorldViewProj - The world view projection matrix
 *
 * @param rCamPos - The position of the camera
 */
void CVisual3DGathered::RenderSurface(D3DXMATRIX const &worldViewProj, D3DXVECTOR3 const &camPos)
{
    // Set the surface texture to the shader resource view
    m_material.m_cSurfaceMap.Set(m_surface.GetSRV());

    // Set the texture coordinate resources
    ID3D10ShaderResourceView *paSRVs[CVisual3DTexAdvection::NUM_TEXCOORD_SETS];

    for (int i = 0; i < CVisual3DTexAdvection::NUM_TEXCOORD_SETS; i++)
    {
        paSRVs[i] = m_texAdvection.GetTextureCoordinateSet(i);
    }
    m_material.m_cSurfaceTexCoords.Set(CVisual3DTexAdvection::NUM_TEXCOORD_SETS, paSRVs);

    // Set the blending weights
    D3DXVECTOR3 const &w = m_texAdvection.GetWeights();
    D3DXVECTOR4 v(w.x, w.y, w.z, 0.0f);
    m_material.m_cTexBlendWeights.Set(v);

    D3DXVECTOR4 cp(camPos.x, camPos.y, camPos.z, 1.0f);
    m_material.m_cCamPos.Set(cp);

    // Set the world view projection matrix
    m_material.m_cWVP.Set(worldViewProj);

    // Apply
    m_material.Apply(2);

    // Render
    RenderTheBuffer();

    // Then free resources
    DropResources();

} // END of RenderSurface()


/**
 * @fn RenderBottom()
 *
 * @brief Renders the bottom of the scene
 *
 * @param rWorldViewProj - The world view projection matrix
 */
void CVisual3DGathered::RenderBottom(D3DXMATRIX const &rWorldViewProj)
{
    // Set the surface texture to the shader resource view
    m_material.m_cSurfaceMap.Set(m_surface.GetSRV());
    // Set the world view projection matrix
    m_material.m_cWVP.Set(rWorldViewProj);
    // Apply
    m_material.Apply(1);
    // Render
    RenderTheBuffer();
    // Then drop resources
    DropResources();

} // END of RenderBottom()

/**
 * @fn Material()
 *
 * @brief Class constructor for the Material class object
 *
 * @param rData - Common visualization data
 */
CVisual3DGathered::Material::Material(CVisual3DCommonData &rData)
: CDXMaterialWVP(L"gathered_sky.fx", 1, &s_vtxDesc, m_pMacro)
, m_cSurfaceMap(m_pEffect, "g_surfaceMap")
, m_cBottomDiff(m_pEffect, "g_bottomDiff")
, m_cSurfaceDiff(m_pEffect, "g_surfaceDiff")
, m_cSurfaceTexCoords(m_pEffect, "g_surfaceTexCoords")
, m_cSkyEnvMap(m_pEffect, "g_skyEnvMap")
, m_cBottomGradient(m_pEffect, "g_bottomGradient")
, m_cSurfaceGradient(m_pEffect, "g_surfaceGradient")
, m_cIndexXForm(m_pEffect, "g_indexXForm")
, m_cNormalScale(m_pEffect, "g_normalScale")
, m_cTexBlendWeights(m_pEffect, "g_texBlendWeights")
, m_cCamPos(m_pEffect, "g_camPos")
, m_cBottomScales(m_pEffect, "g_bottomScales")
, m_cSurfaceLocalNormalScale(m_pEffect, "g_surfaceLocalNormalScale")
{
    m_cBottomDiff.Set(rData.m_pBottomTexture);
    m_cSurfaceDiff.Set(rData.m_pSurfaceTexture);
    m_cSkyEnvMap.Set(rData.m_pSkyTexture);
}
// End of File