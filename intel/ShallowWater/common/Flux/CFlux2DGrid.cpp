/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFlux2DGrid.cpp
 *
 * @brief Contains the routines implementation for water surface grid initialization and file IO. 
 */


#include <float.h>
#include <math.h>
#include "macros.h"
#include "Util/CFileMemory.h"
#include "CFlux2DSceneDef.h"
#include "CFlux2DGrid.h"
#include "actions/CActionManager.h"

/**
 * @fn CFlux2DGrid(int domainW, int domainH)
 *
 * @brief Class constructor, initializes the size of the grid
 *
 * @param domainW - Width of the grid
 *
 * @param domainH - Height of the grid
 */
CFlux2DGrid::CFlux2DGrid(int domainW, int domainH) 
    : m_PrevSurfaceIdx(0), 
    m_bottom(domainW, domainH), 
    m_WindWavesH(domainW, domainH),
    m_pModifier(NULL)
{
    m_InnerMaskData.SetSize(domainW,domainH,0);
    m_InnerMaskFlag = 0;

    m_WindWavesH.Set(0);

    m_pWindWavesHPrev = new(UseOwnMem) DynamicMatrix<float>(domainW,domainH);
    m_pWindWavesHPrev->Set(0);
    m_pWindWavesHCur = new(UseOwnMem) DynamicMatrix<float>(domainW,domainH);
    m_pWindWavesHCur->Set(0);

    // Create surfaces
    m_pSurfaces[0] = new (UseOwnMem) CFlux2DSurface(domainW, domainH);
    m_pSurfaces[1] = new (UseOwnMem) CFlux2DSurface(domainW, domainH);
    m_pSurfaces[2] = new (UseOwnMem) CFlux2DSurface(domainW, domainH);

}

/**
 * @fn ResetToZeroState()
 *
 * @brief This function sets the velocity map to zero and the absolute water level to zero
 */
void CFlux2DGrid::ResetToZeroState()
{
    // Declare and initialize working variables, get the height, width and the border size
    int width  = m_bottom.m_heightMap.Width();
    int height = m_bottom.m_heightMap.Height();
    int bs = m_bottom.m_heightMap.Border();
    // Get pointers to the current and past surfaces
    CFlux2DSurface &surf = GetCurrentSurface();
    CFlux2DSurface &surfPrev = GetPastSurface();
    int x,y;

    // For every line
    for(y=-bs;y<height+bs;++y)
    {
        // For every pixel in the line
        for(x=-bs;x<width+bs;++x)
        {
            float   B = m_bottom.m_heightMap.At(x,y);
            // Adjust the water level to be greater than or equal to zero
            surfPrev.m_heightMap.At(x,y) = __max(0.0f, B);
            surf.m_heightMap.At(x,y) = __max(0.0f, B);
        }
    }
    // Set the velocity map to zero
    surf.m_uMap.Set(0);
    surf.m_vMap.Set(0);
    surfPrev.m_uMap.Set(0);
    surfPrev.m_vMap.Set(0);
}

/**
 * @fn ResetToDryState()
 *
 * @brief This function sets velocity map and the height map to zero
 */
void CFlux2DGrid::ResetToDryState()
{
    CFlux2DSurface &surf = GetCurrentSurface();
    surf.m_heightMap.Set(0);
    surf.m_uMap.Set(0);
    surf.m_vMap.Set(0);
}

/**
 * @fn LoadBottomFromSceneDef(struct CFlux2DSceneDef &def, float x1, float y1, float x2, float y2)
 *
 * @brief Loads the bottom within the specified region from a scene definition
 *
 * @param def - The scene definition 
 *
 * @param x1 - The starting X coordinate
 *
 * @param y1 - The starting Y coordinate
 *
 * @param x2 - The end X coordinate
 *
 * @param y2 - The end Y coorinate
 */
