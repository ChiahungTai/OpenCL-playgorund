/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file AABBCalcAlgorithm.h

#pragma once

#include "AlgorithmBase.h"

namespace collada
{

	struct IAABBCalcAlgorithm : public IAlgorithmSimple
	{
		enum ResourceSemantics
		{
			IN_POSITIONS,
			OUT_AABB,
		};
		struct AABBType
		{
			float center[3]; // bounding box center position in local space
			float aabb[3];  // bounding box size in local space
		};
	};

	// default CPU-based implementation
	struct AccessProviderLocal;
	void CreateAABBCalcAlgorithmDefault(AccessProviderLocal *providerLocal, IAABBCalcAlgorithm **ppResult);

} // end of namespace collada
// end of file
