/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file CFile2DBCReflectLine.h
 *
 * @brief Contains piece-wise linear boundary conditions support (BC) declaration. 
 */

#pragma once

#include <vector>
#include <list>
#include "Flux/actions/CFlux2DAction.h"
#include "Flux/CFlux2DGrid.h"

// Forward declaration
void TestSquareIntersecion(CFlux2DGrid *grid);

/**
 * @class CFlux2DBCReflectLine
 *
 * @brief Implements a boundary condition reflection line
 */
class CFlux2DBCReflectLine : 
    public CFlux2DAction
{
public:
    DEF_CREATE_FUNC(CFlux2DBCReflectLine);

    /**
     * @enum PrimitiveType
     *
     * @brief Defines types of primitives
     *
     * @note For triangle types, the nomenclature is as follows:
     *       The first character indicates the primary axis of the
     *       triangle, either the Horizontal or Vertical axis of the graph
     *       The second character is a vertical refererence, either Up or Down
     *       The third characters is a horizontal reference, either Left or Right
     *       The direction indicated by the second and third characters varies
     *       according to the value of the first character. The matching reference
     *       value indicates which axis of the triangle is the primary axis. The
     *       non-matching value indicates the direction from the center point. For 
     *       example, an HUL triangle is horizontal, it's upper axis is the primary
     *       horizontal axis, and it is to the left of the center point.
     *
     * 
     * @note                     \ VUR |VDL  /
     *                            \    |    /
     *                             \   |   /
     *                              \  |  /
     *                           HDL \ | / HDR
     *                      ___________|__________
     *                                /|\
     *                           HUL / | \ HUR
     *                              /  |  \
     *                             /   |   \
     *                            /    |    \
     *                           / VDR |VDL  \
     */
    enum PrimitiveType 
    {
        HULTriangle,
        HDLTriangle,
        HURTriangle,
        HDRTriangle,
        VULTriangle,
        VDLTriangle,
        VURTriangle,
        VDRTriangle,
        Joint,
        Fake
    };

    /**
     * @fn CFlux2DBCReflectLine(grid, x1, y1, x2, y2, startJoinType, endJoinType, intersect)
     *
     * @brief Class constructor
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
    CFlux2DBCReflectLine( CFlux2DGrid *grid, 
        float x1, 
        float y1, 
        float x2, 
        float y2, 
        PrimitiveType startJoinType, 
        PrimitiveType endJoinType, 
        bool intersect = true );

    /**
     * @fn fillBorder()
     *
     * @brief Fills the border
     */
    void fillBorder();

    /**
     * @fn CFlux2DBCReflectLine(file)
     *
     * @brief Class constructor
     *
     * @param file - The file containing the line's parameters
     */
    CFlux2DBCReflectLine(CFile &file);

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
    virtual void Run( CFlux2DGrid* pG, 
        CFlux2DCalculatorPars const &pars, 
        CFlux2DGlobalParameters const &globals );

    /**
     * @fn SaveData(CFile &file)
     *
     * @brief Used to pack action data into the file
     *
     * @param file - A reference to the interface used for packing
     */
    virtual void SaveData(CFile &file) const;

    /**
     * @struct Line
     *
     * @brief Implements a line
     */

    struct Line
    {
        float y0; // start point y
        float x0; // start point x
        float y1; // end point y
        float x1; // end point x
        float vX; // x component of vector
        float vY; // y component of vector

        inline float nX() const 
        {
            return -vY;
        }
        inline float nY() const 
        {
            return vX;
        }
    };

    /**
     * @struct Coefficients
     *
     * @brief Coefficients for one "Primitive" - clockwise starting from top left corner
     */
    struct Coefficients
    {
        float c0;
        float c1;
        float c2;
        float c3;
    };

    /**
     * @struct Primitive
     *
     * @brief Parameters for a "Primitive"
     */
    struct Primitive
    {
        PrimitiveType type;
        Point2D32i      n0;
        Point2D32i      n1;
        Point2D32i      n2;
        Point2D32i      n3;
    };

    DynamicArray<Point2D32i>    m_InnerBorder;	

protected:

    Line					m_line;
    CFlux2DGrid*            m_grid;
    Coefficients			m_coefficients;
    Coefficients			m_start_joincoefitiants;
    Coefficients			m_end_joincoefitiants;
    PrimitiveType			m_primitiveType;

    DynamicArray<Primitive> m_Primitivs;
    DynamicMatrix<float>*   m_pBottom;
    DynamicMatrix<float>*   m_pSrcH;
    DynamicMatrix<float>*   m_pDstH;

    DynamicMatrix<float>*   m_pSrcU;
    DynamicMatrix<float>*   m_pDstU;
    DynamicMatrix<float>*   m_pSrcV;
    DynamicMatrix<float>*   m_pDstV;

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
    void PerformJoin(Point2D32i const &n, Point2D32i const &n1, Point2D32i const &n2, Coefficients &c);

    /**
     * @fn ApplyCondition()
     *
     * @brief Initializes primitive cooefficients for all primitives
     */
    void ApplyCondition();

    /**
     * @fn ProccesNodePrimitive(PrimitiveType type, Point2D32i const &n)
     *
     * @brief Adds a primitive to the list based on the primitive type
     *
     * @param type - The primitive type
     *
     * @param n - Base point of the primitive
     */
    void ProccesNodePrimitive(PrimitiveType type, Point2D32i const &n);

    /**
     * @fn getPrimitivesTypes()
     *
     * @brief Gets the primitive types for the current line
     *
     * @return The primitive type
     */
    PrimitiveType getPrimitivesTypes();

    /**
     * @fn getPrimitiveCoefficients(pt)
     *
     * @brief Get the primitive coefficients for the specified primitive type
     *
     * @param pt - The primitive type
     *
     * @return - The coefficients of the primitive type
     */
    Coefficients getPrimitiveCoefficients(PrimitiveType pt);

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
    void AddTriangle(Point2D32i const &n, int dx1, int dy1, int dx2, int dy2, PrimitiveType t);

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
    void AddHULTriangle(Point2D32i const &n);

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
    void AddHDLTriangle(Point2D32i const &n);

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
    void AddHURTriangle(Point2D32i const &n);

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
    void AddHDRTriangle(Point2D32i const &n);

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
    void AddVULTriangle(Point2D32i const &n);

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
    void AddVDLTriangle(Point2D32i const &n);

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
    void AddVURTriangle(Point2D32i const &n);

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
    void AddVDRTriangle(Point2D32i const &n);

};

