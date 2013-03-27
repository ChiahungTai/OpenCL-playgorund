/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file commondatastruct.h
 *
 * @brief Defines common data structures
 */

#pragma once

#include "physics_3D_fluid_sim_common.h"

/**
 * @struct Point2D32i
 *
 * @brief A structure to store the X and Y integer coordinates of a 2D point
 */
struct Point2D32i
{
    int x,y;
    
    /**
     * @fn Point2D32i()
     *
     * @brief Structure constructor, initializes a point to (0,0)
     */
    Point2D32i(int _x=0, int _y=0)
    {
        x = _x;
        y = _y;
    }

    /**
     * @fn operator==(Point2D32i& b)
     *
     * @brief compares two points
     *
     * @return - TRUE if the points are identical, FALSE otherwise
     */
    int operator==(Point2D32i& b)
    {
        return (x==b.x && y==b.y);
    }
};

/**
 * @struct Point2D32f
 *
 * @brief A structure to store the X and Y float coordinates of a 2D point
 */
struct Point2D32f
{
    float   x; // x coordinate of 2D point 
    float   y; // y coordinate of 2D point 

    /**
     * @fn Point2D32i()
     *
     * @brief Structure constructor, initializes a point to (0.0,0.0)
     */
    Point2D32f(float _x=0, float _y=0)
    {
        x = _x;
        y = _y;
    }
};

/**
 * @struct CFlux2DCalculatorPars
 *
 * @brief A structure used to transfer calculation parameters
 */
struct CFlux2DCalculatorPars
{
    float time; // current time of the simulation
    float tau;  // current time step
};

/**
 * @def MAX_NUMBER_OF_GRIDS
 *
 * @brief Defines the maximal number of grids allowed on the scene 
 */
#define MAX_NUMBER_OF_GRIDS 4

/**
 * @struct WIND_WAVE_PARAM
 *
 * @brief A structure to transfer wind wave generation parameters from the HOST to OCL device
 */
typedef struct 
{
    /**
     * @var number
     *
     * @brief The number of stohastic seeds
     */
    float   number;

    /**
     * @var speed
     *
     * @brief The propogation speed of wind waves
     */
    float   speed;

    /**
     * @var amplitude
     *
     * @brief The amplitude of stohastic seeds
     */
    float   amplitude;

    /**
     * @var force
     *
     * @brief The artifical force used to generate directed waves
     */
    float   force;

    /**
     * @var desipation
     *
     * @brief The wave dessipation coefficient used to stabilize increasing wave power
     */
    float   desipation;

    /**
     * @var direction
     *
     * @brief The direction in degrees of the directed wind wave
     */
    float   direction;

    /**
     * @var directed_waves
     *
     * @brief If this value is 1 then directed waves will be generated and the direction filed will be used
     */
    int     directed_waves; 

}WIND_WAVE_PARAM;

/**
 * @struct WATER_STRUCT
 *
 * @brief A structure to transfer water simulation parameters and control from HOST to OCL device
 */
typedef struct 
{
    bool  is_wind_waves;                    // True if wind waves should be generated in scene
    WIND_WAVE_PARAM ww;                     // parameters for wind waves generation

    bool    wasClicked;                     // Has the user clicked to the destiantion point for boat
    float   x;                              // Contains X destination point coordinates if wasClicked == true
    float   y;                              // Contains Y destination point coordinates if wasClicked == true
    float   Noise;                          // noise level for patch edge wind generation
    float   NoiseSpaceCoherence;            // distance between independent points on edge 
    float   NoiseTimeCoherence;             // time distance between independent generation moments 
    float   omega;                          // frequency of patch edge generation
    float   ampl;                           // amlitude of patch edge wave generation
    float   time_step;                      // simulation time step
    bool    enableBoat;                     // true if the boat is simulated
    float   boatTransform[16];              // transform matrix for the boat animation
    int     DomainW[MAX_NUMBER_OF_GRIDS];   // widths for each grid (LOD control)
    int     DomainH[MAX_NUMBER_OF_GRIDS];   // heights for each grid (LOD control)
    int     solverNumber;                   // use OCL or native solver
    int     sceneNumber;                    // scene number in the list
    bool    enableGPUProcessing;            // run OCL on GPU device (true or false) default false - run on CPU
}WATER_STRUCT;
