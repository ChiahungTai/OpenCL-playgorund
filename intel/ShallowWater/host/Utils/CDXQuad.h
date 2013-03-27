/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CDXQuad.h
 *
 * @brief Defines data structures and functions for quads (2x2 pixels)
 */

#pragma once

#include <d3d10.h>
#include "CDXMeshBase.h"

class CDXQuad : 
    public CDXMeshBase
{
public:

    /**
     * @fn Quad()
     *
     * @brief Class constructor, initializes resource data and creates a D3D10 device buffer
     */
    CDXQuad();

    /**
     * @class Material
     *
     * @brief Storage for the texture and vertex layout description
     */
    class Material : 
        public CDXMaterialBase
    {
    public:

        /**
         * @fn Material(wchar_t const *effectName)
         *
         * @brief Class constructor
         *
         * @param effectName - Name of the effect to load
         */
        Material(wchar_t const *effectName);
    };

    /**
     * @class XFormMaterial
     *
     * @brief Class used for tranforming a quad
     */
    class XFormMaterial : 
        public CDXQuad::Material
    {
    public:

        /**
         * @fn XFormMaterial(wchar_t const *effectName)
         *
         * @brief Class constructor
         *
         * @param effectName - Name of the effect to load
         */
        XFormMaterial(wchar_t const *effectName);

        /**
         * @fn SetXForm(D3DXVECTOR4 const *pXform)
         *
         * @brief Sets the material transform vector
         *
         * @param pXform - The transformation vector
         */
        void SetXForm(D3DXVECTOR4 const *pXform) 
        { 
            m_cXForm.Set(2, pXform); 
        }

        /**
         * @fn SetFullQuadXFrom()
         *
         * @brief Sets the transformation vector to be a full quad
         *
         * @note The transformation vector has two internal vectors. In this
         *       case, the first is (2.0,-2.0,-1.9, 1.0) and the second is
         *       (1.0, 1.0, 0.0, 0.0)
         */
        void SetFullQuadXFrom();

        /**
         * @var m_cXForm
         *
         * @brief The transformation vector
         */
        CFXVector   m_cXForm;

        /**
         * @var m_cTex
         *
         * @brief The texture
         */
        CFXResource m_cTex;
    };

    /**
     * @fn Render()
     *
     * @brief Renders the quad
     */
    virtual void Render();
};

/**
 * @class CDXQuadWithBoundary
 *
 * @brief Defines a quad with a boundary
 */
class CDXQuadWithBoundary : 
    public CDXQuad
{
public:

    /**
     * @fn CDXQuadWithBoundary(int dimW, int dimH)
     *
     * @brief Class constructor. Defines a quad with a boundary, maps a region to the quad
     *
     * @param dimW - Width in pixels of the reqion
     *
     * @param dimH - Height in pixels of the region
     */
    CDXQuadWithBoundary(int dimW, int dimH);

    /**
     * @fn ~CDXQuadWithBoundary()
     *
     * @brief Class destructor. Releases the buffer
     */
    ~CDXQuadWithBoundary();

    /**
     * @fn RenderInner(XFormMaterial &mat, int techIndex)
     *
     * @brief Renders the quad
     *
     * @param mat - The material to render
     *
     * @param techIndex - Technique index
     */
    void RenderInner(XFormMaterial &mat, int techIndex);

    /**
     * @fn RenderBoundary(XFormMaterial &mat, int techIndex)
     *
     * @brief Renders the quad
     *
     * @param mat - The material to render
     *
     * @param techIndex - Technique index
     */
    void RenderBoundary(XFormMaterial &mat, int techIndex);

protected:

    /**
     * @var m_uPixelStep
     *
     * @brief Horizontal pixel step (1/dimW, see class constructor above)
     */
    float        m_uPixelStep;

    /**
     * @var m_vPixelStep
     *
     * @brief Vertical pixel step (1/dimH, see class constructor above)
     */
    float        m_vPixelStep;

    /**
     * @var m_pIB
     *
     * @brief The index buffer
     */
    ID3D10Buffer *m_pIB;
};

/**
 * @fn Region2XForm(D3DXVECTOR4 &xform, float x1, float y1, float x2, float y2)
 *
 * @brief Converts a region into a transformation vector
 *
 * @param xform - Output - The transformation vector
 *
 * @param x1 - The upper-Left X coordinate of the region
 *
 * @param y1 - The upper-Left Y coordinate of the region
 *
 * @param x2 - The lower-right X coordinate of the region
 *
 * @param y2 - The lower-right Y coordinate of the region
 */
inline void Region2XForm(D3DXVECTOR4 &xform, float x1, float y1, float x2, float y2)
{
    xform.x = x2 - x1;
    xform.y = y2 - y1;
    xform.z = x1;
    xform.w = y1;
}
