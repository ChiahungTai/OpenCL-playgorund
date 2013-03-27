/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file MorphingAlgorithmD3D10.h

#pragma once

#include "MorphingAlgorithm.h"

namespace collada
{
	struct AccessProviderD3D10;
	struct IMessageListener;
	void CreateMorphingAlgorithmD3D10(AccessProviderD3D10 *providerD3D10, AccessProviderLocal *providerLocal, IMessageListener *msg, IMorphingAlgorithm **ppResult);
}

// end of file
