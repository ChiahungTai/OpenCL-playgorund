/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2009 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGSceneAdapter.h

#pragma once

#include "SceneGraph.h"
#include "AutoPtr.h"

#include <set>

using namespace collada;

struct IViewFactoryCreator
{
	virtual IViewFactory* CreatePlugin()=0;
};


typedef IViewFactory* (*CreatePluginProc)();

class CViewFactoryCreatorProc: public IViewFactoryCreator
{
	CreatePluginProc m_proc;
public:
	CViewFactoryCreatorProc(CreatePluginProc proc);
	virtual IViewFactory* CreatePlugin();
};

template <class A1>
class CViewFactoryCreatorProc1: public IViewFactoryCreator
{
	IViewFactory* (*m_proc)(A1);
	A1 m_arg1;
public:
	CViewFactoryCreatorProc1(IViewFactory* (*proc)(A1), A1 a1)
		: m_proc(proc)
		, m_arg1(a1)
	{}
	virtual IViewFactory* CreatePlugin()
	{
		return m_proc(m_arg1);
	}
};

template <class A1, class A2>
class CViewFactoryCreatorProc2: public IViewFactoryCreator
{
	IViewFactory* (*m_proc)(A1, A2);
	A1 m_arg1;
	A2 m_arg2;
public:
	CViewFactoryCreatorProc2(IViewFactory* (*proc)(A1, A2), A1 a1, A2 a2)
		: m_proc(proc)
		, m_arg1(a1)
		, m_arg2(a2)
	{}
	virtual IViewFactory* CreatePlugin()
	{
		return m_proc(m_arg1, m_arg2);
	}
};


class SceneAdapter
	: public Scene
{
public:
	SceneAdapter(Scene *s, size_t pluginId, IViewFactoryCreator* pluginGen);
	virtual ~SceneAdapter();
	Scene* GetSource() const;


	//
	// Scene interface
	//

	virtual MemoryManager* GetMemoryManager() const throw();
	virtual IMessageListener* GetMessageListener() const throw();
	virtual bool QueryEditableScene(EditableScene **ppResult);
	virtual unsigned int AddRef();
	virtual unsigned int Release();
	virtual void AddListener(ISceneListener *sl);
	virtual void RemoveListener(ISceneListener *sl);
	virtual Animation* GetAnimation(size_t idx);
	virtual size_t GetAnimationCount() const;
	virtual AnimationClip* GetAnimationClip(size_t idx);
	virtual size_t GetAnimationClipCount() const;
	virtual Effect* GetEffect(size_t idx);
	virtual size_t GetEffectCount() const;
	virtual Material* GetMaterial(size_t idx);
	virtual size_t GetMaterialCount() const;
	virtual Node* GetNode(size_t idx);
	virtual size_t GetNodeCount() const;
	virtual Image* GetImage(size_t idx);
	virtual size_t GetImageCount() const;
	virtual MeshInstance* GetMeshInstance(size_t idx);
	virtual size_t GetMeshInstanceCount() const;
	virtual Mesh* GetMesh(size_t idx);
	virtual size_t GetMeshCount() const;
	virtual SkinControllerInstance* GetSkinCtrlInstance(size_t idx);
	virtual size_t GetSkinCtrlInstanceCount() const;
	virtual MorphControllerInstance* GetMorphCtrlInstance(size_t idx);
	virtual size_t GetMorphCtrlInstanceCount() const;
	virtual SkinController* GetSkinCtrl(size_t idx);
	virtual size_t GetSkinCtrlCount() const;
	virtual MorphController* GetMorphCtrl(size_t idx);
	virtual size_t GetMorphCtrlCount() const;
	virtual CameraInstance* GetCameraInstance(size_t idx);
	virtual size_t GetCameraInstanceCount() const;
	virtual Camera* GetCamera(size_t idx);
	virtual size_t GetCameraCount() const;
	virtual size_t GetLightInstanceCount() const;
	virtual LightInstance* GetLightInstance(size_t index);
	virtual size_t GetLightCount() const;
	virtual Light* GetLight(size_t index);
	virtual NodeLayers RegisterLayer(const char *layerName);
	virtual size_t GetLayerCount() const;
	virtual NodeLayers GetLayer(size_t index) const;
	virtual const char* GetLayerName(size_t index) const;
	virtual void SetWorldUpAxis(UpAxis axis);
	virtual void SetTargetUpAxis(const float *v3right, const float *v3up, const float *v3in);
	virtual const float* GetUpAxis() const; //float4x4
	virtual const float* GetSceneCenter(NodeLayers incl, NodeLayers excl, AccessProviderLocal *providerLocal) const; // float3
	virtual const float* GetSceneAABB(NodeLayers incl, NodeLayers excl, AccessProviderLocal *providerLocal) const; // float3
	virtual const TCHAR* GetDebugInfo() const;
	virtual IViewFactory* GetViewFactory(size_t id) const;
	virtual size_t RegisterViewFactory(IViewFactory *pFactory, Scene *contextImmediate);
	virtual void UnregisterViewFactory(size_t id);
	virtual Base* ResolveContext(Base *target);
	virtual Scene* GetPrevContext() const;
	virtual IView* CreateView(Scene *context, Base *o, size_t viewId);
	virtual void ReleaseView(Base *o, size_t viewId);

protected:
	size_t GetPluginId() const { return m_pluginId; }

private:
	AutoPtr<Scene> m_target;
	std::set<ISceneListener *> m_listeners;
	size_t m_pluginId;
	unsigned int m_refCount;
	bool m_isPluginExternal;
};

// end of file
