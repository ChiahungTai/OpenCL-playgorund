/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file IndexCombinerPlugin.h

#pragma once

#include "SceneGraph.h"

namespace collada
{
	struct IIndexCombinerAlgorithm;
	struct AccessProviderLocal;

///////////////////////////////////////////////////////////////////////////////
// batch wrapper

struct IUnifiedBatchView : public IView
{
	virtual Batch* GetUnifiedBatch() = 0;
};

///////////////////////////////////////////////////////////////////////////////
// mesh wrapper

struct IUnifiedMeshView : public IView
{
	virtual Mesh* GetUnifiedMesh() = 0;
};

///////////////////////////////////////////////////////////////////////////////
// default implementation

IViewFactory* CreateDefaultIndexCombinerPlugin(IIndexCombinerAlgorithm *algorithm, AccessProviderLocal *providerLocal);

///////////////////////////////////////////////////////////////////////////////
// box adapter

void CreateUnifiedSceneAdapter(Scene *source, Scene **ppResult, IIndexCombinerAlgorithm *algorithm, AccessProviderLocal *providerLocal, size_t combinerPluginId = -1);

} // end of namespace collada

// end of file
