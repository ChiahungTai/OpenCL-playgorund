/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SkinningPlugin.h

#pragma once

#include "SceneGraphBase.h"

namespace collada
{

struct ISkinnedMeshInstanceView : public collada::IView
{
	virtual collada::MeshInstance* GetMeshInstance() = 0;
	virtual collada::MemoryObject* GetSkinningMatrices() = 0; // array of float4x4
};

///////////////////////////////////////////////////////////////////////////////
// default implementation

struct ISkinningAlgorithm;
struct AccessProviderLocal;

IViewFactory* CreateDefaultSkinningPlugin(ISkinningAlgorithm *algorithm, AccessProviderLocal *providerLocal);

///////////////////////////////////////////////////////////////////////////////
// box adapter

void CreateSkinningSceneAdapter(Scene *source, Scene **ppResult, ISkinningAlgorithm *algorithm, AccessProviderLocal *providerLocal, size_t skinningPluginId = -1);


///////////////////////////////////////////////////////////////////////////////
}; // end of namespace collada


// end of file
