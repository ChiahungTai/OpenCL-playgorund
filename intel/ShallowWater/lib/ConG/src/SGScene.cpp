/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGScene.cpp

#include "stdafx.h"

#include "SGScene.h"
#include "MemoryManager.h"
#include "MessageListener.h"

// objects implementation
#include "SGCamera.h"
#include "SGLight.h"
#include "SGImage.h"
#include "SGEffect.h"
#include "SGMaterial.h"
#include "SGMesh.h"
#include "SGSkinnedMesh.h"
#include "SGMorphedMesh.h"
#include "SGNode.h"
#include "SGAnimation.h"

#include <tchar.h>
#include <sstream>

///////////////////////////////////////////////////////////////////////////////
//   0123
// 0 ***0 - right
// 1 ***0 - up
// 2 ***0 - in
// 3 0001
static void AxisMatrix(float4x4 *out, UpAxis axis)
{
	memset(out, 0, sizeof(float4x4));
	switch( axis )
	{
	case UPAXIS_X:
		(*out)(0, 1) = -1;  // Negative y
		(*out)(1, 0) =  1;  // Positive x
		(*out)(2, 2) =  1;  // Positive z
		break;
	case UPAXIS_Y:
		(*out)(0, 0) =  1;  // Positive x
		(*out)(1, 1) =  1;  // Positive y
		(*out)(2, 2) =  1;  // Positive z
		break;
	case UPAXIS_Z:
		(*out)(0, 0) =  1;  // Positive x
		(*out)(1, 2) =  1;  // Positive z
		(*out)(2, 1) = -1;  // Negative y
		break;
	default:
		assert(false);
	}
	(*out)(3, 3) = 1;
}


template <class T>
T* SceneImpl::CreateObject(PtrOwnerArray<T> &storage)
{
	T *obj = new T(this);
	storage.push_back(obj);
	OnNativeObjectCreate(obj);
	return obj;
}

template <class T, class Arg1>
T* SceneImpl::CreateObject(PtrOwnerArray<T> &storage, Arg1 a1)
{
	T *obj = new T(this, a1);
	storage.push_back(obj);
	OnNativeObjectCreate(obj);
	return obj;
}

template <class T, class Arg1, class Arg2>
T* SceneImpl::CreateObject(PtrOwnerArray<T> &storage, Arg1 a1, Arg2 a2)
{
	T *obj = new T(this, a1, a2);
	storage.push_back(obj);
	OnNativeObjectCreate(obj);
	return obj;
}

template <class T, class Arg1, class Arg2, class Arg3>
T* SceneImpl::CreateObject(PtrOwnerArray<T> &storage, Arg1 a1, Arg2 a2, Arg3 a3)
{
	T *obj = new T(this, a1, a2, a3);
	storage.push_back(obj);
	OnNativeObjectCreate(obj);
	return obj;
}

template <class T, class Arg1, class Arg2, class Arg3, class Arg4>
T* SceneImpl::CreateObject(PtrOwnerArray<T> &storage, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4)
{
	T *obj = new T(this, a1, a2, a3, a4);
	storage.push_back(obj);
	OnNativeObjectCreate(obj);
	return obj;
}

///////////////////////////////////////////////////////////////////////////////

SceneImpl::SceneImpl(IMessageListener *ls, MemoryManager *pMemManager)
  : m_worldUp(UPAXIS_Z)
  , m_memManager(pMemManager)
  , m_msgListener(ls)
  , m_targetAxis(1, 0, 0, 0, // Positive x
                 0, 1, 0, 0, // Positive y
                 0, 0,-1, 0, // Negative z
                 0, 0, 0, 1)
{
	UpdateUpAxis();
}

SceneImpl::~SceneImpl()
{
	while( !m_listeners.empty() )
	{
		// FIXME: possible hang here
		(*m_listeners.begin())->OnSceneDestroy(this);
	}
	assert(m_listeners.empty()); // all listeners should remove them self on scene destroy
	assert(m_factories.empty());
}

MemoryManager* SceneImpl::GetMemoryManager() const throw()
{
	return m_memManager;
}

IMessageListener* SceneImpl::GetMessageListener() const throw()
{
	return m_msgListener;
}

