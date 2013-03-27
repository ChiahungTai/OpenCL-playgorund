/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFlux2DBC_Gen.cpp
 *
 * @brief Contains the waves generation boundary conditions implementation.   
 */

#include <math.h>
#include "Flux/CFlux2DGrid.h"
#include "Flux/actions/CFlux2DBC_Gen.h"
#include "physics_3D_fluid_sim_common.h"

/**
 * @fn CFlux2DBCSineWaveGenVerticalLeft(CFile &file)
 *
 * @brief Class constructor, this instances initializes the generator action from memory
 *
 * @param file - A reference to a CFile interface from which to read the data for initialization
 */
CFlux2DBCSineWaveGenVerticalLeft::CFlux2DBCSineWaveGenVerticalLeft(CFile &file)
{
    file.Restore(m_ampl);
    file.Restore(m_omega);
    file.Restore(m_lengthAlongV);
    m_pNoise = NULL;
    m_NoiseSize = 0;
}

/**
 * @fn void SaveData(CFile &file)
 *
 * @brief Used to pack the action data into a file
 *
 * @param file - A reference to the file interface used for packing
 */
void CFlux2DBCSineWaveGenVerticalLeft::SaveData(CFile &file) const
{
    file.Save(m_ampl);
    file.Save(m_omega);
    file.Save(m_lengthAlongV);
}

extern float g_Noise;
extern float g_NoiseSpaceCoherence;
extern float g_NoiseTimeCoherence;
extern float g_omega;
extern float g_ampl;

//#define DEBUG_PRINT

/**
 * @fn Run(pG, &pars, &globals)
 *
 * @brief Runs the generation process
 *
 * @note This function generates a water height profile on the boundary according to a
 * sinus function with a stochastic phase. This allows the generation of irregular waves
 * on the boundary. The function Calculates the u and v velocity components according to
 * the new water level on the boundary.
 *
 * @param pG - The grid to be modified
 *
 * @param &pars - The current simulation step parameters
 *
 * @param &globals ) The global simulation parameters
 */
