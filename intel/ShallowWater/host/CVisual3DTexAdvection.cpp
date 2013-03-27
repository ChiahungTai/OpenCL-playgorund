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
 * @brief Contains implementations for visualization routines for texture advection effects (for gathered visualization mode)
 */

#include "Utils/CVBGridGenerator.h"
#include "CVisual3DTexAdvection.h"

#define DEFAULT_TEXTURE_COORDINATE_W 5.0f
#define DEFAULT_TEXTURE_COORDINATE_H 5.0f
#define DEFAULT_RESET_LATENCY 10.0f
#define DEFAULT_RESET_THRESHOLD 0.005f

/**
 * @fn CVisual3DTexAdvection(int resW, int resH)
 *
 * @brief Class constructor, creates all texture coordinate sets
 *
 * @param resW - Width of the resource
 *
 * @param resH - Height of the resource
 */
CVisual3DTexAdvection::CVisual3DTexAdvection(int resW, int resH) 
    : m_pingPongIndex(0), 
    m_maxTextureCoordinates(DEFAULT_TEXTURE_COORDINATE_W, DEFAULT_TEXTURE_COORDINATE_H), 
    m_resetLatency(DEFAULT_RESET_LATENCY), 
    m_resetIndex(NUM_TEXCOORD_SETS), 
    m_time(0.0f)
{
    CDXRTRestorer rt;

    // Create all texture coordinate sets
    for (int i = 0; i < NUM_TEXCOORD_SETS; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            m_pCoords[i].m_pTexCoords[j] = new CDXRenderTarget(resW, resH, DXGI_FORMAT_R32G32_FLOAT);
        }
    }

    m_resetIndex = 0;
}

/**
 * @fn InitTextureCoordinates()
 *
 * @brief Resets the texture for all texture coordinates
 */
void CVisual3DTexAdvection::InitTextureCoordinates()
{
    for (int i = 0; i < NUM_TEXCOORD_SETS; i++)
    {
        ResetTexture(*m_pCoords[i].m_pTexCoords[0]);
    }
}

/**
 * @fn Material()
 *
 * @brief Class Constructor, loads material, velocity texture and spatial scale
 */
CVisual3DTexAdvection::Material::Material()
    : CDXQuad::XFormMaterial(L"advect.fx"),
    m_cTexVelocity(m_pEffect, "g_texVelocity"),
    m_cSpatialScale(m_pEffect, "g_spatialScale")
{
    SetFullQuadXFrom();
}

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
void CVisual3DTexAdvection::SetSpatialDimensions(float lenW, float lenH, float texCoordScale)
{
    m_maxTextureCoordinates.x = lenW * texCoordScale;
    m_maxTextureCoordinates.y = lenH * texCoordScale;
    m_spatialScale.x = 1.0f / lenW;
    m_spatialScale.y = 1.0f / lenH;
}

/**
 * @fn Update(CDXRenderTarget &rVelocityTex, float dt)
 *
 * @brief Calculates weights, resets coordinates as necessary, applies advection
 *
 * @param rVelocityTex - The texture to update
 *
 * @param dt - Delta time
 */
void CVisual3DTexAdvection::Update(CDXRenderTarget &rVelocityTex, float dt)
{
    // Update weights
    float phase     = 2.0f * M_PI * m_time / m_resetLatency;
    float phaseStep = 2.0f * M_PI / float(NUM_TEXCOORD_SETS);
    int newResetIndex = -1;
    for (int i = 0; i < NUM_TEXCOORD_SETS; i++, phase -= phaseStep)
    {
        //Calculate texures weights
        m_weights[i] = (1.0f/((float)NUM_TEXCOORD_SETS)) * (1.0f - cosf(phase)); 
        if (m_weights[i] < DEFAULT_RESET_THRESHOLD)
        {
            newResetIndex = i;
        }
    }

    // Reset coordinates if neccessary
    if (newResetIndex >= 0 && m_resetIndex != newResetIndex)
    {
        m_resetIndex = newResetIndex;
        assert(m_weights[m_resetIndex] < DEFAULT_RESET_THRESHOLD);

        // Reset coordinate
        {
            CDXRTRestorer rt;
            ResetTexture(*m_pCoords[m_resetIndex].m_pTexCoords[m_pingPongIndex]);
        }
        DropResources();
    }


    // Advect all
    D3DXVECTOR2 scale = -m_spatialScale * dt;
    for (int i = 0; i < NUM_TEXCOORD_SETS; i++)
    {
        Advect(m_pCoords[i], rVelocityTex, scale);
    }

    // Ping-pong textures
    m_pingPongIndex = 1 - m_pingPongIndex;

    // Update time
    m_time += dt;
}

/**
 * @fn ResetTexture(CDXRenderTarget &rTexCoord)
 *
 * @brief Generates a transform for the texture material and renders the quad
 *
 * @param rTexCoord - The texture to reset
 */
void CVisual3DTexAdvection::ResetTexture(CDXRenderTarget &rTex)
{
    D3DXVECTOR4 xforms[2];
    xforms[0] = D3DXVECTOR4(2.0f, -2.0f, -1.0f, 1.0f);
    xforms[1] = D3DXVECTOR4(m_maxTextureCoordinates.x, m_maxTextureCoordinates.y, 0.0f, 0.0f);

    rTex.SetIt();
    m_material.SetXForm(xforms);
    m_material.Apply(0);
    m_quad.Render();
}

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
void CVisual3DTexAdvection::Advect(TexCoordSet &rSet, CDXRenderTarget &rVelocityTex, D3DXVECTOR2 const &rScale)
{
    D3DXVECTOR4 scale4D(rScale.x, rScale.y, 0.0f, 0.0f);

    CDXRTRestorer rt;
    rSet.m_pTexCoords[1-m_pingPongIndex]->SetIt();
    m_material.SetFullQuadXFrom();
    m_material.m_cTex.Set(rSet.m_pTexCoords[m_pingPongIndex]->GetSRV());
    m_material.m_cTexVelocity.Set(rVelocityTex.GetSRV());
    m_material.m_cSpatialScale.Set(scale4D);
    m_material.Apply(1);

    m_quad.Render();
}

/**
 * @fn ~CVisual3DTexAdvection()
 *
 * @brief Class destructor, deletes all texture coordiante sets
 */
CVisual3DTexAdvection::~CVisual3DTexAdvection()
{
    for (int i = 0; i < NUM_TEXCOORD_SETS; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            delete m_pCoords[i].m_pTexCoords[j];
        }
    }
}
