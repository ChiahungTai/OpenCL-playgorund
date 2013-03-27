// ColladaUtilsMath.h

#pragma once

#include "mathstub.h"
#include <dom/domCOLLADA.h>


static inline float3 Vector3FromDomVector(const domListOfFloats &src)
{
	assert(src.getCount() >= 3);
	return float3((float) src[0], (float) src[1], (float) src[2]);
}

static inline float4 Vector4FromDomVector(const domListOfFloats &src)
{
	assert(src.getCount() >= 4);
	return float4((float) src[0], (float) src[1], (float) src[2], (float) src[3]);
}

template<class Array>
static inline float4x4 Matrix4FromDomFloatArray(const Array &src)
{
	return float4x4((float) src[0x0], (float) src[0x4], (float) src[0x8], (float) src[0xC],
	                (float) src[0x1], (float) src[0x5], (float) src[0x9], (float) src[0xD],
	                (float) src[0x2], (float) src[0x6], (float) src[0xA], (float) src[0xE],
	                (float) src[0x3], (float) src[0x7], (float) src[0xB], (float) src[0xF]);
}


// end of file