void CFlux2DBCSineWaveGenVerticalLeft::Run( CFlux2DGrid* pG, 
    CFlux2DCalculatorPars const &pars, 
    CFlux2DGlobalParameters const &globals )
{
    int const dimH = pG->GetDomainH();

    m_omega = g_omega;
    m_ampl = g_ampl;

    float   yPhaseCoeff = m_lengthAlongV > 0.0f ? (2.0f * M_PI * pG->GetGridStepW() / m_lengthAlongV) : 0.0f;
    float   NoiseScaleTime = 1.0f;

    if(g_Noise>0)
    {
        // Generate low noise for stochastic waves
        float*  pNoiseVector = NULL;
        float   AlphaTime = 1.0f;
        int     AverSize = (int)(0.5f+dimH*g_NoiseSpaceCoherence);
        // Ensure that the average size is at least one
        if(AverSize<1)
        {
            AverSize=1;
        }

#ifdef DEBUG_PRINT
        printf("g_NoiseTimeCoherence = %f\n",g_NoiseTimeCoherence);
#endif
        
        if(g_NoiseTimeCoherence>0)
        {
            // If there is a coherence between the time samples. we should calculate
            // a smaller blending coefficient
            float   A = 2/(g_NoiseTimeCoherence*100); 
            AlphaTime = (1-exp(-A));
            NoiseScaleTime = sqrt(1-exp(-2*A))/AlphaTime;

#ifdef DEBUG_PRINT
            printf("noise Scale Time %f\n",NoiseScaleTime);
#endif

        }

        // If we have noise and the size of the noise is less than the domain height
        if(m_pNoise && m_NoiseSize<dimH)
        {
            // Free the noise
            free(m_pNoise);
            m_pNoise = NULL;
        }

        // If we do not have noise
        if(m_pNoise==NULL)
        {
            // Set the noise size to the domain height
            m_NoiseSize = dimH;

            // Allocate memory for the noise
            m_pNoise = (float*)malloc(sizeof(float)*m_NoiseSize*12);

            // If we allocated the memory successfully
            if(m_pNoise)
            {
                // for every line, zero out the noise
                for (int y = 0; y < dimH; y++)
                {
                    m_pNoise[y] = 0.0f;
                    m_pNoise[dimH+y] = m_pNoise[y];
                }
            }
            m_LastTime = pars.time - (m_LastTime+g_NoiseTimeCoherence);
        }

        // Now, if we have noise
        if(m_pNoise)
        {
            // Compute the noise vector pointer
            pNoiseVector = m_pNoise+2*dimH;
        }
        else
        {
            // Otherwise, just return
            return;
        }

        // If the average size is greater than ten times the domain height
        if(AverSize>(dimH*10))
        {
            // Reduce the average size to that value
            AverSize=dimH*10;
        }

#ifdef DEBUG_PRINT
        printf("pars.time = %f\n",pars.time);
        printf("m_LastTime = %f\n",m_LastTime);
        printf("g_NoiseTimeCoherence = %f\n",g_NoiseTimeCoherence);
#endif
        // If the time of the current simulation step is greater than the sum of
        // the last time plus the noise time coherence..
        if(pars.time>=(m_LastTime+g_NoiseTimeCoherence))
        {
            static int initialized = 0;
            printf("Change phase noise\n");

            // Update the last time variable
            m_LastTime += g_NoiseTimeCoherence;

            // Copy the phase profile and generate a new one
            for (int y = 0; y < AverSize; y++)
            {
                // Generate the initial noise vector
                pNoiseVector[y] = ((float)rand()/(float)RAND_MAX)-0.5f;
            }

            // Average by space to get low frequency by space
            for(int y = 0; y < dimH; y++)
            {
                int     yy;
                float   sum = 0;
                float   count = 0;
                m_pNoise[y] = m_pNoise[dimH+y];

                // Average the generated vector
                for(yy=0;yy<AverSize;++yy)
                { 
                    sum += pNoiseVector[yy];
                    count++;
                }

                // Then generate a new noise value
                pNoiseVector[y%AverSize] = ((float)rand()/(float)RAND_MAX)-0.5f;

                /* *************************** DEPRECATED CODE ***************************** */
                //sum = (y-dimH*0.5f)/dimH;
                //count = 1;
                /* ************************* END DEPRECATED CODE ********************** */

                if(count>0) 
                {   // Save the new phase to the next time array
                    m_pNoise[dimH+y] = (sum/count)*sqrt(count);
                    if(!initialized) m_pNoise[y] = m_pNoise[dimH+y];
                }
            }
            initialized = 1;
        }
        
    } // End of - if(g_Noise>0) then generate low noise for stochastic waves

    // For every line
    for (int y = 0; y < dimH; y++)
    {
        // Get the current and destination surfaces
        CFluxVecPtr inPtr(pG->GetCurrentSurface(), -1,y);
        CFluxVecPtr outPtr(pG->GetDestSurface(), -1,y);

        float* pB  = pG->GetBottom().m_heightMap.GetLinePtr(y)-1;

        // Initialize working variables
        float Hright = inPtr.pH[1];
        float Uright = inPtr.pU[1];
        float speedRight = sqrt(globals.gravity*Hright); 
        float   t = g_NoiseTimeCoherence>0?((pars.time-m_LastTime)/g_NoiseTimeCoherence):1;
        float Phase = g_Noise>0?(m_omega*g_Noise*(m_pNoise[y]*(1-t)+m_pNoise[dimH+y]*t)*2*M_PI):0.0f;
        
#ifdef DEBUG_PRINT
        printf("t=%f\n",t);
#endif        

        float sinVal = sin(m_omega*pars.time + yPhaseCoeff * y + Phase);


        outPtr.pH[0] = pB[0] + m_ampl*sinVal;
        if(outPtr.pH[0] > 0)
        {
            float speed  = sqrt(globals.gravity*outPtr.pH[0]);
            float Uinter = 2.0f * (speed - speedRight) + Uright;

            outPtr.pU[0] = inPtr.pU[0] + 
                pars.tau * (Uinter - inPtr.pU[0])*(speedRight - Uright + speed - Uinter) / (pG->GetRcpGridStepW() * 2.0f);
            if(fabs(outPtr.pU[0]) <= pG->GetGlobalPars().minNonZeroSpeed)
            {
                outPtr.pU[0] = 0.0f;
            }

            outPtr.pV[0] = 0.0f;
        }
        else
        {
            outPtr.pH[0] = 0.0f;
            outPtr.pU[0] = 0.0f;
            outPtr.pV[0] = 0.0f;
        }
    }
}

/**
 * @fn CFlux2DBCWaterRiseVerticalLeft(CFile &file)
 *
 * @brief Class constructor, initializes the raise generator from memory
 *
 * @param file - A reference to CFile interface to read data for initialization data
 */
CFlux2DBCWaterRiseVerticalLeft::CFlux2DBCWaterRiseVerticalLeft(CFile &file)
{
    file.Restore(m_startHeight);
    file.Restore(m_stopHeight);
    file.Restore(m_timeInterval);
}

/**
 * @fn void SaveData(CFile &file)
 *
 * @brief Used to pack the action data into CFileMemory
 *
 * @param file - The reference to the interface used for packing
 */
void CFlux2DBCWaterRiseVerticalLeft::SaveData(CFile &file) const
{
    file.Save(m_startHeight);
    file.Save(m_stopHeight);
    file.Save(m_timeInterval);
}

/**
 * @fn Run(pG, pars, globals)
 *
 * @brief This method runs the raise process
 *
 * @note This function rises water level on left boundary. It
 * calculates the u and v velocity components according to the 
 * new water level on the boundary. 
 *
 * @param pG - The grid to be modified
 *
 * @param pars - Address of the current simulation step parameters
 *
 * @param globals - Address of the global simulation parameters
 */
