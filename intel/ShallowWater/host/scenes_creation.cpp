/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file scenes_creation.cpp
 *
 * @brief Host-side class objects for creatung water physics simulation scenes 
 *
 * @note Not used for Collada scenes.
 */

#include <string.h>
#include "Flux/CFlux2DScene.h"
#include "Flux/CFlux2DSceneDef.h"
#include "Flux/actions/CFlux2DBCCopy.h"
#include "Flux/actions/CFlux2DBC_Gen.h"
#include "Flux/actions/CFlux2DBCReflectLine.h"
#include "scenes_creation.h"
#include "Util/memory.h"
#include <math.h>
#include <float.h>


namespace
{
    /**
     * @class ProfileCurve
     *
     * @brief 
     */
    class ProfileCurve
    {
    public:

        /**
         * @fn ProfileCurve( )
         *
         * @brief Class constructor
         */
        ProfileCurve( ) 
        {
        
        }

        /**
         * @fn Read(wchar_t const *fileName)
         *
         * @brief Reads the profile curve from an IO stream
         *
         * @param fileName - The IO stream
         *
         * @return - TRUE if successful, FALSE otherwise
         */
        bool Read(wchar_t const *fileName)
        {
            // Open the file and validate
            FILE *pFile;
            if (_wfopen_s(&pFile, fileName, L"rt") != 0)
            {
                return false;
            }

            // Initizliae min and max points with opposing values
            m_min.x = m_min.y = FLT_MAX;
            m_max.x = m_max.y = -FLT_MAX;
            // Prepare to enter the loop
            int true_val = 1;
            do
            {
                Point p;
                // Read a point from the file and validate
                int err = fscanf_s(pFile, "%f,%f\n", &p.x, &p.y);
                // If we reach the end of the file, exit the loop
                if (err == EOF)
                {
                    break;
                }

                // Insert the point at the end of the vector buffer
                m_curve.push_back(p);
                // Re-set minimum and maximum values based on the new point's values
                m_min.x = __min(m_min.x, p.x);
                m_max.x = __max(m_max.x, p.x);
                m_min.y = __min(m_min.y, p.y);
                m_max.y = __max(m_max.y, p.y);
            } 
            while (true_val);

            fclose(pFile);

            // Adjust for the minimum value of X to normalize

            // For every point in the curve
            for (int i = 0; i < int(m_curve.size()); i++)
            {
                // Subtract the minimum value
                m_curve[i].x -= m_min.x;
            }
            // Decrement the maximum value by the minimum value
            m_max.x -= m_min.x;
            // And then set the minimum to zero
            m_min.x = 0.0f;

            return true;
        }


        /**
         * @fn Rescale(float minH, float maxH, float length)
         *
         * @brief Rescales the curve with new parameters
         *
         * @param minH - The minimum height
         *
         * @param maxH - The maximum height
         *
         * @param length - The X extent 
         */
        void Rescale(float minH, float maxH, float length)
        {
            // Declare and initialize scaling factors
            float scaleH = (maxH - minH) / (m_max.y - m_min.y);
            float scaleL = length / m_max.x;
            // Declare and initialize an offset factor
            float offsH  = minH - scaleH * m_min.y;
            // For every point in the curve
            for (int i = 0; i < int(m_curve.size()); i++)
            {
                // Scale the point
                m_curve[i].x *= scaleL;
                // Add offset to the Y coordinate as needed
                m_curve[i].y = m_curve[i].y * scaleH + offsH;
            }
        }


        /**
         * @fn StartSampling()
         *
         * @brief Resets index and offset values to zero to start sampling
         */
        void StartSampling()
        {
            m_curIndex = 0;
            m_curXOffs = 0;
        }


        /**
         * @fn GetLength()
         *
         * @brief Determines the length of the curve
         *
         * @return - The maximum X value
         */
        float GetLength() const 
        { 
            return m_max.x; 
        }


