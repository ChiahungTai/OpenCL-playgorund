/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFile2DBCReflectLine.cpp
 *
 * @brief Contains piece-wise linear boundary conditions support (BC) implementation. 
 */

#include <vector>
#include <utility>
#include <list>
#include <limits>

#include "CFlux2DBCReflectLine.h"
#include "math.h"

#include "physics_3D_fluid_sim_common.h"

static const float s_zeroTolerance = 0.00001f;
static const float s_plusInf = 10e8;
static const float s_minusInf = -s_plusInf;

/**
 * @fn rasterize()
 *
 * @brief - Used to "Draw" a line for the boundary condition
 *
 * @param l - The line to draw
 *
 * @param Width - Width of the line
 *
 * @param - Height of the line
 *
 * @param pRes - Output - rasterized line
 */
static void rasterize(CFlux2DBCReflectLine::Line l, 
    int Width, 
    int Height, 
    DynamicArray<Point2D32i>* pRes )
{
    // Clear the output buffer
    pRes->Clear();

    // Initialize working variables
    float   lx = fabs(l.x0-l.x1)*Width;
    float   ly = fabs(l.y0-l.y1)*Height;
    int     steps = (int)sqrt(lx*lx+ly*ly)+2;

    // Validate "steps"
    assert(steps > 0);
    if (steps < 0) return;

    pRes->SetCapacity(steps);

    // Declare a point to store the previous working point data
    Point2D32i Prev;

    // For every step
    for (int i = 0; i < steps; ++i) 
    {
        Point2D32i  n;	
        int         FlagAdd = 1;
        // For each fractional part of a step
        float       t = (float)i/(float)(steps-1);
        // Compute X and Y based on the step, ensure no round-off error by adding 0.5f
        n.x = (int)((l.x0 * (1-t) + l.x1*t)*(Width-1)+0.5f);
        n.y = (int)((l.y0 * (1-t) + l.y1*t)*(Height-1)+0.5f);

        // If both X and Y are within range
        if( n.x>=0 && n.x < Width &&
            n.y>=0 && n.y < Height) 
        {
            // If the point is the same as the previous point and
            // if the previous point has already been added
            if(pRes->Size()>0 && Prev.x == n.x && Prev.y == n.y)
            {
                // There is no need to add the new point
                FlagAdd = 0;
            }
        }
        else // One of the two variables is out of range, do not add this point
        {
            FlagAdd = 0;
        }

        // If we have a valid new point, add it
        if(FlagAdd)
        {
            pRes->Append(n);
            Prev = n;
        }
    }// Now go get the next point
}

/**\
 * @fn getLine(float x1, float y1, float x2, float y2)
 *
 * @brief Constructs a CFlux2DBCReflectLine from the input coordinates
 *
 * @param x1 - The first X coordinate
 *
 * @param y1 - The first Y coordinate
 *
 * @param x2 - The second X coordinate
 *
 * @param y2 - The second Y coordinate
 *
 * @return - A pointer to a CFlux2DBCReflectLine
 */

CFlux2DBCReflectLine::Line getLine(float x1, float y1, float x2, float y2)
{
    CFlux2DBCReflectLine::Line l;

    // Initialize the line with the input parameters
    l.x0 = x1;
    l.y0 = y1;
    l.x1 = x2;
    l.y1 = y2;

    // Compute the delta X and Y values
    l.vX = x2 - x1;
    l.vY = y2 - y1;

    // Compute the normal
    float norm = sqrt(l.vX * l.vX + l.vY * l.vY);

    assert(norm > s_zeroTolerance);

    // Adjust if the normal is greater than the allowed tolerance
    if (norm > s_zeroTolerance) 
    {
        l.vX /= norm;
        l.vY /= norm;
    }

    // Return the line
    return l;
}

/**
 * @fn AddTriangle(n, dx1, dy1, dx2, dy2, t)
 *
 * @brief Adds a triangle to the primitives list
 *
 * @param n - Point on the triangle
 *
 * @param dx1 - Delta X to the second point of the triangle
 *
 * @param dy1 - Delta Y to the second point of the triangle
 *
 * @param dx2 - Delta X to the third point of the triangle
 *
 * @param dy2 - Delta Y to the third point of the triangle
 *
 * @param t - The type of the primitive
 */
