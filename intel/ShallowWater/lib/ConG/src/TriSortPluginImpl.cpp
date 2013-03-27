/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file TriSortPluginImpl.cpp

#include "stdafx.h"

#include "mathstub.h"

#include "TriSortPluginImpl.h"
#include "SGMesh.h"
#include "SceneUtils.h"

///////////////////////////////////////////////////////////////////////////////

static unsigned long GetIndex(const void *raw, size_t idxSize, size_t idx)
{
	union
	{
		unsigned long idx32;
		unsigned short idx16;
	};

	if( sizeof(unsigned long) == idxSize )
	{
		idx32 = *((unsigned long *) raw + idx);
	}
	else
	{
		idx32 = *((unsigned short *) raw + idx);
		if( 0xffff == idx16 )
		{
			idx32 |= 0xffff0000;
		}
	}

	return idx32;
}

namespace
{
#pragma pack (push)
#pragma pack (2)
	struct Tri16
	{
		unsigned short idx[3];
	};
#pragma pack (4)
	struct Tri32
	{
		unsigned long idx[3];
	};
#pragma pack (pop)

	template<class ItemType>
	struct CompareItems : public std::binary_function<ItemType*, ItemType*, bool>
	{
		const float3 * const positions;
		const float4x4 *wv;

		CompareItems(const float *pos, const float *mWorldView)
		  : positions((const float3 *) pos)
		  , wv((const float4x4 *) mWorldView)
		{
		}

		bool operator () (const ItemType &left, const ItemType &right)
		{
			float3 v1(0,0,0), v2(0,0,0);
			for( int i = 0; i < 3; ++i )
			{
				Vec3Add(&v1, &v1, positions + left.idx[i]);
				Vec3Add(&v2, &v2, positions + right.idx[i]);
			}
			Vec3TransformCoord(&v1, &v1, wv);
			Vec3TransformCoord(&v2, &v2, wv);
			return v1.z > v2.z;
		}
	};
} // namespace


SortedBatchImpl::SortedBatchImpl(Batch *src, SortedMeshInstanceImpl *inst, 
                                 TriSortPluginFactory *factory, Scene *context)
  : BatchAdapter(src, context)
  , m_vtxSrc(NULL)
  , m_instance(inst)
  , m_factory(factory)
  , m_triCount(CalcPrimitiveCount(src))
{
	// only triangle list topology is supported
	assert(PRIMITIVE_TOPOLOGY_TRIANGLELIST == GetTarget()->GetPrimitiveTopology());

	AccessProviderLocal *provider = NULL; // TODO: get provider somewhere
	assert(provider);

	// accessor for vertex positions
	m_vtxSrc = FindVertexBuffer(GetTarget()->GetMesh(context), "POSITION");

	// copy original indices
//	AutoPtr<AccessorReadDefault> reader;
//	provider->CreateAccessorReadDefault(GetTarget()->GetIndexData(), &reader);
//	MapHelper<const unsigned long, AccessorReadDefault> mappedIndicesOriginal(reader);
	GetMemoryManager()->Allocate(GetTarget()->GetIndexData()->GetBytesSize(), NULL/*&mappedIndicesOriginal[0]*/, &m_idxbuf);
}

/*
const void* SortedBatchImpl::GetRawIndexData() const
{
	if( m_idxbuf.empty() )
	{
		return NULL;
	}

	const float4x4 &w = *(const float4x4 *) m_instance->GetNode(GetNativeContext())->GetWorld(GetNativeContext());
	const float4x4 &v = *(const float4x4 *) m_factory->GetView();
	float4x4 mWorldView;
	MatrixMultiply(&mWorldView, &w, &v);

	FireUpdate(UPDATE_FLAG_STATE_INT|UPDATE_FLAG_BATCH_INDICES, true);

	Tri32 *tri = (Tri32 *) &m_idxbuf[0];
	std::sort(tri, tri + m_triCount, CompareItems<Tri32>(m_vtxSrc, mWorldView));

	FireUpdate(UPDATE_FLAG_STATE_INT|UPDATE_FLAG_BATCH_INDICES, false);

	return &m_idxbuf[0];
}
*/

MemoryObject* SortedBatchImpl::GetIndexData() const
{
	return m_idxbuf;
}

///////////////////////////////////////////////////////////////////////////////

SortedMeshImpl::SortedMeshImpl(Mesh *src, SortedMeshInstanceImpl *inst, 
                               TriSortPluginFactory *factory, Scene *context)
  : MeshAdapter(src, context)
  , m_instance(inst)
{
	for( size_t i = 0; i < GetSource()->GetBatchCount(); ++i )
	{
		Batch *srcBatch = GetSource()->GetBatch(i, context->GetPrevContext());
		m_batches.push_back(new SortedBatchImpl(srcBatch, m_instance, factory, context));
	}
}

