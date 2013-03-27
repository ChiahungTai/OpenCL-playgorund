/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file EffectManagerD3D10Impl.h

#pragma once

#include "AutoPtr.h"
#include "RefCountedImpl.h"
#include "SceneGraphBase.h"
#include "MsgListenerWrapper.h"
#include "d3d10/DeviceManagerD3D10.h"

#include <map>
#include <atlcomcli.h>

namespace collada
{
	struct AccessProviderD3D10;
};

using namespace collada;


//namespace
//{


struct RenderSettingsDX10
{
	// display
	D3DXVECTOR4 defaultLightColor;
	bool wireframe;
	bool drawNormals;

	// misc
	collada::Channel textureDebug;

	// workarounds
	enum CullMode
	{
		CULL_DEFAULT,
		CULL_FORCE_SINGLE_SIDED,
		CULL_FORCE_DOUBLE_SIDED,
	} cullMode;
	bool flipTextureV;
	bool mayaAmbientFix;
	bool noBump;

	// default values
	RenderSettingsDX10()
		: defaultLightColor(0,0,0,0)
		, wireframe(false)
		, drawNormals(false)
		, textureDebug(collada::CH_UNKNOWN)
		, cullMode(CULL_DEFAULT)
		, flipTextureV(true)
		, mayaAmbientFix(true)
		, noBump(false)
	{
	}
};


typedef UINT32 RuntimeMaterial;

class EffectManagerD3D10Impl
	: public RefCountedImpl<RefCounted>
	, private IDeviceListenerD3D10
{
public:
	EffectManagerD3D10Impl(IMessageListener *ls, AccessProviderD3D10 *providerD3D10);
	virtual ~EffectManagerD3D10Impl();

	void InitFrame(Scene *scene, const D3DXMATRIX *pView);
	ID3D10EffectTechnique* GetTechnique(MaterialBinding *mb, Scene *scene);

	void SetRenderSettings(const RenderSettingsDX10 *pSettings);
	void GetRenderSettings(RenderSettingsDX10 *pSettingsOut);


	//
	// Effect pool contains the common effect and variables for all materials' child effects
	//

	struct MyEffectPool
	{
		CComPtr<ID3D10EffectPool>   pPool;

		ID3D10EffectMatrixVariable* pWorldViewProj;
		ID3D10EffectMatrixVariable* pWorldViewN;
		ID3D10EffectMatrixVariable* pWorldView;
		ID3D10EffectMatrixVariable* pProj;

		ID3D10EffectVariable* pLightDir;
		ID3D10EffectVariable* pLightSpot;
		ID3D10EffectVariable* pLightPoint;

		ID3D10EffectVectorVariable* pLightAmbient;
		ID3D10EffectVectorVariable* pDefaultLight;

		ID3D10EffectVectorVariable* pEmission;
		ID3D10EffectVectorVariable* pAmbient;
		ID3D10EffectVectorVariable* pDiffuse;
		ID3D10EffectVectorVariable* pSpecular;
		ID3D10EffectVectorVariable* pReflection;
		ID3D10EffectVectorVariable* pTransparent;
		ID3D10EffectScalarVariable* pTransparency;
		ID3D10EffectScalarVariable* pShininess;

		ID3D10EffectShaderResourceVariable* pEmissionTexture;
		ID3D10EffectShaderResourceVariable* pAmbientTexture;
		ID3D10EffectShaderResourceVariable* pDiffuseTexture;
		ID3D10EffectShaderResourceVariable* pSpecularTexture;
		ID3D10EffectShaderResourceVariable* pReflectionTexture;
		ID3D10EffectShaderResourceVariable* pTransparentTexture;
		ID3D10EffectShaderResourceVariable* pBumpTexture;
	};

	MyEffectPool m_effectPool; // TODO: make private


private:
	// IDeviceListenerD3D10
	virtual HRESULT OnSetDevice(ID3D10Device *pd3dDevice);

private:
	struct GlobalEffectPoolParams
	{
		int dirLightCount;
		int spotLightCount;
		int pointLightCount;
	};
	GlobalEffectPoolParams m_effectPoolParams;
	void CompileEffectPool();


	struct MyMaterial
	{
		CComPtr<ID3D10Effect>   pEffect;            // owner
		ID3D10EffectTechnique*  tech;
	};

	typedef std::map<RuntimeMaterial, MyMaterial> MyMaterialMap;
	MyMaterialMap m_materials;
	RenderSettingsDX10 m_settings;
	MsgListenerWrapper m_msg;
	AutoPtr<AccessProviderD3D10> m_accessProvider;
};

//} // end of unnamed namespace
// end of file
