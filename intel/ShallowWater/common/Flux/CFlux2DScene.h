/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFlux2DScene.h
 *
 * @brief Declares routines for scene and grid initialization and for file IO.
 */

#pragma once

// - #Util/CRegion.h - header contains scene region class.
#include "Util/CRegion.h" 
// - #CFlux2DGrid.h - header contains declarations for routines to manage water surface grid initialization and file IO.
#include "CFlux2DGrid.h" 
// - #Util/CSamplerBilinear.h - header contains the bilinear sampler class.
#include "Util/CSamplerBilinear.h"


// Global constants and initial values for shallow water solver variables.
#define RECOMENDED_TIME_STEP 0.03f
#define GRAVITY 10.0f
#define DISSIPATION_COEFFICIENT 0.15f
#define MIN_HEIGHT_THRESHOLD 0.05f
#define MIN_NON_ZERO_SPEED_THRESHOLD 0.003f
#define WAVE_SPEED 8.0f

/**
 * @struct CSeamData
 *
 * @brief Tracks the grid seam, top, left, right and bottom
 */
typedef struct 
{
    int     GridTop;
    int     GridBottom;
    int     GridLeft;
    int     GridRight;
} CSeamData;

/**
 * @class CFlux2DScene
 *
 * @brief Variables and routines for scene manamement
 */
class CFlux2DScene
{
public:

#ifdef DEBUG_FLUX2DSCENE

    /**
     * @fn PrintGrids()
     *
     * @brief printes out the previous and current grid surfaces
     *
     * #note This code is kept for debug purposes
     */
    void PrintGrids()
    {
        int i,x,y;
        FILE* out = fopen("scene_grids.txt","at");
        for(i=0;i<m_Grids.Size();++i)
        {
            DynamicMatrix<float>*   pH = &(m_Grids[i].pFlux->GetPastSurface().m_heightMap);
            int                     bs = pH->Border();
            fprintf(out,"Grid %d PrevSurf:\n",i);
            for(y=-bs;y<pH->Height()+bs;++y)
            {
                fprintf(out,"y=%03d ",y);
                for(x=-bs;x<pH->Width()+bs;++x)
                {
                    fprintf(out,"%3.1f ",pH->At(x,y));
                }
                fprintf(out,"\n");
            }

            pH = &(m_Grids[i].pFlux->GetCurrentSurface().m_heightMap);
            fprintf(out,"Grid %d CurSurf:\n",i);
            for(y=-bs;y<pH->Height()+bs;++y)
            {
                fprintf(out,"y=%03d ",y);
                for(x=-bs;x<pH->Width()+bs;++x)
                {
                    fprintf(out,"%3.1f ",pH->At(x,y));
                }
                fprintf(out,"\n");
            }
        }
        fclose(out);
    }
#endif

    /**
     * @fn CFlux2DScene(int numGrids)
     *
     * @brief Class constructor, initializes the scene, sets paramaters
     *
     * @param numGrids - The number of grids
     */
    CFlux2DScene(int numGrids)
        : m_maxGrids(numGrids),
        m_recommendedTimeStep(RECOMENDED_TIME_STEP),
        m_time(0.0f)
    {
        // Key parameters for all methods
        m_GlobalParams.gravity         = GRAVITY;
        m_GlobalParams.Cf              = DISSIPATION_COEFFICIENT; 
        m_GlobalParams.Hmin            = MIN_HEIGHT_THRESHOLD;
        m_GlobalParams.minNonZeroSpeed = MIN_NON_ZERO_SPEED_THRESHOLD;
        m_GlobalParams.waveSpeed       = WAVE_SPEED;
    }

    /**
     * @fn SetCf(float Cf)
     *
     * @brief Sets the dissipation coefficient
     *
     * @param Cf - The dissipation coefficient
     */
    void SetCf(float Cf)
    { 
        m_GlobalParams.Cf = Cf; 
    }

    /**
     * @fn SetHmin(float Hmin)
     *
     * @brief Sets the minimum height threshold
     *
     * @param Hmin - The minimum height threshold
     */
    void SetHmin(float Hmin) 
    { 
        m_GlobalParams.Hmin = Hmin; 
    }


