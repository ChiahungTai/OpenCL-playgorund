/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGUtils.h

#pragma once

#include "SceneGraphBase.h"
#include <cassert>
#include <vector>

// array of pointers; it calls delete on each stored pointer when the array is being destroyed
template <class T>
class PtrOwnerArray : public std::vector<T *>
{
public:
	PtrOwnerArray() {}
	~PtrOwnerArray()
	{
		for( iterator it = begin(); it != end(); ++it )
		{
			delete *it;
		}
	}
};

///////////////////////////////////////////////////////////////////////////////

inline collada::ScalarType IdxTypeFromSize(size_t size)
{
	const collada::ScalarType type[4] =
	{
		collada::ST_UINT16,
		collada::ST_UNKNOWN,
		collada::ST_UINT32,
	};
	assert(2 == size || 4 == size);
	return type[size - 2];
}

///////////////////////////////////////////////////////////////////////////////

// you should always specify template argument as required return type
template <class T>
inline T* SafeResolveContext(collada::Scene *context, T *target)
{
	if( context && target )
	{
		collada::Base *result = context->ResolveContext(target);
		assert(dynamic_cast<T*>(result));
		return static_cast<T*>(result);
	}
	return target;
}

// end of file
