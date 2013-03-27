/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SkinningAlgorithmDefaultImpl.cpp

#include "stdafx.h"

#include "mathstub.h"

#include "SkinningAlgorithmDefaultImpl.h"
#include "AccessProviderDefault.h"
#include "SceneGraph.h"


SkinningAlgorithmDefaultImpl::SkinningAlgorithmDefaultImpl(AccessProviderLocal *providerLocal)
  : m_provider(providerLocal)
{
}

SkinningAlgorithmDefaultImpl::~SkinningAlgorithmDefaultImpl()
{
}

void SkinningAlgorithmDefaultImpl::ComputeJoints(const AlgorithmResourceAccessor *accessors)
{
	MapHelper<const Joint, AccessorReadDefault> joints(FindAccessor<AccessorReadDefault>(accessors, IN_JOINTS));
	MapHelper<float4x4, AccessorWriteDefault> jointMatrices(FindAccessor<AccessorWriteDefault>(accessors, OUT_JOINT_MATRICES));
	for( size_t i = 0; i < jointMatrices.count; ++i )
	{
		jointMatrices[i] = *(const float4x4 *) joints[i].invBindMatrix * *(const float4x4 *) joints[i].node->GetWorld(NULL);
	}
}

void SkinningAlgorithmDefaultImpl::ComputeSkinningMatrices(const AlgorithmResourceAccessor *accessors, const float* bindShape4x4)
{
	FindAccessor<AccessorReadDefault> inJointsAccessor(accessors, IN_JOINT_MATRICES);
	FindAccessor<AccessorReadDefault> inWeightsBindingsAccessor(accessors, IN_WEIGHTS_BINDINGS);
	FindAccessor<AccessorReadDefault> inWeightedJointsAccessor(accessors, IN_WEIGHTED_JOINTS);
	FindAccessor<AccessorWriteDefault> outAccessor(accessors, OUT_TRANSFORMS);

	MapHelper<float4x4, AccessorWriteDefault> skinMatrices(outAccessor);
	MapHelper<const float4x4, AccessorReadDefault> joints(inJointsAccessor);
	MapHelper<const VertexWeightsBinding, AccessorReadDefault> wb(inWeightsBindingsAccessor);
	MapHelper<const WeightedJoint, AccessorReadDefault> weightedJoints(inWeightedJointsAccessor);
	for( size_t i = 0; i < skinMatrices.count; ++i )
	{
		float4x4 &skinMatrix = skinMatrices[i];
		if( wb[i].begin < wb[i].end )
		{
			{
				const WeightedJoint &wj = weightedJoints[wb[i].begin];
				skinMatrix = joints[wj.jointIdx] * wj.weight;
			}
			for( size_t j = wb[i].begin+1; j < wb[i].end; ++j )
			{
				const WeightedJoint &wj = weightedJoints[j];
				skinMatrix += joints[wj.jointIdx] * wj.weight;
			}
			MatrixMultiply(&skinMatrix, reinterpret_cast<const float4x4 *>(bindShape4x4), &skinMatrix);
		}
		else
		{
			skinMatrix = *reinterpret_cast<const float4x4 *>(bindShape4x4);
		}
	}
}

void SkinningAlgorithmDefaultImpl::TransformPositions(const AlgorithmResourceAccessor *accessors)
{
	FindAccessor<AccessorReadDefault> inPositionsAccessor(accessors, IN_POSITIONS);
	FindAccessor<AccessorReadDefault> inMatricesAccessor(accessors, IN_TRANSFORMS);
	FindAccessor<AccessorWriteDefault> outPositionsAccessor(accessors, OUT_POSITIONS);

	assert(inPositionsAccessor->GetMemoryObject()->GetBytesSize() == outPositionsAccessor->GetMemoryObject()->GetBytesSize());
	assert(inPositionsAccessor->GetMemoryObject()->GetBytesSize() % sizeof(float3) == 0);
	assert(inMatricesAccessor->GetMemoryObject()->GetBytesSize() % sizeof(float4x4) == 0);

	MapHelper<const float4x4, AccessorReadDefault> matrices(inMatricesAccessor);
	MapHelper<const float3, AccessorReadDefault> posSrc(inPositionsAccessor);
	MapHelper<float3, AccessorWriteDefault> posDst(outPositionsAccessor);

	size_t vcount = outPositionsAccessor->GetMemoryObject()->GetBytesSize() / sizeof(float3);
	for( size_t i = 0; i < vcount; ++i )
	{
		Vec3TransformCoord(&posDst[i], &posSrc[i], &matrices[i]);
	}
}

