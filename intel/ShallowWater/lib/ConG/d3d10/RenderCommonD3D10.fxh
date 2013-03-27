/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file RenderCommonD3D10.fxh

//
// grep: lighting globals
//

#ifndef DIRECT_LIGHT_COUNT
 #define DIRECT_LIGHT_COUNT  1
#endif

#ifndef SPOT_LIGHT_COUNT
 #define SPOT_LIGHT_COUNT    1
#endif

#ifndef POINT_LIGHT_COUNT
 #define POINT_LIGHT_COUNT   1
#endif


shared Texture2D EmissionTexture;
shared Texture2D AmbientTexture;
shared Texture2D DiffuseTexture;
shared Texture2D SpecularTexture;
shared Texture2D TransparentTexture;
shared Texture2D BumpTexture;
shared Texture2D ReflectionTexture;


struct PointLight
{
	float4 color;
	float4 vpos;  // w is ignored
	float  a0;
	float  a1;
	float  a2;
	float dummy;
};

struct SpotLight
{
	float4 color;
	float4x4 proj;
	float4 vpos;  // w is ignored
	float  a0;
	float  a1;
	float  a2;
	float  exponent;
};

struct DirectLight
{
	float4 color;
	float4 vpos;
};



//-----------------------------------------------------------------------------
// Constant Buffer Variables
//-----------------------------------------------------------------------------

shared cbuffer cbChangesEveryFrame
{
#if SPOT_LIGHT_COUNT
	SpotLight   LightSpot[SPOT_LIGHT_COUNT];
#endif
#if DIRECT_LIGHT_COUNT
	DirectLight LightDir[DIRECT_LIGHT_COUNT];
#endif
#if POINT_LIGHT_COUNT
	PointLight  LightPoint[POINT_LIGHT_COUNT];
#endif
	float4      LightAmbient;
	float4      DefaultLight;

	float4 MaterialEmission = float4(0, 0, 0, 1);
	float4 MaterialAmbient = float4(0.5, 0.5, 0.5, 1);
	float4 MaterialDiffuse = float4(1,1,1,1);
	float4 MaterialSpecular = float4(0.5, 0.5, 0.5, 1);
	float4 MaterialReflection = float4(1, 1, 1, 1);
	float4 MaterialTransparent = float4(0, 0, 0, 0.5);
	float  MaterialShininess = 1;
	float  MaterialTransparency = 1;
}

shared cbuffer cbChangesEveryPass
{
//	matrix ViewI;
	matrix Proj;
}

shared cbuffer cbChangesEverySubmesh
{
	matrix WorldView;
	matrix WorldViewN;     // the transpose of the inverse of the WorldView
	matrix WorldViewProj;
}

///////////////////////////////////////////////////////////////////////////////


#pragma warning (disable: 3571)

#ifndef TEXTURE_DEBUG
 #define TEXTURE_DEBUG       1
#endif


//
// mesh params
//

#ifndef DOUBLE_SIDED
 #define DOUBLE_SIDED        1
#endif



//
// material params
//

#ifndef HAS_EMISSION_TEX
 #define HAS_EMISSION_TEX    0
#endif

#ifndef HAS_AMBIENT_TEX
 #define HAS_AMBIENT_TEX     0
#endif

#ifndef HAS_DIFFUSE_TEX
 #define HAS_DIFFUSE_TEX     1
#endif

#ifndef HAS_SPECULAR_TEX
 #define HAS_SPECULAR_TEX    1
#endif

#ifndef HAS_REFLECTION_TEX
 #define HAS_REFLECTION_TEX  0
#endif

#ifndef HAS_TRANSPARENT_TEX
 #define HAS_TRANSPARENT_TEX 0
#endif

#ifndef HAS_BUMP_TEX
 #define HAS_BUMP_TEX        1
#endif


#define A_ONE     1
#define RGB_ZERO  2
#ifndef OPACITY_MODE       // 0 - opaque
 #define OPACITY_MODE   1
#endif

#if OPACITY_MODE > 2
 #error unsupported opacity mode
#endif



//  mode   normals   specular   shader
//-------------------------------------------------
//  0        no         no      constant
//  1        yes        no      lambert
//  2        yes        yes     phong
//  3        yes        yes     blinn_phong
//  4        yes        yes     blinn_torrance_sparrow

#ifndef SHADING_MODE
 #define SHADING_MODE       2
#endif

#if SHADING_MODE > 4
 #error unsupported shading mode
#endif


#ifndef AUTO_NORMALS
 #define AUTO_NORMALS       1
#endif




//
// misc
//

#ifndef WIREFRAME_MODE
 #define WIREFRAME_MODE  1
#endif

#ifndef INVERSE_SCALE
 #define INVERSE_SCALE   1
#endif

#ifndef DRAW_NORMALS
 #define DRAW_NORMALS    1
#endif

#ifndef DEFAULT_LIGHT
 #define DEFAULT_LIGHT   1
#endif


//
// workarounds
//

#ifndef FLIP_V
 #define FLIP_V              0
#endif

#ifndef MAYA_AMBIENT_FIX
 #define MAYA_AMBIENT_FIX    0
#endif

///////////////////////////////////////////////////////////////////////////////

