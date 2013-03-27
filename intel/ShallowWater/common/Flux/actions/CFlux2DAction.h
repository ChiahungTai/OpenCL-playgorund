/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFlux2DAction.h
 *
 * @brief Defines different grid actions taken after the simulation step
 */

#pragma once

#include "Util/memory.h"

class CFlux2DGrid;
class CFile;

/**
 * @class CFlux2DAction
 *
 * @brief A class that is used to apply different actions to the grid right after a simulation step
 *
 * @note It could be border condition or wave generator and so on.
 */
class CFlux2DAction
{
public:

    /**
     * @fn ~CFlux2DAction()
     *
     * @brief A virtual destructor to make all futher destructors virtual
     */
    virtual ~CFlux2DAction() 
    {
    
    }

    /**
     * @fn Run(pG, &pars, &globals )
     *
     * @brief Run an action to modify the correspondence grid
     *
     * @param CFlux2DGrid* pG - The grid to be modified
     *
     * @param CFlux2DCalculatorPars const &pars - The current simulation step parameters
     *
     * @param CFlux2DGlobalParameters const &globals- The global simulation parameters
     */
    virtual void Run(
        CFlux2DGrid* pG,
        struct CFlux2DCalculatorPars const &pars,
        struct CFlux2DGlobalParameters const &globals ) = 0;


    /**
     * @fn SaveData(CFile &file)
     *
     * @brief Implements a pack-save action into one block of memory
     *
     * @param file-  A reference to the interface in which to pack internal data
     */
    virtual void SaveData(CFile &file) const = 0;

    /**
     * @fn (*GetCreateFunction())(CFile &file)
     *
     * @brief Gets the creation function for the CFile
     *
     * @param file - The CFile to read
     */
    virtual CFlux2DAction* (*GetCreateFunction())(CFile &file)=0;
};


/**
 * @def DEF_CREATE_FUNC(_TYPE)
 *
 * @brief Creates a function of the specified type
 */
#define DEF_CREATE_FUNC(_TYPE)\
    static CFlux2DAction* CreateFromFile(CFile& file){return new (UseOwnMem)_TYPE(file);}\
    virtual CFlux2DAction* (*GetCreateFunction())(CFile &file){return CreateFromFile;};

// End of File