/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */


/**
 * @file CFXConstWrapper.h
 *
 * @brief  Defines wrapper classes, variables, and functions to manage effects 
 */

#pragma once

#include <d3d10.h>
#include <d3dx10.h>
#include "macros.h"

/**
 * @ckass CFXConstWrapper
 *
 * @brief  Converts effect variables into constants.
 */
template <class T>
class CFXConstWrapper
{
public:
    /**
     * @fn CFXConstWrapper(ID3D10Effect *pEffect, char const *name)
     *
     * @brief  Converts an effect variable into a constant.
     *
     * @param pEffect - The effect from which to extract the variable
     *
     * @param name - The name of the effect variable
     */
    CFXConstWrapper(ID3D10Effect *pEffect, char const *name)
    {
        // Declare and initialize a working copy of the effect variable
        ID3D10EffectVariable *pVar = pEffect->GetVariableByName( name );
        // Ensure validity
        assert(pVar != NULL);
        // Set the constant's value from the effect
        if (pVar != NULL)
        {
            m_pConst = Variable2Const(pVar);
        }
        else
        {
            m_pConst = NULL;
        }
    }

protected:
    /**
     * @fn Variable2Const(ID3D10EffectVariable *pVar)
     *
     * @brief Template function to do the conversion from variable to constant
     *
     * @param pVar - The effect variable whose value is to be produced
     *
     * @return The value of the effect variable
     */
    T * Variable2Const(ID3D10EffectVariable *pVar);

    /**
     * @var m_pConst
     *
     * @brief The constant value of the effect variable
     */
    T * m_pConst;
};

/**
 * @struct CVectorInt
 *
 * @brief Implements an integer version of a (X,Y,Z,W) vector
 */
struct CVectorInt
{
    /** 
     * @fn CVectorInt(int x_, int y_, int z_, int w_)
     *
     * @brief Class constructor, initializes the integer vector
     *
     * @param x_ - The X coordiante of the vector
     *
     * @param y_ - The Y coordinate of the vector
     *
     * @param z_ - The Z coordinate of the vector
     *
     * @param w_ - The W coordinate of the vector
     */
    CVectorInt(int x_, int y_, int z_, int w_) : x(x_), y(y_), z(z_), w(w_) 
    {
    }
    /**
     * @var x - X vector coordinate
     * @var y - Y vector coordinate
     * @var z - Z vector coordinate
     * @var w - W vector coordinate
     */
    int x, y, z, w;
};


/**
 * @fn Variable2Const(pVar)
 *
 * @brief Extracts the value of an effect variable and returns a constant value of the appropriate type
 *
 * @note The function is overloaded based on the expected return value type
 *
 * @return - A pointer to a ID3D10EffectMatrixVariable
 * @return - A pointer to a ID3D10EffectVectorVariable
 * @return - A pointer to a ID3D10EffectShaderResourceVariable
 * @return - A pointer to a ID3D10EffectScalarVariable
 */
inline ID3D10EffectMatrixVariable* CFXConstWrapper<ID3D10EffectMatrixVariable>::Variable2Const(ID3D10EffectVariable *pVar) { return pVar->AsMatrix(); }
inline ID3D10EffectVectorVariable* CFXConstWrapper<ID3D10EffectVectorVariable>::Variable2Const(ID3D10EffectVariable *pVar) { return pVar->AsVector(); }
inline ID3D10EffectShaderResourceVariable* CFXConstWrapper<ID3D10EffectShaderResourceVariable>::Variable2Const(ID3D10EffectVariable *pVar) { return pVar->AsShaderResource(); }
inline ID3D10EffectScalarVariable* CFXConstWrapper<ID3D10EffectScalarVariable>::Variable2Const(ID3D10EffectVariable *pVar) { return pVar->AsScalar(); }


/**
 * @class CFXMatrix
 *
 * @brief Implements a D3DXMatrix wrapper 
 */
class CFXMatrix : public CFXConstWrapper<ID3D10EffectMatrixVariable>
{
public:

    /**
     * @fn CFXMatrix(ID3D10Effect *pEffect, char const *name)
     *
     * @brief Child class constructor
     *
     * @param pEffect - The effect 
     *
     * @param - The name of the effect variable
     */
    CFXMatrix(ID3D10Effect *pEffect, char const *name) : CFXConstWrapper(pEffect, name) 
    {
    }

