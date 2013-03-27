/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFlux2DSceneDef.h
 *
 * @brief Contains the declaration of routines for scene initialization and file IO. 
 */

#pragma once

#include "macros.h"
#include "Util/CSamplerBilinear.h"

#define SMALL_VALUE 0.001f

/**
 * @struct CFlux2DSceneDef
 *
 * @brief Defines a flux 2D scene definition
 */
struct CFlux2DSceneDef
{

    /**
     * @var lenWidth
     *
     * @brief Width of the grid
     */
    float lenWidth;

    /**
     * @var lenHeight
     *
     * @brief Height of the grid
     */
    float lenHeight;

    /**
     * @var resWidth
     *
     * @brief Width of the scene source in the .bmp file
     */
    int resWidth;

    /**
     * @var resHeight
     *
     * @brief Height of the scene source in the .bmp file
     */
    int resHeight;

    /**
     * @var heightMap
     *
     * @brief A dynamic matrix height map
     */
    DynamicMatrix<float> heightMap;

    /**
     * @fn GetSample(float u, float v)
     *
     * @brief Used to find the height at (u,v)
     *
     * @param u - The horizontal component
     *
     * @param v - The vertical component
     *
     * @return - The value of the height at (u,v)
     */
    float GetSample(float u, float v)
    {
        // Clamp the address to be in the range from (0,0) to the resource (width - 1, height - 1)
        u = __max(0.0f, __min(float(resWidth-1)-SMALL_VALUE, u));
        v = __max(0.0f, __min(float(resHeight-1)-SMALL_VALUE, v));
        return heightMap.GetVal(u,v);
    }
};


