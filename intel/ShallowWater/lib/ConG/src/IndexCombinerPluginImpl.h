/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file IndexCombinerPluginImpl.h

#pragma once

#include "IndexCombinerPlugin.h"
#include "IndexCombinerAlgorithm.h"

#include "SceneGraph.h"
#include "SGMeshAdapter.h"
#include "SGSceneAdapter.h"

#include "ProcessorBaseImpl.h"

using namespace collada;

///////////////////////////////////////////////////////////////////////////////

class UnifiedVertexBufferProcessor : public ProcessorBase
{
public:
	UnifiedVertexBufferProcessor(IIndexCombinerAlgorithm *algorithm, MemoryObject *const*inIndexBuffers, 
		unsigned long *vertexOffsets, size_t taskCount, unsigned int vertexStride, MemoryObject* inVertexBuffer, MemoryObject* outVertexBuffer);
	IIndexCombinerAlgorithm* GetAlgorithm() { return static_cast<IIndexCombinerAlgorithm*>(ProcessorBase::GetAlgorithm()); }

	// Processor
	virtual bool DoProcess();

private:
	std::vector<unsigned long> m_vertexOffsets;
	unsigned long m_vertexStride;
};

///////////////////////////////////////////////////////////////////////////////
// Batch wrapper
class UnifiedBatchImpl : public BatchAdapter
{
public:
	UnifiedBatchImpl(class UnifiedMeshImpl *mesh, Batch *srcBatch, Scene *context);
	void SetIndexBuffer(MemoryObject *buf);

	// Batch interface override
	virtual unsigned int GetIndexStride() const;
	virtual MemoryObject* GetIndexData() const;
	virtual const VertexElementDescEx* GetExtraVertexElementDescs() const;
	virtual size_t GetExtraVertexElementCount() const;
	virtual unsigned int GetPrimaryOffset() const;

private:
	UnifiedBatchImpl(const UnifiedBatchImpl&); // protect from copy
	UnifiedBatchImpl& operator = (const UnifiedBatchImpl&);

	AutoPtr<MemoryObject> m_idxbufOverride;
	size_t m_indexCount;
	std::vector<VertexElementDescEx> m_layoutOverride; // indexOffset replaced with 0
};

class UnifiedBatchViewImpl : public IUnifiedBatchView
{
public:
	UnifiedBatchViewImpl(const Batch *srcBatch, size_t thisViewId, Scene *context);

	// IUnifiedBatchView interface implementation
	virtual Batch* GetUnifiedBatch();

	// IView interface implementation
	virtual void OnDetach();

private:
	UnifiedBatchViewImpl(const UnifiedBatchViewImpl&); // protect from copy
	size_t m_batchIndex;
	size_t m_thisViewId;
	Mesh *m_srcMesh;
	Scene *m_context;
};

///////////////////////////////////////////////////////////////////////////////
// Mesh wrapper

class UnifiedMeshImpl : public MeshAdapter
{
public:
	UnifiedMeshImpl(IIndexCombinerAlgorithm *algorithm, AccessProviderLocal *providerLocal, Mesh *srcMesh, Scene *context);

	// Mesh interface override
	virtual size_t GetVertexBufferCount() const;
	virtual MemoryObject* GetVertexBuffer(size_t bufferIdx) const;
	virtual unsigned int GetRawVertexSize(size_t bufferIdx) const;
	virtual Batch* GetBatch(size_t batchIdx, Scene *context) const; // returns unified batch

private:
	UnifiedMeshImpl(const UnifiedMeshImpl&); // protect from copy

	PtrOwnerArray<UnifiedBatchImpl> m_batches;

	typedef std::vector<AutoPtr<MemoryObject>> MemoryObjectVector;
	std::vector<unsigned long> m_indexCounts;

	MemoryObjectVector m_vtxBuffersSrc;
	MemoryObjectVector m_vtxBuffersOverride;
	std::vector<unsigned int> m_vtxBuffersStrides;
};

class UnifiedMeshViewImpl : public IUnifiedMeshView
{
public:
	UnifiedMeshViewImpl(IIndexCombinerAlgorithm *algorithm, AccessProviderLocal *providerLocal, Mesh *srcMesh, size_t thisViewId, Scene *context);

	// IUnifiedMeshView interface implementation
	virtual Mesh* GetUnifiedMesh();

	// IView interface implementation
	virtual void OnDetach();

private:
	UnifiedMeshViewImpl(const UnifiedMeshViewImpl&); // protect from copy
	std::auto_ptr<UnifiedMeshImpl> m_mesh;
	Mesh *m_source;
};

///////////////////////////////////////////////////////////////////////////////

class IndexCombinerPluginFactory : public IViewFactory
{
public:
	IndexCombinerPluginFactory(IIndexCombinerAlgorithm *algorithm, AccessProviderLocal *providerLocal);

	// IViewFactory interface implementation
	virtual IView* CreateView(Scene *context, Base *b, size_t thisViewId);
	virtual void OnDetach();

private:
	AutoPtr<IIndexCombinerAlgorithm> m_algorithm;
	AutoPtr<AccessProviderLocal> m_provider;
};

///////////////////////////////////////////////////////////////////////////////

class UnifiedSceneAdapter : public SceneAdapter
{
public:
	UnifiedSceneAdapter(IIndexCombinerAlgorithm *algorithm, AccessProviderLocal *providerLocal, Scene *src, size_t pluginId);

	// Scene interface override
	virtual Base* ResolveContext(Base *target);
};


///////////////////////////////////////////////////////////////////////////////

// end of file
