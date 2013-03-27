/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file RenderD3D10Impl.cpp

#include "stdafx.h"

#include "RenderD3D10Impl.h"
#include "SceneUtils.h"
#include "SceneGraph.h"

#include "ResourceWrapperD3D10Impl.h"
#include "d3d10/AccessProviderD3D10.h"

using namespace collada;


///////////////////////////////////////////////////////////////////////////////

static const float g_gamma = 2.2f;

static D3DXVECTOR3 GammaCorrection(const D3DXVECTOR3 &v, float gamma)
{
	return D3DXVECTOR3(pow(v.x, gamma), pow(v.y, gamma), pow(v.z, gamma));
}

static D3DXVECTOR4 GammaCorrection(const D3DXVECTOR4 &v, float gamma)
{
	return D3DXVECTOR4(pow(v.x, gamma), pow(v.y, gamma), pow(v.z, gamma), pow(v.w, gamma));
}

static int ToMS(const LARGE_INTEGER &perfCounter0, const LARGE_INTEGER &perfCounter1)
{
	LARGE_INTEGER perfFreq;
	QueryPerformanceFrequency(&perfFreq);
	return int((double) (perfCounter1.QuadPart - perfCounter0.QuadPart) / (double) perfFreq.QuadPart * 1000 + 0.5);
}

///////////////////////////////////////////////////////////////////////////////

CSceneRenderDX10::CSceneRenderDX10(IMessageListener *ls, collada::AccessProviderD3D10 *providerD3D10)
  : m_msg(ls, "D3D10 render")
  , m_provider(providerD3D10)
  , m_pScene(NULL)
  , m_dxViewId(-1)
  , m_effectManager(new EffectManagerD3D10Impl(ls, providerD3D10))
{
	m_provider->GetDeviceManager()->AddListener(this);
}

CSceneRenderDX10::~CSceneRenderDX10()
{
	DetachScene();
	m_provider->GetDeviceManager()->RemoveListener(this);
}

void CSceneRenderDX10::AttachScene(Scene *pScene)
{
	DetachScene();

	assert(pScene);
	m_pScene = pScene;

	m_factoryDX.reset(new DXObjectsFactory(m_provider));
	m_dxViewId = m_pScene->RegisterViewFactory(m_factoryDX.get(), NULL);
}

void CSceneRenderDX10::DetachScene()
{
	if( m_pScene )
	{
		m_objectsCache.clear();

		m_pScene->UnregisterViewFactory(m_dxViewId);
		m_dxViewId = -1;
		m_factoryDX.reset();

		m_pScene = NULL;
	}
}

HRESULT CSceneRenderDX10::OnSetDevice(ID3D10Device *pd3dDevice)
{
	m_pDefaultSRV.Release();
	m_objectsCache.clear(); // need to clear object cache because of cached input layouts

	if( pd3dDevice )
	{
		//
		// create image that will be substituted at the place of missing textures
		//

		static const unsigned char bytes[] = {
			0,  0,  0,  0,     0,  0,  0,  0,   255,255,255,255,   255,255,255,255,
			0,  0,  0,  0,     0,  0,  0,  0,   255,255,255,255,   255,255,255,255,
			255,255,255,255,   255,255,255,255,     0,  0,  0,  0,     0,  0,  0,  0,
			255,255,255,255,   255,255,255,255,     0,  0,  0,  0,     0,  0,  0,  0,
			0,  0,  0,  0,     0,  0,  0,  0,   255,255,255,255,   255,255,255,255,
			0,  0,  0,  0,     0,  0,  0,  0,   255,255,255,255,   255,255,255,255,
			255,255,255,255,   255,255,255,255,     0,  0,  0,  0,     0,  0,  0,  0,
			255,255,255,255,   255,255,255,255,     0,  0,  0,  0,     0,  0,  0,  0,
			0,  0,  0,  0,     0,  0,  0,  0,   255,255,255,255,   255,255,255,255,
			0,  0,  0,  0,     0,  0,  0,  0,   255,255,255,255,   255,255,255,255,
			255,255,255,255,   255,255,255,255,     0,  0,  0,  0,     0,  0,  0,  0,
			255,255,255,255,   255,255,255,255,     0,  0,  0,  0,     0,  0,  0,  0,
			0,  0,  0,  0,     0,  0,  0,  0,   255,255,255,255,   255,255,255,255,
			0,  0,  0,  0,     0,  0,  0,  0,   255,255,255,255,   255,255,255,255,
			255,255,255,255,   255,255,255,255,     0,  0,  0,  0,     0,  0,  0,  0,
			255,255,255,255,   255,255,255,255,     0,  0,  0,  0,     0,  0,  0,  0,
		};

		const UINT width = 8;
		const UINT height = 8;
		const UINT bpp = 32;

		CD3D10_TEXTURE2D_DESC tex_desc(
			DXGI_FORMAT_R8G8B8A8_UNORM,   // format
			width,
			height,
			1,        // array size
			1         // mip levels
		);

		D3D10_SUBRESOURCE_DATA data;
		data.SysMemPitch = width * bpp >> 3;
		data.SysMemSlicePitch = 0;
		data.pSysMem = bytes;

		CComPtr<ID3D10Texture2D> tex;
		HRESULT hr;
		V_RETURN(pd3dDevice->CreateTexture2D(&tex_desc, &data, &tex));
		V_RETURN(pd3dDevice->CreateShaderResourceView(tex, NULL, &m_pDefaultSRV));
	}

	return S_OK;
}