bool SceneImpl::QueryEditableScene(EditableScene **ppResult)
{
	AutoPtr<EditableScene> result(this);
	DetachRawPtr(result, ppResult);
	return true;
}

void SceneImpl::AddListener(ISceneListener *ls)
{
	assert(m_listeners.count(ls) == 0);
	m_listeners.insert(ls);
}

void SceneImpl::RemoveListener(ISceneListener *ls)
{
	assert(m_listeners.count(ls) != 0);
	m_listeners.erase(ls);
}

void SceneImpl::UpdateUpAxis()
{
	float4x4 source;
	AxisMatrix(&source, m_worldUp);
	MatrixInverse(&source, NULL, &source);
	MatrixMultiply(&m_upAxis, &source, &m_targetAxis);
}

Image* SceneImpl::CreateImage()
{
	return CreateObject<ImageImpl>(m_images);
}

Effect* SceneImpl::CreateEffect()
{
	return CreateObject<EffectImpl>(m_effects);
}

Material* SceneImpl::CreateMaterial()
{
	return CreateObject<MaterialImpl>(m_materials);
}

Animation* SceneImpl::CreateAnimation(const AnimationDataDesc dataDescs[], size_t dataDescCount,
                                      const AnimationSamplerDesc samplerDescs[], size_t samplerDescCount)
{
	return CreateObject<AnimationImpl>(m_animations, dataDescs, dataDescCount, samplerDescs, samplerDescCount);
}

AnimationClip* SceneImpl::CreateAnimationClip()
{
	return CreateObject<AnimationClipImpl>(m_animationClips);
}

Node* SceneImpl::CreateNode(Node *parent)
{
	assert(!parent || (this == parent->GetNativeContext() && dynamic_cast<NodeImpl*>(parent)));
	return CreateObject<NodeImpl>(m_nodes, static_cast<NodeImpl *>(parent));
}

Camera* SceneImpl::CreateCamera()
{
	return CreateObject<CameraImpl>(m_cameras);
}

Light* SceneImpl::CreateLight()
{
	return CreateObject<LightImpl>(m_lights);
}

EditableMesh* SceneImpl::CreateEditableMesh(const VertexBufferDesc *bufferDescs, size_t bufferDescCount)
{
	return CreateObject<EditableMeshImpl>(m_staticMeshes, bufferDescs, bufferDescCount);
}

SkinController* SceneImpl::CreateSkinController(size_t weightedJointCount, size_t bindingCount)
{
	return CreateObject<SkinControllerImpl>(m_skinControllers, weightedJointCount, bindingCount);
}

MorphController* SceneImpl::CreateMorphController()
{
	return CreateObject<MorphControllerImpl>(m_morphControllers);
}

CameraInstance* SceneImpl::CreateCameraInstance(Node *node, Camera *camera)
{
	return CreateObject<CameraInstanceImpl>(m_camInstances, camera, node);
}

LightInstance* SceneImpl::CreateLightInstance(Node *node, Light *light)
{
	return CreateObject<LightInstanceImpl>(m_lightInstances, light, node);
}

MeshInstance* SceneImpl::CreateRigidMeshInstance(Node *node, Mesh *mesh)
{
	return CreateObject<MeshInstanceImpl>(m_meshInstances, node, mesh);
}

SkinControllerInstance* SceneImpl::CreateSkinCtrlInstance(Node *node, SkinController *skinCtrl)
{
	return CreateObject<SkinControllerInstanceImpl>(m_skinnedMeshInstances, node, skinCtrl);
}

MorphControllerInstance* SceneImpl::CreateMorphCtrlInstance(Node *node, MorphController *morphCtrl)
{
	return CreateObject<MorphControllerInstanceImpl>(m_morphedMeshInstances, node, morphCtrl);
}

Animation* SceneImpl::GetAnimation(size_t idx)
{
	return m_animations[idx];
}

size_t SceneImpl::GetAnimationCount() const
{
	return m_animations.size();
}

AnimationClip* SceneImpl::GetAnimationClip(size_t idx)
{
	return m_animationClips[idx];
}

size_t SceneImpl::GetAnimationClipCount() const
{
	return m_animationClips.size();
}

