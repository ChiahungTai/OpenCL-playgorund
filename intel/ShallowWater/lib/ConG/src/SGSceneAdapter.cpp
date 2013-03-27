/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2009 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGSceneAdapter.cpp

#include "stdafx.h"

#include "SGSceneAdapter.h"
#include "SGUtils.h"

///////////////////////////////////////////////////////////////////////////////

CViewFactoryCreatorProc::CViewFactoryCreatorProc(CreatePluginProc proc)
  : m_proc(proc)
{
}

IViewFactory* CViewFactoryCreatorProc::CreatePlugin()
{
	return m_proc();
}

///////////////////////////////////////////////////////////////////////////////

SceneAdapter::SceneAdapter(Scene *s, size_t pluginId, IViewFactoryCreator* pPluginGen)
  : m_target(s)
  , m_pluginId(pluginId)
  , m_isPluginExternal(-1 != pluginId)
  , m_refCount(0)
{
	assert(m_target);
	assert(pPluginGen);
	if( !m_isPluginExternal )
	{
		m_pluginId = m_target->RegisterViewFactory(pPluginGen->CreatePlugin(), NULL);
	}
}

SceneAdapter::~SceneAdapter()
{
	if( !m_isPluginExternal )
	{
		m_target->UnregisterViewFactory(m_pluginId);
	}
	_ASSERT(m_listeners.empty());
}


Scene* SceneAdapter::GetSource() const
{
	return m_target;
}


//
// Scene interface
//

MemoryManager* SceneAdapter::GetMemoryManager() const throw()
{
	return m_target->GetMemoryManager();
}

IMessageListener* SceneAdapter::GetMessageListener() const throw()
{
	return m_target->GetMessageListener();
}

bool SceneAdapter::QueryEditableScene(EditableScene **ppResult)
{
	assert(NULL == *ppResult);
	return false;
}

unsigned int SceneAdapter::AddRef()
{
	return ++m_refCount;
}

unsigned int SceneAdapter::Release()
{
	assert(m_refCount > 0);
	if( 0 == --m_refCount )
	{
		// destroy current adapter and all its children
		while( !m_listeners.empty() )
		{
			// FIXME: possible hang here
			(*m_listeners.begin())->OnSceneDestroy(this);
		}
		assert(m_listeners.empty()); // all listeners should remove them self on scene destroy
		delete this;
		return 0;
	}
	return m_refCount;
}

void SceneAdapter::AddListener(ISceneListener *ls)
{
	assert(m_refCount > 0);
	assert(m_listeners.count(ls) == 0);
	m_listeners.insert(ls);
}

void SceneAdapter::RemoveListener(ISceneListener *ls)
{
	assert(m_listeners.count(ls) != 0);
	m_listeners.erase(ls);
}

Animation* SceneAdapter::GetAnimation(size_t idx)
{
	return SafeResolveContext<Animation>(this, m_target->GetAnimation(idx));
}

size_t SceneAdapter::GetAnimationCount() const
{
	return m_target->GetAnimationCount();
}

AnimationClip* SceneAdapter::GetAnimationClip(size_t idx)
{
	return SafeResolveContext<AnimationClip>(this, m_target->GetAnimationClip(idx));
}

size_t SceneAdapter::GetAnimationClipCount() const
{
	return m_target->GetAnimationClipCount();
}

Effect* SceneAdapter::GetEffect(size_t idx)
{
	return m_target->GetEffect(idx);
}

size_t SceneAdapter::GetEffectCount() const
{
	return m_target->GetEffectCount();
}

Material* SceneAdapter::GetMaterial(size_t idx)
{
	return m_target->GetMaterial(idx);
}

size_t SceneAdapter::GetMaterialCount() const
{
	return m_target->GetMaterialCount();
}

Node* SceneAdapter::GetNode(size_t idx)
{
	return SafeResolveContext<Node>(this, m_target->GetNode(idx));
}

size_t SceneAdapter::GetNodeCount() const
{
	return m_target->GetNodeCount();
}

Image* SceneAdapter::GetImage(size_t idx)
{
	return SafeResolveContext<Image>(this, m_target->GetImage(idx));
}

size_t SceneAdapter::GetImageCount() const
{
	return m_target->GetImageCount();
}

MeshInstance* SceneAdapter::GetMeshInstance(size_t idx)
{
	return SafeResolveContext<MeshInstance>(this, m_target->GetMeshInstance(idx));
}

size_t SceneAdapter::GetMeshInstanceCount() const
{
	return m_target->GetMeshInstanceCount();
}

Mesh* SceneAdapter::GetMesh(size_t idx)
{
	return SafeResolveContext<Mesh>(this, m_target->GetMesh(idx));
}

size_t SceneAdapter::GetMeshCount() const
{
	return m_target->GetMeshCount();
}

