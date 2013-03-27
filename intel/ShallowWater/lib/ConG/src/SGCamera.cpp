/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGCamera.cpp

#include "stdafx.h"

#include "SGCamera.h"


#pragma warning (disable: 4355) // 'this' : used in base member initializer list

///////////////////////////////////////////////////////////////////////////////


CameraImpl::CameraImpl(Scene *s)
  : BaseImpl(s)
  , m_type(UNDEFINED)
  , m_yfov(this, (float) M_PI_4, "yfov")
  , m_xfov(this, (float) M_PI_4, "xfov")
  , m_aspect(this, 1.0f, "aspect")
  , m_fixedValues(FIXED_XFOV_ASPECT)
  , m_near(this, 1.0f, "znear")
  , m_far(this, 1000.0f, "zfar")
  , m_screenAspect(1.0f)
  , m_isValidProj(false)
{
}

Camera::ProjectionType CameraImpl::GetType() const
{
	return m_type;
}

const float* CameraImpl::GetProj() const
{
	if( !m_isValidProj )
	{
		// COLLADA – Digital Asset Schema Release 1.4.1, 2nd Edition [5-20]
	
		switch( m_fixedValues )
		{
		case FIXED_YFOV_ASPECT:
			MatrixPerspectiveFovLH(&m_proj, m_yfov.GetFloat(),
				m_screenAspect * m_aspect.GetFloat(), m_near.GetFloat(), m_far.GetFloat());
			break;
		case FIXED_XFOV_ASPECT:
			MatrixPerspectiveFovLH(&m_proj, atanf(tanf(m_xfov.GetFloat() / 2) / m_screenAspect) * 2,
				m_screenAspect * m_aspect.GetFloat(), m_near.GetFloat(), m_far.GetFloat());
			break;
		case FIXED_XFOV_YFOV:
			MatrixPerspectiveFovLH(&m_proj, m_yfov.GetFloat(),
				tanf(m_yfov.GetFloat() / 2) / tanf(m_xfov.GetFloat() / 2), m_near.GetFloat(), m_far.GetFloat());
			break;
		}

		m_isValidProj = true;
	}
	return m_proj;
}

void CameraImpl::SetScreenAspect(float a)
{
	if( m_screenAspect != a )
	{
		m_screenAspect = a;
		m_isValidProj = false;
	}
}

float CameraImpl::GetScreenAspect() const
{
	return m_screenAspect;
}

float CameraImpl::GetYFov() const
{
	return m_yfov.GetFloat();
}

float CameraImpl::GetXFov() const
{
	return m_xfov.GetFloat();
}

float CameraImpl::GetAspect() const
{
	return m_aspect.GetFloat();
}

float CameraImpl::GetNear() const
{
	return m_near.GetFloat();
}

float CameraImpl::GetFar() const
{
	return m_far.GetFloat();
}

CameraImpl::FixedValues CameraImpl::GetFixedValues() const
{
	return m_fixedValues;
}

void CameraImpl::SetType(ProjectionType type)
{
	m_type = type;
}

void CameraImpl::SetYFov(float yfov)
{
	m_yfov.SetValue(&yfov);
}

void CameraImpl::SetXFov(float xfov)
{
	m_xfov.SetValue(&xfov);
}

void CameraImpl::SetAspect(float a)
{
	m_aspect.SetValue(&a);
}

void CameraImpl::SetNear(float znear)
{
	m_near.SetValue(&znear);
}

void CameraImpl::SetFar(float zfar)
{
	m_far.SetValue(&zfar);
}

void CameraImpl::SetFixedValues(FixedValues fv)
{
	m_fixedValues = fv;
}

size_t CameraImpl::GetPropertyCount() const
{
	// grep camera_properties
	return 5;
}

Property* CameraImpl::GetProperty(size_t idx)
{
	// grep camera_properties
	assert(idx < GetPropertyCount());
	return &m_yfov + idx;
}

///////////////////////////////////////////////////////////////////////////////
// end of file