void SkinningAlgorithmDefaultImpl::TransformNormals(const AlgorithmResourceAccessor *accessors)
{
	FindAccessor<AccessorReadDefault> inNormalsAccessor(accessors, IN_NORMALS);
	FindAccessor<AccessorReadDefault> inMatricesAccessor(accessors, IN_TRANSFORMS);
	FindAccessor<AccessorWriteDefault> outNormalsAccessor(accessors, OUT_NORMALS);

	MapHelper<const float4x4, AccessorReadDefault> matrices(inMatricesAccessor);
	MapHelper<const float3, AccessorReadDefault> normSrc(inNormalsAccessor);
	MapHelper<float3, AccessorWriteDefault> normDst(outNormalsAccessor);

	if( const AlgorithmResourceAccessor *resN = FindResourceById(accessors, IN_REINDEX_NORMALS) )
	{
		const AlgorithmResourceAccessor *resT = FindResourceById(accessors, IN_REINDEX_TRANSFORMS);
		assert(resT);
		MapHelper<const unsigned long, AccessorReadDefault> reindexN(static_cast<AccessorReadDefault*>(resN->accessor));
		MapHelper<const unsigned long, AccessorReadDefault> reindexT(static_cast<AccessorReadDefault*>(resT->accessor));
		assert(normDst.count == reindexT.count && normDst.count == reindexN.count);
		for( unsigned int i = 0; i < normDst.count; ++i )
		{
			Vec3TransformNormal(&normDst[i], &normSrc[reindexN[i]], &matrices[reindexT[i]]);
		}
	}
	else
	{
		// direct mapping
		assert(normDst.count == normSrc.count && normDst.count == matrices.count);
		for( size_t i = 0; i < normDst.count; ++i )
		{
			Vec3TransformNormal(&normDst[i], &normSrc[i], &matrices[i]);
		}
	}
}

void SkinningAlgorithmDefaultImpl::CreateAccessors(const AlgorithmResource *inputs, IResourceTracker *tracker, Processor *p) const
{
	for( ;inputs->semanticId != -1; ++inputs )
	{
		switch( inputs->semanticId )
		{
		case IN_POSITIONS:
		case IN_NORMALS:
		case IN_TRANSFORMS:
		case IN_JOINTS:
		case IN_JOINT_MATRICES:
		case IN_WEIGHTED_JOINTS:
		case IN_WEIGHTS_BINDINGS:
			assert(inputs->data);
		case IN_REINDEX_NORMALS:     // reindex inputs are optional
		case IN_REINDEX_TRANSFORMS:
			if( inputs->data )
			{
				AutoPtr<AccessorReadDefault> reader;
				m_provider->CreateAccessorReadDefault(inputs->data, &reader);
				tracker->OnCreateReadAccessor(inputs->semanticId, reader);
			}
			break;
		case OUT_POSITIONS:
		case OUT_NORMALS:
		case OUT_TRANSFORMS:
		case OUT_JOINT_MATRICES:
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
	void CreateSkinningAlgorithmDefault(AccessProviderLocal *providerLocal, ISkinningAlgorithm **ppResult)
	{
		AutoPtr<ISkinningAlgorithm> tmp(new SkinningAlgorithmDefaultImpl(providerLocal));
		DetachRawPtr(tmp, ppResult);
	}
}

// end of file
