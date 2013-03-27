/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFlux2DGrid.h
 *
 * @brief Contains the routines declaration for water surface grid initialization and file IO. 
 */

#pragma once

#include "CFlux2DSurface.h"
#include "actions/CFlux2DAction.h"

#include "commondatastruct.h"

#define MAX_NUMBER_OF_ACTIONS 8

class CFlux2DGrid;

/**
 * @struct CFlux2DGlobalParameters
 *
 * @brief For all methods, manages key global parameters
 */
struct CFlux2DGlobalParameters
{
    float gravity;
    float Cf; 
    float Hmin;
    float minNonZeroSpeed;
    float waveSpeed;
};

/**
 * @struct CFlux2DGridParams
 *
 * @brief Manages grid parameters
 */
struct CFlux2DGridParams
{
    float gridStepW;
    float gridStepH;
    float rcpGridStepW;
    float rcpGridStepH;
};

/**
 * *class CFlux2DGrid
 *
 * @brief Basic flux code
 */

class CFlux2DGrid
{
public:
    NON_COPYABLE(CFlux2DGrid);

    /**
     * @enum CRITICAL_SPEED
     *
     * @brief Defines negative and positive less and more critical speeds
     */
    enum
    {
        SPEED_NEG_MORECRITICAL = 0,
        SPEED_NEG_LESSCRITICAL = 1,
        SPEED_POS_LESSCRITICAL = 2,
        SPEED_POS_MORECRITICAL = 3
    };

    /**
     * @enum Location Flags
     *
     * @brief Defines something as inside, on the border, or external to
     */
    enum 
    {
        OUTER = 0,
        BORDER = 1,
        INNER = 2
    };

    /**
     * @enum Save Actions
     *
     * @brief Save flags
     */
    enum
    {
        SAVE_ACTIONS = (1L << 0)
    };

    /**
     * @struct Modifier
     *
     * @brief Used to control modivication of a grid
     */
    struct Modifier
    {

        /**
         * @fn Apply(CFlux2DGrid &flux, float dt)
         *
         * @brief Sets the integration step time of the grid to the specified value
         *
         * @param flux - The grid
         *
         * @param dt - The integration step time
         */
        virtual void Apply(CFlux2DGrid &flux, float dt) = 0;

        /**
         * @fn Restore(CFlux2DGrid &flux)
         *
         * @brief Restores the integration step time of the specified grid
         *
         * @param flux - The grid
         */
        virtual void Restore(CFlux2DGrid &flux) = 0;
    };

    /**
     * @fn CFlux2DGrid(int domainW, int domainH)
     *
     * @brief Class constructor, initializes the size of the grid
     *
     * @param domainW - Width of the grid
     *
     * @param domainH - Height of the grid
     */
    CFlux2DGrid(int domainW, int domainH);

    /**
     * @fn ~CFlux2DGrid()
     *
     * @brief Class destructor
     */
    ~CFlux2DGrid();

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
    void LoadBottomFromSceneDef(struct CFlux2DSceneDef &def, float x1, float y1, float x2, float y2);

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
    void RaiseBottomOnMask(float RaiseValue);

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
    void SetBottomConstant(float depth, float lenWidth, float lenHeight);

    /**
     * @fn AddAction(CFlux2DAction* in_pAction)
     *
     * @brief Gets a pointer to the action object and appends it to the sync actions object.
     *
     * @note Memory for this object should be allocated in external code and shouldn't be released
     *
     * @param action - The action to add
     */
    CFlux2DAction* AddAction(CFlux2DAction* action);

    /**
     * @fn SetModifier(Modifier *pM)
     *
     * @brief Used to duplicate the modifier from another instance of the same class
     *
     * @param pM - A pointer to the instantiated modifier
     */
    void SetModifier(Modifier *pM) 
    { 
        m_pModifier = pM; 
    }

    /**
     * @fn ApplyModifier(float dt)
     *
     * @brief Calls the corresponding Modifier structure function
     *
     * @param dt - The value to send to the modifier
     */
    void ApplyModifier(float dt) 
    { 
        if (m_pModifier != NULL)
        {
            m_pModifier->Apply(*this, dt);
        }
    }

