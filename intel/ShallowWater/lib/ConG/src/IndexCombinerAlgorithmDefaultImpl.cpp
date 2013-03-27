/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
        Copyright (C) 2009-2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file IndexCombinerAlgorithmDefaultImpl.cpp

#include "stdafx.h"

#include "IndexCombinerAlgorithmDefaultImpl.h"
#include "AccessProviderDefault.h"


IndexCombinerAlgorithmDefaultImpl::IndexCombinerAlgorithmDefaultImpl(AccessProviderLocal *providerLocal)
  : m_provider(providerLocal)
{
}

void IndexCombinerAlgorithmDefaultImpl::Execute(const AlgorithmResourceAccessor *accessors, unsigned long *vertexOffsets, size_t taskCount, unsigned int vertexStride)
{
	FindAccessor<AccessorReadDefault> inVertexBufferAccessor(accessors, IN_VERTEX_BUFFER);
	FindAccessor<AccessorWriteDefault> outVertexBufferAccessor(accessors, OUT_VERTEX_BUFFER);

	for( unsigned int taskIdx = 0; taskIdx < taskCount; ++taskIdx )
	{
		FindAccessor<AccessorReadDefault> inIndexBufferAccessor(accessors, IN_INDEX_BUFFER, taskIdx);

		MapHelper<const unsigned long, AccessorReadDefault> idxData(inIndexBufferAccessor);
		size_t idxCount = inIndexBufferAccessor->GetMemoryObject()->GetBytesSize() / sizeof(unsigned long);

		MapHelper<const char, AccessorReadDefault> vtxSrc(inVertexBufferAccessor);
		MapHelper<char, AccessorWriteDefault> vtxDst(outVertexBufferAccessor);

		for( size_t idxNew = 0; idxNew != idxCount; ++idxNew )
		{
			memcpy(&vtxDst[(vertexOffsets[taskIdx] + idxNew) * vertexStride], &vtxSrc[idxData[idxNew] * vertexStride], vertexStride);
		}
	}
}

void IndexCombinerAlgorithmDefaultImpl::CreateAccessors(const AlgorithmResource *inputs, IResourceTracker *tracker, Processor *p) const
{
	for( ;inputs->semanticId != -1; ++inputs )
	{
		switch( inputs->semanticId )
		{
		case IN_VERTEX_BUFFER:
		case IN_INDEX_BUFFER:
			{
				AutoPtr<AccessorReadDefault> reader;
				m_provider->CreateAccessorReadDefault(inputs->data, &reader);
				tracker->OnCreateReadAccessor(inputs->semanticId, reader);
			}
			break;
		case OUT_VERTEX_BUFFER:
			{
				AutoPtr<AccessorWriteDefault> writer;
				m_provider->CreateAccessorWriteDefault(inputs->data, &writer, p);
				tracker->OnCreateWriteAccessor(inputs->semanticId, writer);
			}
			break;
		default:
			assert(false);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

// factory
namespace collada
{
	void CreateIndexCombinerAlgorithmDefault(AccessProviderLocal *providerLocal, IIndexCombinerAlgorithm **ppResult)
	{
		AutoPtr<IIndexCombinerAlgorithm> tmp(new IndexCombinerAlgorithmDefaultImpl(providerLocal));
		DetachRawPtr(tmp, ppResult);
	}
}

// end of file
