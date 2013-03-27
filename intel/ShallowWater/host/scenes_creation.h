/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file scenes_creation.cpp
 *
 * @brief Declarations of host-side class objects used for creating the water physics simulation scenes
 *
 * @note Not for Collada scenes
 */

#pragma once

#include "Flux/CFlux2DScene.h"

/**
 * @struct CSceneDef
 *
 * @brief Describes the definition for a scene, coordinates and file source
 */
struct CSceneDef
{
    /**
     * @fn CSceneDef( )
     *
     * @brief Raw structure constructor
     */
    CSceneDef( ) 
    {

    }

    /**
     * @fn CSceneDef(wchar_t const *fileName_, float minH_, float maxH_, float lenW_, float lenH_) 
     *
     * @brief Parameterized class constructor
     *
     * @param fileName_ - File from which to read the scene
     *
     * @param minH_ - Minimum height for the scene
     *
     * @param maxH_ - Maximum height for the scene
     *
     * @param lenW_ - The length of the width
     *
     * @param lenH_ - The length of the height
     */
    CSceneDef(wchar_t const *fileName_, float minH_, float maxH_, float lenW_, float lenH_) 
        : fileName(fileName_), 
        minH(minH_), 
        maxH(maxH_), 
        lengthWidth(lenW_), 
        lengthHeight(lenH_)
    {
    
    }

    /**
     * @var fileName
     *
     * @brief The file from which the scene is read
     */
    wchar_t const *fileName;

    /**
     * @var minH
     *
     * @brief Minimum H value for the scene
     */
    float minH;

    /**
     * @var maxH
     *
     * @brief Maximum H value for the scene
     */
    float maxH;

    /**
     * @var lengthWidth
     *
     * @brief The horizontal length of the scene
     */
    float lengthWidth;

    /**
     * @var lengthHeight
     *
     * @brief The vertical length of the scene
     */
    float lengthHeight;
};

/**
 * @fn SceneCreateBasicParametrized( def, resW, resH, ampl, omega, lengthAlongV, bReflectingEnd)
 *
 * @brief Creates a basic parameterized scene with a single grid
 *
 * @param def - The scene definition
 *
 * @param resW - Width of the scene
 *
 * @param resH - Height of the scene
 *
 * @param ampl - Amplitude of the waves
 *
 * @param omega - Wave frequency
 *
 * @param lengthAlongV - The wave length along the left border
 *
 * @param bReflectingEnd - Indicates whether or not the right border is reflected
 *
 * @return Pointer to the created scene
 */
CFlux2DScene* SceneCreateBasicParametrized( CSceneDef const &def, 
    int resW, 
    int resH, 
    float ampl, 
    float omega, 
    float lengthAlongV, 
    bool bReflectingEnd = false );

/**
 * @fn SceneCreateWaterRisingParametrized( def, resW, resH, startH, endH, time )
 *
 * @brief Creates a basic scene with water rising boundary conditions (parametrized).
 *
 * @note Used for rivers and lakes scenes. 
 *
 * @param def - The scene definition
 *
 * @param resW - The resource width
 *
 * @param resH - The resource height
 *
 * @param startH - The starting water height
 *
 * @param endH - The ending water height
 *
 * @param time - The time for the water to rise
 *
 * @return - Pointer to the created scene
 */
CFlux2DScene* SceneCreateWaterRisingParametrized( CSceneDef const &def, int resW, int resH, float startH, float endH, float time );

/**
 * @fn SceneCreatePoolParametrized( CSceneDef const &def, int resW, int resH )
 *
 * @brief Creates a basic scene with free boundary conditions (parametrized).
 *
 * @param def - The scene definition
 *
 * @param resW - The resource width
 *
 * @param resH - The resource height
 *
 * @return Pointer to the created scene
 */
CFlux2DScene* SceneCreatePoolParametrized( CSceneDef const &def, int resW, int resH );

/**
 * @fn SceneCreateExperiment( int resW, int resH )
 *
 * @brief Creates an experimental scene
 *
 * @param resW - The resource width
 *
 * @param resH - The resource height
 *
 * @return Pointer to the created scene
 */
CFlux2DScene* SceneCreateExperiment( int resW, int resH );

/**
 * @fn SceneCreateMGridExperiment3GFlip()
 *
 * @brief Creates a multi-grid experimental scene
 *
 * @return Pointer to the created scene
 */
CFlux2DScene* SceneCreateMGridExperiment3GFlip();

/**
 * @fn SceneCreateLineReflectionChannel( def, resW, resH, ampl, omega, lengthAlongV, sinWave )
 *
 * @brief Creates a scene with linear boundary conditions (channels).
 *
 * @param def - The scene definition
 *
 * @param resW - Width of the scene
 *
 * @param resH - Height of the scene
 *
 * @param ampl - Amplitude of the waves
 *
 * @param omega - Wave frequency
 *
 * @param lengthAlongV - The wave length along the left border
 *
 * @param sinWave - Indicates whether or not to use sine wave generation
 *
 * @return Pointer to the created scene
 */
CFlux2DScene* SceneCreateLineReflectionChannel( CSceneDef const &def, 
    int resW,
    int resH,
    float ampl,
    float omega,
    float lengthAlongV,
    bool sinWave );

/**
 * @fn SceneCreateLineReflectionDemo( def, resW, resH, ampl, omega, lengthAlongV, sinWave )
 *
 * @brief Creates a scene with linear boundary conditions (channels demo).
 *
 * @param def - The scene definition
 *
 * @param resW - Width of the scene
 *
 * @param resH - Height of the scene
 *
 * @param ampl - Amplitude of the waves
 *
 * @param omega - Wave frequency
 *
 * @param lengthAlongV - The wave length along the left border
 *
 * @param sinWave - Indicates whether or not to use sine wave generation
 *
 * @return Pointer to the created scene
 */
CFlux2DScene* SceneCreateLineReflectionDemo( CSceneDef const &def,  
    int resW, 
    int resH, 
    float ampl, 
    float omega, 
    float lengthAlongV, 
    bool sinWave );

