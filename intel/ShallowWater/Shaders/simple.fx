/* ************************************************************************* *\
*                INTEL CORPORATION PROPRIETARY INFORMATION
*   This software is supplied under the terms of a license agreement of
*   nondisclosure agreement with Intel Corporation and may not be copied
*   or disclosed except in accordance with the terms of that agreement.
*        Copyright (C) 2008-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

// This shader source file contains the simple visualization shaders implementation

//--------------------------------------------------------------------------------------
// Global variables

float4x4 g_mWVP;

// DepthStates
DepthStencilState EnableDepth
{
  DepthEnable    = TRUE;
  DepthWriteMask = ALL;
  DepthFunc      = LESS_EQUAL;
};

// Vertex shader output structure
struct VS_OUTPUT
{
  float4 vPos     : SV_POSITION;
  float4 vDiffuse : COLOR0;     
  float2 vTexUV   : TEXCOORD0;  
};

// Surface shader
void SurfaceVS( float4 vPos : POSITION, float4 vColor : COLOR, out VS_OUTPUT output )
{
  output.vPos     = mul(vPos, g_mWVP);
  output.vDiffuse = vColor; //float4(0.3,0.47,0.53,1) * (1.0 + vPos.y / 5.0); //vColor; // float4(0.1,0.6,0.9,1) * (1.0 + vPos.y / 5.0);
  output.vTexUV   = vPos.xz;
}

// Surface pixel shader
float4 SurfacePS(VS_OUTPUT input) : SV_Target
{
  return input.vDiffuse;
}

//--------------------------------------------------------------------------------------
// Techniques section

technique10 Surface
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, SurfaceVS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, SurfacePS() ) );
    SetDepthStencilState( EnableDepth, 0 );
  }
}
