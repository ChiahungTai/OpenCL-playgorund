/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file MorphingAlgorithmDefaultImpl.h

#include "MorphingAlgorithm.h"
#include "RefCountedImpl.h"

using namespace collada;

namespace
{
	class MorphingAlgorithmDefaultImpl : public RefCountedImpl<IMorphingAlgorithm>
	{
	public:
		MorphingAlgorithmDefaultImpl(AccessProviderLocal *providerLocal);
		virtual ~MorphingAlgorithmDefaultImpl();

		// IAlgorithmSimple
		virtual void DoProcess(const AlgorithmResourceAccessor *accessors) const;

		// IAlgorithm
		virtual void CreateAccessors(const AlgorithmResource *inputs, IResourceTracker *tracker, Processor *p) const;

	private:
		AutoPtr<AccessProviderLocal> m_providerLocal;
	};
} // end of unnamed namespace

// end of file