Effect* SceneImpl::GetEffect(size_t idx)
{
	return m_effects[idx];
}

size_t SceneImpl::GetEffectCount() const
{
	return m_effects.size();
}

Material* SceneImpl::GetMaterial(size_t idx)
{
	return m_materials[idx];
}

size_t SceneImpl::GetMaterialCount() const
{
	return m_materials.size();
}

Node* SceneImpl::GetNode(size_t idx)
{
	return m_nodes[idx];
}

size_t SceneImpl::GetNodeCount() const
{
	return m_nodes.size();
}

Image* SceneImpl::GetImage(size_t idx)
{
	return m_images[idx];
}

size_t SceneImpl::GetImageCount() const
{
	return m_images.size();
}

MeshInstance* SceneImpl::GetMeshInstance(size_t idx)
{
	return m_meshInstances[idx];
}

size_t SceneImpl::GetMeshInstanceCount() const
{
	return m_meshInstances.size();
}

Mesh* SceneImpl::GetMesh(size_t idx)
{
	return m_staticMeshes[idx];
}

size_t SceneImpl::GetMeshCount() const
{
	return m_staticMeshes.size();
}

SkinControllerInstance* SceneImpl::GetSkinCtrlInstance(size_t idx)
{
	return m_skinnedMeshInstances[idx];
}

size_t SceneImpl::GetSkinCtrlInstanceCount() const
{
	return m_skinnedMeshInstances.size();
}

MorphControllerInstance* SceneImpl::GetMorphCtrlInstance(size_t idx)
{
	return m_morphedMeshInstances[idx];
}

size_t SceneImpl::GetMorphCtrlInstanceCount() const
{
	return m_morphedMeshInstances.size();
}

SkinController* SceneImpl::GetSkinCtrl(size_t idx)
{
	return m_skinControllers[idx];
}

size_t SceneImpl::GetSkinCtrlCount() const
{
	return m_skinControllers.size();
}

MorphController* SceneImpl::GetMorphCtrl(size_t idx)
{
	return m_morphControllers[idx];
}

size_t SceneImpl::GetMorphCtrlCount() const
{
	return m_morphControllers.size();
}

size_t SceneImpl::GetCameraInstanceCount() const
{
	return m_camInstances.size();
}

CameraInstance* SceneImpl::GetCameraInstance(size_t index)
{
	return m_camInstances[index];
}

size_t SceneImpl::GetCameraCount() const
{
	return m_cameras.size();
}

Camera* SceneImpl::GetCamera(size_t index)
{
	return m_cameras[index];
}

size_t SceneImpl::GetLightInstanceCount() const
{
	return m_lightInstances.size();
}

LightInstance* SceneImpl::GetLightInstance(size_t index)
{
	return m_lightInstances[index];
}

size_t SceneImpl::GetLightCount() const
{
	return m_lights.size();
}

Light* SceneImpl::GetLight(size_t index)
{
	return m_lights[index];
}

NodeLayers SceneImpl::RegisterLayer(const char *layerName)
{
	assert(m_layers.size() < 63);
	m_layers.push_back(LayersVector::value_type(NodeLayers(1) << (m_layers.size() + 1), layerName?layerName:""));
	return m_layers.back().first;
}

size_t SceneImpl::GetLayerCount() const
{
	return m_layers.size();
}

NodeLayers SceneImpl::GetLayer(size_t index) const
{
	return m_layers[index].first;
}

const char* SceneImpl::GetLayerName(size_t index) const
{
	return m_layers[index].second.c_str();
}

void SceneImpl::SetWorldUpAxis(UpAxis axis)
{
	m_worldUp = axis;
	UpdateUpAxis();
}

void SceneImpl::SetTargetUpAxis(const float *v3right, const float *v3up, const float *v3in)
{
	*(float3 *) &m_targetAxis(0,0) = *(float3 *) v3right;
	*(float3 *) &m_targetAxis(1,0) = *(float3 *) v3up;
	*(float3 *) &m_targetAxis(2,0) = *(float3 *) v3in;
	UpdateUpAxis();
}

const float* SceneImpl::GetUpAxis() const
{
	return m_upAxis;
}

