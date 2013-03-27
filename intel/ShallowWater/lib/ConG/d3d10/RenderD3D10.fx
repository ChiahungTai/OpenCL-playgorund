/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file RenderD3D10.fx


#include "common.fxh"


//-----------------------------------------------------------------------------
// Vertex Shader
//-----------------------------------------------------------------------------

void VS(VS_INPUT input,
  #if AUTO_NORMALS
    out GS_INPUT output
  #else
    out PS_INPUT output
  #endif
        )
{
    // Transform the position from object space to homogeneous projection space
    output.hpos = mul(float4(input.pos, 1), WorldViewProj);

#if HAS_REFLECTION_TEX || SHADING_MODE > 0
  #if AUTO_NORMALS
    output.vposN = mul(float4(input.pos, 1), WorldViewN).xyz;
  #else
    output.vnorm = mul(input.norm, (float3x3) WorldViewN);
  #endif
#endif

#if HAS_BUMP_TEX
    output.vtextangent  = mul(input.textangent, (float3x3) WorldViewN);
    output.vtexbinormal = mul(input.texbinormal, (float3x3) WorldViewN);
#endif

#if NEED_VPOS
    output.vpos = (float3) mul(float4(input.pos, 1), WorldView);
#endif

    output.tex = input.tex;
}

//--------------------------------------------------------------------------------------
// Geometry Shader - for automatic normals
//--------------------------------------------------------------------------------------

#if AUTO_NORMALS
[MaxVertexCount(3)]
void GS( triangle GS_INPUT inputPos[3], inout TriangleStream<PS_INPUT> output )
{
	float3 n = normalize(cross(inputPos[1].vposN - inputPos[0].vposN, inputPos[0].vposN - inputPos[2].vposN));
	for( int i = 0; i < 3; ++i )
	{
		PS_INPUT tmp = inputPos[i];
		tmp.vnorm = n;
		output.Append(tmp);
	}
}
#endif


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////

float3 GetTextureDensity(Texture2D t, float2 texcoord)
{
	float2 texSize;
	t.GetDimensions(texSize.x, texSize.y);
	float2x2 mm = float2x2(texSize * ddx(texcoord), texSize * ddy(texcoord));
	float dd = abs(determinant(mm));
	const float3 good = float3(0.5,0.5,0.5);
	const float3 tooHigh = float3(1,0,0);
	const float3 tooLow = float3(0,1,0);
	float3 temp = lerp(good, tooHigh, saturate(dd - 1));
	return lerp(temp, tooLow, saturate(1/dd - 1));
}

