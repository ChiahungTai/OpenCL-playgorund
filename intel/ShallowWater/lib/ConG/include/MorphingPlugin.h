/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

// MorphingPlugin.h


#pragma once

#include "SceneGraphBase.h"

namespace collada
{
	// forward declaration
	struct AccessProviderLocal;
	struct IMorphingAlgorithm;

///////////////////////////////////////////////////////////////////////////////

struct IMorphedMeshView : public collada::IView
{
	virtual collada::Mesh* GetMorphedMesh() = 0;
};

struct IMorphedMeshInstanceView : public collada::IView
{
	virtual collada::MeshInstance* GetMeshInstance() = 0;
};

///////////////////////////////////////////////////////////////////////////////
// default implementation

IViewFactory* CreateMorphPlugin(IMorphingAlgorithm *algorithm, AccessProviderLocal *providerLocal);

///////////////////////////////////////////////////////////////////////////////
// box adapter

void CreateMorphingSceneAdapter(Scene *source, Scene **ppResult, IMorphingAlgorithm *algorithm, AccessProviderLocal *providerLocal, size_t pluginId = -1);

///////////////////////////////////////////////////////////////////////////////
}; // end of namespace collada



