/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGMeshAdapter.cpp

#include "stdafx.h"

#include "SGMesh.h"
#include "SGMeshAdapter.h"


BatchAdapter::BatchAdapter(Batch *target, Scene *context)
  : BaseImpl(context)
  , m_target(target)
{
}

BatchAdapter::~BatchAdapter()
{
}


//
// Batch interface
//

Mesh* BatchAdapter::GetMesh(Scene *context) const
{
	return m_target->GetMesh(context);
}

unsigned int BatchAdapter::GetIndexStride() const
{
	return m_target->GetIndexStride();
}

MemoryObject* BatchAdapter::GetIndexData() const
{
	return m_target->GetIndexData();
}

MemoryObject* BatchAdapter::GetCountData() const
{
	return m_target->GetCountData();
}

const VertexElementDescEx* BatchAdapter::GetExtraVertexElementDescs() const
{
	return m_target->GetExtraVertexElementDescs();
}

size_t BatchAdapter::GetExtraVertexElementCount() const
{
	return m_target->GetExtraVertexElementCount();
}

//const float* BatchAdapter::GetCenter(Scene *context) const // float3
//{
//	return m_target->GetCenter(context);
//}
//
//const float* BatchAdapter::GetAABB(Scene *context) const // float3
//{
//	return m_target->GetAABB(context);
//}

bool BatchAdapter::GetDoubleSided() const
{
	return m_target->GetDoubleSided();
}

unsigned int BatchAdapter::GetPrimarySet() const
{
	return m_target->GetPrimarySet();
}

unsigned int BatchAdapter::GetPrimaryOffset() const
{
	return m_target->GetPrimaryOffset();
}

PrimitiveTopology BatchAdapter::GetPrimitiveTopology() const
{
	return m_target->GetPrimitiveTopology();
}

///////////////////////////////////////////////////////////////////////////////

MeshAdapter::MeshAdapter(Mesh *target, Scene *context)
  : BaseImpl(context)
  , m_target(target)
{
}

MeshAdapter::~MeshAdapter()
{
}

//
// Mesh interface
//

const VertexElementDesc* MeshAdapter::GetPrimaryVertexElementDescs() const
{
	return m_target->GetPrimaryVertexElementDescs();
}

size_t MeshAdapter::GetPrimaryVertexElementCount() const
{
	return m_target->GetPrimaryVertexElementCount();
}

size_t MeshAdapter::GetVertexBufferCount() const
{
	return m_target->GetVertexBufferCount();
}

MemoryObject* MeshAdapter::GetVertexBuffer(size_t bufferIdx) const
{
	return m_target->GetVertexBuffer(bufferIdx);
}

//size_t MeshAdapter::GetVertexCount(size_t bufferIdx) const
//{
//	return m_target->GetVertexCount(bufferIdx);
//}

unsigned int MeshAdapter::GetRawVertexSize(size_t bufferIdx) const
{
	return m_target->GetRawVertexSize(bufferIdx);
}

size_t MeshAdapter::GetBatchCount() const
{
	return m_target->GetBatchCount();
}

Batch* MeshAdapter::GetBatch(size_t batchIdx, Scene *context) const
{
	return m_target->GetBatch(batchIdx, context);
}

// end of file
