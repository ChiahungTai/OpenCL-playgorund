/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file HybridMethod.cpp
 *
 * @brief Contains the class implementation which unifies the shallow water simulation and interaction with the floating boat
 */

#include <cassert>
#include <iostream>
#include "HybridMethod.h"
#include "Util/memory.h"
#include "common/physics_3D_fluid_sim_common.h"
#include "commondatastruct.h"

/**
 * @fn HybridMethod()
 *
 * @brief Class constructor, initializes internal variables
 */
HybridMethod::HybridMethod( )
    : m_pScene(0),
    m_pMethod(0)
{
    m_pBoat=NULL;
}

/**
 * @fn ~HybridMethod()
 *
 * @brief Hybrid method class destructor
 */
HybridMethod::~HybridMethod()
{
    DestroyObject(m_pMethod);
    DestroyObject(m_pMethodSSE);
    DestroyObject(m_pMethodOCL);
    DestroyObject(m_pMethodSerial);
    DestroyObject(m_pMethodSSESerial);
    if(m_pBoat)DestroyObject(m_pBoat);
    if(m_pScene)
    {
        DestroyObject(m_pScene);
        m_pScene = 0;
    }
}

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
void HybridMethod::Init(CFileMemory* pMemFile, unsigned int numThreads, void* in_pData, float x_boat_pos, float y_boat_pos, bool device_type_gpu)
{
    // --------------- 2D simulation initialization ---------------

    // Create the scene for the shallow water simulation
    if(m_pScene)
    {
        DestroyObject(m_pScene);
        m_pScene = 0;
    }
    m_pScene = new (UseOwnMem) CFlux2DScene(*pMemFile);
    assert(m_pScene);

    // Create the method to simulate the scene
    m_pMethod = (CFlux2DMethod*) new (UseOwnMem) CFlux2DCurrentMethod(*m_pScene, numThreads);
    m_pMethodSSE = (CFlux2DMethod*) new (UseOwnMem) CFlux2DCurrentMethodSSE(*m_pScene, numThreads);
    m_pMethodOCL = new (UseOwnMem) CFlux2DMethodOpenCLScalar(*m_pScene, numThreads, device_type_gpu);
    m_pMethodSerial = (CFlux2DMethod*) new (UseOwnMem) CFlux2DCurrentMethodSerial(*m_pScene, numThreads);
    m_pMethodSSESerial = (CFlux2DMethod*) new (UseOwnMem) CFlux2DCurrentMethodSSESerial(*m_pScene, numThreads);

    assert(m_pMethod);

    // Set the time step
    m_currentTimeStep = m_pScene->GetRecommendedTimeStep();

    // Initialize the boat model
    m_pBoat = new (UseOwnMem) Boat(in_pData, m_pScene, x_boat_pos, y_boat_pos);
}// End of HybridMethod::Init

/**
 * @fn SetTimeStep(float dt)
 *
 * @brief Sets the time step
 *
 * @param  dt - Delta time, the time step of the integration
 */
void HybridMethod::SetTimeStep(float dt)
{
    m_currentTimeStep = dt;
}

/**
 * @fn GetScene()
 *
 * @brief Gets the shallow water scene
 *
 * @return A pointer to the shallow water scene
 */
CFlux2DScene* HybridMethod::GetScene()
{
    return m_pScene;
}

/**
 * @fn GetTimeStep()
 *
 * @brie3f Gets the current time step of integration
 *
 * @return The time step of integration
 */
float HybridMethod::GetTimeStep()
{
    return m_currentTimeStep;
}

/**
 * @fn Update(WATER_STRUCT* in_pData)
 *
 * @brief Do the integration step for shallow water scene
 *
 * @param  in_pData - Data received from the host
 */
void HybridMethod::Update(WATER_STRUCT* in_pData)
{
    // Shallow water update
    switch(in_pData->solverNumber)
    {
    case 0: m_pMethodOCL->Step(m_currentTimeStep); break;  
    case 1: m_pMethod->Step(m_currentTimeStep); break;
    case 2: m_pMethodSSE->Step(m_currentTimeStep);  break;
    case 3: m_pMethodSerial->Step(m_currentTimeStep);  break;
    case 4: m_pMethodSSESerial->Step(m_currentTimeStep);  break;
    }

    // Boat update
    m_pBoat->Move(in_pData, m_pScene);
}

/**
* @fn RebuildOCLKernel(bool in_bRelaxedMath, bool in_bRunOnGPU)
*
* @brief Rebuilds OpenCL kernel for OpenCL calculator
*
* @param  in_bRelaxedMath - Relaxed math build option true or false
*/
void HybridMethod::RebuildOCLKernel(bool in_bRelaxedMath, bool in_bRunOnGPU)
{
    m_pMethodOCL->RebuildOCLKernel(in_bRelaxedMath, in_bRunOnGPU); 
}

