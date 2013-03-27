/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file TriangulatorAlgorithm.h

#pragma once

#include "AlgorithmBase.h"

namespace collada
{
	struct AccessProviderLocal;

	struct ITriangulatorAlgorithm : public IAlgorithm
	{
		enum ResourceSemantics
		{
			IN_INDICES,
			IN_COUNTS,
			OUT_INDICES,
		};

		virtual void Triangulate(const AlgorithmResourceAccessor *accessors) = 0;
	};

	// default CPU-based implementation
	void CreateTriangulatorAlgorithmDefault(AccessProviderLocal *providerLocal, ITriangulatorAlgorithm **ppResult);

} // end of namespace collada
// end of file
