/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2007-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/**
 * @file boatlogic.cpp
 *
 * @brief Implements parameters and classes to implement boat logic and the fluid-boat interaction
 */


#include "boatlogic.h"
#include <new>

/**
 * @def SAFE_FREE
 *
 * @brief Safely frees memory allocated 
 */

#define SAFE_FREE(POINTER_TO_OBJECT)\
    if((POINTER_TO_OBJECT) != NULL)\
    free(POINTER_TO_OBJECT);

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
float4 Boat::GetCenterOfGravity( float3* in_pVertices, 
    unsigned int in_verticesCount, 
    ScalarType const in_indicesType, 
    unsigned int const in_indicesCount, 
    void* const in_indices )
{
    // Declare a temporary variable to store mesh triangles
    float3 pTriangleVertices[3];

    // Assign the center of gravity with initial value
    float4 CenterOfGravity(0,0,0,0);

    // Declare a temporary variable to store the center of gravity of a triangle
    float4 TriangleGravityCenter; 

    // Indices can be stored as uint16 or uint32
    if(in_indicesType == ST_UINT32)
    {
        // Get indices array according to its type
        unsigned int *indices32 = (unsigned int*)in_indices;

        // Process the triangles
        for(unsigned int i = 0; i<in_indicesCount; i+=3)
        {
            // Get the triangle's vertices
            pTriangleVertices[0] = in_pVertices[indices32[i    ]];
            pTriangleVertices[1] = in_pVertices[indices32[i + 1]];
            pTriangleVertices[2] = in_pVertices[indices32[i + 2]];

            // Calculate the current triangle's center of gravity
            TriangleGravityCenter = GetTriangleGravityCenter(pTriangleVertices);

            // Add the calculated mass to the global mass sum and to the mass sum for each coordinate
            CenterOfGravity.w += TriangleGravityCenter.w;
            CenterOfGravity.x += TriangleGravityCenter.x*TriangleGravityCenter.w;
            CenterOfGravity.y += TriangleGravityCenter.y*TriangleGravityCenter.w;
            CenterOfGravity.z += TriangleGravityCenter.z*TriangleGravityCenter.w;
        }
    }
    else if(in_indicesType == ST_UINT16)
    {
        // Get indices array according to its type
        unsigned short *indices16 = (unsigned short*)in_indices;

        // Process the triangles
        for(unsigned int i = 0; i<in_indicesCount; i+=3)
        {
            // Get triangle vertices
            pTriangleVertices[0] = in_pVertices[indices16[i    ]];
            pTriangleVertices[1] = in_pVertices[indices16[i + 1]];
            pTriangleVertices[2] = in_pVertices[indices16[i + 2]];

            // Calculate the current triangle's center of gravity
            TriangleGravityCenter = GetTriangleGravityCenter(pTriangleVertices);

            // Add the calculated mass to the global mass sum and to the mass sum for each coordinate
            CenterOfGravity.w += TriangleGravityCenter.w;
            CenterOfGravity.x += TriangleGravityCenter.x*TriangleGravityCenter.w;
            CenterOfGravity.y += TriangleGravityCenter.y*TriangleGravityCenter.w;
            CenterOfGravity.z += TriangleGravityCenter.z*TriangleGravityCenter.w;
        }
    }
    // Unexpected type of indices.
    else
    {
        throw "Unexpected indices type!";
    }


    // The last task is to calculate the center of gravity for the boat by averaging masses
    CenterOfGravity.x = CenterOfGravity.x / CenterOfGravity.w;
    CenterOfGravity.y = CenterOfGravity.y / CenterOfGravity.w;
    CenterOfGravity.z = CenterOfGravity.z / CenterOfGravity.w;

    return CenterOfGravity;
}



/**
 * @fn CalcNextBoatPosition()
 *
 * @brief Updates boat position parameters according to data sent from the host.
 *
 * @param in_pDataFromHost - Parameter structure received from the host.
 */
void Boat::CalcNextBoatPosition(const WATER_STRUCT* in_pDataFromHost)
{
    // If the user clicked the mouse in order to move the boat.
    if(in_pDataFromHost->wasClicked)
    {
        SpecifyNewDestinationPoint(in_pDataFromHost);
    }

    // If the user clicks on a point in the water, the boat will move to that point.
    // When the boat riches the destination point, the animation from the collada is
    // used to get the boat's path..
    if(m_isAnimationFrozen)
    {
        // The user clicked. The animation is frozen.
        MoveWithoutAnimation(in_pDataFromHost);
    }
    else
    {
        // Move the boat according to the coordinates in the animation from the host.
        MoveUsingAnimation(in_pDataFromHost);
    }
}

/**
 * @fn SpecifyNewDestinationPoint()
 *
 * @brief Updates the boat movement parameters when the user clicks on a new destination point.
 *
 * @param in_pDataFromHost - Parametric data received from the host.
 */
void Boat::SpecifyNewDestinationPoint(const WATER_STRUCT* in_pDataFromHost)
{
    // Stop the boat animation
    m_isAnimationFrozen = true;

    // Save the place where the user clicked.
    m_destination.x = in_pDataFromHost->x;
    m_destination.y = in_pDataFromHost->y;

    // Calculate a new reference point
    m_referencePoint = m_referencePoint +  m_offset;

    // Check to see if the user clicked on the point where the boat is located.
    if(m_destination != m_referencePoint)
    {
        // Calculate distance to reach destination point.
        float2 MoveVector = m_referencePoint - m_destination;
        float l = Vec2Length(&MoveVector);

        m_horizontalAcceleration = BOAT_ACCELERATION;

        // The initial speed for boat won't be zero,  this is an aid when the boat starts from the shoal
        m_horizontalSpeed = START_BOAT_SPEED;

        // Calculate the Y angle
        m_yAngle = -asin((m_referencePoint.x - m_destination.x)/l);

        // An asin function returns the angle between 0 and PI, so the other angles should be re-calculated 
        if(m_destination.y < m_referencePoint.y) m_yAngle = -m_yAngle - M_PI;
    }
    else
    {
        // No actions are needed. Continue the animation.
        m_isAnimationFrozen = false;
    }
}

/**
 * @fn MoveWithoutAnimation()
 *
 * @brief Moves the boat according to a user mouse click.
 *
 * @param  in_pDataFromHost - Parameters received from the host structure.
 */
void Boat::MoveWithoutAnimation(const WATER_STRUCT* in_pDataFromHost)
{   
    float2 nextPosition;
    m_wasAnimationFrozen = true;
    m_offset.x = 0.0f;
    m_offset.y = 0.0f;

    // Calculate the coordinates that will be in the next step
    nextPosition.x = m_referencePoint.x + m_horizontalSpeed * sin( m_yAngle) * TIME_STEP;
    nextPosition.y = m_referencePoint.y + m_horizontalSpeed * cos( m_yAngle) * TIME_STEP;

    // Get the next coordinates step
    float2 NextStepMoveVector = m_destination - nextPosition;
    // Get the previous coordinates step
    float2 PreviousStepMoveVector = m_destination - m_referencePoint;

    // Check to see if the destination point was reached during the previous step
    if(Vec2Length(&NextStepMoveVector) > Vec2Length(&PreviousStepMoveVector))
    {
        // Stop the boat
        m_horizontalSpeed = 0.0f;
        m_horizontalAcceleration = 0.0f;
        nextPosition = m_destination;
        m_referencePoint = m_destination;
        // Now apply the animation
        m_isAnimationFrozen = false;
    }

    float newSurface;

    // Get the depth in the next point
    if(IsPointReachableForBoat(nextPosition))
    {
        //newBottom = GetBottomAt((int)nextPosition.x,(int)nextPosition.y);
        newSurface = GetSurfaceHeightAt((int)nextPosition.x,(int)nextPosition.y);
    }
    else
    {
        //newBottom = 0;
        newSurface = 0;
    }

    // If the depth is too low
    if((newSurface < 0.4f) && !((newSurface != 0.0f) && (m_numberOfForces == 0)))
    {
        // Stop the boat
        m_horizontalSpeed = 0.0f;
        m_horizontalAcceleration = 0.0f;
    }

    // If the user clicked the mouse
    if(in_pDataFromHost->wasClicked)
    {
        // Perform the step even if the boat is on the shoal. This is useful when the user trys to go out from the shoal.
        m_referencePoint = nextPosition;
        nextPosition.x = m_referencePoint.x + m_horizontalSpeed * sin( m_yAngle) * TIME_STEP;
        nextPosition.y = m_referencePoint.y + m_horizontalSpeed * cos( m_yAngle) * TIME_STEP;
    }

    // Calculate the new speed
    m_horizontalSpeed += m_horizontalAcceleration * TIME_STEP;
    m_horizontalSpeed -= m_horizontalSpeed * AIR_FRICTION_COEFFITIENT / m_CenterOfGravity.w;

    // Calculate new coordinates for the boat
    m_referencePoint.x = m_referencePoint.x + m_horizontalSpeed * sin( m_yAngle) * TIME_STEP;
    m_referencePoint.y = m_referencePoint.y + m_horizontalSpeed * cos( m_yAngle) * TIME_STEP;
}

