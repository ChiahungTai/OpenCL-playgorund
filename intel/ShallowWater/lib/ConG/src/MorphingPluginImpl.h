/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

// MorphingPluginImpl.h

#pragma once

#include "MorphingPlugin.h"

#include "SceneGraph.h"
#include "SGMeshAdapter.h"
#include "SGSceneAdapter.h"
#include "SGGeometryInstanceAdapter.h"
#include "SGMorphedMesh.h"
#include "ProcessorBaseImpl.h"

using namespace collada;

///////////////////////////////////////////////////////////////////////////////

class MorphMeshImpl;

class MorphedBatchImpl : public BatchAdapter
{
public:
	MorphedBatchImpl(Batch *srcBatch, MorphMeshImpl *mesh, Scene *context);

	// Batch interface override
	virtual Mesh* GetMesh(Scene *context) const;
private:
	MorphMeshImpl *m_mesh;
};

class MorphMeshImpl : public MeshAdapter
{
public:
	MorphMeshImpl(IMorphingAlgorithm *morphAlgorithm, MorphController *morphCtrl, AccessProviderLocal *providerLocal, Scene *context);

	// Mesh interface override
	virtual MemoryObject* GetVertexBuffer(size_t bufferIdx) const;
	virtual size_t GetBatchCount() const;
	virtual Batch* GetBatch(size_t batchIdx, Scene *context) const;

private:
	PtrOwnerArray<MorphedBatchImpl> m_batches;
	std::vector< AutoPtr< MemoryObject > > m_AllBuffers;
};

class MorphedMeshInstanceImpl : public GeometryInstanceAdapter<MeshInstance, MorphControllerInstance, Mesh>
{
public:
	MorphedMeshInstanceImpl(IMorphingAlgorithm *morphAlgorithm, MorphControllerInstance *src, AccessProviderLocal *providerLocal, Scene *context);

	// MeshInstance interface override
	virtual Mesh* GetEntity(Scene *context) const;

private:
	std::auto_ptr<MorphMeshImpl> m_mesh;
};
///////////////////////////////////////////////////////////////////////////////

class MorphedMeshViewImpl : public IMorphedMeshView
{
public:
	MorphedMeshViewImpl(IMorphingAlgorithm *morphAlgorithm, MorphController *morphCtrl, AccessProviderLocal *providerLocal, Scene *context);

	// IMorphMeshView interface implementation
	virtual Mesh* GetMorphedMesh();

	// IView interface implementation
	virtual void OnDetach();

private:
	MorphController *m_source;
	std::auto_ptr<MorphMeshImpl> m_mesh;
};

///////////////////////////////////////////////////////////////////////////////

class MorphMeshInstanceViewImpl : public IMorphedMeshInstanceView
{
public:
	MorphMeshInstanceViewImpl(IMorphingAlgorithm *morphAlgorithm, MorphControllerInstance *morphCtrlInst, AccessProviderLocal *providerLocal, Scene *context);

	//  interface implementation
	virtual MeshInstance* GetMeshInstance();

	// IView interface implementation
	virtual void OnDetach();

private:
	std::auto_ptr<MorphedMeshInstanceImpl> m_inst;
};

///////////////////////////////////////////////////////////////////////////////

class MorphPluginFactory : public collada::IViewFactory
{
public:
	MorphPluginFactory(IMorphingAlgorithm *morphAlgorithm, AccessProviderLocal *providerLocal);

	// IViewFactory interface implementation
	virtual IView* CreateView(Scene *context, Base *b, size_t thisViewId);
	virtual void OnDetach();

private:
	AutoPtr<IMorphingAlgorithm> m_morphAlgorithm;
	AutoPtr<AccessProviderLocal> m_provider;
};

///////////////////////////////////////////////////////////////////////////////

class MorphSceneAdapter : public SceneAdapter
{
public:
	MorphSceneAdapter(IMorphingAlgorithm *morphAlgorithm, AccessProviderLocal *providerLocal, Scene *src, size_t triPluginId);

	// Scene interface override
	virtual MeshInstance* GetMeshInstance(size_t idx);
	virtual size_t GetMeshInstanceCount() const;
	virtual Mesh* GetMesh(size_t idx);
	virtual size_t GetMeshCount() const;
	virtual MorphController* GetMorphCtrl(size_t idx);
	virtual size_t GetMorphCtrlCount() const;
	virtual MorphControllerInstance* GetMorphCtrlInstance(size_t idx);
	virtual size_t GetMorphCtrlInstanceCount() const;
private:
	size_t m_morphCtrlInstCount; // for GetMeshInstance
	size_t m_morphCtrlCount; // for GetMesh
};

