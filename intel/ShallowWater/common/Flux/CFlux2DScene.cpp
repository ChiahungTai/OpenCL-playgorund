/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFlux2DScene.cpp
 *
 * @brief Contains the routines implementation for scene initialization and file IO.
 */

#include <float.h>
#include <math.h>
#include <new>
#include "Flux/CFlux2DSceneDef.h"
#include "Flux/CFlux2DScene.h"
#include "Util/CFileMemory.h"

/**
 * @fn CFlux2DScene(CFile &file)
 *
 * @brief Class constructor, reads from an IO stream
 *
 * @param file - The IO stream from which to read
 */
CFlux2DScene::CFlux2DScene(CFile &file)
{
    InitFromFile(file);
}

/**
 * @fn CFlux2DScene()
 *
 * @brief Class constructor, no initialization performed
 */
CFlux2DScene::CFlux2DScene()
{
}

/**
 * @fn ~CFlux2DScene()
 *
 * @brief Class destructor 
 */
CFlux2DScene::~CFlux2DScene()
{
    for(int i = 0; i<m_Grids.Size(); i++)
    {
        DestroyObject(m_Grids[i].pFlux);
    }
}

/**
 * @fn InitFromFile(CFile &file)
 *
 * @brief Restores the grids from a file, restores the state as well
 *
 * @param file - The source file
 */
void CFlux2DScene::InitFromFile(CFile &file)
{
    // Read the number of grids and then create the grids
    file.Restore(m_maxGrids);
    m_Grids.SetSize(m_maxGrids);
    for(int i = 0; i<m_maxGrids;i++)
    {
        m_Grids[i] = *(new (&m_Grids[i]) Grid());
    }

    // For each grid in the file
    for (int i = 0; i < m_maxGrids; i++)
    {
        // Read the width & height
        int width, height;
        file.Restore(width);
        file.Restore(height);
        // Create the grid
        m_Grids[i].pFlux = new (UseOwnMem) CFlux2DGrid(width, height);
    }

    // Now, restore the state
    RestoreState(file);
    // Initialize borders
    ProcessSeams(0);
}

/**
 * @fn Save(CFile &file)
 *
 * @brief Save the grids and the state to a file
 *
 * @param file - The output file
 */
void CFlux2DScene::Save(CFile &file) const
{
    // Save key information for creating the fluxes

    // First, the number of grids
    file.Save(m_Grids.Size());
    // And then the grids
    for (int i = 0; i < m_Grids.Size(); i++)
    {
        // Store the width and height of each flux
        file.Save(m_Grids[i].pFlux->GetDomainW());
        file.Save(m_Grids[i].pFlux->GetDomainH());
    }

    // Now, save the state
    SaveState(file, CFlux2DGrid::SAVE_ACTIONS);
}

/**
 * @fn RestoreState(CFile &file)
 *
 * @brief Restores the scene including seam data, parameters and fluxes from the specified file
 *
 * @note This method needs accurate memory management (e.g. with seam data)
 *
 * @param file - The input file
 */
void CFlux2DScene::RestoreState(CFile &file)
{
    int i,SeamNum;

    // First, restore the seam data 

    // Get the number of seams
    file.Restore(SeamNum);
    // Allocate memory
    m_SeamArray.SetSize(SeamNum);
    // Read the seams
    for(i = 0; i < m_SeamArray.Size(); i++)
    {
        file.Restore(m_SeamArray[i]);
    }

    // Restore parameters
    file.Restore(m_GlobalParams);
    file.Restore(m_recommendedTimeStep);
    file.Restore(m_time);
    file.Restore(m_region);

    // Restore the fluxes

    // For every grid
    for(i = 0; i < m_Grids.Size(); i++)
    {
        // Read the region parameters
        file.Restore(m_Grids[i].region);
        // Then restore the flux
        m_Grids[i].pFlux->Restore(file);
    }
}

/**
 * @fn SaveState(CFile &file, int flags = 0)
 *
 * @brief Saves seam data, some parameters and all fluxes to the specified file
 *
 * @param file - The output file
 *
 * @param flags - Action flags
 */