        /**
         * @fn SampleAndShift2Next(float shift)
         *
         * @brief Samples the curve and shifts the index
         *
         * @param shift - The number of X offsets to shift
         *
         * @return - The Y value for the current index
         */
        float SampleAndShift2Next(float shift)
        {
            // If the current index is for the last point in the curve
            if (m_curIndex == (int)(m_curve.size() - 1))
            {
                // Return the Y value of the current point
                return m_curve[m_curIndex].y;
            }

            // Compute the delta X value and validate
            float dx = m_curve[m_curIndex+1].x - m_curve[m_curIndex].x;
            assert(dx > 0.0f);

            // Compute Y as the current point Y plus the delta Y to the next point multiplied by the
            // number of X offsets divided by the delta X
            float y = m_curve[m_curIndex].y + (m_curve[m_curIndex+1].y - m_curve[m_curIndex].y) * m_curXOffs / dx;

            // Now, do the shift .....

            m_curXOffs += shift;
            // While there are more X offsets the change in X
            while (m_curXOffs > dx)
            {
                // Decrement the offsets by the delta X
                m_curXOffs -= dx;
                // Increment the current index
                m_curIndex++;
                // Continue until we reach the last point in the curve
                if (m_curIndex == (int)(m_curve.size() - 1))
                {
                    break;
                }
                // Get the next delta X value
                dx = m_curve[m_curIndex+1].x - m_curve[m_curIndex].x;
            }

            return y;
        }

    private:

        /**
         * @struct Point
         *
         * @brief A 2D point
         */
        struct Point
        {
            float x;
            float y;
        };

        /**
         * @var m_curve
         *
         * @brief Vector of points in the curve
         */
        std::vector<Point> m_curve;

        /**
         * @var m_min
         *
         * @brief Minimum value
         */
        Point m_min;

        /**
         * @var m_max
         *
         * @brief Maximum value
         */
        Point m_max;

        /**
         * @var m_curIndex
         *
         * @brief Current index into the point vector
         */
        int   m_curIndex;

        /**
         * @var m_curXOffs
         *
         * @brief The number of current X offsets
         */
        float m_curXOffs;
    };
}



/**
 * @fn LoadScnDefFromBitmap(CSceneDef const &srcDef, CFlux2DSceneDef &def)
 *
 * @brief Loads the specified scene from a bitmap file
 *
 * @param srcDef - The source scene definition
 *
 * @param def - Output - The Flux 2D scene definition
 *
 * @return - TRUE if successful, FALSE otherwise
 */
bool LoadScnDefFromBitmap(CSceneDef const &srcDef, CFlux2DSceneDef &def)
{
    // Load the scene from the file and validate
    HBITMAP hBMP = NULL;
    hBMP = ( HBITMAP )::LoadImage( 0, srcDef.fileName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION );  
    if ( hBMP == NULL )
    {
        return false;
    }

    // Get the bitmap
    BITMAP bmp;
    GetObject( hBMP, sizeof(bmp), &bmp );   
    HDC hdc  = GetDC(NULL);
    HDC memDC = CreateCompatibleDC( hdc );
    if(memDC == NULL)
    {
        return false;
    }

    // Put the image to a compatible DC.
    HBITMAP hOldBitmap  = ( HBITMAP )SelectObject( memDC, hBMP );  

    int const width = bmp.bmWidth;
    int const height = bmp.bmHeight;

    // Initialize the destination scene
    def.lenWidth  = srcDef.lengthWidth;
    def.lenHeight = srcDef.lengthHeight;
    def.resWidth  = bmp.bmWidth;
    def.resHeight = bmp.bmHeight;
    def.heightMap.SetSize(width,height);
    // For every line
    for (int i = 0, idx = 0; i < height; i++)
    {
        // For every pixel in the line
        for (int j = 0; j < width; j++, idx++)
        {
            // Get it and store it
            int pix = GetPixel( memDC, j, i ) & 0xFF;
            def.heightMap.At(j,i) = -(srcDef.minH + (srcDef.maxH - srcDef.minH) * float(pix) / 255.f);
        }
    }

    // Clean up
    SelectObject( memDC, hOldBitmap );
    DeleteDC( memDC );   
    ReleaseDC( NULL, hdc );

    return true;
}