Batch* SortedMeshImpl::GetBatch(size_t batchIdx, Scene *context) const
{
	return SafeResolveContext<Batch>(context, m_batches[batchIdx]);
}

///////////////////////////////////////////////////////////////////////////////

SortedMeshInstanceImpl::SortedMeshInstanceImpl(MeshInstance *src, TriSortPluginFactory *factory, Scene *context)
  : GeometryInstanceAdapter(src, context)
{
	m_mesh.reset(new SortedMeshImpl(src->GetEntity(context->GetPrevContext()), this, factory, context));
}

Mesh* SortedMeshInstanceImpl::GetEntity(Scene *context) const
{
	return m_mesh.get();
}

///////////////////////////////////////////////////////////////////////////////

SortedMeshInstanceViewImpl::SortedMeshInstanceViewImpl(MeshInstance *src, TriSortPluginFactory *factory, Scene *context)
  : m_instance(new SortedMeshInstanceImpl(src, factory, context))
  , m_factory(factory)
{
}

SortedMeshInstanceViewImpl::~SortedMeshInstanceViewImpl()
{
	m_factory->OnDestroyView(this);
}

void SortedMeshInstanceViewImpl::OnUpdateViewMatrix()
{
	assert(false); // TODO:
}


//
// ISortedMeshInstanceView interface
//

MeshInstance* SortedMeshInstanceViewImpl::GetMeshInstance()
{
	return m_instance.get();
}


//
// IView interface implementation
//

void SortedMeshInstanceViewImpl::OnDetach()
{
	delete this;
}


///////////////////////////////////////////////////////////////////////////////

// constructor
TriSortPluginFactory::TriSortPluginFactory()
{
}

void TriSortPluginFactory::OnDestroyView(SortedMeshInstanceViewImpl *view)
{
	assert(m_views.count(view));
	m_views.erase(view);
}


//
// ITriSortViewFactory interface implementation
//

void TriSortPluginFactory::SetView(const float *mView) // float4
{
	memcpy(&m_mView, mView, sizeof(m_mView));
	for( ViewSet::const_iterator it = m_views.begin(); it != m_views.end(); ++it )
	{
		(*it)->OnUpdateViewMatrix();
	}
}


//
// IViewFactory interface implementation
//

IView* TriSortPluginFactory::CreateView(Scene *context, Base *b, size_t thisViewId)
{
	if( MeshInstance *mi = dynamic_cast<MeshInstance *>(b) )
	{
		return *m_views.insert(new SortedMeshInstanceViewImpl(mi, this, context)).first;
	}
	return NULL;
}

void TriSortPluginFactory::OnDetach()
{
	delete this;
}

///////////////////////////////////////////////////////////////////////////////


SortedSceneImpl::SortedSceneImpl(Scene *source, size_t sortPluginId)
  : SceneAdapter(source, sortPluginId, &CViewFactoryCreatorProc((CreatePluginProc)CreateDefaultSortingPlugin))
  , m_factory(dynamic_cast<ITriSortViewFactory*>(GetSource()->GetViewFactory(GetPluginId())))
{
	assert(m_factory);
}

void SortedSceneImpl::SetView(const float *mView) // float4x4
{
	m_factory->SetView(mView);
}

Base* SortedSceneImpl::ResolveContext(Base *target)
{
	if( this != target->GetNativeContext() )
	{
		target = __super::ResolveContext(target);
		assert(this != target->GetNativeContext());
		if( dynamic_cast<MeshInstance *>(target) )
		{
			target = target->GetViewT<ISortedMeshInstanceView>(GetPluginId(), this)->GetMeshInstance();
		}
	}
	return target;
}

///////////////////////////////////////////////////////////////////////////////

SceneSorterImpl::SceneSorterImpl(Scene *source, size_t sortPluginId)
  : m_source(source)
  , m_sortedScene(new SortedSceneImpl(source, sortPluginId))
  , m_refCount(0)
{
}

SceneSorterImpl::~SceneSorterImpl()
{
}


//
// ISceneSorter interface implementation
//

Scene* SceneSorterImpl::GetSortedScene()
{
	return m_sortedScene;
}

void SceneSorterImpl::SetView(const float *mView) // float4x4
{
	m_sortedScene->SetView(mView);
}

///////////////////////////////////////////////////////////////////////////////

// factory
namespace collada
{
	ITriSortViewFactory* CreateDefaultSortingPlugin()
	{
		return new TriSortPluginFactory();
	}

	void CreateSceneSorter(Scene *source, ISceneSorter **ppResult, size_t sortPluginId)
	{
		AutoPtr<ISceneSorter> result(new SceneSorterImpl(source, sortPluginId));
		DetachRawPtr(result, ppResult);
	}
}

// end of file
