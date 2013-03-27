/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGLight.cpp

#include "stdafx.h"

#include "SGLight.h"


///////////////////////////////////////////////////////////////////////////////

LightImpl::LightImpl(Scene *s)
  : BaseImpl(s)
  , m_type(UNDEFINED)
  , m_color(0,0,0,0)
  , m_a0(1)
  , m_a1(0)
  , m_a2(0)
  , m_falloff((float) M_PI)
  , m_exponent(0)
{
}

LightImpl::LightType LightImpl::GetType() const
{
	return m_type;
}

const float* LightImpl::GetColor() const
{
	return m_color;
}

float LightImpl::GetAttenuation0() const
{
	assert(SPOT == m_type || POINT == m_type);
	return m_a0;
}

float LightImpl::GetAttenuation1() const
{
	assert(SPOT == m_type || POINT == m_type);
	return m_a1;
}

float LightImpl::GetAttenuation2() const
{
	assert(SPOT == m_type || POINT == m_type);
	return m_a2;
}

float LightImpl::GetFalloffAngle() const
{
	assert(SPOT == m_type);
	return m_falloff;
}

float LightImpl::GetFalloffExponent() const
{
	assert(SPOT == m_type);
	return m_exponent;
}


void LightImpl::SetType(LightType type)
{
	m_type = type;
}

void LightImpl::SetColor(const float *data, size_t compCount)
{
	assert(compCount <= 4);
	memcpy(&m_color, data, compCount * sizeof(float));
}

void LightImpl::SetAttenuation0(float a0)
{
	assert(SPOT == m_type || POINT == m_type);
	m_a0 = a0;
}

void LightImpl::SetAttenuation1(float a1)
{
	assert(SPOT == m_type || POINT == m_type);
	m_a1 = a1;
}

void LightImpl::SetAttenuation2(float a2)
{
	assert(SPOT == m_type || POINT == m_type);
	m_a2 = a2;
}

void LightImpl::SetFalloffAngle(float angle)
{
	assert(SPOT == m_type);
	m_falloff = angle;

}

void LightImpl::SetFalloffExponent(float e)
{
	assert(SPOT == m_type);
	m_exponent = e;
}


// end of file
