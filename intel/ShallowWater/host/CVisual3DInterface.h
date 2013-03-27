/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CVisual3DInterface.h
 *
 * @brief Contains declarations for the visualization effects interface 
 */

#pragma once

#include <d3d10.h>
#include <d3dx10.h>

/**
 * @class CVisual3DInterface
 *
 * @brief Defines functions for rendering the surface and bottom scenes
 */
class CVisual3DInterface
{
public:
    virtual ~CVisual3DInterface() 
    {
    }

    /**
     * @fn Update(struct VisMode const *pVM, float dt)
     *
     * @brief Virtual function used to update the scene grid by getting data from the
     *        OCL side and preparing it for rendering
     *
     * @param pVM - The visualization mode. (Does not make sense for CVisual3DGathered) 
     *
     * @param dt - The time step
     */
    virtual void Update(struct VisMode const *pVM, float dt) = 0;

    /**
     * @fn RenderSurface(D3DXMATRIX const &rWorldViewProj, D3DXVECTOR3 const &rCamPos)
     *
     * @brief Pure virtual function that renders the surface of the water
     *
     * @param rWorldViewProj - The world view projection matrix
     *
     * @param rCamPos - The position of the camera
     */
    virtual void RenderSurface(D3DXMATRIX const &rWorldViewProj, D3DXVECTOR3 const &rCamPos) = 0;

    /**
     * @fn RenderBottom(D3DXMATRIX const &rWorldViewProj)
     *
     * @brief Pure virtual fucntion that renders the bottom of the scene
     *
     * @param rWorldViewProj - The world view projection matrix
     */
    virtual void RenderBottom(D3DXMATRIX const &rWorldViewProj) = 0;

    /**
     * @fn GetName()
     *
     * @brief Pure virtual function used to obtain the name of the render engine
     */
    virtual wchar_t const * GetName() const = 0;
};