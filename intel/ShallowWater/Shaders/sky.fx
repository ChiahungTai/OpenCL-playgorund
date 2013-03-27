/* ************************************************************************* *\
*                INTEL CORPORATION PROPRIETARY INFORMATION
*   This software is supplied under the terms of a license agreement of
*   nondisclosure agreement with Intel Corporation and may not be copied
*   or disclosed except in accordance with the terms of that agreement.
*        Copyright (C) 2008-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

// This shader source file contains the sky visualization shaders implementation

//--------------------------------------------------------------------------------------
// Global variables

TextureCube g_tex;
SamplerState g_samLinear
{
  Filter = MIN_MAG_MIP_LINEAR;
  AddressU = Clamp;
  AddressV = Clamp;
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
  float3 vTexUV   : TEXCOORD0;  
};

void VSSky( float4 vPos : POSITION, out VS_OUTPUT output )
{
  output.vPos     = mul(vPos, g_mWVP);
  output.vTexUV   = normalize(vPos);
}

float4 PSSky(VS_OUTPUT input) : SV_Target
{
  return g_tex.Sample(g_samLinear, input.vTexUV);
}

//--------------------------------------------------------------------------------------
// Techniques section

technique10 Bottom
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, VSSky() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, PSSky() ) );
    SetDepthStencilState( EnableDepth, 0 );
  }
}
