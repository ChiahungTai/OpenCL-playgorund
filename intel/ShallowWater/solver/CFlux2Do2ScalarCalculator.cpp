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

#include "CFlux2Do2ScalarCalculator.h"
#include <math.h>    
#include "Flux/CFlux2DGrid.h"


/**
 * @def DISSIPATION_COEFFICIENT
 *
 * @brief Used to simulate viscosity
 */
#define DISSIPATION_COEFFICIENT 0.03f

/**
 * @fn Init()
 *
 * @brief  This method initializes the calculator to compute the simulation step
 *
 * @param pars - Parameters for the current simulation step 
 *
 * @param globals - Parameters for all simulation steps 
 */
void CFlux2Do2ScalarCalculator::Init(CFlux2DCalculatorPars const &pars, CFlux2DGlobalParameters const &globals)
{
    m_GlobalParams = globals;
    m_CalcParams = pars;
    
    m_DissipationAlpha = DISSIPATION_COEFFICIENT;
    m_DissipationAlpha = m_CalcParams.tau * m_DissipationAlpha/(1+m_CalcParams.tau * m_DissipationAlpha);
}

/**
 * @fn ProcessTile()
 *
 * @brief  Process a tile of the grid
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
void CFlux2Do2ScalarCalculator::ProcessTile(CFlux2DGrid* pG, int x0, int y0, int dimW, int dimH) 
{

    assert(fabs(pG->GetRcpGridStepH() - pG->GetRcpGridStepW()) < 0.01);

    // Calculate factors
    m_factor = m_GlobalParams.waveSpeed * pG->GetRcpGridStepH() * m_CalcParams.tau;
    m_factor *= m_factor;
    m_2factor = 2*m_factor;


    // For every line in the tile
    for (int y = 0; y < dimH; y++)
    {
        // Initialize working variables
        float*  pPrevH = pG->GetPastSurface().m_heightMap.GetLinePtr(y+y0)+x0;
        float*  pDstH = pG->GetDestSurface().m_heightMap.GetLinePtr(y+y0)+x0;
        float*  pCurH = pG->GetCurrentSurface().m_heightMap.GetLinePtr(y+y0)+x0;
        float*  pCurHUp = pG->GetCurrentSurface().m_heightMap.GetLinePtr(y+y0-1)+x0;
        float*  pCurHDn = pG->GetCurrentSurface().m_heightMap.GetLinePtr(y+y0+1)+x0;
        char*   pMask = pG->m_InnerMaskFlag?pG->m_InnerMaskData.GetLinePtr(y+y0)+x0:NULL;

        // For every vector in the line
        for(int x = 0; x < dimW; ++x)
        {
            if(pMask && pMask[x])
            {
                // Skip inner
                pDstH[x] = 0.0f; // pCurH[x];
            }
            else
            {
                // Process the pixel

                /* ********************************* Mathematical equivalent ****************************************** *\
                
                                                       Wave equation:
                
                                             d^2U /(dt)^2    - a^2 * Laplacian(U) = f(x,y,t) 
                
                                                   which is the same as

                            d^2U / (dt)^2 - a^2 ( d^2U    / (dx)^2 + d^2U    / (dy)^2) = f(x,y,t)
                
                   Let:                                                  
                   dx == dy    
                   f = (a * dt / dx) ^ 2
                   f(x,y,t) == 0
                
                   Then (the 2nd order approximation):
                   U[i,j,t+1] = 2 * ( 1 - 2f) * U [i, j, t] + f * ( U[i-1,j,t] + U[i+1,j,t] + U[i,j-1,t] + U[i,j+1,t]) - U[i,j,n-1]

                \* ****************************** End of Mathematical equivalent ************************************** */
    
                float NewH = 
                    2 * (1 - m_2factor) * pCurH[x] + 
                    m_factor * (pCurH[x+1] + pCurH[x-1] + pCurHUp[x] + pCurHDn[x]) - pPrevH[x];
                
                // Add some viscosity to avoid increasing energy
                NewH = NewH*(1-m_DissipationAlpha) + m_DissipationAlpha*pPrevH[x];
                NewH *= 0.999f;

                // Store the result in the destination
                pDstH[x] = NewH;
            }
        }
    }
}

// End of file