void CFlux2DBCReflectLine::AddTriangle(Point2D32i const &n, int dx1, int dy1, int dx2, int dy2, PrimitiveType t) 
{
    Primitive   p;

    // Store the type and compute the additional verticies of the triangle
    p.type = t;
    // The first computed vertice is the start point of the triangle plus the deltas to the second point
    p.n0 = Point2D32i(n.x+dx1,n.y+dy1);

    // The second computed vertice is the start point of the triangle plus the deltas to the third point
    p.n1 = Point2D32i(n.x+dx2,n.y+dy2);

    // We know the type of triangle and we have the coordinates for the reflective line
    // so we can always compute the original point. We do not need any other points
    // in the primitive and so we initialize them to zero to be on the safe side
    p.n2 = Point2D32i(0,0);
    p.n3 = Point2D32i(0,0);

    // We need to ensure that both computed points are within the domain range
    if (p.n0.x < 0 || p.n0.x >= m_grid->GetDomainW() || 
        p.n0.y < 0 || p.n0.y >= m_grid->GetDomainH() || 
        p.n1.x < 0 || p.n1.x >= m_grid->GetDomainW() || 
        p.n1.y < 0 || p.n1.y >= m_grid->GetDomainH() ) 
    {
        // If they are not, then the primitive is labeled a "Fake" and all points set to zero
        p.type = CFlux2DBCReflectLine::Fake;
        p.n0 = p.n1 = p.n2;
    }

    // Regardless of the type, add the primitive to the list
    m_Primitivs.Append() = p;
}

/**
 * @fn AddHULTriangle(n)
 *
 * @brief Adds a horizonally aligned triangle with its upper axis aligned
 * with the horizontal axis and the specified point to the right of the triangle
 *                                  ____
 *                                 HUL /
 *                                    /
 *
 * @param n - Point on the triangle
 */
void CFlux2DBCReflectLine::AddHULTriangle(Point2D32i const &n) 
{
    AddTriangle(n,-1,0,-1,-1,HULTriangle);	
}

/**
 * @fn AddHDLTriangle(n)
 *
 * @brief Adds a horizontaly aligned triangle with its lower axis aligned
 * with the horizontal axis and the point to the right of the triangle
 *                                   \
 *                                HDL \
 *                                 ____\
 *
 * @param n - Point on the triangle
 */
void CFlux2DBCReflectLine::AddHDLTriangle(Point2D32i const &n) 
{
    AddTriangle(n,-1,1,-1,0,HDLTriangle);		
}

/**
 * @fn AddHURTriangle(n)
 *
 * @brief Adds a horizontaly aligned triangle with its upper axis aligned
 * with the horizontal axis and the point to the left of the triangle
 *                               _______
 *                               \  HUR  
 *                                \  
 *                                 \
 *
 * @param n - Point on the triangle
 */
void CFlux2DBCReflectLine::AddHURTriangle(Point2D32i const &n) 
{
    AddTriangle(n,1,0,1,-1,HURTriangle);	
}

/**
 * @fn AddHDRTriangle(n)
 *
 * @brief Adds a horizontaly aligned triangle with its lower axis aligned
 * with the horizontal axis and the point to the left of the triangle
 *                                 /  
 *                                / HDR 
 *                               /______
 *
 * @param n - Point on the triangle
 */
void CFlux2DBCReflectLine::AddHDRTriangle(Point2D32i const &n) 
{
    AddTriangle(n,1,1,1,0,HDRTriangle);		
}

/**
 * @fn AddVULTriangle(n)
 *
 * @brief Adds a vertically aligned triangle with its left axis aligned
 * with the vertical axis and the point below the triangle
 *                               |VUL/
 *                               |  /
 *                               | /
 *                               |/
 *
 * @param n - Point on the triangle
 */
void CFlux2DBCReflectLine::AddVULTriangle(Point2D32i const &n) 
{
    AddTriangle(n,0,1,1,1,VULTriangle);	
}

/**
 * @fn AddVDLTriangle(n)
 *
 * @brief Adds a vertically aligned triangle with its left axis aligned
 * with the vertical axis and the point above the triangle
 *                               |\
 *                               | \
 *                               |  \
 *                               |VDL\
 *
 * @param n - Point on the triangle
 */
void CFlux2DBCReflectLine::AddVDLTriangle(Point2D32i const &n) 
{
    AddTriangle(n,0,-1,1,-1,VDLTriangle);		
}

/**
 * @fn AddVURTriangle(n)
 *
 * @brief Adds a vertically aligned triangle with its right axis aligned
 * with the vertical axis and the point below the triangle
 *                              \VUR|
 *                               \  |
 *                                \ |
 *                                 \|
 *
 * @param n - Point on the triangle
 */
