/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CActionManager.h
 *
 * @brief Contains the IO routine declarations for boundary conditions storage/reading.
 */

#pragma once

#include "commondatastruct.h"
#include "Flux/actions/CFlux2DAction.h"
#include "Util/CFileMemory.h"
#include "Util/memory.h"

/**
 * @class CActionManager
 *
 * @brief A class used to manage the storage and acquisition of boundary conditions
 */
class CActionManager
{
public:

    /**
     * @fn CActionManager()
     *
     * @brief Class constructor
     */
    CActionManager();

    /**
     * @fn Create(CFile &file)
     *
     * @brief Creates an CFlux2DAction from memory
     *
     * @param file - The file used to create storage 
     */
    CFlux2DAction* Create(CFile &file);

    /**
     * @fn Save(CFile &file, CFlux2DAction* pAction)
     *
     * @brief Saves the action to the CFile
     *
     * @param file - The file to save the action to
     *
     * @param pAction - Pointer to the action to save
     */
    void Save(CFile &file, CFlux2DAction* pAction);

private:

    /**
     * @struct TypeInfo
     *
     * @brief Stores inforamtion about the action
     */
    struct TypeInfo
    {
        /**
         * @var rawTypeName
         *
         * @brief The name of the action class
         */
        char const *rawTypeName;

        /**
         * @fn  (*creatorFunc)(CFile &file)
         *
         * @brief The function that creates the class
         */
        CFlux2DAction* (*creatorFunc)(CFile &file); 
    };

    /**
     * @var m_pTypes
     *
     * @brief A dynamic array of action types
     */
    DynamicArray<TypeInfo>  m_pTypes;
};
