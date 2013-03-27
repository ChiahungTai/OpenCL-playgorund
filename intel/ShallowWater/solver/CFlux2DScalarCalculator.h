/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFlux2DScalarCalculator.h
 *
 * @brief This file implements a non-optimized calculator for shallow water simulation based on a flux splitting approach
 */

#pragma once

#include <math.h>
#include "Flux/CFlux2DScene.h"

/**
 * @def wsMAX_FLOW_VAL
 *
 * @brief The maximum allowed change for water bar height
 */
#define wsMAX_FLOW_VAL 2000.0f


/**
 * @def wsMAX_SPEED_VAL
 *
 * @brief The maximum allowed change for water bar speed
 */
#define wsMAX_SPEED_VAL 500.0f

/**
 * @class CFlux2DScalarCalculator
 *
 * @brief This class intantiates the non-optimized calculator for shallow water simulation based on a flux splitting approach
 */
__declspec(align(64)) class CFlux2DScalarCalculator
{
public:

    // Constants
    float        Hmin;                          // The minimal allowed water bar height
    float        gravity;                       // A local copy of gravity value
    float        halfGravity;                   // The gravity multiplied by 0.5
    float        tau_div_deltaw;                // The time interval divided by grid cell width
    float        tau_div_deltah;                // The time interval divided by grid cell height
    float        tau_div_deltaw_quarter;        // A quarter of the time interval divided by the grid cell width
    float        tau_div_deltah_quarter;        // A quarter of the time interval divided by the grid cell height
    float        tau_mul_Cf;                    // The time interval multiplied by friction coefficient
    float        minNonZeroSpeed;               // The minimal value of water bar speed

    CFlux2DCalculatorPars    m_CalcParams;      // A local copy of calculation parameters for curretn time step

    // Pointers to processed lines of data 
    float*  pHbUp;                              // A pointer to upper line of bottom height
    float*  pHb;                                // A pointer to current line of bottom height
    float*  pHbDn;                              // A pointer to lower line of bottom height
    float*  pHbSqrtUp;                          // A pointer to upper line of bottom height square root
    float*  pHbSqrt;                            // A pointer to current line of bottom height square root
    float*  pHbSqrtDn;                          // A pointer to lower line of bottom height square root
    float*  pSpeedUp;                           // A pointer to uppper line of calculated wave speed
    float*  pSpeed;                             // A pointer to current line of calculated wave speed
    float*  pSpeedDn;                           // A pointer to lower line of calculated wave speed
    CFluxVecPtr inPtrUp;                        // Pointers to the uppper line of H,U,V
    CFluxVecPtr inPtr;                          // Pointers to the current line of H,U,V
    CFluxVecPtr inPtrDn;                        // Pointers to the lower line of H,U,V

    // Output pointers
    CFluxVecPtr outPtr;                         // Pointers to the current line of H,U,V for storing results

    // Flow parts
    CFluxVec<float> flow;                       // The flow for H,U,V

    // Current value
    CFluxVec<float> cur;                        // The current values for H,U,V
    float        curHb;                         // The current value of the bottom height
    float        curHbSqrt;                     // The current value of the bottom height's square root
    float        curSpeed;                      // The current speed of the wave

    DynamicMatrix<float> m_SpeedCacheMatrix;    // A cyclic buffer to keep the wave speed for the 3 last lines


public:
    /**
     * @fn InitForScene()
     *
     * @brief This method is used to initialize the calculator to calculate a given scene
     *
     * @param pScn - A pointer to the scene for which the calculator will be initialized
     */    
    void InitForScene(CFlux2DScene* pScn)
    {
        int maxSize = 0;
        int i,num = pScn->GetNumGrids();
        for(i=0;i<num;++i)
        {/** calc maximal width of grid */
            maxSize = max(maxSize,pScn->GetGrid(i)->GetDomainW());
        }
        /** set size for cycle buffer */
        m_SpeedCacheMatrix.SetSize(maxSize,3,1);
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
    void Init(CFlux2DCalculatorPars const &pars, CFlux2DGlobalParameters const &globals)
    {
        Hmin            = globals.Hmin;
        gravity         = globals.gravity;
        halfGravity     = 0.5f * globals.gravity;
        Hmin            = globals.Hmin;
        minNonZeroSpeed = globals.minNonZeroSpeed;
        tau_mul_Cf      = pars.tau * globals.Cf;
        m_CalcParams = pars;
    }
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
    void ProcessTile(CFlux2DGrid* pG, int x0, int y0, int dimW, int dimH)
    {
        int       i;
        float*    pH = NULL;

        tau_div_deltaw  = m_CalcParams.tau * pG->GetRcpGridStepW();
        tau_div_deltah  = m_CalcParams.tau * pG->GetRcpGridStepH();
        tau_div_deltaw_quarter  = tau_div_deltaw * 0.25f;
        tau_div_deltah_quarter  = tau_div_deltah * 0.25f;


        pSpeedUp = m_SpeedCacheMatrix.GetLinePtr(0);
        pSpeed = m_SpeedCacheMatrix.GetLinePtr(1);
        pSpeedDn = m_SpeedCacheMatrix.GetLinePtr(2);

        // Calculate -1 line of speed that will be used in future
        pH = pG->GetCurrentSurface().m_heightMap.GetLinePtr(y0-1)+x0;
        for(i=-1;i<dimW+1;++i)
        {
            pSpeedUp[i] = sqrtf(gravity*pH[i]);
        }

        // Calculate 0 line of speed that will be used in future
        pH = pG->GetCurrentSurface().m_heightMap.GetLinePtr(y0)+x0;
        for(i=-1;i<dimW+1;++i)
        {
            pSpeed[i] = sqrtf(gravity*pH[i]);
        }

        // Start loop, for every line in the tile
        for (int y = 0; y < dimH; y++)
        {
            InitPointersForLine(pG,x0,y0,y);
            
            // Calculate +1 line of speed that will be used in future
            for(i=-1;i<dimW+1;++i)
            {
                pSpeedDn[i] = sqrtf(gravity*inPtrDn.pH[i]);
            }

            // For every pixel in the line
            for (int x = 0; x < dimW; x++)
            {
                // calculate each pixel
                ProcessNode();
                // And then advance the pointers
                Advance();
            }// End of next pixel in the line
        }// End of next line of the tile
    }// End of ProcessTile

    /**
     * @fn CorrectSpeed()
     *
     * @brief Used to correct the speed in order to avoid division by close to a zero value
     *
     * @param  speed - The corrected speed
     *
     * @return The absolute value of the corrected speed value
     *         Zero if the corrected speed is below the minimum
     */
    inline float correctSpeed(float speed)
    {
        return (fabs(speed) >= minNonZeroSpeed) ? speed : 0.0f;
    }

    /**
     * @enum DRYSTATES 
     *
     * @brief Used to specify node dry states
     */
    enum
    {
        LT_MASK = (1 << 0),     // The left node is dry
        RT_MASK = (1 << 1),     // The right node is dry
        UP_MASK = (1 << 2),     // The top node is dry
        DN_MASK = (1 << 3),     // The bottom node is dry
        FULL_MASK = LT_MASK | RT_MASK | UP_MASK | DN_MASK
    };
    /**
     * @fn Advance()
     *
     * @brief Used to shift all current pointers by given elements
     *
     * @param step - The number elements to shift
     */
    void Advance(int step=1)
    {
        // Advance read pointers
        inPtr.Advance(step);
        inPtrUp.Advance(step);
        inPtrDn.Advance(step);
        outPtr.Advance(step);
        pHb+=step;
        pHbDn+=step;
        pHbUp+=step;
        pHbSqrt+=step;
        pHbSqrtUp+=step;
        pHbSqrtDn+=step;
        pSpeed+=step;
        pSpeedUp+=step;
        pSpeedDn+=step;
    }

    /**
     * @fn CalcU_Neg_MoreCritical()
     *
     * @brief This method calculates the flow component related with a NEGATIVE water bar speed
     *        U component that has an absolute value that is MORE than the wave speed in the node
     */
    void CalcU_Neg_MoreCritical();

    /**
     * @fn CalcU_Pos_MoreCritical()
     *
     * @brief This method calculates the flow component related with a POSITIVE water bar speed U component 
     *        that has an absolute value MORE than the wave speed in the node
     */
    void CalcU_Pos_MoreCritical();

    /**
     * @fn CalcU_Neg_LessCritical()
     *
     * @brief This method calculates the flow component related with a NEGATIVE water bar speed U component 
     *        that has an absolute value LESS than the wave speed in the node
     */
    void CalcU_Neg_LessCritical();

    /**
     * @fn CalcU_Pos_LessCritical()
     *
     * @brief This method calculates the flow component related with a POSITIVE water bar speed U component 
     *        that has absolute value LESS than the wave speed in the node
     */
    void CalcU_Pos_LessCritical();

    /**
     * @fn CalcV_Neg_MoreCritical()
     *
     * @brief This method calculates the flow component related with a NEGATIVE water bar speed V component 
     *        that has an absolute value MORE than the wave speed in the node
     */
    void CalcV_Neg_MoreCritical();

    /**
     * @fn CalcV_Pos_MoreCritical()
     *
     * @brief  This method calculates the flow component related with a POSITIVE water bar speed V component 
     *         that has an absolute value MORE than the wave speed in the node
     */
    void CalcV_Pos_MoreCritical();

    /**
     * @fn CalcV_Neg_LessCritical()
     *
     * @brief  This method calculates the flow component related with a NEGATIVE water bar speed V component 
     *         that has an absolute value LESS than the wave speed in the node
     */
    void CalcV_Neg_LessCritical();

    /**
     * @fn CalcV_Pos_LessCritical()
     *
     * @brief  This method calculates the flow component related with a POSITIVE water bar speed V component 
     * that has an absolute value LESS than the wave speed in the node
     */
    void CalcV_Pos_LessCritical();

    /**
     * @fn ProcessNode()
     *
     * @brief Processes the current node. Calculates the flow for the node and changes the current speed and height of the water bar
     */
    inline void ProcessNode()
    {
        // calculate each pixel
        int state=0;

        // Load current values
        cur.H = *inPtr.pH;
        cur.u     = *inPtr.pU;
        cur.v     = *inPtr.pV;
        curHb     = *pHb;
        curHbSqrt = *pHbSqrt;

        // Calculate critical speed
        curSpeed = *pSpeed;

        // Init flows
        flow.H  = 0.0f;
        flow.u  = 0.0f;
        flow.v  = 0.0f;
        
        // Compute the state
        if (cur.H == 0.0f)
        {
            if (inPtr.pH[-1] == 0.0f)
            {
                state |= LT_MASK;
            }
            if (inPtr.pH[1] == 0.0f)
            {
                state |= RT_MASK;
            }
            if (inPtrUp.pH[0] == 0.0f)
            {
                state |= UP_MASK;
            }
            if (inPtrDn.pH[0] == 0.0f)
            {
                state |= DN_MASK;
            }
            if (state == FULL_MASK)
            {
                *outPtr.pH = 0.0f;
                *outPtr.pU = 0.0f; 
                *outPtr.pV = 0.0f; 
                return;
            }
        }


        if (state == 0)
        {
            // Calculate the U component of flows
            if (inPtr.pU[0] >= 0.0f)
            {
                if (inPtr.pU[0] > curSpeed)
                {
                    CalcU_Pos_MoreCritical();
                }
                else
                {
                    CalcU_Pos_LessCritical();
                }
            }
            else
            {
                if (inPtr.pU[0] < -curSpeed)
                {
                    CalcU_Neg_MoreCritical();
                }
                else
                {
                    CalcU_Neg_LessCritical();
                }
            }

            // Calculate the V component of flows
            if (inPtr.pV[0] >= 0.0f)
            {
                if (inPtr.pV[0] > curSpeed)
                {
                    CalcV_Pos_MoreCritical();
                }
                else
                {
                    CalcV_Pos_LessCritical();
                }
            }
            else
            {
                if (inPtr.pV[0] < -curSpeed)
                {
                    CalcV_Neg_MoreCritical();
                }
                else
                {
                    CalcV_Neg_LessCritical();
                }
            }
        }
        else // State is NOT zero
        {
            // First, check right and left
            switch (state & (LT_MASK | RT_MASK))
            {
                case 0: // Both neighbour node are active. 
                {
                    CalcU_Pos_LessCritical(); 
                    break;  
                }
                case LT_MASK: // The left node is dry. Flow goes from the right only.
                {
                    CalcU_Neg_MoreCritical(); 
                    break; 
                }
                case RT_MASK:// The right node is dry. Flow goes from left only. 
                {
                    CalcU_Pos_MoreCritical(); 
                    break; 
                }
            }

            // Then check up and down
            switch (state & (UP_MASK | DN_MASK))
            {
                case 0: // Both neighbour node are active.
                {
                    CalcV_Pos_LessCritical(); 
                    break;       
                }
                case UP_MASK: // The up node is dry. Flow goes from down only.
                {
                    CalcV_Neg_MoreCritical(); 
                    break; 
                }
                case DN_MASK: // The right node is dry. Flow goes from left only. 
                {
                     CalcV_Pos_MoreCritical(); 
                     break; 
                }
            }
        }

        // NOTE: flow values are already multiplied by corresponding values

        // Calculate new H value
        float Hnew = inPtr.pH[0] - flow.H;
        if (Hnew >= 0.0f)
        {
            *outPtr.pH = Hnew;
            // Calculate new (u,v) values
            float sq = tau_mul_Cf * sqrtf(inPtr.pU[0] * inPtr.pU[0] + inPtr.pV[0] * inPtr.pV[0]);
            float rcpHnew = 1.0f / __max(Hmin, Hnew);
            *outPtr.pU = correctSpeed( rcpHnew * ((cur.H - sq) * cur.u - flow.u) );
            assert(-wsMAX_SPEED_VAL < *outPtr.pU && *outPtr.pU < wsMAX_SPEED_VAL);
            *outPtr.pV = correctSpeed( rcpHnew * ((cur.H - sq) * cur.v - flow.v) );
            assert(-wsMAX_SPEED_VAL < *outPtr.pV && *outPtr.pV < wsMAX_SPEED_VAL);
        }
        else
        {
            *outPtr.pH = 0.0f;
            *outPtr.pU = 0.0f; 
            *outPtr.pV = 0.0f; 
        }
    }// End of ProcessNode

    /**
     * @fn InitPointersForLine()
     *
     * @brief Used to initialize all pointers for a given line
     *
     * @param pG - The grid that will be processed
     *
     * @param TileX0 - The X coordinate of the tile to be processed 
     *
     * @param TileY0 - The Y coordinate of the tile to be processed
     *
     * @param y - The Y coordinate of the line in the tile to be processed
     */
    inline void InitPointersForLine(CFlux2DGrid* pG, int TileX0, int TileY0, int y)
    {
        // Initialize pointers to the first elements in the line */
        inPtr.Init(pG->GetCurrentSurface(), TileX0,y+TileY0);
        inPtrUp.Init(pG->GetCurrentSurface(), TileX0,y+TileY0-1);
        inPtrDn.Init(pG->GetCurrentSurface(), TileX0,y+TileY0+1);
        outPtr.Init(pG->GetDestSurface(), TileX0,y+TileY0);
        pHbUp   = pG->m_bottom.m_heightMap.GetLinePtr(y+TileY0-1)+TileX0;
        pHb     = pG->m_bottom.m_heightMap.GetLinePtr(y+TileY0)+TileX0;
        pHbDn   = pG->m_bottom.m_heightMap.GetLinePtr(y+TileY0+1)+TileX0;
        pHbSqrtUp = pG->m_bottom.m_precompDataMap.GetLinePtr(y+TileY0-1)+TileX0;
        pHbSqrt = pG->m_bottom.m_precompDataMap.GetLinePtr(y+TileY0)+TileX0;
        pHbSqrtDn = pG->m_bottom.m_precompDataMap.GetLinePtr(y+TileY0+1)+TileX0;
        pSpeedUp = m_SpeedCacheMatrix.GetLinePtr(y%3);
        pSpeed = m_SpeedCacheMatrix.GetLinePtr((y+1)%3);
        pSpeedDn = m_SpeedCacheMatrix.GetLinePtr((y+2)%3);
    }
}; // End of InitPointersForLine()
// End of File

