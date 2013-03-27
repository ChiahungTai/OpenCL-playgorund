/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file HybridMethod.h
 *
 * @brief Contains the class declarations which unify shallow water simulation and interaction with the floating boat
 */

#ifndef HYBRID_METHOD_H
#define HYBRID_METHOD_H


#include "Util/CFileMemory.h"
#include "CFlux2DMethod.h"
#include "boatlogic.h"
#include "physics_3D_fluid_sim_common.h"

/**
 *
 * @brief Defines using the shallow water calculator
 */

#include "CFlux2DSSECalculator.h"
// OpenMP parallel and serial SSE code 
typedef CFlux2DMethodOpenMP<CFlux2DSSECalculator> CFlux2DCurrentMethodSSE;
typedef CFlux2DMethodSerial<CFlux2DSSECalculator> CFlux2DCurrentMethodSSESerial;


#include "CFlux2DScalarCalculator.h"
// OpenMP parallel and serial scalar code 
typedef CFlux2DMethodOpenMP<CFlux2DScalarCalculator> CFlux2DCurrentMethod;
typedef CFlux2DMethodSerial<CFlux2DScalarCalculator> CFlux2DCurrentMethodSerial;


/**
 * @Class HybridMethod 
 *
 * @brief Defines the hybrid approach to 3D fluid simulation: Shallow Water
 */
class HybridMethod
{
    /* ********************************************************************** *\
                             2D surface simulation 
    \* ********************************************************************** */

    /**
     * @var m_pScene
     *
     * @brief The Shallow Water simulation scene
     */
    CFlux2DScene*      m_pScene;

    /**
     * @var m_pMethod
     *
     * @brief The method used to simulate shallow water
     */
    CFlux2DMethod*                 m_pMethod;
    CFlux2DMethod*                 m_pMethodSSE;
    CFlux2DMethodOpenCLScalar*     m_pMethodOCL;
    CFlux2DMethod*     m_pMethodSerial;
    CFlux2DMethod*     m_pMethodSSESerial;


    /* ********************************************************************** *\
                            Boat model
    \* ********************************************************************** */

    /**
     * @var m_pBoat
     *
     * @brief A class that implements the boat's behaviour and wave generation from the boat
     */
    Boat*   m_pBoat; 

    /**
     * @var m_currentTimeStep
     *
     * @brief The time step of the integration
     */
    float               m_currentTimeStep;

public:

    NON_COPYABLE(HybridMethod);

    /**
     * @fn HybridMethod()
     *
     * @brief Class constructor, initializes internal variables
     */
    HybridMethod();

    /**
     * @fn ~HybridMethod()
     *
     * @brief Hybrid method class destructor
     */
    ~HybridMethod();

    /**
     * @fn Init(CFileMemory* pMemFile, unsigned int numThreads, void* in_pData)
     *
     * @brief Initializes the hybrid object
     *
     * @param  pMemFile - The scene model
     *
     * @param  numThreads - The number of threads
     *
     * @param  in_pData - data for boat model scene
     */
    void Init(CFileMemory* pMemFile, unsigned int numThreads, void* in_pData, float x_boat_pos, float y_boat_pos, bool device_type_gpu);

    /**
     * @fn SetTimeStep(float dt)
     *
     * @brief Sets the time step
     *
     * @param  dt - Delta time, the time step of the integration
     */
    void SetTimeStep(float dt);

    /**
     * @fn GetScene()
     *
     * @brief Gets the shallow water scene
     *
     * @return A pointer to the shallow water scene
     */
    CFlux2DScene* GetScene();

    /**
     * @fn GetBoat()
     *
     * @brief Get the pointer to the boat
     *
     * @return - The pointer to the boat
     */
    Boat* GetBoat()
    {
        assert(m_pBoat);
        return m_pBoat;
    }

    /**
     * @fn GetTimeStep()
     *
     * @brie3f Gets the current time step of integration
     *
     * @return The time step of integration
     */
    float GetTimeStep();

    /**
     * @fn Update(WATER_STRUCT* in_pData)
     *
     * @brief Do the integration step for shallow water scene
     *
     * @param  in_pData - Data received from the host
     */
    void Update(WATER_STRUCT* in_pData);

    /**
     * @fn RebuildOCLKernel(bool in_bRelaxedMath, bool in_bRunOnGPU)
     *
     * @brief Rebuilds OpenCL kernel for OpenCL calculator
     *
     * @param  in_bRelaxedMath - Relaxed math build option true or false
	 *
	 * @param  in_bRunOnGPU - Disable optimizations on GPU option true or false
     */
    void RebuildOCLKernel(bool in_bRelaxedMath = false, bool in_bRunOnGPU = false);

};

#endif // HYBRID_METHOD_H
