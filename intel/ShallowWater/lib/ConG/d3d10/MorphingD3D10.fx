/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file MorphingD3D10.fx

Buffer<float3> g_targets;
Buffer<float> g_weights;

cbuffer cbChangesFrequently
{
	int numBS;
}

struct VS_INPUT
{
    float3 v : DATA;
};

float3 VS_morph(VS_INPUT input, uint vertexID : SV_VertexID ) : DATA
{
	float3 output = 0;
	float tmpBaseWeight = 1.0;
	for(int i = 0; i < numBS; i++)
	{
		float weight = g_weights.Load(i);
		float3 cur = g_targets.Load(i * numBS + vertexID);
		output += cur * weight;
		tmpBaseWeight -= weight;
	}
	output += input.v * tmpBaseWeight;
	return output;
}

DepthStencilState DisableDepthStencil
{
    DepthEnable = false;
    StencilEnable = false;
};

technique10 Morphing
{
	pass p0
	{
		SetDepthStencilState(DisableDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, VS_morph()));
		SetGeometryShader(ConstructGSWithSO(CompileShader(vs_4_0, VS_morph()), "DATA.xyz;"));
		SetPixelShader(NULL);
	}
}

// end of file
