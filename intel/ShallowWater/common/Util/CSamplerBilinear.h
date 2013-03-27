/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CSamplerBilinear.h
 *
 * @brief Contains the bilinear sampler routines implementation. 
 */

#pragma once

#include "util/memory.h"

/**
 * @class CSamplerBilinear
 *
 * @brief Class used to implement the bilinear routines
 */
class CSamplerBilinear
{
public:

    /**
     * @fn CSamplerBilinear()
     *
     * @brief Class constructor
     */
    CSamplerBilinear() 
    {
    
    }

    /**
     * @fn CSamplerBilinear(float u, float v)
     *
     * @brief Parameterized class constructor
     */
    CSamplerBilinear(float u, float v)
    {
        Init(u, v);
    }

    /**
     * @fn Init(float u, float v) 
     *
     * @brief Used to initialize the sampler
     */
    void Init(float u, float v)
    {
        m_iU = int(u);
        if(u<0) m_iU-=1;
        m_iV = int(v);
        if(v<0) m_iV-=1;
        m_fU = u - m_iU;
        m_fV = v - m_iV;
        W[0] = (1-m_fU)*(1-m_fV);
        W[1] = (m_fU)*(1-m_fV);
        W[2] = (1-m_fU)*(m_fV);
        W[3] = (m_fU)*(m_fV);
    }
    
    /**
     * @fn operator()
     *
     * @brief Sampler operator() implementation
     */
    float operator()(DynamicMatrix<float> *pM)  
    { 
        return
            W[0]*pM->At(m_iU,m_iV) + W[1]*pM->At(m_iU+1,m_iV)+
            W[2]*pM->At(m_iU,m_iV+1) + W[3]*pM->At(m_iU+1,m_iV+1);
    }

    /**
     * @fn AddValue(DynamicMatrix<float> *pM, float Val )
     *
     * @brief Adds a new value to the matrix
     *
     * @param pM - The dynamic matrix
     *
     * @param Val - The value to add to the matrix
     */
    void AddValue(DynamicMatrix<float> *pM, float Val )  
    { 
        pM->At(m_iU,m_iV)    += W[0]*Val;
        pM->At(m_iU+1,m_iV)  += W[1]*Val;
        pM->At(m_iU,m_iV+1)  += W[2]*Val;
        pM->At(m_iU+1,m_iV+1)+= W[3]*Val;
    }

    /**
     * @fn SetValue(DynamicMatrix<float> *pM, float Val )
     *
     * @brief Sets the matrix to the specified value
     */
    void SetValue(DynamicMatrix<float> *pM, float Val )  
    { 
        pM->At(m_iU,m_iV)    = W[0]*Val;
        pM->At(m_iU+1,m_iV)  = W[1]*Val;
        pM->At(m_iU,m_iV+1)  = W[2]*Val;
        pM->At(m_iU+1,m_iV+1)= W[3]*Val;
    }

    /**
     * @fn GetXY(int &x, int &y)
     *
     * @brief Retrieves the current XY coordinates
     *
     * @param x - output - The X coordinate
     *
     * @param y - output - The Y coordinate
     */
    void GetXY(int &x, int &y) const
    {
        y = m_iU;
        x = m_iV;
    }

    /**
     * @fn IsInsideRect(int x0, int y0, int x1, int y1 )
     *
     * @brief Check whether a point is inside a rectangle or not
     *
     * @param x0 - The left coordinate of the rectangle
     *
     * @param y0 - The bottom coordinate of the rectangle
     *
     * @param  x1 - The right coordinate of the rectangle
     *
     * @param  y0 - The top coordinate of the rectangle
     */
    int IsInsideRect(int x0, int y0, int x1, int y1 )
    {
        return (m_iU>=x0 && (m_iU+1)<=x1) && (m_iV>=y0 && (m_iV+1)<=y1);
    }

    /* ****************************************************************************************** *\
                                    Retrieve individual components
    \* ****************************************************************************************** */

    /**
     * @fn GetUFrac()
     *
     * @brief Retrieve the U fractional component
     */
    float GetUFrac() const 
    { 
        return m_fU; 
    }

    /**
     * @fn GetVFrac()
     *
     * @brief Retrieve the V fractional component
     */
    float GetVFrac() const 
    { 
        return m_fV; 
    }

    /**
     * @fn GetUInt()
     *
     * @brief Retrieve the U integer component
     */
    int GetUInt() const 
    { 
        return m_iU; 
    }

    /**
     * @fn GetVInt()
     *
     * @brief Retrieve the V integer component
     */
    int GetVInt() const 
    { 
        return m_iV; 
    }

private:
    int m_iU;
    int m_iV;
    float   W[4];
    float m_fU;
    float m_fV;
};