    /**
     * @fn CFlux2DScene(CFile &file)
     *
     * @brief Class constructor, reads from an IO stream
     *
     * @param file - The IO stream from which to read
     */
    CFlux2DScene(CFile &file);

    /**
     * @fn CFlux2DScene()
     *
     * @brief Class constructor, raw 
     */
    CFlux2DScene();

    /**
     * @fn ~CFlux2DScene()
     *
     * @brief Class destructor 
     */
    ~CFlux2DScene();

    /**
     * @fn InitFromFile(CFile &file)
     *
     * @brief Restores the grids from a file, restores the state as well
     *
     * @param file - The source file
     */
    void InitFromFile(CFile &file);

    /**
     * @fn Save(CFile &file)
     *
     * @brief Save the grids and the state to a file
     *
     * @param file - The output file
     */
    void Save(CFile &file) const;

    /**
     * @fn SaveState(CFile &file, int flags = 0)
     *
     * @brief Saves seam data, some parameters and all fluxes to the specified file
     *
     * @param file - The output file
     *
     * @param flags - Action flags
     */
    void SaveState(CFile &file, int flags = 0) const;

    /**
     * @fn RestoreState(CFile &file)
     *
     * @brief Restores the scene including seam data, parameters and fluxes from the specified file
     *
     * @param file - The input file
     */
    void RestoreState(CFile &file);

    /**
     * @fn AddGrid(CFlux2DSceneDef &scnDef, int resX, int resY, CRegion const &reg)
     *
     * @brief Adds a grid to the scene, validates that the region is within the scene
     *
     * @param scnDef - The scene
     *
     * @param resX - Width of the gird
     *
     * @param resY - Height of the grid
     *
     * @param reg - Region in which to add the grid
     *
     * @return - The total number of grids
     */
    int AddGrid(CFlux2DSceneDef &scnDef, int resX, int resY, CRegion const &reg);

    /**
     * @fn AddGrid(int resX, int resY, CRegion const &reg)
     *
     * @brief Validates the region and then creates the grid, adding it to the list of grids
     *
     * @param resX - The width of the grid
     *
     * @param resY - The height of the grid
     *
     * @param reg - The region of the scene
     *
     * @return - The total number of grids
     */
    int AddGrid(int resX, int resY, CRegion const &reg);

    /**
     * @fn CalculateBoundingRegion()
     *
     * @brief Calculates the bounding region and applies it to each grid
     */
    void CalculateBoundingRegion();

    /**
     * @fn FindGrid(float x, float y, CSamplerBilinear* pSampler = NULL)
     *
     * @brief Used to find a grid that contain the (x,y) point.
     *
     * @param x - The X coordinate of the point on the scene
     *
     * @param y - The Y coordinate of the point on the scene
     *
     * @param pSampler - A pointer to the sampler that will be initialized
     *                   to get the interpolated value from the found grid.
     *
     * @note If pSampler is NULL then the initiazation does not occur
     *
     * @return - Pointer to the grid is sucessful.
     *           Null if the point is out of the area
     */
    CFlux2DGrid* FindGrid(float x, float y, CSamplerBilinear* pSampler = NULL);

    /**
     * @fn GetSurfaceHeight(float x, float y)
     *
     * @brief Get the absolute height of the water surface at the specified point
     *
     * @param x - The X coordinate of the scene point 
     *
     * @param y - The Y coordinate of the scene point 
     *
     * @return - The height of the surface
     */
    float GetSurfaceHeight(float x, float y)
    {
        float   WaterHeight = 0;
        CSamplerBilinear   sampler;

        // Find the grid that contains the point
        CFlux2DGrid *pF = FindGrid(x, y, &sampler);
        // If found, get the water height
        if(pF)
        {
            WaterHeight = sampler(&pF->GetCurrentSurface().m_heightMap);


            WaterHeight -= sampler(&pF->GetBottom().m_heightMap);


        }
        return WaterHeight;
    }

