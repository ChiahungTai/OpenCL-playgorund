/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFlux2DSurface.h
 *
 * @brief Defines and provides utilities to manage a flux 2D surface.
 */

#pragma once

#include <memory.h>
#include "macros.h"
#include "CommonDataStruct.h"
#include "Util/memory.h"

/**
 * @fn SpecialResize(DynamicMatrix<T>* pSrc,DynamicMatrix<T>* pDst)
 *
 * @brief Resizes a matrix
 *
 * @note The destination matrix must be pre-initialized, (height, width and allocation)
 *
 * @param pSrc - The source matrix to be resized
 *
 * @param pDst - The target matrix, the source resized, output.
 */
template <typename T> 
void SpecialResize(DynamicMatrix<T>* pSrc,DynamicMatrix<T>* pDst)
{
    // Get the smaller border of the two
    int     b = min(pSrc->Border(),pDst->Border());
    // Declare working variables
    int     x,y;
    float   SX,SY;
    // Get the destination width and height
    int WDst = pDst->Width();
    int HDst = pDst->Height();
    // Get the source width and height
    int Crc = pSrc->Width();
    int HSrc = pSrc->Height();

    // Ensure validity
    assert(HDst>1);
    assert(WDst>1);

    // Compute ratios dividing source by destination
    SX = (float)(Crc-1)/(float)(WDst-1);
    SY = (float)(HSrc-1)/(float)(HDst-1);

    // We fit the source into the destination...

    // For every line in the destination including the borders
    for(y=-b;y<HDst+b;++y)
    {
        // For every pixel in the line including the borders
        for(x=-b;x<WDst+b;++x)
        {
            // Compute the source values
            float   xf = (x)*SX;
            float   yf = (y)*SY;
            // Take the integer portion
            int     xi = (int)xf;
            int     yi = (int)yf;
            // And the remainder
            float   xr = xf-xi;
            float   yr = yf-yi;

            // If on the left border
            if(x<=0)
            {
                xi = x;
                xr = 0;
            }

            // If on the right border
            if(x>=(WDst-1))
            {
                xi = x+(Crc-1)-(WDst-1)-1;
                xr = 1;
            }

            // If on the bottom border
            if(y<=0)
            {
                yi = y;
                yr = 0;
            }

            // If on the top border
            if(y>=(HDst-1))
            {
                yi = y+HSrc-HDst-1;
                yr = 1;
            }
            // Update the destination value by extracting the value from the source at the computed coordinates
            pDst->At(x,y) = 
                (1-yr)*((1-xr)*pSrc->At(xi,yi) + xr*pSrc->At(xi+1,yi))+
                (yr)*((1-xr)*pSrc->At(xi,yi+1) + xr*pSrc->At(xi+1,yi+1));
        }// next x (pixel)
    }// next y (line)
}// End of SpecialResize()

/**
 * @fn CopyData(DynamicMatrix<T>* pSrc,DynamicMatrix<T>* pDst)
 *
 * @brief Copies the source matrix into the destination matrix
 *
 * @note The destination matrix must be pre-initialized
 *
 * @param pSrc - The source matrix
 *
 * @param pDst - The destination matrix
 */
template <typename T> 
void CopyData(DynamicMatrix<T>* pSrc,DynamicMatrix<T>* pDst)
{
    // Find the minimum border size
    int     b = min(pSrc->Border(),pDst->Border()); 
    int     x,y;
    // Get the destination height and width
    int WDst = pDst->Width();
    int HDst = pDst->Height();

    // Validate the matricies
    assert(HDst>3);
    assert(WDst>3);
    assert(WDst==pSrc->Width());
    assert(HDst==pSrc->Height());

    // For every line in the destination
    for(y=-b;y<HDst+b;++y)
    {
        // For every pixel in the line
        for(x=-b;x<WDst+b;++x)
        {
            // Copy the source into the destination
            pDst->At(x,y) = pSrc->At(x,y); 
        }// Next x (pixel)
    }// Next y (line)
}// End of  CopyData()

/**
 * @class CFlux2DSurface
 *
 * @brief This class defines the contents and functions of a surface
 *
 * @note The class defines the U, V, and H maps, provides a temporary
 *       working map of the same size, and defines map manipulation
 *        functionality
 */