const float* SceneImpl::GetSceneCenter(NodeLayers incl, NodeLayers excl, AccessProviderLocal *providerLocal) const // float3
{
	BoundsCacheType::_Pairib result =  m_sceneBoundsCache.insert(
		BoundsCacheType::value_type(IncludeExludePair(incl, excl), Bounds()));
	if( result.second )
	{
		CalcSceneBounds(result.first->first, result.first->second, providerLocal);
	}
	return result.first->second.center;
}

const float* SceneImpl::GetSceneAABB(NodeLayers incl, NodeLayers excl, AccessProviderLocal *providerLocal) const // float3
{
	BoundsCacheType::_Pairib result =  m_sceneBoundsCache.insert(
		BoundsCacheType::value_type(IncludeExludePair(incl, excl), Bounds()));
	if( result.second )
	{
		CalcSceneBounds(result.first->first, result.first->second, providerLocal);
	}
	return result.first->second.AABB;
}

#include "AABBCalcAlgorithm.h"
#include "SceneUtils.h"
#include "ProcessorBaseImpl.h"
#include "AccessProviderDefault.h"

void SceneImpl::CalcSceneBounds(const IncludeExludePair &ie, Bounds &bounds, AccessProviderLocal *providerLocal) const
{
	float3 min(-1,-1,-1), max(1,1,1);
	bool first = true;
	const float half = 0.5f;
	static const float3 s[8] = {
		float3(half,half,half),float3(half,half,-half),float3(half,-half,half),float3(half,-half,-half),
		float3(-half,half,half),float3(-half,half,-half),float3(-half,-half,half),float3(-half,-half,-half),
	};
	for( size_t i = 0; i < GetMeshInstanceCount(); ++i )
	{
		MeshInstance *inst = m_meshInstances[i]; //GetMeshInstance(i);
		Node *node = inst->GetNode(NULL);
		NodeLayers layers = node->GetLayers();
		if( 0 == (layers & ie.first) || 0 != (layers & ie.second) )
			continue; // node was excluded or was not included

		AutoPtr<IAABBCalcAlgorithm> alg;
		CreateAABBCalcAlgorithmDefault(providerLocal, &alg);
		AlgorithmResource inputs[] = 
		{
			{ IAABBCalcAlgorithm::IN_POSITIONS, FindVertexBuffer(inst->GetEntity(NULL), "POSITION") },
			{ -1, NULL } // terminator
		};
		AutoPtr<MemoryObject> aabb;
		m_memManager->Allocate(sizeof(IAABBCalcAlgorithm::AABBType), NULL, &aabb);
		AlgorithmResource outputs[] = 
		{
			{ IAABBCalcAlgorithm::OUT_AABB, aabb },
			{ -1, NULL } // terminator
		};
		AutoPtr<Processor>(new ProcessorSimple(alg, inputs, outputs));

		AutoPtr<AccessorReadDefault> aabbReader;
		providerLocal->CreateAccessorReadDefault(aabb, &aabbReader);
		MapHelper<const IAABBCalcAlgorithm::AABBType, AccessorReadDefault> aabbMapped(aabbReader);

		const float3 *a = (const float3 *) aabbMapped[0].center;
		const float3 *b = (const float3 *) aabbMapped[0].aabb;
		const float4x4 *m = (const float4x4 *) inst->GetNode(NULL)->GetWorld(NULL);

		for( int i = 0; i < 8; ++i )
		{
			float3 tmp(a->x+s[i].x*b->x, a->y+s[i].y*b->y, a->z+s[i].z*b->z);
			Vec3TransformCoord(&tmp, &tmp, m);
			if( first )
			{
				first = false;
				min = max = tmp;
			}
			else
			{
				Vec3Maximize(&max, &max, &tmp);
				Vec3Minimize(&min, &min, &tmp);
			}
		}
	}

	bounds.center = (max + min) / 2;
	bounds.AABB = max - min;

	// TODO: calc bounds for skinned instances
}

IViewFactory* SceneImpl::GetViewFactory(size_t id) const
{
	assert(m_factories.count(id));
	return m_factories.find(id)->second.first;
}