void CFlux2DBCReflectLine::AddVURTriangle(Point2D32i const &n) 
{
    AddTriangle(n,-1,1,0,1,VURTriangle);	
}

/**
 * @fn AddVDRTriangle(n)
 *
 * @brief Adds a vertically aligned triangle with its right axis aligned
 * with the vertical axis and the point above the triangle
 *                                 /|
 *                                / |
 *                               /  |
 *                              /VDR|
 *
 * @param n - Point on the triangle
 */
void CFlux2DBCReflectLine::AddVDRTriangle(Point2D32i const &n) 
{
    AddTriangle(n,-1,-1,0,-1,VDRTriangle);		
}

/**
 * @fn ProccesNodePrimitive(PrimitiveType type, Point2D32i const &n)
 *
 * @brief Adds a primitive to the list based on the primitive type
 *
 * @param type - The primitive type
 *
 * @param n - Base point of the primitive
 */
void CFlux2DBCReflectLine::ProccesNodePrimitive(PrimitiveType type, Point2D32i const &n) 
{
    // Depending on the type of primitive to process, add the primitive
    switch(type)
    {
    case HULTriangle:
        AddHULTriangle(n);
        break;
    case HURTriangle:
        AddHURTriangle(n);
        break;
    case HDLTriangle:
        AddHDLTriangle(n);
        break;
    case HDRTriangle:
        AddHDRTriangle(n);
        break;
    case VULTriangle:
        AddVULTriangle(n);
        break;
    case VURTriangle:
        AddVURTriangle(n);
        break;
    case VDLTriangle:
        AddVDLTriangle(n);
        break;
    case VDRTriangle:
        AddVDRTriangle(n);
        break;	
    case Joint:
        AddTriangle(n,0,0,0,0,Joint);
        break;
    default:
        assert(false);
        break;
    }

}

/**
 * @fn PerformJoin(n, n1, n2, c);
 *
 * @brief Creates a joint primitive joining two points
 *
 * @param n - Address of a point (ignored)
 *
 * @param n1 - Address of the first point
 *
 * @param n2 - Address of the second point
 *
 * @param c - Output - The coefficients of the joint primitive
 */
void CFlux2DBCReflectLine::PerformJoin(Point2D32i const &n, Point2D32i const &n1, Point2D32i const &n2, Coefficients &c) 
{
    Primitive p;
    p.type = Joint;

    p.n0 = n1;
    p.n1 = n2;
    p.n2 = Point2D32i(0,0);
    p.n3 = Point2D32i(0,0);

    m_Primitivs.Append()  = p;	
    c = getPrimitiveCoefficients(p.type);
}

//#define INDEX(_n) _n

/**
 * @fn ApplyCondition()
 *
 * @brief Initializes primitive cooefficients for all primitives
 */