/**
 * @fn LoadScnDefFromCSVProfile( srcDef, resW, resH, def, bRescaleValues )
 *
 * @brief Loads a scene from a CSV file.
 *
 * @param srcDef - The source scene definition
 *
 * @param resW - Resource width
 *
 * @param resH - Resource height
 *
 * @param def - Output - The loaded scene
 *
 * @param bRescaleValues - Indicates whether or not to rescale the values
 *
 * @return - TRUE if successful, FALSE otherwise
 */
bool LoadScnDefFromCSVProfile( CSceneDef const &srcDef, 
    int resW, 
    int resH, 
    CFlux2DSceneDef &def, 
    bool bRescaleValues = false )
{
    // First, read the curve from the file
    ProfileCurve prof;
    if (!prof.Read(srcDef.fileName))
    {
        return false;
    }

    // Rescale if required
    if (bRescaleValues)
    {
        prof.Rescale(srcDef.minH, srcDef.maxH, srcDef.lengthWidth);
    }

    // Initialize the output scene
    def.lenWidth  = prof.GetLength();
    def.lenHeight = srcDef.lengthHeight;
    def.resWidth  = resW;
    def.resHeight = resH;
    def.heightMap.SetSize(resW,resH);

    // Compute the step by dividing the source length by the resource width
    float step = prof.GetLength() / float(resW - 1);
    // Reset counters
    prof.StartSampling();

    // For every column in the resource
    for (int x = 0; x < resW; x++)
    {
        int idx = x;
        float height = -prof.SampleAndShift2Next(step);
        // For every line in the resource
        for (int y = 0; y < resH; y++, idx += resW)
        {
            def.heightMap.At(x,y) = height;
        }
    }

    return true;
}


/**
 * @fn IsFileNameCSV(wchar_t const *name)
 *
 * @brief Checks CSV file extension.
 *
 * @param name - The filename to check
 *
 * @return - TRUE if the file extension is ".csv", FALSE otherwise
 */
bool IsFileNameCSV(wchar_t const *name)
{
    size_t const len = wcslen(name);
    if (len <= 4)
    {
        return false;
    }
    return _wcsicmp(name + len - 4, L".csv") == 0;
}


/**
 * @fn OneGridSceneCreation(CSceneDef const &def, int resW, int resH)
 *
 * @brief Implements the creation of a single grid (not multi-grid) scene.
 *
 * @param def - The scene definition
 *
 * @param resW - Width of the scene
 *
 * @param resH - Height of the scene
 *
 * @return - Pointer to the created scene, NULL to indicate failure
 */
CFlux2DScene* OneGridSceneCreation(CSceneDef const &def, int resW, int resH)
{
    CFlux2DSceneDef scnDef;
    // Validate the filename
    if (!IsFileNameCSV(def.fileName))
    {
        // Load the scene from the file
        if (!LoadScnDefFromBitmap(def, scnDef))
        {
            return NULL;
        }
    }
    else // Load from the profile
    {
        if (!LoadScnDefFromCSVProfile(def, resW, resH, scnDef))
        {
            return NULL;
        }
    }

    // Allocate the scene
    CFlux2DScene* pScn = new (UseOwnMem) CFlux2DScene(1);
    // Add the single grid
    pScn->AddGrid(scnDef, resW, resH, CRegion(0.0f, 0.0f, scnDef.lenWidth, scnDef.lenHeight));
    pScn->CalculateBoundingRegion();

    return pScn;
}


