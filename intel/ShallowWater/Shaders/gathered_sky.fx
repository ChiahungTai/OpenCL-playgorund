/* ************************************************************************* *\
*                INTEL CORPORATION PROPRIETARY INFORMATION
*   This software is supplied under the terms of a license agreement of
*   nondisclosure agreement with Intel Corporation and may not be copied
*   or disclosed except in accordance with the terms of that agreement.
*        Copyright (C) 2008-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

// This shader source file contains the gathered visualization shaders implementation

//--------------------------------------------------------------------------------------
// Global variables

Texture2D g_surfaceMap;
Texture2D g_bottomDiff;
Texture2D g_surfaceDiff;
Texture2D g_surfaceTexCoords[3];
TextureCube g_skyEnvMap;

Texture2D g_bottomGradient;
Texture2D g_surfaceGradient;

Texture2D g_wavesMap;

SamplerState g_samLinear
{
  Filter = MIN_MAG_MIP_LINEAR;
  AddressU = Wrap;
  AddressV = Wrap;
};

SamplerState g_samLinearClamp
{
  Filter = MIN_MAG_MIP_LINEAR;
  AddressU = Clamp;
  AddressV = Clamp;
};

float4x4 g_mWVP;
int3     g_indexXForm;
float3   g_normalScale;
float3   g_texBlendWeights;
float3   g_camPos;
float2   g_bottomScales;
float2   g_surfaceLocalNormalScale;

// DepthStates
DepthStencilState EnableDepth
{
  DepthEnable    = TRUE;
  DepthWriteMask = ALL;
  DepthFunc      = LESS_EQUAL;
};

// States
BlendState NoBlend
{
  BlendEnable[0] = FALSE;
};

BlendState Interpolate
{
  BlendEnable[0] = TRUE;
  SrcBlend = SRC_ALPHA;
  DestBlend = INV_SRC_ALPHA;
  BlendOp = ADD;
  SrcBlendAlpha = ZERO;
  DestBlendAlpha = ZERO;
  BlendOpAlpha = ADD;
};


// Vertex shader output structure
struct VSOutput
{
  float4 vPos     : SV_POSITION;
  float4 vDiffuse : COLOR0;
  float3 vNormal  : TEXCOORD0;
  float3 vUVec    : TEXCOORD1;
  float3 vVVec    : TEXCOORD2;
  float3 vEyeDir  : TEXCOORD3;
  float2 vTex[3]  : TEXCOORD4;  
  float  depth    : TEXCOORD7;
};

struct VSOutputBottom
{
  float4 vPos        : SV_POSITION;
  float4 vColor      : COLOR0;
  float2 vDiffuseTex : TEXCOORD0;  
  float2 vGradTex    : TEXCOORD1;
};

int3 VSMakeIndex(uint vtxIndex)
{
  return int3(vtxIndex & g_indexXForm.x, vtxIndex >> g_indexXForm.z, 0);
}

float VSLoadSurfaceHeight(int3 index)
{
    float height;
    float2 heightBottom = g_surfaceMap.Load(index);
#ifdef CONST_BOTTOM_SMALL_WAVE
    height = heightBottom.x;
#else // CONST_BOTTOM_SMALL_WAVE
    height = heightBottom.x - heightBottom.y;
#endif // CONST_BOTTOM_SMALL_WAVE
    return height;
}

float3 VSMakeSurfaceNormal(int3 index)
{
  int3 offset = index;
  offset.x = max(0, index.x - 1);
  float hL = VSLoadSurfaceHeight(offset);
  
  offset.x = min(g_indexXForm.x, index.x + 1);
  float hR = VSLoadSurfaceHeight(offset);

  offset.x = index.x;
  
  offset.y = max(0, index.y - 1);
  float hU = VSLoadSurfaceHeight(offset);

  offset.y = min(g_indexXForm.y, index.y + 1);
  float hD = VSLoadSurfaceHeight(offset);

  return normalize(float3(hL - hR, 1.0f, hU - hD) * g_normalScale);
}

void VSMakeSurfaceUVVectors(float3 vN, out float3 vU, out float3 vV)
{
  float3 vNSqr  = vN * vN;
  float2 vRSqrt = rsqrt(vNSqr.xz + vNSqr.yy);
  vU = float3(-1, 1, 0) * vN * vRSqrt.x;
  vV = float3(0, 1, -1) * vN * vRSqrt.y;
}

void VSSurface(float2 vPosXZ : POSITION, uint vtxIndex : SV_VertexID, out VSOutput vtx)
{
  int3 index = VSMakeIndex(vtxIndex);
    float height;
    float2 heightBottom = g_surfaceMap.Load(index);
#ifdef CONST_BOTTOM_SMALL_WAVE
    height = heightBottom.x;
#else // CONST_BOTTOM_SMALL_WAVE
    height = heightBottom.x - heightBottom.y;
#endif // CONST_BOTTOM_SMALL_WAVE
  float4 vPos = float4(vPosXZ.x, height - 0.05, vPosXZ.y, 1);

  float3 vNormal = VSMakeSurfaceNormal(index);
  
  [unroll] for (int i = 0; i < 3; i++)
    vtx.vTex[i] = g_surfaceTexCoords[i].Load(index);

  vtx.vPos = mul(vPos, g_mWVP);
  vtx.vEyeDir  = normalize(vPos.xyz - g_camPos);
  vtx.vDiffuse = 0.2f + vNormal.y;

  vtx.vNormal = vNormal;
  VSMakeSurfaceUVVectors(vNormal, vtx.vUVec, vtx.vVVec);

#ifdef CONST_BOTTOM_SMALL_WAVE
    vtx.depth = heightBottom.x + heightBottom.y;
#else // CONST_BOTTOM_SMALL_WAVE
    vtx.depth = heightBottom.x;
#endif // CONST_BOTTOM_SMALL_WAVE

}

[maxvertexcount(3)]
void GSSurfaceWithTriRemoving( triangle VSOutput input[3], inout TriangleStream<VSOutput> outStream )
{
  float sum = input[0].depth + input[1].depth + input[2].depth;
  if (sum > 0.0)
  {
        for (int v = 0; v < 3; v++)
          outStream.Append(input[v]);
  }
}

float4 PSSurface(VSOutput input) : SV_Target
{
  // Read advected dUdV from textures
  float4 dUdV = g_surfaceDiff.Sample(g_samLinear, input.vTex[0]) * g_texBlendWeights.x;
  dUdV += g_surfaceDiff.Sample(g_samLinear, input.vTex[1]) * g_texBlendWeights.y;
  dUdV += g_surfaceDiff.Sample(g_samLinear, input.vTex[2]) * g_texBlendWeights.z;
  // Make local normal
  float3 localNormal;
  localNormal.xz = 2 * dUdV.xy - 1;
  localNormal.y  = g_surfaceLocalNormalScale.x; // Local normal scale
  localNormal = normalize(localNormal);
  // Transform local normal to world position
  float3 globalNormal = localNormal.x * input.vUVec;
  globalNormal += localNormal.y * input.vNormal;
  globalNormal += localNormal.z * input.vVVec;
  // Reflection vector & Fresnel
  float dotEN = dot(input.vEyeDir, globalNormal);
  float3 refl = input.vEyeDir - 2 * dotEN * globalNormal; // Reflection
  float fresnelCoeff = 1 - dotEN;
  float fresPow2 = fresnelCoeff * fresnelCoeff; // power = 2
  fresnelCoeff = fresPow2 * fresPow2;     // power = 4
  fresnelCoeff *= fresPow2;               // power = 6
  fresnelCoeff = 1 / fresnelCoeff;
  // Get sky color
  float4 skyColor = g_skyEnvMap.Sample(g_samLinear, refl);
  // Form output color
  float4 outputColor = skyColor * input.vDiffuse;
  outputColor.w = fresnelCoeff;
  
  //add foam for the shallow water
  #define SHALLOW_START 1.5f
  if (input.depth < SHALLOW_START)
  {
    outputColor.rgb += lerp(float3(0, 0, 0), float3(0.7, 0.8, 0.8), (SHALLOW_START-input.depth)/SHALLOW_START);
    outputColor.w *= lerp(1, 0.7, (SHALLOW_START-input.depth)/SHALLOW_START);
  }
  
  
  return outputColor;
}

//--------------------------------------------------------------------------------------
// Bottom shaders

float3 VSMakeBottomNormal(int3 index)
{
  int3 offset = index;
  offset.x = max(0, index.x - 1);
  float hL = g_surfaceMap.Load(offset).y;
  
  offset.x = min(g_indexXForm.x, index.x + 1);
  float hR = g_surfaceMap.Load(offset).y;

  offset.x = index.x;
  
  offset.y = max(0, index.y - 1);
  float hU = g_surfaceMap.Load(offset).y;

  offset.y = min(g_indexXForm.y, index.y + 1);
  float hD = g_surfaceMap.Load(offset).y;

  return normalize(float3(hR - hL, 1.0f, hD - hU) * g_normalScale);
}

float4 VSMakeBottomColor(float3 normal)
{
  return normal.yyyy;
}

void VSBottom(float2 vPosXZ : POSITION, uint vtxIndex : SV_VertexID, out VSOutputBottom output)
{
  int3 index = VSMakeIndex(vtxIndex);

  float2 heightBottom = g_surfaceMap.Load(index);
  float4 vPos = float4(vPosXZ.x, -heightBottom.y, vPosXZ.y, 1);

  // make a little extrude for the main terrain (bottom)
  float3 normal = VSMakeBottomNormal(index);
  vPos.xyz += normal * 0.8;
  
  output.vPos   = mul(vPos, g_mWVP);
  
  output.vColor = VSMakeBottomColor(VSMakeBottomNormal(index));

  output.vDiffuseTex = vPosXZ * g_bottomScales.x;
#ifdef CONST_BOTTOM_SMALL_WAVE
  output.vGradTex    = float2((heightBottom.x + heightBottom.y)* g_bottomScales.y, 0);
#else // CONST_BOTTOM_SMALL_WAVE
  output.vGradTex    = float2(heightBottom.x * g_bottomScales.y, 0);
#endif // CONST_BOTTOM_SMALL_WAVE
}

float kBlack=0.1;

float4 PSBottom(VSOutputBottom input) : SV_Target
{
  float4 btmGrad = g_bottomGradient.Sample(g_samLinearClamp, input.vGradTex);
  float4 surfGrad = g_surfaceGradient.Sample(g_samLinearClamp, input.vGradTex);
  float4 bottomColor = g_bottomDiff.Sample(g_samLinear, input.vDiffuseTex) * lerp(float4(1,1,1,1), input.vColor, kBlack);

  return lerp(surfGrad, bottomColor * btmGrad, surfGrad.w);
}

//--------------------------------------------------------------------------------------
// Techniques section

technique10 Surface
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, VSSurface() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, PSSurface() ) );
    SetDepthStencilState( EnableDepth, 0 );
    SetBlendState( Interpolate, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 Bottom
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, VSBottom() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, PSBottom() ) );
    SetDepthStencilState( EnableDepth, 0 );
    SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}

technique10 SurfaceWithTriRemoving
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, VSSurface() ) );
    SetGeometryShader( CompileShader( gs_4_0, GSSurfaceWithTriRemoving() ) );
    SetPixelShader( CompileShader( ps_4_0, PSSurface() ) );
    SetDepthStencilState( EnableDepth, 0 );
    SetBlendState( Interpolate, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
  }
}
