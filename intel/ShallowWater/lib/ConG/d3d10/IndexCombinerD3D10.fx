/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file IndexCombinerD3D10.fx


uint VS_1(uint data: DATA) : DATA
{
	return data;
}

uint2 VS_2(uint2 data: DATA) : DATA
{
	return data;
}

uint3 VS_3(uint3 data: DATA) : DATA
{
	return data;
}

uint4 VS_4(uint4 data: DATA) : DATA
{
	return data;
}

DepthStencilState DisableDepthStencil
{
    DepthEnable = false;
    StencilEnable = false;
};


technique10 DoProcess
{
	pass p0
	{
		SetPixelShader(NULL);
		SetDepthStencilState( DisableDepthStencil, 0 );
	}
	pass p1
	{
		SetVertexShader( CompileShader( vs_4_0, VS_1() ) );
		SetGeometryShader( ConstructGSWithSO( CompileShader( vs_4_0, VS_1() ), "DATA.x"));
	}
	pass p2
	{
		SetVertexShader( CompileShader( vs_4_0, VS_2() ) );
		SetGeometryShader( ConstructGSWithSO( CompileShader( vs_4_0, VS_2() ), "DATA.xy"));
	}
	pass p3
	{
		SetVertexShader( CompileShader( vs_4_0, VS_3() ) );
		SetGeometryShader( ConstructGSWithSO( CompileShader( vs_4_0, VS_3() ), "DATA.xyz"));
	}
	pass p4
	{
		SetVertexShader( CompileShader( vs_4_0, VS_4() ) );
		SetGeometryShader( ConstructGSWithSO( CompileShader( vs_4_0, VS_4() ), "DATA.xyzw"));
	}
}

// end of file