    /**
     * @fn GetNumGrids()
     *
     * @brief Used to determine the number of grids in the scene 
     *
     * @return - The number of grids in the scene
     */
    int GetNumGrids() 
    { 
        return m_Grids.Size(); 
    } 

    /**
     * @fn GetGrid(int index = 0) 
     *
     * @brief Finds the grid with a specified index 
     *
     * @param index - The grid index
     *
     * @return - A pointer to the grid
     */
    CFlux2DGrid* GetGrid(int index = 0) 
    { 
        return m_Grids[index].pFlux; 
    }

    /**
     * @fn GetRegion(int index)
     *
     * @brief Finds the region with the specified index
     *
     * @param index - The region index
     *
     * @return - A pointer to the region
     */
    CRegion &GetRegion(int index)
    { 
        return m_Grids[index].region; 
    }

    /**
     * @fn GetRegion()
     *
     * @brief Used to obtain the scene's region
     *
     * @return - A pointer to the scene's region
     */
    CRegion &GetRegion() 
    { 
        return m_region; 
    }


    /**
     * @fn GetTime()
     *
     * @brief Used to determine the elapsed time
     *
     * @return - The scene time
     */
    float GetTime() const 
    { 
        return m_time; 
    }

    /**
     * @fn ResetTime()
     *
     * @brief Used to reset the time to zero
     */
    void  ResetTime() 
    { 
        m_time = 0.0f; 
    }

    /**
     * @fn Proceed2NextStep(float dt)
     *
     * @brief Advances the time, re-computes grids, processes seams
     *
     * @param dt - Delta or step time
     */
    void  Proceed2NextStep(float dt);


    /**
     * @fn GetRecommendedTimeStep()
     *
     * @brief Used to find the recommended time step
     *
     * @return - The recommended time step, the time delta
     */
    float GetRecommendedTimeStep() const 
    { 
        return m_recommendedTimeStep; 
    }

    /**
     * @fn SetRecommendedTimeStep(float step)
     *
     * @brief Sets the recommended time step
     *
     * @param - The recommended delta time or time step
     */
    void  SetRecommendedTimeStep(float step) 
    { 
        m_recommendedTimeStep = step; 
    }

    //private:  OK, not so private.......

    /**
     * @struct Grid
     *
     * @brief Defines the grid region and member fluxes 
     */
    struct Grid
    {
        CRegion      region;
        CFlux2DGrid* pFlux;
    };

    /**
     * @var m_GlobalParams
     *
     * @brief Global parameters for the sample
     */
    CFlux2DGlobalParameters  m_GlobalParams;


    /**
     * @fn GetGlobalPars()
     *
     * @brief Used to retrieve the global parameters
     *
     * @return - The global parameters structure
     */
    CFlux2DGlobalParameters & GetGlobalPars()
    {   
        return m_GlobalParams; 
    }

    /**
     * @var m_maxGrids
     *
     * @brief The maximum number of grids within the scene
     */
    int m_maxGrids;

    /**
     * @var m_region
     *
     * @brief The scene's region
     */
    CRegion m_region;

    /**
     * @var m_recommendedTimeStep
     *
     * @brief The recommended time step or delta time.
     */
    float m_recommendedTimeStep;

    /**
     * @var m_time
     *
     * @brief Virtual elapsed time
     */
    float m_time;

    /**
     * @var m_Grids
     *
     * @brief An array of grids associated with the scene
     */
    DynamicArray<Grid>  m_Grids;

    /**
     * @var m_SeamArray
     *
     * @brief An array of seam grids
     */
    DynamicArray<CSeamData> m_SeamArray;

    /**
     * @fn ProcessSeams(float dt)
     *
     * @brief Processes the seams, performs correction
     *
     * @param dt - The delta time
     */
    void ProcessSeams(float dt);

    /**
     * @fn SimulateNoiseWindWaves(float dt, WIND_WAVE_PARAM* pParam)
     *
     * @brief Applies wind force, noise, and viscosity to waves
     *
     * @param dt - Delta time
     *
     * @param pParam - Parameters for the wind waves
     */
    void SimulateNoiseWindWaves(float dt, WIND_WAVE_PARAM* pParam);

};