void PS(PS_INPUT input, bool isFrontFace: SV_IsFrontFace,
        out
        #if OPACITY_MODE && !WIREFRAME_MODE
          float4
        #else
          float3
        #endif
                out_color : SV_Target0
         )
{
    #if OPACITY_MODE && !WIREFRAME_MODE
        float4 transp_tmp = 
                #if HAS_TRANSPARENT_TEX
                    TransparentTexture.Sample(samLinear, TexCoord(input.tex.transparent))
                #else
                    MaterialTransparent
                #endif
            * MaterialTransparency;
        float a = 
                    #if OPACITY_MODE == A_ONE
                             transp_tmp.a
                    #else                 // RGB_ZERO, TODO: emulate through the blend mode
                             saturate(1-dot(transp_tmp.rgb, float3(1,1,1))/3)
                    #endif
            ;
        if( a == 0 ) discard;
    #endif


#if TEXTURE_DEBUG
	float3 result = float3(0,0,0);
	// grep 'enum Channel'
	#if   TEXTURE_DEBUG-1 == 0 // CH_EMISSION
		#if HAS_EMISSION_TEX
			result = GetTextureDensity(EmissionTexture, input.tex.emission);
		#endif
	#elif TEXTURE_DEBUG-1 == 1 // CH_AMBIENT
		#if HAS_AMBIENT_TEX
			result = GetTextureDensity(AmbientTexture, input.tex.ambient);
		#endif
	#elif TEXTURE_DEBUG-1 == 2 // CH_DIFFUSE
		#if HAS_DIFFUSE_TEX
			result = GetTextureDensity(DiffuseTexture, input.tex.diffuse);
		#endif
	#elif TEXTURE_DEBUG-1 == 3 // CH_SPECULAR
		#if HAS_SPECULAR_TEX
			result = GetTextureDensity(SpecularTexture, input.tex.specular);
		#endif
	#elif TEXTURE_DEBUG-1 == 4 // CH_TRANSPARENT
		#if HAS_TRANSPARENT_TEX
			result = GetTextureDensity(TransparentTexture, input.tex.transparent);
		#endif
	#elif TEXTURE_DEBUG-1 == 5 // CH_BUMP
		#if HAS_BUMP_TEX
			result = GetTextureDensity(BumpTexture, input.tex.bump);
		#endif
	#endif
#else // TEXTURE_DEBUG
    #if HAS_REFLECTION_TEX || SHADING_MODE > 0
        float3 vnorm = GetViewNormal(input, isFrontFace);
    #endif

    #if SHADING_MODE > 0
        // Compute directional lighting equation
        float3 TotalDiffuse = float3(0, 0, 0);

        #if SHADING_MODE > 1
            float3 TotalSpecular = float3(0, 0, 0);
            float3 vpos = normalize(input.vpos);
        #endif

        #if DIRECT_LIGHT_COUNT
            [unroll] for( int d = 0; d < DIRECT_LIGHT_COUNT; ++d )
            {
                Light(LightDir[d].color.rgb, LightDir[d].vpos.xyz, vnorm, TotalDiffuse
                    #if SHADING_MODE > 1
                        , vpos
                        , TotalSpecular
                    #endif
                );
            }
        #endif

        #if SPOT_LIGHT_COUNT
            [unroll] for( int s = 0; s < SPOT_LIGHT_COUNT; ++s )
            {
                float3 vlight = LightSpot[s].vpos.xyz - input.vpos;
                float d = length(vlight);
                float attenuation = LightSpot[s].a0 + LightSpot[s].a1 * d + LightSpot[s].a2 * d*d;
                float4 sp = mul(float4(input.vpos, 1), LightSpot[s].proj);
                sp.xy /= sp.w;
                float r = length(sp.xy);
                float m = step(r, 1) * step(sp.w, 0) * ((r < 1) ? pow(1 - r, LightSpot[s].exponent) : 1);
                Light(LightSpot[s].color.rgb * m / attenuation, vlight / d, vnorm, TotalDiffuse
                    #if SHADING_MODE > 1
                        , vpos
                        , TotalSpecular
                    #endif
                );
            }
        #endif

        #if POINT_LIGHT_COUNT
            [unroll] for( int p = 0; p < POINT_LIGHT_COUNT; ++p )
            {
                float3 vlight = LightPoint[p].vpos.xyz - input.vpos;
                float d = length(vlight);
                float attenuation = LightPoint[p].a0 + LightPoint[p].a1 * d + LightPoint[p].a2 * d*d;
                Light(LightPoint[p].color.rgb / attenuation, vlight / d, vnorm, TotalDiffuse
                    #if SHADING_MODE > 1
                        , vpos
                        , TotalSpecular
                    #endif
                );
            }
        #endif

        #if DEFAULT_LIGHT
            // default point light from eye
            float3 vlight = -normalize(input.vpos);
            Light(DefaultLight.rgb, vlight, vnorm, TotalDiffuse
                #if SHADING_MODE > 1
                    , vpos
                    , TotalSpecular
                #endif
            );
        #endif
    #endif // SHADING_MODE > 0

    float3 result =
    (
        #if HAS_REFLECTION_TEX
            // TODO: calculate reflection vector in world space
            //	float3 vr = normalize(reflect(-input.vpos, vnorm));
            //	vr = mul(vr, ViewI);
            + MaterialReflection.rgb * ReflectionTexture.Sample(samLinear, (float2(1,1) + normalize(reflect(-input.vpos, vnorm)).xy) / 2).rgb
        #endif


        #if HAS_EMISSION_TEX
            + EmissionTexture.Sample(samLinear, TexCoord(input.tex.emission)).rgb
        #else
            + MaterialEmission.rgb
        #endif
        // pre-multiplied alpha
        #if OPACITY_MODE && !WIREFRAME_MODE
          * a
        #endif

        #if !MAYA_AMBIENT_FIX
            #if HAS_AMBIENT_TEX
                + AmbientTexture.Sample(samLinear, TexCoord(input.tex.ambient)).rgb
            #else
                + MaterialAmbient.rgb
            #endif

            * LightAmbient.rgb

            // pre-multiplied alpha
            #if OPACITY_MODE && !WIREFRAME_MODE
              * a
            #endif
        #endif


        //
        // material diffuse component
        //

        #if HAS_DIFFUSE_TEX
            + DiffuseTexture.Sample(samLinear, TexCoord(input.tex.diffuse)).rgb
        #else
            + MaterialDiffuse.rgb
        #endif


        //
        // light diffuse component
        //

        * (
            #if MAYA_AMBIENT_FIX
                // environment ambient
                LightAmbient.rgb

                // object local ambient
                #if HAS_AMBIENT_TEX
                    + AmbientTexture.Sample(samLinear, TexCoord(input.tex.ambient)).rgb
                #else
                    + MaterialAmbient.rgb
                #endif
            #else
                0     // to avoid compiler error
            #endif

            // total diffuse came from scene lights
            #if SHADING_MODE > 0
                + TotalDiffuse
            #endif
          )
        // pre-multiplied alpha
        #if OPACITY_MODE && !WIREFRAME_MODE
          * a
        #endif


        //
        // light specular compoent
        //

        #if SHADING_MODE > 1
           + TotalSpecular * 
                #if HAS_SPECULAR_TEX
                    SpecularTexture.Sample(samLinear, TexCoord(input.tex.specular)).rgb
                #else
                    MaterialSpecular.rgb
                #endif
        #endif
    );
#endif // TEXTURE_DEBUG

    #if OPACITY_MODE && !WIREFRAME_MODE
        out_color = float4(result, a);
    #else
        out_color = result;
    #endif
}