void CFlux2DBCReflectLine::ApplyCondition() 
{
    assert(m_InnerBorder.Size() > 3);
    assert(m_InnerBorder.Size() == m_Primitivs.Size());
    assert(NULL != m_pDstH);
    assert(NULL != m_pSrcH);
    int size = m_InnerBorder.Size();


    // Apply the first cell condition
    Primitive    p;
    p = m_Primitivs[0];


    m_pDstH->At(m_InnerBorder[0]) = m_pBottom->At(m_InnerBorder[0]) + 
        m_start_joincoefitiants.c0 * (m_pSrcH->At(p.n0)-m_pBottom->At(p.n0)) + 
        m_start_joincoefitiants.c1 * (m_pSrcH->At(p.n1)-m_pBottom->At(p.n1)) + 
        m_start_joincoefitiants.c2 * (m_pSrcH->At(p.n2)-m_pBottom->At(p.n2)) + 
        m_start_joincoefitiants.c3 * (m_pSrcH->At(p.n3)-m_pBottom->At(p.n3));
    m_pDstU->At(m_InnerBorder[0]) = 
        m_start_joincoefitiants.c0 * m_pSrcU->At(p.n0) + 
        m_start_joincoefitiants.c1 * m_pSrcU->At(p.n1) + 
        m_start_joincoefitiants.c2 * m_pSrcU->At(p.n2) + 
        m_start_joincoefitiants.c3 * m_pSrcU->At(p.n3);
    m_pDstV->At(m_InnerBorder[0]) = 
        m_start_joincoefitiants.c0 * m_pSrcV->At(p.n0) + 
        m_start_joincoefitiants.c1 * m_pSrcV->At(p.n1) + 
        m_start_joincoefitiants.c2 * m_pSrcV->At(p.n2) + 
        m_start_joincoefitiants.c3 * m_pSrcV->At(p.n3);


    //DebugPrint::DebugFormat(L"\n\n\nH000: %f ", dest(m_innerBorder[0]) );

    // For all cells between the first and last
    for (int i = 1; i < size - 1; ++i) 
    {
        p = m_Primitivs[i];
        if (p.type != CFlux2DBCReflectLine::Fake) 
        {

            m_pDstH->At(m_InnerBorder[i]) = m_pBottom->At(m_InnerBorder[i])+
                m_coefficients.c0 * (m_pSrcH->At(p.n0)-m_pBottom->At(p.n0)) + 
                m_coefficients.c1 * (m_pSrcH->At(p.n1)-m_pBottom->At(p.n1));
            m_pDstU->At(m_InnerBorder[i]) = 
                m_coefficients.c0 * m_pSrcU->At(p.n0) + 
                m_coefficients.c1 * m_pSrcU->At(p.n1);
            m_pDstV->At(m_InnerBorder[i]) = 
                m_coefficients.c0 * m_pSrcV->At(p.n0) + 
                m_coefficients.c1 * m_pSrcV->At(p.n1);

        }
    }

    // The last cell
    p = m_Primitivs[size - 1];


    m_pDstH->At(m_InnerBorder[size-1]) = m_pBottom->At(m_InnerBorder[size-1]) + 
        m_end_joincoefitiants.c0 * (m_pSrcH->At(p.n0)-m_pBottom->At(p.n0)) + 
        m_end_joincoefitiants.c1 * (m_pSrcH->At(p.n1)-m_pBottom->At(p.n1)) + 
        m_end_joincoefitiants.c2 * (m_pSrcH->At(p.n2)-m_pBottom->At(p.n2)) + 
        m_end_joincoefitiants.c3 * (m_pSrcH->At(p.n3)-m_pBottom->At(p.n3));
    m_pDstU->At(m_InnerBorder[size-1]) = 
        m_end_joincoefitiants.c0 * m_pSrcU->At(p.n0) + 
        m_end_joincoefitiants.c1 * m_pSrcU->At(p.n1) + 
        m_end_joincoefitiants.c2 * m_pSrcU->At(p.n2) + 
        m_end_joincoefitiants.c3 * m_pSrcU->At(p.n3);
    m_pDstV->At(m_InnerBorder[size-1]) = 
        m_start_joincoefitiants.c0 * m_pSrcV->At(p.n0) + 
        m_start_joincoefitiants.c1 * m_pSrcV->At(p.n1) + 
        m_start_joincoefitiants.c2 * m_pSrcV->At(p.n2) + 
        m_start_joincoefitiants.c3 * m_pSrcV->At(p.n3);

}

/**
 * @fn Run(pG, pars, globals )
 *
 * @brief This method runs the process
 *
 * @param pG - The grid to be modified
 *
 * @param pars - Address of the current simulation step parameters
 *
 * @param globals - Address of the global simulation parameters
 */
void CFlux2DBCReflectLine::Run( CFlux2DGrid* pG, 
    CFlux2DCalculatorPars const &pars, 
    CFlux2DGlobalParameters const &globals )	
{
    // Initialize parameters for coefficients
    m_pBottom = &pG->GetBottom().m_heightMap;
    m_pSrcH = &pG->GetCurrentSurface().m_heightMap;
    m_pDstH	 = &pG->GetDestSurface().m_heightMap;


    m_pSrcU = &pG->GetCurrentSurface().m_uMap;
    m_pSrcV = &pG->GetCurrentSurface().m_vMap;
    m_pDstU = &pG->GetDestSurface().m_uMap;
    m_pDstV = &pG->GetDestSurface().m_vMap;


    ApplyCondition();
}

/**
 * @fn getPrimitivesTypes()
 *
 * @brief Gets the primitive types for the current line
 *
 * @return The primitive type
 */
