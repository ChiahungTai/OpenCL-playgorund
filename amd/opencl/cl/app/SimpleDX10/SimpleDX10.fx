/**********************************************************************
Copyright ©2012 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

struct VS_IN
{
    float4 pos: pos;
};

struct VS_OUT
{
    float2 tc: TEXCOORD0;
    float4 pos: SV_Position;
};

float4x4 matXform;

VS_OUT vs_main(VS_IN In)
{
    VS_OUT Out = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    
    Out.tc = In.pos.xy * float2(2.0, -2.0) + float2(-1.0, 1.0);
    Out.pos = mul(In.pos, matXform); //

    return Out;
}

SamplerState smp
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};
Texture2D tex;

float4 ps_main(VS_OUT In) : SV_Target
{
    float3 color = tex.Sample(smp, In.tc);
    return float4(color, 1.0);
}

/////////////////////////

technique10 t0
{
    pass p0
    {
        SetVertexShader( CompileShader( vs_4_0, vs_main() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, ps_main() ) );
    }
}