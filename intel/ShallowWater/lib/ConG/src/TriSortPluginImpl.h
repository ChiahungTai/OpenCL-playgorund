/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file TriSortPluginImpl.h

#pragma once

#include "TriSortPlugin.h"

#include "SGSceneAdapter.h"
#include "SGMeshAdapter.h"
#include "SGGeometryInstanceAdapter.h"

#include "RefCountedImpl.h"

using namespace collada;


///////////////////////////////////////////////////////////////////////////////

class SortedMeshInstanceImpl;
class TriSortPluginFactory;


class SortedBatchImpl : public BatchAdapter
{
public:
	SortedBatchImpl(Batch *srcBatch, SortedMeshInstanceImpl *inst, TriSortPluginFactory *factory, Scene *context);

	// Batch interface override
	virtual MemoryObject* GetIndexData() const;

private:
	AutoPtr<MemoryObject> m_vtxSrc;
	AutoPtr<MemoryObject> m_idxbuf;
	size_t m_triCount;
	SortedMeshInstanceImpl *m_instance;
	TriSortPluginFactory *m_factory;
};

///////////////////////////////////////////////////////////////////////////////

class SortedMeshImpl : public MeshAdapter
{
public:
	SortedMeshImpl(Mesh *src, SortedMeshInstanceImpl *inst, TriSortPluginFactory *factory, Scene *context);

	// Mesh interface override
	virtual Batch* GetBatch(size_t batchIdx, Scene *context) const;

private:
	PtrOwnerArray<SortedBatchImpl> m_batches;
	SortedMeshInstanceImpl *m_instance;
};

///////////////////////////////////////////////////////////////////////////////

class SortedMeshInstanceImpl : public GeometryInstanceAdapter<MeshInstance, MeshInstance, Mesh>
{
public:
	SortedMeshInstanceImpl(MeshInstance *src, TriSortPluginFactory *factory, Scene *context);

	// MeshInstance interface override
	virtual Mesh* GetEntity(Scene *context) const;

private:
	std::auto_ptr<SortedMeshImpl> m_mesh;
};

///////////////////////////////////////////////////////////////////////////////

class SortedMeshInstanceViewImpl : public ISortedMeshInstanceView
{
public:
	SortedMeshInstanceViewImpl(MeshInstance *src, TriSortPluginFactory *factory, Scene *context);
	virtual ~SortedMeshInstanceViewImpl();

	void OnUpdateViewMatrix();

	// ISortedMeshInstanceView interface
	virtual MeshInstance* GetMeshInstance();

	// IView interface implementation
	virtual void OnDetach();

private:
	SortedMeshInstanceViewImpl(const SortedMeshInstanceViewImpl&); // no copy
	SortedMeshInstanceViewImpl& operator= (const SortedMeshInstanceViewImpl&);

	TriSortPluginFactory *m_factory;
	std::auto_ptr<SortedMeshInstanceImpl> m_instance;
};

///////////////////////////////////////////////////////////////////////////////

class TriSortPluginFactory : public ITriSortViewFactory
{
public:
	TriSortPluginFactory();

	const float* GetView() const { return m_mView; } // float4x4
	void OnDestroyView(SortedMeshInstanceViewImpl *view); // called by view destructor

	// ITriSortViewFactory interface implementation
	virtual void SetView(const float *mView); // float4x4

	// IViewFactory interface implementation
	virtual IView* CreateView(Scene *context, Base *b, size_t thisViewId);
	virtual void OnDetach();

private:
	typedef std::set<SortedMeshInstanceViewImpl *> ViewSet;
	ViewSet m_views;
	float m_mView[16];
};

///////////////////////////////////////////////////////////////////////////////

class SortedSceneImpl : public SceneAdapter
{
public:
	SortedSceneImpl(Scene *source, size_t sortPluginId);

	void SetView(const float *mView); // float4x4

	// Scene interface override
	virtual Base* ResolveContext(Base *target);

private:
	ITriSortViewFactory *m_factory;
};

///////////////////////////////////////////////////////////////////////////////

class SceneSorterImpl : public RefCountedImpl<ISceneSorter>
{
public:
	SceneSorterImpl(Scene *source, size_t sortPluginId);
	virtual ~SceneSorterImpl();

	// ISceneSorter interface implementation
	virtual Scene* GetSortedScene();
	virtual void SetView(const float *mView); // float4x4

public:
	AutoPtr<SortedSceneImpl> m_sortedScene;
	AutoPtr<Scene> m_source;
	unsigned int m_refCount;
};

// end of file
