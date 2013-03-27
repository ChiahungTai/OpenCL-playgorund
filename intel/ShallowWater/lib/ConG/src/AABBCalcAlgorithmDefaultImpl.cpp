/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file AABBCalcAlgorithmDefaultImpl.cpp

#include "stdafx.h"

#include "mathstub.h"

#include "AABBCalcAlgorithmDefaultImpl.h"
#include "AccessProviderDefault.h"

#include <cassert>
#include <float.h>


IAABBCalcAlgorithmDefaultImpl::IAABBCalcAlgorithmDefaultImpl(AccessProviderLocal *providerLocal)
  : m_provider(providerLocal)
{
}

IAABBCalcAlgorithmDefaultImpl::~IAABBCalcAlgorithmDefaultImpl()
{
}

void IAABBCalcAlgorithmDefaultImpl::DoProcess(const AlgorithmResourceAccessor *accessors) const
{
	MapHelper<const float3, AccessorReadDefault> inPositions(FindAccessor<AccessorReadDefault>(accessors, IN_POSITIONS));
	MapHelper<AABBType, AccessorWriteDefault> outAABB(FindAccessor<AccessorWriteDefault>(accessors, OUT_AABB));
	float3 min(FLT_MAX,FLT_MAX,FLT_MAX);
	float3 max(-FLT_MAX,-FLT_MAX,-FLT_MAX);
	for( size_t i = 0; i < inPositions.count; ++i )
	{
		Vec3Maximize(&max, &max, &inPositions[i]);
		Vec3Minimize(&min, &min, &inPositions[i]);
	}
	reinterpret_cast<float3&>(outAABB[0].aabb) = max - min;
	reinterpret_cast<float3&>(outAABB[0].center) = (max + min) / 2;
}

void IAABBCalcAlgorithmDefaultImpl::CreateAccessors(const AlgorithmResource *inputs, IResourceTracker *tracker, Processor *p) const
{
	for( ;inputs->semanticId != -1; ++inputs )
	{
		switch( inputs->semanticId )
		{
		case IN_POSITIONS:
			{
				AutoPtr<AccessorReadDefault> reader;
				m_provider->CreateAccessorReadDefault(inputs->data, &reader);
				tracker->OnCreateReadAccessor(inputs->semanticId, reader);
				break;
			}
		case OUT_AABB:
			{
				AutoPtr<AccessorWriteDefault> writer;
				m_provider->CreateAccessorWriteDefault(inputs->data, &writer, p);
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
	void CreateAABBCalcAlgorithmDefault(AccessProviderLocal *providerLocal, IAABBCalcAlgorithm **ppResult)
	{
		AutoPtr<IAABBCalcAlgorithm> tmp(new IAABBCalcAlgorithmDefaultImpl(providerLocal));
		DetachRawPtr(tmp, ppResult);
	}
}

// end of file
