/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file EffectManagerD3D10Impl.cpp

#include "stdafx.h"
#include "EffectManagerD3D10Impl.h"
#include "SceneGraph.h"
#include "SceneUtils.h"
#include "d3d10/AccessProviderD3D10.h"

using namespace collada;


//
// inline effects
//

static const char g_shaderRender[] =
	#include "RenderD3D10.fx.h"
;

static const char g_shaderCommon[] =
	#include "RenderCommonD3D10.fxh.h"
;


class MyInclude : public ID3D10Include
{
	STDMETHOD(Open)(THIS_ D3D10_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
	{
		if( 0 == strcmp(pFileName, "common.fxh") )
		{
			*ppData = &g_shaderCommon;
			*pBytes = sizeof(g_shaderCommon)-1;  // exclude '\0' terminator
			return S_OK;
		}

		assert(m_buffer.empty());

		FILE *f = NULL;
		fopen_s(&f, pFileName, "rb");
		if( !f )
		{
			return E_FAIL;
		}
		fseek(f, 0, SEEK_END);
		m_buffer.resize(ftell(f));
		fseek(f, 0, SEEK_SET);

		*ppData = &m_buffer[0];
		*pBytes = (UINT) m_buffer.size();
		fread(&m_buffer[0], 1, m_buffer.size(), f);
		fclose(f);

		return S_OK;
	}
	STDMETHOD(Close)(THIS_ LPCVOID pData)
	{
		m_buffer.clear();
		return S_OK;
	}
	std::vector<char> m_buffer;
};

static MyInclude g_include;

static const float g_gamma = 2.2f;

static D3DXVECTOR3 GammaCorrection(const D3DXVECTOR3 &v, float gamma)
{
	return D3DXVECTOR3(pow(v.x, gamma), pow(v.y, gamma), pow(v.z, gamma));
}

static D3DXVECTOR4 GammaCorrection(const D3DXVECTOR4 &v, float gamma)
{
	return D3DXVECTOR4(pow(v.x, gamma), pow(v.y, gamma), pow(v.z, gamma), pow(v.w, gamma));
}



//
// material flags constants
//


// global
static const RuntimeMaterial RM_GLOBAL_MASK         = 0xf0000000;
static const RuntimeMaterial RM_GLOBAL_INDICATOR    = 0x80000000;

// batch
static const RuntimeMaterial RM_BATCH_MASK          = 0x0f000000;
static const RuntimeMaterial RM_BATCH_INDICATOR     = 0x08000000;
static const RuntimeMaterial RM_DOUBLE_SIDED        = 0x01000000;
static const RuntimeMaterial RM_SINGLE_SIDED        = 0x02000000;
static const RuntimeMaterial RM_AUTO_NORMALS        = 0x04000000;

// node
static const RuntimeMaterial RM_NODE_MASK           = 0x00f00000;
static const RuntimeMaterial RM_NODE_INDICATOR      = 0x00800000;
static const RuntimeMaterial RM_INVERSE_SCALE       = 0x00100000;

// material
static const RuntimeMaterial RM_MATERIAL_MASK       = 0x000fffff;
static const RuntimeMaterial RM_MATERIAL_INDICATOR  = 0x00080000;
static const RuntimeMaterial RM_HAS_EMISSION_TEX    = 0x00000001;
static const RuntimeMaterial RM_HAS_AMBIENT_TEX     = 0x00000002;
static const RuntimeMaterial RM_HAS_DIFFUSE_TEX     = 0x00000004;
static const RuntimeMaterial RM_HAS_SPECULAR_TEX    = 0x00000008;
static const RuntimeMaterial RM_HAS_REFLECTION_TEX  = 0x00000010;
static const RuntimeMaterial RM_HAS_TRANSPARENT_TEX = 0x00000020;
static const RuntimeMaterial RM_HAS_BUMP_TEX        = 0x00000040;
                                                                  //                                      0       1           2             3         4
static const RuntimeMaterial RM_SHADING_MODE        = 0x0001c000; // 0x00004000|0x00008000|0x00010000 : const, Lambert, blinn_torrance, blin_phong, Phong
static const RuntimeMaterial RM_OPACITY_MODE        = 0x00060000; // 0x00020000|0x00040000 : none, a_one, rgb_zero

///////////////////////////////////////////////////////////////////////////////

static UINT32 ExtractFlag(UINT32 flags, UINT32 bits)
{
	UINT32 result = 0;
	int bitcount = 0;
	for( int i = 0; i < sizeof(UINT32) * 8; ++i )
	{
		const UINT32 tmp = 1 << i;
		if( tmp & bits )
		{
			if( tmp & flags )
			{
				result |= 1 << bitcount;
			}
			++bitcount;
		}
	}
	return result;
}

static UINT32 PackFlag(UINT32 bits, UINT32 value)
{
	UINT32 result = 0;
	int bitcount = 0;
	for( int i = 0; i < sizeof(UINT32) * 8; ++i )
	{
		const UINT32 tmp = 1 << i;
		if( tmp & bits )
		{
			if( value & (1 << bitcount) )
			{
				result |= tmp;
			}
			++bitcount;
		}
	}
	return result;
}

static bool CheckTexture(const Effect *effect, Channel channel, Scene *context)
{
	return effect->GetImage(channel, context) != NULL;
}

static RuntimeMaterial GetMaterialFlags(const Material *src, Scene *context)
{
	RuntimeMaterial material = RM_MATERIAL_INDICATOR;

	Effect *e = src->GetEffect(context);

	material |= PackFlag(RM_HAS_EMISSION_TEX,    CheckTexture(e, CH_EMISSION, context));
	material |= PackFlag(RM_HAS_AMBIENT_TEX,     CheckTexture(e, CH_AMBIENT, context));
	material |= PackFlag(RM_HAS_DIFFUSE_TEX,     CheckTexture(e, CH_DIFFUSE, context));
	material |= PackFlag(RM_HAS_SPECULAR_TEX,    CheckTexture(e, CH_SPECULAR, context));
	material |= PackFlag(RM_HAS_REFLECTION_TEX,  CheckTexture(e, CH_REFLECTION, context));
	material |= PackFlag(RM_HAS_TRANSPARENT_TEX, CheckTexture(e, CH_TRANSPARENT, context));
	material |= PackFlag(RM_HAS_BUMP_TEX,        CheckTexture(e, CH_BUMP, context));

	switch( e->GetShader() )
	{
	case SHADER_PHONG: material |= PackFlag(RM_SHADING_MODE, 2); break;
	case SHADER_LAMBERT: material |= PackFlag(RM_SHADING_MODE, 1); break;
	case SHADER_CONSTANT: material |= PackFlag(RM_SHADING_MODE, 0); break;
	case SHADER_BLINN_PHONG: material |= PackFlag(RM_SHADING_MODE, 3); break;
	case SHADER_BLINN_TORRANCE_SPARROW: material |= PackFlag(RM_SHADING_MODE, 4); break;
	default: assert(0); // unknown shader
	}

	switch( e->GetOpacityMode() )
	{
	case OPAQUE_NONE: material |= PackFlag(RM_OPACITY_MODE, 0); break;
	case OPAQUE_A_ONE: material |= PackFlag(RM_OPACITY_MODE, 1); break;
	case OPAQUE_RGB_ZERO: material |= PackFlag(RM_OPACITY_MODE, 2); break;
	default: assert(0); // unknown mode
	}

	assert(0 == (~RM_MATERIAL_MASK & material));
	return material;
}

static RuntimeMaterial GetMaterialFlags(const Batch *src, Scene *context)
{
	RuntimeMaterial material = RM_BATCH_INDICATOR;
	if( src->GetDoubleSided() )
	{
		material |= RM_DOUBLE_SIDED;
		if( !FindVertexBuffer(src->GetMesh(context), "NORMAL") )
		{
			bool found = false;
			for( size_t i = 0; i < src->GetExtraVertexElementCount(); ++i )
			{
				if( 0 == strcmp(src->GetExtraVertexElementDescs()[i].semanticName, "NORMAL") )
				{
					found = true;
					break;
				}
			}
			if( !found )
			{
				material |= RM_AUTO_NORMALS;
			}
		}
	}
	assert(0 == (~RM_BATCH_MASK & material));
	return material;
}

static RuntimeMaterial GetMaterialFlags(const Node *src, Scene *context)
{
	RuntimeMaterial material = RM_NODE_INDICATOR;
	if( D3DXMatrixDeterminant((const D3DXMATRIX *) src->GetWorld(context)) < 0 )
	{
		material |= RM_INVERSE_SCALE;
	}
	assert(0 == (~RM_NODE_MASK & material));
	return material;
}

///////////////////////////////////////////////////////////////////////////////

namespace
{
	struct LightBase
	{
		D3DXVECTOR4 color; // color value in linear space
	};