void CFlux2DScene::SaveState(CFile &file, int flags) const
{
    // First, save the seam data 
    int i,SeamNum = m_SeamArray.Size();
    file.Save(SeamNum);
    for(i = 0; i < SeamNum; i++)
    {
        file.Save(m_SeamArray[i]);
    }

    // Save critical parameters
    file.Save(m_GlobalParams);
    file.Save(m_recommendedTimeStep);
    file.Save(m_time);
    file.Save(m_region);

    // Save all fluxes
    for(i = 0; i < m_Grids.Size(); i++)
    {
        file.Save(m_Grids[i].region);
        m_Grids[i].pFlux->Save(file, flags);
    }

}

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
int CFlux2DScene::AddGrid(int resX, int resY, CRegion const &reg)
{
    assert(0.0f <= reg.x1 && reg.x1 < reg.x2);
    assert(0.0f <= reg.y1 && reg.y1 < reg.y2);
    // Get the current grid
    Grid* pG = &(m_Grids.Append());
    // Add a new grid
    pG = new (pG) Grid();
    // Create flux scene adding the new grid
    pG->pFlux = new (UseOwnMem) CFlux2DGrid(resX, resY);
    // Set the parameters
    pG->pFlux->SetGlobalPars(GetGlobalPars());
    // Copy the region
    pG->region = reg;
    // And return the total number of grids
    return m_Grids.Size() - 1;
}

/**
 * @fn AddGrid(CFlux2DSceneDef &scnDef, int resX, int resY, CRegion const &reg)
 *
 * @brief Adds a grid to the scene
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
int CFlux2DScene::AddGrid(CFlux2DSceneDef &scnDef, int resX, int resY, CRegion const &reg)
{
    assert(reg.x2 <= scnDef.lenWidth);
    assert(reg.y2 <= scnDef.lenHeight);
    int index = AddGrid(resX, resY, reg);
    GetGrid(index)->LoadBottomFromSceneDef(scnDef, reg.x1, reg.y1, reg.x2, reg.y2);
    return index;
}

/**
 * @fn CalculateBoundingRegion()
 *
 * @brief Calculates the bounding region and applies it to each grid
 */
void CFlux2DScene::CalculateBoundingRegion()
{
    // Initialize the region to maximum values
    m_region.x1 = FLT_MAX;
    m_region.x2 = -FLT_MAX;
    m_region.y1 = FLT_MAX;
    m_region.y2 = -FLT_MAX;

    // For every grids
    for (int i = 0; i < m_Grids.Size(); i++)
    {
        // Reduce the size of the region to the size of the smallest grid region
        m_region.x1 = __min(m_region.x1, m_Grids[i].region.x1);
        m_region.y1 = __min(m_region.y1, m_Grids[i].region.y1);
        m_region.x2 = __max(m_region.x2, m_Grids[i].region.x2);
        m_region.y2 = __max(m_region.y2, m_Grids[i].region.y2);
    }
}

/**
 * @fn Proceed2NextStep(float dt)
 *
 * @brief Advances the time, re-computes grids, processes seams
 *
 * @param dt - Delta or step time
 */
void CFlux2DScene::Proceed2NextStep(float dt)
{
    // First, advance the time
    m_time += dt;

    // For each grid
    for (int i = 0; i < m_Grids.Size(); i++)
    {
        // Get the surface
        CFlux2DSurface* pS = &(m_Grids[i].pFlux->GetDestSurface());
        int k;

        // Restore modifier results
        GetGrid(i)->RestoreModifier();


        // Fill the grid corners
        for(k=0;k<3;++k)
        {
            // Get a map depending on the value of k
            DynamicMatrix<float>* pD = (k==0)?&(pS->m_heightMap):(k==1)?&(pS->m_uMap):&(pS->m_vMap);
            // Get the height and width
            int W = pD->Width();
            int H = pD->Height();
            // Calculate averages for the corners
            pD->At(-1,-1) = 0.5f*(pD->At(0,-1)+pD->At(-1,0));
            pD->At(W,-1) = 0.5f*(pD->At(W-1,-1)+pD->At(W,0));
            pD->At(-1,H) = 0.5f*(pD->At(0,H)+pD->At(-1,H-1));
            pD->At(W,H) = 0.5f*(pD->At(W-1,H)+pD->At(W,H-1));
        }// Move to the next map

        m_Grids[i].pFlux->Proceed2NextStep();
    }

    ProcessSeams(dt);
}

/**
 * @def DEEP_CORRECTION
 *
 * @brief Define this if you want to correct the water surface of the grid in deep but not only on the edge
 *
 * @note This definition could help to stitch grids with different cell sizes
 */

#define DEEP_CORRECTION

/**
 * @def XY(_x,_y)
 *
 * @brief A macro that switches X and Y for vertical seams (X,Y) -> (Y,X)
 */
#define XY(_x,_y) HorizontalSeam?(_x):(_y),HorizontalSeam?(_y):(_x)

/**
 * @fn ProcessSeams(float dt)
 *
 * @brief Processes seams
 *
 * @param dt - Delta time, step time
 */
