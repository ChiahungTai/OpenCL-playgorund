/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGProperty.h

#pragma once

#include "SceneGraph.h"

using namespace collada;

// forward declaration
struct PropertyListener;

class PropertyImpl : public Property
{
public:
	PropertyImpl(PropertyListener *owner, float *targetArray, size_t size, const char *semantic = NULL);

	// Property interface implementation
	virtual const char* GetSemantic() const;
	virtual size_t GetSize() const;
	virtual void GetValue(float *result) const;
	virtual void SetValue(const float *floatArray);

protected:
	PropertyListener * const m_owner;
	float * const m_targetArray;
	size_t m_size;
	const char *m_semantic;
};

// end of file