    /**
     * @fn RestoreModifier()
     *
     * @brief Calls the corresponding Modifier structure function
     */
    void RestoreModifier()
    { 
        if (m_pModifier != NULL)
        {
            m_pModifier->Restore(*this);
        }
    }

    /**
     * @fn ResetToZeroState()
     *
     * @brief This function sets the velocity map to zero and the absolute water level to zero
     */
    void ResetToZeroState();

    /**
     * @fn ResetToDryState()
     *
     * @brief This function sets velocity map and the height map to zero
     */
    void ResetToDryState();

    /**
     * @fn Save(CFile &file, int flags)
     *
     * @brief Saves the grid and all parameters to a file
     *
     * @param file - The IO stream to write to
     *
     * @param flags - Bit-wize operation flags
     */
    void Save(CFile &file, int flags = 0) const;

    /**
     * @fn Restore(CFile &file)
     *
     * @brief Restores a grid from a file including parameters and flags
     *
     * @param file - The IO stream from which to read the grid
     */
    void Restore(CFile &file);

    /**
     * @fn Proceed2NextStep()
     *
     * @brief Advances the previous surface index to the next step
     */
    void Proceed2NextStep() 
    { 
        m_PrevSurfaceIdx =  (m_PrevSurfaceIdx+1)%3; 
    }

    /**
     * @fn const &GetPastSurface() const
     *
     * @brief Used to retrieve a constant pointer to the previous surface
     *
     * @return - The pointer
     */
    CFlux2DSurface const &GetPastSurface() const 
    { 
        return *m_pSurfaces[m_PrevSurfaceIdx]; 
    }

    /**
     * @fn GetPastSurface()
     *
     * @brief Used to retrieve a pointer to the previous surface
     *
     * @return - The pointer
     */
    CFlux2DSurface       &GetPastSurface()       
    { 
        return *m_pSurfaces[m_PrevSurfaceIdx]; 
    }

    /**
     * @fn const &GetCurrentSurface() const 
     *
     * @brief Used to retrieve a constant pointer to the current surface
     *
     * @return - The pointer
     */
    CFlux2DSurface const &GetCurrentSurface() const 
    { 
        return *m_pSurfaces[(m_PrevSurfaceIdx+1)%3]; 
    }

    /**
     * @fn &GetCurrentSurface()
     *
     * @brief Used to retrieve a pointer to the current surface
     *
     * @return - The pointer
     */
    CFlux2DSurface       &GetCurrentSurface()   
    { 
        return *m_pSurfaces[(m_PrevSurfaceIdx+1)%3]; 
    }

    /**
     * @fn const &GetDestSurface() const
     *
     * @brief Used to retrieve a constant pointer to the destination surface
     *
     * @return - The pointer
     */
    CFlux2DSurface const &GetDestSurface() const 
    { 
        return *m_pSurfaces[(m_PrevSurfaceIdx+2)%3]; 
    }

    /**
     * @fn &GetDestSurface()
     *
     * @brief Used to retrieve a pointer to the destination surface
     *
     * @return - The pointer
     */
    CFlux2DSurface       &GetDestSurface()   
    { 
        return *m_pSurfaces[(m_PrevSurfaceIdx+2)%3]; 
    }

    /**
     * @fn GetBottom()
     *
     * @brief Used to retrieve a pointer to the bottom
     *
     * @return - The pointer
     */
    CFlux2DBottom        &GetBottom()       
    { 
        return m_bottom; 
    }

    /**
     * @fn GetGridStepW()
     *
     * @brief Used to obtain the width grid step
     *
     * @return - The width grid step
     */
    float GetGridStepW() const 
    { 
        return m_Params.gridStepW; 
    }

    /**
     * @fn GetGridStepH()
     *
     * @brief Used to obtain the height grid step
     *
     * @return - The height grid step
     */
    float GetGridStepH() const 
    {
        return m_Params.gridStepH; 
    }

    /**
     * @fn GetRcpGridStepW()
     *
     * @brief Used to obtain the reciprocal of the width grid step
     *
     * @return - The reciprocal value
     */
    float GetRcpGridStepW() const 
    { 
        return m_Params.rcpGridStepW; 
    }

