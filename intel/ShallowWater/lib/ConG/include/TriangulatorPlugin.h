/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
        Copyright (C) 2008-2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file TriangulatorPlugin.h

#pragma once

#include "SceneGraphBase.h"

namespace collada
{
	// forward declaration
	struct AccessProviderLocal;
	struct ITriangulatorAlgorithm;

///////////////////////////////////////////////////////////////////////////////

struct ITriangleBatchView : public collada::IView
{
	virtual collada::Batch* GetTriangleBatch() = 0;
};

///////////////////////////////////////////////////////////////////////////////
// default implementation

IViewFactory* CreateDefaultTriangulatorPlugin(AccessProviderLocal *providerLocal);

///////////////////////////////////////////////////////////////////////////////
// box adapter

void CreateTriangulatedSceneAdapter(Scene *source, Scene **ppResult, ITriangulatorAlgorithm *triangulatorAlgorithm, AccessProviderLocal *providerLocal, size_t triPluginId = -1);

///////////////////////////////////////////////////////////////////////////////
}; // end of namespace collada


// end of file
