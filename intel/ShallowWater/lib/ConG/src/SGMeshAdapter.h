/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGMeshAdapter.h

#include "SceneGraph.h"
#include "SGImplBase.h"

using namespace collada;

//namespace collada
//{
///////////////////////////////////////////////////////////////////////////////

class BatchAdapter : public BaseImpl<Batch>
{
public:
	BatchAdapter(Batch *target, Scene *context);
	virtual ~BatchAdapter() = 0;

	Batch* GetTarget() const { return m_target; }


	// Batch interface
	virtual Mesh* GetMesh(Scene *context) const;
	virtual unsigned int GetIndexStride() const;
	virtual MemoryObject* GetIndexData() const;
	virtual MemoryObject* GetCountData() const;
	virtual const VertexElementDescEx* GetExtraVertexElementDescs() const;
	virtual size_t GetExtraVertexElementCount() const;
//	virtual const float* GetCenter(Scene *context) const; // float3
//	virtual const float* GetAABB(Scene *context) const; // float3
	virtual bool GetDoubleSided() const;
	virtual unsigned int GetPrimarySet() const;
	virtual unsigned int GetPrimaryOffset() const;
	virtual PrimitiveTopology GetPrimitiveTopology() const;

private:
	Batch *m_target;
};

///////////////////////////////////////////////////////////////////////////////

class MeshAdapter : public BaseImpl<Mesh>
{
public:
	MeshAdapter(Mesh *target, Scene *context);
	virtual ~MeshAdapter() = 0;

	Mesh* GetSource() const { return m_target; }

	// Mesh interface
	virtual const VertexElementDesc* GetPrimaryVertexElementDescs() const;
	virtual size_t GetPrimaryVertexElementCount() const;
	virtual size_t GetVertexBufferCount() const;
	virtual MemoryObject* GetVertexBuffer(size_t bufferIdx) const;
	virtual unsigned int GetRawVertexSize(size_t bufferIdx) const;
	virtual size_t GetBatchCount() const;
	virtual Batch* GetBatch(size_t batchIdx, Scene *context) const;
//	virtual const float* GetCenter() const; // float3
//	virtual const float* GetAABB() const; // float3

private:
	Mesh *m_target;
};

///////////////////////////////////////////////////////////////////////////////
//} // end of namespace collada

// end of file
