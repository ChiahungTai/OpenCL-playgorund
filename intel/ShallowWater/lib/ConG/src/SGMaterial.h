/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGMaterial.h

#pragma once

#include "SceneGraph.h"
#include "SGImplBase.h"

using namespace collada;

///////////////////////////////////////////////////////////////////////////////

class MaterialImpl : public BaseImpl<Material>
{
public:
	// construction
	MaterialImpl(Scene *s);

	// Material interface
	virtual Effect* GetEffect(Scene *context) const { return SafeResolveContext<Effect>(context, m_effect); }
	virtual void SetEffect(Effect *effect);

private:
	Effect *m_effect;
};

// end of file
