/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGProperty.cpp

#include "stdafx.h"

#include "SGProperty.h"
#include "SGImplBase.h"

///////////////////////////////////////////////////////////////////////////////

PropertyImpl::PropertyImpl(PropertyListener *owner, float *targetArray, size_t size, const char *semantic)
  : m_owner(owner)
  , m_targetArray(targetArray)
  , m_size(size)
  , m_semantic(semantic)
{
}

const char* PropertyImpl::GetSemantic() const
{
	return m_semantic;
}

size_t PropertyImpl::GetSize() const
{
	return m_size;
}

void PropertyImpl::SetValue(const float *floatArray)
{
	memcpy(m_targetArray, floatArray, sizeof(float) * m_size);
	m_owner->OnPropertyChange();
}

void PropertyImpl::GetValue(float *result) const
{
	memcpy(result, m_targetArray, sizeof(float) * m_size);
}


// end of file