    /**
     * @fn Set(D3DXMATRIX const &matrix)
     *
     * @brief Sets the effect matrix variable
     *
     * @param matrix - The matrix as input
     */
    void Set(D3DXMATRIX const &matrix) 
    { 
        m_pConst->SetMatrix( const_cast<float*>((float const*)&matrix) ); 
    }
};

/**
 * @class CFXVector
 *
 * @brief Implements a vector wrapper
 */
class CFXVector : public CFXConstWrapper<ID3D10EffectVectorVariable>
{
public:

    /**
     * @fn CFXVector(ID3D10Effect *pEffect, char const *name)
     *
     * @brief Child class constructor
     *
     * @param pEffect - The effect 
     *
     * @param - The name of the effect variable
     */
    CFXVector(ID3D10Effect *pEffect, char const *name) : CFXConstWrapper(pEffect, name) 
    {
    }

    /**
     * @fn Set(D3DXVECTOR4 const &vector)
     *
     * @brief Sets the effect vector
     *
     * @param vector - A D3DXVECTOR4 vector as input
     */
    void Set(D3DXVECTOR4 const &vector)
    { 
        m_pConst->SetFloatVector( const_cast<float*>(&vector.x) ); 
    }

    /**
     * @fn Set(CVectorInt const &vector)
     *
     * @brief Sets the effect integer vector
     *
     * @param vector - A CVectorInt vector as input
     */
    void Set(CVectorInt const &vector)
    {
        m_pConst->SetIntVector( const_cast<int*>(&vector.x) ); 
    }

    /**
     * @fn Set(int num, D3DXVECTOR4 const *pVectors)
     *
     * @brief Sets the effect vector array
     *
     * @param num - The number of vectors to set
     *
     * @param pVectors - An array of DXDXVectors as input
     */
    void Set(int num, D3DXVECTOR4 const *pVectors)
    { 
        m_pConst->SetFloatVectorArray( const_cast<float*>(&pVectors[0].x), 0, num ); 
    }
};

/**
 * @class CFXResource
 *
 * @brief Implements a resource wrapper
 */
class CFXResource : public CFXConstWrapper<ID3D10EffectShaderResourceVariable>
{
public:

    /**
     * @fn CFXResource(ID3D10Effect *pEffect, char const *name)
     *
     * @brief Child class constructor
     *
     * @param pEffect - The effect 
     *
     * @param - The name of the effect variable
     */
    CFXResource(ID3D10Effect *pEffect, char const *name) : CFXConstWrapper(pEffect, name) 
    {
    }

    /**
     * @fn Set(ID3D10ShaderResourceView *pSRV)
     *
     * @brief Sets the effect's resource to a Shader Resource View
     *
     * @param pSRV - The shader resource view resource
     */
    void Set(ID3D10ShaderResourceView *pSRV)
    { 
        m_pConst->SetResource( pSRV ); 
    }

    /**
     * @fn Set(int num, ID3D10ShaderResourceView **ppSRV)
     *
     * @brief Sets the effect's resource to a Shader Resource View array
     *
     * @param num - The number of Shader Resource Views to set
     *
     * @param ppSRV - Array of Shader Resource View resources
     */
    void Set(int num, ID3D10ShaderResourceView **ppSRV) 
    { 
        m_pConst->SetResourceArray( ppSRV, 0, num ); 
    }
};

/**
 * @class CFXScalar
 *
 * @brief Implements a scalar wrapper
 */
class CFXScalar : public CFXConstWrapper<ID3D10EffectScalarVariable>
{
public:

    /**
     * @fn CFXScalar(ID3D10Effect *pEffect, char const *name)
     *
     * @brief Child class constructor
     *
     * @param pEffect - The effect 
     *
     * @param - The name of the effect variable
     */
    CFXScalar(ID3D10Effect *pEffect, char const *name) : CFXConstWrapper(pEffect, name) 
    {
    }

    /**
     * @fn Set(float const &vector)
     *
     * @brief Sets the effect's resource to a scalar vector
     *
     * @param vector - The scalar vector
     */
    void Set(float const &vector)
    {
        m_pConst->SetFloat(vector);
    }
};
