/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file TriSortPlugin.h

#pragma once

#include "SceneGraphBase.h"
#include "RefCounted.h"

namespace collada
{
///////////////////////////////////////////////////////////////////////////////

struct ISortedMeshInstanceView : public collada::IView
{
	virtual collada::MeshInstance* GetMeshInstance() = 0;
};

struct ITriSortViewFactory : public IViewFactory
{
	virtual void SetView(const float *mView) = 0; // float4x4
};

///////////////////////////////////////////////////////////////////////////////
// default implementation

ITriSortViewFactory* CreateDefaultSortingPlugin();

///////////////////////////////////////////////////////////////////////////////
// box adapter

struct ISceneSorter : public RefCounted
{
	virtual Scene* GetSortedScene() = 0;
	virtual void SetView(const float *mView) = 0; // float4x4
};

void CreateSceneSorter(Scene *source, ISceneSorter **ppResult, size_t sortPluginId = -1);

///////////////////////////////////////////////////////////////////////////////
}; // end of namespace collada


// end of file
