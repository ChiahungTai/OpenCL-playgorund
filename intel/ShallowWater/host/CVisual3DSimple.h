/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CVisual3DSimple.h
 *
 * @brief Defines classes used for simple rendering of the water surface and the bottom grids
 */


#pragma once

#include "Util/CRegion.h"
#include "CVisual3DCommonData.h"
#include "CVisual3DBottom.h"
#include "CVisual3DInterface.h"
#include "util/memory.h"
#include "flux/CFlux2DScene.h"

/**
 * @class CVisual3DSimple
 *
 * @brief A class that declares virtual rendering functions for grids
 */
class CVisual3DSimple : 
    public CVisual3DInterface
{
public:

    /**
     * @fn CVisual3DSimple(CFlux2DScene &rScene, CVisual3DCommonData &rData)
     *
     * @brief Class constructor, creates buffers
     *
     * @param rScene - The scene for this instance of the class
     *
     * @param rData - Common data, shader resource views
     */
    CVisual3DSimple(CFlux2DScene &rScene, CVisual3DCommonData &rData);

    /**
     * @fn ~CVisual3DSimple()
     *
     * @brief Class Destructor, deletes grids and destroys all buffers
     */
    ~CVisual3DSimple();

    NON_COPYABLE(CVisual3DSimple);

    /**
     * @fn Update(struct VisMode const *pVM, float dt)
     *
     * @brief Updates the display, processes all buffers
     *
     * @param pVM - The visualization mode
     *
     * @param dt - Delta time
     */
    virtual void Update(struct VisMode const *pVM, float dt);

    /**
     * @fn RenderSurface(D3DXMATRIX const &rWorldViewProj, D3DXVECTOR3 const &rCamPos)
     *
     * @brief Renders the water surface
     *
     * @param rWorldViewProj - The world view projection matrix
     *
     * @param rCamPos - A vector representing the camera position
     */
    virtual void RenderSurface(D3DXMATRIX const &rWorldViewProj, D3DXVECTOR3 const &rCamPos);

    /**
     * @fn RenderBottom(D3DXMATRIX const &rWorldViewProj)
     *
     * @brief Renders the bottom scene
     *
     * @param rWorldViewProj - The world view projection matrix
     */
    virtual void RenderBottom(D3DXMATRIX const &rWorldViewProj);

    /**
     * @fn GetName()
     *
     * @brief Gets the name of the method
     *
     * @param The method name
     */
    virtual wchar_t const * GetName() const 
    { 
        return L"Simple"; 
    }

private:
    class GridData
    {
    public:

        /**
         * @fn GridData(CFlux2DGrid* pG, CRegion const &rReg, bool bVisBC)
         *
         * @brief Class constructor
         *
         * @param pG - The grid
         *
         * @param rReg - The region
         *
         * @param bVisBC - Boolean variable indicating the visiblity of borders
         */
        GridData(CFlux2DGrid* pG, CRegion const &rReg, bool bVisBC);

        /**
         * @fn ~GridData()
         *
         * @brief Class destructor
         */
        ~GridData();


        /**
         * @fn Update(pG, rReg, pVM, bVisBC)
         *
         * @brief Updates the surface buffer, creates a mesh
         *
         * @param pG - The grid to update to
         *
         * @param rReg - The region for the update
         *
         * @param pVM - The visualization mode
         *
         * @param bVisBC - Boolean variable indicating the visiblity of borders
         */
        void Update(CFlux2DGrid* pG, CRegion const &rReg, VisMode const *pVM, bool bVisBC);

        /**
         * @fn UpdateSize(CFlux2DGrid* pG)
         *
         * @brief Resizes the buffers if the size of the grid has changed
         *
         * @param The grid to be used in the resize operation
         */
        void UpdateSize(CFlux2DGrid* pG);

        /**
         * @fn RenderSurface()
         *
         * @brief Renderes the water surface
         */
        void RenderSurface();

        /**
         * @fn RenderBottom()
         *
         * @brief Renders the bottom scene
         */
        void RenderBottom();

        /**
         * @var m_numIndices
         *
         * @brief The number of indicies in the buffer
         */
        int               m_numIndices;

        /**
         * @var m_pSurfaceVBuff
         *
         * @brief Buffer containing the water surface V component
         */
        ID3D10Buffer      *m_pSurfaceVBuff;

        /**
         * @var m_bottom
         *
         * @brief The bottom scene
         */
        CVisual3DBottom  m_bottom;

    protected:

        /**
         * @var m_dimW
         *
         * @brief Width of the grid
         */
        int m_dimW;

        /**
         * @var m_dimH
         *
         * @brief - Height of the grid
         */
        int m_dimH;

        /**
         * @var m_bVisBC
         *
         * @brief Boolean variable indicating whether or not borders are visible
         */
        int m_bVisBC;
    };


    /**
     * @fn IsCreated()
     *
     * @brief Used to determine if the scene has been created
     *
     * @return - TRUE if the scene has been created
     *           FALSE otherwise
     */
    bool IsCreated();

    /**
     * @var m_scene
     *
     * @brief The scene to be rendered
     */
    CFlux2DScene&          m_scene;

    /**
     * @var m_rCommonData
     *
     * @brief Common data structure
     */
    CVisual3DCommonData&   m_rCommonData;

    /**
     * @var m_bVisBC
     *
     * @brief A boolean variable indicating whether or not to display borders
     */
    bool                m_bVisBC;

    /**
     * @var m_bottomMat
     *
     * @brief Material for rendering the bottom
     */
    CVisual3DBottom::Material  m_bottomMat;

    /**
     * @var m_surfaceMat
     *
     * @brief Material for rendering the surface
     */
    CDXMaterialWVP             m_surfaceMat;


    /**
     * @var m_Grids
     *
     * @brief Dynamic arrays of grids
     */
    DynamicArray<GridData*> m_Grids;



    /**
     * @var m_pHeightBuffer
     *
     * @brief A buffer to store the regular water height on OCL
     */
    void*            m_pHeightBuffer;

    /**
     * @var m_pUBuffer
     *
     * @brief A buffer to store the U component of the water bars on OCL
     */
    void*            m_pUBuffer;

    /**
     * @var m_pVBuffer
     *
     * @brief A buffer to store the V component of the water bars on OCL
     */
    void*            m_pVBuffer;


};
