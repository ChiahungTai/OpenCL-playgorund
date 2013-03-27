/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGCamera.h

#pragma once

#include "mathstub.h"

#include "SceneGraph.h"
#include "SGImplBase.h"


using namespace collada;

///////////////////////////////////////////////////////////////////////////////

class CameraImpl : public BaseImpl<Camera>
{
public:
	// construction
	CameraImpl(Scene *s);

	//
	// Camera interface implementation
	//
	virtual const float* GetProj() const; // float4x4 row major; TODO: move to view class

	virtual ProjectionType GetType() const;
	virtual float GetScreenAspect() const;     //  width : height
	virtual float GetYFov() const;
	virtual float GetXFov() const;
	virtual float GetAspect() const;
	virtual float GetNear() const;
	virtual float GetFar() const;
	virtual FixedValues GetFixedValues() const;

	virtual void SetType(ProjectionType type);
	virtual void SetScreenAspect(float a);     //  width : height
	virtual void SetYFov(float yfov);
	virtual void SetXFov(float xfov);
	virtual void SetAspect(float a);
	virtual void SetNear(float near);
	virtual void SetFar(float zfar);
	virtual void SetFixedValues(FixedValues fv);

	//
	// Base interface
	//

	virtual size_t GetPropertyCount() const;
	virtual Property* GetProperty(size_t idx);

private:
	ProjectionType m_type;
	float m_screenAspect;

	mutable float4x4 m_proj;    // cached projection matrix
	mutable bool m_isValidProj;

	void OnPropertyChange(Property *x) { m_isValidProj = false; }

	// grep camera_properties
	StaticPropertyFloat1<CameraImpl, &OnPropertyChange> m_yfov;
	StaticPropertyFloat1<CameraImpl, &OnPropertyChange> m_xfov;
	StaticPropertyFloat1<CameraImpl, &OnPropertyChange> m_aspect;
	StaticPropertyFloat1<CameraImpl, &OnPropertyChange> m_near;
	StaticPropertyFloat1<CameraImpl, &OnPropertyChange> m_far;

	FixedValues m_fixedValues;
};

// end of file