void CFlux2DScene::ProcessSeams(float dt)
{
    int     i;

#ifndef DEEP_CORRECTION
    float   W1 = 0.0f; // Weight for the initial configuration
    float   W2 = 1.0f; // Weight for the final configuration
#endif

    // For every seam in the array
    for( i=0; i < m_SeamArray.Size(); ++i)
    {
        // Initialize variables
        int             CorrectionNum[2] = {0};
        float           Correction[2][1024];
        // Get the seam
        CSeamData*      pSD = &(m_SeamArray[i]);
        // See if it is horizontal or vertical
        int             HorizontalSeam = (pSD->GridTop>=0 && pSD->GridBottom>=0);

        // Working variables
        CRegion         R[2];                   // The regions of the grids
        CFlux2DGrid*    pG[2];                  // The grids
        
        float           StepOrt[2];             // The size of cells along the direction
                                                // that is orthogonal to the seam line
        float           StepAlong[2];           // The size of cells along the seam line
        int             SizeOrt[2];             // The size of the grid along the direction
                                                // that is orthogonal to the seam line
        int             SizeAlong[2];           // The size of the grid along the seam line
        float           RegionStartAlong[2];    // The first coordinate of the region along the seam line
        float           RegionStopAlong[2];     // The second coordinate of the region along the seam line
        int             tSmallStep=-1;
        int             t,tt;                   // grid indicies
        int             x;
        float           x0,x1;                  // The start and stop coordinates of the region intersection
                                                // along the seam line
        int             x0i[2],x1i[2];          // The start and stop indicies of grid elements along seam borders
        int             y[2][4];                // The index of lines along the seam that will be used for 
                                                // shift calculations

        // For horizontal seams
        if(HorizontalSeam)
        {
            // Get top and bottom regions and grids
            R[0] = GetRegion(pSD->GridTop);
            R[1] = GetRegion(pSD->GridBottom);
            pG[0] = GetGrid(pSD->GridTop);
            pG[1] = GetGrid(pSD->GridBottom);
            // Get the size of the cells
            StepOrt[0] = pG[0]->GetGridStepH();
            StepOrt[1] = pG[1]->GetGridStepH();
            StepAlong[0] = pG[0]->GetGridStepW();
            StepAlong[1] = pG[1]->GetGridStepW();
            // Get the size of the grids
            SizeOrt[0] = pG[0]->GetDomainH();
            SizeOrt[1] = pG[1]->GetDomainH();
            SizeAlong[0] = pG[0]->GetDomainW();
            SizeAlong[1] = pG[1]->GetDomainW();
            // Get the coorinates of the region
            RegionStartAlong[0] = R[0].x1;
            RegionStartAlong[1] = R[1].x1;
            RegionStopAlong[0] = R[0].x2;
            RegionStopAlong[1] = R[1].x2;
        }
        else // We have a Vertical seam
        {   
            // Check it
            assert(pSD->GridLeft>=0 && pSD->GridRight>=0);
            // Get the left and right regions and grids
            R[0] = GetRegion(pSD->GridLeft);
            R[1] = GetRegion(pSD->GridRight);
            pG[0] = GetGrid(pSD->GridLeft);
            pG[1] = GetGrid(pSD->GridRight);
            // Get the size of the cells
            StepOrt[0] = pG[0]->GetGridStepW();
            StepOrt[1] = pG[1]->GetGridStepW();
            StepAlong[0] = pG[0]->GetGridStepH();
            StepAlong[1] = pG[1]->GetGridStepH();
            // Get the size of the grids
            SizeOrt[0] = pG[0]->GetDomainW();
            SizeOrt[1] = pG[1]->GetDomainW();
            SizeAlong[0] = pG[0]->GetDomainH();
            SizeAlong[1] = pG[1]->GetDomainH();
            // Get the coordinates of the region
            RegionStartAlong[0] = R[0].y1;
            RegionStartAlong[1] = R[1].y1;
            RegionStopAlong[0] = R[0].y2;
            RegionStopAlong[1] = R[1].y2;
        }
        
        // Find the start and stop coordinates
        x0 = max(RegionStartAlong[0],RegionStartAlong[1]);
        x1 = min(RegionStopAlong[0],RegionStopAlong[1]);

        // Determine the small step size
        if(StepOrt[0]<StepOrt[1])
        {
            tSmallStep = 0;
        }
        else
        {
            tSmallStep = 1;
        }

        // Compute start and stop indicies
        x0i[0] = max((int)((x0-RegionStartAlong[0])/StepAlong[0]),0);
        x1i[0] = min((int)((x1-RegionStartAlong[0])/StepAlong[0]),SizeAlong[0]-1);
        x0i[1] = max((int)((x0-RegionStartAlong[1])/StepAlong[1]),0);
        x1i[1] = min((int)((x1-RegionStartAlong[1])/StepAlong[1]),SizeAlong[1]-1);

        // Calculate the Y coordinates for the four points that are used for interpolation
        y[0][0] = SizeOrt[0]-(tSmallStep==0?3:2);
        y[0][1] = SizeOrt[0]-1;
        y[0][2] = 0;
        y[0][3] = (tSmallStep==0)?1:2;
        y[1][0] = y[0][3];
        y[1][1] = y[0][2];
        y[1][2] = y[0][1];
        y[1][3] = y[0][0];

        if(StepOrt[0] > 1.5f*StepOrt[1] || StepOrt[1] > 1.5f*StepOrt[0])
        {   
            // Calculate the top grid correction only for different cell sizes

            // Make a seam for the top/left and after that for the bottom/right
            for(t=0;t<2;++t)
            {
                float           dx1 = StepOrt[t]*labs(y[t][0]-y[t][1]);
                float           dx2 = StepOrt[1-t]*labs(y[t][2]-y[t][3]);
                float           dx12 = (StepOrt[t]+StepOrt[1-t])*0.5f;

#ifndef DEEP_CORRECTION
                float           dx1_2 = dx1*dx1;
                float           dx2_2 = dx2*dx2;
                float           dx12_2 = dx12*dx12;
                float           Den = (W1+W2)*(dx1_2+dx2_2)*dx12_2 + 4*W2*((dx2+dx1)*dx12+2*dx2*dx1)*dx1*dx2;
                float           RcpDen = W2/Den;
                float           WH1 = -dx2_2*dx12*(2*dx1+dx12)*RcpDen;
                float           WH2 = dx2*dx1*(dx1*dx12+4*dx2*dx1+dx12*dx2)*RcpDen;
                float           WH3 = -dx1_2*dx12*(2*dx2+dx12)*RcpDen;
#else // IF NOT DEEP_CORRECTION
                float           Den = -4*dx1*dx2;
                float           RcpDen = 1/Den;
                float           WH1 = dx2*dx12*RcpDen;
                float           WH2 = -2*dx1*dx2*RcpDen;
                float           WH3 = dx1*dx12*RcpDen;
#endif // NOT DEEP_CORRECTION

                // Get surface and bottom height map pointers for each grid
                DynamicMatrix<float>* pH1 = &(pG[t]->GetCurrentSurface().m_heightMap);
                DynamicMatrix<float>* pB1 = &(pG[t]->GetBottom().m_heightMap);
                DynamicMatrix<float>* pH2 = &(pG[1-t]->GetCurrentSurface().m_heightMap);
                DynamicMatrix<float>* pB2 = &(pG[1-t]->GetBottom().m_heightMap);

                // Calculate the  horizontal coordinate of the seam line start point
                float xf0 = 
                    ((RegionStartAlong[t]  +0.5f*StepAlong[t])-
                     (RegionStartAlong[1-t]+0.5f*StepAlong[1-t]))/StepAlong[1-t];
                // Compute a step factor between the two cell sizes 
                float xfK = StepAlong[t]/StepAlong[1-t];
                int     xiMax = SizeAlong[1-t]-1;

                // For every pixel in the seam
                for(x=x0i[t];x<=x1i[t];++x)
                {
                    // Initialize variables
                    float   xf = xf0 + x*xfK;
                    int     xi = (int)xf;
                    float   xr = xf-xi;
                    float   DH = 0;

                    // If we are off to the left or below....
                    if(xi<0)
                    {
                        xi=0;
                        xr=0;
                    }

                    // If we are off to the right or above
                    if(xi>=xiMax)
                    {
                        xi=xiMax;
                        xr=0;
                    }
                    // Test the height pointers to ensure that they are in range
                    if( pH1->At(XY(x,y[t][0]))>0.05f &&
                        pH1->At(XY(x,y[t][1]))>0.05f &&
                        pH2->At(XY(xi,  y[t][2]))>0.05f &&
                        pH2->At(XY(xi+1,y[t][2]))>0.05f &&
                        pH2->At(XY(xi,  y[t][3]))>0.05f &&
                        pH2->At(XY(xi+1,y[t][3]))>0.05f)
                    {

                        // h1-h4 are absloute water surface levels in neighbouring cells on a line orthogonal
                        // to the seam line. Calculate corrections values for neighbouring grids on seam line
 
                        float h1 = pH1->At(XY(x,y[t][0]))-pB1->At(XY(x,y[t][0]));
                        float h2 = pH1->At(XY(x,y[t][1]))-pB1->At(XY(x,y[t][1]));
                        float h3 = 
                            (pH2->At(XY(xi,  y[t][2]))-pB2->At(XY(xi,  y[t][2])))*(1-xr)+
                            (pH2->At(XY(xi+1,y[t][2]))-pB2->At(XY(xi+1,y[t][2])))*xr;
                        float h4 = 
                            (pH2->At(XY(xi,  y[t][3]))-pB2->At(XY(xi,  y[t][3])))*(1-xr)+
                            (pH2->At(XY(xi+1,y[t][3]))-pB2->At(XY(xi+1,y[t][3])))*xr;
                        // DH is the correction value
                        DH = WH1*(h2-h1)+WH2*(h3-h2)+WH3*(h4-h3);

                        {
                            // Check the wave front gradient.  v1,v2 are gradient values for the 
                            // water surface in neighbouring cells on a line orthogonal to the seam line

                            float   v1 = fabs(h2-h1)/dx1;
                            float   v2 = fabs(h4-h3)/dx2;
                            float   v = max(v1,v2);
                            const float   vt0 = 0.10f; // 0.1 - 0.3 is the buffer zone
                            const float   vt1 = 0.30f; // 0.1 - 0.3 is the buffer zone
                            // "W" is weight 
                            float   W = 1.0f;
                            // If v is less than the lower buffer zone value, W remains 1.0
                            if(v<vt0) W = 1.0f;
                            // However, if v is greater than the upper buffer zone limit, W is set to zero
                            else if(v>vt1) W = 0.0f;
                            // If v is within the buffer zone, then caclulate the intermediate weight. 't' is normalized v
                            else
                            {
                                float t = (v-vt0)/(vt1-vt0);
                                W = 0.5f*(1+cos(t*3.141592653589793f));
                            }
                            DH *= W;
                        }
                    }
                    Correction[t][CorrectionNum[t]++] = DH;
                }// Move to the next pixel of the horizontal/vertical seam
            }// The next iteration is the calculation for the bottom/right grid
            
            /* ****************************************************************************** *\
                                  Now that we have corrections for the pixels
                                  of the top/left and bottom/right grids, lets
                                  apply them to the top/left and bottom/right 
            \* ****************************************************************************** */

            for(t=0;t<2;++t)
            {
                // The number impacted cells for small cells grid is larger than number of impacted cells for big cells grid
                int k,kN=(t==tSmallStep?16:8); 

#ifndef DEEP_CORRECTION
                kN=1;
#endif // DEEP_CORRECTION

                // For each line
                for(k=0;k<kN;++k)
                {
                    float S = (float)(kN-k)/(float)(kN);
                    int yy = y[t][1] + ((t==0)?-k:+k);

                    // For every pixel in the line
                    for(x=x0i[t];x<=x1i[t];++x)
                    {
                        // Correct the height
                        float   DH = Correction[t][x-x0i[t]];
                        // If the correction is above the threshold and the surface height is above 0.05
                        if(fabs(DH)>0.001f && pG[t]->GetCurrentSurface().m_heightMap.At(XY(x,yy))>0.05f)
                        {
                            // Add the correction
                            pG[t]->GetCurrentSurface().m_heightMap.At(XY(x,yy)) += S*DH;
                            // If the height at the pixel is now less than zero
                            if(pG[t]->GetCurrentSurface().m_heightMap.At(XY(x,yy))<=0.0f)
                            {
                                // Set it to zero for all maps
                                pG[t]->GetCurrentSurface().m_heightMap.At(XY(x,yy))=0;
                                pG[t]->GetCurrentSurface().m_uMap.At(XY(x,yy))=0;
                                pG[t]->GetCurrentSurface().m_vMap.At(XY(x,yy))=0;
                            }
                        }
                    } // Next pixel in the line
                } // Next line
            }

#ifndef DEEP_CORRECTION

            t=tSmallStep;
            {
                // For small cells, apply the correction to the mid points
                DynamicMatrix<float>* pU = &(pG[t]->GetCurrentSurface().m_uMap);
                DynamicMatrix<float>* pV = &(pG[t]->GetCurrentSurface().m_vMap);
                DynamicMatrix<float>* pH = &(pG[t]->GetCurrentSurface().m_heightMap);
                DynamicMatrix<float>* pB = &(pG[t]->GetBottom().m_heightMap);

                int k0 = y[t][0];
                int k1 = y[t][1];
                int k;

                // Swap Y indexes to get an increasing order
                if(k0>k1)
                {
                    int tt=k0;k0=k1;k1=tt;
                }

                // For every grid line (near the seam) do water surface correction
                for(k=k0+1;k<k1;++k)
                {
                    float kt = (float)(k-k0)/(float)(k1-k0);
                    // For every pixel in the line
                    for(x=x0i[t];x<=x1i[t];++x)
                    {
                        // Trace the border to correct H
                        float   H0 = pH->At(XY(x,k0))-pB->At(XY(x,k0));
                        float   H1 = pH->At(XY(x,k1))-pB->At(XY(x,k1));
                        pH->At(XY(x,k)) = H0+(H1-H0)*kt + pB->At(XY(x,k));
                        // If the result is less than zero, set to zero
                        if(pH->At(XY(x,k))<=0)
                        {
                            pH->At(XY(x,k)) = 0;
                            pU->At(XY(x,k)) = 0;
                            pV->At(XY(x,k)) = 0;
                        }
                    }
                }
            }
#endif // NOT DEFINED DEEP_CORRECTION
        }

            /* ****************************************************************************** *\

                                           Calculate border values

            \* ****************************************************************************** */

        for(tt=0;tt<2;++tt)
        {
            // Process big cells first

            // The variable "t" is an index into the destination grid
            int     t = tt?tSmallStep:(1-tSmallStep);

            // Get the maps for both grids
            DynamicMatrix<float>*  pH1 = &(pG[t]->GetCurrentSurface().m_heightMap);
            DynamicMatrix<float>*  pU1 = &(pG[t]->GetCurrentSurface().m_uMap);
            DynamicMatrix<float>*  pV1 = &(pG[t]->GetCurrentSurface().m_vMap);
            DynamicMatrix<float>*  pB1 = &(pG[t]->GetBottom().m_heightMap);
            DynamicMatrix<float>*  pH2 = &(pG[1-t]->GetCurrentSurface().m_heightMap);
            DynamicMatrix<float>*  pU2 = &(pG[1-t]->GetCurrentSurface().m_uMap);
            DynamicMatrix<float>*  pV2 = &(pG[1-t]->GetCurrentSurface().m_vMap);
            DynamicMatrix<float>*  pB2 = &(pG[1-t]->GetBottom().m_heightMap);
            
            // Some working variables
            float   xfK,xf0;
            float   yf;
            int     yi;
            float   yr;
            int     ydst;
            int     xdst;

            // Calculate the horizontal coordinate of the seam line 
            xf0 = ((RegionStartAlong[t]  +0.5f*StepAlong[t])-
                   (RegionStartAlong[1-t]+0.5f*StepAlong[1-t]))/StepAlong[1-t];
            // Calculate a step factor
            xfK = StepAlong[t]/StepAlong[1-t];

            // Calculate the destination line coordinate
            ydst = (t==0)?SizeOrt[t]:-1;
            // Calculate the source line coordinate
            yf = 0.5f*(1+StepOrt[t])/StepOrt[1-t]-1;
            // Second time through, adjust axis
            if(t==1) yf = SizeOrt[1-t]-1-yf;
            
            yi = (int)yf;
            yr = yf-yi;

            // If off to the left or below
            if(yi<0)
            {
                yi=0;
                yr=0;
            }

            // If off to the right or above
            if(yi>=(SizeOrt[1-t]-1))
            {
                yi=(SizeOrt[1-t]-1);
                yr=0;
            }

            // For each pixel in the seam
            for(xdst=x0i[t];xdst<=x1i[t];++xdst)
            {
                float   xf = xf0 + xfK*xdst;
                int     xi = (int)xf;
                float   xr = xf-xi;
                // If off to the left or below
                if(xi<0)
                {
                    xi=0;
                    xr=0;
                }
                // If off to the right or above
                if(xi>=(SizeAlong[1-t]-1))
                {
                    xi=(SizeAlong[1-t]-1);
                    xr=0;
                }
                {
                    // v1, v2  are temporary values for bilinear interpolation 
                    float v1,v2;
                    v1 = 
                        (pH2->At(XY(xi+0,yi+0))-pB2->At(XY(xi+0,yi+0)))*(1-xr)+
                        (pH2->At(XY(xi+1,yi+0))-pB2->At(XY(xi+1,yi+0)))*xr;
                    v2 = 
                        (pH2->At(XY(xi+0,yi+1))-pB2->At(XY(xi+0,yi+1)))*(1-xr)+
                        (pH2->At(XY(xi+1,yi+1))-pB2->At(XY(xi+1,yi+1)))*xr;
                    // Finish the bilinear interpolation
                    pH1->At(XY(xdst,ydst)) = v1*(1-yr)+v2*yr + pB1->At(XY(xdst,ydst));

                    // If the result is less than or equal to zero
                    if(pH1->At(XY(xdst,ydst))<=0)
                    {
                        // Ensure that all values are set to zero
                        pH1->At(XY(xdst,ydst)) = 0;
                        pV1->At(XY(xdst,ydst)) = 0;
                        pU1->At(XY(xdst,ydst)) = 0;
                    }
                    else
                    {
                        float minVel = m_GlobalParams.minNonZeroSpeed;

                        // v1, v2 are temporary values for bilinear interpolation 
                        v1 = pU2->At(XY(xi+0,yi+0))*(1-xr)+
                             pU2->At(XY(xi+1,yi+0))*xr;
                        v2 = pU2->At(XY(xi+0,yi+1))*(1-xr)+
                             pU2->At(XY(xi+1,yi+1))*xr;
                        pU1->At(XY(xdst,ydst)) = v1*(1-yr)+v2*yr;
                        pU1->At(XY(xdst,ydst)) = (pU1->At(XY(xdst,ydst)) >= minVel || -pU1->At(XY(xdst,ydst)) >= minVel) ? pU1->At(XY(xdst,ydst)) : 0.0f;

                        v1 = pV2->At(XY(xi+0,yi+0))*(1-xr)+
                             pV2->At(XY(xi+1,yi+0))*xr;
                        v2 = pV2->At(XY(xi+0,yi+1))*(1-xr)+
                             pV2->At(XY(xi+1,yi+1))*xr;
                        pV1->At(XY(xdst,ydst)) = v1*(1-yr)+v2*yr;
                        pV1->At(XY(xdst,ydst)) = (pV1->At(XY(xdst,ydst)) >= minVel || -pV1->At(XY(xdst,ydst)) >= minVel) ? pV1->At(XY(xdst,ydst)) : 0.0f;
                    }

                    //printf("x=%f,y=%f,H=%f\n",xf,yf,pH1[ydst*H1Step+xdst]);
                }
            }// Next pixel of the horizontal/vertical seam
        }// Next grid
    }
}/* CFlux2DScene::ProcessSeams() */
#undef XY