/**
 * @fn OneGridSceneCreation(int resW, int resH, float lenW, float lenH)
 *
 * @brief Implements the creation of a single grid (not multi-grid) scene.
 *
 * @param resW - Width of the scene
 *
 * @param resH - Height of the scene
 *
 * @param lenW - Width of the region
 *
 * @param lenH - Height of the region
 *
 * @return - Pointer to the created scene
 */
CFlux2DScene* OneGridSceneCreation(int resW, int resH, float lenW, float lenH)
{
    CFlux2DScene* pScn = new (UseOwnMem) CFlux2DScene(1);
    pScn->AddGrid(resW, resH, CRegion(0.0f, 0.0f, lenW, lenH));
    return pScn;
}


/**
 * @fn SceneCreateBasicParametrized( def, resW, resH, ampl, omega, lengthAlongV, bReflectingEnd)
 *
 * @brief Creates a basic parameterized scene with a single grid
 *
 * @param def - The scene definition
 *
 * @param resW - Width of the scene
 *
 * @param resH - Height of the scene
 *
 * @param ampl - Amplitude of the waves
 *
 * @param omega - Wave frequency
 *
 * @param lengthAlongV - The wave length along the left border
 *
 * @param bReflectingEnd - Indicates whether or not the right border is reflected
 *
 * @return Pointer to the created scene
 */
CFlux2DScene* SceneCreateBasicParametrized( CSceneDef const &def, 
    int resW, 
    int resH, 
    float ampl, 
    float omega, 
    float lengthAlongV, 
    bool bReflectingEnd )
{
    // Create and validate the scene
    CFlux2DScene* pScn = OneGridSceneCreation(def, resW, resH);
    if (pScn == NULL)
    {
        return pScn;
    }

    // Reset the water surface state
    CFlux2DGrid *pF = pScn->GetGrid();
    pF->ResetToZeroState();

    // -----------------------------------------------
    // Add border conditions

    // Add sine wave generation
    pF->AddAction(new (UseOwnMem) CFlux2DBCSineWaveGenVerticalLeft(ampl, omega, lengthAlongV));

    // Add upper & lower borders
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::TOP));
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::BOTTOM));
    // Add right border
    if (bReflectingEnd)
    {
        pF->AddAction(new (UseOwnMem) CFlux2DBCWaterWallVerticalRight());
    }
    else
    {
        pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::RIGHT));

    }

    return pScn;
}


/**
 * @fn SceneCreatePoolParametrized( CSceneDef const &def, int resW, int resH )
 *
 * @brief Creates a basic scene with free boundary conditions (parametrized).
 *
 * @param def - The scene definition
 *
 * @param resW - The resource width
 *
 * @param resH - The resource height
 *
 * @return Pointer to the created scene
 */
CFlux2DScene* SceneCreatePoolParametrized(CSceneDef const &def, int resW, int resH)
{
    // Create and validate the scene
    CFlux2DScene* pScn = OneGridSceneCreation(def, resW, resH);
    if (pScn == NULL)
    {
        return pScn;
    }

    // Reset the water surface state
    CFlux2DGrid *pF = pScn->GetGrid();
    pF->ResetToZeroState();

    // Add borders
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::TOP));
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::BOTTOM));
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::LEFT));
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::RIGHT));

    return pScn;
}


/**
 * @fn SceneCreateWaterRisingParametrized( def, resW, resH, startH, endH, time )
 *
 * @brief Creates a basic scene with water rising boundary conditions (parametrized).
 *
 * @note Used for rivers and lakes scenes. 
 *
 * @param def - The scene definition
 *
 * @param resW - The resource width
 *
 * @param resH - The resource height
 *
 * @param startH - The starting water height
 *
 * @param endH - The ending water height
 *
 * @param time - The time for the water to rise
 *
 * @return - Pointer to the created scene
 */