/**
 * @fn MoveUsingAnimation()
 *
 * @brief Move the boat according to the animation coordinates.
 *
 * @note In this function, the X coordinate will be inverted in the animation.
 *       This is done because of Z coordinate direction differences between
 *       the Collada and DirectX
 *
 * @param in_pDataFromHost - Parametric data received from the host.
 */
void Boat::MoveUsingAnimation(const WATER_STRUCT* in_pDataFromHost)
{
    // For the next boat coordinates.
    float2 nextPosition;
    // Anf for the previous boat coordinates.
    float2 previousPosition;
    float2 newOffset;
    float2 animationPosition;

    previousPosition = m_referencePoint + m_offset;

    // The start angle of the animation should be equal to the animation angle of the previous movement.
    // The calculation of the initial value depends on the state of the m_calcStartAnimationAngle flag.
    if(m_calcStartAnimationAngle)
    {
        // The start animation angle is calculated below. There is no need to calculate it in the next frame.
        m_calcStartAnimationAngle = false;
        // Calculate The next position if the start animation angle is zero.
        float2 nextPositionWithoutRotation(
            -in_pDataFromHost->boatTransform[12] * BOAT_ANIMATION_MULTIPLIER - m_startAnimationPoint.x,
            in_pDataFromHost->boatTransform[14] * BOAT_ANIMATION_MULTIPLIER - m_startAnimationPoint.y);

        // Calculate the start animation angle.
        float l = Vec2Length(&nextPositionWithoutRotation);
        if(l!=0)
        {
            m_startAnimationAngle = -asin(-nextPositionWithoutRotation.x/l);
            if(nextPositionWithoutRotation.x < 0) m_startAnimationAngle = - m_startAnimationAngle - M_PI;
            m_startAnimationAngle = m_startAnimationAngle - m_yAngle;
        }
        else
        {
            m_startAnimationAngle = m_yAngle;
        }
    }

    // The new boat offset according to the animation matrix from the host
    if(m_wasAnimationFrozen)
    {
        m_calcStartAnimationAngle = true;
        m_startAnimationAngle = 0;
        m_startAnimationPoint.x = -in_pDataFromHost->boatTransform[12] * BOAT_ANIMATION_MULTIPLIER;
        m_startAnimationPoint.y = in_pDataFromHost->boatTransform[14] * BOAT_ANIMATION_MULTIPLIER;
        m_wasAnimationFrozen = false;
    }

    // Calculate the animation position  and the start animation point according to the new data from the host
    animationPosition.x = -in_pDataFromHost->boatTransform[12] * BOAT_ANIMATION_MULTIPLIER - m_startAnimationPoint.x;
    animationPosition.y = in_pDataFromHost->boatTransform[14] * BOAT_ANIMATION_MULTIPLIER - m_startAnimationPoint.y;

    // Rotate the animation position by m_startAnimationAngle
    newOffset.x = animationPosition.x * cos(m_startAnimationAngle) + animationPosition.y * (-sin(m_startAnimationAngle));
    newOffset.y = animationPosition.x * sin(m_startAnimationAngle) + animationPosition.y * cos(m_startAnimationAngle);

    // Calculate the coordinates in the next step
    nextPosition = m_referencePoint + newOffset;

    float2 MoveVector = nextPosition - previousPosition;

    // Calculate the angle of movement for the boat.
    float l = Vec2Length(&MoveVector);
    if(l != 0)
    {
        // Calculate the Y angle
        m_yAngle = -asin((previousPosition.x - nextPosition.x)/l);
        // The asin function returns an angle between 0 and PI, so all another angles should be re-calculated 
        if(nextPosition.y < previousPosition.y) m_yAngle = -m_yAngle - M_PI;
    }

    m_offset = newOffset;

    // Calculate the bottom for the next step.
    float newSurface;
    if(IsPointReachableForBoat(nextPosition))
    {
        //newBottom = GetBottomAt((int)nextPosition.x, (int)nextPosition.y);
        newSurface = GetSurfaceHeightAt((int)nextPosition.x, (int)nextPosition.y);
    }
    else
    {
        newSurface = 0;
    }

    // If the bottom is too low
    //if(newBottom < 0.2)
    if(newSurface < 0.2)
    {
        // Stop the boat
        m_referencePoint = nextPosition;
        m_offset.x = 0.0f;
        m_offset.y = 0.0f;
        m_isAnimationFrozen = true;
        nextPosition = previousPosition;
    }

    float2 t = nextPosition - previousPosition;
    m_horizontalSpeed = Vec2Length(&t) / TIME_STEP;
}

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
void Boat::GetWaterLinePoints( const float3* in_pVertices,
    const unsigned int in_verticesCount, 
    float3* &out_pWaterLinePoints, 
    unsigned int &out_waterLinePointsCount )
{
    float waterPointsCoeffitientsSum = 0;
    // The Convex Hull calculation algorithm. To find the convex hull, a quick hull algorithm is used.
    // The algorithmn uses extreme points of the boat's points array for the calculation of the 
    // indices of the extreme points in the boat's projection to the water surface
    unsigned int    back = 0,
        front = 0,
        right = 0,
        left = 0;

    // Calculating the extreme indices
    for(unsigned int i = 1;i<in_verticesCount;i++)
    {
        if(in_pVertices[right].x    < in_pVertices[i].x) right = i; 
        if(in_pVertices[left].x     > in_pVertices[i].x) left = i; 
        if(in_pVertices[front].z    < in_pVertices[i].z) front = i; 
        if(in_pVertices[back].z     > in_pVertices[i].z) back = i; 
    }

    // To define the convex hull, you use an array that contains the indices of the convex hull points
    // If a point with the I index in the in_pVertices array belongs to the convex hull then pIsInHull[I]
    // contains its index in clockwise order. The front point has index 1. If the point with the I index
    // doesn't belong to the convex hull then pIsInHull[I] contains 0.
    unsigned int* pIsInHull;

    // Allocate memory to store the vertices
    pIsInHull = (unsigned int*)malloc(sizeof(unsigned int)*in_verticesCount); 
    memset(pIsInHull, 0, sizeof(unsigned int) * in_verticesCount);

    // Set the front point index in clockwise order as the first
    pIsInHull[front] = 1;

    // Minimal unused positive index in the pIsInHull array. The number of points in the convex hull is nextHullPointIndex - 1.
    unsigned int nextHullPointIndex = 2;

    // Initialize 2d copies of the extreme points
    float2 frontPointProjection = float2(in_pVertices[front].x, in_pVertices[front].z);
    float2 rightPointProjection = float2(in_pVertices[right].x, in_pVertices[right].z);
    float2 leftPointProjection  = float2(in_pVertices[left].x, in_pVertices[left].z);
    float2 backPointProjection = float2(in_pVertices[back].x, in_pVertices[back].z);

    // Iterations of the quick hull algorithm
    nextHullPointIndex = QuickHull(frontPointProjection, rightPointProjection, in_pVertices, pIsInHull, in_verticesCount, nextHullPointIndex);
    if(pIsInHull[right] == 0) pIsInHull[right] = nextHullPointIndex++;
    nextHullPointIndex = QuickHull(rightPointProjection, backPointProjection, in_pVertices, pIsInHull, in_verticesCount, nextHullPointIndex);
    if(pIsInHull[back] == 0) pIsInHull[back] = nextHullPointIndex++;
    nextHullPointIndex = QuickHull(backPointProjection, leftPointProjection, in_pVertices, pIsInHull, in_verticesCount, nextHullPointIndex);
    if(pIsInHull[left] == 0) pIsInHull[left] = nextHullPointIndex++;
    nextHullPointIndex = QuickHull(leftPointProjection, frontPointProjection, in_pVertices, pIsInHull, in_verticesCount, nextHullPointIndex);

    unsigned int numberOfHullPoints = nextHullPointIndex-1;

    // Allocate memory to store the hull indices
    unsigned int* pHullIndices;
    pHullIndices = (unsigned int*)malloc(sizeof(unsigned int)*numberOfHullPoints);

    // Store them
    for(unsigned int i = 0; i<m_verticesCount; i++)
    {
        if(pIsInHull[i] != 0)
        {
            pHullIndices[pIsInHull[i] - 1] = i;
        }
    }

    // A variable for the length of the convex hull
    float HullLength = 0;

    // Calculate the length of the convex hull
    for(unsigned int i = 0; i<numberOfHullPoints-1; i++)
    {
        float2 HullSegment;
        HullSegment.x = in_pVertices[pHullIndices[i]].x - in_pVertices[pHullIndices[i+1]].x;
        HullSegment.y = in_pVertices[pHullIndices[i]].z - in_pVertices[pHullIndices[i+1]].z;
        HullLength += Vec2Length(&HullSegment);
    }

    // Go along the convex hull using a computed step based on the number of waterr line points.
    // After each step, an orthogonal offset is used to step out of the boat. The final point is:
    //
    //      (point on hull) + (offset that is orthogonal to the convex hull)
    //
    float step = HullLength / (NUMBER_OF_WATER_LINE_POINTS + 1);

    // Allocate memory for water line points and stored levels of water
    // Water line points contain points where the boat's influence on the water surface is quite low 
    out_pWaterLinePoints = (float3*)malloc(sizeof(float3)*NUMBER_OF_WATER_LINE_POINTS);
    
    m_pStoredWaterLevels = (float*)malloc(sizeof(float)*NUMBER_OF_WATER_LINE_POINTS);

    // Traverse along the convex hull from segment to segment

    // Get the start hull segment point
    float2 startHullSegmentPoint;
    startHullSegmentPoint.x = in_pVertices[pHullIndices[0]].x;
    startHullSegmentPoint.y = in_pVertices[pHullIndices[0]].z;

    // Get the end hull segment point
    float2 endHullSegmentPoint;
    endHullSegmentPoint.x = in_pVertices[pHullIndices[1]].x;
    endHullSegmentPoint.y = in_pVertices[pHullIndices[1]].z;

    // Subtract the start from the finish to get the segment vector
    float2 hullSegmentVector = endHullSegmentPoint - startHullSegmentPoint;

    // Get the lenght of the segment vector
    float hullSegmentLength = Vec2Length(&hullSegmentVector);

    // Initialize working variables
    unsigned int hullSegmentIndex = 0;
    float hullSegmentOffset = 0;
    float hullOffset = 0;
    unsigned int waterLinePointIndex = 0;
    // The current part of the hull is a parametrized segment - 
    //
    //              alfa*A + (1-alfa)*B
    //
    // Where A and B are the end points of the segment

    float alfa;                             
    
    // Go along the hull until the start point is reached
    while(hullOffset + step < HullLength)
    {
        hullSegmentOffset += step;
        hullOffset += step;

        // If the current offset of the hull segment exceeds its length
        // then the next hull segment should be used
        while(hullSegmentOffset > hullSegmentLength)
        {
            hullSegmentIndex++;
            hullSegmentOffset -= hullSegmentLength;
            // set the start segment to the current end segment
            startHullSegmentPoint = endHullSegmentPoint;

            // Get the new end segment
            endHullSegmentPoint.x = in_pVertices[pHullIndices[hullSegmentIndex + 1]].x;
            endHullSegmentPoint.y = in_pVertices[pHullIndices[hullSegmentIndex + 1]].z;

            // Subtract to get the vector
            hullSegmentVector = endHullSegmentPoint - startHullSegmentPoint;

            // Then compute the vector length
            hullSegmentLength = Vec2Length(&hullSegmentVector);
        }

        // Calculate a new coefficient for segment parametrization
        alfa = hullSegmentOffset / hullSegmentLength;

        float3 OrthogonalVector;
        float3 PointOnHull;

        PointOnHull.x = alfa * startHullSegmentPoint.x + (1 - alfa) * endHullSegmentPoint.x;
        PointOnHull.y = 0; 
        PointOnHull.z = alfa * startHullSegmentPoint.y + (1 - alfa) * endHullSegmentPoint.y;

        OrthogonalVector.x = - hullSegmentVector.y / hullSegmentLength * UNAFFECTED_WATER_DISTANCE;
        OrthogonalVector.y = 0;
        OrthogonalVector.z = hullSegmentVector.x   / hullSegmentLength * UNAFFECTED_WATER_DISTANCE;

        // Do not take into account those points that are located in the back side of the boat
        if(OrthogonalVector.z <= 0.f)
        {
            out_pWaterLinePoints[waterLinePointIndex] = (PointOnHull * BOAT_PROJECTION_MULTIPLIER + OrthogonalVector);
            waterPointsCoeffitientsSum += OrthogonalVector.z - 0.4f;
            // Store the sum of the coefficients that will be used to distribute displaced water
            // volume among all points of the water surface that are located at front of the boat
            // in UNEFFECTED_WATER_DISTANCE distance. The value of the orthogonal vector is reduced
            // in order to adjust boat physics to the numerical method
            m_pWaterLinePointsMultipliers[waterLinePointIndex] = OrthogonalVector.z - 0.4f;
            m_pStoredWaterLevels[waterLinePointIndex] = 0.0f;
            waterLinePointIndex++;
        }
    }

    // Adjust physics to be stable
    for(unsigned int i = 0; i<waterLinePointIndex; i++)
    {
        m_pWaterLinePointsMultipliers[i] /= waterPointsCoeffitientsSum*SIDE_WAVES_DIVISOR;
    }

    out_waterLinePointsCount = waterLinePointIndex;
    SAFE_FREE(pIsInHull);
    SAFE_FREE(pHullIndices);
}


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
unsigned int Boat::QuickHull( const float2 in_pPoint1, 
    const float2 in_pPoint2, 
    const float3* in_pVertices, 
    unsigned int* &inout_pIsInHull,
    const unsigned int in_verticesCount,
    const unsigned int in_index )
{
    unsigned int currentIndex = in_index;
    // The algorithm is recursive. If the next point was founded then the Quick Hull function is run recursively
    //bool wasNextPointFounded = false;
    float maxDistanceToPoint = 0;
    // The initial value for the distance to the point. This means that the distance to other points will be greater.
    float currentDistanceToPoint = 0.0001f;
    // A value that can never present for the point index.
    unsigned int mostDistantPointIndex = 1000000;

    //float alfa = 0, beta = 0;

    // For every vertice...
    for(unsigned int i = 0; i<in_verticesCount; i++)
    {
        // Search for the 2D convex Hull. Get it from 3D coordinates.
        float2 verticesXZProjection = float2(in_pVertices[i].x, in_pVertices[i].z);
        // Calculate the distance from a point to a line defined by two given line points.
        // The result will be negative if the point wouldn't enlarge the existing convex hull.
        currentDistanceToPoint = CalcDistanceToLine(in_pPoint1, in_pPoint2, verticesXZProjection);
        // Update the maximal distance to the point
        if(currentDistanceToPoint >= maxDistanceToPoint)
        {
            maxDistanceToPoint = currentDistanceToPoint;
            mostDistantPointIndex = i;
        }
    }
    // Check to see if the most distant point was found. If so, recursively run the QuickHull function
    if(mostDistantPointIndex != 1000000)
    {
        float2 mostDistantPointXZProjection = float2(in_pVertices[mostDistantPointIndex].x, in_pVertices[mostDistantPointIndex].z);
        currentIndex = QuickHull(in_pPoint1, mostDistantPointXZProjection, in_pVertices, inout_pIsInHull, in_verticesCount, currentIndex);
        inout_pIsInHull[mostDistantPointIndex] = currentIndex;
        currentIndex ++;
        currentIndex = QuickHull(mostDistantPointXZProjection, in_pPoint2,in_pVertices, inout_pIsInHull, in_verticesCount, currentIndex);
    }

    return currentIndex;
}