SkinControllerInstance* SceneAdapter::GetSkinCtrlInstance(size_t idx)
{
	return SafeResolveContext<SkinControllerInstance>(this, m_target->GetSkinCtrlInstance(idx));
}

size_t SceneAdapter::GetSkinCtrlInstanceCount() const
{
	return m_target->GetSkinCtrlInstanceCount();
}

MorphControllerInstance* SceneAdapter::GetMorphCtrlInstance(size_t idx)
{
	return SafeResolveContext<MorphControllerInstance>(this, m_target->GetMorphCtrlInstance(idx));
}

size_t SceneAdapter::GetMorphCtrlInstanceCount() const
{
	return m_target->GetMorphCtrlInstanceCount();
}

SkinController* SceneAdapter::GetSkinCtrl(size_t idx)
{
	return SafeResolveContext<SkinController>(this, m_target->GetSkinCtrl(idx));
}

size_t SceneAdapter::GetSkinCtrlCount() const
{
	return m_target->GetSkinCtrlCount();
}

MorphController* SceneAdapter::GetMorphCtrl(size_t idx)
{
	return SafeResolveContext<MorphController>(this, m_target->GetMorphCtrl(idx));
}

size_t SceneAdapter::GetMorphCtrlCount() const
{
	return m_target->GetMorphCtrlCount();
}

CameraInstance* SceneAdapter::GetCameraInstance(size_t idx)
{
	return SafeResolveContext<CameraInstance>(this, m_target->GetCameraInstance(idx));
}

size_t SceneAdapter::GetCameraInstanceCount() const
{
	return m_target->GetCameraInstanceCount();
}

Camera* SceneAdapter::GetCamera(size_t idx)
{
	return SafeResolveContext<Camera>(this, m_target->GetCamera(idx));
}

size_t SceneAdapter::GetCameraCount() const
{
	return m_target->GetCameraCount();
}

size_t SceneAdapter::GetLightInstanceCount() const
{
	return m_target->GetLightInstanceCount();
}

LightInstance* SceneAdapter::GetLightInstance(size_t index)
{
	return SafeResolveContext<LightInstance>(this, m_target->GetLightInstance(index));
}

size_t SceneAdapter::GetLightCount() const
{
	return m_target->GetLightCount();
}

Light* SceneAdapter::GetLight(size_t index)
{
	return SafeResolveContext<Light>(this, m_target->GetLight(index));
}

NodeLayers SceneAdapter::RegisterLayer(const char *layerName)
{
	return m_target->RegisterLayer(layerName);
}

size_t SceneAdapter::GetLayerCount() const
{
	return m_target->GetLayerCount();
}

NodeLayers SceneAdapter::GetLayer(size_t index) const
{
	return m_target->GetLayer(index);
}

const char* SceneAdapter::GetLayerName(size_t index) const
{
	return m_target->GetLayerName(index);
}

void SceneAdapter::SetWorldUpAxis(UpAxis axis)
{
	m_target->SetWorldUpAxis(axis);
}

void SceneAdapter::SetTargetUpAxis(const float *v3right, const float *v3up, const float *v3in)
{
	m_target->SetTargetUpAxis(v3right, v3up, v3in);
}

const float* SceneAdapter::GetUpAxis() const //float4x4
{
	return m_target->GetUpAxis();
}

const float* SceneAdapter::GetSceneCenter(NodeLayers incl, NodeLayers excl, AccessProviderLocal *providerLocal) const // float3
{
	return m_target->GetSceneCenter(incl, excl, providerLocal);
}

const float* SceneAdapter::GetSceneAABB(NodeLayers incl, NodeLayers excl, AccessProviderLocal *providerLocal) const // float3
{
	return m_target->GetSceneAABB(incl, excl, providerLocal);
}

const TCHAR* SceneAdapter::GetDebugInfo() const
{
	return m_target->GetDebugInfo();
}

IViewFactory* SceneAdapter::GetViewFactory(size_t id) const
{
	return m_target->GetViewFactory(id);
}

size_t SceneAdapter::RegisterViewFactory(IViewFactory *pFactory, Scene *contextImmediate)
{
	return m_target->RegisterViewFactory(pFactory, contextImmediate);
}

void SceneAdapter::UnregisterViewFactory(size_t id)
{
	m_target->UnregisterViewFactory(id);
}

Base* SceneAdapter::ResolveContext(Base *target)
{
	if( this != target->GetNativeContext() )
	{
		return m_target->ResolveContext(target);
	}
	return target;
}

Scene* SceneAdapter::GetPrevContext() const
{
	return m_target;
}

IView* SceneAdapter::CreateView(Scene *context, Base *o, size_t viewId)
{
	return m_target->CreateView(context, o, viewId);
}

void SceneAdapter::ReleaseView(Base *o, size_t viewId)
{
	m_target->ReleaseView(o, viewId);
}

// end of file
