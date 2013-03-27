/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file boatlogic.h
 *
 * @brief Implements parameters and classes to implement boat logic and the fluid-boat interaction
 */


#ifndef BOATLOGIC_H
#define BOATLOGIC_H


#include "flux/CFlux2DScene.h"
#include "common/physics_3D_fluid_sim_common.h"
#include "commondatastruct.h"

#include "mathstub.h"

#include "SceneGraphBase.h"

using namespace collada;

/* ******************************************************************************************* *\
                    Parameters that define the interaction between the boat and fluid.
\* ******************************************************************************************* */

#define MAX_NUMBER_OF_FORCES 5000           // The maximal number of forces applied to the fluid surface that are result of fluid/boat iteraction.
#define GRID_BORDER_WIDTH 1.0f              // The marginal extra cell's width in the fluid surface grid.
#define BOAT_ANIMATION_MULTIPLIER 0.3f      // A value used to scale animation coordinates.

#define BOAT_DENSITY 2688                   // The density of the boat used in calculations. Density doesn't affect the visual appearance of material.
#define WATER_DENSITY 1000.0f               // The density of the fluid used in calculations. Density doesn't affect the visual appearance of fluid.
#define GRAVITY 9.81f                       // An acceleration of gravity value.

#define BOAT_THICKNESS 0.1f                 // Thickness of the boat frame.

#define AIR_FRICTION_COEFFITIENT 500.0f     // Air friction coefficient
#define WATER_FRICTION_COEFFITIENT 1500.0f  // Water friction coefficient

#define NUMBER_OF_WATER_LINE_POINTS 40      // Number of points where the fluid level will be checked. Using more points can guarantee more accurate calculations.
#define UNAFFECTED_WATER_DISTANCE   0.1f    // The distance from the boat on the fluid surface projection where boat to fluid influence is quite low.
#define BOAT_TO_BORDER_DISTANCE     6.0f    // The minimal distance from boat center to the end of the grid. Used to avoid moving beyond the bounds.


#define MAX_FORCE_VALUE             3.0f    // Maximum value of boat force applied to the fluid surface.
#define FORCE_MULTIPLIER            0.25f   // A multiplier for force during boat movement.


#define SIDE_WAVES_DIVISOR          7.0f    // Value to adjust the height of boat side waves.
#define TIME_STEP                   0.08f   // Time step used for boat movement visualisation.
#define BOAT_PROJECTION_MULTIPLIER  1.2f    // Multiplier for applying boat forces to the fluid surface.
#define BOAT_ACCELERATION           1.0f    // Horizontal acceleration for the boat when user specifies a new destination point.
#define START_BOAT_SPEED            15.0f   // Horizontal speed for the boat when user specifies a new destination point.
#define MIN_FLUID_DEPTH             1.0f    // Minimal value of fluid depth when the boat speed decreases.
#define STOP_ACCELERATION           0.5f    // Friction acceleration when the bottom is lower then MIN_FLUID_DEPTH

/**
 * @struct INT_BOUNDING_BOX
 *
 * @brief A structure that represents the bounding box for one part of the boat.
 *
 * @note This structure is used in the collision detection algorithm
 */
typedef struct
{
    int maxX;
    int minX;
    int maxY;
    int minY;
    int maxZ;
    int minZ;
}INT_BOUNDING_BOX;

/**
 * @class Boat
 *
 * @brief This class defines boat position parameters, collision geometry,
 *        displaced fluid volume calculation parameters, and specialized boat functions
 */
class Boat
{
private:

    /* *********************************************************************************************** *\ 
                                        Boat position parameters
    \* *********************************************************************************************** */

    // Mouse controlled coefficients
    float2 m_destination;                   // Destination point for the boat to move to.
    float m_horizontalSpeed;                // Horizontal component of boat speed.
    float m_horizontalAcceleration;         // Horizontal component of boat acceleration. Used for boat movement according to user mouse clicks.

    CFlux2DScene*  m_pScene;                // Scene class used for fluid surface modeling.

    // During the animation, boat ccordinates will be calculated as m_referencePoint + m_offset.
    // Offset calculation is performed using rotation around the Z angle by m_startAnimationAngle.
    // In the case of frozen animation, m_offset isn't used.

