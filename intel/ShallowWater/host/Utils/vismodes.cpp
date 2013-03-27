/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file vismodes.cpp
 *
 * @brief Implements visual modes and methods
 */

#pragma once

#include <windows.h>
#include <math.h>

#include "Flux/CFlux2DGrid.h"
#include "vismodes.h"

namespace
{
    float const gravity = 10.0f;

    /**
     * @fn Speed2Val(float s, float c)
     *
     * @brief calculates color for velocity color map
     *
     * @param s - speed
     *
     * @param c - critical speed
     */
    int Speed2Val(float s, float c)
    {
        int col;
        if (s >= 0.0f)
        {
            col = (s > c) ? 3 : 2;
        }
        else
        {
            col = (s < -c) ? 0 : 1;
        }
        return col * 64;
    }
}

/**
 * @fn VisModeLevel::GetColor(class CFlux2DGrid &flux, int x, int y)
 *
 * @brief Gets the color of the grid in the cell (X,Y)
 *
 * @param flux - The flux grid
 *
 * @param x - The cell X coordinate in the grid
 *
 * @param y - The cell Y coordinate in the grid
 *
 * @return - An integer representation of the color
 */
int VisModeLevel::GetColor(CFlux2DGrid &flux, int x, int y) const
{

    // Color is determined by the water depth, the height between the bottom and the surface
    float height = flux.GetCurrentSurface().m_heightMap.At(x,y) - flux.GetBottom().m_heightMap.At(x,y);
    // The highest value possible for color computation is 0xFF, the lowest is zero
    int val = max(0, min(0xFF, int(height * m_a + m_b)));
    return RGB(val, val, val);
}

/**
 * @fn VisModeLevelColored::GetColor(class CFlux2DGrid &flux, int x, int y)
 *
 * @brief Gets the color of the grid in the cell (X,Y)
 *
 * @param flux - The flux grid
 *
 * @param x - The cell X coordinate in the grid
 *
 * @param y - The cell Y coordinate in the grid
 *
 * @return - An integer representation of the color
 */
int VisModeLevelColored::GetColor(CFlux2DGrid &flux, int x, int y) const
{

    // Color is determined by the water depth, the height between the bottom and the surface
    float height = flux.GetCurrentSurface().m_heightMap.At(x,y) - flux.GetBottom().m_heightMap.At(x,y);

    float val = height * m_a + m_b;

    // In this case, we use the RGB components of the base color
    return RGB(max(0, min(0xFF, int(val * m_color.b))), max(0, min(0xFF, int(val * m_color.g))), max(0, min(0xFF, int(val * m_color.r))));
}

/**
 * @fn VisModeHeightColored::GetColor(class CFlux2DGrid &flux, int x, int y)
 *
 * @brief Gets the color of the grid in the cell (X,Y)
 *
 * @param flux - The flux grid
 *
 * @param x - The cell X coordinate in the grid
 *
 * @param y - The cell Y coordinate in the grid
 *
 * @return - An integer representation of the color
 */
int VisModeHeightColored::GetColor(CFlux2DGrid &flux, int x, int y) const
{
    // Get the height
    float h = flux.GetCurrentSurface().m_heightMap.At(x,y);
    if (h > 0.0f)
    {
        // The value used for color computation can be at most 1.0
        float val = min(1.0f, h * m_rcpMaxHeight);
        // The low color is used as a base. The computed value used as a factor applied
        // to the difference between the high and low colors and added to the base low color
        return RGB(int( (m_colorLo.b + (m_colorHi.b - m_colorLo.b) * val) * 255.0f ), 
            int( (m_colorLo.g + (m_colorHi.g - m_colorLo.g) * val) * 255.0f ), 
            int( (m_colorLo.r + (m_colorHi.r - m_colorLo.r) * val) * 255.0f ));
    }
    else
    {
        // If the height is zero or less, use the ground color
        return RGB(int(m_colorGround.b * 255.0f), int(m_colorGround.g * 255.0f), int(m_colorGround.r * 255.0f));
    }
}

/**
 * @fn VisModeSpeed::GetColor(class CFlux2DGrid &flux, int x, int y)
 *
 * @brief Gets the color of the grid in the cell (X,Y)
 *
 * @param flux - The flux grid
 *
 * @param x - The cell X coordinate in the grid
 *
 * @param y - The cell Y coordinate in the grid
 *
 * @return - An integer representation of the color
 */
int VisModeSpeed::GetColor(CFlux2DGrid &flux, int x, int y) const
{
    int rVal = 0;

    float u = flux.GetCurrentSurface().m_uMap.At(x,y);
    int gVal = max(0, min(0xFF, int(u * m_a + m_b)));

    float v = flux.GetCurrentSurface().m_vMap.At(x,y);
    int bVal = max(0, min(0xFF, int(v * m_a + m_b)));

    if (m_visCritical)
    {
        float height = flux.GetCurrentSurface().m_heightMap.At(x,y);
        float criticalSpeed = sqrtf(height * gravity);
        if (fabsf(u) > criticalSpeed || fabsf(v) > criticalSpeed)
        {
            rVal = 0x7F;
        }
    }

    return RGB(rVal, gVal, bVal);
}

/**
 * @fn VisModeMoment::GetColor(class CFlux2DGrid &flux, int x, int y)
 *
 * @brief Gets the color of the grid in the cell (X,Y)
 *
 * @param flux - The flux grid
 *
 * @param x - The cell X coordinate in the grid
 *
 * @param y - The cell Y coordinate in the grid
 *
 * @return - An integer representation of the color
 */
