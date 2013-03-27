/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file MorphingAlgorithmDefaultImpl.cpp

#include "stdafx.h"
#include "mathstub.h"
#include "MorphingAlgorithmDefaultImpl.h"
#include "AccessProviderDefault.h"

#include <cassert>
#include <float.h>


MorphingAlgorithmDefaultImpl::MorphingAlgorithmDefaultImpl(AccessProviderLocal *providerLocal)
  : m_providerLocal(providerLocal)
{
}

MorphingAlgorithmDefaultImpl::~MorphingAlgorithmDefaultImpl()
{
}

void MorphingAlgorithmDefaultImpl::DoProcess(const AlgorithmResourceAccessor *accessors) const
{
	MapHelper<const float3, AccessorReadDefault> inBaseBuffer(FindAccessor<AccessorReadDefault>(accessors, IN_BASEBUFFER));
	MapHelper<const float3, AccessorReadDefault> inTargetsBuffers(FindAccessor<AccessorReadDefault>(accessors, IN_TARGETSBUFFERS));
	MapHelper<const float, AccessorReadDefault> inWeights(FindAccessor<AccessorReadDefault>(accessors, IN_WEIGHTS));

	MapHelper<float3, AccessorWriteDefault> outBuffer(FindAccessor<AccessorWriteDefault>(accessors, OUT_BUFFER));

	for (size_t i = 0; i < inBaseBuffer.count; i++ )
	{
		outBuffer[i] = float3(0,0,0);
		float tmpBaseWeight = 1.0f;
		for ( size_t j = 0; j < inWeights.count; j++ )
		{
			outBuffer[i] = outBuffer[i] + inWeights[j] * inTargetsBuffers[j * inBaseBuffer.count + i];
			tmpBaseWeight = tmpBaseWeight - inWeights[j];
		}
		outBuffer[i] = outBuffer[i] + tmpBaseWeight * inBaseBuffer[i];
	}
}

void MorphingAlgorithmDefaultImpl::CreateAccessors(const AlgorithmResource *inputs, IResourceTracker *tracker, Processor *p) const
{
	for( ;inputs->semanticId != -1; ++inputs )
	{
		switch( inputs->semanticId )
		{
		case IN_BASEBUFFER:
		case IN_TARGETSBUFFERS:
		case IN_WEIGHTS:
			if (inputs->data)
			{
				AutoPtr<AccessorReadDefault> reader;
				m_providerLocal->CreateAccessorReadDefault(inputs->data, &reader);
				tracker->OnCreateReadAccessor(inputs->semanticId, reader);
				break;
			}
		case OUT_BUFFER:
			if (inputs->data)
			{
				AutoPtr<AccessorWriteDefault> writer;
				m_providerLocal->CreateAccessorWriteDefault(inputs->data, &writer, p);
				tracker->OnCreateWriteAccessor(inputs->semanticId, writer);
				break;
			}
		default:
			assert(false);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

// factory
namespace collada
{
	void CreateMorphingAlgorithmDefault(AccessProviderLocal *providerLocal, IMorphingAlgorithm **ppResult)
	{
		AutoPtr<IMorphingAlgorithm> tmp(new MorphingAlgorithmDefaultImpl(providerLocal));
		DetachRawPtr(tmp, ppResult);
	}
}

// end of file