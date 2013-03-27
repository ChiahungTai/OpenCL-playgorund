// Copyright (c) 2009-2011 Intel Corporation
// All rights reserved.
// 
// WARRANTY DISCLAIMER
// 
// THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
// MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Intel Corporation is the author of the Materials, and requests that all
// problem reports or change requests be submitted to it directly


// geo shader particle shader

struct VS_In
{
    float4  pos		   : POSITION;
};

struct VS_GS
{
    float3 pos			: POSITION;
};

struct GS_AxisDraw
{
    float4 pos			: SV_POSITION;
};

struct GS_ParticleDraw
{
    float2 tex			: TEXCOORD0;
    float4 pos			: SV_POSITION;
};

struct PS_AxisDraw
{
	// nada?
};

struct PS_ParticleDraw
{
	float2 tex			: TEXCOORD0;
};

BlendState AdditiveBlending
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = TRUE;
    SrcBlend = SRC_ALPHA;
    DestBlend = ONE;
    BlendOp = ADD;
    SrcBlendAlpha = ZERO;
    DestBlendAlpha = ZERO;
    BlendOpAlpha = ADD;
    RenderTargetWriteMask[0] = 0x0F;
};

DepthStencilState DisableDepth
{
    DepthEnable = FALSE;
    DepthWriteMask = ALL;
};

DepthStencilState EnableDepth
{
    DepthEnable = TRUE;
    DepthWriteMask = ZERO;
};

SamplerState g_samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

cbuffer cb0
{
	float4x4 g_mInvView;
    float4x4 g_mWorldViewProj;   
};

cbuffer cbImmutable
{
	float3 g_positions[4] =
    {
        float3( -1, 1, 0 ),
        float3( 1, 1, 0 ),
        float3( -1, -1, 0 ),
        float3( 1, -1, 0 ),
    };
    float2 g_texcoords[4] = 
    { 
        float2(0,1), 
        float2(1,1),
        float2(0,0),
        float2(1,0),
    };
	float g_fParticleRad = 1.0f;
};

Texture2D		        g_txDiffuse;

//
// Vertex shader for drawing the point-sprite particles
//
VS_GS VSParticleDraw(VS_In input)
{
    VS_GS output;

	output.pos = input.pos.xyz;

    return output;
}

//
// GS for rendering axes.  Takes a point and turns it into a line coming from the origin
//
[maxvertexcount(2)]
void GSAxisDraw(point VS_GS input[1], inout LineStream<GS_AxisDraw> AxisLineStream)
{
    GS_AxisDraw origin;
	GS_AxisDraw output;

	float3 originpos = float3(0,0,0);
	originpos = mul( originpos, (float3x3)g_mInvView );
	origin.pos = mul( float4(originpos,1.0), g_mWorldViewProj ); 

	AxisLineStream.Append(origin);    

	float3 axisendpos = input[0].pos;
    output.pos = mul( float4(axisendpos,1.0), g_mWorldViewProj ); 
 
    AxisLineStream.Append(output);

    AxisLineStream.RestartStrip();
}

//
// GS for rendering point sprite particles.  Takes a point and turns it into 2 tris.
//
[maxvertexcount(4)]
void GSParticleDraw(point VS_GS input[1], inout TriangleStream<GS_ParticleDraw> SpriteStream)
{
    GS_ParticleDraw output;
    
    //
    // Emit two new triangles
    //
    for(int i=0; i<4; i++)
    {
		float3 position = g_positions[i] * g_fParticleRad;
        position = mul( position, (float3x3)g_mInvView ) + input[0].pos;
        output.pos = mul( float4(position,1.0), g_mWorldViewProj ); 
 
        output.tex = g_texcoords[i];
        SpriteStream.Append(output);
    }
    SpriteStream.RestartStrip();
}

//
// PS for drawing axes
//
float4 PSAxisDraw(PS_AxisDraw input) : SV_TARGET
{   
    return float4(0,1,0,1);
}

//
// PS for drawing particles
//
float4 PSParticleDraw(PS_ParticleDraw input) : SV_TARGET
{   
    return g_txDiffuse.Sample( g_samLinear, input.tex );
}

//--------------------------------------------------------------------------------------
// Renders axes 
//--------------------------------------------------------------------------------------
technique10 RenderAxes
{
    pass P0
    {       
		SetVertexShader( CompileShader( vs_4_0, VSParticleDraw() ) );
        SetGeometryShader( CompileShader( gs_4_0, GSAxisDraw() ) );
        SetPixelShader( CompileShader( ps_4_0, PSAxisDraw() ) );

		SetBlendState( AdditiveBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( EnableDepth, 0 );
	}
}

//--------------------------------------------------------------------------------------
// Renders particles 
//--------------------------------------------------------------------------------------
technique10 RenderParticles
{
    pass P0
    {       
		SetVertexShader( CompileShader( vs_4_0, VSParticleDraw() ) );
        SetGeometryShader( CompileShader( gs_4_0, GSParticleDraw() ) );
        SetPixelShader( CompileShader( ps_4_0, PSParticleDraw() ) );

		SetBlendState( AdditiveBlending, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
        SetDepthStencilState( DisableDepth, 0 );
	}
}
