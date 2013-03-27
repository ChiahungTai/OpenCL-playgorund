/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGMesh.h

#pragma once

#include "mathstub.h"

#include "SceneGraph.h"
#include "SGImplBase.h"
#include "SGGeometryInstance.h"
#include "AutoPtr.h"

#include <set>

using namespace collada;

///////////////////////////////////////////////////////////////////////////////
// mesh subset

class EditableBatchImpl : public BaseImpl<EditableBatch>
{
public:
	EditableBatchImpl(Scene *s, Mesh *mesh);
	virtual ~EditableBatchImpl();


	//
	// Batch interface implementation
	//

	virtual Mesh* GetMesh(Scene *context) const;
	virtual unsigned int GetPrimarySet() const;
	virtual unsigned int GetPrimaryOffset() const;
	virtual unsigned int GetIndexStride() const;
	virtual MemoryObject* GetIndexData() const;
	virtual MemoryObject* GetCountData() const;
	virtual PrimitiveTopology GetPrimitiveTopology() const;
	virtual const VertexElementDescEx* GetExtraVertexElementDescs() const;
	virtual size_t GetExtraVertexElementCount() const;
//	virtual const float* GetCenter(Scene *context) const; // float3
//	virtual const float* GetAABB(Scene *context) const; // float3
	virtual bool GetDoubleSided() const { return m_doubleSided; }


	//
	// EditableBatch interface implementation
	//
	virtual void SetIndexData(MemoryObject *indexData);
	virtual void SetCountData(MemoryObject *countData);
	virtual void AddExtraVertexElement(const VertexElementDescEx *desc);
	virtual void SetPrimarySet(unsigned int primarySet);
	virtual void SetPrimaryOffset(unsigned int primaryOffset);
	virtual void SetDoubleSided(bool value);
	virtual void SetPrimitiveTopology(PrimitiveTopology pt);

protected:
	static void CalcBounds(float3 &center, float3 &aabb, const Batch *batch, Scene *context);
	void InvalidateAllBounds();

	Mesh *m_mesh;

	// input layout
	std::vector<VertexElementDescEx> m_layout;
	std::set<std::string> m_semanticNameCache;
	unsigned int m_primarySet;
	unsigned int m_primaryOffset;

	PrimitiveTopology m_topology;

	// raw index data
	AutoPtr<MemoryObject> m_indexData;
	unsigned int m_indexStride;

	// raw count data
	AutoPtr<MemoryObject> m_countData;

	struct Bounds
	{
		float3 center;
		float3 aabb;
		bool isValid;
		Bounds() : isValid(false) {}
	};
	typedef std::map<const Scene *, Bounds> BoundsCache;
	mutable BoundsCache m_bounds;

	bool m_doubleSided;
};

///////////////////////////////////////////////////////////////////////////////

class EditableMeshImpl : public BaseImpl<EditableMesh>
{
public:
	// construction
	EditableMeshImpl(EditableScene *s, const VertexBufferDesc *bufferDescs, size_t bufferDescCount);
	virtual ~EditableMeshImpl();

	// Mesh interface implementation
	virtual const VertexElementDesc* GetPrimaryVertexElementDescs() const;
	virtual size_t GetPrimaryVertexElementCount() const;
	virtual size_t GetVertexBufferCount() const;
	virtual MemoryObject* GetVertexBuffer(size_t bufferIdx) const;
	virtual unsigned int GetRawVertexSize(size_t bufferIdx) const;
	virtual size_t GetBatchCount() const;
	virtual Batch* GetBatch(size_t batchIdx, Scene *context) const;

	// EditableMesh interface implementation
	virtual void SetVertexBuffer(size_t bufferIdx, MemoryObject *data);
	virtual void AddPrimaryVertexElementDesc(const VertexElementDesc *desc);
	virtual EditableBatch* CreateBatch();

private:
	EditableMeshImpl(const EditableMeshImpl&); // protect from copy
	PtrOwnerArray<EditableBatchImpl> m_batches;

	// raw vertex data
	struct VertexBuffer
	{
		unsigned int       bytesStride;
		AutoPtr<MemoryObject> mem;
		VertexBuffer()
			: bytesStride(-1)
		{}
	};

	std::vector<VertexBuffer> m_primaryBuffers;
	size_t m_vtxBufferLockCount;

	std::vector<VertexElementDesc> m_primaryLayout;
	std::set<std::string> m_semanticNameCache;
};

///////////////////////////////////////////////////////////////////////////////

class MeshInstanceImpl : public GeometryInstanceImpl<MeshInstance, Mesh>
{
public:
	// create static mesh instance
	MeshInstanceImpl(Scene *s, Node *node, Mesh *mesh);
};


// end of file