void CFlux2DGrid::LoadBottomFromSceneDef(CFlux2DSceneDef &def, float x1, float y1, float x2, float y2)
{
    // Declare and initialize working variables
    const int bs = m_bottom.m_heightMap.Border();
    const int width  = m_bottom.m_heightMap.Width();
    const int height = m_bottom.m_heightMap.Height();

    // Compute the horizontal and vertical grid steps as well as their recriprocol
    m_Params.gridStepW = (x2 - x1) / float(width);
    m_Params.gridStepH = (y2 - y1) / float(height);
    m_Params.rcpGridStepW = 1.0f / m_Params.gridStepW;
    m_Params.rcpGridStepH = 1.0f / m_Params.gridStepH;

    /* **************************************** DEPRECATED CODE *********************** */
    //float xCellSize = float(def.lenWidth) / float(def.resWidth);
    //float yCellSize = float(def.lenHeight) / float(def.resHeight);

    //float xStart = (x1 + (m_Params.gridStepW - xCellSize) * 0.5f) / xCellSize;
    //float yStart = (y1 + (m_Params.gridStepH - yCellSize) * 0.5f) / yCellSize;

    //float xStep = m_Params.gridStepW / xCellSize;
    //float yStep = m_Params.gridStepH / yCellSize;
    /* *************************************** END DEPRECATED CODE ********************** */

    // For every line in the height map
    for (int i = -bs; i < height+bs; i++)
    {
        // Compute the y coordinate and value
        float y = (i+0.5f)*m_Params.gridStepH + y1;
        float yVal = y*float(def.resHeight)/float(def.lenHeight) - 0.5f;
        // Get a pointer to the line in both the height and pre-computed data maps
        float *pH = m_bottom.m_heightMap.GetLinePtr(i);
        float *pD = m_bottom.m_precompDataMap.GetLinePtr(i);
        // For every pixel in the line
        for (int j = -bs; j < width+bs; j++)
        {
            // Compute the coordinate and value
            float x = (j+0.5f)*m_Params.gridStepW + x1;
            float xVal = x*float(def.resWidth)/float(def.lenWidth) - 0.5f;

            // Compute the sample height and store the value in the height map
            float h = def.GetSample(xVal, yVal);
            pH[j] = h;
            // Ensure that the modifier is greater than or equal to zero
            float hMod = __max(0.0f, h);
            // And store the value in the pre-computed data map
            pD[j] = hMod * sqrt(m_Globals.gravity * hMod);
        }
    }
    m_ParamsBase = m_Params;
}

/**
 * @fn RaiseBottomOnMask(float RaiseValue)
 *
 * @brief This function raises the bottom level according to a mask 
 *        indicating the dry or wet state
 *
 * @note This function is used for generating the channel bottom profile
 *
 * @param RaiseValue - The raise value
 */
void CFlux2DGrid::RaiseBottomOnMask(float RaiseValue)
{
    // Declare and initialize working variables
    int x,y;
    const int width  = m_bottom.m_heightMap.Width();
    const int height = m_bottom.m_heightMap.Height();

    // For every line in the height map
    for (y = 0; y < height; y++)
    {
        // Get past, present and future surfaces
        float* pH0 = &(GetPastSurface().m_heightMap.At(0,y));
        float* pH1 = &(GetDestSurface().m_heightMap.At(0,y));
        float* pH2 = &(GetCurrentSurface().m_heightMap.At(0,y));
        // Get pointers into the maps and data
        float* pB = &(m_bottom.m_heightMap.At(0,y));
        float* pD = &(m_bottom.m_precompDataMap.At(0,y));
        char*  pM = m_InnerMaskData.GetLinePtr(y);

        // For every pixel in the line
        for (x = 0; x < width; x++)
        {
            // If the mask for the previous pixel has a value
            if(pM[x-1])
            {


                // Raise the level for past, present and future surfaces
                pH0[x] -= RaiseValue;
                pH1[x] -= RaiseValue;
                pH2[x] -= RaiseValue;

                // Raise the bottom height
                pB[x] -= RaiseValue;
                // Ensure the modifier from the pre-computed data is greater than or equal to zero
                float hMod = __max(0.0f, pD[x]);
                // Adjust the pre-computed value
                pD[x] = hMod * sqrt(m_Globals.gravity * hMod);
            }
        }
    }
}

/**
 * @fn SetBottomConstant(float depth, float lenWidth, float lenHeight)
 *
 * @brief Sets the depth of the height map, updates the pre-computed data map, updates parameters
 *
 * @param depth - The new depth
 *
 * @param lenWidth  - The width of a cell used to compute the grid W step
 *
 * @param lenHeight - The height of a cell used to compute the grid H step
 */
void CFlux2DGrid::SetBottomConstant(float depth, float lenWidth, float lenHeight)
{
    // Declare and initialize working variables for the border width
    // and the height and width of the height map
    const   int bs = m_bottom.m_heightMap.Border();
    const int width  = m_bottom.m_heightMap.Width();
    const int height = m_bottom.m_heightMap.Height();

    // Initialize paramaters accounting for the border
    m_Params.gridStepW = lenWidth / float(width - 1+2*bs);
    m_Params.gridStepH = lenHeight / float(height - 1+2*bs);
    m_Params.rcpGridStepW = 1.0f / m_Params.gridStepW;
    m_Params.rcpGridStepH = 1.0f / m_Params.gridStepH;
    // Set the depth
    m_bottom.m_heightMap.Set(depth);
    m_bottom.m_precompDataMap.Set(depth * sqrt(m_Globals.gravity * depth));
    // Store the parameters
    m_ParamsBase = m_Params;
}

