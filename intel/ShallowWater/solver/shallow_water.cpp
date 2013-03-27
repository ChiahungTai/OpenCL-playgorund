/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */


/**
 * @file shallow_water.cpp
 *
 * @brief Defines the entry point for the OCL application.
 */

#include "Util/memory.h"
#include "HybridMethod/HybridMethod.h"
#include "common/physics_3D_fluid_sim_common.h"


/**
 * @name Global Varaibles @{
 */

/**
 * @var g_Noise
 *
 * @brief Power of noise that is used to generate stochastic wave on the left side of scene,
 * more power gives more stochastic waves 
 */
extern float   g_Noise;

/**
 * @var g_NoiseSpaceCoherence
 *
 * @brief The spacial distance of a random wave on the left side of the scene 
 */
extern float   g_NoiseSpaceCoherence;

/**
 * @var g_NoiseTimeCoherence
 *
 * @brief The time distance of a random wave on the left side of the scene
 */
extern float   g_NoiseTimeCoherence;

/** 
 * @var g_omega
 *
 * @brief The frequency of a wave generated on the left side of the scene
 */
extern float   g_omega;

/**
 * @var g_ampl
 *
 * #brief The amplitude for a wave generated on the left side of the scene
 */
extern float   g_ampl;

/**
 * @var g_pScene
 *
 * @brief A pointer to the shallow water scene (set of grids)
 */
CFlux2DScene*  g_pScene = NULL;

/**
 * @var g_pFluid
 *
 * @brief A pointer to complex hybrid method that contains the scene itself, the boat and vertices 
 */
HybridMethod*   g_pFluid = NULL;

/**
 * @var g_WaterSimulationParams
 *
 * @brief Stores parameters from the host to control the water simulation
 */
WATER_STRUCT    g_WaterSimulationParams;

/**
 * @}
 */

