/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file ColladaDataFlow.h

#pragma once

#include "ColladaUtils.h"


// forward declarations
class domSource;
class domSampler;

///////////////////////////////////////////////////////////////////////////////

class SourceImpl
{
public:
	enum Type
	{
		VT_UNDEFINED,
		VT_BOOL,
		VT_FLOAT,
		VT_INT,
		VT_NAME,   // general purpose strings
		VT_IDREF,  // element id's within document
	};

	SourceImpl(const domSource *src, void*);
	Type GetType() const;

	unsigned int GetElementCount() const;
	unsigned int GetElementSize() const; // in sub elements

	const float* GetFloatArray() const;
	const char* GetName(size_t rawIdx) const;
	daeElement* GetRef(size_t rawIdx) const;

	template <class T>
	T* GetRefT(size_t rawIdx) const
	{
		T *result = daeSafeCast<T>(GetRef(rawIdx));
		if( !result )
		{
			std::ostringstream buf;
			buf << "IDREF element '" << GetRef(rawIdx)->getID() << "' has incompatible type";
			buf << " (" << typeid(T).name() << " expected, got " << typeid(*GetRef(rawIdx)).name() << ")";
			throw std::runtime_error(buf.str());
		}
		return result;
	}

	const char* GetSourceTag() const { return m_sourceTag; }

private:
	const char *m_sourceTag;
	Type m_type;
	unsigned int m_elementCount;
	unsigned int m_elementSize;
	std::vector<char> m_rawData;
	std::vector<size_t> m_nameIndex;
};

typedef ResourceCache<const domSource, SourceImpl> SourceCache;

///////////////////////////////////////////////////////////////////////////////

class SamplerInfo
{
	SourceImpl *m_interpolation;
	SourceImpl *m_input;
	SourceImpl *m_output;
	SourceImpl *m_inT;
	SourceImpl *m_outT;
	const char *m_samplerTag;

public:
	SamplerInfo(const domSampler *src, SourceCache *sourceCache);

	SourceImpl* GetInterpolation() const { return m_interpolation; }
	SourceImpl* GetInput() const { return m_input; }
	SourceImpl* GetOutput() const { return m_output; }
	SourceImpl* GetInT() const { return m_inT; }
	SourceImpl* GetOutT() const { return m_outT; }

	const char* GetSamplerTag() const { return m_samplerTag; }
};

typedef ResourceCache<const domSampler, SamplerInfo, SourceCache> SamplerCache;

///////////////////////////////////////////////////////////////////////////////

class AnimationTargetParser
{
	std::string m_target;   // storage for full target string specification

	const char *m_targetId;  // always points to the beginning of the m_target storage
	const char *m_targetSid;   // points to the middle of the m_target storage; optional. TODO: support for several sids
	const char *m_targetParam; // points to the middle of the m_target storage; optional

public:
	AnimationTargetParser(const domChannel *src);

	const char* GetTargetId() const;
	const char* GetTargetSid() const;      // TODO: support several sids
	const char* GetTargetParam() const;
};

// end of file