#if DRAW_NORMALS && SHADING_MODE > 0

float3 PS_Normals(PS_INPUT input) : SV_Target
{
    return float3(0,0,1);
}

//--------------------------------------------------------------------------------------
// Geometry Shader
//--------------------------------------------------------------------------------------

[MaxVertexCount(6)]
void GS( triangle PS_INPUT inputPos[3], inout LineStream<PS_INPUT> output )
{
    PS_INPUT v = (PS_INPUT) 0;

    float3 s = cross(inputPos[1].vpos - inputPos[0].vpos, inputPos[2].vpos - inputPos[0].vpos);
    float ls = sqrt(length(s));

	[unroll] for( int i = 0; i < 3; ++i )
	{
		v.hpos = inputPos[i].hpos;
		output.Append(v);
		v.hpos = mul(float4(inputPos[i].vpos + normalize(inputPos[i].vnorm) * ls, 1), Proj);
		output.Append(v);

		output.RestartStrip();
	}
}

#endif


//--------------------------------------------------------------------------------------
// Technique
//--------------------------------------------------------------------------------------

RasterizerState TurnMultisampleOn
{
    MultisampleEnable = true;
  #if WIREFRAME_MODE
    FillMode = WIREFRAME;
  #endif
  #if DOUBLE_SIDED
    CullMode = NONE;
  #else
  #if INVERSE_SCALE
    CullMode = BACK;
  #else
    CullMode = FRONT;  // we use invert culling to comply with COLLADA standard
  #endif
#endif
};

BlendState BlendEnable
{
    BlendEnable[0] = true;

    SrcBlend = One;
    DestBlend = Inv_Src_Alpha;
};

BlendState BlendDisable
{
    BlendEnable[0] = false;
};

DepthStencilState EnableDepth
{
    DepthEnable = true;
    DepthWriteMask = ALL;
};

DepthStencilState DisableDepth
{
    DepthEnable = true;
    DepthWriteMask = 0;
};




technique10 Render
{
    pass P0
    {
        SetRasterizerState( TurnMultisampleOn );
      #if OPACITY_MODE && !WIREFRAME_MODE
		SetDepthStencilState( DisableDepth, 0 );
		SetBlendState( BlendEnable, float4(1.0, 1.0, 1.0, 1.0), 0xffffffff);
      #else
		SetDepthStencilState( EnableDepth, 0 );
		SetBlendState( BlendDisable, float4(1.0, 1.0, 1.0, 1.0), 0xffffffff);
      #endif

        SetVertexShader( CompileShader( vs_4_0, VS() ) );
      #if AUTO_NORMALS
        SetGeometryShader( CompileShader( gs_4_0, GS() ) );
      #else
        SetGeometryShader( NULL );
      #endif
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
    }

  #if DRAW_NORMALS && SHADING_MODE > 0
    pass P1
    {
        SetRasterizerState( TurnMultisampleOn );
        SetDepthStencilState( EnableDepth, 0 );
		SetBlendState( BlendDisable, float4(1.0, 1.0, 1.0, 1.0), 0xffffffff);

        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( CompileShader( gs_4_0, GS() ) );
        SetPixelShader( CompileShader( ps_4_0, PS_Normals() ) );
    }
  #endif
}


// end of file
