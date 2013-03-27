/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file TriangulatorPluginImpl.cpp

#include "stdafx.h"

#include "SGMesh.h"
#include "TriangulatorPluginImpl.h"
#include "AccessProviderDefault.h"

///////////////////////////////////////////////////////////////////////////////

inline static void CopyIndexStride(unsigned long *dst, const unsigned long *src, size_t widx, size_t idxStride)
{
	memcpy(dst, src + widx * idxStride, sizeof(unsigned long) * idxStride);
}

///////////////////////////////////////////////////////////////////////////////

TriangleBatchImpl::TriangleBatchImpl(Batch *src, AccessProviderLocal *providerLocal, Scene *context)
  : BatchAdapter(src, context)
{
	const size_t primitiveCountOriginal = CalcPrimitiveCount(GetTarget());
	// only polylist topology is supported
	if( PRIMITIVE_TOPOLOGY_POLYLIST == GetTarget()->GetPrimitiveTopology() && primitiveCountOriginal )
	{
		unsigned int idxStride = GetTarget()->GetIndexStride();

		AutoPtr<AccessorReadDefault> accessorIdx;
		providerLocal->CreateAccessorReadDefault(GetTarget()->GetIndexData(), &accessorIdx);
		MapHelper<const unsigned long, AccessorReadDefault> lockerIdx(accessorIdx);

		AutoPtr<AccessorReadDefault> accessorCnt;
		providerLocal->CreateAccessorReadDefault(GetTarget()->GetCountData(), &accessorCnt);
		MapHelper<const unsigned long, AccessorReadDefault> lockerCnt(accessorCnt);

		// calculate number of triangles
		size_t triCount = 0;
		for( size_t p = 0; p < primitiveCountOriginal; ++p )
		{
			unsigned long c = lockerCnt[p];
			assert(c >= 3);    // at least three vertices in polygon
			triCount += c - 2;
		}

		if( size_t indexCount = triCount * 3 * idxStride )
		{
			//
			// allocate space
			//

			GetMemoryManager()->Allocate(sizeof(unsigned long) * indexCount, NULL, &m_idxbuf);
			AutoPtr<AccessorWriteDefault> accessorDst;
			providerLocal->CreateAccessorWriteDefault(m_idxbuf, &accessorDst, NULL);
			MapHelper<unsigned long, AccessorWriteDefault> lockerDst(accessorDst);
			unsigned long *dstPtr = &lockerDst[0];


			//
			// do simple triangulation
			//

			std::vector<unsigned long> index123(idxStride * 3, -1); // space for one triangle

			size_t primitive = 0;
			size_t vcount = 0;
			size_t wideIdxCount = GetTarget()->GetIndexData()->GetBytesSize() / (sizeof(unsigned long) * idxStride);
			for( size_t widx = 0; widx < wideIdxCount; ++widx )
			{
				if( vcount < 3 )
				{
					CopyIndexStride(&index123[idxStride * vcount], &lockerIdx[0], widx, idxStride);
				}
				else
				{
					std::copy(index123.begin() + idxStride*2, index123.begin() + idxStride*3, index123.begin() + idxStride);
					CopyIndexStride(&index123[idxStride * 2], &lockerIdx[0], widx, idxStride);
				}

				if( ++vcount >= 3 )
				{
					CopyIndexStride(dstPtr, &index123[0], 0, idxStride*3);
					dstPtr += idxStride*3;
					assert((const char *) dstPtr <= (const char *) &lockerDst[0] + m_idxbuf->GetBytesSize());
				}

				if( lockerCnt[primitive] == vcount )
				{
					vcount = 0;
					++primitive;
				}
			}
		}
	}
}

MemoryObject* TriangleBatchImpl::GetIndexData() const
{
	return m_idxbuf;
}

MemoryObject* TriangleBatchImpl::GetCountData() const
{
	return NULL;
}


PrimitiveTopology TriangleBatchImpl::GetPrimitiveTopology() const
{
	return PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

///////////////////////////////////////////////////////////////////////////////

TriangleBatchViewImpl::TriangleBatchViewImpl(Batch *src, AccessProviderLocal *providerLocal, Scene *context)
  : m_source(src)
  , m_batch(PRIMITIVE_TOPOLOGY_POLYLIST == src->GetPrimitiveTopology() ? 
            new TriangleBatchImpl(src, providerLocal, context) : NULL)
{
}

Batch* TriangleBatchViewImpl::GetTriangleBatch()
{
	return m_batch.get() ? m_batch.get() : m_source;
}

void TriangleBatchViewImpl::OnDetach()
{
	delete this;
}

///////////////////////////////////////////////////////////////////////////////

// constructor
TriangulatorPluginFactory::TriangulatorPluginFactory(AccessProviderLocal *providerLocal)
  : m_provider(providerLocal)
{
}

//
// IViewFactory interface implementation
//

IView* TriangulatorPluginFactory::CreateView(Scene *context, Base *b, size_t thisViewId)
{
	if( Batch *batch = dynamic_cast<Batch *>(b) )
	{
		return new TriangleBatchViewImpl(batch, m_provider, context);
	}
	return NULL;
}

void TriangulatorPluginFactory::OnDetach()
{
	delete this;
}

///////////////////////////////////////////////////////////////////////////////

TriangulatedSceneAdapter::TriangulatedSceneAdapter(AccessProviderLocal *providerLocal, Scene *src, size_t pluginId)
  : SceneAdapter(src, pluginId, &CViewFactoryCreatorProc1<AccessProviderLocal*>(CreateDefaultTriangulatorPlugin, providerLocal))
{
}

Base* TriangulatedSceneAdapter::ResolveContext(Base *target)
{
	if( this != target->GetNativeContext() )
	{
		target = __super::ResolveContext(target);
		assert(this != target->GetNativeContext());
		if( dynamic_cast<Batch *>(target) )
		{
			target = target->GetViewT<ITriangleBatchView>(GetPluginId(), this)->GetTriangleBatch();
		}
	}
	return target;
}


// factory
namespace collada
{
	IViewFactory* CreateDefaultTriangulatorPlugin(AccessProviderLocal *providerLocal)
	{
		return new TriangulatorPluginFactory(providerLocal);
	}
	void CreateTriangulatedSceneAdapter(Scene *source, Scene **ppResult, ITriangulatorAlgorithm *triangulatorAlgorithm, AccessProviderLocal *providerLocal, size_t triPluginId)
	{
		assert(0 == source->GetSkinCtrlCount());
		AutoPtr<Scene> result(new TriangulatedSceneAdapter(providerLocal, source, triPluginId));
		DetachRawPtr(result, ppResult);
	}
}

// end of file
