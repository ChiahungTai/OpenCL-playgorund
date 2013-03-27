/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
        Copyright (C) 2009-2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file IndexCombinerAlgorithmDefaultImpl.h

#pragma once

#include "IndexCombinerAlgorithm.h"
#include "AccessProviderDefault.h"
#include "RefCountedImpl.h"

using namespace collada;

namespace
{
	class IndexCombinerAlgorithmDefaultImpl : public RefCountedImpl<IIndexCombinerAlgorithm>
	{
	public:
		IndexCombinerAlgorithmDefaultImpl(AccessProviderLocal *providerLocal);

		// IIndexCombinerAlgorithm
		virtual void Execute(const AlgorithmResourceAccessor *accessors, unsigned long *vertexOffsets, size_t taskCount, unsigned int vertexStride);

		// IAlgorithm
		virtual void CreateAccessors(const AlgorithmResource *inputs, IResourceTracker *tracker, Processor *p) const;

	private:
		AutoPtr<AccessProviderLocal> m_provider;
	};
}

// end of file