size_t SceneImpl::RegisterViewFactory(IViewFactory *pFactory, Scene *contextImmediate)
{
	size_t freeId = 0;
	while( m_factories.count(freeId) )
	{
		++freeId;
	}
	m_factories[freeId] = RegisteredFactory(pFactory, contextImmediate);

	if( contextImmediate )
	{
		try
		{
			for( ObjectSet::const_iterator it = m_nativeObjects.begin(); it != m_nativeObjects.end(); ++it )
			{
				(*it)->GetView(freeId, contextImmediate);
			}
		}
		catch(...)
		{
			UnregisterViewFactory(freeId);
			throw;
		}
	}

	return freeId;
}

void SceneImpl::UnregisterViewFactory(size_t id)
{
	assert(m_factories.count(id));

	for( ObjectViewCount::const_iterator it = m_objectViewCount.begin(); it != m_objectViewCount.end(); )
	{
		ObjectViewCount::const_iterator temp(it++);
		temp->first->DetachView(id); // this may cause iterator to become invalid
	}

	m_factories[id].first->OnDetach();
	m_factories.erase(id);
}

IView* SceneImpl::CreateView(Scene *context, Base *o, size_t viewId)
{
	assert(m_factories.count(viewId));
	IView *result = m_factories.find(viewId)->second.first->CreateView(context, o, viewId);
	if( result )
	{
		++m_objectViewCount[o];
	}
	return result;
}

void SceneImpl::ReleaseView(Base *o, size_t viewId)
{
	assert(m_factories.count(viewId));
	assert(m_objectViewCount.count(o));
	assert(m_objectViewCount[o] > 0);
	if( 0 == --m_objectViewCount[o] )
	{
		m_objectViewCount.erase(o);
	}
}

Base* SceneImpl::ResolveContext(Base *target)
{
	assert(this == target->GetNativeContext());
	return target;
}

Scene* SceneImpl::GetPrevContext() const
{
	return NULL;
}

const TCHAR* SceneImpl::GetDebugInfo() const
{
	using std::endl;

	#ifdef _UNICODE
		std::wstringstream buf;
	#else _UNICODE
		std::stringstream buf;
	#endif _UNICODE


	buf.str(_T(""));
	buf << _T("Geometry objects: ") << m_staticMeshes.size() << endl;
	buf << _T("Skin controllers: ") << m_skinControllers.size() << endl;
	buf << _T("Geometry instances: ") << m_meshInstances.size() << endl;
	buf << _T("Skin ctrl instances: ") << m_skinnedMeshInstances.size() << endl;
	buf << _T("Space nodes: ") << m_nodes.size() << endl;
	buf << _T("Cameras: ") << m_camInstances.size() << endl;
	buf << _T("Lights: ") << m_lightInstances.size() << endl;
	buf << _T("Images: ") << m_images.size() << endl;
	buf << _T("Effects: ") << m_effects.size() << endl;
	buf << _T("Materials: ") << m_materials.size() << endl;
	buf << _T("Animations: ") << m_animations.size() << endl;

	m_dbgInfoCache = buf.str();
	return m_dbgInfoCache.c_str();
}

void SceneImpl::OnNativeObjectCreate(Base *pObj)
{
	m_nativeObjects.insert(pObj);
	for( RegisteredFactoryList::const_iterator it = m_factories.begin(); it != m_factories.end(); ++it )
	{
		if( it->second.second )
		{
			// touch view if it's not on demand
			pObj->GetView(it->first, it->second.second);
		}
	}
}

void SceneImpl::OnNativeObjectDestroy(Base *pObj)
{
	assert(m_nativeObjects.count(pObj));
	m_nativeObjects.erase(pObj);
}



//
// factory
//

namespace collada
{
	void CreateScene(EditableScene **ppResult, IMessageListener *pMsgListener, MemoryManager *pMemManager)
	{
		AutoPtr<MemoryManager> memMan(pMemManager);
		AutoPtr<IMessageListener> msgLs(pMsgListener);
		if( !memMan ) CreateMemoryManager(&memMan);
		if( !msgLs ) CreateMessageListenerStd(&msgLs);
		AutoPtr<EditableScene> result(new SceneImpl(msgLs, memMan));
		DetachRawPtr(result, ppResult);
	}
}

// end of file
