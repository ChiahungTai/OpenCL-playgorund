/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file AABBCalcAlgorithmDefaultImpl.h

#pragma once

#include "AABBCalcAlgorithm.h"
#include "RefCountedImpl.h"

using namespace collada;

namespace
{
	class IAABBCalcAlgorithmDefaultImpl : public RefCountedImpl<IAABBCalcAlgorithm>
	{
	public:
		IAABBCalcAlgorithmDefaultImpl(AccessProviderLocal *providerLocal);
		virtual ~IAABBCalcAlgorithmDefaultImpl();

		// IAlgorithmSimple
		virtual void DoProcess(const AlgorithmResourceAccessor *accessors) const;

		// IAlgorithm
		virtual void CreateAccessors(const AlgorithmResource *inputs, IResourceTracker *tracker, Processor *p) const;

	private:
		AutoPtr<AccessProviderLocal> m_provider;
	};
}
// end of file