/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SkinningAlgorithmDefaultImpl.h

#pragma once

#include "SkinningAlgorithm.h"
#include "AccessProviderDefault.h"
#include "RefCountedImpl.h"
using namespace collada;

namespace
{
	class SkinningAlgorithmDefaultImpl : public RefCountedImpl<ISkinningAlgorithm>
	{
	public:
		SkinningAlgorithmDefaultImpl(AccessProviderLocal *providerLocal);
		virtual ~SkinningAlgorithmDefaultImpl();

		// ISkinningAlgorithm
		virtual void ComputeJoints(const AlgorithmResourceAccessor *accessors);
		virtual void ComputeSkinningMatrices(const AlgorithmResourceAccessor *accessors, const float* bindShape4x4);
		virtual void TransformPositions(const AlgorithmResourceAccessor *accessors);
		virtual void TransformNormals(const AlgorithmResourceAccessor *accessors);

		// IAlgorithm
		virtual void CreateAccessors(const AlgorithmResource *inputs, IResourceTracker *tracker, Processor *p) const;

	private:
		AutoPtr<AccessProviderLocal> m_provider;
	};
}


// end of file
