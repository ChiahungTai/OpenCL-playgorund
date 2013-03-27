/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file RenderD3D10Impl.h

#pragma once

#include "d3d10/DeviceManagerD3D10.h"

#include "SceneGraphBase.h"
#include "MsgListenerWrapper.h"
#include "AutoPtr.h"
#include "RefCountedImpl.h"

#include "EffectManagerD3D10Impl.h"

#include <list>
#include <map>
#include <vector>
#include <string>
#include <vector>

#include <D3DX10math.h>
#include <atlcomcli.h>


// forward declarations
class DXObjectsFactory;
class DXBatchView;
class DXMeshView;

namespace collada
{
	struct AccessProviderD3D10;
}

///////////////////////////////////////////////////////////////////////////////

class CSceneRenderDX10
	: public collada::RefCountedImpl<collada::RefCounted>
	, private collada::IDeviceListenerD3D10
{
#ifdef _UNICODE
	typedef std::wstring string_t;
#else _UNICODE
	typedef std::string string_t;
#endif _UNICODE

public:
	CSceneRenderDX10(collada::IMessageListener *ls, collada::AccessProviderD3D10 *providerD3D10);

	void AttachScene(collada::Scene *pScene);
	void DetachScene();

	size_t GetDXWrapperId() const { return m_dxViewId; }

//	void SetShaderFiles(
//		const TCHAR *render = NULL,
//		const TCHAR *common = NULL
//	);
//	ID3D10Effect* GetSuperEffect() const;

	// these two functions are deprecated and will be moved to effect manager soon
	void SetRenderSettings(const RenderSettingsDX10 *pSettings);
	void GetRenderSettings(RenderSettingsDX10 *pSettingsOut);

	HRESULT Render(const D3DXMATRIX *pView, const D3DXMATRIX *pProj, collada::NodeLayers layersInclude = -1, collada::NodeLayers layersExclude = 0);

	const TCHAR* GetDebugInfo(collada::NodeLayers layersInclude, collada::NodeLayers layersExclude) const;

protected:
	// destroy via Release
	virtual ~CSceneRenderDX10();

private:
	// IDeviceListenerD3D10
	virtual HRESULT OnSetDevice(ID3D10Device *pd3dDevice);

private:
	struct Item
	{
		ID3D10EffectTechnique *tech;
		const DXBatchView *dxbatch;
		const DXMeshView *dxmesh;
		collada::Node *node;
	//	collada::MaterialBinding *mb;
		const collada::Material *mat;
		const collada::Effect *effect;
		ID3D10InputLayout *pInputLayout;

		struct
		{
			ID3D10ShaderResourceView* pSRV;
			D3DXVECTOR4 colorLinear;
		} textures[collada::CH_COUNT];

		D3DXMATRIX mWorldView;
		D3DXMATRIX mWorldViewN; // the transpose of the inverse of the mWorldView
		D3DXMATRIX mWorldViewProj;
	};

	struct ObjectsCacheKey
	{
		collada::NodeLayers _incl;
		collada::NodeLayers _excl;
		ObjectsCacheKey(collada::NodeLayers incl, collada::NodeLayers excl)
			: _incl(incl), _excl(excl)
		{
		}
		bool operator < (const ObjectsCacheKey &other) const
		{
			return memcmp(this, &other, sizeof(ObjectsCacheKey)) < 0;
		}
	};
	typedef std::list<Item> ItemList;
	typedef std::map<ObjectsCacheKey, std::pair<ItemList,ItemList> > ObjectsCache;
	ObjectsCache m_objectsCache;

	MsgListenerWrapper m_msg;
	
	std::auto_ptr<EffectManagerD3D10Impl> m_effectManager;

	string_t m_renderEffect;
	string_t m_commonEffect;

	collada::AutoPtr<collada::Scene> m_pScene;

	collada::AutoPtr<collada::AccessProviderD3D10> m_provider;
	CComPtr<ID3D10ShaderResourceView> m_pDefaultSRV;

	std::auto_ptr<DXObjectsFactory> m_factoryDX;
	size_t m_dxViewId;

	mutable string_t m_dbgInfoCache;

	void RenderHelper(ItemList &list, const D3DXMATRIX *pView, const D3DXMATRIX *pProj);
};



// end of file