///extern "C" 
//{
    /**
     * @fn LoadScene()
     *
     * @brief Used to initialize the scene and implement a hybrid method by data received from HOST side
     *
     * @param Bufs.pBufArray[0] - Data to initialize the scene
     *
     * @param in_pData - Data to initialize the boat
     *
     * @param size - Size of the boat data
     */
    int LoadScene(void* in_pData0, uint32_t size0, void* in_pData, uint32_t size, float x_boat_pos, float y_boat_pos, bool device_type_gpu)
    {
        int     ThreadNum = -1;

        // Create a hybrid simulator
        g_pFluid = new (UseOwnMem) HybridMethod();
        {
            // Re-construct the hybrid simulator
            CFileMemory    SceneMemFile;

            // Get the packed scene 
            SceneMemFile.SetData((char*)in_pData0, size0);
            g_pFluid->Init(&SceneMemFile, ThreadNum, in_pData, x_boat_pos, y_boat_pos, device_type_gpu);
        }

        // Get a pointer to the scene
        g_pScene = g_pFluid->GetScene(); 


        return 1;
    }

    /**
     * @fn SimulationStep()
     *
     * @brief Makes one step of the simulation of shallow water and the complex hybrid method 
     *
     * @param Bufs - A list of buffers
     *
     * @param in_pData - data to control simulation. see WATER_STRUCT definition 
     *
     * @param size - size of in_pData data. This has to be equal ti sizeof(WATER_STRUCT) 
     */
    int SimulationStep(void* in_pData)
    {
        int             i;
        int             gridNum = g_pScene->GetNumGrids();;
        WATER_STRUCT*   pInfo = (WATER_STRUCT*)in_pData;

        /// Save the input parameters for the water simulation
        g_WaterSimulationParams = pInfo[0];

        // Get some values from the control structure
        g_Noise = pInfo->Noise;
        g_NoiseSpaceCoherence = pInfo->NoiseSpaceCoherence;
        g_NoiseTimeCoherence = pInfo->NoiseTimeCoherence;
        g_omega = pInfo->omega;
        g_ampl = pInfo->ampl;

        // For every grid, set the sizes of the grid
        for(i=0;i<gridNum;++i)
        {
            g_pScene->GetGrid(i)->SetDomainSize(pInfo->DomainW[i],pInfo->DomainH[i]);
        }


        // Do the simulation step
        g_pFluid->Update(pInfo);

        
        if(g_WaterSimulationParams.is_wind_waves)
        {
            // Make the stochastic middle scale wind wave simulation and add it to the main simulated water
            g_pScene->SimulateNoiseWindWaves(g_pFluid->GetTimeStep(),&pInfo->ww);
        }

        return 1;
    }/* Step */

    /**
     * @fn RebuildOCLKernel(bool in_bRelaxedMath)
     *
     * @brief Rebuilds OpenCL kernel for OpenCL calculator
     *
     * @param  in_bRelaxedMath - Relaxed math build option true or false
     */
    void RebuildOCLKernel(bool in_bRelaxedMath, bool in_bRunOnGPU)
    {
        g_pFluid->RebuildOCLKernel(in_bRelaxedMath, in_bRunOnGPU);
    }/* RebuildOCLKernel */

    /**
     * @fn GetGrid()
     *
     * @brief Get the grids data of shallow water maps
     *
     * @note This function will fill 3 buffers for H,U and V for the given grid.
     *
     * @param Bufs - Array of buffers as follows:
     *               Bufs.pBufArray[0] - buffer for H component of wave surface 
     *               Bufs.pBufArray[1] - buffer for U component of wave surface
     *               Bufs.pBufArray[2] - buffer for V component of wave surface
     *
     * @param in_pData - The pointer to an index of the grid
     *
     * @param size - The size of in_pData
     */
    int GetGrid(void* pHData, void* pUData, void* pVData, int i)
    {
        int                 gridIndex= i;//*(int*)in_pData;
        CFlux2DGrid*       pG = g_pScene->GetGrid(gridIndex);
        CFlux2DSurface*    pS = &pG->GetCurrentSurface();
        int                 dataSize = pS->m_heightMap.m_DataSize;

        // Get the internal format
        // Loat the format description depending on whether or not wind waves are enabled
        if(g_WaterSimulationParams.is_wind_waves)
        {
            memcpy(pHData, pG->m_WindWavesH.m_pPointer, dataSize);
        }
        else
        {
            memcpy(pHData, pS->m_heightMap.m_pPointer, dataSize);
        }


        memcpy(pUData, pS->m_uMap.m_pPointer, dataSize);

        memcpy(pVData, pS->m_vMap.m_pPointer, dataSize);

        return 1;
    }/* GetGrid */


    /**
     * @fn GetScene()
     *
     * @brief Get the scene data of shallow water maps
     *
     * @note This function fill 2 buffers with 2 channel for H+B and U+V for whole scene
     *
     * @param Bufs - Array of buffers such that
     *               Bufs.pBufArray[0] - buffer for H+B
     *               Bufs.pBufArray[1] - buffer for U+V component of wave surface
     *
     * @param in_pData is pointer to sizes (width and height) of output matrix
     *
     * @param size - The size of in_pData
     *
     * @return - 1 if succesful, -1 otherwise
     */
    int GetScene(void* pHBData, void* pUVData, void* in_pData)
    {
        // Declare working variables, initializing where appropriate
        int         k,kN;
        CRegion     R;
        float       dxMin,dyMin;
        int         dimW= ((int*)in_pData)[0];
        int         dimH= ((int*)in_pData)[1];


        // Validate the scene, create buffers as needed
        if(g_pScene==NULL)
        {
            memset(pHBData,0,sizeof(float)*dimW*dimH*2);
            memset(pUVData,0,sizeof(float)*dimW*dimH*2);
            return 1;
        }

        // Get the number of grids and their height and width
        kN =  g_pScene->GetNumGrids();
        dxMin = g_pScene->GetGrid(0)->GetGridStepW();
        dyMin = g_pScene->GetGrid(0)->GetGridStepH();

        // For each grid
        for(k=1;k<kN;k++)
        {
            // Find the minimum Dx and Dx grid step values
            CFlux2DGrid*       pG = g_pScene->GetGrid(k);
            dxMin = min(dxMin,pG->GetGridStepW());
            dyMin = min(dyMin,pG->GetGridStepH());
        }

        // Calculate the output grid region 
        R = g_pScene->GetRegion();
        R.x1 += dxMin*0.5f;
        R.y1 += dyMin*0.5f;
        R.x2 -= dxMin*0.5f;
        R.y2 -= dyMin*0.5f;

        // For each grid
        for(k=0;k<kN;k++)
        {
            // Initialize working variables
            int                 x,y;
            CRegion            RG = g_pScene->GetRegion(k);
            CFlux2DGrid*       pG = g_pScene->GetGrid(k);
            ///CFlux2DSurface*    pS = &pG->GetCurrentSurface();

            // Calculate the region in the output array for the current grid
            float   xf0 = (dimW-1)*(RG.x1-R.x1)/(R.x2-R.x1);
            float   xf1 = (dimW-1)*(RG.x2-R.x1)/(R.x2-R.x1);
            float   yf0 = (dimH-1)*(RG.y1-R.y1)/(R.y2-R.y1);
            float   yf1 = (dimH-1)*(RG.y2-R.y1)/(R.y2-R.y1);
            int     x0 = (int)xf0;
            int     x1 = (int)xf1;
            int     y0 = (int)yf0;
            int     y1 = (int)yf1;
            if(xf0<0)x0=0;
            if(xf1>=dimW)x1=dimW-1;
            if(yf0<0)y0=0;
            if(yf1>=dimH)y1=dimH-1;

            // Process the region by the output image, for every line in the grid
            for(y=y0;y<=y1;++y)
            {
                // Initialize variables
                float*  pDstHB = (float*)pHBData + dimW*2*y;
                float*  pDstUV = (float*)pUVData + dimW*2*y;
                float   yfGlobal = R.y1+(R.y2-R.y1)*(float)y/(float)(dimH-1);
                float   yfGrid = pG->GetDomainH()*(yfGlobal - RG.y1)/(RG.y2-RG.y1) - 0.5f;

                // Then for every pixel in the line
                for(x=x0;x<=x1;++x)
                {

                    float   xfGlobal = R.x1+(R.x2-R.x1)*(float)x/(float)(dimW-1);
                    float   xfGrid = pG->GetDomainW()*(xfGlobal - RG.x1)/(RG.x2-RG.x1) - 0.5f;
                    CSamplerBilinear S(xfGrid,yfGrid);
                    assert(S.GetUInt()>=-1);
                    assert(S.GetUInt()<pG->GetDomainW());
                    assert(S.GetVInt()>=-1);
                    assert(S.GetVInt()<pG->GetDomainH());

                    if(g_WaterSimulationParams.is_wind_waves)
                    {
                        pDstHB[2*x+0] = S(&pG->m_WindWavesH);
                    }
                    else
                    {
                        pDstHB[2*x+0] = S(&pG->GetCurrentSurface().m_heightMap);
                    }
                    pDstHB[2*x+1] = S(&pG->m_bottom.m_heightMap);
                    pDstUV[2*x+0] = S(&pG->GetCurrentSurface().m_uMap);
                    pDstUV[2*x+1] = S(&pG->GetCurrentSurface().m_vMap);
                }/// next grid pixel
            }/// next grid line 
        }/// next grid
        return 1;
    }/* GetScene */

    /**
     * @fn GetHybridMethodData()
     *
     * @brief Gets data produced by the hybrid method (vertices and boat )
     *
     * @param Bufs - An array of buffers such that:
     *               Bufs.pBufArray[0] - Is the boat data if enabled
     *
     * @param in_pData is pointer to sizes (width and height) of output matrix
     *
     * @param size - The size of in_pData
     *
     * @return - 1 if succesful, -1 otherwise
     */
    int GetHybridMethodDataHelper(void* in_pData)
    {

        // If the boat is enabled
        if(g_WaterSimulationParams.enableBoat)
        {

            // Get the transformation matrix, if any
            float *pBoatMatrix = g_pFluid->GetBoat()->GetTransformationMatrix();
            
            // If we have a transformation matrix
            if(pBoatMatrix)
            {
                // Copy the transformation matrix to the buffer
                memcpy(in_pData, pBoatMatrix, 16*sizeof(float)); 
            }
            bool isAnimationFrozen  = g_pFluid->GetBoat()->GetAnimationState();
            
            // Copy the flag indicating whether or not the animation on the HOST should be frozen
            memcpy(&((float*)in_pData)[16], &isAnimationFrozen, sizeof(bool)); 
        }

        return 1;
    }/* GetHybridMethodData */

    /**
     * @fn ReleaseScene()
     *
     * @brief Release all memory 
     *
     * @param Bufs - An array of buffers
     *
     * @param in_pData - A pointer to data
     *
     * @param size - The size of in_pData
     *
     * @return - 0 if succesful, -1 otherwise
     */
    int ReleaseScene()
    {
        // Destroy objects
        DestroyObject(g_pFluid);
        g_pScene = 0;
        g_pFluid = 0;

        return 0;
    }/* ReleaseScene */

///} /* extern "C" */
