/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SkinningPluginImpl.h

#pragma once

#include "mathstub.h"

#include "SkinningPlugin.h"
#include "SkinningAlgorithm.h"
#include "ProcessorBaseImpl.h"

#include "SceneGraph.h"
#include "SGGeometryInstanceAdapter.h"
#include "SGMeshAdapter.h"
#include "SGSceneAdapter.h"
#include "SGImplBase.h"

using namespace collada;

class SkinnedMeshImpl;

// skinned mesh subset
class SkinnedBatchImpl : public BatchAdapter
{
public:
	SkinnedBatchImpl(Batch *srcBatch, SkinnedMeshImpl *mesh, Scene *context);
	void SetIndexBufferOverride(MemoryObject *buf);

	VertexElementDescEx* GetExtraVertexElementDescsOverride();

	// Batch interface override
	virtual const VertexElementDescEx* GetExtraVertexElementDescs() const;
	virtual Mesh* GetMesh(Scene *context) const;
	virtual MemoryObject* GetIndexData() const;

private:
	SkinnedMeshImpl *m_mesh;
	std::vector<VertexElementDescEx> m_extraLayoutOverride;
	AutoPtr<MemoryObject> m_idxBufOverride;
	unsigned int m_idxStride;
};

///////////////////////////////////////////////////////////////////////////////

class SkinnedMeshImpl : public MeshAdapter
{
public:
	SkinnedMeshImpl(ISkinningAlgorithm *algorithm, AccessProviderLocal *providerLocal,
		Scene *context, const SkinController *skinCtrl, MemoryObject *joints);

	MemoryObject* GetSkinningMatrices() const { return m_skinMatrices; }


	//
	// Mesh interface
	//

	virtual size_t GetVertexBufferCount() const;
	virtual MemoryObject* GetVertexBuffer(size_t bufferIdx) const;
	virtual unsigned int GetRawVertexSize(size_t bufferIdx) const;
	virtual const VertexElementDesc* GetPrimaryVertexElementDescs() const;
	virtual size_t GetBatchCount() const;
	virtual Batch* GetBatch(size_t batchIdx, Scene *context) const;
//	virtual const float* GetCenter() const; // float3
//	virtual const float* GetAABB() const; // float3

private:
	PtrOwnerArray<SkinnedBatchImpl> m_batches;
	std::vector<VertexElementDesc> m_primaryLayoutOverride;
	std::vector<AutoPtr<MemoryObject> > m_vtxBufOverride;
	std::vector<unsigned int> m_vtxBufStrideOverride;

	AutoPtr<MemoryObject> m_jointMatrices; ///< buffer of float4x4
	AutoPtr<MemoryObject> m_skinMatrices; ///< buffer of float4x4

	void HandleNormalsIndirect(ISkinningAlgorithm *algorithm, AccessProviderLocal *providerLocal, VertexElementDescEx *desc, SkinnedBatchImpl *batch);
	void HandleNormals(ISkinningAlgorithm *algorithm, VertexElementDesc *desc);


	class JointsProcessor
		: public ProcessorBase
		, private INodeListener
	{
	public:
		JointsProcessor(ISkinningAlgorithm *algorithm, MemoryObject *outJointMatrices, MemoryObject *inJoints);
		virtual ~JointsProcessor();
		ISkinningAlgorithm* GetAlgorithm() { return static_cast<ISkinningAlgorithm*>(ProcessorBase::GetAlgorithm()); }

		// Processor
		virtual bool DoProcess();

	private:
		// INodeListener
		virtual void OnNodeUpdate(Node *node);
	};

	class SkinningMatricesProcessor : public ProcessorBase
	{
	public:
		SkinningMatricesProcessor(ISkinningAlgorithm *algorithm, MemoryObject *outMatrices,
			MemoryObject *inJointMatrices, MemoryObject *wb, MemoryObject *wj, const float* bindShape4x4);
		ISkinningAlgorithm* GetAlgorithm() { return static_cast<ISkinningAlgorithm*>(ProcessorBase::GetAlgorithm()); }