int VisModeMoment::GetColor(CFlux2DGrid &flux, int x, int y) const
{
    // Calculate color for height map color visualization
    // Color represents the impulse value for the water bar 
    float height = flux.GetCurrentSurface().m_heightMap.At(x,y);
    float uM = height * flux.GetCurrentSurface().m_uMap.At(x,y);
    int gVal = max(0, min(0xFF, int(uM * m_a + m_b)));

    float vM = height * flux.GetCurrentSurface().m_vMap.At(x,y);
    int bVal = max(0, min(0xFF, int(vM * m_a + m_b)));

    return RGB(bVal, gVal, 0);
}

/**
 * @fn VisModeCritical::GetColor(class CFlux2DGrid &flux, int x, int y)
 *
 * @brief Gets the color of the grid in the cell (X,Y)
 *
 * @param flux - The flux grid
 *
 * @param x - The cell X coordinate in the grid
 *
 * @param y - The cell Y coordinate in the grid
 *
 * @return - An integer representation of the color
 */
int VisModeCritical::GetColor(CFlux2DGrid &flux, int x, int y) const
{
    // Calculate color for height map color visualization
    // Color represents the speed value for the water bar
    // in comparion to the critical speed value
    float height = flux.GetCurrentSurface().m_heightMap.At(x,y);
    float criticalSpeed = sqrtf(height * gravity);
    float u = flux.GetCurrentSurface().m_uMap.At(x,y);
    float v = flux.GetCurrentSurface().m_vMap.At(x,y);

    int rVal = (fabsf(u) > criticalSpeed || fabsf(v) > criticalSpeed) ? 0x7F : 0;
    return RGB(Speed2Val(v, criticalSpeed), Speed2Val(u, criticalSpeed), rVal);
}

/**
 * @fn VisModeState::GetColor(class CFlux2DGrid &flux, int x, int y)
 *
 * @brief Gets the color of the grid in the cell (X,Y)
 *
 * @param flux - The flux grid
 *
 * @param x - The cell X coordinate in the grid
 *
 * @param y - The cell Y coordinate in the grid
 *
 * @return - An integer representation of the color
 */
int VisModeState::GetColor(CFlux2DGrid &flux, int x, int y) const
{
    // Decalare and initialize working variables
    float const Hmin = 0.0f;
    int   const normalStateColor = RGB(255, 0, 0);
    int   const dryStateColor = RGB(0, 0, 127);
    float height = flux.GetCurrentSurface().m_heightMap.At(x,y);
    
    // If the height is not zero, return the normal state color
    if( height != Hmin )
    {
        return normalStateColor;
    }

    // Get the width of the border
    int bs = flux.GetCurrentSurface().m_heightMap.Border();
    // If the X coordinate is greater than the left edge of the border
    // and if the left neighbor height is not zero, return the normal state color
    if (x > -bs && flux.GetCurrentSurface().m_heightMap.At(x-1,y) != Hmin)
    {
        return normalStateColor;
    }

    // If the X coordinate is to the left of the right border and the right neighbor
    // height is not zero, then return the normal state color
    if (x < (flux.GetCurrentSurface().m_heightMap.Width()+bs-1) && flux.GetCurrentSurface().m_heightMap.At(x+1,y) != Hmin)
    {
        return normalStateColor;
    }

    // If the Y coordinate is below the top border and the upper neighbor height is not zero, then use the normal state color
    if (y > -bs && flux.GetCurrentSurface().m_heightMap.At(x,y-1) != Hmin)
    {
        return normalStateColor;
    }

    // If the Y coordinate is above the bottom border and the lower neighbor height is not zero, then use the normal state color
    if (y < (flux.GetCurrentSurface().m_heightMap.Height()-1+bs) && flux.GetCurrentSurface().m_heightMap.At(x,y+1) != Hmin)
    {
        return normalStateColor;
    }

    // Otherwise, return the dry state color
    return dryStateColor;
}

/**
 * @fn VisModeState::GetColor(class CFlux2DGrid &flux, int x, int y)
 *
 * @brief Gets the color of the grid in the cell (X,Y)
 *
 * @param flux - The flux grid
 *
 * @param x - The cell X coordinate in the grid
 *
 * @param y - The cell Y coordinate in the grid
 *
 * @return - An integer representation of the color
 */
int VisModeFroude::GetColor(CFlux2DGrid &flux, int x, int y) const
{
    // Calculate color for height map color visualization
    // Color represents the speed value for the water bar
    // in comparion to the critical speed value
    float height = flux.GetCurrentSurface().m_heightMap.At(x,y);
    float criticalSpeed = sqrtf(height * gravity);
    float u = flux.GetCurrentSurface().m_uMap.At(x,y);
    float v = flux.GetCurrentSurface().m_vMap.At(x,y);

    int uVal = criticalSpeed > 0.0f ? int( (u / criticalSpeed) * 100.0f + 127.0f ) : 0;
    int vVal = criticalSpeed > 0.0f ? int( (v / criticalSpeed) * 100.0f + 127.0f ) : 0;
    int rVal = (fabsf(u) > criticalSpeed || fabsf(v) > criticalSpeed) ? 0x7F : 0;
    return RGB(uVal, vVal, rVal);
}