/**
 * @fn GetDisplacedWaterVolume()
 *
 * @brief This method approximates the displaced fluid volume.
 *
 * @note The algorightm approximates the displaced water volume using one plane which
 *       will approximate the volume of water has been displaced by the boat
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
float Boat::GetDisplacedWaterVolume( const float3* in_pVertices, 
    const unsigned int in_verticesCount, 
    const float3* in_pWaterLinePoints, 
    const unsigned int in_waterLinePointsCount, 
    const void* in_pIndices,
    const unsigned int in_indicesCount,
    ScalarType in_IndicesType, 
    const float4x4 in_TransformationMatrix, 
    const CFlux2DGrid *in_pGrid )
{
    // Transform the vertices array according to the transformation matrix for collision geometry
    float4 vec;
    float4 TransformationResult;
    for(unsigned int j = 0; j<in_verticesCount; j++)
    {
        vec.x = in_pVertices[j].x;
        vec.y = in_pVertices[j].y;
        vec.z = in_pVertices[j].z;
        vec.w = 1.0f;
        Vec4Transform(&TransformationResult, &vec, &in_TransformationMatrix);
        m_pTransformedVertices[j].x = TransformationResult.x;
        m_pTransformedVertices[j].y = TransformationResult.y;
        m_pTransformedVertices[j].z = TransformationResult.z;
    }

    // Declare a vareiable to represent the volume to return.
    float displacedWaterVolume = 0;

    // A temporary array for storing water line points.
    float4 pWaterLinePoint4D;

    // A temporary array for string transformed water line point, 4D.
    float4 pTransformedWaterLinePoint4D;

    // The plane will be based on 3 points that will be calculated as an average of 3 water line points.
    float3 pCentersForPlane[3];
    memset(pCentersForPlane, 0, sizeof(float3)*3);

    // The central point's coordinates are an average values. That iss why it is important to choose the 
    // points for averaging. The number of points for averaging is stored in the variable below
    float pNumberOfCentralPoints[3];
    memset(pNumberOfCentralPoints, 0, sizeof(float)*3);

    // Approximate the plane that will represent the water level.
    for(unsigned int i = 0; i<in_waterLinePointsCount; i++)
    {
        /// Fill a 4D vector in order to transform the points.
        pWaterLinePoint4D.x = in_pWaterLinePoints[i].x;
        pWaterLinePoint4D.y = in_pWaterLinePoints[i].y;
        pWaterLinePoint4D.z = in_pWaterLinePoints[i].z;
        pWaterLinePoint4D.w = 1.0f;

        // Transform the water line points and write the result into a transformed water line points array.
        float4 transformedWaterLinePoint4D;
        Vec4Transform(&transformedWaterLinePoint4D, &pWaterLinePoint4D, &in_TransformationMatrix);
        m_pTransformedWaterLinePoints[i].x = transformedWaterLinePoint4D.x;
        m_pTransformedWaterLinePoints[i].y = transformedWaterLinePoint4D.y;
        m_pTransformedWaterLinePoints[i].z = transformedWaterLinePoint4D.z;

        // Get the grid index of the grid where the current water line point is located
        int index = GetSingleGridIndex(m_pTransformedWaterLinePoints[i].x, m_pTransformedWaterLinePoints[i].z);

        if(index != -1)
        {
            // If a grid was found, then calculate a new value for the water level
            float surface = GetSurfaceHeightAt((int)m_pTransformedWaterLinePoints[i].x, (int)m_pTransformedWaterLinePoints[i].z, index);
            float bottom = GetBottomAt((int)m_pTransformedWaterLinePoints[i].x, (int)m_pTransformedWaterLinePoints[i].z, index);


            m_pTransformedWaterLinePoints[i].y = surface - bottom;
            //m_pTransformedWaterLinePoints[i].y = surface;

            m_pStoredWaterLevels[i] = m_pTransformedWaterLinePoints[i].y;
        }
        else
        {
            // If the grid for a given water line point wasn't found, then use the stored water level.
            // (This happens when the boat is located near the grid border.)
            m_pTransformedWaterLinePoints[i].y = m_pStoredWaterLevels[i];
        }

        // The approximate water surface is represented by the plane. For calculating the
        // plane, three points are used, each point is the average of the coordinates of several points
        unsigned int centersIndex = i * (int)3 / (int)in_waterLinePointsCount;
        pNumberOfCentralPoints[centersIndex]++;
        pCentersForPlane[centersIndex] = pCentersForPlane[centersIndex] + m_pTransformedWaterLinePoints[i];
    }

    // Calculate the average points.
    pCentersForPlane[0] /= pNumberOfCentralPoints[0];
    pCentersForPlane[1] /= pNumberOfCentralPoints[1];
    pCentersForPlane[2] /= pNumberOfCentralPoints[2];

    // The plane for displaced water volume is calculated parametrically.
    // A point and 2 vectors are needed to represent the plane.
    float3 pPointForPlane[3];
    memcpy(pPointForPlane, pCentersForPlane, sizeof(float3)*3);

    float3 pVector1ForPlane;
    float3 pVector2ForPlane;
    pVector1ForPlane = pCentersForPlane[1] - pCentersForPlane[0];
    pVector2ForPlane = pCentersForPlane[2] - pCentersForPlane[0];

    // Check the type of indices
    if(in_IndicesType == ST_UINT32)
    {
        unsigned int *indices32 = (unsigned int*)in_pIndices;
        // The coordinates of the current triangle
        float3 pTriangleVertices[3];
        float pPlanePointsHeight[3];
        // For all indices
        for(unsigned int i = 0; i<in_indicesCount; i+=3)
        {
            pTriangleVertices[0] = m_pTransformedVertices[ indices32[i]  ];
            pTriangleVertices[1] = m_pTransformedVertices[ indices32[i+1]];
            pTriangleVertices[2] = m_pTransformedVertices[ indices32[i+2]];
            bool isTrianlgeUnderThePlane = true;
            for(int j = 0; j<3; j++)
            {
                // Calculate the height of the pPointForPlane projection to the triangle plane
                pPlanePointsHeight[j] = GetPointHeight(pPointForPlane, pPointForPlane[j].x, pPointForPlane[j].z);
                // If the boat triangle point is greater then the approximated water plane
                if(pTriangleVertices[j].y > pPlanePointsHeight[j])
                {
                    // Then the triangle is not under the plane
                    isTrianlgeUnderThePlane = false;
                }
            }

            // If the triangle is under the plane and if it is a bottom triangle, then this water column
            // should be replaced by this particular triangle. If it is a top triangle of the boat collision
            // geometry, then the water column should be reduced by the volume of the triangle cylinder.
            // This algorithm doesn't take into account those triangles that have intersections with the water plane.
            if(isTrianlgeUnderThePlane)
            {
                // Calculate the displaced water volume as the volume of the triangle cylinder

                // First, get the plane height
                float planeHeight = (pPlanePointsHeight[0]  + pPlanePointsHeight[1] + pPlanePointsHeight[2]) / 3.0f;
                // Then the triangle height
                float triangleHeight = (pTriangleVertices[0].y + pTriangleVertices[1].y + pTriangleVertices[2].y) / 3.0f;
                // Subtract to get the cylinder height
                float cylinderHeight = planeHeight - triangleHeight;
                // Set the height on the three triangle vertices
                pTriangleVertices[0].y = triangleHeight;
                pTriangleVertices[1].y = triangleHeight;
                pTriangleVertices[2].y = triangleHeight;
                // Then compute the area
                float triangleArea = GetTriangleArea(pTriangleVertices);

                // Now, compute vectors for triangles
                float2 firstTriangleVector;
                float2 secondTriangleVector;
                float crossProductResult;
                // Initialize the vectors by subtracting
                firstTriangleVector.x  = pTriangleVertices[1].x - pTriangleVertices[0].x;
                firstTriangleVector.y  = pTriangleVertices[1].z - pTriangleVertices[0].z;
                secondTriangleVector.x = pTriangleVertices[2].x - pTriangleVertices[0].x;
                secondTriangleVector.y = pTriangleVertices[2].z - pTriangleVertices[0].z;

                // Then compute the cross product
                crossProductResult = Vec2CCW(&firstTriangleVector, &secondTriangleVector);

                // Determine whether it is triangle that is on the top of the boat or if it is bottom triangle
                if(crossProductResult > 0)
                {
                    displacedWaterVolume+= triangleArea*cylinderHeight;
                }
                else
                {
                    displacedWaterVolume-= triangleArea*cylinderHeight;
                }
            }
        }
    }
    else if(in_IndicesType == ST_UINT16)
    {
        unsigned short *indices16 = (unsigned short*)in_pIndices;
        // The coordinates of the current triangle
        float3 pTriangleVertices[3];
        float pPlanePointsHeight[3];
        for(unsigned int i = 0; i<in_indicesCount; i+=3)
        {
            pTriangleVertices[0] = m_pTransformedVertices[ indices16[i] ];
            pTriangleVertices[1] = m_pTransformedVertices[indices16[i+1]];
            pTriangleVertices[2] = m_pTransformedVertices[indices16[i+2]];
            bool isTrianlgeUnderThePlane = true;

            for(int j = 0; j<3; j++)
            {
                // Calculate the height of the pPointForPlane projection to the triangle plane
                pPlanePointsHeight[j] = GetPointHeight(pPointForPlane, pPointForPlane[j].x, pPointForPlane[j].z);

                // If the boat's triangle point is greater then the approximated water plane
                if(pTriangleVertices[j].y > pPlanePointsHeight[j])
                {
                    // The triangle is not under the plane
                    isTrianlgeUnderThePlane = false;
                }
            }

            // If the triangle is under the plane and if it is a bottom triangle, then this water column
            // should be replaced by this particular triangle. If it is a top triangle of the boat collision
            // geometry, then the water column should be reduced by the volume of the triangle cylinder.
            // This algorithm doesn't take into account those triangles that have intersections with the water plane.
            if(isTrianlgeUnderThePlane)
            {
                // Compute the triangle area by subtracting the triangle height from the plane height
                // and assigning the value to the Y coordinate of the triangle vertices
                float planeHeight = (pPlanePointsHeight[0]  + pPlanePointsHeight[1] + pPlanePointsHeight[2]) / 3.0f;
                float triangleHeight = (pTriangleVertices[0].y + pTriangleVertices[1].y + pTriangleVertices[2].y) / 3.0f;
                float cylinderHeight = planeHeight - triangleHeight;
                pTriangleVertices[0].y = triangleHeight;
                pTriangleVertices[1].y = triangleHeight;
                pTriangleVertices[2].y = triangleHeight;
                float triangleArea = GetTriangleArea(pTriangleVertices);

                // Compute vectors
                float2 firstTriangleVector;
                float2 secondTriangleVector;
                float crossProductResult;
                firstTriangleVector.x  = pTriangleVertices[1].x - pTriangleVertices[0].x;
                firstTriangleVector.y  = pTriangleVertices[1].z - pTriangleVertices[0].z;
                secondTriangleVector.x = pTriangleVertices[2].x - pTriangleVertices[0].x;
                secondTriangleVector.y = pTriangleVertices[2].z - pTriangleVertices[0].z;

                // Take the cross product
                crossProductResult = Vec2CCW(&firstTriangleVector, &secondTriangleVector);
                
                // Determine whether it is a triangle on the top of the boat or it is a bottom triangle
                if(crossProductResult > 0)
                {
                    displacedWaterVolume+= triangleArea*cylinderHeight;
                }
                else
                {
                    displacedWaterVolume-= triangleArea*cylinderHeight;
                }
            }
        }
    }
    return displacedWaterVolume;
}

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
void Boat::GetGridPoints( const unsigned int in_verticesCount,
    const float3* in_pVertices, 
    const float4x4 in_verticesTransform, 
    const ScalarType in_indicesType, 
    const unsigned int in_indicesCount, 
    const void* in_indices, 
    float3* out_pForces, 
    unsigned int &out_numberOfForces )
{
    float  waterVolume;
    waterVolume = 0;

    float4 vec;
    float4 transformedVec4;
    float3 pTriangleVertices[3];

    // Transform the boat collision geometry according to the transformation matrix.
    for(unsigned int j = 0; j<in_verticesCount; j++)
    {
        vec.x = in_pVertices[j].x;
        vec.y = in_pVertices[j].y;
        vec.z = in_pVertices[j].z;
        vec.w = 1.0f;
        Vec4Transform(&transformedVec4, &vec, &in_verticesTransform);
        m_pTransformedVertices[j].x = transformedVec4.x;
        m_pTransformedVertices[j].y = transformedVec4.y;
        m_pTransformedVertices[j].z = transformedVec4.z;
    }

    float4 waterLinePoint4D;

    for(unsigned int i = 0; i<m_waterLinePointsCount; i++)
    {
        // Fill a 4D vector in order to transform the points
        waterLinePoint4D.x = m_pWaterLinePoints[i].x;
        waterLinePoint4D.y = m_pWaterLinePoints[i].y;
        waterLinePoint4D.z = m_pWaterLinePoints[i].z;
        waterLinePoint4D.w = 1.0f;

        // Transform the points and write the results into the transformed water line points array
        float4 transformedWaterLinePoint4D;
        Vec4Transform(&transformedWaterLinePoint4D, &waterLinePoint4D, &in_verticesTransform);
        m_pTransformedWaterLinePoints[i].x = transformedWaterLinePoint4D.x;
        m_pTransformedWaterLinePoints[i].y = transformedWaterLinePoint4D.y;
        m_pTransformedWaterLinePoints[i].z = transformedWaterLinePoint4D.z;
    }

    // Check for the right type for indices
    if(in_indicesType == ST_UINT32)
    {
        unsigned int *indices32 = (unsigned int*)in_indices;
        out_numberOfForces = 0;
        INT_BOUNDING_BOX CurBoundingBox;
        for(unsigned int i = 0; i<in_indicesCount; i+=3)
        {
            pTriangleVertices[0] = m_pTransformedVertices[indices32[i]];
            pTriangleVertices[1] = m_pTransformedVertices[indices32[i+1]];
            pTriangleVertices[2] = m_pTransformedVertices[indices32[i+2]];

            // Reduce the number of points to check with the help of the 2D bounding box of the triangle
            CurBoundingBox = GetTriangleBoundingBox(pTriangleVertices);

            // For each point in the bounding box
            for(int i = CurBoundingBox.minX; i<CurBoundingBox.maxX; i++)
            {
                for(int j = CurBoundingBox.minZ; j<CurBoundingBox.maxZ; j++)
                {
                    // Check to see whether this point is inside the triangle
                    if(IsPointInsideTriangle(pTriangleVertices, (float)i,(float)j))
                    {
                        // Force shouldn't be applayed in this point yet.
                        if(!Contains(out_pForces, out_numberOfForces, i,j))
                        {
                            // Calculate and apply force
                            float pointHeight = GetPointHeight(pTriangleVertices,(float)i,(float)j);
                            int   index = GetSingleGridIndex((float)i,(float)j);
                            float surfaceHeight;

                            // If the grid index is valid, we can compute the surface height
                            if(index != -1)
                            {
                                float surfaceH = GetSurfaceHeightAt(i,j);
                                float bottomH  = GetBottomAt(i,j);

                                surfaceHeight = surfaceH - bottomH;

                            }
                            else // (index == -1) and invalid grid
                            {
                                surfaceHeight = 0;
                            }

                            // Get the height difference
                            float tmp_height = pointHeight - surfaceHeight;

                            // If the difference is less than zero, add the force and decrement the water volume
                            if(tmp_height < 0)
                            {
                                out_pForces[out_numberOfForces].x = (float)i;
                                out_pForces[out_numberOfForces].y = (float)j;
                                out_pForces[out_numberOfForces].z = tmp_height * FORCE_MULTIPLIER;
                                out_numberOfForces++;
                                waterVolume -= tmp_height;
                            }
                        }
                    }
                }
            }
        }
    }

    if(in_indicesType == ST_UINT16)
    {
        unsigned short *indices16 = (unsigned short*)in_indices;
        out_numberOfForces = 0;
        INT_BOUNDING_BOX CurBoundingBox;
        for(unsigned int i = 0; i<in_indicesCount; i+=3)
        {
            pTriangleVertices[0] = m_pTransformedVertices[indices16[ i ]];
            pTriangleVertices[1] = m_pTransformedVertices[indices16[i+1]];
            pTriangleVertices[2] = m_pTransformedVertices[indices16[i+2]];

            /// Reduce the number of points to check with the help of the 2D bounding box of the triangle
            CurBoundingBox = GetTriangleBoundingBox(pTriangleVertices);

            // For each point in the bounding box.
            for(int i = CurBoundingBox.minX; i<CurBoundingBox.maxX; i++)
            {
                for(int j = CurBoundingBox.minZ; j<CurBoundingBox.maxZ; j++)
                {
                    // Check to see whether this point is inside the triangle.
                    if(IsPointInsideTriangle(pTriangleVertices, (float)i,(float)j))
                    {
                        // Force shouldn't be applayed in this point yet.
                        if(!Contains(out_pForces, out_numberOfForces, i,j))
                        {
                            // Calculate and apply force.
                            float pointHeight = GetPointHeight(pTriangleVertices,(float)i,(float)j);
                            int   index = GetSingleGridIndex((float)i,(float)j);
                            float surfaceHeight;

                            // If the grid index is valid, we can compute the surface height
                            if(index != -1)
                            {
                                float surfaceH = GetSurfaceHeightAt(i,j);
                                float bottomH  = GetBottomAt(i,j);

                                surfaceHeight = surfaceH - bottomH;

                            }
                            else // (index == -1) and invalid grid
                            {
                                surfaceHeight = 0;
                            }

                            // Get the height difference
                            float tmp_height = pointHeight - surfaceHeight;

                            // If the difference is less than zero, add the force and decrement the water volume
                            if(tmp_height < 0)
                            {
                                out_pForces[out_numberOfForces].x = (float)i;
                                out_pForces[out_numberOfForces].y = (float)j;
                                out_pForces[out_numberOfForces].z = tmp_height * FORCE_MULTIPLIER;
                                out_numberOfForces++;
                                waterVolume -= tmp_height;
                            }
                        }
                    }
                }
            }
        }
    }

    // If the water volumne is not zero, then we need to process the water line points
    if(waterVolume != 0) 
    {
        // For each water line point
        for(unsigned int i = 0; i<m_waterLinePointsCount; i++)
        {
            // If the transformed water line point is NOT already in the list of forces
            if(!Contains(out_pForces, out_numberOfForces, (int)m_pTransformedWaterLinePoints[i].x, (int)m_pTransformedWaterLinePoints[i].z))
            {
                // Add it
                out_pForces[out_numberOfForces].x = m_pTransformedWaterLinePoints[i].x;
                out_pForces[out_numberOfForces].y = m_pTransformedWaterLinePoints[i].z;
                out_pForces[out_numberOfForces].z = waterVolume*m_pWaterLinePointsMultipliers[i];
                out_numberOfForces++;
            }
        }
    }
}

/**
 * @fn GetCurrentBoatYHeight()
 *
 * @brief Returns the current boat height Y coordinate value 
 */
