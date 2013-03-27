/* ************************************************************************* *\
*                INTEL CORPORATION PROPRIETARY INFORMATION
*   This software is supplied under the terms of a license agreement of
*   nondisclosure agreement with Intel Corporation and may not be copied
*   or disclosed except in accordance with the terms of that agreement.
*        Copyright (C) 2008-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

// This shader source file contains the bottom visualization shaders implementation

//--------------------------------------------------------------------------------------
// Global variables

Texture2D g_bottomDiffuse;
SamplerState g_samLinear
{
  Filter = MIN_MAG_MIP_LINEAR;
  AddressU = Wrap;
  AddressV = Wrap;
};

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

void BottomVS( float4 vPos : POSITION, float4 vColor : COLOR, out VS_OUTPUT output )
{
  output.vPos     = mul(vPos, g_mWVP);
  output.vDiffuse = vColor; // float4(0.5,0.5,0.4,1);
  output.vTexUV   = vPos.xz * float2(0.00390625,0.00390625); //float2(0.07,0.07);
}

// Bottom pixel shader
float4 BottomPS(VS_OUTPUT input) : SV_Target
{
  return g_bottomDiffuse.Sample(g_samLinear, input.vTexUV) * input.vDiffuse;
}

//--------------------------------------------------------------------------------------
// Techniques section

technique10 Bottom
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, BottomVS() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, BottomPS() ) );
    SetDepthStencilState( EnableDepth, 0 );
  }
}
