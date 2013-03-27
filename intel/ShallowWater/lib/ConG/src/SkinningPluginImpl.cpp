/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SkinningPluginImpl.cpp

#include "stdafx.h"

#include "mathstub.h"

#include "SkinningPluginImpl.h"
#include "AccessProviderDefault.h"
#include "SGSkinnedMesh.h" // FIXME: we should not depend on database implementation

///////////////////////////////////////////////////////////////////////////////

SkinnedBatchImpl::SkinnedBatchImpl(Batch *srcBatch, SkinnedMeshImpl *mesh, Scene *context)
  : BatchAdapter(srcBatch, context)
  , m_mesh(mesh)
  , m_idxStride(srcBatch->GetIndexStride())
  , m_extraLayoutOverride(srcBatch->GetExtraVertexElementDescs(),
                          srcBatch->GetExtraVertexElementDescs() + srcBatch->GetExtraVertexElementCount())
{
}

void SkinnedBatchImpl::SetIndexBufferOverride(MemoryObject *buf)
{
	m_idxBufOverride = buf;
}

VertexElementDescEx* SkinnedBatchImpl::GetExtraVertexElementDescsOverride()
{
	return &m_extraLayoutOverride[0];
}

const VertexElementDescEx* SkinnedBatchImpl::GetExtraVertexElementDescs() const
{
	return &m_extraLayoutOverride[0];
}

Mesh* SkinnedBatchImpl::GetMesh(Scene *context) const
{
	return SafeResolveContext<Mesh>(context, m_mesh);
}

MemoryObject* SkinnedBatchImpl::GetIndexData() const
{
	return m_idxBufOverride ? m_idxBufOverride : GetTarget()->GetIndexData();
}

///////////////////////////////////////////////////////////////////////////////


SkinnedMeshImpl::JointsProcessor::JointsProcessor(ISkinningAlgorithm *algorithm, MemoryObject *outJointMatrices, MemoryObject *inJoints)
  : ProcessorBase(algorithm)
{
	AlgorithmResource outputs[] =
	{
		{ ISkinningAlgorithm::OUT_JOINT_MATRICES, outJointMatrices },
		{ -1, NULL } // terminator
	};
	AlgorithmResource inputs[] =
	{
		{ ISkinningAlgorithm::IN_JOINTS, inJoints },
		{ -1, NULL } // terminator
	};
	SetData(inputs, outputs);


	MapHelper<const Joint, AccessorReadDefault> joints(FindAccessor<AccessorReadDefault>(GetAccessors(), ISkinningAlgorithm::IN_JOINTS));
	std::set<Node *> nodes; // to filter repeating nodes
	for( size_t i = 0; i < joints.count; ++i )
	{
		assert(joints[i].node);
		if( nodes.insert(joints[i].node).second )
		{
			joints[i].node->AddListener(this);
		}
	}
}

SkinnedMeshImpl::JointsProcessor::~JointsProcessor()
{
	MapHelper<const Joint, AccessorReadDefault> joints(FindAccessor<AccessorReadDefault>(GetAccessors(), ISkinningAlgorithm::IN_JOINTS));
	std::set<Node *> nodes; // to filter repeating nodes
	for( size_t i = 0; i < joints.count; ++i )
	{
		assert(joints[i].node);
		if( nodes.insert(joints[i].node).second )
		{
			joints[i].node->RemoveListener(this);
		}
	}
}

bool SkinnedMeshImpl::JointsProcessor::DoProcess()
{
	if( GetDirty() )
	{
		GetAlgorithm()->ComputeJoints(GetAccessors());
		ClearDirtyFlag();
		return true;
	}
	return false;
}

void SkinnedMeshImpl::JointsProcessor::OnNodeUpdate(Node *node)
{
	SetDirty();
}

///////////////////////////////////////////////////////////////////////////////