    float2 m_referencePoint;                // Reference point during animation.
    float2 m_offset;                        // Boat offset relative to the m_referencePoint during animation.
    float m_startAnimationAngle;            // Angle used to rotate animation coordinates around m_referencePoint.

    //  It helps to start the animation in the same direction that the boat was moving previously 

    float2 m_startAnimationPoint;           // Starting point coordinates in the collada based animation.
    bool  m_wasAnimationFrozen;             // Indicates whether the animation was frozen in the previous step.
    bool  m_calcStartAnimationAngle;        // Shows if m_startAnimationAngle needs to be calculated.

    float m_yAngle;                         // Movement direction angle.
    float m_verticalSpeed;                  // Boat vertical speed.
    float m_verticalAcceleration;           // Boat vertical acceleration.
    float m_height;                         // Boat center height in the current step.
    bool  m_isAnimationFrozen;              // Indicates if the boat uses animation coordinates in the current step

    float4 m_CenterOfGravity;               // Boat center of gravity. Position + mass.

    /* *********************************************************************************************** *\ 
                                        Boat collision geometry
    \* *********************************************************************************************** */

    float3* m_pVertices;                    // Vertices of boat collision geometry used for collision detection.
    unsigned int m_verticesCount;           // Number of vertices in the boat collision geometry.
    float3* m_pTransformedVertices;         // Vertices of boat collision geometry with applied current step transformation.
    void* m_pIndices;                       // Indices of boat collision geometry triangles in vertices array.
    unsigned int m_indicesCount;            // Number of indices in m_pIndices array.
    float3* m_pForces;                      // Forces caused by boat movement applyed to the fluid surface.
    unsigned int m_numberOfForces;          // Number of forces in m_pForces array.
    collada::ScalarType m_IndicesType;      // Type of indices used in m_pIndices array.

    /* *********************************************************************************************** *\ 
                               Displaced fluid volume calculation parameters
    \* *********************************************************************************************** */

    float3* m_pWaterLinePoints;             // Fluid points where the fluid level will be checked to approximately calculate the fluid surface.
    float3* m_pTransformedWaterLinePoints;  // m_pWaterLinePoints with applied boat transformation.
    float* m_pWaterLinePointsMultipliers;   // An array of multipliers for applying force to the fluid surface. The force value will depend on
                                            // the point position relative to the boat. If the point is in the boat front then the multiplier
                                            // is greater. If the point is in the back of the boat, then the multiplier is lower.

    float* m_pStoredWaterLevels;            // Stored value of the water level. If the next point of the fluid surface is out of the fluid
                                            // grid bounds, then the stored value will be used.

    unsigned int m_waterLinePointsCount;    // The number of elements in the m_pStoredfluidLevels array.

    float m_displacedWaterVolume;           // Approximately calculated volume of the fluid displaced by the boat.

    float m_previousXAngle;                 // Contains the previous value of the boat rotation angle around the X axis.
    float m_previousYAngle;                 // Contains the previous value of the boat rotation angle around the Y axis.
    /////////////////

    float4x4 m_pTransformationMatrix;       // Transformation matrix for boat rendering. A new transformation matrix is calculated by the
                                            // Move() function call.

    /**
     * @fn GetCenterOfGravity()
     *
     * @brief Calculates the center of gravity for the given geometry and returns the center coordinates and mass.
     *
     * @param  in_pVertices - Boat collision vertices.
     *
     * @param  in_verticesCount - The number of boat collision vertices.
     *
     * @param  in_indicesType - The type of indices used to store boat triangles.
     *
     * @param  in_indicesCount - The number of boat trangle indices.
     *
     * @param  in_pIndices - The boat triangle indices.
     *
     * @return - The boat's center of gravity consisting of center coordinates and mass
     */
    float4 GetCenterOfGravity( float3* in_pVertices, 
        unsigned int in_verticesCount, 
        collada::ScalarType const in_indicesType, 
        unsigned int const in_indicesCount, 
        void* const in_pIndices );