const TCHAR* CSceneRenderDX10::GetDebugInfo(NodeLayers layersInclude, NodeLayers layersExclude) const
{
	using std::endl;

#ifdef _UNICODE
	std::wstringstream buf;
#else _UNICODE
	std::stringstream buf;
#endif _UNICODE

	buf.str(_T(""));

	ObjectsCache::const_iterator cacheResult = m_objectsCache.find(ObjectsCacheKey(layersInclude, layersExclude));
	if( m_objectsCache.end() != cacheResult )
	{
		const ObjectsCache::value_type::second_type &lists = cacheResult->second;

		UINT indicesOpaque = 0;
		for( ItemList::const_iterator it = lists.first.begin(); it != lists.first.end(); ++it )
		{
			indicesOpaque += it->dxbatch->GetIndexCount();
		}

		UINT indicesTransparent = 0;
		for( ItemList::const_iterator it = lists.second.begin(); it != lists.second.end(); ++it )
		{
			indicesTransparent += it->dxbatch->GetIndexCount();
		}

		buf << _T("Draw calls opaque: ") << lists.first.size() << endl;
		buf << _T("Triangles opaque:  ") << (indicesOpaque/3) << endl;
		buf << _T("Draw calls transparent: ") << lists.second.size() << endl;
		buf << _T("Triangles transparent:  ") << (indicesTransparent/3) << endl;
	}

	m_dbgInfoCache = buf.str();
	return m_dbgInfoCache.c_str();
}

HRESULT CSceneRenderDX10::Render(const D3DXMATRIX *pView, const D3DXMATRIX *pProj,
                                 NodeLayers layersInclude, NodeLayers layersExclude)
{
	assert(m_provider->GetDeviceManager()->GetDevice());
	assert(m_pScene);

	m_effectManager->InitFrame(m_pScene, pView);


	//
	// prepare separate opaque and transparent geometry lists
	//

	ObjectsCacheKey listKey(layersInclude, layersExclude);
	ObjectsCache::_Pairib cacheResult = m_objectsCache.insert(
		ObjectsCache::value_type(listKey, ObjectsCache::value_type::second_type()));
	ObjectsCache::value_type::second_type &lists = cacheResult.first->second;
	if( cacheResult.second )
	{
		for( GeometryInstanceIterator it(m_pScene); it.IsValid(); it.StepNext() )
		{
			Node *node = it.GetNode();
			NodeLayers layers = node->GetLayers();
			if( 0 == (layers & layersInclude) || 0 != (layers & layersExclude) )
			{
				continue; // node was excluded or was not included
			}

			Batch *batch = it.GetBatch();

			MaterialBinding *mb = it.GetMaterialBinding();

			Item item;
//			item.mb = mb;
			item.tech = m_effectManager->GetTechnique(mb, m_pScene);
			item.dxbatch = batch->GetViewT<DXBatchView>(m_dxViewId, m_pScene);
			item.dxmesh = batch->GetMesh(m_pScene)->GetViewT<const DXMeshView>(m_dxViewId, m_pScene);
			item.node = node;
			item.mat = mb->GetMaterial(m_pScene);
			item.effect = item.mat->GetEffect(m_pScene);
			item.pInputLayout = mb->GetViewT<DXMaterialBindingView>(m_dxViewId, m_pScene)->GetInputLayout(m_effectManager.get());

			for( int ch = 0; ch < CH_COUNT; ++ch )
			{
				Image *im = item.effect->GetImage((Channel) ch, m_pScene);
				const DXImageView *dximg = im ? im->GetViewT<const DXImageView>(m_dxViewId, m_pScene) : NULL;
				item.textures[ch].colorLinear = GammaCorrection(*(const D3DXVECTOR4 *) item.effect->GetColor((Channel) ch), g_gamma);
				ID3D10ShaderResourceView* pSRV = dximg ? dximg->GetSRV() : NULL;
				item.textures[ch].pSRV = pSRV ? pSRV : m_pDefaultSRV;
			}

			assert(item.dxbatch);
			if( item.dxbatch->GetIndexBuffer() )
			{
				bool transparent = (mb->GetMaterial(m_pScene)->GetEffect(m_pScene)->GetOpacityMode() != OPAQUE_NONE);
				(transparent ? lists.second : lists.first).push_back(item);
			}
		}
	}


	//
	// render mesh
	//

	m_effectManager->m_effectPool.pProj->SetMatrix((float *) pProj);

	RenderHelper(lists.first, pView, pProj);
	RenderHelper(lists.second, pView, pProj);

	return S_OK;
}