float Boat::GetCurrentBoatYHeight()
{
    // Update the vertical acceleration value
    m_verticalAcceleration = m_displacedWaterVolume * GRAVITY  * WATER_DENSITY / m_CenterOfGravity.w;
    m_verticalAcceleration -= GRAVITY;

    // Calculate the speed without taking into account the friction effect
    float speedWithoutFriction = m_verticalSpeed + TIME_STEP*m_verticalAcceleration;

    // Modify the vertical acceleration with friction
    m_verticalAcceleration -= m_verticalSpeed * WATER_FRICTION_COEFFITIENT / m_CenterOfGravity.w * m_numberOfForces;
    m_verticalAcceleration -= m_verticalSpeed * AIR_FRICTION_COEFFITIENT  / m_CenterOfGravity.w;

    // Calculate the new speed
    float speedWithFriction = m_verticalSpeed + TIME_STEP*m_verticalAcceleration;

    // Check to see if friction cause the boat to move backward.
    if((m_verticalSpeed != 0) && (Sign(speedWithFriction) != Sign(speedWithoutFriction)))
    {
        // If so, zero out the speed
        m_verticalSpeed = 0.0f;
    }
    else
    {
        m_verticalSpeed = speedWithFriction;
    }

    // Update the height value
    m_height += m_verticalSpeed*TIME_STEP;

    float height;

    // Get the bottom height at the reference point
    float bottom = GetBottomAt((int)m_referencePoint.x, (int)m_referencePoint.y);
    height = GetSurfaceHeightAt((int)m_referencePoint.x, (int) m_referencePoint.y);

    const float HALF_HEIGHT = 1.5f;

    if((m_height < -bottom + HALF_HEIGHT) && (m_verticalSpeed < 0.0))
    {
        m_height = - bottom + HALF_HEIGHT;
        m_verticalSpeed = 0.0f;
    }

    return m_height;
}