CFlux2DScene* SceneCreateWaterRisingParametrized(CSceneDef const &def, int resW, int resH, float startH, float endH, float time)
{
    // Create the scene
    CFlux2DScene* pScn = OneGridSceneCreation(def, resW, resH);
    if (pScn == NULL)
    {
        return pScn;
    }

    // Reset the water surface state
    CFlux2DGrid *pF = pScn->GetGrid();
    pF->ResetToDryState();

    // -----------------------------------------------
    // Add border conditions

    pF->AddAction(new (UseOwnMem) CFlux2DBCWaterRiseVerticalLeft(startH, endH, time));
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::TOP));
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::BOTTOM));
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::RIGHT));

    return pScn;
}


/**
 * @fn SceneCreateExperiment( int resW, int resH )
 *
 * @brief Creates an experimental scene
 *
 * @param resW - The resource width
 *
 * @param resH - The resource height
 *
 * @return Pointer to the created scene
 */
CFlux2DScene* SceneCreateExperiment(int resW, int resH)
{
    float const lenW = 1000.0f;
    float const lenH = 9.0f;

    // Create and validate the scene
    CFlux2DScene* pScn = OneGridSceneCreation(resW, resH, lenW, lenH);
    if (pScn == NULL)
    {
        return pScn;
    }

    // Reset the water surface state
    CFlux2DGrid *pF = pScn->GetGrid();
    pF->SetBottomConstant(10.0f, lenW, lenH);
    pF->ResetToZeroState();

    // -----------------------------------------------
    // Add border conditions

    // Add sine wave generation
    pF->AddAction(new (UseOwnMem) CFlux2DBCSineWaveGenVerticalLeft(1.0f, 0.5f, 0.0f));
    // Our constants
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::TOP));
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::BOTTOM));
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::RIGHT));
    return pScn;
}


/**
 * @fn SceneCreateMGridExperiment3GFlip()
 *
 * @brief Creates a multi-grid experimental scene
 *
 * @return Pointer to the created scene
 */
CFlux2DScene* SceneCreateMGridExperiment3GFlip()
{
    CFlux2DScene* pScene = NULL;

    // Create the test definition
    CSceneDef       testDef( L"data/hm_cape.bmp"  , -5.0f, 5.0f, 256.0f, 256.0f );
    // Load the test scene definition
    CFlux2DSceneDef scnDef;
    if (!LoadScnDefFromBitmap(testDef, scnDef))
    {
        return NULL;
    }
    
    // Create the scene
    pScene = new (UseOwnMem) CFlux2DScene(3);

    // Add grids for left, top, and bottom
    int X0=64;
    int leftGrid = pScene->AddGrid(scnDef, X0, 256, CRegion(0.0f, 0.0f, (float)X0, 256.0f));
    int rightTopGrid = pScene->AddGrid(scnDef, (256-X0)/2, (128)/2, CRegion((float)X0, 0.0f, 256.0f, 128.0f));
    int rightBtmGrid = pScene->AddGrid(scnDef, 256-X0, 128, CRegion((float)X0, 128.0f, 256.0f, 256.0f));
    // Calculate the bounding box
    pScene->CalculateBoundingRegion();
    CSeamData* pSD;
    
    // Add a horizontal seam 
    pSD = &(pScene->m_SeamArray.Append());
    pSD->GridTop = rightTopGrid;
    pSD->GridBottom = rightBtmGrid;
    pSD->GridLeft = -1;
    pSD->GridRight = -1;
    // Add a vertical  seam
    pSD = &(pScene->m_SeamArray.Append());
    pSD->GridTop = -1;
    pSD->GridBottom = -1;
    pSD->GridLeft = leftGrid;
    pSD->GridRight = rightTopGrid;
    // Add second vertical seam
    pSD = &(pScene->m_SeamArray.Append());
    pSD->GridTop = -1;
    pSD->GridBottom = -1;
    pSD->GridLeft = leftGrid;
    pSD->GridRight = rightBtmGrid;

    // Add border conditions & initialize

    // To the top-right part
    CFlux2DGrid *pF = pScene->GetGrid(rightTopGrid);
    pF->ResetToZeroState();
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::TOP));
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::RIGHT));
    
    // To the bottom-right part
    pF = pScene->GetGrid(rightBtmGrid);
    pF->ResetToZeroState();
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::BOTTOM));
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::RIGHT));
    
    // To the left part
    pF = pScene->GetGrid(leftGrid);
    pF->ResetToZeroState();
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::TOP));
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::BOTTOM));
    pF->AddAction(new (UseOwnMem) CFlux2DBCSineWaveGenVerticalLeft(2.5f, 1.7f, 0.0f));
    //CFlux2DAddBC_CopyRt(*pF); (DEPRECATED)

    return pScene;
}


