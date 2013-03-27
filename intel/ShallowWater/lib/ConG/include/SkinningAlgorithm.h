/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SkinningAlgorithm.h

#pragma once
#include "mathstub.h"
#include "AlgorithmBase.h"

namespace collada
{

struct AccessProviderLocal;

struct ISkinningAlgorithm : public IAlgorithm
{
	// TODO: handle tangents and binormals
	enum ResourceSemantics
	{
		IN_POSITIONS,
		IN_NORMALS,
		IN_TRANSFORMS,
		IN_REINDEX_TRANSFORMS,
		IN_REINDEX_NORMALS,
		IN_JOINT_MATRICES,
		IN_JOINTS,
		IN_WEIGHTS_BINDINGS,
		IN_WEIGHTED_JOINTS,
		OUT_POSITIONS,
		OUT_NORMALS,
		OUT_TRANSFORMS,
		OUT_JOINT_MATRICES,

		IN_TMP_TRANSFORMS_SERIALIZED,   // FIXME: hide from here
	};

	virtual void ComputeJoints(const AlgorithmResourceAccessor *accessors) = 0;
	virtual void ComputeSkinningMatrices(const AlgorithmResourceAccessor *accessors, const float* bindShape4x4) = 0;
	virtual void TransformPositions(const AlgorithmResourceAccessor *accessors) = 0;
	virtual void TransformNormals(const AlgorithmResourceAccessor *accessors) = 0;
};

// default CPU-based implementation
void CreateSkinningAlgorithmDefault(AccessProviderLocal *providerLocal, ISkinningAlgorithm **ppResult);
void CreateSkinningAlgorithmSSE(AccessProviderLocal *providerLocal, ISkinningAlgorithm **ppResult);

}

// end of file
