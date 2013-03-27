/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGMesh.cpp

#include "stdafx.h"

#include "SGMesh.h"

///////////////////////////////////////////////////////////////////////////////

EditableBatchImpl::EditableBatchImpl(Scene *s, Mesh *mesh)
  : BaseImpl(s)
  , m_mesh(mesh)
  , m_indexStride(1) // at least one index per vertex (base index)
  , m_primarySet(-1)
  , m_primaryOffset(-1)
  , m_doubleSided(false)
  , m_topology(PRIMITIVE_TOPOLOGY_UNDEFINED)
{
}

EditableBatchImpl::~EditableBatchImpl()
{
}

Mesh* EditableBatchImpl::GetMesh(Scene *context) const
{
	return SafeResolveContext<Mesh>(context, m_mesh);
}

void EditableBatchImpl::SetIndexData(MemoryObject *indexData)
{
	m_indexData = indexData;
}

void EditableBatchImpl::SetCountData(MemoryObject *countData)
{
	m_countData = countData;
}

void EditableBatchImpl::AddExtraVertexElement(const VertexElementDescEx *desc)
{
	m_layout.push_back(*desc);

	// cache semantic name
	m_layout.back().semanticName = m_semanticNameCache.insert(desc->semanticName).first->c_str();

	// update index stride
	if( m_indexStride < desc->indexOffset + 1 )
		m_indexStride = desc->indexOffset + 1;
}

void EditableBatchImpl::SetPrimarySet(unsigned int primarySet)
{
	m_primarySet = primarySet;
}

void EditableBatchImpl::SetPrimaryOffset(unsigned int primaryOffset)
{
	// update index stride
	if( m_indexStride < primaryOffset + 1 )
		m_indexStride = primaryOffset + 1;
	m_primaryOffset = primaryOffset;
}

unsigned int EditableBatchImpl::GetPrimarySet() const
{
	return m_primarySet;
}

unsigned int EditableBatchImpl::GetPrimaryOffset() const
{
	return m_primaryOffset;
}

static inline unsigned long GetIndex(const void *raw, size_t idx, unsigned int indexStride, size_t offset)
{
	assert(indexStride > 0);
	assert(offset < indexStride);

	return *((const unsigned long *) raw + idx * indexStride + offset);
}

/*
void EditableBatchImpl::CalcBounds(float3 &center, float3 &aabb, const Batch *batch, Scene *context)
{
	batch = SafeResolveContext<Batch>(context, const_cast<Batch *>(batch));
	assert(batch);

	Mesh *mesh = batch->GetMesh(context);
	assert(mesh);

	const VertexElementDesc *desc = NULL;
	for( size_t i = 0; i < mesh->GetPrimaryVertexElementCount(); ++i )
	{
		if( 0 == _stricmp("POSITION", mesh->GetPrimaryVertexElementDescs()[i].semanticName) )
		{
			desc = mesh->GetPrimaryVertexElementDescs() + i;
			break;
		}
	}
	assert(desc);
	assert(ST_FLOAT == desc->componentType);
	assert(desc->componentCount >= 3);
	// TODO: template versions for 1d,2d,4d

	{
		MemoryObject *idxMem = batch->GetIndexData();
		MemoryObject *vtxMem = mesh->GetVertexBuffer(desc->inputSlot);

		const void *idata = idxMem->Lock(MemoryObject::LOCK_READ);
		const void *vdata = vtxMem->Lock(MemoryObject::LOCK_READ);
		unsigned int vstride = mesh->GetRawVertexSize(desc->inputSlot);
		unsigned int ioffset = batch->GetPrimaryOffset();

		float3 min(0,0,0), max(0,0,0);
		bool first = true;
		size_t iVertCount = batch->GetIndexCountTotal() / batch->GetIndexStride();
		for( size_t i = 0; i < iVertCount; ++i )
		{
			unsigned long idx = GetIndex(idata, i, batch->GetIndexStride(), ioffset);
			assert(idx < mesh->GetVertexCount(desc->inputSlot));
			const float3 *pos = (const float3 *) ((const char *) vdata + vstride * idx);
			if( first )
			{
				first = false;
				min = max = *pos;
			}
			else
			{
				Vec3Maximize(&max, &max, pos);
				Vec3Minimize(&min, &min, pos);
			}
		}
		center = (max + min) / 2;
		aabb = max - min;

		vtxMem->Unlock();
		idxMem->Unlock();
	}
	else
	{
		assert(false); // native representation not supported
	}
}
*/

