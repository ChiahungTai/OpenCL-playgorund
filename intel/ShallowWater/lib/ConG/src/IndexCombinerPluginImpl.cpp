/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file IndexCombinerPluginImpl.cpp

#include "stdafx.h"

#include "SGMesh.h"
#include "IndexCombinerPluginImpl.h"

#include "AccessProviderDefault.h"
#include "IndexCombinerAlgorithm.h"

///////////////////////////////////////////////////////////////////////////////

typedef std::vector<std::map<unsigned int, unsigned int>> LocalIdxOffsetsToConvertedBufferIdxType;

namespace
{
	// compares old indices
	class IdxCompare : public std::binary_function<size_t, size_t, bool>
	{
	public:
		IdxCompare(const unsigned long *p, unsigned int stride)
			: m_p(p)
			, m_stride(stride)
		{
		}

		bool operator () (size_t left, size_t right) const
		{
			for( unsigned int i = 0; i < m_stride; ++i )
			{
				const unsigned long leftIdx = m_p[m_stride*left + i];
				const unsigned long rightIdx = m_p[m_stride*right + i];
				if( leftIdx < rightIdx )
					return true;   // lt
				if( leftIdx > rightIdx )
					return false;  // gt
			}
			return false;          // eq
		}

	private:
		const unsigned long * const m_p; // this disables default constructor and cmp operators
		unsigned int m_stride;
	};
} // end of anonymous namespace


static unsigned int GetIndexOffset(const Batch *srcBatch, size_t inputSlot, Scene *context)
{
	unsigned int result = -1;
	for( size_t i = 0; i < srcBatch->GetMesh(context)->GetPrimaryVertexElementCount(); ++i )
	{
		if( srcBatch->GetMesh(context)->GetPrimaryVertexElementDescs()[i].inputSlot == inputSlot )
		{
			assert(-1 == result);
			result = srcBatch->GetPrimaryOffset();
		}
	}
	for( size_t i = 0; i < srcBatch->GetExtraVertexElementCount(); ++i )
	{
		if( srcBatch->GetExtraVertexElementDescs()[i].inputSlot == inputSlot )
		{
			assert(-1 == result);
			result = srcBatch->GetExtraVertexElementDescs()[i].indexOffset;
		}
	}
	return result;
}

static size_t CreateTaskList(const AutoPtr<MemoryObject> *convertedIndexBuffers,
                    const std::map<unsigned int, unsigned int> *localIdxOffsetsToConvertedBufferIdx,
                    const unsigned long *inIndexCounts,
                    UnifiedBatchImpl *const* inNewBatches, 
                    size_t batchCount, 
                    unsigned int vertexBufferSlot, 
                    unsigned long *outVertexOffsets,
                    MemoryObject **outIndexBuffers)
{
	size_t taskCount = 0;
	unsigned long dstVertexOffset = 0;
	for( size_t batchIdx = 0; batchIdx < batchCount; ++batchIdx )
	{
		if( unsigned long idxCount = inIndexCounts[batchIdx] )
		{
			Batch *batchOld = inNewBatches[batchIdx]->GetTarget();
			unsigned int offset = GetIndexOffset(batchOld, vertexBufferSlot, NULL);
			if( -1 != offset )
			{
				LocalIdxOffsetsToConvertedBufferIdxType::value_type::const_iterator it = 
					localIdxOffsetsToConvertedBufferIdx[batchIdx].find(offset);
				assert(localIdxOffsetsToConvertedBufferIdx[batchIdx].end() != it);
				outVertexOffsets[taskCount] = dstVertexOffset;
				outIndexBuffers[taskCount] = convertedIndexBuffers[it->second];
				++taskCount;
			}
			dstVertexOffset += idxCount;
		}
	}
	return taskCount;
}

///////////////////////////////////////////////////////////////////////////////

UnifiedVertexBufferProcessor::UnifiedVertexBufferProcessor(IIndexCombinerAlgorithm *algorithm,
                                                           MemoryObject *const*inIndexBuffers,
                                                           unsigned long *vertexOffsets,
                                                           size_t taskCount,
                                                           unsigned int vertexStride,
                                                           MemoryObject* inVertexBuffer,
                                                           MemoryObject* outVertexBuffer)
  : ProcessorBase(algorithm)
  , m_vertexOffsets(vertexOffsets, vertexOffsets + taskCount)
  , m_vertexStride(vertexStride)
{
	std::vector<AlgorithmResource> inputs(1);
	inputs[0].semanticId = IIndexCombinerAlgorithm::IN_VERTEX_BUFFER;
	inputs[0].data = inVertexBuffer;
	for( size_t taskIdx = 0; taskIdx < taskCount; ++taskIdx )
	{
		inputs.push_back(AlgorithmResource());
		inputs.back().semanticId = IIndexCombinerAlgorithm::IN_INDEX_BUFFER;
		inputs.back().data = inIndexBuffers[taskIdx];
	}
	inputs.push_back(AlgorithmResource()); // terminator
	inputs.back().semanticId = -1;
	inputs.back().data = NULL;

	AlgorithmResource outputs[] = 
	{
		{ IIndexCombinerAlgorithm::OUT_VERTEX_BUFFER, outVertexBuffer },
		{ -1, NULL } // terminator
	};
	SetData(&inputs[0], outputs);
}

