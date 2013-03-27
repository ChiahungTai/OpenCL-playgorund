/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGScene.h

#pragma once

#include "mathstub.h"

#include "SceneGraph.h"
#include "SGImplBase.h"
#include "RefCountedImpl.h"
#include "AutoPtr.h"

#include <set>
#include <map>

using namespace collada;


// forward declarations
//namespace collada
//{
	class NodeImpl;
	class MeshInstanceImpl;
	class SkinControllerInstanceImpl;
	class SkinControllerImpl;
	class MorphControllerInstanceImpl;
	class MorphControllerImpl;
	class CameraImpl;
	class LightImpl;
	class EditableMeshImpl;
	class ImageImpl;
	class EffectImpl;
	class MaterialImpl;

	class AnimationClipImpl;
	class AnimationImpl;


	typedef InstanceImpl<LightInstance, Light>  LightInstanceImpl;
	typedef InstanceImpl<CameraInstance, Camera>  CameraInstanceImpl;
//}

namespace collada
{
	struct AccessProviderLocal;
}

///////////////////////////////////////////////////////////////////////////////

class SceneImpl : public RefCountedImpl<EditableScene>
{
public:
	SceneImpl(IMessageListener *pMsgListener, MemoryManager *pMemManager);
	virtual ~SceneImpl();

	// implementation specific
	void OnNativeObjectCreate(Base *pObj);
	void OnNativeObjectDestroy(Base *pObj);

	//
	// Scene interface
	//

	virtual MemoryManager* GetMemoryManager() const throw();
	virtual IMessageListener* GetMessageListener() const throw();

	virtual bool QueryEditableScene(EditableScene **ppResult);

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

	virtual LightInstance* GetLightInstance(size_t index);
	virtual size_t GetLightInstanceCount() const;

	virtual Light* GetLight(size_t index);
	virtual size_t GetLightCount() const;

	virtual NodeLayers RegisterLayer(const char *layerName);
	virtual size_t GetLayerCount() const;
	virtual NodeLayers GetLayer(size_t index) const;
	virtual const char* GetLayerName(size_t index) const;

	virtual void SetTargetUpAxis(const float *v3right, const float *v3up, const float *v3in);
	virtual void SetWorldUpAxis(UpAxis axis);
	virtual const float* GetUpAxis() const; // float4x4

	virtual const TCHAR* GetDebugInfo() const;

	virtual IViewFactory* GetViewFactory(size_t id) const;
	virtual size_t RegisterViewFactory(IViewFactory *pFactory, Scene *contextImmediate);
	virtual void UnregisterViewFactory(size_t viewId);
	virtual Base* ResolveContext(Base *target);
	virtual Scene* GetPrevContext() const;

	virtual IView* CreateView(Scene *context, Base *o, size_t viewId); // called by Base implementation
	virtual void ReleaseView(Base *o, size_t viewId); // called by Base implementation

	// this two functions are deprecated and to be removed
	virtual const float* GetSceneCenter(NodeLayers incl, NodeLayers excl, AccessProviderLocal *providerLocal) const; // float3
	virtual const float* GetSceneAABB(NodeLayers incl, NodeLayers excl, AccessProviderLocal *providerLocal) const; // float3


	//
	// EditableScene interface
	//

	virtual Animation* CreateAnimation(const AnimationDataDesc dataDescs[], size_t dataDescCount,
	                                   const AnimationSamplerDesc samplerDescs[], size_t samplerDescCount);
	virtual AnimationClip* CreateAnimationClip();
	virtual Node* CreateNode(Node *parent);
	virtual Image* CreateImage();
	virtual Effect* CreateEffect();
	virtual Material* CreateMaterial();
	virtual Camera* CreateCamera();
	virtual Light* CreateLight();
	virtual EditableMesh* CreateEditableMesh(const VertexBufferDesc *bufferDescs, size_t bufferDescCount);
	virtual SkinController* CreateSkinController(size_t weightedJointCount, size_t bindingCount);
	virtual MorphController* CreateMorphController();