class CFlux2DSurface
{
public:

    /**
     * @var m_heightMap
     *
     * @brief Surface height map
     */
    DynamicMatrix<float> m_heightMap;

    /**
     * @var m_uMap
     *
     * @brief Surface U map (horizontal)
     */
    DynamicMatrix<float> m_uMap;

    /**
     * @var m_vMap
     *
     * @brief Surface V map (vertical)
     */
    DynamicMatrix<float> m_vMap;

    /**
     * @var m_TmpMap
     *
     * @brief Temporary working map
     */
    DynamicMatrix<float> m_TmpMap;
    
    /**
     * @fn CFlux2DSurface(int domainW, int domainH)
     *
     * @brief Class constructor, initialized and clears maps
     *
     * @param domainW - Width of the surface
     *
     * @param domainH - Height of the surface
     */
    CFlux2DSurface(int domainW, int domainH)
    {
        m_heightMap.SetSize(domainW, domainH, 1);
        m_uMap.SetSize(domainW, domainH, 1);
        m_vMap.SetSize(domainW, domainH, 1);
        m_TmpMap.SetSize(domainW, domainH, 1);
        m_heightMap.Set(0);
        m_uMap.Set(0);
        m_vMap.Set(0);
    }
    
    /**
     * @fn SetDomainSize(int w, int h)
     *
     * @brief Sets the domain size of the surface
     *
     * @param w - Width
     *
     * @param h - Height
     */
    void SetDomainSize(int w, int h)
    {
        // Validate height and width
        assert(w>3);
        assert(h>3);
        // Set the size of the temporary map
        m_TmpMap.SetSize(w,h);

        // Resize the surfaces of the height map into the temporary map
        SpecialResize<float>(&m_heightMap,&m_TmpMap);
        // Set the new size for the height map
        m_heightMap.SetSize(w,h);
        // Then copy the temporary map contents into the height map
        CopyData<float>(&m_TmpMap,&m_heightMap);

        // Repeat this process for the U and V maps

        SpecialResize<float>(&m_uMap,&m_TmpMap);
        m_uMap.SetSize(w,h);
        CopyData<float>(&m_TmpMap,&m_uMap);

        SpecialResize<float>(&m_vMap,&m_TmpMap);
        m_vMap.SetSize(w,h);
        CopyData<float>(&m_TmpMap,&m_vMap);

    }// End of SetDomainSize(w,h)

    /**
     * @fn Save(CFile &file)
     *
     * @brief Saves the surface to the specified IO stream
     *
     * @param file - The IO stream
     */
    void Save(CFile &file) const
    {
        m_heightMap.Save(file);
        m_uMap.Save(file);
        m_vMap.Save(file);
    }

    /**
     * @fn Restore(CFile &file)
     *
     * @brief Reads the surface from the specified IO stream
     *
     * @param file - The IO stream
     */
    void Restore(CFile &file)
    {
        m_heightMap.Restore(file);
        m_uMap.Restore(file);
        m_vMap.Restore(file);
    }
};

/**
 * @class CFlux2DBottom
 *
 * @brief Defines the maps and functions for the bottom
 */
class CFlux2DBottom
{
public:

    /**
     * @var m_heightMap
     *
     * @brief The bottom surface height map
     */
    DynamicMatrix<float> m_heightMap;

    /**
     * @var m_precompDataMap
     *
     * @brief Pre-computed data for the height map
     *
     * @note Here we'll precompute the h*sqrt(gh) value
     */
    DynamicMatrix<float> m_precompDataMap;

    /**
     * @var m_heightMapBase
     *
     * @brief The base height map for the bottom
     */
    DynamicMatrix<float> m_heightMapBase;

    /**
     * @var m_precompDataMapBase
     *
     * @brief Pre-computed data for the base height map
     *
     * @note Here we'll precompute the h*sqrt(gh) value
     */
    DynamicMatrix<float> m_precompDataMapBase;

    /**
     * @fn CFlux2DBottom(int domainW, int domainH)
     *
     * @brief Class constructor
     *
     * @param domainW - Width of the surface domain
     *
     * @param domainH - Height of the surface domain
     */
    CFlux2DBottom(int domainW, int domainH) 
    {
        m_heightMap.SetSize(domainW, domainH,1);
        m_precompDataMap.SetSize(domainW, domainH,1);
        m_heightMapBase.SetSize(domainW, domainH,1);
        m_precompDataMapBase.SetSize(domainW, domainH,1);
    }