/**
 * @fn GetTriangleGravityCenter()
 *
 * @brief Calculates the center of gravity and the full mass of the boat on the assumption that
 *        the boat is an evenly distributed mass object.
 *
 * @param  in_pTriangle - Pointer to the array with the coordinates of the triangle's points.
 */
float4 Boat::GetTriangleGravityCenter(float3* const in_pTriangle)
{
    // Declare a variable for the center of gravity's coordinates
    float4 returnMassPoint;

    // Calculate the mass of the triangle
    returnMassPoint.w = GetTriangleArea(in_pTriangle) * BOAT_DENSITY * BOAT_THICKNESS;

    // Add averaged coordinates to the mass point
    returnMassPoint.x = (in_pTriangle[0].x + in_pTriangle[1].x + in_pTriangle[2].x) * 0.33333333f;
    returnMassPoint.y = (in_pTriangle[0].y + in_pTriangle[1].y + in_pTriangle[2].y) * 0.33333333f;
    returnMassPoint.z = (in_pTriangle[0].z + in_pTriangle[1].z + in_pTriangle[2].z) * 0.33333333f;

    return returnMassPoint;
}

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
short Boat::Sign(const float k)
{
    if(k<0) return -1;
    return 1;
}

/**
 * @fn GetTriangleBoundingBox()
 *
 * @brief Gets a 2 dimensional bounding box in the XZ projection for a 3-dimensional triangle.
 *
 * @param in_pVertices - The coordinates of the array with the coordinates of the triangle's points.
 */
