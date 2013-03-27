/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

#pragma once

/**
 * @file CRegion.h
 *
 * @brief Defines a region in terms of opposite corners (X1,Y1)-(X2,Y2)
 */

/**
 * @struct CRegion
 *
 * @brief Defines the opposite corners of a region
 */
struct CRegion
{
    /**
     * @fn CRegion()
     *
     * @brief Class constructor
     */
    CRegion() {}

    /**
     * @fn CRegion(float x1_, float y1_, float x2_, float y2_)
     *
     * @brief Parameterized class constructor
     *
     * @param x1_ - Left coordinate of the region
     *
     * @param y1_ - Bottom coordinate of the region
     *
     * @param x2_ - Right coordinate of the region
     *
     * @param y2_ - Top coordinate of the region
     */
    CRegion(float x1_, float y1_, float x2_, float y2_) : x1(x1_), y1(y1_), x2(x2_), y2(y2_) {}

    /**
     * @var x1
     *
     * @brief - Left coordinate of the region
     */
    float x1;

    /**
     * @var y1
     *
     * @brief - Bottom coordinate of the region
     */
    float y1;

    /**
     * @var x2
     *
     * @brief - Right coordinate of the region
     */
    float x2;

    /**
     * @var y2
     *
     * @brief - Top coordinate of the region
     */
    float y2;
};