    /**
     * @fn SetDomainSize(int w, int h)
     *
     * @brief Sets the domain size for the bottom
     *
     * @param w - Width of the domain
     *
     * @param h - Height of the domain
     */
    void SetDomainSize(int w, int h)
    {
        // If there is no difference between the base height map
        // and the height map, then just copy the data.
        if( m_heightMap.Width()==m_heightMapBase.Width() &&
            m_heightMap.Height()==m_heightMapBase.Height())
        {   
            // Coy the height map into the base height map to preserve it
            CopyData<float>(&m_heightMap,&m_heightMapBase);
            // Do the same for the pre-computed data
            CopyData<float>(&m_precompDataMap,&m_precompDataMapBase);
        }

        // Set the size of the height and pre-computed height maps
        m_precompDataMap.SetSize(w,h);
        m_heightMap.SetSize(w,h);

        // If the height map and the base height map are the same size
        // then just copy the data
        if( m_heightMap.Width()==m_heightMapBase.Width() &&
            m_heightMap.Height()==m_heightMapBase.Height())
        {
            CopyData<float>(&m_precompDataMapBase,&m_precompDataMap);
            CopyData<float>(&m_heightMapBase,&m_heightMap);
        }
        else
        {
            // They are not the same size, re-size is required
            SpecialResize<float>(&m_precompDataMapBase,&m_precompDataMap);
            SpecialResize<float>(&m_heightMapBase,&m_heightMap);
        }
    }

    /**
     * @fn Save(CFile &file)
     *
     * @brief Saves the bottom to the specified IO stream
     *
     * @param file - The IO stream
     */
    void Save(CFile &file) const
    {
        m_heightMap.Save(file);
        m_precompDataMap.Save(file);
    }

    /**
     * @fn Restore(CFile &file)
     *
     * @brief Restores the bottom from the specified IO stream
     *
     * @param file - The IO stream
     */
    void Restore(CFile &file)
    {
        m_heightMap.Restore(file);
        m_precompDataMap.Restore(file);
    }
};

/**
 * @struct CFluxVec
 *
 * @brief A vector with H, U, and V components
 */
template<typename T>
struct CFluxVec
{
    T H;
    T u;
    T v;
};

/**
 * @struct CFluxVecPtr
 *
 * @brief Implements a pointer version of the vector with H, U, and V components
 */
struct CFluxVecPtr
{
    float *pH;
    float *pU;
    float *pV;

    /**
     * @fn CFluxVecPtr()
     *
     * @brief Structure constructor, initializes buffers to NULL
     */
    CFluxVecPtr() 
        : pH(NULL), 
        pU(NULL), 
        pV(NULL)
    {
    
    }

    /**
     * @fn CFluxVecPtr(CFlux2DSurface &surf, int x0, int y0)
     *
     * @brief Parameterized structure constructor
     *
     * @param surf - The surface from which to extract the vector
     *
     * @param x0 - The X coordinate of the surface
     *
     * @param y0 - The Y coordinate of the surface
     */
    CFluxVecPtr(CFlux2DSurface &surf, int x0, int y0) 
    { 
        Init(surf, x0,y0); 
    }

    /**
     * @fn Init(CFlux2DSurface &surf, int x0, int y0)
     *
     * @brief Initializes a vector pointer into the surface at (x0,y0)
     *
     * @param surf - The surface from which to extract the vector
     *
     * @param x0 - The X coordinate of the surface
     *
     * @param y0 - The Y coordinate of the surface
     */
    void Init(CFlux2DSurface &surf, int x0, int y0)
    {
        pH = &(surf.m_heightMap.At(x0,y0));
        pU = &(surf.m_uMap.At(x0,y0));
        pV = &(surf.m_vMap.At(x0,y0));
    }

    /**
     * @fn Advance (int step =1)
     *
     * @brief Advances the three buffers in the structure
     *
     * @param step - The measure of the advancement
     */
    void Advance(int step=1)
    {
        pH += step;
        pU += step;
        pV += step;
    }
};