		// Processor
		virtual bool DoProcess();

	private:
		float4x4 m_bindShape;
	};

	class PositionsProcessor : public ProcessorBase
	{
	public:
		PositionsProcessor(ISkinningAlgorithm *algorithm, MemoryObject *outPos, MemoryObject *inPos, MemoryObject *inMatrices);
		ISkinningAlgorithm* GetAlgorithm() { return static_cast<ISkinningAlgorithm*>(ProcessorBase::GetAlgorithm()); }

		// Processor
		virtual bool DoProcess();
	};

	class NormalsProcessor : public ProcessorBase
	{
	public:
		NormalsProcessor(ISkinningAlgorithm *algorithm, MemoryObject *outNorm,
			MemoryObject *inNorm, MemoryObject *inMatricesNorm,
			MemoryObject *inReindexNormals, MemoryObject *inReindexTransforms);
		ISkinningAlgorithm* GetAlgorithm() { return static_cast<ISkinningAlgorithm*>(ProcessorBase::GetAlgorithm()); }

		// Processor
		virtual bool DoProcess();
	};

	AutoPtr<JointsProcessor> m_jointsProcessor;
};

///////////////////////////////////////////////////////////////////////////////

class SkinnedMeshInstanceImpl : public GeometryInstanceAdapter<MeshInstance, SkinControllerInstance, Mesh>
{
public:
	SkinnedMeshInstanceImpl(ISkinningAlgorithm *algorithm, AccessProviderLocal *providerLocal, SkinControllerInstance *src, Scene *context);

	// MeshInstance interface override
	virtual Mesh* GetEntity(Scene *context) const;

private:
	std::auto_ptr<SkinnedMeshImpl> m_mesh;
};

///////////////////////////////////////////////////////////////////////////////

class SkinControllerInstanceViewImpl : public ISkinnedMeshInstanceView
{
public:
	SkinControllerInstanceViewImpl(ISkinningAlgorithm *algorithm, AccessProviderLocal *providerLocal, SkinControllerInstance *src, Scene *context);

	// ISkinnedMeshInstanceView interface implementation
	virtual MeshInstance* GetMeshInstance();
	virtual MemoryObject* GetSkinningMatrices();

	// IView interface implementation
	virtual void OnDetach();

private:
	std::auto_ptr<SkinnedMeshInstanceImpl> m_inst;
};

///////////////////////////////////////////////////////////////////////////////

class SkinningPluginFactory : public collada::IViewFactory
{
public:
	SkinningPluginFactory(ISkinningAlgorithm *algorithm, AccessProviderLocal *providerLocal);

	// IViewFactory interface implementation
	virtual IView* CreateView(Scene *context, Base *b, size_t thisViewId);
	virtual void OnDetach();

private:
	AutoPtr<ISkinningAlgorithm> m_algorithm;
	AutoPtr<AccessProviderLocal> m_provider;
};

///////////////////////////////////////////////////////////////////////////////

class SkinningSceneAdapter : public SceneAdapter
{
public:
	SkinningSceneAdapter(ISkinningAlgorithm *algorithm, AccessProviderLocal *providerLocal, Scene *src, size_t skinningPluginId);
	virtual ~SkinningSceneAdapter();


	//
	// Scene interface override
	//

	virtual MeshInstance* GetMeshInstance(size_t idx);
	virtual size_t GetMeshInstanceCount() const;
	virtual Mesh* GetMesh(size_t idx);
	virtual size_t GetMeshCount() const;
	virtual SkinController* GetSkinCtrl(size_t idx);
	virtual size_t GetSkinCtrlCount() const;
	virtual SkinControllerInstance* GetSkinCtrlInstance(size_t idx);
	virtual size_t GetSkinCtrlInstanceCount() const;

private:
	size_t m_skinCount;
};

// end of file
