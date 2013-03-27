/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file NodeControlPlugin.h

#pragma once

#include "SceneGraph.h"

namespace collada
{

/// Applied to Node
struct INodeControlView : public IView
{
	virtual void Reset() = 0;
	virtual void TranslateDelta(float x, float y, float z) = 0;
	virtual void RotateDeltaFP(float yaw, float pitch, float roll) = 0;
	virtual void RotateDelta(float yaw, float pitch, float roll) = 0;
	virtual void RemoveRoll(const float *pCOI) = 0; ///< @arg pCOI Pointer to vector of 3 floats.
};

IViewFactory* CreateDefaultNodeControlPlugin();


} // end of namespace collada

// end of file
