/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CDXMeshBase.cpp
 *
 * @brief Implements material and mesh base classes
 */

#include <DXUT.h>
#include "macros.h"
#include "CVBGridGenerator.h"
#include "CDXMeshBase.h"

/**
 * @fn CDXMaterialBase( effectName, numVtxElements, pDesc, pMacro)
 *
 * @brief Class constructor, loads the effect and creates an input layout
 *
 * @param effectName - The effect to load
 *
 * @param numVtxElements - The number of vertex elements
 *
 * @param pDesc - The input description 
 *
 * @param pMacro - The effect shader macro
 */
CDXMaterialBase::CDXMaterialBase( wchar_t const *effectName, 
    int numVtxElements, 
    D3D10_INPUT_ELEMENT_DESC *pDesc, 
    D3D10_SHADER_MACRO* pMacro )
{
    // Load the effect
    m_pEffect = 0;
    m_pEffect = LoadEffect(effectName, pMacro);
    m_pVtxLayout = NULL;

    // Create the vertex layout
    if(m_pEffect)
    {
        D3D10_PASS_DESC passDesc;
        m_pEffect->GetTechniqueByIndex(0)->GetPassByIndex(0)->GetDesc(&passDesc);
        HRESULT hr = DXUTGetD3D10Device()->CreateInputLayout(pDesc, numVtxElements, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &m_pVtxLayout);
        assert(hr == S_OK);
        hr;
    }
}

/**
 * @fn ~CDXMaterialBase()
 *
 * @brief Class destructor
 */
CDXMaterialBase::~CDXMaterialBase()
{
    SAFE_RELEASE(m_pVtxLayout);
    SAFE_RELEASE(m_pEffect);
}

/**
 * @fn Apply(int techIndex)
 *
 * @brief Sets the input layout and applies the technique specified by index
 *
 * @param techindex - The index for the technique
 */
void CDXMaterialBase::Apply(int techIndex) const
{
    DXUTGetD3D10Device()->IASetInputLayout(m_pVtxLayout);
    m_pEffect->GetTechniqueByIndex(techIndex)->GetPassByIndex(0)->Apply(0);
}

/**
 * @fn ApplyEffectOnly(int techIndex = 0)
 *
 * @brief Applies the technique without setting the input layout
 *
 * @param techIndex - The index of the technique to apply
 */
void CDXMaterialBase::ApplyEffectOnly(int techIndex) const
{
    m_pEffect->GetTechniqueByIndex(techIndex)->GetPassByIndex(0)->Apply(0);
}

/**
 * @fn ApplyRestorePass(int techIndex = 0)
 *
 * @brief Applies an effect pass based on the technique index parameter value
 *
 * @param techIndex - The index to the techique
 */
void CDXMaterialBase::ApplyRestorePass(int techIndex) const
{
    ID3D10EffectPass *pP = m_pEffect->GetTechniqueByIndex(techIndex)->GetPassByIndex(1);
    if (pP != NULL)
    {
        pP->Apply(0);
    }
}

/**
 * @fn Render()
 *
 * @brief Renders the mesh
 */
void CDXMeshBase::Render()
{
    // Get the output device
    ID3D10Device *pDev = DXUTGetD3D10Device();
    // Set the topology
    pDev->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    // Set the index buffer
    pDev->IASetIndexBuffer(m_pIBuff, DXGI_FORMAT_R32_UINT, 0);
    // Set the vertex buffer and input layout, then render
    UINT offset = 0;
    pDev->IASetVertexBuffers(0, 1, &m_pVBuff, (UINT*)&m_vertexSize, &offset);
    pDev->DrawIndexed(m_numIndices, 0, 0);
}

/**
 * @fn ~CDXMeshBase()
 *
 * @brief Class destructor, releases buffers
 */
CDXMeshBase::~CDXMeshBase()
{
    SAFE_RELEASE(m_pVBuff);
    SAFE_RELEASE(m_pIBuff);
}
