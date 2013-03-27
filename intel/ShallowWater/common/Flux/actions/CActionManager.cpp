/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */


/**
 * @file CActionManager.cpp
 *
 * @brief Contains the IO routine implementations for boundary condition storage/reading.
 */

#include <string.h>
#include "Flux/actions/CActionManager.h"

#include "Flux/actions/CFlux2DBC_Gen.h"
#include "Flux/actions/CFlux2DBCCopy.h"
#include "Flux/actions/CFlux2DBCReflectLine.h"

/**
 * @def ADD_ACTION(_T)
 *
 * @brief Adds an action
 */
//#define ADD_ACTION(_T)\
//{\
//    TypeInfo &ti = m_pTypes.Append();\
//    ti.rawTypeName = #_T;\
//    ti.creatorFunc = _T::CreateFromFile;\
//    assert(ti.creatorFunc);\
//}

#define ADD_ACTION(_T)\
{\
    TypeInfo &ti = m_pTypes.Append();\
    ti.rawTypeName = #_T;\
    ti.creatorFunc = _T::CreateFromFile;\
}

/**
 * @fn CActionManager()
 *
 * @brief Class constructor
 */
CActionManager::CActionManager()
{
    m_pTypes.Clear();
    // Register all BC types here
    ADD_ACTION(CFlux2DBCSineWaveGenVerticalLeft);
    ADD_ACTION(CFlux2DBCWaterRiseVerticalLeft);
    ADD_ACTION(CFlux2DBCWaterWallVerticalRight);
    ADD_ACTION(CFlux2DBCCopy);
    ADD_ACTION(CFlux2DBCReflectLine);
};
#undef ADD_ACTION

/**
 * @fn Create(CFile &file)
 *
 * @brief Creates an CFlux2DAction from memory
 *
 * @param file - The file used to create storage 
 */
CFlux2DAction* CActionManager::Create(CFile &file)
{
    // Restore the raw class name
    char rawTypeName[256];
    file.Restore(rawTypeName, 256);

    // Look for the creation function
    for (int i = 0; i < m_pTypes.Size(); i++)
    {
        if (strcmp(m_pTypes[i].rawTypeName, rawTypeName) == 0)
        { 
            // Found! Call creation
            return (*m_pTypes[i].creatorFunc)(file);
        }
    }
    // Type not found!
    return NULL;
};

/**
 * @fn Save(CFile &file, CFlux2DAction* pAction)
 *
 * @brief Saves the action to the CFile
 *
 * @param file - The file to save the action to
 *
 * @param pAction - Pointer to the action to save
 */
void CActionManager::Save(CFile &file, CFlux2DAction* pAction)
{
    int i;
    for (i = 0; i < m_pTypes.Size(); i++)
    {
        if(m_pTypes[i].creatorFunc == pAction->GetCreateFunction())
        { 
            // Found! save with this name
            file.Save(m_pTypes[i].rawTypeName);
            pAction->SaveData(file);
            break;
        }
    }
}
