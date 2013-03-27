/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CVisual3DTexAdvection.h
 *
 * @brief Contains declarations for visualization routines for texture advection effects (for gathered visualization mode)
 */

#pragma once

#include <d3d10.h>
#include "macros.h"
#include "Utils/CDXRenderTarget.h"
#include "Utils/CDXQuad.h"
#include "Utils/CDXMeshBase.h"

/**
 * @class CVisual3DTexAdvection
 *
 * @brief Declares variables and functions for texture advection effects
 */
class CVisual3DTexAdvection
{
public:
    enum 
    { 
        NUM_TEXCOORD_SETS = 3 
    };

    /**
     * @fn CVisual3DTexAdvection(int resW, int resH)
     *
     * @brief Class constructor, creates all texture coordinate sets
     *
     * @param resW - Width of the resource
     *
     * @param resH - Height of the resource
     */
    CVisual3DTexAdvection(int resW, int resH);

    /**
     * @fn ~CVisual3DTexAdvection()
     *
     * @brief Class destructor, deletes all texture coordiante sets
     */
    ~CVisual3DTexAdvection();


    /**
     * @fn SetSpatialDimensions(float lenW, float lenH, float texCoordScale)
     *
     * @brief Sets the maxmimum texture coordinates based on the parameters
     *
     * @param lenW - The length of the texture
     *
     * @param lenH - The height of the texture
     *
     * @param texCoordScale - The scale to apply to the length and height
     */
    void SetSpatialDimensions(float lenW, float lenH, float texCoordScale);

    /**
     * @fn SetLatency(float latency)
     *
     * @brief Sets the latency to the specified value
     *
     * @param latency - The specified latency value
     */
    void SetLatency(float latency) 
    { 
        m_resetLatency = latency; 
    }

    /**
     * @fn GetLatency() 
     *
     * @brief Retrieves the current latency setting
     *
     * @return The latency
     */
    float GetLatency() const 
    { 
        return m_resetLatency; 
    }

    /**
     * @fn InitTextureCoordinates()
     *
     * @brief Resets the texture for all texture coordinates
     */
    void InitTextureCoordinates();

    /**
     * @fn Update(CDXRenderTarget &rVelocityTex, float dt)
     *
     * @brief Calculates weights, resets coordinates as necessary, applies advection
     *
     * @param rVelocityTex - The texture to update
     *
     * @param dt - Delta time
     */
    void Update(CDXRenderTarget &rVelocityTex, float dt);


    /**
     * @fn &GetWeights( )
     *
     * @brief Used to retrieve the weight vector
     *
     * @return - The weight vector
     */
    D3DXVECTOR3 const &GetWeights( ) const 
    { 
        return m_weights; 
    }


    /**
     * @fn GetTextureCoordinateSet(int index)
     *
     * @brief Gets the coordinate set for the texture with the specified index
     *
     * @param index - The index into the texture buffer
     *
     * @return - The coordinate set
     */
    ID3D10ShaderResourceView *GetTextureCoordinateSet(int index) 
    {
        assert(0 <= index && index < NUM_TEXCOORD_SETS);
        return m_pCoords[index].m_pTexCoords[m_pingPongIndex]->GetSRV();
    }

private:

    class Material : 
        public CDXQuad::XFormMaterial
    {
    public:

        /**
         * @fn Material()
         *
         * @brief Class Constructor, loads material, velocity texture and spatial scale
         */
        Material();

        /**
         * @var m_cTexVelocity
         *
         * @brief Texture Velocity resource
         */
        CFXResource m_cTexVelocity;

        /**
         * @var m_cSpatialScale
         *
         * @brief A vector representing a scaling factor
         */
        CFXVector   m_cSpatialScale;
    };

    /**
     * @struct TexCoordSet
     *
     * @brief Defines a buffer for sets of texture coordinate pairs
     */
    struct TexCoordSet
    {
        CDXRenderTarget* m_pTexCoords[2];
    };

    int         m_pingPongIndex;
    Material    m_material;
    CDXQuad    m_quad;
    TexCoordSet m_pCoords[NUM_TEXCOORD_SETS];
    D3DXVECTOR2 m_maxTextureCoordinates;
    D3DXVECTOR2 m_spatialScale;
    float       m_resetLatency;
    float       m_time;
    int         m_resetIndex;
    D3DXVECTOR3 m_weights;

    /**
     * @fn ResetTexture(CDXRenderTarget &rTexCoord)
     *
     * @brief Generates a transform for the texture material and renders the quad
     *
     * @param rTexCoord - The texture to reset
     */
    void ResetTexture(CDXRenderTarget &rTexCoord);

    /**
     * @fn Advect(TexCoordSet &rSet, CDXRenderTarget &rVelocityTex, D3DXVECTOR2 const &rScale)
     *
     * @brief Sets coordinates in the shader resource view applying the scale and then renders
     *
     * @param rSet - A texture coordinate set
     *
     * @param rVelocityTex - Velocity texture
     *
     * @param rScale - Scaling factor
     */
    void Advect(TexCoordSet &rSet, CDXRenderTarget &rVelocityTex, D3DXVECTOR2 const &rScale);
};