/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SceneUtils.cpp

#include "stdafx.h"

#include "mathstub.h"
#include "SceneUtils.h"
#include "SceneGraph.h"
#include "AccessProviderDefault.h"
#include "AutoPtr.h"
using namespace collada;

static float Zero[16] ={0};


namespace collada
{

CameraInstance* CreateDefaultCamera(EditableScene *es, const char *camTag, const char *nodeTag, const char *instTag)
{
	Camera *pCamera = es->CreateCamera();
	pCamera->SetTag(camTag);
	Node *pNode = es->CreateNode(NULL);
	pNode->SetTag(nodeTag);
	CameraInstance *pCI = es->CreateCameraInstance(pNode, pCamera);
	pCI->SetTag(instTag);
	Property* pTransform = pNode->AppendTransform(Transform::MATRIX, Zero)->GetDelegate(NULL);

	AutoPtr<AccessProviderLocal> providerLocal;
	CreateAccessProviderLocal(&providerLocal);

	float3 aabb(es->GetSceneAABB(-1, 0, providerLocal));
	float3 center(es->GetSceneCenter(-1, 0, providerLocal));
	float4x4 ypr;
	MatrixRotationYawPitchRoll(&ypr, 3.141692f/4, -3.141692f/4, 0);

	float4x4 t0;
	MatrixTranslation(&t0, center.x, center.y, center.y);

	float4x4 t1;
	float bbD = Vec3Length(&aabb);
	MatrixTranslation(&t1, 0, 0, bbD*2);
	float4x4 up = es->GetUpAxis();
	float4x4 upI;
	MatrixInverse(&upI, 0, &up);
	float4x4 minusZ;
	MatrixScaling(&minusZ, 1,1,-1);
	float4x4 m = t1*ypr*t0*minusZ*upI;
	MatrixTranspose(&m, &m);
	pTransform->SetValue(m);
	pCamera->SetFar(10*bbD);

	return pCI;
}

///////////////////////////////////////////////////////////////////////////////

MemoryObject* FindVertexBuffer(Mesh *mesh, const char *semantic)
{
	const collada::VertexElementDesc *descs = mesh->GetPrimaryVertexElementDescs();
	for( size_t i = 0; i < mesh->GetPrimaryVertexElementCount(); ++i )
	{
		if( 0 == strcmp(descs[i].semanticName, semantic) )
		{
			return mesh->GetVertexBuffer(descs[i].inputSlot);
		}
	}
	return NULL;
}

MemoryObject* FindVertexBuffer(Batch *batch, Scene *context, const char *semantic)
{
    const collada::VertexElementDescEx *descs = batch->GetExtraVertexElementDescs();
    for( size_t i = 0; i < batch->GetExtraVertexElementCount(); ++i )
	{
		if( 0 == strcmp(descs[i].semanticName, semantic) )
		{
            return batch->GetMesh(context)->GetVertexBuffer(descs[i].inputSlot);
		}
	}
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////

GeometryInstanceIterator::GeometryInstanceIterator(Scene *s)
  : m_scene(s)
  , m_instIdx(0)
  , m_batchIdx(0)
  , m_currentMesh(NULL)
{
	if( IsValid() && GetMesh()->GetBatchCount() == 0 )
	{
		StepNext();
	}
}

bool GeometryInstanceIterator::IsValid() const
{
	return m_instIdx < m_scene->GetMeshInstanceCount();
}

void GeometryInstanceIterator::StepNext()
{
	assert(IsValid());
	++m_batchIdx;
	if( m_batchIdx >= GetMesh()->GetBatchCount() )
	{
		m_batchIdx = 0;
		m_currentMesh = NULL;
		do
		{
			++m_instIdx; // skip empty meshes
		} while( IsValid() && GetMesh()->GetBatchCount() == 0 );
	}
}

GeometryInstance* GeometryInstanceIterator::GetInst()
{
	assert(IsValid());
	return m_scene->GetMeshInstance(m_instIdx);
}

Mesh* GeometryInstanceIterator::GetMesh()
{
	assert(IsValid());
	if( !m_currentMesh )
	{
		m_currentMesh = m_scene->GetMeshInstance(m_instIdx)->GetEntity(m_scene);
	}
	return m_currentMesh;
}

Node* GeometryInstanceIterator::GetNode()
{
	assert(IsValid());
	return GetInst()->GetNode(m_scene);
}

Batch* GeometryInstanceIterator::GetBatch()
{
	assert(IsValid());
	return GetMesh()->GetBatch(m_batchIdx, m_scene);
}

MaterialBinding* GeometryInstanceIterator::GetMaterialBinding()
{
	assert(IsValid());
	return GetInst()->GetMaterialBinding(m_batchIdx, m_scene);
}


} // end of namespace collada
// end of file