INT_BOUNDING_BOX Boat::GetTriangleBoundingBox(float3* in_pVertices)
{
    // Calculate the bounding box for the triangle
    INT_BOUNDING_BOX BoxToReturn;
    BoxToReturn.maxY = (int)floor(in_pVertices[0].y) + 1;
    BoxToReturn.minY = (int)floor(in_pVertices[0].y);
    BoxToReturn.maxX = (int)floor(in_pVertices[0].x) + 1;
    BoxToReturn.minX = (int)floor(in_pVertices[0].x);
    BoxToReturn.maxZ = (int)floor(in_pVertices[0].z) + 1;
    BoxToReturn.minZ = (int)floor(in_pVertices[0].z);
    
    // For each coordinate
    for(int i = 0; i<3; i++)
    {
        // Check to see if the vertice value is greater than the maximum or less than the minimum.
        // If either case is true, adjust the bounding box value accordingly
        if(in_pVertices[i].x>BoxToReturn.maxX) BoxToReturn.maxX = (int)floor(in_pVertices[i].x) + 1;
        if(in_pVertices[i].x<BoxToReturn.minX) BoxToReturn.minX = (int)floor(in_pVertices[i].x);
        if(in_pVertices[i].y>BoxToReturn.maxY) BoxToReturn.maxY = (int)floor(in_pVertices[i].y) + 1;
        if(in_pVertices[i].y<BoxToReturn.minY) BoxToReturn.minY = (int)floor(in_pVertices[i].y);
        if(in_pVertices[i].z>BoxToReturn.maxZ) BoxToReturn.maxZ = (int)floor(in_pVertices[i].z) + 1;
        if(in_pVertices[i].z<BoxToReturn.minZ) BoxToReturn.minZ = (int)floor(in_pVertices[i].z);
    }
    return BoxToReturn;
}

/**
 * @fn Contains()
 *
 * @brief Used to check if force exists in the coordinates X and Z.
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
bool Boat::Contains(const float3* in_forces, unsigned int in_numOfForces, int x, int z)
{
    // For every force
    for(unsigned int i = 0; i<in_numOfForces; i++)
    {
        // Check the X and Z coordinates to inclusion
        if((in_forces[i].x==(float)x)&&(in_forces[i].z==(float)z))
        {
            return true;
        }
    }
    return false;
}

/**
 * @fn IsPointInsideTriangle()
 *
 * @brief This method checks whether a point with X and Z coordinates is positioned inside a triangle.
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
bool Boat::IsPointInsideTriangle(const float3* in_triangle, float x, float z)
{
    bool result = true;
    float2 v1, v2;
    float newCrossProduct;
    float curCrossProduct;

    // Initialze working variables, subtracting the first triangle vertice coordinates from the point coordinates
    v1.x = x - in_triangle[0].x;
    v1.y = z - in_triangle[0].z;
    // Now, subtract the first triangle vertice coordinates from the second triangle vertice
    v2.x = in_triangle[1].x - in_triangle[0].x;
    v2.y = in_triangle[1].z - in_triangle[0].z;

    // Then compute the cross product of the two 2D vectors
    newCrossProduct = Vec2CCW(&v1, &v2);

    // Compute the 2D vector from the second triangle vertice to the point
    v1.x = x - in_triangle[1].x;
    v1.y = z - in_triangle[1].z;
    // And from the third triangle vertice to the second
    v2.x = in_triangle[2].x - in_triangle[1].x;
    v2.y = in_triangle[2].z - in_triangle[1].z;

    // And get their cross product
    curCrossProduct = Vec2CCW(&v1, &v2);

    // The point should be located between the (1,0) and (2,1) vectors
    if(newCrossProduct*curCrossProduct<0)
    {
        return false;
    }

    newCrossProduct = curCrossProduct;

    v1.x = x - in_triangle[2].x;
    v1.y = z - in_triangle[2].z;
    v2.x = in_triangle[0].x - in_triangle[2].x;
    v2.y = in_triangle[0].z - in_triangle[2].z;

    curCrossProduct = Vec2CCW(&v1, &v2);

    // The point should be located between (2,1) and (0,2) vectors
    if(newCrossProduct*curCrossProduct<0) 
    {
        return false;
    }

    return result;
}

/**
 * @fn GetPointHeight()
 *
 * @brief This method returns height of the point (x,z) from the plane defined by the 
 *        triangle with in_pTriangleVertices coordinates.
 *
 * @param  in_pTriangleVertices - The coordinates of the triangle.
 *
 * @param  i - The X coordinate of the point.
 *
 * @param  j - The Z coordinate of the point.
 *
 * @return - The height of the given point.
 */