SkinnedMeshImpl::SkinningMatricesProcessor::SkinningMatricesProcessor(ISkinningAlgorithm *algorithm, MemoryObject *outMatrices, MemoryObject *inJointMatrices,
                                                                      MemoryObject *wb, MemoryObject *wj, const float* bindShape4x4)
  : ProcessorBase(algorithm)
  , m_bindShape(bindShape4x4)
{
	AlgorithmResource inputs[] =
	{
		{ ISkinningAlgorithm::IN_JOINT_MATRICES, inJointMatrices },
		{ ISkinningAlgorithm::IN_WEIGHTS_BINDINGS, wb },
		{ ISkinningAlgorithm::IN_WEIGHTED_JOINTS, wj },
		{ -1, NULL } // terminator
	};
	AlgorithmResource outputs[] =
	{
		{ ISkinningAlgorithm::OUT_TRANSFORMS, outMatrices },
		{ -1, NULL } // terminator
	};
	SetData(inputs, outputs);
}

bool SkinnedMeshImpl::SkinningMatricesProcessor::DoProcess()
{
	if( GetDirty() )
	{
		GetAlgorithm()->ComputeSkinningMatrices(GetAccessors(), m_bindShape);
		ClearDirtyFlag();
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////

SkinnedMeshImpl::PositionsProcessor::PositionsProcessor(ISkinningAlgorithm *algorithm, MemoryObject *outPos, MemoryObject *inPos, MemoryObject *inMatrices)
  : ProcessorBase(algorithm)
{
	AlgorithmResource inputs[] =
	{
		{ ISkinningAlgorithm::IN_POSITIONS, inPos },
		{ ISkinningAlgorithm::IN_TRANSFORMS, inMatrices },
		{ -1, NULL } // terminator
	};
	AlgorithmResource outputs[] =
	{
		{ ISkinningAlgorithm::OUT_POSITIONS, outPos },
		{ -1, NULL } // terminator
	};
	SetData(inputs, outputs);
}

bool SkinnedMeshImpl::PositionsProcessor::DoProcess()
{
	if( GetDirty() )
	{
		GetAlgorithm()->TransformPositions(GetAccessors());
		ClearDirtyFlag();
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////

SkinnedMeshImpl::NormalsProcessor::NormalsProcessor(ISkinningAlgorithm *algorithm, MemoryObject *outNorm, 
                                                    MemoryObject *inNormals, MemoryObject *inMatricesNorm,
                                                    MemoryObject *inReindexNormals, MemoryObject *inReindexTransforms)
  : ProcessorBase(algorithm)
{
	AlgorithmResource inputs[] = 
	{
		{ ISkinningAlgorithm::IN_NORMALS,    inNormals      },
		{ ISkinningAlgorithm::IN_TRANSFORMS, inMatricesNorm },
		{ ISkinningAlgorithm::IN_REINDEX_NORMALS,    inReindexNormals },
		{ ISkinningAlgorithm::IN_REINDEX_TRANSFORMS, inReindexTransforms },
		{ -1, NULL } // terminator
	};
	AlgorithmResource outputs[] =
	{
		{ ISkinningAlgorithm::OUT_NORMALS, outNorm },
		{ -1, NULL } // terminator
	};
	SetData(inputs, outputs);
}

bool SkinnedMeshImpl::NormalsProcessor::DoProcess()
{
	if( GetDirty() )
	{
		GetAlgorithm()->TransformNormals(GetAccessors());
		ClearDirtyFlag();
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////

SkinnedMeshImpl::SkinnedMeshImpl(ISkinningAlgorithm *algorithm, AccessProviderLocal *providerLocal, 
                                 Scene *context, const SkinController *skinCtrl, MemoryObject *joints)
  : MeshAdapter(skinCtrl->GetBaseMesh(context->GetPrevContext()), context)
  , m_primaryLayoutOverride(GetSource()->GetPrimaryVertexElementDescs(),
                            GetSource()->GetPrimaryVertexElementDescs() + GetSource()->GetPrimaryVertexElementCount())
  , m_vtxBufOverride(GetSource()->GetVertexBufferCount())
  , m_vtxBufStrideOverride(GetSource()->GetVertexBufferCount())
{
#ifndef NDEBUG
	// to check that all source vertex buffers are unique
	std::set<MemoryObject*> checkUnique;
#endif

	for( size_t i = 0; i < GetSource()->GetVertexBufferCount(); ++i )
	{
		assert(checkUnique.insert(GetSource()->GetVertexBuffer(i)).second);
		m_vtxBufStrideOverride[i] = GetSource()->GetRawVertexSize(i);
	}

	// search POSITION in the primary layout
	for( size_t i = 0; i < m_primaryLayoutOverride.size(); ++i )
	{
		if( 0 == strcmp(m_primaryLayoutOverride[i].semanticName, "POSITION") )
		{
			assert(!m_skinMatrices);
			assert(ST_FLOAT == m_primaryLayoutOverride[i].componentType && 3 == m_primaryLayoutOverride[i].componentCount);
			assert(GetSource()->GetRawVertexSize(m_primaryLayoutOverride[i].inputSlot) == sizeof(float3));

			GetMemoryManager()->Allocate(joints->GetBytesSize() / sizeof(Joint) * sizeof(float4x4), NULL, &m_jointMatrices);
			m_jointsProcessor = new JointsProcessor(algorithm, m_jointMatrices, joints);

			AutoPtr<MemoryObject> bindings;
			GetMemoryManager()->Allocate(skinCtrl->GetBindingCount() * sizeof(VertexWeightsBinding),
				skinCtrl->GetBindings(), &bindings);

			AutoPtr<MemoryObject> weightedJoints;
			GetMemoryManager()->Allocate(skinCtrl->GetWeightedJointCount() * sizeof(WeightedJoint),
				skinCtrl->GetWeightedJoints(), &weightedJoints);


			MemoryObject *srcPositions = GetSource()->GetVertexBuffer(m_primaryLayoutOverride[i].inputSlot);

			// skinning matrices buffer
			const size_t vcount = srcPositions->GetBytesSize() / sizeof(float3);
			GetMemoryManager()->Allocate(vcount * sizeof(float4x4), NULL, &m_skinMatrices);
			AutoPtr<RefCounted>(new SkinningMatricesProcessor(algorithm, m_skinMatrices, m_jointMatrices, bindings, weightedJoints, skinCtrl->GetBindShape()));

			// positions buffer
			GetMemoryManager()->Allocate(srcPositions->GetBytesSize(), NULL, &m_vtxBufOverride[m_primaryLayoutOverride[i].inputSlot]);
			AutoPtr<RefCounted>(new PositionsProcessor(algorithm, m_vtxBufOverride[m_primaryLayoutOverride[i].inputSlot], srcPositions, m_skinMatrices));
		}
	}
	assert(m_skinMatrices);

	// search NORMAL in the primary layout
	for( size_t i = 0; i < GetSource()->GetPrimaryVertexElementCount(); ++i )
	{
		// search NORMAL in the primary layout
		if( 0 == strcmp(m_primaryLayoutOverride[i].semanticName, "NORMAL") )
		{
			HandleNormals(algorithm, &m_primaryLayoutOverride[i]);
		}
	}

	// override batches and search normals in original batches' extra attributes
	m_batches.reserve(GetSource()->GetBatchCount());
	for( size_t batchIdx = 0; batchIdx < GetSource()->GetBatchCount(); ++batchIdx )
	{
		Batch *batchOriginal = GetSource()->GetBatch(batchIdx, context->GetPrevContext());
		m_batches.push_back(new SkinnedBatchImpl(batchOriginal, this, context));
		SkinnedBatchImpl *batchOverride = m_batches.back();

		// search NORMAL in the extra vertex layout
		VertexElementDescEx *descs = batchOverride->GetExtraVertexElementDescsOverride();
		for( size_t i = 0; i < batchOverride->GetExtraVertexElementCount(); ++i )
		{
			if( 0 == strcmp(descs[i].semanticName, "NORMAL") )
			{
				HandleNormalsIndirect(algorithm, providerLocal, descs + i, m_batches[batchIdx]);
			}
		}
	}
}

void SkinnedMeshImpl::HandleNormalsIndirect(ISkinningAlgorithm *algorithm, AccessProviderLocal *providerLocal, 
                                            VertexElementDescEx *desc, SkinnedBatchImpl *batch)
{
	assert(batch && desc);
	assert(-1 != desc->indexOffset);
	assert(ST_FLOAT == desc->componentType && 3 == desc->componentCount);
	assert(GetSource()->GetRawVertexSize(desc->inputSlot) == sizeof(float3));


	//
	// calculate reindex tables
	//

	typedef std::pair<unsigned long, unsigned long> NormPos;
	typedef std::map<NormPos, unsigned long> IdxMap;

	std::vector<unsigned long> reindexN;
	std::vector<unsigned long> reindexT;
	IdxMap idxMap;

	const Batch *oldBatch = batch->GetTarget();
	AutoPtr<AccessorReadDefault> accessor;
	providerLocal->CreateAccessorReadDefault(oldBatch->GetIndexData(), &accessor);
	MapHelper<const unsigned long, AccessorReadDefault> idxDataOld(accessor);
	unsigned int idxStride = oldBatch->GetIndexStride();
	size_t idxCount = oldBatch->GetIndexData()->GetBytesSize() / (sizeof(unsigned long) * idxStride);
	unsigned int posOffset = oldBatch->GetPrimaryOffset();

	// copy all indices from the source
	std::vector<unsigned long> idxDataNew(&idxDataOld[0], &idxDataOld[0] + oldBatch->GetIndexData()->GetBytesSize() / sizeof(unsigned long));
	for( size_t i = 0; i < idxCount; ++i )
	{
		unsigned long norm = idxDataOld[i * idxStride + desc->indexOffset];
		unsigned long pos = idxDataOld[i * idxStride + posOffset];
		unsigned long normIndexNew = static_cast<unsigned long>(idxMap.size());
		IdxMap::_Pairib tmp = idxMap.insert(IdxMap::value_type(NormPos(norm, pos), normIndexNew));
		if( tmp.second )
		{
			reindexN.push_back(norm);
			reindexT.push_back(pos);
		}
		// replace only normal index
		idxDataNew[i * idxStride + desc->indexOffset] = tmp.first->second;
	}
	assert(reindexN.size() == reindexT.size() && reindexN.size() == idxMap.size());

	AutoPtr<MemoryObject> idxBufferOverride;
	GetMemoryManager()->Allocate(oldBatch->GetIndexData()->GetBytesSize(), &idxDataNew[0], &idxBufferOverride);
	batch->SetIndexBufferOverride(idxBufferOverride);


	AutoPtr<MemoryObject> reindexBufferN;
	AutoPtr<MemoryObject> reindexBufferT;
	GetMemoryManager()->Allocate(idxMap.size() * sizeof(unsigned long), &reindexN[0], &reindexBufferN);
	GetMemoryManager()->Allocate(idxMap.size() * sizeof(unsigned long), &reindexT[0], &reindexBufferT);

	// override vertex buffer pointer
	MemoryObject *srcBuffer = GetSource()->GetVertexBuffer(desc->inputSlot);
	if( m_vtxBufOverride[desc->inputSlot] )
	{
		// allocate in a new slot if the existing one is busy
		m_vtxBufStrideOverride.push_back(m_vtxBufStrideOverride[desc->inputSlot]);
		desc->inputSlot = static_cast<unsigned int>(m_vtxBufOverride.size());
		m_vtxBufOverride.push_back(NULL);
	}
	GetMemoryManager()->Allocate(sizeof(float3) * idxMap.size(), NULL, &m_vtxBufOverride[desc->inputSlot]);
	AutoPtr<RefCounted>(new NormalsProcessor(algorithm, m_vtxBufOverride[desc->inputSlot],
		srcBuffer, m_skinMatrices, reindexBufferN, reindexBufferT));
}

void SkinnedMeshImpl::HandleNormals(ISkinningAlgorithm *algorithm, VertexElementDesc *desc)
{
	assert(ST_FLOAT == desc->componentType && 3 == desc->componentCount);
	assert(GetSource()->GetRawVertexSize(desc->inputSlot) == sizeof(float3));
	// override buffer pointer
	MemoryObject *srcBuffer = GetSource()->GetVertexBuffer(desc->inputSlot);
	if( m_vtxBufOverride[desc->inputSlot] )
	{
		// allocate in a new slot if the existing one is busy
		m_vtxBufStrideOverride.push_back(m_vtxBufStrideOverride[desc->inputSlot]);
		desc->inputSlot = static_cast<unsigned int>(m_vtxBufOverride.size());
		m_vtxBufOverride.push_back(NULL);
	}
	GetMemoryManager()->Allocate(srcBuffer->GetBytesSize(), NULL, &m_vtxBufOverride[desc->inputSlot]);
	AutoPtr<RefCounted>(new NormalsProcessor(algorithm, m_vtxBufOverride[desc->inputSlot], srcBuffer, m_skinMatrices, NULL, NULL));
}

size_t SkinnedMeshImpl::GetVertexBufferCount() const
{
	return m_vtxBufOverride.size();
}

MemoryObject* SkinnedMeshImpl::GetVertexBuffer(size_t bufferIdx) const
{
	assert(m_vtxBufOverride[bufferIdx] || bufferIdx < GetSource()->GetVertexBufferCount());
	return m_vtxBufOverride[bufferIdx] ? m_vtxBufOverride[bufferIdx] : GetSource()->GetVertexBuffer(bufferIdx);
}

unsigned int SkinnedMeshImpl::GetRawVertexSize(size_t bufferIdx) const
{
	return m_vtxBufStrideOverride[bufferIdx];
}

const VertexElementDesc* SkinnedMeshImpl::GetPrimaryVertexElementDescs() const
{
	return &m_primaryLayoutOverride[0];
}

size_t SkinnedMeshImpl::GetBatchCount() const
{
	return m_batches.size();
}

Batch* SkinnedMeshImpl::GetBatch(size_t batchIdx, Scene *context) const
{
	return SafeResolveContext<Batch>(context, m_batches[batchIdx]);
}

///////////////////////////////////////////////////////////////////////////////

SkinnedMeshInstanceImpl::SkinnedMeshInstanceImpl(ISkinningAlgorithm *algorithm, AccessProviderLocal *providerLocal,
                                                 SkinControllerInstance *src, Scene *context)
  : GeometryInstanceAdapter(src, context)
  , m_mesh(new SkinnedMeshImpl(algorithm, providerLocal, context, src->GetEntity(context), src->GetJoints()))
{
}


//
// MeshInstance interface override
//

Mesh* SkinnedMeshInstanceImpl::GetEntity(Scene *context) const
{
	return SafeResolveContext<Mesh>(context, m_mesh.get());
}

///////////////////////////////////////////////////////////////////////////////

// constructor
SkinControllerInstanceViewImpl::SkinControllerInstanceViewImpl(ISkinningAlgorithm *algorithm, AccessProviderLocal *providerLocal,
                                                               SkinControllerInstance *src, Scene *context)
  : m_inst(new SkinnedMeshInstanceImpl(algorithm, providerLocal, src, context))
{
}


//
// ISkinnedMeshInstanceView interface implementation
//

MeshInstance* SkinControllerInstanceViewImpl::GetMeshInstance()
{
	return m_inst.get();
}

MemoryObject* SkinControllerInstanceViewImpl::GetSkinningMatrices()
{
	SkinnedMeshImpl *m = static_cast<SkinnedMeshImpl*>(m_inst->GetEntity(NULL));
	return m->GetSkinningMatrices();
}


//
// IView interface implementation
//

void SkinControllerInstanceViewImpl::OnDetach()
{
	delete this;
}

///////////////////////////////////////////////////////////////////////////////

// constructor
SkinningPluginFactory::SkinningPluginFactory(ISkinningAlgorithm *algorithm, AccessProviderLocal *providerLocal)
  : m_algorithm(algorithm)
  , m_provider(providerLocal)
{
}

//
// IViewFactory interface implementation
//

IView* SkinningPluginFactory::CreateView(Scene *context, Base *b, size_t thisViewId)
{
	if( SkinControllerInstance *inst = dynamic_cast<SkinControllerInstance *>(b) )
	{
		return new SkinControllerInstanceViewImpl(m_algorithm, m_provider, inst, context);
	}
	return NULL;
}

void SkinningPluginFactory::OnDetach()
{
	delete this;
}

// factory
namespace collada
{
	IViewFactory* CreateDefaultSkinningPlugin(ISkinningAlgorithm *algorithm, AccessProviderLocal *providerLocal)
	{
		return new SkinningPluginFactory(algorithm, providerLocal);
	}
}

///////////////////////////////////////////////////////////////////////////////

SkinningSceneAdapter::SkinningSceneAdapter(ISkinningAlgorithm *algorithm, AccessProviderLocal *providerLocal, Scene *src, size_t skinningPluginId)
  : SceneAdapter(src, skinningPluginId, &CViewFactoryCreatorProc2<ISkinningAlgorithm*, AccessProviderLocal*>(CreateDefaultSkinningPlugin, algorithm, providerLocal))
  , m_skinCount(src->GetSkinCtrlInstanceCount())
{
}

SkinningSceneAdapter::~SkinningSceneAdapter()
{
}

MeshInstance* SkinningSceneAdapter::GetMeshInstance(size_t idx)
{
	if( idx < m_skinCount )
	{
		return GetSource()->GetSkinCtrlInstance(idx)
			->GetViewT<ISkinnedMeshInstanceView>(GetPluginId(), this)->GetMeshInstance();
	}
	return GetSource()->GetMeshInstance(idx - m_skinCount);
}

size_t SkinningSceneAdapter::GetMeshInstanceCount() const
{
	return GetSource()->GetMeshInstanceCount() + m_skinCount;
}

Mesh* SkinningSceneAdapter::GetMesh(size_t idx)
{
	if( idx < m_skinCount )
	{
		return GetSource()->GetSkinCtrlInstance(idx)
			->GetViewT<ISkinnedMeshInstanceView>(GetPluginId(), this)->GetMeshInstance()->GetEntity(this);
	}
	return GetSource()->GetMesh(idx - m_skinCount);
}

size_t SkinningSceneAdapter::GetMeshCount() const
{
	return GetSource()->GetMeshCount() + m_skinCount;
}

SkinController* SkinningSceneAdapter::GetSkinCtrl(size_t idx)
{
	assert(false);
	return NULL;
}

size_t SkinningSceneAdapter::GetSkinCtrlCount() const
{
	return 0;
}

SkinControllerInstance* SkinningSceneAdapter::GetSkinCtrlInstance(size_t idx)
{
	assert(false);
	return NULL;
}

size_t SkinningSceneAdapter::GetSkinCtrlInstanceCount() const
{
	return 0;
}


///////////////////////////////////////////////////////////////////////////////

namespace collada
{
	void CreateSkinningSceneAdapter(Scene *source, Scene **ppResult, ISkinningAlgorithm *algorithm, AccessProviderLocal *providerLocal, size_t skinningPluginId)
	{
		AutoPtr<Scene> tmp(new SkinningSceneAdapter(algorithm, providerLocal, source, skinningPluginId));
		DetachRawPtr(tmp, ppResult);
	}
}
// end of file
