/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

#pragma once

/**
 * @file physics_3D_fluid_sim_common.h
 *
 * @brief Common information to share between host and OCL sides.
 */


/**
 * @def SCENE_NAME
 *
 * @brief The name of the scene. 
 *
 */
#define SCENE_NAME L"Lighthouse"
#define SCENE_NAME_2 L"Lake"
#define SCENE_NAME_4 L"Lighthouse_Large"
#define SCENE_NAME_5 L"Lake_Large"
#define SCENE_NAME_6 L"Lighthouse_XL"
#define SCENE_NAME_7 L"Lake_XL"


/**
 * @def STOP_SIMULATION_FOR_BAD_HEIGHT
 *
 * @brief Define this to stop the simulation in case of to high a height
 *
 * @note This works only for a simple visualization when the whole grid's values are transfered from OCL to HOST
 */
//#define STOP_SIMULATION_FOR_BAD_HEIGHT