void CSceneRenderDX10::RenderHelper(ItemList &list, const D3DXMATRIX *pView, const D3DXMATRIX *pProj)
{
	ID3D10Device *device = m_provider->GetDeviceManager()->GetDevice();
	assert(device);

	ItemList::iterator endIt = list.end();
	for( ItemList::iterator it = list.begin(); it != endIt; ++it )
	{
		Item &item = *it;

		D3DXMatrixMultiply(&item.mWorldView, (const D3DXMATRIX*) item.node->GetWorld(m_pScene), pView);
		D3DXMatrixInverse(&item.mWorldViewN, NULL, &item.mWorldView);
		D3DXMatrixTranspose(&item.mWorldViewN, &item.mWorldViewN);
		D3DXMatrixMultiply(&item.mWorldViewProj, &item.mWorldView, pProj);


		//
		// setup input assembler
		//

		device->IASetIndexBuffer(item.dxbatch->GetIndexBuffer(), item.dxbatch->GetIndexBufferFormat(), 0);
		device->IASetPrimitiveTopology(item.dxbatch->GetPrimitiveTopology());
		device->IASetInputLayout(item.pInputLayout);
		device->IASetVertexBuffers(
			0,      // start slot
			item.dxmesh->GetVertexBuffersCount(),
			item.dxmesh->GetVertexBuffers(),
			item.dxmesh->GetStrides(),
			item.dxmesh->GetOffsets()
		);


		//
		// setup matrices
		//

		m_effectManager->m_effectPool.pWorldView->SetMatrix(const_cast<D3DXMATRIX&>(item.mWorldView));
		m_effectManager->m_effectPool.pWorldViewN->SetMatrix(const_cast<D3DXMATRIX&>(item.mWorldViewN));
		m_effectManager->m_effectPool.pWorldViewProj->SetMatrix(const_cast<D3DXMATRIX&>(item.mWorldViewProj));


		//
		// setup material
		//

		m_effectManager->m_effectPool.pEmission->SetFloatVector(item.textures[CH_EMISSION].colorLinear);
		m_effectManager->m_effectPool.pEmissionTexture->SetResource(item.textures[CH_EMISSION].pSRV);

		m_effectManager->m_effectPool.pAmbient->SetFloatVector(item.textures[CH_AMBIENT].colorLinear);
		m_effectManager->m_effectPool.pAmbientTexture->SetResource(item.textures[CH_AMBIENT].pSRV);

		m_effectManager->m_effectPool.pDiffuse->SetFloatVector(item.textures[CH_DIFFUSE].colorLinear);
		m_effectManager->m_effectPool.pDiffuseTexture->SetResource(item.textures[CH_DIFFUSE].pSRV);

		m_effectManager->m_effectPool.pSpecular->SetFloatVector(item.textures[CH_SPECULAR].colorLinear);
		m_effectManager->m_effectPool.pSpecularTexture->SetResource(item.textures[CH_SPECULAR].pSRV);

		m_effectManager->m_effectPool.pReflection->SetFloatVector(item.textures[CH_REFLECTION].colorLinear);
		m_effectManager->m_effectPool.pReflectionTexture->SetResource(item.textures[CH_REFLECTION].pSRV);

		m_effectManager->m_effectPool.pTransparent->SetFloatVector(item.textures[CH_TRANSPARENT].colorLinear);
		m_effectManager->m_effectPool.pTransparentTexture->SetResource(item.textures[CH_TRANSPARENT].pSRV);

		m_effectManager->m_effectPool.pBumpTexture->SetResource(item.textures[CH_BUMP].pSRV);


		m_effectManager->m_effectPool.pTransparency->SetFloat(item.effect->GetTransparency());
		m_effectManager->m_effectPool.pShininess->SetFloat(item.effect->GetShininess());


		//
		// draw batch
		//

		D3D10_TECHNIQUE_DESC techDesc;
	//	ID3D10EffectTechnique *tech = m_effectManager->GetTechnique(item.mb, m_pScene);
		item.tech->GetDesc(&techDesc);
		for( UINT p = 0; p < techDesc.Passes; ++p )
		{
			item.tech->GetPassByIndex(p)->Apply(p);
			device->DrawIndexed(item.dxbatch->GetIndexCount(), 0, 0);
		}
	}
}

void CSceneRenderDX10::SetRenderSettings(const RenderSettingsDX10 *pSettings)
{
	m_effectManager->SetRenderSettings(pSettings);
	m_objectsCache.clear();
}

void CSceneRenderDX10::GetRenderSettings(RenderSettingsDX10 *pSettingsOut)
{
	m_effectManager->GetRenderSettings(pSettingsOut);
}


// end of file