float Boat::GetPointHeight(const float3* in_pTriangleVertices,float i,float j)
{
    float t1,t2;

    // Calculate the coefficient for a linear system of coordinates
    float x1 = in_pTriangleVertices[1].x - in_pTriangleVertices[0].x;
    float x2 = in_pTriangleVertices[2].x - in_pTriangleVertices[0].x;
    float y1 = in_pTriangleVertices[1].z - in_pTriangleVertices[0].z;
    float y2 = in_pTriangleVertices[2].z - in_pTriangleVertices[0].z;
    float xc = i - in_pTriangleVertices[0].x;
    float yc = j - in_pTriangleVertices[0].z;
    GetSystemSolution(x1,x2,y1,y2,xc,yc,t1,t2);
    float newZ;

    // Calculate the z coordinate using the system solution of the linear system
    newZ = in_pTriangleVertices[0].y + (in_pTriangleVertices[1].y - in_pTriangleVertices[0].y)*t1 + (in_pTriangleVertices[2].y - in_pTriangleVertices[0].y)*t2;

    return newZ;
}

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
float Boat::CalcXAngle(CFlux2DGrid* pF, float x, float y)
{
    float delta = 0;
    float upYValue = 0;
    float downYValue = 0;
    float surface;
    float bottom;
    // Check 6 points and compare the values to the water surface summing the diferences.
    // Then approximate the X Angle for the boat using the average of this difference.
    for(int i = -3; i<3; i++)
    {

        int index1 = GetSingleGridIndex(x+i, y-3);
        int index2 = GetSingleGridIndex(x+i, y+3);

        if((index1 != -1) && (index2 != -1))
        {
            surface = GetSurfaceHeightAt((int)x+i, (int)y-3, index1);
            bottom  = GetBottomAt ((int)x+i, (int)y-3, index1);


            upYValue += __max(surface - bottom, -bottom);

            surface = GetSurfaceHeightAt((int)x+i, (int)y+3, index2);
            bottom  = GetBottomAt ((int)x+i, (int)y+3, index2);


            downYValue += __max(surface - bottom, -bottom);

        }
    }

    // Calculate The averages.
    upYValue /= 7;
    downYValue /= 7;
    // Get the difference
    delta = upYValue - downYValue;
    // Compute the angle
    float newXAngle = asin(delta / sqrt(36 + delta*delta));
    // And the angle step, the difference between the previous angle and the new angle
    float XAngleStep = newXAngle - m_previousXAngle;
    XAngleStep = max(-0.03f, min(0.03f,XAngleStep)); // This is a hack to minimize boat swinging.
    // Update the privious andgle value and return it
    m_previousXAngle = m_previousXAngle + XAngleStep;
    return m_previousXAngle;
}

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
float Boat::CalcYAngle(CFlux2DGrid* pF, float x, float y)
{
    float delta = 0;
    float upYValue = 0;
    float downYValue = 0;
    float surface;
    float bottom;

    // Check 6 points and compare the value to water surface, summing the differences.
    // Then approximate the Y Angle for the boat using the average of this difference.
    for(int i = -3; i<3; i++)
    {
        int index1 = GetSingleGridIndex(x-3, y+i);
        int index2 = GetSingleGridIndex(x+3, y+i);

        if((index1 != -1) && (index2 != -1))
        {
            surface = GetSurfaceHeightAt((int)x-3, (int)y+i, index1);
            bottom  = GetBottomAt ((int)x-3, (int)y+i, index1);

            
            upYValue += __max(surface - bottom, -bottom);

            surface = GetSurfaceHeightAt((int)x+3, (int)y+i, index2);
            bottom  = GetBottomAt ((int)x+3, (int)y+i, index2);


            downYValue += __max(surface - bottom, -bottom);

        }
    }
    // Calculate the averages
    upYValue /= 7;
    downYValue /= 7;
    // Get the difference
    delta = downYValue - upYValue;
    // Compute the new angle
    float newYAngle = asin(delta / sqrt(36 + delta*delta));
    // Compute the difference between this angle and the previous angle
    float YAngleStep = newYAngle - m_previousYAngle;
    YAngleStep = max(-0.03f, min(0.03f,YAngleStep));        // This is a hack to minimize boat swinging
    // Increment the previous angle and return it.
    m_previousYAngle = m_previousYAngle + YAngleStep;
    return m_previousYAngle;
}

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
float Boat::GetTriangleArea(float3* const in_pTriangle)
{
    float3 V1, V2, crossProd;
    float crossProdAbsolute;

    // Create a vector for each of two sides of the triangle
    V1 = in_pTriangle[1] - in_pTriangle[0];
    V2 = in_pTriangle[2] - in_pTriangle[0];

    // Compute the cross product of two sides, giving a perpendicular with magnitude
    Vec3Cross(&crossProd, &V1, &V2);
    // Compute the length of the cross product vector
    crossProdAbsolute = Vec3Length(&crossProd);

    // Then divide by two to get the area of the triangle
    return crossProdAbsolute/2;
}


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
void Boat::GetSystemSolution( float in_x1, 
    float in_x2, 
    float in_y1, 
    float in_y2, 
    float in_xc, 
    float in_yc, 
    float &out_t1, 
    float &out_t2 )
{
    float   detInv = 1.0f/(in_x1*in_y2-in_y1*in_x2 + 1e-6f); // (!!!)
    out_t1 = (in_xc*in_y2-in_yc*in_x2)*detInv;
    out_t2 = (in_x1*in_yc-in_y1*in_xc)*detInv;
}

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
float Boat::CalcDistanceToLine(const float2 in_pLinePoint1, const float2 in_pLinePoint2, const float2 in_pPoint)
{
    // Create a vector lying on the line
    float2 lineVector;
    lineVector = in_pLinePoint1 - in_pLinePoint2;

    // Create a vector based on the separate point and the second point in the line
    float2 pointToLineVector;
    pointToLineVector = in_pPoint - in_pLinePoint2;

    // Normalize the first vector
    float vectorAbsolute;
    vectorAbsolute = Vec2Length(&lineVector);
    lineVector = lineVector / vectorAbsolute;

    // Normalize the second vector
    vectorAbsolute = Vec2Length(&pointToLineVector);
    pointToLineVector = pointToLineVector/vectorAbsolute;

    // Calculate the cross product and the inner product
    float crossProd;
    float innerProd;

    innerProd = Vec2Dot(&pointToLineVector, &lineVector);
    crossProd = Vec2CCW(&pointToLineVector, &lineVector);

    // Get the length of the normalized point to line-point vector
    vectorAbsolute = Vec2Length(&lineVector);

    // If both the cross and inner products are greater than zero, we can 
    // compute the distance by dividing the cross product by the length of
    // the point to line vector
    if((crossProd>0) && (innerProd>0))
    {
        return crossProd / vectorAbsolute;
    }
    else
    {
        return -1;
    }
}

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
void Boat::DeserializeReceivedBuffer( const void* in_receivedData,
    unsigned int &out_verticesCount, 
    float3* &out_pVertices, 
    collada::ScalarType &out_indicesType, 
    unsigned int &out_indicesCount, 
    void* &out_pIndices )
{
    // Initialize the current data pointer with the start pointer of data from the host.
    // This pointer will move along the buffer.
    void* currentDataPointer = (void*)in_receivedData;
    // The first element of currentDataPointer contains the number of vertices
    memcpy(&out_verticesCount, currentDataPointer, sizeof(unsigned int));
    // Adjust currentDataPointer to point to the first element of data
    currentDataPointer = &((unsigned int*)currentDataPointer)[1];

    // Allocate memory for the output
    out_pVertices = (float3*)malloc(sizeof(float3)*out_verticesCount);

    // Copy the current data into the output
    memcpy(out_pVertices, currentDataPointer, out_verticesCount*sizeof(float3));

    // Point currentDataPointer to the end of the vertices, it now points at the index type
    currentDataPointer = &((float3*)currentDataPointer)[out_verticesCount];

    // Store the index type
    memcpy(&out_indicesType, currentDataPointer, sizeof(ScalarType));
    // And set currentDataPointer to point at the index count (the next element in the array)
    currentDataPointer = &((ScalarType*)currentDataPointer)[1];

    // Store the index count
    memcpy(&out_indicesCount, currentDataPointer, sizeof(unsigned int));
    // And advance currentDataPointer to point at the indices
    currentDataPointer = &((unsigned int*)currentDataPointer)[1];

    // Check the type of index and store indices accordingly
    if(out_indicesType == ST_UINT16)
    {
        out_pIndices = malloc(sizeof(unsigned short)*out_indicesCount);

        memcpy(out_pIndices, currentDataPointer, sizeof(unsigned short)*out_indicesCount);
    }

    if(out_indicesType == ST_UINT32)
    {
        out_pIndices = malloc(sizeof(unsigned int)*out_indicesCount);
        memcpy(out_pIndices, currentDataPointer, sizeof(unsigned int)*out_indicesCount);
    }
}

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
int Boat::GetSingleGridIndex(float x, float y)
{
    // Assume there is no right index for this task
    int index = -1;
    int numGrids = m_pScene->GetNumGrids();
    // For every grid
    for(int i = 0; i<numGrids; i++)
    {
        // Get the grid region
        CRegion region = m_pScene->GetRegion(i);
        // Check to see if the grid with index i contains the given point
        if((region.x1 <=x) && (x<region.x2) && (region.y1 <=y) && (y<region.y2))
        {
            // If it does, update the index
            index = i;
        }
    }
    return index;
}


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
bool Boat::IsPointReachableForBoat(float2 in_p)
{
    // This check means that we have sufficient offset (BOAT_TO_BORDER_DISTANCE is minimal)
    // to the end of the grid
    if((GetSingleGridIndex(in_p.x, in_p.y + BOAT_TO_BORDER_DISTANCE) != -1)&&
        (GetSingleGridIndex(in_p.x, in_p.y - BOAT_TO_BORDER_DISTANCE) != -1) &&
        (GetSingleGridIndex(in_p.x + BOAT_TO_BORDER_DISTANCE, in_p.y) != -1) &&
        (GetSingleGridIndex(in_p.x - BOAT_TO_BORDER_DISTANCE, in_p.y) != -1))
    {
        return true;
    }
    else
    {
        return false;
    }

}

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
float Boat::GetSurfaceHeightAt(int x, int y)
{
    int index = GetSingleGridIndex((float)x, (float)y);
    return GetSurfaceHeightAt(x, y, index);
}

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
float Boat::GetSurfaceHeightAt(int x, int y, int index)
{
    if(index != -1)
    {
        // If we have a grid index, get the associated region
        CRegion region = m_pScene->GetRegion(index);
        // Get the reciprocal of the grid step height and step width 
        float gridRcpHeight = m_pScene->GetGrid(index)->GetRcpGridStepH();
        float gridRcpWidth = m_pScene->GetGrid(index)->GetRcpGridStepW();
        // Then calculate the surface height
        return m_pScene->GetGrid(index)->GetCurrentSurface().m_heightMap.At( (int)((x-region.x1) * gridRcpWidth), (int)((y - region.y1) * gridRcpHeight));
    }
    else
    {
        return 0.0f;
    }
}

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
float Boat::GetBottomAt(int x, int y)
{
    int index = GetSingleGridIndex((float)x,(float)y);
    return GetBottomAt(x, y, index);
}

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
void Boat::AddForce(float in_x, float in_y, float multiplier)
{
    // Force should be added to each of the grids even if they overlap at the given point.

    // For every grid
    for(int i = 0; i<m_pScene->GetNumGrids(); i++)
    {
        CFlux2DGrid*            pG = m_pScene->GetGrid(i);
        CRegion                 R = m_pScene->GetRegion(i);
        DynamicMatrix<float>*   pH = &pG->GetCurrentSurface().m_heightMap;
        int                     W = pH->Width();
        int                     H = pH->Height();
        int                     bs = pH->Border();

        // Calculate the coordinates for the current grid
        float   xf = (in_x - R.x1)*pG->GetRcpGridStepW();
        float   yf = (in_y - R.y1)*pG->GetRcpGridStepH();
        int     xi = (int)xf;
        int     yi = (int)yf;

        if(xf<0)xi -= 1;
        if(yf<0)yi -= 1;

        // If we are still in the grid, apply force at this point
        if(xi>=-bs && xi<W+bs && yi>=-bs && yi < H+bs)
        {
            pH->At(xi,yi) += multiplier;
        }
    }
}

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
float Boat::GetBottomAt(int x, int y, int index)
{
    if(index != -1)
    {
        // If we have a valid grid index
        CRegion region = m_pScene->GetRegion(index);
        float gridRcpHeight = m_pScene->GetGrid(index)->GetRcpGridStepH();
        float gridRcpWidth = m_pScene->GetGrid(index)->GetRcpGridStepW();
        return m_pScene->GetGrid(index)->GetBottom().m_heightMap.At((int)((x - region.x1) * gridRcpWidth), (int)((y - region.y1) * gridRcpHeight));
    }
    else
    {
        return 0.0f;
    }
}