    /**
     * @fn GetRcpGridStepH()
     *
     * @brief Used to obtain the reciprocal of the height grid step
     *
     * @return - The reciprocal value
     */
    float GetRcpGridStepH() const 
    { 
        return m_Params.rcpGridStepH; 
    }


    /**
     * @fn GetDomainW()
     *
     * @brief Used to obtain the width of the bottom height map
     *
     * @return - The width of the bottom height map
     */
    int GetDomainW() const 
    { 
        return m_bottom.m_heightMap.Width(); 
    }

    /**
     * @fn GetDomainH()
     *
     * @brief Used to obtain the height of the bottom height map
     *
     * @return - The height of the bottom height map
     */
    int GetDomainH() const 
    { 
        return m_bottom.m_heightMap.Height(); 
    }


    /**
     * @fn SetDomainSize(int w, int h)
     *
     * @brief Used to set the domain size of the bottom height map
     *
     * @param w - Width of the bottom height map
     *
     * @param h - Height of the bottom height map
     */
    void SetDomainSize(int w, int h)
    {
        float     SX,SY;

        // If the height map is already the specified size, return and do nothing
        if(w==m_bottom.m_heightMap.Width() && h==m_bottom.m_heightMap.Height()) return;

        // Set the new sizes for the data
        m_bottom.SetDomainSize(w,h);
        m_WindWavesH.SetSize(w,h);
        m_pSurfaces[0]->SetDomainSize(w,h);
        m_pSurfaces[1]->SetDomainSize(w,h);
        m_pSurfaces[2]->SetDomainSize(w,h);

        // Scale global parameters, grid step is calculated by determining the ratio
        // between the new settings and the old settings.
        SX = (float)(m_bottom.m_heightMap.Width())/(float)(m_bottom.m_heightMapBase.Width());
        SY = (float)(m_bottom.m_heightMap.Height())/(float)(m_bottom.m_heightMapBase.Height());
        m_Params = m_ParamsBase;
        m_Params.gridStepH /= SY;
        m_Params.gridStepW /= SX;
        m_Params.rcpGridStepW *= SX;
        m_Params.rcpGridStepH *= SY;
    }

    /**
     * @fn SetLOD(float level)
     *
     * @brief Sets the level of detail
     *
     * @note The specified level must be a value from 0 to 1.0 where
     *       0 means very rough data
     *       1 means very detailed data
     *
     * @param level - The desired level of detail
     */
    void SetLOD(float level)
    {
        // Declare and initialize working variables
        int   HBase = m_bottom.m_heightMapBase.Height();
        int   WBase = m_bottom.m_heightMapBase.Width();
        int   WNew = 16*(int)((WBase*level/16)+0.5f);
        // We cannot compute the new height factor until we test the width factor
        int   HNew;

        // Ensure that the new width is at least 16
        if(WNew<16)WNew=16;
        // Ensure that the new width is not greater than the base width
        if(WNew>WBase)WNew=WBase;

        // Compute a new level of detail based on the results
        level = (float)WNew/(float)WBase;

        // Calculate the new height factor
        HNew = (int)(HBase*level+0.5f);
        // Ensure that it is at least one
        if(HNew<1)HNew=1;
        // Ensure that it is not greater than the base height
        if(HNew>HBase)HNew=HBase;

        // Then set the new domain size
        SetDomainSize(WNew,HNew);
    }

    /**
     * @var  m_Globals
     *
     * @brief Global parameters
     */
    CFlux2DGlobalParameters  m_Globals;

    /**
     * @fn GetGlobalPars()
     *
     * @brief Used to obtain the global parameters
     *
     * @return - The global parameters object
     */
    CFlux2DGlobalParameters const& GetGlobalPars() const 
    { 
        return m_Globals; 
    }

