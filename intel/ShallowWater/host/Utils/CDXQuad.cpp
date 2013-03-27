/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

#include <DXUT.h>
#include "macros.h"
#include "CVBGridGenerator.h"
#include "CDXQuad.h"

namespace
{
    /**
     * @var s_vtxLayoutDesc[]
     *
     * @brief Defines the input layout
     */
    D3D10_INPUT_ELEMENT_DESC s_vtxLayoutDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    };
}

/**
 * @fn Quad()
 *
 * @brief Class constructor, initializes resource data and creates a D3D10 device buffer
 */
CDXQuad::CDXQuad()
{
    m_vertexSize = sizeof(D3DXVECTOR2);
    m_numIndices = 4;

    D3DXVECTOR2 quad[4] = { D3DXVECTOR2(0.0f, 0.0f), D3DXVECTOR2(0.0f, 1.0f), D3DXVECTOR2(1.0f, 0.0f), D3DXVECTOR2(1.0f, 1.0f) };

    D3D10_SUBRESOURCE_DATA initData;
    initData.pSysMem          = quad;
    initData.SysMemPitch      = 0;
    initData.SysMemSlicePitch = 0;

    CD3D10_BUFFER_DESC desc(4 * sizeof(D3DXVECTOR2), D3D10_BIND_VERTEX_BUFFER, D3D10_USAGE_IMMUTABLE);
    HRESULT hr = DXUTGetD3D10Device()->CreateBuffer(&desc, &initData, &m_pVBuff);
    assert(hr == S_OK);
    hr;
}

/**
 * @fn Render()
 *
 * @brief Renders the quad
 */
void CDXQuad::Render()
{
    // Get the output device
    ID3D10Device *pDev = DXUTGetD3D10Device();
    // Set the output topology
    pDev->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    // Set the vertex buffer and input layout, then render
    UINT offset = 0;
    pDev->IASetVertexBuffers(0, 1, &m_pVBuff, (UINT*)&m_vertexSize, &offset);
    pDev->Draw(4, 0);
}

/**
 * @fn Material(wchar_t const *effectName)
 *
 * @brief Class constructor
 *
 * @param effectName - Name of the effect to load
 */
CDXQuad::Material::Material(wchar_t const *effectName) 
: CDXMaterialBase(effectName, DIM(s_vtxLayoutDesc), s_vtxLayoutDesc)
{}

/**
 * @fn XFormMaterial(wchar_t const *effectName)
 *
 * @brief Class constructor
 *
 * @param effectName - Name of the effect to load
 */
CDXQuad::XFormMaterial::XFormMaterial(wchar_t const *effectName)
: Material(effectName)
, m_cXForm(m_pEffect, "g_xforms")
, m_cTex(m_pEffect, "g_tex")
{}

/**
 * @fn SetFullQuadXFrom()
 *
 * @brief Sets the transformation vector to be a full quad
 *
 * @note The transformation vector has two internal vectors. In this
 *       case, the first is (2.0,-2.0,-1.9, 1.0) and the second is
 *       (1.0, 1.0, 0.0, 0.0)
 */
void CDXQuad::XFormMaterial::SetFullQuadXFrom()
{
    static D3DXVECTOR4 xformFullQuad[2] = { D3DXVECTOR4(2.0f, -2.0f, -1.0f, 1.0f), D3DXVECTOR4(1.0f, 1.0f, 0.0f, 0.0f) };
    SetXForm(xformFullQuad);
}

/**
 * @fn CDXQuadWithBoundary(int dimW, int dimH)
 *
 * @brief Class constructor. Defines a quad with a boundary, maps a region to the quad
 *
 * @param dimW - Width in pixels of the reqion
 *
 * @param dimH - Height in pixels of the region
 */
CDXQuadWithBoundary::CDXQuadWithBoundary( int dimW, int dimH )
    : m_uPixelStep(1.0f / float(dimW)),
    m_vPixelStep(1.0f / float(dimH))
{
    int lineIndices[5] = {0, 1, 3, 2, 0};
    m_pIB = CreateIndexBufferImmutable(5, lineIndices);
}

/**
 * @fn ~CDXQuadWithBoundary()
 *
 * @brief Class destructor. Releases the buffer
 */
CDXQuadWithBoundary::~CDXQuadWithBoundary()
{
    SAFE_RELEASE(m_pIB);
}

/**
 * @fn RenderInner(XFormMaterial &mat, int techIndex)
 *
 * @brief Renders the quad
 *
 * @param mat - The material to render
 *
 * @param techIndex - Technique index
 */
void CDXQuadWithBoundary::RenderInner(XFormMaterial &mat, int techIndex)
{
    D3DXVECTOR4 xform[2];

    // Construct the transformation vector
    Region2XForm(xform[0], m_uPixelStep * 2.0f - 1.0f, m_vPixelStep * 2.0f - 1.0f, 
        (1.0f - m_uPixelStep) * 2.0f - 1.0f, (1.0f - m_vPixelStep) * 2.0f - 1.0f);
    Region2XForm(xform[1], m_uPixelStep, m_vPixelStep, 
        (1.0f - m_uPixelStep), (1.0f - m_vPixelStep));
    // Set it
    mat.SetXForm(xform);
    // Apply the rendering technique and render the quad
    mat.Apply(techIndex);
    Render();
}

/**
 * @fn RenderBoundary(XFormMaterial &mat, int techIndex)
 *
 * @brief Renders the quad
 *
 * @param mat - The material to render
 *
 * @param techIndex - Technique index
 */
void CDXQuadWithBoundary::RenderBoundary(XFormMaterial &mat, int techIndex)
{
    D3DXVECTOR4 xform[2];

    // Construct the transformation vector
    Region2XForm(xform[0], -1.0f, -1.0f, 1.0f, 1.0f);
    Region2XForm(xform[1], 0.0f, 0.0f, 1.0f, 1.0f);
    // Set it
    mat.SetXForm(xform);
    // Apply the rendering techique
    mat.Apply(techIndex);

    // Get the device
    ID3D10Device *pDev = DXUTGetD3D10Device();
    // Set the output topology
    pDev->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP);
    // Set the index buffer
    pDev->IASetIndexBuffer(m_pIB, DXGI_FORMAT_R32_UINT, 0);
    // Set the vertex buffer and input layout, then render
    UINT offset = 0;
    pDev->IASetVertexBuffers(0, 1, &m_pVBuff, (UINT*)&m_vertexSize, &offset);
    // Draw four lines
    pDev->DrawIndexed(4, 0, 0);
}