/**
 * @fn Boat()
 *
 * @brief Boat contructor.
 *
 * @param in_pDataFromHost - Serialized data received from the host.
 *
 * @param in_pScene - The fluid surface 2D scene where the boat should be located.
 */
Boat::Boat(const void* in_pDataFromHost, CFlux2DScene*  in_pScene,  float x_boat_pos, float y_boat_pos)
    : m_pScene(in_pScene),
    m_horizontalAcceleration(0.0f),
    m_horizontalSpeed(0.0f),
    m_isAnimationFrozen(false),
    m_yAngle(0.0f),
    m_verticalSpeed(0.0f),
    m_verticalAcceleration(0.0f),
    m_height(0.0f),
    m_numberOfForces(0),
    m_waterLinePointsCount(NUMBER_OF_WATER_LINE_POINTS),
    m_displacedWaterVolume(0.0f),
    m_startAnimationPoint(0,0),
    m_referencePoint(x_boat_pos,y_boat_pos),//m_referencePoint(50,110),
    m_offset(0,0),
    m_destination(0,0),
    m_calcStartAnimationAngle(true),
    m_CenterOfGravity(0,0,0,0),
    m_previousXAngle(0),
    m_previousYAngle(0),
    m_wasAnimationFrozen(true)
{
    // Topology deserialization
    DeserializeReceivedBuffer(in_pDataFromHost, m_verticesCount, m_pVertices, m_IndicesType, m_indicesCount, m_pIndices);

    // Calculate the center of gravity for the boat model
    m_CenterOfGravity = GetCenterOfGravity(m_pVertices, m_verticesCount, m_IndicesType, m_indicesCount, m_pIndices);

    // Allocate memory for the forces array
    m_pForces = (float3*)malloc(sizeof(float3)*MAX_NUMBER_OF_FORCES);

    // Allocate memory for transformed boat vertices
    m_pTransformedVertices = (float3*)malloc(sizeof(float3)*m_verticesCount);

    // Allocate memory for transformed water line point multipliers.
    // This array will store multipliers used for the determination of the water level boost according to any movement
    m_pWaterLinePointsMultipliers = (float*)malloc(sizeof(float)*m_waterLinePointsCount);

    // Get the coordinates of points where the water level will be checked on the next step.
    // The purpose of this is to correctly calculate displaced water volume
    // m_waterLinePoints is a one-dimenshional array with m_waterLinePointsCount*3 elements
    GetWaterLinePoints(m_pVertices, m_verticesCount, m_pWaterLinePoints, m_waterLinePointsCount);

    // Allocate memory for the transformed water line points.
    // This array will be used for by the displaced water volume calculation algorithm
    m_pTransformedWaterLinePoints = (float3*)malloc(sizeof(float3)*m_waterLinePointsCount);
}

/**
 * @fn Move()
 *
 * @brief Moves the boat to the next position and updates the value of m_pTransformationMatrix.
 *
 * @param in_pDataFromHost - Parametric data received from the host.
 *
 * @param in_pScene - The fluid surface 2D scene.
 */
void Boat::Move(const WATER_STRUCT* in_pDataFromHost, const CFlux2DScene* in_pScene)
{
    static float m_time = 0;

    // Fill in global variables for the boat position.
    CalcNextBoatPosition(in_pDataFromHost); 

    // Calculate the boat's position on the next step
    float2 newBoatPosition = m_referencePoint + m_offset;

    // Declare transformation matrices 
    float4x4 tempMatrix;
    float4x4 currentMatrix;

    // Get the water 2D grid
    CFlux2DGrid *pF = m_pScene->GetGrid();

    // Calculate the translation matrix
    MatrixTranslation(&m_pTransformationMatrix, newBoatPosition.x, GetCurrentBoatYHeight(), newBoatPosition.y);
    currentMatrix = m_pTransformationMatrix;

    // Calculate the rotation for X
    MatrixRotationX(&tempMatrix, CalcXAngle(pF, newBoatPosition.x, newBoatPosition.y));
    // And apply to the current matrix
    MatrixMultiply(&m_pTransformationMatrix, &tempMatrix, &currentMatrix);
    currentMatrix = m_pTransformationMatrix;

    // Calculate the rotation for Z
    MatrixRotationZ(&tempMatrix, CalcYAngle(pF, newBoatPosition.x, newBoatPosition.y));
    // And apply to the current matrix
    MatrixMultiply(&m_pTransformationMatrix, &tempMatrix, &currentMatrix);
    currentMatrix = m_pTransformationMatrix;

    // Calculate the rotation for Y
    MatrixRotationY(&tempMatrix, M_PI + m_yAngle);
    // And apply to the current matrix
    MatrixMultiply(&m_pTransformationMatrix, &tempMatrix, &currentMatrix);
    currentMatrix = m_pTransformationMatrix;

    // Calculate the grid points that are supposed to be in the force influence.
    // The boat area that touches the water surface is also calculated here
    GetGridPoints(m_verticesCount, m_pVertices, m_pTransformationMatrix, m_IndicesType, m_indicesCount, m_pIndices, m_pForces, m_numberOfForces);

    // Calculate the displaced water volume
    m_displacedWaterVolume = GetDisplacedWaterVolume(m_pVertices, m_verticesCount, m_pWaterLinePoints, m_waterLinePointsCount, m_pIndices, m_indicesCount, m_IndicesType, m_pTransformationMatrix, pF);

    // Determine any additional force multiplier
    float additionalForceMultiplier;
    // Calculate the boat speed, use pythogorean theorem
    float boatSpeed = sqrt((m_horizontalSpeed * m_horizontalSpeed) + (m_verticalSpeed * m_verticalSpeed));

    additionalForceMultiplier = min(FORCE_MULTIPLIER * boatSpeed, MAX_FORCE_VALUE); // Experimentally calculated values

    // For each force
    for(unsigned int i = 0; i<m_numberOfForces; i++)
    {
        AddForce(m_pForces[i].x, m_pForces[i].y, m_pForces[i].z * additionalForceMultiplier);
    }
}

/**
 * @fn ~Boat()
 *
 * @brief Class destructor
 */
Boat::~Boat()
{
    // Free all resources
    SAFE_FREE(m_pForces);
    SAFE_FREE(m_pVertices);
    SAFE_FREE(m_pTransformedVertices);
    SAFE_FREE(m_pIndices);
    SAFE_FREE(m_pWaterLinePoints);
    SAFE_FREE(m_pTransformedWaterLinePoints);
    SAFE_FREE(m_pStoredWaterLevels);
    SAFE_FREE(m_pWaterLinePointsMultipliers);
}