    /**
     * @fn CalcNextBoatPosition()
     *
     * @brief Updates boat position parameters according to data sent from the host.
     *
     * @param in_pDataFromHost - Parameter structure received from the host.
     */
    void CalcNextBoatPosition(const WATER_STRUCT* in_pDataFromHost);

    /**
     * @fn GetWaterLinePoints()
     *
     * @brief Creates an array of points for the fluid that should be checked while calculating displaced fluid volume.
     *        These points are the named water line.
     *
     * @param  in_pVertices - Boat collision vertices.
     *
     * @param  in_verticesCount - The number of boat collision vertices.
     *
     * @param  out_pWaterLinePoints - The points that should be checked while calculating displaced fluid volume.
     *
     * @param  out_waterLinePoints - The number of points that should be checked while calculating displaced fluid volume.
     */
    void GetWaterLinePoints( const float3* in_pVertices, 
        const unsigned int in_verticesCount, 
        float3* &out_pWaterLinePoints, 
        unsigned int &out_waterLinePointsCount );

    /**
     * @fn QuickHull()
     *
     * @brief The core function for the convex hull calculation algorithm for the boat's horizontal projection points.
     *        This method is performed in order to get water line points.
     *
     * @param  in_pPoint1 - First marginal point for the algorithm.
     *
     * @param  in_pPoint2 - Second marginal point for the algorithm.
     *
     * @param  in_pVertices - Boat collision vertices.
     *
     * @param  in_verticesCount - The number of boat collision vertices.
     *
     * @param  inout_pIsInHull - Indicates whether the points are in the convex hull. If the point is one of the hull points,
     *                           then inout_pIsInHull contains its number in clockwise order, otherwise, inout_pIsInHull contains 0.
     *
     * @param  in_index - The contains current index of the last element in the convex hull in clockwise order
     */
    unsigned int QuickHull( const float2 in_pPoint1, 
        const float2 in_pPoint2,
        const float3* in_pVertices, 
        unsigned int* &inout_pIsInHull,
        const unsigned int in_verticesCount, 
        const unsigned int in_index );

    /**
     * @fn GetDisplacedWaterVolume()
     *
     * @brief This method approximates the displaced fluid volume.
     *
     * @param  in_pVertices - Boat collision vertices.
     *
     * @param  in_verticesCount - The number of boat collision vertices.
     *
     * @param in_pWaterLinePoints - Points that should be checked while calculating displaced fluid volume.
     *
     * @param  out_waterLinePoints - The number of points that should be checked while calculating displaced fluid volume.
     *
     * @param  in_indicesType - Type of indices used to store boat triangles.
     *
     * @param  in_indicesCount - The number of boat trangle indices.
     *
     * @param  in_pIndices - Boat triangle indices.
     *
     * @param  in_pTransformationMatrix - A matrix for boat transformation, received from host
     *
     * @param in_pGrid - Fluid surface 2D grid.
     *
     * @return - The volume of fluid displaced by boat geometry
     */
    float GetDisplacedWaterVolume( const float3* in_pVertices,
        const unsigned int in_verticesCount, 
        const float3* in_pWaterLinePoints, 
        const unsigned int in_waterLinePointsCount, 
        const void* in_pIndices,
        const unsigned int in_indicesCount,
        collada::ScalarType in_IndicesType, 
        const float4x4 in_TransformationMatrix, 
        const CFlux2DGrid *in_pGrid );

    /**
     * @fn GetGridPoints()
     *
     * @brief Returns the points where force should be applied and the force value for these points.
     *
     * @param  in_pVertices - Boat collision vertices.
     *
     * @param  in_verticesCount - The number of boat collision vertices.
     *
     * @param  in_transformationMatrix - Boat transformation matrix received from the host
     *
     * @param  in_indicesType - The type of indices used to store boat triangles.
     *
     * @param  in_indicesCount - The number of boat trangle indices.
     *
     * @param  in_pIndices - Boat triangle indices.
     *
     * @param  out_pForces - An array of points where force should be applied and the force value in these points.
     *
     * @param  out_numberOfForces - The number of points where force should be applied.
     */
    void GetGridPoints( const unsigned int in_verticesCount,
        const float3* in_pVertices, 
        const float4x4 in_TransformationMatrix, 
        const collada::ScalarType in_indicesType, 
        const unsigned int in_indicesCount, 
        const void* in_indices, 
        float3* out_pForces, 
        unsigned int &out_numberOfForces );

