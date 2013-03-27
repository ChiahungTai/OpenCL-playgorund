/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file Cflux2Do2ScalarCalculator.h
 *
 * @brief Defines a class to implement a scalar solution of simple 2D wave equations
 */

#pragma once

#include "Flux/CFlux2DScene.h"

/**
 * @class 
 *
 * @brief Implements a scalar version of a water simulation method based on a simple wave equation approach.
 *        This class implement a solution of simple 2D wave equations
 */
class CFlux2Do2ScalarCalculator
{
public:
    /**
     * @fn InitForScene()
     *
     * @brief This method is used to initialize the calculator to calculate a given scene
     *
     * @param pScn - A pointer to the scene for which the calculator will be initialized
     *
     * @note This initialization is not needed for this calculator, it is here for compatibility 
    */    
    void InitForScene(CFlux2DScene* pScn)
    {
    };

    /**
     * @fn Init()
     *
     * @brief This method initializes the calculator to calculate a simulation step
     *
     * @param pars - Local parameters for the current simulation step 
     *
     * @param globals - Global parameters for all simulation steps 
    */
    void Init(CFlux2DCalculatorPars const &pars, CFlux2DGlobalParameters const &globals);

    /**
     * @fn ProcessTile()
     *
     * @brief This method is used to process a tile of a grid
     *
     * @param pG - The grid tile to be processed
     *
     * @param x0 - The X coordinate of the left-top corner of the tile that will be processed
     *
     * @param y0 - The Y coordinate of the left-top corner of the tile that will be processed
     *
     * @param dimW - The X size of the tile that will be processed
     *
     * @param dimH - The Y size of the tile that will be processed
     */
    void ProcessTile(CFlux2DGrid* pG, int x0, int y0, int dimW, int dimH);

protected:

    /**
     * @var m_factor
     *
     * @brief Equal to (a * dt / h)^2 - The grid step divided by the square root of the wave speed
     */
    float   m_factor;   
    /**
     * @var m_2factor
     *
     * @brief  Equalt to 2 * m_factor
     */
    float   m_2factor; 
    /**
     * @var m_DissipationAlpha
     *
     * @brief A dissipation coefficient that simulates viscosity
     */
    float   m_DissipationAlpha; 
    /**
     * @var m_GlobalParams
     *
     * @brief Global parameters
     */
    CFlux2DGlobalParameters    m_GlobalParams;

    /**
     * @var m_CalcParams
     *
     * @brief Calculator specific parameters
     */
    CFlux2DCalculatorPars      m_CalcParams;
};
// End of file