CFlux2DBCReflectLine::PrimitiveType CFlux2DBCReflectLine::getPrimitivesTypes() 
{
    // Declare and initialize local working variables
    float ly = fabs(m_line.vY);
    float lx = fabs(m_line.vX);
    int   n = 0;
    const static int is_lower_splain =   1 << 2;
    const static int is_left_splain =   1 << 1;
    const static int is_upper_or_lower = 1 << 0;

    // Correct order (counter-clockwise from noon) is {1,2,4,3,8,7,5,6}
    const static PrimitiveType ps[] = {
        VURTriangle, 
        HDLTriangle, 
        VDRTriangle, 
        HULTriangle, 
        VULTriangle, 
        HDRTriangle, 
        VDLTriangle, 
        HURTriangle
    };

    // The primitive type for a line depends upon its overall vector determined by analysis of the
    // X and Y delta's in the line. For our purposes, assume that a vector of zero degrees lies on the
    // X axis in a positive direction. A vector of ninety degrees would be vertical and pointing upward.

    // We compute offsets into the primitive type array by analyzing the vector V

    // If the V >= 90 degrees and V < 270 degrees, we consider the region to the left

    if (m_line.vX < -s_zeroTolerance || (lx < s_zeroTolerance && m_line.vY > s_zeroTolerance)) 
    {
        n |= is_left_splain;
    }

    // If V > 180 degrees and V < 360 degrees, we consider the region below the X axis

    if (m_line.vY < 0) 
    {
        n |= is_lower_splain;
    }

    // Finally, if the absolute value of Dy is greater than the absolute value of Dx, than 
    // either V > 45 degrees and V < 135 degrees or V > 225 degrees and V < 315 degrees, we
    // consider the regions along the major (longest) axis which is the vertical axis
    //
    //                      \----/
    //                       \--/
    //                        \/
    //                        /\
    //                       /--\
    //                      /----\
    //
    if (ly > lx) 
    {
        n |= is_upper_or_lower;
    }

    // The resulting primitive type will be as follows:
    //
    //                               \ HUL | HDL /
    //                                \    |    /
    //                                 \   |   /
    //                                  \  |  /
    //                              VDR  \ | /  VUR
    //                            ________\|/________
    //                                    /|\
    //                                   / | \
    //                              VDL /  |  \ VUL
    //                                 /   |   \
    //                                /    |    \
    //                               / HUR | HDR \
    //
    return ps[n];
}

/**
 * @fn getPrimitiveCoefficients(pt)
 *
 * @brief Get the primitive coefficients for the specified primitive type
 *
 * @param pt - The primitive type
 *
 * @return - The coefficients of the primitive type
 */
CFlux2DBCReflectLine::Coefficients CFlux2DBCReflectLine::getPrimitiveCoefficients(CFlux2DBCReflectLine::PrimitiveType pt) 
{
    // Declare working variables
    Coefficients cf;
    float alpha;

    // Allocate memory for the co-efficients
    memset(&cf,0,sizeof(Coefficients));

    // The co-efficients depend upon the primitive type
    switch(pt)
    {
    case HULTriangle:
        alpha = m_line.nY() / m_line.nX();
        cf.c0 = 1-alpha;
        cf.c1 = alpha;
        break;
    case HURTriangle:
        alpha = m_line.nY() / m_line.nX();
        cf.c0 = 1+alpha;
        cf.c1 = -alpha;
        break;
    case HDLTriangle:
        alpha = m_line.nY() / m_line.nX();		
        cf.c0 = -alpha;
        cf.c1 = 1+alpha;
        break;
    case HDRTriangle:
        alpha = m_line.nY() / m_line.nX();
        cf.c0 = alpha;
        cf.c1 = 1-alpha;
        break;
    case VULTriangle:
        alpha = m_line.nX() / m_line.nY();
        cf.c0 = 1-alpha;
        cf.c1 = alpha;		
        break;
    case VURTriangle:
        alpha = m_line.nX() / m_line.nY();
        cf.c0 = -alpha;
        cf.c1 = 1+alpha;		
        break;
    case VDLTriangle:
        alpha = m_line.nX() / m_line.nY();
        cf.c0 = 1+alpha;
        cf.c1 = -alpha;		
        break;
    case VDRTriangle:
        alpha = m_line.nX() / m_line.nY();
        cf.c0 = alpha;
        cf.c1 = 1-alpha;		
        break;	
    case Joint:
        cf.c0 = 0.5f;
        cf.c1 = 0.5f;
        break;
    default:
        assert(false);
        break;
    }

    // We don't use the second set of points, so we initialize the co-efficients to zero
    cf.c2 = 0.f;
    cf.c3 = 0.f;
    return cf;
}