/**
 * @fn SceneAddPoly(CFlux2DGrid *pF, Point2D32f* pV, int Num, float ShiftToFillX, float ShiftToFillY)
 *
 * @brief Routine to create reflective boundary conditions in scenes with linear boundary conditions
 *        according to an input polygon.
 *
 * @param pF - The grid to which to add the 
 *
 * @param pV - Buffer of points
 *
 * @param Num - Number of points
 *
 * @param ShiftToFillX - 
 *
 * @param ShiftToFillY - 
 */
static void SceneAddPoly(CFlux2DGrid *pF, Point2D32f* pV, int Num, float ShiftToFillX, float ShiftToFillY)
{
    int i;
    // Get the last point in the buffer
    Point2D32f  pp = pV[Num-1];
    // Declare workiing variables
    Point2D32f  pc;
    Point2D32i  SeedPoint;
    Point2D32i  FirstPoint;
    Point2D32i  CenterAcc;
    int         CenterCount = 0;

    // Initialize the center action location to zero
    CenterAcc.x = CenterAcc.y = 0;

    // For every point
    for (i=0;i<Num; ++i)
    {
        // Working variables
        int j;
        CFlux2DBCReflectLine *pLine;

        // Get the point
        pc = pV[i];
        
        // Create a line from the current point to the last point used
        pLine = new (UseOwnMem) CFlux2DBCReflectLine(
            pF, 
            pp.x, pp.y, 
            pc.x, pc.y, 
            CFlux2DBCReflectLine::Joint,CFlux2DBCReflectLine::Joint, 
            false);
        // Add it
        pF->AddAction((CFlux2DAction*)pLine);

        // If this is the second point in the buffer, store the first point
        if(i==1)
        {
            FirstPoint = pLine->m_InnerBorder[0];
        }

        // For every pixel in the line
        for(j=0;j<pLine->m_InnerBorder.Size();++j)
        {
            // Set the borders flag on line pixels
            Point2D32i  p = pLine->m_InnerBorder[j];
            // Then draw the border pixel
            pF->m_InnerMaskData.At(p) = CFlux2DGrid::BORDER;
            
            // Increment the center action and count
            CenterAcc.x += p.x;
            CenterAcc.y += p.y;
            CenterCount++;
        }
        
        // Set the last point to the current point
        pp = pc;
    } // Move to the next point

    // If we added any actions
    if(CenterCount>0)
    {
        // Fill the mask from the center to drawn borders
        SeedPoint.x = CenterAcc.x / CenterCount;
        SeedPoint.y = CenterAcc.y / CenterCount;
    }

    {
        // Try to find the seed point by direction from the first point
        float L = 0.5f*sqrtf(ShiftToFillX*ShiftToFillX+ShiftToFillY*ShiftToFillY);
        if(fabs(L)>FLT_MIN)
        {
            float   dx = ShiftToFillX / L;
            float   dy = ShiftToFillY / L;
            int     k;
            for(k=0;k<1024;++k)
            {
                int x = (int)(FirstPoint.x + dx*k);
                int y = (int)(FirstPoint.y + dy*k);
                // Test for inclusion within the domain, if so, we have a seedpoint
                if(x>=0 && x<pF->GetDomainW() && y>=0 && y<pF->GetDomainH() &&
                    pF->m_InnerMaskData.At(x,y) == CFlux2DGrid::OUTER)
                {
                    SeedPoint.x = x;
                    SeedPoint.y = y;
                    break;
                }
            }
        }
    }

    pF->FillInnerMask(SeedPoint);
}