#ifndef NEED_VPOS
 #define NEED_VPOS (DEFAULT_LIGHT || POINT_LIGHT_COUNT || SPOT_LIGHT_COUNT \
                   || HAS_REFLECTION_TEX || DRAW_NORMALS || SHADING_MODE > 1)
#endif
//-----------------------------------------------------------------------------

struct TextureCoords
{
    #if HAS_EMISSION_TEX
        float2 emission    : TEX_EMISSION;
    #endif
    #if HAS_AMBIENT_TEX
        float2 ambient     : TEX_AMBIENT;
    #endif
    #if HAS_DIFFUSE_TEX
        float2 diffuse     : TEX_DIFFUSE;
    #endif
    #if HAS_SPECULAR_TEX
        float2 specular    : TEX_SPECULAR;
    #endif
    #if HAS_TRANSPARENT_TEX
        float2 transparent : TEX_TRANSPARENT;
    #endif
    #if HAS_BUMP_TEX
        float2 bump        : TEX_BUMP;
    #endif
};

struct VS_INPUT
{
    float3 pos             : POSITION;
#if !AUTO_NORMALS && (HAS_REFLECTION_TEX || SHADING_MODE > 0)   // 0 means constant, so no normals required
    float3 norm            : NORMAL;                            // in autonormals mode they are being spawned in GS
#endif
#if HAS_BUMP_TEX
	float3 textangent  : TEXTANGENT;
	float3 texbinormal : TEXBINORMAL;
#endif
	TextureCoords tex;
};

struct GS_INPUT
{
	float4 hpos : SV_POSITION;
#if HAS_REFLECTION_TEX || SHADING_MODE > 0
    float3 vposN        : VPOSN;
#endif
#if HAS_BUMP_TEX
	float3 vtextangent  : TEXTANGENT;
	float3 vtexbinormal : TEXBINORMAL;
#endif
#if NEED_VPOS
    float3 vpos   : COLOR0;       // vertex pos in view space
#endif
    TextureCoords tex;
};

struct PS_INPUT
{
	float4 hpos : SV_POSITION;
#if HAS_REFLECTION_TEX || SHADING_MODE > 0
    float3 vnorm  : NORMAL;
#endif
#if HAS_BUMP_TEX
	float3 vtextangent  : TEXTANGENT;
	float3 vtexbinormal : TEXBINORMAL;
#endif
#if NEED_VPOS
    float3 vpos   : COLOR0;       // vertex pos in view space
#endif
    TextureCoords tex;
};



shared SamplerState samLinear
{
	Filter = ANISOTROPIC;
	AddressU = Wrap;
	AddressV = Wrap;
};


#if SHADING_MODE > 0
void Light(float3 lightColor, float3 vlight, float3 vnorm, inout float3 diffuse
    #if SHADING_MODE > 1
           , float3 vpos                // view-space pos required for specular calculation
           , inout float3 specular      //
    #endif
           )
{
    float ndotl = max(0, dot(vnorm, vlight));
    diffuse += lightColor * ndotl;

    #if SHADING_MODE == 2
        // phong
        float vdotr = dot(vpos, reflect(vlight, vnorm));
        specular += lightColor * (vdotr > 0 ? pow(vdotr, MaterialShininess) : 0);
    #endif // SHADING_MODE == 2

    #if SHADING_MODE > 2
        float3 H = normalize(vlight - vpos);
        float ndoth = dot(vnorm, H);

        #if SHADING_MODE == 3
            // blinn-phong
            specular += lightColor * (ndoth > 0 && ndotl > 0 ? pow(ndoth, MaterialShininess) : 0);
        #endif // SHADING_MODE == 3

        #if SHADING_MODE == 4
            // blinn-torrance-sparrow
            float ndote = -dot(vpos, vnorm);
            float edoth = -dot(vpos, H);
            float ss = MaterialShininess*MaterialShininess;
            float d = ss / (ndoth*ndoth * (ss-1) + 1);
            float Gb = 2 * ndote * ndoth / edoth;
            float Gc = 2 * ndotl * ndoth / edoth;
            float G = min(1,min(Gb, Gc));
            specular += lightColor * d*d * G / ndote;
        #endif // SHADING_MODE == 4
    #endif // SHADING_MODE > 2
}
#endif // SHADING_MODE > 0

float2 TexCoord(float2 texcoord)
{
    return float2(texcoord.x,
           #if FLIP_V
                    1 -  // invert y-coordinate
           #endif
                       texcoord.y);
}

#if HAS_REFLECTION_TEX || SHADING_MODE > 0
float3 GetViewNormal(PS_INPUT input, bool isFrontFace)
{
    float3 vnorm = normalize(input.vnorm);
    #if DOUBLE_SIDED
        // workaround for nvidia cards (instead of ?: operator)
        vnorm = lerp(-vnorm, vnorm, 
          #if INVERSE_SCALE && !AUTO_NORMALS
            isFrontFace
          #else
            !isFrontFace
          #endif
            );
    #endif
    #if HAS_BUMP_TEX
        float3 vt = normalize(input.vtextangent);
        float3 vb = normalize(input.vtexbinormal);
        float3 nm = BumpTexture.Sample(samLinear, TexCoord(input.tex.bump)).rgb;
        vnorm = normalize(vt*(0.5-nm.x)*2 + vb*(0.5-nm.y)*2 - vnorm*(0.5-nm.z)*2);
    #endif
    return vnorm;
}
#endif

// end of file