    /**
     * @fn SetGlobalPars(CFlux2DGlobalParameters &GlobalParams)
     *
     * @brief Used to set global parameters for the sample
     *
     * @param GlobalParams - A parameter object with the global parameters
     */
    void SetGlobalPars(CFlux2DGlobalParameters &GlobalParams) 
    { 
        m_Globals = GlobalParams; 
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
    void RunActions(struct CFlux2DCalculatorPars const &pars, CFlux2DGlobalParameters const &globals);

    // Actions
    DynamicArray<CFlux2DAction*> m_SyncActions;
    CFlux2DGridParams            m_Params;          // Working parameter structure
    CFlux2DGridParams            m_ParamsBase;      // Base parameters, fixed

    DynamicMatrix<float>      m_WindWavesH;         // Array of waves with added synthetic wind waves
    DynamicMatrix<float>*     m_pWindWavesHPrev;    // The previous array of waves
    DynamicMatrix<float>*     m_pWindWavesHCur;     // An array of waves for the future
    CFlux2DBottom            m_bottom;              // The bottom

    DynamicMatrix<char> m_InnerMaskData;            // The inner mask
    int                 m_InnerMaskFlag;            // Set if the inner mask should be used

    /**
     * @fn FillInnerMask(Point2D32i coor)
     *
     * @brief 
     *
     * @param coor - Coordinates for the seed point
     */
    void  FillInnerMask(Point2D32i coor) 
    {
        // Declare and initialize working variables
        int             W = m_InnerMaskData.Width();
        int             H = m_InnerMaskData.Height();
        Point2D32i*     pQueue = NULL;
        // The size of the queue will be the height times the width
        int             QueueSize = W*H;
        int             QueueFront = -1;
        int             QueueEnd = -1;

        // Allocate memeory for the queue
        pQueue = (Point2D32i*)malloc(QueueSize*sizeof(Point2D32i));

        // Validate the queue pointer
        if(pQueue ==NULL)
        {
            printf("malloc return success but pointer to memory is NULL\n");
            return;
        };

        // If the coordinates are outside of the domain, update to inside the domain
        if(m_InnerMaskData.At(coor) == OUTER) 
        {
            m_InnerMaskData.At(coor) = INNER;
        }

        // Initialize the first entry in the queue with the seed point
        QueueFront = 0;
        QueueEnd = 0;
        pQueue[0] = coor;

        // While the queue end is greater than or equal to the front
        // (both start off at the value zero)
        while(QueueEnd>=QueueFront) 
        {
            int i;
            // Take the point from the front of the queue
            Point2D32i c = pQueue[QueueFront++];
            // Check in four directions
            for(i=0;i<4;++i)
            {
                // Flood fill
                Point2D32i NewP = c;

                // If i = 1 then decrement X or if i = 3 then increment X 
                if(i&1) NewP.x += (i&2)-1;
                // Otherwise, if i is 0 then decrement Y or if i is 2 then increment Y
                else NewP.y += (i&2)-1;

                // If either of the computed X or Y values is out of range, exit the loop
                if(NewP.x<0 || NewP.x>=W) continue;
                if(NewP.y<0 || NewP.y>=H) continue;

                // Check to see whether or not this point is filled
                // If it is NOT outside, we are done and can exit the loop
                if(m_InnerMaskData.At(NewP) != OUTER) continue;

                // Set the point as inner and add it to the queue
                m_InnerMaskData.At(NewP) = INNER;
                // Add the point into the queue

                // If the front of the queue has been incremented
                if(QueueFront>0)
                {
                    // Make sure that the we can legally decrement the index
                    assert((QueueFront-1)>=0);
                    // Replace the original point with the new point
                    pQueue[--QueueFront] = NewP;
                }
                else // We have already replaced the first item in the queue
                {
                    // Ensure that we don't overrun the queue
                    assert((QueueEnd+1)<QueueSize);
                    // Add the point to the end of the queue
                    pQueue[++QueueEnd] = NewP;
                }
            }// Check the next direction
        }// Move to the next point in the queue


        free(pQueue);
    };

protected:
    // For start and stop X calculations
    int                           m_PrevSurfaceIdx; // Index into previous surface
    CFlux2DSurface*               m_pSurfaces[3];   // Past, present and future surfaces
    Modifier*                     m_pModifier;      // Modifier object
};