	virtual CameraInstance* CreateCameraInstance(Node *node, Camera *camera);
	virtual LightInstance* CreateLightInstance(Node *node, Light *light);
	virtual MeshInstance* CreateRigidMeshInstance(Node *node, Mesh *mesh);
	virtual SkinControllerInstance* CreateSkinCtrlInstance(Node *node, SkinController *skinCtrl);
	virtual MorphControllerInstance* CreateMorphCtrlInstance(Node *node, MorphController *morphCtrl);

private:
	SceneImpl(const SceneImpl &); // no copy
	SceneImpl& operator =(SceneImpl &);


	AutoPtr<MemoryManager> m_memManager;
	AutoPtr<IMessageListener> m_msgListener;

	std::set<ISceneListener *> m_listeners;

	typedef std::map<Base *, size_t> ObjectViewCount; // registered views count for each object
	ObjectViewCount m_objectViewCount;

	typedef std::set<Base *> ObjectSet;
	ObjectSet m_nativeObjects;

	PtrOwnerArray<NodeImpl>             m_nodes;
	PtrOwnerArray<CameraInstanceImpl>   m_camInstances;
	PtrOwnerArray<LightInstanceImpl>    m_lightInstances;
	PtrOwnerArray<MeshInstanceImpl>     m_meshInstances;

	PtrOwnerArray<SkinControllerInstanceImpl> m_skinnedMeshInstances;
	PtrOwnerArray<SkinControllerImpl>         m_skinControllers;

	PtrOwnerArray<MorphControllerInstanceImpl> m_morphedMeshInstances;
	PtrOwnerArray<MorphControllerImpl>         m_morphControllers;

	PtrOwnerArray<CameraImpl> m_cameras;
	PtrOwnerArray<LightImpl>  m_lights;
	PtrOwnerArray<EditableMeshImpl> m_staticMeshes;

	PtrOwnerArray<ImageImpl>    m_images;
	PtrOwnerArray<EffectImpl>   m_effects;
	PtrOwnerArray<MaterialImpl> m_materials;

	PtrOwnerArray<AnimationClipImpl> m_animationClips;
	PtrOwnerArray<AnimationImpl>     m_animations;

	typedef std::vector<std::pair<NodeLayers, std::string> > LayersVector;
	LayersVector m_layers;

	mutable string_t m_dbgInfoCache;

	float4x4 m_targetAxis;
	UpAxis m_worldUp;
	float4x4 m_upAxis;
	void UpdateUpAxis();

	struct Bounds
	{
		float3 center;
		float3 AABB;
	};
	typedef std::pair<NodeLayers,NodeLayers> IncludeExludePair;
	typedef std::map<IncludeExludePair, Bounds> BoundsCacheType;
	mutable BoundsCacheType m_sceneBoundsCache;

	typedef std::pair<IViewFactory *, Scene*> RegisteredFactory;
	typedef std::map<size_t, RegisteredFactory> RegisteredFactoryList;
	RegisteredFactoryList m_factories;

	void CalcSceneBounds(const IncludeExludePair &ie, Bounds &bounds, AccessProviderLocal *providerLocal) const;

	template <class T>
	T* CreateObject(PtrOwnerArray<T> &storage);

	template <class T, class Arg1>
	T* CreateObject(PtrOwnerArray<T> &storage, Arg1 a1);

	template <class T, class Arg1, class Arg2>
	T* CreateObject(PtrOwnerArray<T> &storage, Arg1 a1, Arg2 a2);

	template <class T, class Arg1, class Arg2, class Arg3>
	T* CreateObject(PtrOwnerArray<T> &storage, Arg1 a1, Arg2 a2, Arg3 a3);

	template <class T, class Arg1, class Arg2, class Arg3, class Arg4>
	T* CreateObject(PtrOwnerArray<T> &storage, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4);
};


// end of file
