/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SkinningD3D10.fx

Buffer<float4> g_joints;
matrix g_bindShape;

tbuffer WeightedJoints
{
	struct
	{
		uint jointIdx;
		float weight;
	} g_weightedJoints[2]; // why does it work??
}

struct VS_INPUT
{
    float3 v         : DATA;
    matrix skinning  : TRANSFORM;
};

float3 VS_pos(VS_INPUT input) : DATA
{
	return mul(float4(input.v, 1), transpose(input.skinning)).xyz;
}

float3 VS_norm(VS_INPUT input) : DATA
{
	return mul(input.v, (float3x3) transpose(input.skinning));
}

uint3 VS_serialize3(uint3 data: DATA) : DATA
{
	return data;
}

struct DATA_16
{
    uint4 v0 : DATA0;
    uint4 v1 : DATA1;
    uint4 v2 : DATA2;
    uint4 v3 : DATA3;
};

DATA_16 VS_serialize16(DATA_16 data)
{
	return data;
}

struct DATA_16f
{
    float4 v0 : DATA0;
    float4 v1 : DATA1;
    float4 v2 : DATA2;
    float4 v3 : DATA3;
};


DATA_16f VS_matrices(uint2 bindings : BINDINGS)
{
	matrix result = 0;
	for( uint i = bindings.x; i < bindings.y; i++ )
	{
		uint jIdx = g_weightedJoints[i].jointIdx;
		matrix joint = matrix(g_joints.Load(jIdx*4), 
		                      g_joints.Load(jIdx*4+1),
		                      g_joints.Load(jIdx*4+2),
		                      g_joints.Load(jIdx*4+3));
		float weight = g_weightedJoints[i].weight;
		result += joint * weight;
	}
	return (DATA_16f) mul(g_bindShape, result);
}

DepthStencilState DisableDepthStencil
{
    DepthEnable = false;
    StencilEnable = false;
};

technique10 ComputeMatrices
{
	pass p0
	{
		SetDepthStencilState(DisableDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, VS_matrices()));
		SetGeometryShader(ConstructGSWithSO(CompileShader(vs_4_0, VS_matrices()), "DATA0.xyzw;DATA1.xyzw;DATA2.xyzw;DATA3.xyzw"));
		SetPixelShader(NULL);
	}
}

technique10 TransformPositions
{
	pass p0
	{
		SetDepthStencilState(DisableDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, VS_pos()));
		SetGeometryShader(ConstructGSWithSO(CompileShader(vs_4_0, VS_pos()), "DATA.xyz"));
		SetPixelShader(NULL);
	}
}

technique10 TransformNormals
{
	pass p0
	{
		SetDepthStencilState(DisableDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, VS_norm()));
		SetGeometryShader(ConstructGSWithSO(CompileShader(vs_4_0, VS_norm()), "DATA.xyz"));
		SetPixelShader(NULL);
	}
}

technique10 Serialize3
{
	pass p0
	{
		SetDepthStencilState(DisableDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, VS_serialize3()));
		SetGeometryShader(ConstructGSWithSO(CompileShader(vs_4_0, VS_serialize3()), "DATA.xyz"));
		SetPixelShader(NULL);
	}
}

technique10 Serialize16
{
	pass p0
	{
		SetDepthStencilState(DisableDepthStencil, 0);

		SetVertexShader(CompileShader(vs_4_0, VS_serialize16()));
		SetGeometryShader(ConstructGSWithSO(CompileShader(vs_4_0, VS_serialize16()), "DATA0.xyzw;DATA1.xyzw;DATA2.xyzw;DATA3.xyzw"));
		SetPixelShader(NULL);
	}
}

// end of file
