/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file mathstub.h

#pragma once

#if 0

#include "mathlib.h"

#else

#include <D3DX10math.h>
typedef D3DXVECTOR2 float2;
typedef D3DXVECTOR3 float3;
typedef D3DXVECTOR4 float4;
typedef D3DXMATRIX float4x4;

#define MatrixMultiply          D3DXMatrixMultiply
#define Vec3Add                 D3DXVec3Add
#define Vec3Length              D3DXVec3Length
#define Vec3TransformCoord      D3DXVec3TransformCoord
#define Vec3TransformNormal     D3DXVec3TransformNormal
#define Vec3Maximize            D3DXVec3Maximize
#define Vec3Minimize            D3DXVec3Minimize
#define MatrixIdentity          D3DXMatrixIdentity
#define MatrixInverse           D3DXMatrixInverse
#define MatrixTranslation       D3DXMatrixTranslation
#define MatrixScaling           D3DXMatrixScaling
#define MatrixTranspose         D3DXMatrixTranspose
#define MatrixRotationAxis      D3DXMatrixRotationAxis
#define MatrixRotationZ         D3DXMatrixRotationZ
#define MatrixPerspectiveFovLH  D3DXMatrixPerspectiveFovLH
#define MatrixDeterminant       D3DXMatrixDeterminant
#define MatrixRotationYawPitchRoll D3DXMatrixRotationYawPitchRoll
#define Vec3Cross               D3DXVec3Cross
#define Vec3Dot                 D3DXVec3Dot
#define Vec4Transform           D3DXVec4Transform
#define Vec2Length              D3DXVec2Length
#define Vec2CCW                 D3DXVec2CCW
#define Vec2Dot                 D3DXVec2Dot
#define MatrixRotationX         D3DXMatrixRotationX
#define MatrixRotationY         D3DXMatrixRotationY
#pragma comment( lib, "d3dx10.lib" )

#endif


// end of file
