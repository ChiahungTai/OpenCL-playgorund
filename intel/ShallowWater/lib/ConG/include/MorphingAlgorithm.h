/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file MorphingAlgorithm.h

#pragma once

#include "AlgorithmBase.h"

namespace collada
{

	struct IMorphingAlgorithm : public IAlgorithmSimple
	{
		enum ResourceSemantics
		{
			IN_BASEBUFFER,
			IN_TARGETSBUFFERS,
			IN_WEIGHTS,
			OUT_BUFFER,
		};
	};

	// default CPU-based implementation
	struct AccessProviderLocal;
	void CreateMorphingAlgorithmDefault(AccessProviderLocal *providerLocal, IMorphingAlgorithm **ppResult);

} // end of namespace collada
// end of file