void CFlux2DBCWaterRiseVerticalLeft::Run( CFlux2DGrid* pG, 
    CFlux2DCalculatorPars const &pars, 
    CFlux2DGlobalParameters const &globals )
{
    // Get the domain height
    int const dimH = pG->GetDomainH();

    // Calculate the current height based on the time interval and the simulation time
    float height;
    if (pars.time < m_timeInterval)
    {
        height = m_startHeight + (m_stopHeight - m_startHeight) * pars.time / m_timeInterval;
    }
    else
    {
        height = m_stopHeight;
    }

    // For every line
    for (int y = 0; y < dimH; y++)
    {
        // Get the current and destination surfaces
        CFluxVecPtr inPtr(pG->GetCurrentSurface(), -1,y);
        CFluxVecPtr outPtr(pG->GetDestSurface(), -1,y);

        float const *pHb = pG->GetBottom().m_heightMap.GetLinePtr(y)-1;

        // Initialize to zero
        outPtr.pH[0] = 0.0f;
        outPtr.pU[0] = 0.0f;
        outPtr.pV[0] = 0.0f;

        if( height > -pHb[0] )
        {
            // Initialize working variables
            float Hright = inPtr.pH[1];
            float Uright = inPtr.pU[1];
            float speedRight = sqrt(globals.gravity*Hright); 

            // Compute the output height

            outPtr.pH[0] = pHb[0] + height;

            // Calculate the U value based on the speed
            if (fabs(Uright) < speedRight)
            {
                float speed  = sqrt(globals.gravity*outPtr.pH[0]);
                float Uinter = 2.0f * (speed - speedRight) + Uright;

                float rcp_dt = 0.5f * (speedRight - Uright + speed - Uinter) * pG->GetRcpGridStepW();

                outPtr.pU[0] = inPtr.pU[0] + pars.tau * (Uinter - inPtr.pU[0]) * rcp_dt;
                if (outPtr.pU[0] < Uright)
                {
                    outPtr.pU[0] = Uright;
                }
            }
            else
            {
                outPtr.pU[0] = Uright;
            }
        }
    }
}

/**
 * @def MAXIMUM_HEIGHT_ALLOWED
 *
 * @brief Defines the maximum hight of the water 
 */
#define MAXIMUM_HEIGHT_ALLOWED 100.0f

    /**
     * @fn Run(pG, pars, globals )
     *
     * @brief This method runs the raise process
     *
     * @note This function rises the water level on the right boundary. The function
     * calculates the u and v velocity components according to the new water level on the boundary. 
     *
     * @param pG - The grid to be modified
     *
     * @param pars - Address of the current simulation step parameters
     *
     * @param globals - Address of the global simulation parameters
     */
void CFlux2DBCWaterWallVerticalRight::Run( CFlux2DGrid* pG, 
    CFlux2DCalculatorPars const &pars, 
    CFlux2DGlobalParameters const &globals )
{

    // Get the current surface height and width
    int const dimW = pG->GetCurrentSurface().m_heightMap.Width();
    int const dimH = pG->GetCurrentSurface().m_heightMap.Height();

    // For every line
    for (int y = 0; y < dimH; y++)
    {
        // Initialize working variables
        float* pInH = pG->GetCurrentSurface().m_heightMap.GetLinePtr(y)+dimW;
        float* pInU = pG->GetCurrentSurface().m_uMap.GetLinePtr(y)+dimW;
        float* pOutH = pG->GetDestSurface().m_heightMap.GetLinePtr(y)+dimW;
        float* pOutU = pG->GetDestSurface().m_uMap.GetLinePtr(y)+dimW;
        float* pOutV = pG->GetDestSurface().m_vMap.GetLinePtr(y)+dimW;

        float Hleft = pInH[-1];
        assert(0.0f <= Hleft && Hleft < MAXIMUM_HEIGHT_ALLOWED);
        float Uleft = pInU[-1];
        float speedLeft = sqrt(globals.gravity*Hleft); 
        float speed     = sqrt(globals.gravity*pInH[0]);
        float rcp_dt    = (speedLeft + 0.75f * Uleft) * pG->GetRcpGridStepW();
        float speedNew  = speed + pars.tau * (speedLeft + 0.5f * Uleft - speed) * rcp_dt;
        // Compute the new height
        float Hnew      = speedNew * speedNew / globals.gravity;
        assert(Hnew < MAXIMUM_HEIGHT_ALLOWED);
        if (Hnew > 0.0f)
        {
            pOutH[0] = Hnew;
            pOutV[0] = pOutV[-1];
        }
        else
        {
            pOutH[0] = 0.0f;
            pOutV[0] = 0.0f;
        }
        pOutU[0] = 0.0f;
    }
}
// End of File