/**
 * @fn SimulateNoiseWindWaves(float dt, WIND_WAVE_PARAM* pParam)
 *
 * @brief Simulates noise generated wind waves 
 *
 * @param dt - Delta time
 *
 * @param pParam - Wind wave parameters
 */
void  CFlux2DScene::SimulateNoiseWindWaves(float dt, WIND_WAVE_PARAM* pParam)
{
    float   Wx = cos(pParam->direction*3.14f/180.0f);
    float   Wy = sin(pParam->direction*3.14f/180.0f);
    int     i;
    float   Alpha = pParam->desipation; // The dissipation coefficient = 0.01f
    Alpha = dt * Alpha/(1+dt*Alpha);    // Convert from 0-inf to 0-1
    
    // For each grid
    for(i=0;i<GetNumGrids();++i)
    {
        // Get the grid
        CFlux2DGrid* pG = GetGrid(i);

        // Declare and initialize working variables
        int     x,y;
        int     W = pG->GetDomainW();
        int     H = pG->GetDomainH();
        float   K;

        // Compute a temporary cooefficient
        K = 2*dt*pParam->speed/(pG->GetGridStepH()+pG->GetGridStepW());
        K *= K;

        // For every line in the grid
        for(y=0;y<H;++y)
        {
            // Get current self and neighbor H lines 
            float* pCurUp = &pG->m_pWindWavesHCur->At(0,y-1);
            float* pCur = &pG->m_pWindWavesHCur->At(0,y);
            float* pCurDn = &pG->m_pWindWavesHCur->At(0,y+1);
            // Get previous H line
            float* pPrev = &pG->m_pWindWavesHPrev->At(0,y);
            // Set the destination to the previous
            float* pDst = pPrev;
            // Height map for the surface 
            float* pMainH = &pG->GetDestSurface().m_heightMap.At(0,y);
            // Target height map
            float* pTargetH = &pG->m_WindWavesH.At(0,y);

            // For every pixel in the line
            for(x=0;x<W;++x)
            {
                float   NewH;
                // Solve the main wave equation
                NewH = 2 * (1 - K*2) * pCur[x] + K * (pCur[x+1] + pCur[x-1] + pCurUp[x] + pCurDn[x]) - pPrev[x];

                // If we have directed waves then add "wind force" to generate the directed waves
                if(pParam->directed_waves)
                {
                    //  Compute average values
                    float   dUx = (pCur[x+1] - pCur[x-1])/(2*pG->GetGridStepW());
                    float   dUy = (pCurDn[x] - pCurUp[x])/(2*pG->GetGridStepH());
                    // Compute force weight 
                    float   UW = dUx*Wx+dUy*Wy;
                    // If the new weight is greater than zero
                    if(UW>0)
                    {
                        // Add the wind force
                        float FK = UW/(0.01f+UW*UW);
                        NewH -= FK*dt*dt*pParam->force; 
                    }
                }

                // Add viscosity
                NewH = NewH*(1-Alpha) + Alpha*pCur[x]; 

                // Add spring to keep the over water level close to zero
                NewH *= 0.999f;

                // Save the result
                pDst[x] = NewH;
                
                // If the bottom is not constant, check to see if the height is above a threshold
                if(pMainH[x]<0.05f) NewH = 0;

                // Add noise wave to the output buffer
                pTargetH[x] = pMainH[x] + NewH;
            }
        }

        // Swap surfaces
        {   
            DynamicMatrix<float>* pT = pG->m_pWindWavesHPrev;
            pG->m_pWindWavesHPrev = pG->m_pWindWavesHCur;
            pG->m_pWindWavesHCur = pT;
        }
        // Process the borders
        { 
            for(y=0;y<H;++y)
            {
                // Left and right borders
                pG->m_pWindWavesHCur->At(-1,y) = 0; //pG->m_pWindWavesHCur->At(0,y);
                pG->m_pWindWavesHCur->At(W,y) = 0;  //pG->m_pWindWavesHCur->At(W-1,y);
            }
            for(x=-1;x<(W+1);++x)
            {
                // upper and lower borders
                pG->m_pWindWavesHCur->At(x,-1) = 0; //pG->m_pWindWavesHCur->At(x,0);
                pG->m_pWindWavesHCur->At(x,H) = 0;  //pG->m_pWindWavesHCur->At(x,H-1);
            }
        }

        // Add force to keep the waves going
        {
            static float Time = 0;
            static float LastGenTime = -1000;
            static int x;
            static int y;
            float Ampl = pParam->amplitude;
            float Period = 1.0f/pParam->number; 

            // Set the time to the delta
            Time += dt;

            if(Time>LastGenTime+Period)
            {
                // Generate a new drop
                int     x = rand()%W;
                int     y = rand()%H;

                if(pParam->directed_waves)
                {
                    // Compute the delta in space by multiplying delta time by velocity
                    float   ds = dt*pParam->speed;
                    float   t;
                    for(t=-0;t<=0;t++) // Currently this executes just once
                    {
                        float   xc = (float)x + t*Wy; 
                        float   yc = (float)y - t*Wx; 
                        CSamplerBilinear   SCur(xc,yc);
                        CSamplerBilinear   SPrev(xc - Wx*ds,yc - Wy*ds);
                        // If both the current and previous samples are within the grid
                        if( SCur.IsInsideRect(0,0,W-1,H-1) && SPrev.IsInsideRect(0,0,W-1,H-1))
                        {
                            // Add the amplitude
                            SCur.AddValue(pG->m_pWindWavesHCur,Ampl);
                            SPrev.AddValue(pG->m_pWindWavesHPrev,Ampl);
                        }
                    }
                }
                else
                {
                    // Add drop for undirected waves
                    pG->m_pWindWavesHCur->At(x,y) += Ampl;
                }
                LastGenTime = Time;
            }
        }
    }
};


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
CFlux2DGrid* CFlux2DScene::FindGrid(float x, float y, CSamplerBilinear* pSampler) 
{
    CFlux2DGrid  *pFlux = NULL;
    int numGrids = GetNumGrids();

    // For each grid
    for (int i = 0; i < numGrids; i++)
    {
        // Validate that the coordinates are within the grid's region
        CRegion  &r = GetRegion(i);
        if (r.x1 <= x && x < r.x2 && r.y1 <= y && y < r.y2)
        {
            // If they are, get the grid
            pFlux = GetGrid(i);
            // Check to see if we have a sampler
            if(pSampler)
            {
                // If we do, initialize the sampler
                pSampler->Init(
                    (x - r.x1) * pFlux->GetRcpGridStepW(), 
                    (y - r.y1) * pFlux->GetRcpGridStepH());
            }
            break;
        }
    }
    return pFlux;
}


