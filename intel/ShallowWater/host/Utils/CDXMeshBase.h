/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CDXMeshBase.h 
 *
 * @brief Declares material and mesh base classes
 */

#pragma once

#include <d3d10.h>
#include "CFXConstWrapper.h"

/**
 * @class CDXMaterialBase
 *
 * @brief Provides a base class for materials
 */
class CDXMaterialBase
{
public:

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
    CDXMaterialBase( wchar_t const *effectName, 
        int numVtxElements, 
        D3D10_INPUT_ELEMENT_DESC *pDesc, 
        D3D10_SHADER_MACRO* pMacro = 0 );

    /**
     * @fn ~CDXMaterialBase()
     *
     * @brief Class destructor
     */
    ~CDXMaterialBase();

    /**
     * @fn Apply(int techIndex = 0)
     *
     * @brief Sets the input layout and applies the technique specified by index
     *
     * @param techindex - The index for the technique
     */
    void Apply(int techIndex = 0) const;

    /**
     * @fn ApplyEffectOnly(int techIndex = 0)
     *
     * @brief Applies the technique without setting the input layout
     *
     * @param techIndex - The index of the technique to apply
     */
    void ApplyEffectOnly(int techIndex = 0) const;

    /**
     * @fn ApplyRestorePass(int techIndex = 0)
     *
     * @brief Applies an effect pass based on the technique index parameter value
     *
     * @param techIndex - The index to the techique
     */
    void ApplyRestorePass(int techIndex = 0) const;

protected:

    /**
     * @var m_pEffect
     *
     * @brief Pointer to an ID3D10 effect
     */
    ID3D10Effect       *m_pEffect;

    /**
     * @var m_pVtxLayout
     *
     * @brief Pointer to an ID3D10 input layout
     */
    ID3D10InputLayout  *m_pVtxLayout;
};

/**
 * @class CDXMaterialWVP
 *
 * @brief Declares the material for a world-view projection
 */
class CDXMaterialWVP : 
    public CDXMaterialBase
{
public:

    /**
     * @fn CDXMaterialWVP( effectName, numVtxElements, pDesc, pMacro)
     *
     * @brief Class constructor, loads effect and creates input layout
     *
     * @param effectName - The effect to load
     *
     * @param numVtxElements - The number of vertex elements
     *
     * @param pDesc - The input description 
     *
     * @param pMacro - The effect shader macro
     */
    CDXMaterialWVP( wchar_t const *effectName, 
        int numVtxElements, 
        D3D10_INPUT_ELEMENT_DESC *pDesc, 
        D3D10_SHADER_MACRO* pMacro = 0 ) 
        : CDXMaterialBase(effectName, numVtxElements, pDesc, pMacro),
        m_cWVP(m_pEffect, "g_mWVP")
    {

    }

    /**
     * @var m_cWVP
     *
     * @brief The world-view projection effect matrix
     */
    CFXMatrix m_cWVP;
};

/**
 * @class CDXMeshBase
 *
 * @brief Defines a base class for meshes 
 */
class CDXMeshBase
{
public:

    /**
     * @fn CDXMeshBase()
     *
     * @brief Class constructor
     */
    CDXMeshBase() : m_pVBuff(NULL), m_pIBuff(NULL) 
    {
    }

    /**
     * @fn ~CDXMeshBase()
     *
     * @brief Class destructor, releases buffers
     */
    virtual ~CDXMeshBase();

    /**
     * @fn Render()
     *
     * @brief Renders the mesh
     */
    virtual void Render();

    /**
     * @fn GetVB()
     *
     * @brief Used to retrieve a pointer to the vertext buffer
     *
     * @return - Pointer to the vertext buffer
     */
    ID3D10Buffer *GetVB() 
    { 
        return m_pVBuff; 
    }

    /**
     * @fn GetIB()
     *
     * @brief Used to retrieve a pointer to the index buffer
     *
     * @return - Pointer to the index buffer
     */
    ID3D10Buffer *GetIB() 
    { 
        return m_pIBuff; 
    }

protected:

    /**
     * @var m_numIndices
     *
     * @brief The number of indices in the index buffer
     */
    int          m_numIndices;

    /**
     * @var m_vertexSize
     *
     * @brief The size of the vertext buffer, the number of verticies
     */
    int          m_vertexSize;

    /**
     * @var m_pVBuff
     *
     * @brief The vertext buffer
     */
    ID3D10Buffer *m_pVBuff;

    /**
     * @var m_pIBuff
     *
     * @brief The index buffer
     */
    ID3D10Buffer *m_pIBuff;
};