    /**
     * @fn GetCurrentBoatYHeight()
     *
     * @brief Returns the current boat height Y coordinate value 
     */
    float GetCurrentBoatYHeight();

    /**
     * @fn MoveWithoutAnimation()
     *
     * @brief Moves the boat according to a user mouse click.
     *
     * @param  in_pDataFromHost - Parameters received from the host structure.
     */
    void MoveWithoutAnimation(const WATER_STRUCT* in_pDataFromHost);

    /**
     * @fn GetTriangleGravityCenter()
     *
     * @brief Calculates the center of gravity and the full mass of the boat on the assumption that
     *        the boat is an evenly distributed mass object.
     *
     * @param  in_pTriangle - Pointer to the array with the coordinates of the triangle's points.
     */
    float4 GetTriangleGravityCenter(float3* const in_pTriangle);

    /**
     * @fn Sign()
     *
     * @brief Returns the sign of a number.
     *
     * @param k - The number to examine
     *
     * @return - One (1) if the sign of the number is positive or the number is zero
     *           Negative One (-1) if the sign of the number is negative
     */
    short Sign(const float k);

    /**
     * @fn GetTriangleBoundingBox()
     *
     * @brief Gets a 2 dimensional bounding box in the XZ projection for a 3-dimensional triangle.
     *
     * @param in_pVertices - The coordinates of the array with the coordinates of the triangle's points.
     */
    INT_BOUNDING_BOX GetTriangleBoundingBox(float3* in_pVertices);

    /**
     * @fn Contains()
     *
     * @brief Used to check if force exists with coordinates X and Z.
     *
     * @param  in_pForces - An array of points where force should be applied.
     *
     * @param  in_forcesCount - The number of forces where force should be applied.
     *
     * @param  x - The X coordinate of the point to check for the existance of force.
     *
     * @param  y - The Y coordinate of the point to check for the existance of force.
     *
     * @return - A value indicating if the point with the given coordinates is in the forces array.
     *           TRUE if the point is in the forces array
     *           FALSE if it is not
     */
    bool Contains(const float3* in_pForces, unsigned int in_forcesCount, int x, int z);

    /**
     * @fn IsPointInsideTriangle()
     *
     * @brief This method checks whether point with X and Z coordinates is positioned inside a triangle.
     *
     * @param  in_pTriangle - The coordinates of the 3 dimentional triangle.
     *
     * @param  x - The X coordinate of the point.
     *
     * @param  y - The Y coordinate of the point.
     *
     * @return TRUE if the point with the given coordinates is inside the triangle
     *         FALSE if it is not
     */
    bool IsPointInsideTriangle(const float3* in_pTriangle, float x, float z);

    /**
     * @fn GetPointHeight()
     *
     * @brief This method returns the (x,z) point height. The point is supposed to belong to a 
     *        triangle with in_pTriangleVertices coordinates.
     *
     * @param  in_pTriangleVertices - The coordinates of the triangle.
     *
     * @param  x - The X coordinate of the point.
     *
     * @param  z - The Z coordinate of the point.
     *
     * @return - The height of the given point.
     */
    float GetPointHeight(const float3* in_pTriangleVertices,float x,float z);

    /**
     * @fn CalcXAngle()
     *
     * @brief This method is used to calculate the XAngle of the boat model transformation.
     *
     * @param  pF - Fluid surface 2D grid.
     *
     * @param  x - The X coordinate of the boat center.
     *
     * @param  y - The Y coordinate of the boat center.
     *
     * @return - The boat's X angle value.
     */
    float CalcXAngle(CFlux2DGrid* pF, float x, float y);

    /**
     * @fn CalcYAngle()
     *
     * @brief This method is used to calculate the YAngle of the boat model transformation.
     *
     * @param  pF - Fluid surface 2D grid.
     *
     * @param  x - The X coordinate of the boat center.
     *
     * @param  y - The Y coordinate of the boat center.
     *
     * @return - The boat's Y angle value.
     */
    float CalcYAngle(CFlux2DGrid* pF, float x, float y);