/**
 * @fn SceneCreateLineReflectionChannel( def, resW, resH, ampl, omega, lengthAlongV, sinWave )
 *
 * @brief Creates a scene with linear boundary conditions (channels).
 *
 * @param def - The scene definition
 *
 * @param resW - Width of the scene
 *
 * @param resH - Height of the scene
 *
 * @param ampl - Amplitude of the waves
 *
 * @param omega - Wave frequency
 *
 * @param lengthAlongV - The wave length along the left border
 *
 * @param sinWave - Indicates whether or not to use sine wave generation
 *
 * @return Pointer to the created scene
 */
CFlux2DScene* SceneCreateLineReflectionChannel( CSceneDef const &def, 
    int resW, 
    int resH, 
    float ampl, 
    float omega, 
    float lengthAlongV, 
    bool sinWave )
{
    // Create the scene
    CFlux2DScene* pScn = OneGridSceneCreation(def, resW, resH);
    if(pScn == NULL) return pScn;

    // Reset the water surface state
    CFlux2DGrid *pF = pScn->GetGrid();
    pF->ResetToZeroState();


    // -----------------------------------------------
    // Add border conditions

    // Add sine wave generation
    if (sinWave) 
    {
        pF->AddAction(new (UseOwnMem) CFlux2DBCSineWaveGenVerticalLeft(ampl, omega, lengthAlongV));
    } 
    else 
    {
        // Add left border
        pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::LEFT));
    }
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::TOP));
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::BOTTOM));
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::RIGHT));

    Point2D32f pV[30];
    
    {   // Initialize inner mask data
        char InitVal = CFlux2DGrid::OUTER;
        pF->m_InnerMaskData.Set(InitVal);
        pF->m_InnerMaskFlag = 1;
    }
    // Manually adjusted boundary conditions for the demo scene
    pV[0] = Point2D32f(678/742.0f,418/1001.0f);
    pV[1] = Point2D32f(486/742.0f,419/1001.0f);
    pV[2] = Point2D32f(417/742.0f,466/1001.0f);
    pV[3] = Point2D32f(346/742.0f,467/1001.0f);
    pV[4] = Point2D32f(309/742.0f,443/1001.0f);
    pV[5] = Point2D32f(273/742.0f,395/1001.0f);
    pV[6] = Point2D32f(223/742.0f,254/1001.0f);
    pV[7] = Point2D32f(222/742.0f,207/1001.0f);
    pV[8] = Point2D32f(206/742.0f,208/1001.0f);
    pV[9] = Point2D32f(204/742.0f,120/1001.0f);
    pV[10] = Point2D32f(304/742.0f,120/1001.0f);
    
    // Draw all lines and fill the inner area by flags
    SceneAddPoly(pF,pV, 11,0,0);

    pV[0] = Point2D32f(90/742.0f,750/1001.0f);
    pV[1] = Point2D32f(82/742.0f,702/1001.0f);
    pV[2] = Point2D32f(62/742.0f,654/1001.0f);
    pV[3] = Point2D32f(62/742.0f,610/1001.0f);
    pV[4] = Point2D32f(167/742.0f,465/1001.0f);
    pV[5] = Point2D32f(175/742.0f,440/1001.0f);
    pV[6] = Point2D32f(224/742.0f,441/1001.0f);
    pV[7] = Point2D32f(257/742.0f,464/1001.0f);
    pV[8] = Point2D32f(295/742.0f,512/1001.0f);
    pV[9] = Point2D32f(328/742.0f,535/1001.0f);
    pV[10] = Point2D32f(345/742.0f,539/1001.0f);
    pV[11] = Point2D32f(348/742.0f,559/1001.0f);
    pV[12] = Point2D32f(417/742.0f,557/1001.0f);
    pV[13] = Point2D32f(451/742.0f,534/1001.0f);
    pV[14] = Point2D32f(488/742.0f,514/1001.0f);
    pV[15] = Point2D32f(486/742.0f,843/1001.0f);
    SceneAddPoly(pF,pV, 16,0,0);

    pV[0] = Point2D32f(165/742.0f,358/1001.0f);
    pV[1] = Point2D32f(186/742.0f,330/1001.0f);
    pV[2] = Point2D32f(207/742.0f,359/1001.0f);
    pV[3] = Point2D32f(184/742.0f,386/1001.0f);
    SceneAddPoly(pF,pV, 4,1,0);
    return pScn;
}


