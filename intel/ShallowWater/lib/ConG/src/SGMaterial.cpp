/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGMaterial.cpp

#include "stdafx.h"

#include "SGMaterial.h"


///////////////////////////////////////////////////////////////////////////////

MaterialImpl::MaterialImpl(Scene *s)
  : BaseImpl(s)
  , m_effect(NULL)
{
}

void MaterialImpl::SetEffect(Effect *effect)
{
	m_effect = effect;
}

// end of file
