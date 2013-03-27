/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFlux2DBC_Gen.h
 *
 * @brief Defines a class to generate waves on the left side of the grid
 */


#pragma once

#include "Flux/actions/CFlux2DAction.h"

/**
 * @class CFlux2DBCSineWaveGenVerticalLeft
 *
 * @brief A class to generate waves on the left side of the grid
 */
class CFlux2DBCSineWaveGenVerticalLeft : 
    public CFlux2DAction
{
public:
    DEF_CREATE_FUNC(CFlux2DBCSineWaveGenVerticalLeft);

    /**
     * @fn CFlux2DBCSineWaveGenVerticalLeft(float ampl, float omega, float lengthAlongV)
     *
     * @brief Class constructior, initializes the generator
     *
     * @param ampl - The amplitude of the waves
     *
     * @param omega - The frequency of the waves
     *
     * @param lengthAlongV - The wave length along left border 
     */
    CFlux2DBCSineWaveGenVerticalLeft(float ampl, float omega, float lengthAlongV)
    {
        m_ampl = ampl;
        m_omega = omega;
        m_lengthAlongV = lengthAlongV;
        m_pNoise = NULL;
        m_NoiseSize = 0;
    }
    
    /**
     * @fn CFlux2DBCSineWaveGenVerticalLeft(CFile &file)
     *
     * @brief Class constructor, this instances initializes the generator action from memory
     *
     * @param file - A reference to a CFile interface from which to read the data for initialization
     */
    CFlux2DBCSineWaveGenVerticalLeft(CFile &file);

    /**
     * @fn ~CFlux2DBCSineWaveGenVerticalLeft()
     *
     * @brief Class destructor, releases resources
     */
    ~CFlux2DBCSineWaveGenVerticalLeft()
    {
        if(m_pNoise)
        {
            free(m_pNoise);
        }
    }

    /**
     * @fn Run(pG, &pars, &globals)
     *
     * @brief Runs the generation process
     *
     * @param pG - The grid to be modified
     *
     * @param &pars - The current simulation step parameters
     *
     * @param &globals ) The global simulation parameters
     */
    virtual void Run(
        CFlux2DGrid* pG,
        struct CFlux2DCalculatorPars const &pars,
        struct CFlux2DGlobalParameters const &globals );

    /**
     * @fn void SaveData(CFile &file)
     *
     * @brief Used to pack the action data into a file
     *
     * @param file - A reference to the file interface used for packing
     */
    virtual void SaveData(CFile &file) const;

private:

    /**
     * @var m_ampl
     *
     * @brief The amplitude of generated waves
     */
    float m_ampl;

    /**
     * @var m_omega
     *
     * @brief The frequency of generated wave
     */
    float m_omega;

    /**
     * @var m_lengthAlongV
     *
     * @brief The wave length along the left border
     */
    float m_lengthAlongV;

    /* **************************************************************************** *\
                         Variables used to generate stochastic waves 
    \* **************************************************************************** */

    /**
     * @var m_pNoise
     *
     * @brief An array of noise values to generate the stochastic phase of a wave
     */
    float*  m_pNoise;

    /**
     * @var m_NoiseSize
     *
     * @brief The size of the noise array
     */
    int     m_NoiseSize;

    /**
     * @var m_LastTime
     *
     * @brief The time when the last random values were generated
     */
    float   m_LastTime;
};

/**
 * @class CFlux2DBCWaterRiseVerticalLeft
 *
 * @brief A class to generate the vertical rise of water on the left side of grid
 */
class CFlux2DBCWaterRiseVerticalLeft :
    public CFlux2DAction
{
public:
    DEF_CREATE_FUNC(CFlux2DBCWaterRiseVerticalLeft);

    /**
     * @fn CFlux2DBCWaterRiseVerticalLeft(float startH, float stopH, float timeInterval)
     *
     * @brief Class constructor, initializes the generator
     *
     * @param startH - The starting height value
     *
     * @param stopH - The endintg height value
     *
     * @param timeInterval - The elapsed time in an interval
     */
    CFlux2DBCWaterRiseVerticalLeft(float startH, float stopH, float timeInterval)
    {
        m_startHeight = startH;
        m_stopHeight = stopH;
        m_timeInterval = timeInterval;
    }
    
    /**
     * @fn CFlux2DBCWaterRiseVerticalLeft(CFile &file)
     *
     * @brief Class constructor, initializes the raise generator from memory
     *
     * @param file - A reference to CFile interface to read data for initialization data
     */
    CFlux2DBCWaterRiseVerticalLeft(CFile &file);

    /**
     * @fn Run(pG, pars, globals)
     *
     * @brief This method runs the raise process
     *
     * @param pG - The grid to be modified
     *
     * @param pars - Address of the current simulation step parameters
     *
     * @param globals - Address of the global simulation parameters
     */
    virtual void Run(
        CFlux2DGrid* pG,
        struct CFlux2DCalculatorPars const &pars,
        struct CFlux2DGlobalParameters const &globals ); 

    /**
     * @fn void SaveData(CFile &file)
     *
     * @brief Used to pack the action data into CFileMemory
     *
     * @param file - The reference to the interface used for packing
     */
    virtual void SaveData(CFile &file) const;

private:

    /**
     * @var m_startHeight
     *
     * @brief The level of water height at the start of the process
     */
    float m_startHeight;

    /**
     * @var m_stopHeight
     *
     * @brief The level of water height at the end of the process
     */
    float m_stopHeight;

    /**
     * @var m_timeInterval
     *
     * @brief The time period of water raising
     */
    float m_timeInterval;
};

/**
 * @class CFlux2DBCWaterWallVerticalRight
 *
 * @brief Defines the action that controls the propagation of a bar of water on the right
 */
class CFlux2DBCWaterWallVerticalRight : 
    public CFlux2DAction
{
public:
    DEF_CREATE_FUNC(CFlux2DBCWaterWallVerticalRight);

    /**
     * @fn CFlux2DBCWaterWallVerticalRight()
     *
     * @brief Class constructor
     */
    CFlux2DBCWaterWallVerticalRight() 
    {
    
    }

    /**
     * @fn CFlux2DBCWaterWallVerticalRight()
     *
     * @brief Class constructor, initializes from the file
     */
    CFlux2DBCWaterWallVerticalRight(CFile &)
    {
    
    }

    /**
     * @fn Run(pG, pars, globals )
     *
     * @brief This method runs the raise process
     *
     * @param pG - The grid to be modified
     *
     * @param pars - Address of the current simulation step parameters
     *
     * @param globals - Address of the global simulation parameters
     */
    virtual void Run( CFlux2DGrid*pG, 
        CFlux2DCalculatorPars const &pars, 
        CFlux2DGlobalParameters const &globals );

    /**
     * @fn void SaveData(CFile &file)
     *
     * @brief Used to pack the action data into CFileMemory
     *
     * @param file - The reference to the interface used for packing
     */
    virtual void SaveData(CFile &) const 
    {
    
    }
};
// End of file