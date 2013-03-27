/* ************************************************************************* *\
*                INTEL CORPORATION PROPRIETARY INFORMATION
*   This software is supplied under the terms of a license agreement of
*   nondisclosure agreement with Intel Corporation and may not be copied
*   or disclosed except in accordance with the terms of that agreement.
*        Copyright (C) 2008-2011 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

// This shader source file contains the advected textures algorithm implementation

//--------------------------------------------------------------------------------------
// Global variables

Texture2D g_tex;
Texture2D g_texVelocity;

SamplerState g_samPoint
{
  Filter = MIN_MAG_MIP_POINT;
  AddressU = Clamp;
  AddressV = Clamp;
};

SamplerState g_samLinear
{
  Filter = MIN_MAG_MIP_LINEAR;
  AddressU = Clamp;
  AddressV = Clamp;
};

// States
BlendState NoBlend
{
  BlendEnable[0] = FALSE;
};

RasterizerState CullOff
{
	CullMode = None;
};


float4 g_xforms[2];
float2 g_spatialScale;

void VSCopy( in float2 vPos : POSITION, out float4 vOutPos : SV_POSITION, out float2 vTex : TEXCOORD0 )
{
  vOutPos.xy = vPos.xy * g_xforms[0].xy + g_xforms[0].zw;
  vOutPos.zw = float2(0.0f, 1.0f);
  vTex = vPos.xy * g_xforms[1].xy + g_xforms[1].zw;
}

float4 PSInit(float4 vPos : SV_POSITION, float2 vTex : TEXCOORD0) : SV_Target
{
  return vTex.xyyy;
}

float4 PSAdvect(float4 vPos : SV_POSITION, float2 vTex : TEXCOORD0) : SV_Target
{
  float2 velocity = g_texVelocity.Sample(g_samPoint, vTex);
  return g_tex.Sample(g_samLinear, vTex + velocity * g_spatialScale);
}

//--------------------------------------------------------------------------------------
// Techniques section

technique10 Init
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, VSCopy() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, PSInit() ) );

    SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    SetRasterizerState( CullOff );
  }
}

technique10 Advect
{
  pass P0
  {
    SetVertexShader( CompileShader( vs_4_0, VSCopy() ) );
    SetGeometryShader( NULL );
    SetPixelShader( CompileShader( ps_4_0, PSAdvect() ) );

    SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
    SetRasterizerState( CullOff );
  }
}