	struct PointLight : public LightBase
	{
		D3DXVECTOR4 vpos;
		float  a0;
		float  a1;
		float  a2;
		float  dummy;
	};

	struct SpotLight : public LightBase
	{
		D3DXMATRIX proj;   // camera view -> light projection
		D3DXVECTOR4 vpos;
		float  a0;
		float  a1;
		float  a2;
		float  exponent;
	};

	struct DirectLight : public LightBase
	{
		D3DXVECTOR4 vpos;
	};

	class IntToStringCached
	{
		std::set<std::string> m_data;
	public:
		const char* operator() (int value)
		{
			std::stringstream buf;
			buf << value;
			return m_data.insert(buf.str()).first->c_str();
		}
	};
}

///////////////////////////////////////////////////////////////////////////////

EffectManagerD3D10Impl::EffectManagerD3D10Impl(IMessageListener *ls, AccessProviderD3D10 *providerD3D10)
  : m_msg(ls, "D3D10 Effect Manager")
  , m_accessProvider(providerD3D10)
{
	if( FAILED(m_accessProvider->GetDeviceManager()->AddListener(this)) )
		throw std::runtime_error("failed to initialize effect manager");
}

EffectManagerD3D10Impl::~EffectManagerD3D10Impl()
{
	m_accessProvider->GetDeviceManager()->RemoveListener(this);
	assert(!m_effectPool.pPool);
}

void EffectManagerD3D10Impl::SetRenderSettings(const RenderSettingsDX10 *pSettings)
{
	m_settings = *pSettings;

	m_effectPool.pPool.Release();
	m_materials.clear();
}

void EffectManagerD3D10Impl::GetRenderSettings(RenderSettingsDX10 *pSettingsOut)
{
	*pSettingsOut = m_settings;
}

ID3D10EffectTechnique* EffectManagerD3D10Impl::GetTechnique(MaterialBinding *mb, Scene *scene)
{
	IntToStringCached i2s;

	Node *node = mb->GetParentInstance(scene)->GetNode(scene);

	RuntimeMaterial nn = GetMaterialFlags(node, scene);
	assert(nn & RM_NODE_INDICATOR);
	assert(0 == (~RM_NODE_MASK & nn));

	Material *mat = mb->GetMaterial(scene);
	RuntimeMaterial mm = GetMaterialFlags(mat, scene);
	assert(mm & RM_MATERIAL_INDICATOR);
	assert(0 == (~RM_MATERIAL_MASK & mm));

	Batch *batch = mb->GetBatch(scene);
	RuntimeMaterial bb = GetMaterialFlags(batch, scene);
	assert(bb & RM_BATCH_INDICATOR);
	assert(0 == (~RM_BATCH_MASK & bb));

	RuntimeMaterial gg = RM_GLOBAL_INDICATOR;

	RuntimeMaterial key = bb | mm | nn | gg;
	MyMaterialMap::iterator materialIt = m_materials.find(key);
	if( m_materials.end() == materialIt )
	{
		D3D10_SHADER_MACRO macro[] =
		{
			// mesh flags
			{"AUTO_NORMALS",        i2s(ExtractFlag(bb, RM_AUTO_NORMALS))        },
			{"DOUBLE_SIDED",        i2s(RenderSettingsDX10::CULL_FORCE_SINGLE_SIDED != m_settings.cullMode
				&& ((bb & RM_DOUBLE_SIDED) || RenderSettingsDX10::CULL_FORCE_DOUBLE_SIDED == m_settings.cullMode))},

			// node flags
			{"INVERSE_SCALE",       i2s(0 != (nn & RM_INVERSE_SCALE))            },

			// material flags
			{"HAS_EMISSION_TEX",    i2s(ExtractFlag(mm, RM_HAS_EMISSION_TEX))    },
			{"HAS_AMBIENT_TEX",     i2s(ExtractFlag(mm, RM_HAS_AMBIENT_TEX))     },
			{"HAS_DIFFUSE_TEX",     i2s(ExtractFlag(mm, RM_HAS_DIFFUSE_TEX))     },
			{"HAS_SPECULAR_TEX",    i2s(ExtractFlag(mm, RM_HAS_SPECULAR_TEX))    },
			{"HAS_REFLECTION_TEX",  i2s(ExtractFlag(mm, RM_HAS_REFLECTION_TEX))  },
			{"HAS_TRANSPARENT_TEX", i2s(ExtractFlag(mm, RM_HAS_TRANSPARENT_TEX)) },
			{"HAS_BUMP_TEX",        i2s(!m_settings.noBump * ExtractFlag(mm, RM_HAS_BUMP_TEX))   },
			{"OPACITY_MODE",        i2s(ExtractFlag(mm, RM_OPACITY_MODE))        },
			{"SHADING_MODE",        i2s(ExtractFlag(mm, RM_SHADING_MODE))        },

			// grep: lighting globals
			{"DIRECT_LIGHT_COUNT",  i2s(m_effectPoolParams.dirLightCount)   },
			{"SPOT_LIGHT_COUNT",    i2s(m_effectPoolParams.spotLightCount)  },
			{"POINT_LIGHT_COUNT",   i2s(m_effectPoolParams.pointLightCount) },

			// misc
			{"WIREFRAME_MODE",      i2s(m_settings.wireframe)      },
			{"DEFAULT_LIGHT",       i2s(D3DXVec4LengthSq(&m_settings.defaultLightColor) > 1e-5) },
			{"DRAW_NORMALS",        i2s(m_settings.drawNormals)    },
			{"FLIP_V",              i2s(m_settings.flipTextureV)   },
			{"MAYA_AMBIENT_FIX",    i2s(m_settings.mayaAmbientFix) },

			{"TEXTURE_DEBUG",       i2s(m_settings.textureDebug==CH_UNKNOWN ? 0 : m_settings.textureDebug + 1) },

			{NULL, NULL}
		};

		MyMaterial newMat;
		CComPtr<ID3D10Blob> pBlob;
		HRESULT hr;
/*
		if( !m_renderEffect.empty() )
		{
			hr = D3DX10CreateEffectFromFile(
				m_renderEffect.c_str(),             // pFileName
				macro,                              // pDefines
				&g_include,                         // pInclude
				"fx_4_0",                           // pProfile
				D3D10_SHADER_ENABLE_STRICTNESS,     // HLSLFlags
				D3D10_EFFECT_COMPILE_CHILD_EFFECT,  // FXFlags
				m_accessProvider->GetDeviceManager()->GetDevice(),
				m_effectPool.pPool,                 // pEffectPool
				NULL,                               // pPump
				&newMat.pEffect,                    // [out] ppEffect
				&pBlob,                             // [out] ppErrors
				NULL                                // [out] pHResult
			);
		}
		else*/
		{
//			assert( m_commonEffect.empty() );
			hr = D3DX10CreateEffectFromMemory(
				&g_shaderRender,                    // pData
				sizeof(g_shaderRender),             // DataLength
				"RenderD3D10.fx",                   // pSrcFileName
				macro,                              // pDefines
				&g_include,                         // pInclude
				"fx_4_0",                           // pProfile
				D3D10_SHADER_ENABLE_STRICTNESS,     // HLSLFlags
				D3D10_EFFECT_COMPILE_CHILD_EFFECT,  // FXFlags
				m_accessProvider->GetDeviceManager()->GetDevice(),
				m_effectPool.pPool,                 // pEffectPool
				NULL,                               // pPump
				&newMat.pEffect,                    // [out] ppEffect
				&pBlob,                             // [out] ppErrors
				NULL                                // [out] pHResult
			);
		}


		if( pBlob )
		{
			m_msg(SUCCEEDED(hr) ? MSG_SEVERITY_WARNING : MSG_SEVERITY_ERROR) 
				<< "Shader compiler says:\n" << (const char *) pBlob->GetBufferPointer();
		}
		assert(SUCCEEDED(hr));
		assert(newMat.pEffect->IsValid());

		newMat.tech = newMat.pEffect->GetTechniqueByIndex(0);
		newMat.pEffect->Optimize();

		// register new material
		materialIt = m_materials.insert(MyMaterialMap::value_type(key, newMat)).first;
		m_msg(MSG_SEVERITY_INFO) << "material compiled: " << (void *) key;
	}

	return materialIt->second.tech;
}

void EffectManagerD3D10Impl::InitFrame(Scene *scene, const D3DXMATRIX *pView)
{
	//
	// collect light info
	//

	D3DXMATRIX mViewI;
	D3DXMatrixInverse(&mViewI, NULL, pView);

	std::vector<DirectLight> lightDir;
	std::vector<SpotLight>   lightSpot;
	std::vector<PointLight>  lightPoint;

	D3DXVECTOR4 ambient(0,0,0,0);
	for( size_t lightIdx = 0; lightIdx < scene->GetLightInstanceCount(); ++lightIdx )
	{
		LightInstance *li = scene->GetLightInstance(lightIdx);
		Light *light = li->GetEntity(scene);

		switch( light->GetType() )
		{
		case Light::AMBIENT:
			ambient += GammaCorrection(*(const D3DXVECTOR4 *) light->GetColor(), g_gamma);
			break;
		case Light::POINT:
			lightPoint.push_back(PointLight());
			lightPoint.back().color = GammaCorrection(*(const D3DXVECTOR4 *) light->GetColor(), g_gamma);
			lightPoint.back().vpos = ((const D3DXVECTOR4 *) li->GetNode(scene)->GetWorld(scene))[3]; // 4th row
			D3DXVec4Transform(&lightPoint.back().vpos, &lightPoint.back().vpos, pView);
			lightPoint.back().a0 = light->GetAttenuation0();
			lightPoint.back().a1 = light->GetAttenuation1();
			lightPoint.back().a2 = light->GetAttenuation2();
			break;
		case Light::SPOT:
			lightSpot.push_back(SpotLight());
			lightSpot.back().color = GammaCorrection(*(const D3DXVECTOR4 *) light->GetColor(), g_gamma);
			lightSpot.back().vpos = ((const D3DXVECTOR4 *) li->GetNode(scene)->GetWorld(scene))[3]; // 4th row
			D3DXVec4Transform(&lightSpot.back().vpos, &lightSpot.back().vpos, pView);
			D3DXMatrixPerspectiveFovLH(&lightSpot.back().proj, light->GetFalloffAngle(), 1, 1, 2); // zf=1+1/zn; in that case w=z after projection
			D3DXMatrixMultiply(&lightSpot.back().proj, 
				&(mViewI * *(const D3DXMATRIX*) li->GetNode(scene)->GetWorldI(scene)), &lightSpot.back().proj);
			D3DXMatrixTranspose(&lightSpot.back().proj, &lightSpot.back().proj); // pass to shader as column major
			lightSpot.back().a0 = light->GetAttenuation0();
			lightSpot.back().a1 = light->GetAttenuation1();
			lightSpot.back().a2 = light->GetAttenuation2();
			lightSpot.back().exponent = light->GetFalloffExponent();
			break;
		case Light::DIRECT:
			lightDir.push_back(DirectLight());
			lightDir.back().vpos = ((const D3DXVECTOR4 *) li->GetNode(scene)->GetWorld(scene))[2];  // 3rd row
			lightDir.back().color = GammaCorrection(*(const D3DXVECTOR4 *) light->GetColor(), g_gamma);
			D3DXVec4Transform(&lightDir.back().vpos, &lightDir.back().vpos, pView);
			D3DXVec3Normalize((D3DXVECTOR3 *) &lightDir.back().vpos, (D3DXVECTOR3 *) &lightDir.back().vpos);
			break;
		}
	}

	GlobalEffectPoolParams params;
	params.dirLightCount = static_cast<int>(lightDir.size());
	params.spotLightCount = static_cast<int>(lightSpot.size());
	params.pointLightCount = static_cast<int>(lightPoint.size());

	if( memcmp(&params, &m_effectPoolParams, sizeof(GlobalEffectPoolParams))
		|| !m_effectPool.pPool )
	{
		m_effectPoolParams = params;
		CompileEffectPool();
	}



	//
	// apply lights
	//

	m_effectPool.pDefaultLight->SetFloatVector(m_settings.defaultLightColor);
	m_effectPool.pLightAmbient->SetFloatVector(ambient);
	if( !lightDir.empty() )
		m_effectPool.pLightDir->SetRawValue(&lightDir[0], 0, static_cast<UINT>(sizeof(DirectLight) * lightDir.size()));
	if( !lightSpot.empty() )
		m_effectPool.pLightSpot->SetRawValue(&lightSpot[0], 0, static_cast<UINT>(sizeof(SpotLight) * lightSpot.size()));
	if( !lightPoint.empty() )
		m_effectPool.pLightPoint->SetRawValue(&lightPoint[0], 0, static_cast<UINT>(sizeof(PointLight) * lightPoint.size()));
}

HRESULT EffectManagerD3D10Impl::OnSetDevice(ID3D10Device *pd3dDevice)
{
	m_effectPool.pPool.Release();
	m_materials.clear();
	return S_OK;
}

void EffectManagerD3D10Impl::CompileEffectPool()
{
	// free existing effect pool
	m_effectPool.pPool.Release();

	ID3D10Device *pd3dDevice = m_accessProvider->GetDeviceManager()->GetDevice();
	assert(pd3dDevice);

	// Read the D3DX effect file
	CComPtr<ID3D10Blob> pBlob;
	HRESULT hr;

	IntToStringCached i2s;

	D3D10_SHADER_MACRO macro[] =
	{
		// grep: lighting globals
		{"DIRECT_LIGHT_COUNT",  i2s(m_effectPoolParams.dirLightCount)   },
		{"SPOT_LIGHT_COUNT",    i2s(m_effectPoolParams.spotLightCount)  },
		{"POINT_LIGHT_COUNT",   i2s(m_effectPoolParams.pointLightCount) },
		{NULL, NULL}
	};

/*		if( !m_commonEffect.empty() )
	{
		hr = D3DX10CreateEffectPoolFromFile(m_commonEffect.c_str(), NULL, &g_include, "fx_4_0", 
			D3D10_SHADER_ENABLE_STRICTNESS, 0, pd3dDevice, NULL, &m_effectPool.pPool, &pBlob, NULL);
	}
	else */
	{
		hr = D3DX10CreateEffectPoolFromMemory(
			&g_shaderCommon,                  // pData
			sizeof(g_shaderCommon),           // DataLength
			"RenderCommonD3D10.fxh",          // pSrcFileName
			macro,                            // pDefines
			NULL,                             // pInclude
			"fx_4_0",                         // pProfile
			D3D10_SHADER_ENABLE_STRICTNESS,   // HLSLFlags
			0,                                // FXFlags
			pd3dDevice,                       // pDevice
			NULL,                             // pThreadPump
			&m_effectPool.pPool,              // ppEffectPool
			&pBlob,                           // ppErrors
			NULL                              // pHResult
		);
	}

	if( pBlob )
	{
		m_msg(SUCCEEDED(hr) ? MSG_SEVERITY_WARNING : MSG_SEVERITY_ERROR)
			<< "Effect pool shader compiler output:\n"
			<< (const char *) pBlob->GetBufferPointer();
	}
	if( FAILED(hr) )
		throw std::runtime_error("failed to create effect pool");

	ID3D10Effect *pSuperEffect = m_effectPool.pPool->AsEffect();
	assert(pSuperEffect->IsValid());
	assert(pSuperEffect->IsPool());

	// Obtain common shader variables
	m_effectPool.pWorldViewProj  = pSuperEffect->GetVariableByName("WorldViewProj")->AsMatrix();
	m_effectPool.pWorldViewN     = pSuperEffect->GetVariableByName("WorldViewN")->AsMatrix();
	m_effectPool.pWorldView      = pSuperEffect->GetVariableByName("WorldView")->AsMatrix();
	m_effectPool.pProj           = pSuperEffect->GetVariableByName("Proj")->AsMatrix();

	m_effectPool.pLightDir       = pSuperEffect->GetVariableByName("LightDir");
	m_effectPool.pLightSpot      = pSuperEffect->GetVariableByName("LightSpot");
	m_effectPool.pLightPoint     = pSuperEffect->GetVariableByName("LightPoint");
	m_effectPool.pLightAmbient   = pSuperEffect->GetVariableByName("LightAmbient")->AsVector();
	m_effectPool.pDefaultLight   = pSuperEffect->GetVariableByName("DefaultLight")->AsVector();

	m_effectPool.pEmission     = pSuperEffect->GetVariableByName("MaterialEmission")->AsVector();
	m_effectPool.pAmbient      = pSuperEffect->GetVariableByName("MaterialAmbient")->AsVector();
	m_effectPool.pDiffuse      = pSuperEffect->GetVariableByName("MaterialDiffuse")->AsVector();
	m_effectPool.pSpecular     = pSuperEffect->GetVariableByName("MaterialSpecular")->AsVector();
	m_effectPool.pReflection   = pSuperEffect->GetVariableByName("MaterialReflection")->AsVector();
	m_effectPool.pTransparent  = pSuperEffect->GetVariableByName("MaterialTransparent")->AsVector();
	m_effectPool.pTransparency = pSuperEffect->GetVariableByName("MaterialTransparency")->AsScalar();
	m_effectPool.pShininess    = pSuperEffect->GetVariableByName("MaterialShininess")->AsScalar();

	m_effectPool.pEmissionTexture = pSuperEffect->GetVariableByName("EmissionTexture")->AsShaderResource();
	m_effectPool.pAmbientTexture  = pSuperEffect->GetVariableByName("AmbientTexture")->AsShaderResource();
	m_effectPool.pDiffuseTexture  = pSuperEffect->GetVariableByName("DiffuseTexture")->AsShaderResource();
	m_effectPool.pSpecularTexture = pSuperEffect->GetVariableByName("SpecularTexture")->AsShaderResource();
	m_effectPool.pReflectionTexture = pSuperEffect->GetVariableByName("ReflectionTexture")->AsShaderResource();
	m_effectPool.pTransparentTexture = pSuperEffect->GetVariableByName("TransparentTexture")->AsShaderResource();
	m_effectPool.pBumpTexture     = pSuperEffect->GetVariableByName("BumpTexture")->AsShaderResource();
}

// end of file
