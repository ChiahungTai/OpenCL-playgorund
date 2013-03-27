/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGLight.h

#pragma once

#include "mathstub.h"

#include "SceneGraph.h"
#include "SGImplBase.h"


using namespace collada;

///////////////////////////////////////////////////////////////////////////////

class LightImpl : public BaseImpl<Light>
{
public:
	LightImpl(Scene *s);

	// Light interface implementation
	virtual LightType GetType() const;
	virtual const float* GetColor() const;
	virtual float GetAttenuation0() const; // constant
	virtual float GetAttenuation1() const; // linear
	virtual float GetAttenuation2() const; // quadratic
	virtual float GetFalloffAngle() const;
	virtual float GetFalloffExponent() const;

	virtual void SetColor(const float *data, size_t compCount);
	virtual void SetType(LightType type);
	virtual void SetAttenuation0(float a0); // constant
	virtual void SetAttenuation1(float a1); // linear
	virtual void SetAttenuation2(float a2); // quadratic
	virtual void SetFalloffAngle(float angle);
	virtual void SetFalloffExponent(float e);

private:
	LightType m_type;
	float4 m_color;
	float m_a0;
	float m_a1;
	float m_a2;
	float m_falloff;
	float m_exponent;
};

// end of file