void EditableBatchImpl::InvalidateAllBounds()
{
	for( BoundsCache::iterator it = m_bounds.begin(); m_bounds.end() != it; ++it )
	{
		it->second.isValid = false;
	}
}

unsigned int EditableBatchImpl::GetIndexStride() const
{
	return m_indexStride;
}

const VertexElementDescEx* EditableBatchImpl::GetExtraVertexElementDescs() const
{
	return m_layout.empty() ? NULL : &m_layout.front();
}

size_t EditableBatchImpl::GetExtraVertexElementCount() const
{
	return m_layout.size();
}

MemoryObject* EditableBatchImpl::GetIndexData() const
{
	return m_indexData;
}

MemoryObject* EditableBatchImpl::GetCountData() const
{
	return m_countData;
}

void EditableBatchImpl::SetDoubleSided(bool value)
{
	m_doubleSided = value;
}

void EditableBatchImpl::SetPrimitiveTopology(PrimitiveTopology pt)
{
	m_topology = pt;
}
/*
const float* EditableBatchImpl::GetCenter(Scene *context) const
{
	Bounds &b = m_bounds[context];
	if( !b.isValid )
	{
		assert(!m_isIndexBufferLocked);
		assert(!m_isCountBufferLocked);
		CalcBounds(b.center, b.aabb, this, context);
		b.isValid = true;
	}
	return b.center;
}

const float* EditableBatchImpl::GetAABB(Scene *context) const // float3
{
	Bounds &b = m_bounds[context];
	if( !b.isValid )
	{
		assert(!m_isIndexBufferLocked);
		assert(!m_isCountBufferLocked);
		CalcBounds(b.center, b.aabb, this, context);
		b.isValid = true;
	}
	return b.aabb;
}
*/
PrimitiveTopology EditableBatchImpl::GetPrimitiveTopology() const
{
	return m_topology;
}

///////////////////////////////////////////////////////////////////////////////

EditableMeshImpl::EditableMeshImpl(EditableScene *s, const VertexBufferDesc *bufferDescs, size_t bufferDescCount)
  : BaseImpl(s)
{
	m_primaryBuffers.resize(bufferDescCount);
	for( size_t bufIdx = 0; bufIdx < bufferDescCount; ++bufIdx )
	{
		m_primaryBuffers[bufIdx].bytesStride = bufferDescs[bufIdx].bytesStride;
		assert(m_primaryBuffers[bufIdx].bytesStride > 0);
	}
}

EditableMeshImpl::~EditableMeshImpl()
{
}


//
// EditableMesh interface implementation
//

void EditableMeshImpl::SetVertexBuffer(size_t bufferIdx, MemoryObject *data)
{
	m_primaryBuffers[bufferIdx].mem = data;
}

void EditableMeshImpl::AddPrimaryVertexElementDesc(const VertexElementDesc *desc)
{
	m_primaryLayout.push_back(*desc);
	// cache semantic name
	m_primaryLayout.back().semanticName = m_semanticNameCache.insert(desc->semanticName).first->c_str();
}

const VertexElementDesc* EditableMeshImpl::GetPrimaryVertexElementDescs() const
{
	return &m_primaryLayout[0];
}

size_t EditableMeshImpl::GetPrimaryVertexElementCount() const
{
	return m_primaryLayout.size();
}

size_t EditableMeshImpl::GetVertexBufferCount() const
{
	return m_primaryBuffers.size();
}

MemoryObject* EditableMeshImpl::GetVertexBuffer(size_t bufferIdx) const
{
	return m_primaryBuffers[bufferIdx].mem;
}

unsigned int EditableMeshImpl::GetRawVertexSize(size_t bufferIdx) const
{
	return m_primaryBuffers[bufferIdx].bytesStride;
}

size_t EditableMeshImpl::GetBatchCount() const
{
	return m_batches.size();
}

Batch* EditableMeshImpl::GetBatch(size_t index, Scene *context) const
{
	return SafeResolveContext<Batch>(context, m_batches[index]);
}

EditableBatch* EditableMeshImpl::CreateBatch()
{
	std::auto_ptr<EditableBatchImpl> batch(new EditableBatchImpl(GetNativeContext(), this));
	m_batches.push_back(batch.release());
//	GetScene()->OnCreateObject(m_batches.back()); // TODO: notify scene graph
	return m_batches.back();
}

///////////////////////////////////////////////////////////////////////////////

MeshInstanceImpl::MeshInstanceImpl(Scene *s, Node *node, Mesh *mesh)
  : GeometryInstanceImpl(s, mesh, node)
{
}

// end of file