/**
 * @fn CFlux2DBCReflectLine(grid, x1, y1, x2, y2, startJoinType, endJoinType, intersect)
 *
 * @brief Class constructor, the class implements a linear reflective boundary condition.
 *
 * @param grid - The grid to act upon
 *
 * @param x1 - Starting X coordinate
 *
 * @param y1 - Starting Y coordinate
 *
 * @param x2 - Ending X coordinate
 *
 * @param y2 - Ending Y coordinate
 *
 * @param startJoinType - Line join type for the beginning of the line - Ignored
 *
 * @param endJoinType - Line join type for the ending of the line - Ignored
 *
 * @param intersect - Always true - Ignored
 */
CFlux2DBCReflectLine::CFlux2DBCReflectLine(
    CFlux2DGrid *grid, 
    float x1, 
    float y1, 
    float x2, 
    float y2, 
    PrimitiveType startJoinType, 
    PrimitiveType endJoinType,
    bool filltri) 
{	
    // Validate the grid
    assert(grid != NULL);
    // Ensure we have a viable region
    assert(fabs(x1-x2) + fabs(y1-y2) > s_zeroTolerance);

    // Save the grid
    m_grid = grid;

    // Compute the line
    m_line = getLine(
        x1, 
        y1, 
        x2, 
        y2);	

    // Clear borders and primitives
    m_InnerBorder.Clear();
    m_Primitivs.Clear();

    // Calculate the border
    rasterize(m_line, grid->GetDomainW(), grid->GetDomainH(), &m_InnerBorder);	

    // Set the capacity to store primitives
    m_Primitivs.SetCapacity(m_InnerBorder.Size());

    // Assign primitives to the border

    // Get the type of primitive
    m_primitiveType = getPrimitivesTypes();
    // Get the primitive co-efficients
    m_coefficients   = getPrimitiveCoefficients(m_primitiveType);

    // Join border points at the start
    PerformJoin(m_InnerBorder[0], m_InnerBorder[1], m_InnerBorder[1], m_start_joincoefitiants);
    // For every cell in the border
    for (int i = 1; i < (int)m_InnerBorder.Size() - 1; ++i) 
    {
        // Process the primitives, adding them to the list
        ProccesNodePrimitive(m_primitiveType, m_InnerBorder[i]);
    }

    // Now join border points at the end
    PerformJoin(m_InnerBorder[m_InnerBorder.Size()-1], m_InnerBorder[m_InnerBorder.Size()-2], m_InnerBorder[m_InnerBorder.Size()-2],m_end_joincoefitiants);

    // Make sure that we have primitives for each cell in the border
    assert(m_Primitivs.Size() == m_InnerBorder.Size());		
}

/**
 * @fn CFlux2DBCReflectLine(file)
 *
 * @brief Class constructor
 *
 * @param file - The file containing the line's parameters
 */
CFlux2DBCReflectLine::CFlux2DBCReflectLine(CFile &file)
{
    // Initialize working variables
    int i;
    int PrimSize = 0;
    int InnerBorderSize = 0;

    // Read the file, get the number of primitives
    file.Restore(PrimSize);

    // Read the primitives from the file and add them to the list
    for (i=0;i<PrimSize;++i)
    {
        file.Restore(m_Primitivs.Append());
    }

    // Do the same for the inner border
    file.Restore(InnerBorderSize);
    for (i=0;i<InnerBorderSize;++i)
    {
        file.Restore(m_InnerBorder.Append());
    }

    // Then restore the co-efficients
    file.Restore(m_coefficients);
    file.Restore(m_end_joincoefitiants);
    file.Restore(m_start_joincoefitiants);
}

/**
 * @fn SaveData(CFile &file)
 *
 * @brief Used to pack action data into the file
 *
 * @param file - A reference to the interface used for packing
 */
void CFlux2DBCReflectLine::SaveData(CFile &file) const
{
    // Initialize working variables
    int i;
    int PrimSize = m_Primitivs.Size();
    int InnerBorderSize = m_InnerBorder.Size();

    // Save the size of the primitive list
    file.Save(PrimSize);
    // Then store the primitives
    for (i=0;i<PrimSize;++i)
    {
        file.Save(m_Primitivs[i]);
    }

    // Do the same for the inner border
    file.Save(InnerBorderSize);
    for (i=0;i<InnerBorderSize;++i)
    {
        file.Save(m_InnerBorder[i]);
    }

    // And then save the co-efficients
    file.Save(m_coefficients);
    file.Save(m_end_joincoefitiants);
    file.Save(m_start_joincoefitiants);
}

// End of file
