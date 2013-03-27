/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFile2DBCCopy.h
 *
 * @brief Contains the basic boundary conditions (BC) declaration. 
 */

#pragma once

#include "Flux/actions/CFlux2DAction.h"

/**
 * @class CFlux2DBCCopy
 *
 * @brief This class defines the actions that fill the border cells values H,U,V from neighborhood cells 
 */
class CFlux2DBCCopy : 
    public CFlux2DAction
{
public:
    DEF_CREATE_FUNC(CFlux2DBCCopy);

    /**
     * @enum DefCopyPlace
     *
     * @brief Defines the borders
     */
    enum DefCopyPlace
    {
        TOP = 0, ///< top border of grid y=-1
        BOTTOM = 1,///< bottom border of grid y=H
        LEFT = 2, ///< left border of grid x=-1
        RIGHT = 3 ///< right border of grid x=W
    };

    /**
     * @var m_CopyPlace
     *
     * @brief The border to process
     */
    DefCopyPlace   m_CopyPlace; 
    
    /**
     * @fn CFlux2DBCCopy(Place)
     *
     * @brief Initializes the copy action
     *
     * @param Place - Indicates which border will be processed
     */
    CFlux2DBCCopy(DefCopyPlace Place)
    {
        m_CopyPlace = Place;
    }
    
    /**
     * @fn CFlux2DBCCopy(CFile &file)
     *
     * @brief Initializes the copy action from memory
     *
     * @param file - A reference to the CFile interface from which to read data for initialization
     */
    CFlux2DBCCopy(CFile &file);
    
    /**
     * @fn SaveData(CFile &file)
     *
     * @brief Used to pack action data into the file
     *
     * @param file - A reference to the interface used for packing
     */
    virtual void SaveData(CFile &file) const;

    /**
     * @fn Run(pG, pars, globals )
     *
     * @brief This method runs the copy process
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

private:

    /**
     * @var m_x0
     *
     * @brief The x start coordinate 
     */
    int m_x0;

    /**
     * @var m_dx 
     *
     * @brief The delta-x used to increment the x coordinate 
     */
    int m_dx;

    /**
     * @var m_y0 
     *
     * @brief The y start coordinate 
     */
    int m_y0;

    /**
     * @var m_dy
     *
     * @brief The delta-y used to increment the y coordinate 
     */
    int m_dy;

    /**
     * @var m_SrcDX
     *
     * @brief The x shift to get the source x coordinate
     */
    int m_SrcDX;

    /**
     * @var m_SrcDY
     *
     * @brief The y shift to get the source y coordinate
     */
    int m_SrcDY;

    /**
     * @var m_Num
     *
     * @brief The number of cell that should be copied
     */
    int m_Num;
};

// End of File