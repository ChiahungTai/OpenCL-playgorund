/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or 
     nondisclosure agreement with Intel Corporation and may not be copied 
     or disclosed except in accordance with the terms of that agreement. 
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file CLogo.fx
///    Effects file for CLogo.


cbuffer cbConstant
{
    float4 LogoPosition = float4(-1, -1, 1, 1);
};

cbuffer cbChangeEveryFrame
{
    float4 LogoColor = float4(1, 1, 1, 1);
};

// Globals
Texture2D g_LogoTexture;

// Texture samplers
SamplerState LogoTextureSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

struct OUTPUT_BOX
{
    float4 pos : SV_POSITION;     // Projection coord
    float2 tex : TEXCOORD0;       // Texture coord
};

OUTPUT_BOX BoxVS(uint id : SV_VertexID)
{
    OUTPUT_BOX result[4] = 
    {
        float4( LogoPosition.z, LogoPosition.w, 0.5, 1), float2( 1, 1 ), 
        float4( LogoPosition.x, LogoPosition.w, 0.5, 1), float2( 0, 1 ),
        float4( LogoPosition.z, LogoPosition.y, 0.5, 1), float2( 1, 0 ),
        float4( LogoPosition.x, LogoPosition.y, 0.5, 1), float2( 0, 0 ),
    };
    return result[id];
}

float4 BoxPS( OUTPUT_BOX in_positionPS ): SV_TARGET
{
    return LogoColor * g_LogoTexture.Sample(LogoTextureSampler, in_positionPS.tex);
}

RasterizerState RS_SolidFill_NoCull
{
    FILLMODE = Solid;
    CullMode = NONE;
};

DepthStencilState DepthTestOff
{
    DepthEnable = false;
};

DepthStencilState DepthTestOn
{
    DepthEnable = true;
};

BlendState SrcAlphaBlending
{
    AlphaToCoverageEnable    = FALSE;
    BlendEnable[0]           = TRUE;
    SrcBlend                 = SRC_ALPHA;
    DestBlend                = INV_SRC_ALPHA;
    BlendOp                  = ADD;
    SrcBlendAlpha            = ZERO;
    DestBlendAlpha           = ZERO;
    BlendOpAlpha             = ADD;
    RenderTargetWriteMask[0] = 0x0F;//D3D10_COLOR_WRITE_ENABLE_ALL;
};

BlendState NoBlending
{
    BlendEnable[0]           = FALSE;
};

// RenderLogo technique
technique10 RenderLogo
{
    pass p0
    {
        SetRasterizerState( RS_SolidFill_NoCull );
        SetDepthStencilState( DepthTestOff, 0 );
        SetBlendState( SrcAlphaBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );

        SetVertexShader( CompileShader( vs_4_0, BoxVS() ) );
        SetGeometryShader(NULL);
        SetPixelShader( CompileShader( ps_4_0, BoxPS() ) );
    }

    pass p1  // just to restore render settings
    {
        SetBlendState( NoBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( DepthTestOn, 0 );
    }
}

// end of file