/**
 * @fn SceneCreateLineReflectionDemo( def, resW, resH, ampl, omega, lengthAlongV, sinWave )
 *
 * @brief Creates a scene with linear boundary conditions (channels demo).
 *
 * @param def - The scene definition
 *
 * @param resW - Width of the scene
 *
 * @param resH - Height of the scene
 *
 * @param ampl - Amplitude of the waves
 *
 * @param omega - Wave frequency
 *
 * @param lengthAlongV - The wave length along the left border
 *
 * @param sinWave - Indicates whether or not to use sine wave generation
 *
 * @return Pointer to the created scene
 */
CFlux2DScene* SceneCreateLineReflectionDemo(CSceneDef const &def, 
    int resW, 
    int resH, 
    float ampl, 
    float omega, 
    float lengthAlongV, 
    bool sinWave )
{
    // Create the scene
    CFlux2DScene* pScn = OneGridSceneCreation(def, resW, resH);
    if(pScn == NULL) return pScn;

    // Reset the water surface state
    CFlux2DGrid *pF = pScn->GetGrid();
    pF->ResetToZeroState();


    // -----------------------------------------------
    // Add border conditions

    // Add sine wave generation
    if (sinWave) 
    {
        pF->AddAction(new (UseOwnMem) CFlux2DBCSineWaveGenVerticalLeft(ampl, omega, lengthAlongV));
    } 
    else 
    {
        // Add left border
        pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::LEFT));
    }
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::TOP));
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::BOTTOM));
    pF->AddAction(new (UseOwnMem) CFlux2DBCCopy(CFlux2DBCCopy::RIGHT));

    Point2D32f pV[30];


    pF->m_InnerMaskData.Set(CFlux2DGrid::OUTER);
    pF->m_InnerMaskFlag = 1;
    
    // Manually adjusted boundary conditions for the demo scene
    pV[0] = Point2D32f(0.2f,1-0.0f);
    pV[1] = Point2D32f(0.25f,1-0.4f);
    pV[2] = Point2D32f(0.4f,1-0.5f);
    pV[3] = Point2D32f(0.6f,1-0.3f);
    pV[4] = Point2D32f(0.7f,1-0.1f);
    pV[5] = Point2D32f(0.8f,1-0.3f);
    pV[6] = Point2D32f(0.9f,1-0.1f);
    pV[7] = Point2D32f(0.95f,1-0.1f);
        
    pV[8] = Point2D32f(0.95f,1-0.9f);
    pV[9] = Point2D32f(0.9f,1-0.9f);
    pV[10] = Point2D32f(0.8f,1-0.7f);
    pV[11] = Point2D32f(0.7f,1-0.9f);
    pV[12] = Point2D32f(0.6f,1-0.7f);
    pV[13] = Point2D32f(0.45f,1-0.75f);
    pV[14] = Point2D32f(0.3f,1-0.6f);
    pV[15] = Point2D32f(0.2f,1-1.0f);

    pV[16] = Point2D32f(1.0f,1-1.0f);
    pV[17] = Point2D32f(1.0f,1-0.0f);

    // Draw all lines and fill the inner area by flags
    SceneAddPoly(pF,pV, 18,1,-1);
    pF->RaiseBottomOnMask(10);

    return pScn;
}