    /**
     * @fn GetTriangleArea()
     *
     * @brief Computes the area of a triangle
     *
     * @note The angle is calculated with the help of a cross product.
     *
     * @param  in_pTriangle - Coordinates of the triangle.
     *
     * @return - The calculated area of the triangle.
     */
    float GetTriangleArea(float3* const in_pTriangle);

    /**
     * @fn GetSystemSolution()
     *
     * @brief Computes a solution of 2 dimentional linear equations. A fudge factor is applied
     *        to ensure that there is never a divide by zero overflow error
     *
     * @note The two outputs are computed such that:
     *            ( in_x1 )            ( in_y1 )   (in_xc)
     *   out_t1 * (       ) + out_t2 * (       ) = (     )
     *            ( in_x2 )            ( in_y2 )   (in_y1)
     *
     * @param in_x1 - Input
     *
     * @param in_x2 - Input
     *
     * @param in_y1 - Input
     *
     * @param in_y2 - Input
     *
     * @param in_xc - Input
     *
     * @param in_yc - Input
     *
     * @param out_t1 - Output - Equal to ((in_xc*in_y2) - (in_yc*in_x2))/((in_x1*in_y2)-(in_y1*in_x2))
     *
     * @param out_t2 - Output - Equal to ((in_x1*in_yc)-(in_y1*in_xc))/((in_x1*in_y2)-(in_y1*in_x2))
     */
    void GetSystemSolution( float in_x1, 
        float in_x2, 
        float in_y1, 
        float in_y2, 
        float in_xc, 
        float in_yc, 
        float &out_t1, 
        float &out_t2 );

    /**
     * @fn CalcDistanceToLine()
     *
     * @brief Calculates the distance from a point to a line.
     *
     * @note The point should be located on the right side.  If the point is positioned on the left side,
     *       then the result is negative. This method is used in the Quick Hull algorithm.
     *
     * @param in_pLinePoint1 - A point on the line.
     *
     * @param in_pLinePoint2 - Another point on the line (not equal to in_pLinePoint1).
     *
     * @param in_pPoint - The point from which to calculate the distance to the line.
     *
     * @return The calculated distance.
     */
    float CalcDistanceToLine(const float2 in_pLinePoint1, const float2 in_pLinePoint2, const float2 in_pPoint);

    /**
     * @fn DeserializeReceivedBuffer()
     *
     * @brief De-serializes a buffer that comes from the host.
     *
     * @param in_pReceivedData - Data received from the host.
     *
     * @param out_verticesCount - Number of vertices in the boat geometry.
     *
     * @param out_pVertices - Vertices of the boat in the boat geometry.
     *
     * @param out_indicesType - The type of indices used to store boat mesh triangles indices in the vertices array.
     *
     * @param out_indicesCount - The number of triangle indices in the vertices array.
     *
     * @param out_pIndices - The triangle indices in the vertices array.
     */
    void  DeserializeReceivedBuffer( const void* in_pReceivedData,
        unsigned int &out_verticesCount, 
        float3* &out_pVertices, 
        collada::ScalarType &out_indicesType, 
        unsigned int &out_indicesCount, 
        void* &out_pIndices );

    /**
     * @fn GetSingleGridIndex()
     *
     * @brief Checks whether a given point is located inside of a multigrid fluid surface.
     *
     * @param x - The X coordinate of the input point.
     *
     * @param y - The Y coordinate of the input point.
     *
     * @return - The index of the grid where the (x,y) point is located or -1 if there is no such grid.
     */
    int GetSingleGridIndex(float x, float y);

    /**
     * @fn IsPointReachableForBoat()
     *
     * @brief Checks whether or not the boat is able to move to the given point.
     *
     * @param in_point - The point for the reachability check.
     *
     * @return - TRUE if the given point is reachable.
     *           FALSE otherwise
     */
    bool IsPointReachableForBoat(float2 in_point);