/**
 * @fn AddAction(CFlux2DAction* in_pAction)
 *
 * @brief Gets a pointer to the action object and appends it to the sync actions object.
 *
 * @note Memory for this object should be allocated in external code and shouldn't be released
 *
 * @param action - The action to add
 */
CFlux2DAction* CFlux2DGrid::AddAction(CFlux2DAction* in_pAction)
{
    m_SyncActions.Append(in_pAction);
    return in_pAction;
}


/**
 * @fn RunActions(struct CFlux2DCalculatorPars const &pars, CFlux2DGlobalParameters const &globals)
 *
 * @brief Traverses the list of sync actions and runs them
 *
 * @param pars - Calculator parameters
 *
 * @param globals - Global parameters
 */
void CFlux2DGrid::RunActions(struct CFlux2DCalculatorPars const &pars, CFlux2DGlobalParameters const &globals)
{
    int i;
    // For every sync action
    for (i = 0; i < m_SyncActions.Size(); i++)
    {
        // Run it
        m_SyncActions[i]->Run(this,pars, globals);
    }
}

/**
 * @fn Save(CFile &file, int flags)
 *
 * @brief Saves the grid and all parameters to a file
 *
 * @param file - The IO stream to write to
 *
 * @param flags - Bit-wize operation flags
 */
void CFlux2DGrid::Save(CFile &file, int flags) const
{
    file.Save(flags);
    file.Save(m_Globals);
    file.Save(m_ParamsBase);
    file.Save(m_PrevSurfaceIdx);

    GetCurrentSurface().Save(file);
    GetPastSurface().Save(file);
    m_bottom.Save(file);
    
    file.Save(m_InnerMaskFlag);
    m_InnerMaskData.Save(file);

    if ((flags & SAVE_ACTIONS) != 0)
    {
        CActionManager am;
        int i;
        // Save the number of actions
        uint32_t size = 0;
        size += m_SyncActions.Size();
        file.Save(size);
        // Save them all
        for (i = 0; i < m_SyncActions.Size(); i++)
        {
            am.Save(file,m_SyncActions[i]);
        }
    }
}

/**
 * @fn Restore(CFile &file)
 *
 * @brief Restores a grid from a file including parameters and flags
 *
 * @param file - The IO stream from which to read the grid
 */
void CFlux2DGrid::Restore(CFile &file)
{
    int flags;

    // Restore flags, global parameters, and the parameter base
    file.Restore(flags);
    file.Restore(m_Globals);
    file.Restore(m_ParamsBase);
    // Use the base parameters for local parameters
    m_Params = m_ParamsBase;
    // The index into the previous surface
    file.Restore(m_PrevSurfaceIdx);

    // Get the current and past surfaces
    GetCurrentSurface().Restore(file);
    GetPastSurface().Restore(file);
    // Restore the bottom
    m_bottom.Restore(file);
    
    // Get the inner mask flag
    file.Restore(m_InnerMaskFlag);
    // Restore its contents
    m_InnerMaskData.Restore(file);

    fflush(NULL);

    // Check to see if we have a save actions flag, if we do, we did
    if ((flags & SAVE_ACTIONS) != 0)
    {
        CActionManager am;
        uint32_t numberOfActions;
        file.Restore(numberOfActions);
        fflush(NULL);

        // For each action saved
        for (uint32_t i = 0; i < numberOfActions; i++)
        {
            // Read the action from the file
            CFlux2DAction* action = am.Create(file);
            assert(action != NULL);
            // If valid, add it to the list of sync actions
            if (action != NULL)
            {
                fflush(NULL);
                AddAction(action);
            }
        }
    }
}

/**
 * @fn ~CFlux2DGrid()
 *
 * @brief Class destructor
 */
CFlux2DGrid::~CFlux2DGrid()
{
    // Free memory
    
    // First, delete the actions
    uint32_t size = m_SyncActions.Size();
    for(uint32_t i=0;i<size;i++ )
    {
        DestroyObject(m_SyncActions[i]);
    }
    // Then the surfaces 
    DestroyObject(m_pSurfaces[0]);
    DestroyObject(m_pSurfaces[1]);
    DestroyObject(m_pSurfaces[2]);
    DestroyObject(m_pWindWavesHCur);
    DestroyObject(m_pWindWavesHPrev);

}