bool UnifiedVertexBufferProcessor::DoProcess()
{
	if( GetDirty() )
	{
		GetAlgorithm()->Execute(GetAccessors(), &m_vertexOffsets[0], m_vertexOffsets.size(), m_vertexStride);
		ClearDirtyFlag();
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////

UnifiedMeshImpl::UnifiedMeshImpl(IIndexCombinerAlgorithm *algorithm, AccessProviderLocal *providerLocal, Mesh *srcMesh, Scene *context)
  : MeshAdapter(srcMesh, context)
  , m_indexCounts(srcMesh->GetBatchCount())
{
	//
	// TODO: separate as algorithm
	// generate unique indices for each child batch
	//
	MemoryObjectVector convertedIndexBuffers;

	LocalIdxOffsetsToConvertedBufferIdxType localIdxOffsetsToConvertedBufferIdx;
	localIdxOffsetsToConvertedBufferIdx.resize(GetSource()->GetBatchCount());
	std::vector<unsigned long> idxNewToOld;
	unsigned long vtxOffset = 0;
	for( size_t batchIdx = 0; batchIdx < GetSource()->GetBatchCount(); ++batchIdx )
	{
		Batch *batchOld = GetSource()->GetBatch(batchIdx, context->GetPrevContext());
		AutoPtr<MemoryObject> idxBufNew;
		if( batchOld->GetIndexData() )
		{
			unsigned long idxCount = static_cast<unsigned long>(batchOld->GetIndexData()->GetBytesSize() / (sizeof(unsigned long) * batchOld->GetIndexStride()));
			AutoPtr<AccessorReadDefault> accessor;
			providerLocal->CreateAccessorReadDefault(batchOld->GetIndexData(), &accessor);
			MapHelper<const unsigned long, AccessorReadDefault> idxDataOld(accessor);
			unsigned int idxStride = batchOld->GetIndexStride();

			typedef std::map<unsigned long, unsigned long, IdxCompare> OldToNewIndexMap;
			OldToNewIndexMap idxMap(IdxCompare(&idxDataOld[0], idxStride));

			std::vector<unsigned long> idxOldToNew(idxCount);
			for( unsigned long oldIdx = 0; oldIdx < idxCount; ++oldIdx )
			{
				OldToNewIndexMap::_Pairib tmp = idxMap.insert(OldToNewIndexMap::value_type(oldIdx, static_cast<unsigned long>(idxMap.size())));
				idxOldToNew[oldIdx] = tmp.first->second + vtxOffset;
				if( tmp.second )
				{
					// newly added
					assert(idxNewToOld.size() == tmp.first->second + vtxOffset);
					idxNewToOld.push_back(oldIdx);
					assert(idxNewToOld.size() == idxMap.size() + vtxOffset);
				}
			}
			GetMemoryManager()->Allocate(idxOldToNew.size() * sizeof(unsigned long), &idxOldToNew[0], &idxBufNew);
			unsigned long vtxCount = static_cast<unsigned long>(idxMap.size());
			m_indexCounts[batchIdx] = vtxCount;

			for( unsigned int bufIdx = 0; bufIdx < GetSource()->GetVertexBufferCount(); ++bufIdx )
			{
				unsigned int idxOffset = GetIndexOffset(batchOld, bufIdx, NULL);
				if( -1 != idxOffset )
				{
					LocalIdxOffsetsToConvertedBufferIdxType::value_type::_Pairib it;
					typedef LocalIdxOffsetsToConvertedBufferIdxType::value_type::value_type Value;
					it = localIdxOffsetsToConvertedBufferIdx[batchIdx].insert(Value(idxOffset, static_cast<unsigned int>(convertedIndexBuffers.size())));
					if( it.second )
					{
						assert(idxOffset < idxStride);
						std::vector<unsigned long> dst(vtxCount);
						for( size_t i = 0; i != vtxCount; ++i )
						{
							dst[i] = idxDataOld[idxNewToOld[i + vtxOffset] * idxStride + idxOffset];
						}
						AutoPtr<MemoryObject> converted;
						GetMemoryManager()->Allocate(dst.size() * sizeof(unsigned long), &dst[0], &converted);
						convertedIndexBuffers.push_back(converted);
					}
				}
			}
			vtxOffset += vtxCount;
		}
		else
		{
			m_indexCounts[batchIdx] = 0;
		}
		m_batches.push_back(new UnifiedBatchImpl(this, batchOld, context));
		m_batches.back()->SetIndexBuffer(idxBufNew);
	}

	//
	// allocate vertex buffers
	//

	m_vtxBuffersOverride.resize(GetSource()->GetVertexBufferCount());
	m_vtxBuffersStrides.resize(GetSource()->GetVertexBufferCount());
	m_vtxBuffersSrc.resize(GetSource()->GetVertexBufferCount());

	std::vector<unsigned long> vertexOffsetsTmp(m_batches.size());
	std::vector<MemoryObject*> indexBuffersTmp(m_batches.size());

	for( unsigned int bufIdx = 0; bufIdx < static_cast<unsigned int>(m_vtxBuffersOverride.size()); ++bufIdx )
	{
		m_vtxBuffersStrides[bufIdx] = GetSource()->GetRawVertexSize(bufIdx);
		m_vtxBuffersSrc[bufIdx] = GetSource()->GetVertexBuffer(bufIdx);
		GetMemoryManager()->Allocate(idxNewToOld.size() * GetSource()->GetRawVertexSize(bufIdx), NULL, &m_vtxBuffersOverride[bufIdx]);
		size_t taskCount = CreateTaskList(&convertedIndexBuffers[0], &localIdxOffsetsToConvertedBufferIdx[0],
			&m_indexCounts[0], &m_batches[0], m_batches.size(), bufIdx, &vertexOffsetsTmp[0], &indexBuffersTmp[0]);
		AutoPtr<RefCounted>(new UnifiedVertexBufferProcessor(algorithm,
			&indexBuffersTmp[0],
			&vertexOffsetsTmp[0],
			taskCount,
			m_vtxBuffersStrides[bufIdx],
			m_vtxBuffersSrc[bufIdx],
			m_vtxBuffersOverride[bufIdx]
		));
	}
}


//
// Mesh interface override
//

size_t UnifiedMeshImpl::GetVertexBufferCount() const
{
	return m_vtxBuffersOverride.size();
}

MemoryObject* UnifiedMeshImpl::GetVertexBuffer(size_t bufferIdx) const
{
	return m_vtxBuffersOverride[bufferIdx];
}

unsigned int UnifiedMeshImpl::GetRawVertexSize(size_t bufferIdx) const
{
	return m_vtxBuffersStrides[bufferIdx];
}

Batch* UnifiedMeshImpl::GetBatch(size_t batchIdx, Scene *context) const // returns unified batch
{
	return SafeResolveContext<Batch>(context, m_batches[batchIdx]);
}

///////////////////////////////////////////////////////////////////////////////

UnifiedBatchImpl::UnifiedBatchImpl(UnifiedMeshImpl *mesh, Batch *src, Scene *context)
  : BatchAdapter(src, context)
  , m_indexCount(-1)
{
	m_layoutOverride.insert(m_layoutOverride.begin(), 
		src->GetExtraVertexElementDescs(),
		src->GetExtraVertexElementDescs() + src->GetExtraVertexElementCount());
	for( size_t i = 0; i < m_layoutOverride.size(); ++i )
	{
		m_layoutOverride[i].indexOffset = 0; // single unified index buffer
	}
}

void UnifiedBatchImpl::SetIndexBuffer(MemoryObject *buf)
{
	m_idxbufOverride = buf;
	m_indexCount = buf ? buf->GetBytesSize() / sizeof(unsigned long) : 0;
}

unsigned int UnifiedBatchImpl::GetIndexStride() const
{
	return 1; // single unified index buffer
}

MemoryObject* UnifiedBatchImpl::GetIndexData() const
{
	return m_idxbufOverride;
}

const VertexElementDescEx* UnifiedBatchImpl::GetExtraVertexElementDescs() const
{
	assert(m_layoutOverride.size() == GetTarget()->GetExtraVertexElementCount());
	return &m_layoutOverride[0];
}

size_t UnifiedBatchImpl::GetExtraVertexElementCount() const
{
	assert(m_layoutOverride.size() == GetTarget()->GetExtraVertexElementCount());
	return m_layoutOverride.size();
}

unsigned int UnifiedBatchImpl::GetPrimaryOffset() const
{
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

UnifiedBatchViewImpl::UnifiedBatchViewImpl(const Batch *srcBatch, size_t thisViewId, Scene *context)
  : m_batchIndex(-1)
  , m_thisViewId(thisViewId)
  , m_srcMesh(srcBatch->GetMesh(context->GetPrevContext()))
  , m_context(context)
{
	// search given batch it its mesh to get batch index
	for( size_t i = 0; i < m_srcMesh->GetBatchCount(); ++i )
	{
		if( m_srcMesh->GetBatch(i, context->GetPrevContext()) == srcBatch )
		{
			m_batchIndex = i;
			break;
		}
	}
	assert(-1 != m_batchIndex);
}


//
// IUnifiedBatchView interface implementation
//

Batch* UnifiedBatchViewImpl::GetUnifiedBatch()
{
	return m_srcMesh->GetViewT<IUnifiedMeshView>(m_thisViewId, m_context)->GetUnifiedMesh()->GetBatch(m_batchIndex, m_context);
}


//
// IView interface implementation
//

void UnifiedBatchViewImpl::OnDetach()
{
	delete this;
}

///////////////////////////////////////////////////////////////////////////////

UnifiedMeshViewImpl::UnifiedMeshViewImpl(IIndexCombinerAlgorithm *algorithm, AccessProviderLocal *provider, Mesh *srcMesh, size_t thisViewId, Scene *context)
  : m_mesh(srcMesh->GetBatchCount() ? new UnifiedMeshImpl(algorithm, provider, srcMesh, context) : NULL)
  , m_source(srcMesh)
{
}


//
// IUnifiedMeshView interface implementation
//

Mesh* UnifiedMeshViewImpl::GetUnifiedMesh()
{
	return m_mesh.get() ? m_mesh.get() : m_source;
}


//
// IView interface implementation
//

void UnifiedMeshViewImpl::OnDetach()
{
	delete this;
}

///////////////////////////////////////////////////////////////////////////////

IndexCombinerPluginFactory::IndexCombinerPluginFactory(IIndexCombinerAlgorithm *algorithm, AccessProviderLocal *providerLocal)
  : m_algorithm(algorithm)
  , m_provider(providerLocal)
{
}

collada::IView* IndexCombinerPluginFactory::CreateView(Scene *context, collada::Base *b, size_t thisViewId)
{
	if( Batch *batch = dynamic_cast<Batch *>(b) )
	{
		return new UnifiedBatchViewImpl(batch, thisViewId, context);
	}
	if( Mesh *mesh = dynamic_cast<Mesh *>(b) )
	{
		return new UnifiedMeshViewImpl(m_algorithm, m_provider, mesh, thisViewId, context);
	}
	return NULL;
}

void IndexCombinerPluginFactory::OnDetach()
{
	delete this;
}

///////////////////////////////////////////////////////////////////////////////

UnifiedSceneAdapter::UnifiedSceneAdapter(IIndexCombinerAlgorithm *algorithm, AccessProviderLocal *providerLocal, Scene *src, size_t pluginId)
  : SceneAdapter(src, pluginId, &CViewFactoryCreatorProc2<IIndexCombinerAlgorithm*, AccessProviderLocal*>(CreateDefaultIndexCombinerPlugin, algorithm, providerLocal))
{
}

Base* UnifiedSceneAdapter::ResolveContext(Base *target)
{
	if( this != target->GetNativeContext() )
	{
		target = __super::ResolveContext(target);
		assert(this != target->GetNativeContext());
		if( dynamic_cast<Mesh *>(target) )
		{
			target = target->GetViewT<IUnifiedMeshView>(GetPluginId(), this)->GetUnifiedMesh();
		}
		else if( dynamic_cast<Batch *>(target) )
		{
			target = target->GetViewT<IUnifiedBatchView>(GetPluginId(), this)->GetUnifiedBatch();
		}
	}
	return target;
}

///////////////////////////////////////////////////////////////////////////////

// factory
namespace collada
{
	IViewFactory* CreateDefaultIndexCombinerPlugin(IIndexCombinerAlgorithm *algorithm, AccessProviderLocal *providerLocal)
	{
		return new IndexCombinerPluginFactory(algorithm, providerLocal);
	}

	void CreateUnifiedSceneAdapter(Scene *source, Scene **ppResult, IIndexCombinerAlgorithm *algorithm, AccessProviderLocal *providerLocal, size_t combinerPluginId)
	{
		assert(0 == source->GetSkinCtrlCount());
		AutoPtr<Scene> tmp(new UnifiedSceneAdapter(algorithm, providerLocal, source, combinerPluginId));
		DetachRawPtr(tmp, ppResult);
	}
}


// end of file
