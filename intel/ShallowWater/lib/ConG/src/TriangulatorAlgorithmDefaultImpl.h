/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file TriangulatorAlgorithmDefaultImpl.h

#include "TriangulatorAlgorithm.h"
#include "RefCountedImpl.h"

using namespace collada;

namespace
{
	class TriangulatorAlgorithmDefaultImpl : public RefCountedImpl<ITriangulatorAlgorithm>
	{
	public:
		TriangulatorAlgorithmDefaultImpl(AccessProviderLocal *providerLocal);
		virtual ~TriangulatorAlgorithmDefaultImpl();

		// ITriangulatorAlgorithm
		virtual void Triangulate(const AlgorithmResourceAccessor *accessors);

		// IAlgorithm
		virtual void CreateAccessors(const AlgorithmResource *inputs, IResourceTracker *tracker, Processor *p) const;

	private:
		AutoPtr<AccessProviderLocal> m_provider;
	};

}

// end of file
