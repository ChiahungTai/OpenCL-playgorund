/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFlux2DSSECalculator.cpp
 *
 * @brief Implements a Streaming SIMD Extension verion of the calculator
 */

#pragma once

#include "sseoperators.h"
#include "CFlux2DScalarCalculator.h"

/**
 * @class CFlux2DSSECalculator
 *
 * @brief Manages parameters and functions for the SSE calculator
 */
__declspec(align(64)) class CFlux2DSSECalculator : 
    public CFlux2DScalarCalculator
{
public:

    // Flow parts
    CFluxVec<__m128> vFlow;
    
    // Current value
    CFluxVec<__m128> vCur;                      // current velocity and height maps
    __m128            vCurHb;                   // current bottom height
    __m128            vCurSpeed;                // current speed
    __m128            vMinusCurSpeed;           // 1- current speed
    __m128            vCurHbSqrt;               // current bottom height square root

    __m128            vHmin;                    // minimal allowed water bar height
    __m128            vGravity;                 // local copy of gravity value
    __m128            vHalfGravity;             // gravity multiplied by 0.5
    __m128            vTau_div_deltaw;          // time interval divided by grid cell width
    __m128            vTau_div_deltah;          // time interval divided by grid cell height
    __m128            vTau_div_deltaw_quarter;  // quarter of time interval divided by grid cell width
    __m128            vTau_div_deltah_quarter;  // quarter of time interval divided by grid cell height
    __m128            vTau_mul_Cf;              // time interval multiplied by friction coefficient
    __m128            vMinNonZeroSpeed;         // minimal value of water bar speed   

    /**
     * #fn Init()
     *
     * @brief Initializes the calculator to make simulation steps
     *
     * @param pars - parameters for the current simulation step
     *
     * @param globals - parameters for all simulation steps 
     */
    void Init(CFlux2DCalculatorPars const &pars, CFlux2DGlobalParameters const &globals)
    {
        //printf("Init SSE\n");
        CFlux2DScalarCalculator::Init(pars, globals);
        vHmin             = _mm_set_ps1(Hmin);
        vGravity          = _mm_set_ps1(gravity);
        vHalfGravity      = _mm_set_ps1(halfGravity);
        vTau_mul_Cf       = _mm_set_ps1(tau_mul_Cf);
        vMinNonZeroSpeed  = _mm_set_ps1(minNonZeroSpeed);
    }

    /**
     * @fn CorrectSpeed()
     *
     * @brief Used to correct the speed in order to avoid division by close to a zero value
     *
     * @param vS - A register representing the speed
     */
    void CorrectSpeed(__m128 &vS)
    {
        __m128 vMask = _mm_cmpge_ps(abs(vS), vMinNonZeroSpeed);
        vS = _mm_blendv_ps(vS, sse_zero, vMask);
    }

    /**
     * @fn ProcessState()
     *
     * @brief Process the current node and neighbors
     *
     * @return TRUE if all the current vector has all elements greater than zero (all dry)
     *         FALSE if at least one element is wet
     */
    bool ProcessState();

    /**
     * @fn CheckNeighbourNodes()
     *
     * @brief Checks the neighborhood of a node for wet and dry nodes
     *
     * @param vHC - Height of the water
     *
     * @param - Write Mask
     *
     * @return FALSE if all neighbor nodes are dry, TRUE otherwise
     */
    bool CheckNeighbourNodes(__m128 vHC, __m128 vMask) const;

    /**
     * @fn BeginSSE()
     *
     * @brief Checks pointer for alignment, loads parameters and values, sets flows to zero
     */
    void BeginSSE();
 
    /**
     * @fn BeginSSE_Prefetch()
     *
     * @brief Checks pointer for alignment, loads parameters and values, sets flows to zero
     */
    void BeginSSE_Prefetch();

    /**
     * @fn EndSSE()
     *
     * @brief Completes SSE calculations
     */
    void EndSSE();

    /**
     * @fn ParseSpeedValue()
     *
     * @brief Ensures that the speed value is large enough to not cause division errors or overflow
     *
     * @param speed - The speed value to parse
     *
     * @return 
     */
    int ParseSpeedValue(__m128 speed);

    /**
     * @fn CalcU_Neg_MoreCritical_SSE()
     * @fn CalcU_Pos_MoreCritical_SSE()
     * @fn CalcU_Neg_LessCritical_SSE()
     * @fn CalcU_Pos_LessCritical_SSE()
     * @fn CalcV_Neg_MoreCritical_SSE()
     * @fn CalcV_Pos_MoreCritical_SSE()
     * @fn CalcV_Neg_LessCritical_SSE()
     * @fn CalcV_Pos_LessCritical_SSE()
     *
     * @brief These methods calculate the flow component related with the water bar speed U and V components
     *        that have an absolute value that is either MORE or less than the wave speed in the node.
     */

    void CalcU_Neg_MoreCritical_SSE();
    void CalcU_Pos_MoreCritical_SSE();
    void CalcU_Neg_LessCritical_SSE();
    void CalcU_Pos_LessCritical_SSE();

    void CalcV_Neg_MoreCritical_SSE();
    void CalcV_Pos_MoreCritical_SSE();
    void CalcV_Neg_LessCritical_SSE();
    void CalcV_Pos_LessCritical_SSE();

    /**
     * @fn Process4NodesSSE_WOBegin()
     *
     * @brief Processes four nodes using SSE if possible. If neighboring nodes
     *        are multi-directional, then we fall back to using the scalar method
     */
    void Process4NodesSSE_WOBegin();

    /**
     * @fn Process4NodesSSE_Inactive()
     *
     * @brief Processes an inactive node, writes zero vectors to the output
     */
    void Process4NodesSSE_Inactive();

    /**
     * @fn Process4NodesSSE_Scalar()
     *
     * @brief Processes four nodes using the scalar calculator
     */
    void Process4NodesScalar()
    {
        CFlux2DScalarCalculator::ProcessNode(); CFlux2DScalarCalculator::Advance();
        CFlux2DScalarCalculator::ProcessNode(); CFlux2DScalarCalculator::Advance();
        CFlux2DScalarCalculator::ProcessNode(); CFlux2DScalarCalculator::Advance();
        CFlux2DScalarCalculator::ProcessNode(); CFlux2DScalarCalculator::Advance();

    };

    /**
     * @fn ProcessTile()
     *
     * @briefd Processes a Tile of a grid
     *
     * @param pG - grid tile that will be processed
     *
     * @param x0 - The x coordinate of the left-top corner of the tile that will be processed
     *
     * @param y0 - The y coordinate of the left-top corner of the tile that will be processed
     *
     * @param dimW - The x size of the tile that will be processed
     *
     * @param dimH - The y size of the tile that will be processed
     */
    void ProcessTile(CFlux2DGrid* pG, int x0, int y0, int dimW, int dimH);
};