    /**
     * @fn GetSurfaceHeightAt()
     *
     * @brief Gets the fluid height for the multigrid at the given coordinates
     *
     * @param x - The X coordinate of the input point.
     *
     * @param y - The Y coordinate of the input point.
     *
     * @return - The height of the surface at the (x,y) point.
     */
    float GetSurfaceHeightAt(int x, int y);

    /**
     * @fn GetSurfaceHeightAt()
     *
     * @brief Gets the fluid height for the multigrid at the given coordinates with a specified grid index.
     *
     * @note Use of the index will help to improve performance when the index of the grid was previously calculated.
     *
     * @param x - The X coordinate of the input point.
     *
     * @param y - The Y coordinate of the input point.
     *
     * @param index - The index of the grid where the (x,y) point is located.
     *
     * @return - The height of the surface at the (x,y) point.
     */
    float GetSurfaceHeightAt(int x, int y, int index);

    /**
     * @fn GetBottomAt()
     *
     * @brief Gets the bottom height for the multigrid at the given coordinates.
     *
     * @param x -  The X coordinate of the input point.
     *
     * @param y - The Y coordinate of the input point.
     *
     * @return - The bottom value at the given point.
     */
    float GetBottomAt (int x, int y);

    /**
     * @fn GetBottomAt()
     *
     * @brief Gets the bottom height for the multigrid at the given coordinates with a specified grid index
     *
     * @note Use of the index will help to improve performance when the index of the grid was previously calculated.
     *
     * @param x - The X coordinate of the input point.
     *
     * @param y - The Y coordinate of the input point.
     *
     * @param index - The index of the grid where the (x,y) point is located.
     *
     * @return - The bottom value at the given point.
     */
    float GetBottomAt (int x, int y, int index);

    /**
     * @fn AddForce()
     *
     * @brief Adds new forces at the specified coordinates.
     *
     * @param in_x - The X coordinate of the point where force should be applied.
     *
     * @param in_y - The Y coordinate of the point where force should be applied.
     *
     * @param in_forceValue - The force value to apply at the given coordinates.
     */
    void  AddForce(float in_x, float in_y, float in_forceValue);

    /**
     * @fn SpecifyNewDestinationPoint()
     *
     * @brief Updates the boat movement parameters when the user clicks on a new destination point.
     *
     * @param in_pDataFromHost - Parametric data received from the host.
     */
    void SpecifyNewDestinationPoint(const WATER_STRUCT* in_pDataFromHost);

    /**
     * @fn MoveUsingAnimation()
     *
     * @brief Move the boat according to the animation coordinates.
     *
     * @param in_pDataFromHost - Parametric data received from the host.
     */
    void MoveUsingAnimation(const WATER_STRUCT* in_pDataFromHost);

public:

    /**
     * @fn Boat()
     *
     * @brief Boat contructor.
     *
     * @param in_pDataFromHost - Serialized data received from the host.
     *
     * @param in_pScene - The fluid surface 2D scene where the boat should be located.
     */
    Boat(const void* in_pDataFromHost, CFlux2DScene*  in_pScene, float x_boat_pos, float y_boat_pos);

    /**
     * @fn Move()
     *
     * @brief Moves the boat to the next position and updates the value of m_pTransformationMatrix.
     *
     * @param in_pDataFromHost - Parametric data received from the host.
     *
     * @param in_pScene - The fluid surface 2D scene.
     */
    void Move(const WATER_STRUCT* in_pDataFromHost,const CFlux2DScene* in_pScene);

    /**
     * @fn GetAnimationState()
     *
     * @brief Returns the current state of the animation indicating whether or not the animation is frozen.
     *
     * @return - TRUE if the animation is frozen.
     *           FALSE if it is not frozen.
     */
    bool  GetAnimationState() 
    {
        return m_isAnimationFrozen;
    };

    /**
     * @fn GetTransformationMatrix()
     *
     * @brief Returns the current transformation matrix for boat geometry.
     *
     * @return m_pTransformationMatrix - The transformation matrix for boat geometry.
     */
    float* GetTransformationMatrix()
    {
        return m_pTransformationMatrix;
    };
    /**
     * @fn ~Boat()
     *
     * @brief Class destructor
     */
    ~Boat();
};

#endif
