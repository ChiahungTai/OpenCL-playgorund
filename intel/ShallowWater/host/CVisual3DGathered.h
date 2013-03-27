/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CVisual3DGathered.h
 *
 * @brief Defines a class to manage the parameters for and rendering of grids
 */

#pragma once

#include "Utils/CDXRenderTarget.h"
#include "Utils/CDXQuad.h"
#include "CVisual3DCommonData.h"
#include "CVisual3DTexAdvection.h"
#include "CVisual3DInterface.h"
#include "util/memory.h"
#include "flux/CFlux2DScene.h"

#include "common/physics_3D_fluid_sim_common.h"

/**
 * #class CVisual3DGathered
 * 
 * @brief A class to draw the height maps of the grids for a whole scene as one grid
 *        This class samples all the grids of a scene into one smaller size grid and
 *        then draws the resulting grid on the screen
 */
class CVisual3DGathered : 
    public CVisual3DInterface
{
public:

    /**
     * @fn CVisual3DGathered()
     *
     * @brief Class constructor of the grid renderer
     *
     * @param scene - The scene to draw
     *
     * @param rData - Common data for all renderers
     *
     * @param dimW - The width of the resulting scene grid (default value is 256)
     *
     * @param dimH - The height of the resulting scene grid (default value is 256)
     */
    CVisual3DGathered(
        CFlux2DScene &scene, 
        CVisual3DCommonData &rData,
        int dimW = 256,
        int dimH = 256);

    /**
     * @fn ~CVisual3DGathered()
     *
     * @brief Class destructor, releases resources
     */
    ~CVisual3DGathered();

    /**
     * @fn Update()
     *
     * @brief Updates the scene grid by getting data from the OCL side and preparing it for rendering
     *
     * @param pVM - The visualization mode. (It does not make sense for CVisual3DGathered) 
     *
     * @param dt - The time step
     */
    virtual void Update( struct VisMode const *pVM, float dt);

    /**
     * @fn RenderSurface()
     *
     * @brief Renders the surface of the water
     *
     * @param rWorldViewProj - The world view projection matrix
     *
     * @param rCamPos - The position of the camera
     */
    virtual void RenderSurface(
        D3DXMATRIX const &rWorldViewProj,
        D3DXVECTOR3 const &rCamPos);

    /**
     * @fn RenderBottom()
     *
     * @brief Renders the bottom of the scene
     *
     * @param rWorldViewProj - The world view projection matrix
     */
    virtual void RenderBottom(D3DXMATRIX const &rWorldViewProj);

    /**
     * @fn GetName()
     *
     * @return = The name of the render engine
     *
     * @note This function is useful if, for example, you want to show the name in a combo box
     */
    virtual wchar_t const * GetName() const 
    { 
        return L"Gathered"; 
    }

    /**
     * @fn SetMaxDepth()
     *
     * @brief Sets the maximum depth for water rendering
     *
     * @param depth - The maximum depth 
     */
    void SetMaxDepth(float depth) 
    { 
        m_maxDepth = depth; 
        UpdateParameters(); 
    }

    /**
     * @fn GetMaxDepth()
     *
     * @brief Gets the maximum depth for water rendering
     *
     * @return - The maximum depth 
     */
    float GetMaxDepth() const 
    { 
        return m_maxDepth; 
    }

    /**
     * @fn SetLocalNormalScale()
     *
     * @brief Sets the normal scale for water rendering
     *
     * @param scale - The normal scale for water rendering
     */
    void SetLocalNormalScale(float scale) 
    { 
        m_localNormalScale = scale; 
        UpdateParameters(); 
    }

    /**
     * @fn GetLocalNormalScale()
     *
     * @brief Gets the normal scale for water rendering
     *
     * @return - The normal scale for water rendering
     */
    float GetLocalNormalScale() const 
    { 
        return m_localNormalScale; 
    }

    /**
     * @fn SetLatency()
     *
     * @brief Sets the latency for the advection texture used for water rendering
     *
     * @param latency - The time latency for the advection texture used for water rendering
     */
    void SetLatency(float latency) 
    { 
        m_texAdvection.SetLatency(latency); 
    }

    /**
     * @fn GetLatency()
     *
     * @brief Gets the latency for the advection texture used for water rendering
     *
     * @return - The time latency for the advection texture used for water rendering
     */
    float GetLatency() const 
    {
        return m_texAdvection.GetLatency(); 
    }

    /**
     * @fn MacroDefinition()
     *
     * @brief This method is used to setup macros for the shaders
     */
    static void MacroDefinition();

protected:

    /**
     * @class Material
     *
     * @brief A class to help render the water surface by shaders
     */
    class Material : 
        public CDXMaterialWVP
    {
    public:
        /**
         * @fn Material()
         *
         * @brief Class constructor for the Material class object
         *
         * @param rData - Common visualization data
         */
        Material(CVisual3DCommonData &rData);

        CFXResource m_cSurfaceMap;                  // 2D texture with water height and bottom values
        CFXResource m_cBottomDiff;                  // 2D texture with bottom and beach colors
        CFXResource m_cSurfaceDiff;                 // 2D texture with 2 channels for advection simulation

        CFXResource m_cSurfaceTexCoords;            // 2D texture with m_cSurfaceDiff texture coordinate to simulate advection.
                                                    // This texture's coordinates are generated based on the U and V components of the 
                                                    // water simulation

        CFXResource m_cSkyEnvMap;                   // The texture of the sky
        CFXResource m_cBottomGradient;              // A small texture with gradient to draw the bottom
        CFXResource m_cSurfaceGradient;             // A small texture with gradient to draw the surface

        CFXVector   m_cIndexXForm;                  // A vector that contain dimW-1, dimH-1, and minimal n such that 2^n >= dimW
        CFXVector   m_cNormalScale;                 // 2D value that used to scale normals for advection drawing
        CFXVector   m_cTexBlendWeights;             // Texture blending weights
        CFXVector   m_cCamPos;                      // The camera position
        CFXVector   m_cBottomScales;                // Scale for the bottom
        CFXVector   m_cSurfaceLocalNormalScale;     // Normal scale for the surface
    };

    Material          m_material;                   // Data for the shaders

    CFlux2DScene&          m_scene;                 // A reference to the rendered water scene
    CVisual3DCommonData&   m_rCommonData;           // Common data for all renderer
    CDXRenderTarget        m_surface;               // A DX surface containing the water height map interpolated from all grids
    CDXRenderTarget        m_velocity;              // A DX 2 channel surface contained the water bar speed. it contains U and V components
    CVisual3DTexAdvection  m_texAdvection;          // A texture to show the surface of moving water

    float             m_maxDepth;                   // The maximal depth for rendering
    float             m_tileSize;
    float             m_globalNormalScale;          // The global scale for normal
    float             m_localNormalScale;           // The local scale for normal

    int               m_numIndices;                 // The number of indexes in m_pIBuff
    ID3D10Buffer      *m_pIBuff;                    // An index buffer to draw the 2D regular grid
    ID3D10Buffer      *m_pVBuff;                    // A buffer of 2D vertices. Each vertex is a 2d coordinate (x,y) for each water bar

    ID3D10ShaderResourceView *m_pBottomGrad;        // A small texture that contains the color map to draw the bottom
    ID3D10ShaderResourceView *m_pSurfaceGrad;       // A small texture that contains the color map to draw the water surface

    /**
     * @fn CreateIndexAndVertexBufferAccordingToBottom()
     *
     * @brief Creates an index and corresponding vertice buffer to help render surfaces
     *
     * @param dimW - The width of the grid to render
     *
     @ @param dimH - The height of the grid to render
     */
    void CreateIndexAndVertexBufferAccordingToBottom(int dimW, int dimH);

    /**
     * @fn RenderTheBuffer()
     *
     * @brief Renders the vertex m_pVBuffand and the index m_pIBuff buffers to draw 
     *        either the water surface or the bottom surface
     */
    void RenderTheBuffer();

    /**
     * @fn UpdateParameters()
     *
     * @brief Transfers new parameter values to the shaders
     */
    void UpdateParameters();

    static D3D10_SHADER_MACRO   m_pMacro[2];

    void*             m_pSceneHeightAndBottomBuffer;     // buffer to receive height and bottom from OCL
    void*             m_pSceneVelocityBuffer;            // buffer to receive velocity from OCL

};
// End of File