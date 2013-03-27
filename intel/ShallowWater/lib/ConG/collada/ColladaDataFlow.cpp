/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file ColladaDataFlow.cpp

#include "ColladaDataFlow.h"

///////////////////////////////////////////////////////////////////////////////

SamplerInfo::SamplerInfo(const domSampler *src, SourceCache *sourceCache)
  : m_interpolation(NULL)
  , m_input(NULL)
  , m_output(NULL)
  , m_inT(NULL)
  , m_outT(NULL)
  , m_samplerTag(src->getID())
{
	std::map<std::string, SourceImpl **> table;
	table["INPUT"] = &m_input;
	table["OUTPUT"] = &m_output;
	table["INTERPOLATION"] = &m_interpolation;
	table["IN_TANGENT"] = &m_inT;
	table["OUT_TANGENT"] = &m_outT;

	for( size_t inputIdx = 0; inputIdx < src->getInput_array().getCount(); ++inputIdx )
	{
		domInputLocal *inputLocal = src->getInput_array()[inputIdx];
		if( table.count(inputLocal->getSemantic()) )
		{
			*table[inputLocal->getSemantic()] = sourceCache->LocateKey(SafeResolve<domSource>(inputLocal->getSource()));
		}
	}

	if( !m_input )
	{
		throw std::runtime_error(std::string("source with INPUT semantic is absent in <sampler id=\"") + src->getID() + "\">");
	}
	if( SourceImpl::VT_FLOAT != m_input->GetType() || m_input->GetElementSize() != 1 )
	{
		throw std::runtime_error(std::string("unsupported INPUT source in <sampler id=\"") + src->getID() + "\">");
	}
	if( !m_output )
	{
		throw std::runtime_error(std::string("source with OUTPUT semantic is absent in <sampler id=\"") + src->getID() + "\">");
	}
	if( m_input->GetElementCount() != m_output->GetElementCount() )
	{
		throw std::runtime_error(std::string("sources with INPUT and OUTPUT semantics have different number of elements in <sampler id=\"") + src->getID() + "\">");
	}
}

///////////////////////////////////////////////////////////////////////////////

AnimationTargetParser::AnimationTargetParser(const domChannel *src)
  : m_target(src->getTarget())
  , m_targetId(NULL)
  , m_targetSid(NULL)
  , m_targetParam(NULL)
{
	//
	// parse target string
	//

	using std::string;

	string::size_type sid(string::npos);
	string::size_type param(string::npos);
	string::size_type tmp(m_target.find_first_of("/.("));

	if( string::npos != tmp && '/' == m_target[tmp] )
	{
		// at least one sid is specified
		// TODO: support for several sids
		m_target[tmp] = '\0'; // split target string
		sid = tmp + 1;
		tmp = m_target.find_first_of(".(", sid); // find beginning of param definition
	}

	if( string::npos != tmp )
	{
		// parameter is specified
		assert('.' == m_target[tmp] || '(' == m_target[tmp]);
		m_target[tmp] = '\0'; // split target string
		param = tmp + 1;
	}

	m_targetId = m_target.c_str();
	if( string::npos != sid )
	{
		m_targetSid = m_targetId + sid;
	}
	if( string::npos != param )
	{
		m_targetParam = m_targetId + param;
	}
}

const char* AnimationTargetParser::GetTargetId() const
{
	return m_targetId;
}

const char* AnimationTargetParser::GetTargetSid() const
{
	return m_targetSid; // TODO: support for several sids
}

const char* AnimationTargetParser::GetTargetParam() const
{
	return m_targetParam;
}

///////////////////////////////////////////////////////////////////////////////

namespace
{
	struct SubElementDesc
	{
		size_t offset;
		unsigned int size;
	};
}

SourceImpl::SourceImpl(const domSource *src, void*)
  : m_type(VT_UNDEFINED)
  , m_elementSize(0)
  , m_sourceTag(src->getId())
{
	domSource::domTechnique_common *common = src->getTechnique_common();
	if( !common )
	{
		throw std::runtime_error(std::string("common technique not found in <source id=\"") + src->getId() + "\">");
	}

	std::vector<SubElementDesc> subElemDescs;

	//
	// supported element types and their sizes
	//
	std::map<std::string, unsigned int> typeSize;
	typeSize["float"] = 1;
	typeSize["float3"] = 3;
	typeSize["float4x4"] = 16;
	typeSize["Name"] = 1;
	typeSize["IDREF"] = 1;


	//
	// fill the elemDescs array
	//

	domAccessor *accessor = common->getAccessor();
	if( !accessor )
		throw std::runtime_error("accessor not found");
	if( accessor->getStride() < accessor->getParam_array().getCount() + accessor->getOffset() )
		throw std::runtime_error("invalid stride value");

	size_t stride = (size_t) accessor->getStride();
	SubElementDesc tmpDesc;
	tmpDesc.offset = static_cast<size_t>(accessor->getOffset());
	for( size_t paramIdx = 0; paramIdx < accessor->getParam_array().getCount(); ++paramIdx )
	{
		domParam *param = accessor->getParam_array()[paramIdx];
		tmpDesc.size = typeSize[param->getType()];
		if( !tmpDesc.size )
		{
			throw std::runtime_error(std::string("unknown parameter type '") + param->getType() + "' in <source id=\"" + src->getId() + "\">");
		}
#ifdef COLLADA_STRICT
		if( param->getName() ) // include only named components (some exporters omit parameter name)
#endif
		{
			if( tmpDesc.offset + tmpDesc.size > stride )
			{
				throw std::runtime_error(std::string("invalid accessor stride in <source id='") + m_sourceTag + "'>");
			}
			subElemDescs.push_back(tmpDesc);
			m_elementSize += tmpDesc.size;
		}
		tmpDesc.offset += tmpDesc.size;
	}

	if( !m_elementSize )
	{
		throw std::runtime_error(std::string("accessor of <source id=\"") + src->getId() + "\"> produces no data");
	}


	//
	// extract data
	//

	if( domFloat_array *array = src->getFloat_array() )
	{
		const domListOfFloats &value = array->getValue();
		m_type = VT_FLOAT;
		m_elementCount = static_cast<unsigned int>(value.getCount() / stride);
		m_rawData.reserve(sizeof(float) * m_elementCount * m_elementSize);
		for( size_t elem = 0; elem < m_elementCount; ++elem )
		{
			for( size_t sub = 0; sub < subElemDescs.size(); ++sub )
			{
				for( size_t i = 0; i < subElemDescs[sub].size; ++i )
				{
					float tmp = (float) value[elem*stride + subElemDescs[sub].offset + i];
					m_rawData.insert(m_rawData.end(), (char *) &tmp, (char *) &tmp + sizeof(float));
				}
			}
		}
		return;
	}

	if( domName_array *array = src->getName_array() )
	{
		const domListOfNames &value = array->getValue();
		m_type = VT_NAME;
		m_elementCount = static_cast<unsigned int>(value.getCount() / stride);
		for( size_t elem = 0; elem < m_elementCount; ++elem )
		{
			for( size_t sub = 0; sub < subElemDescs.size(); ++sub )
			{
				assert(1 == subElemDescs[sub].size);
				const char *tmp = value[elem*stride + subElemDescs[sub].offset];
				m_nameIndex.push_back(m_rawData.size());
				m_rawData.insert(m_rawData.end(), tmp, tmp + strlen(tmp) + 1); // including '\0'
			}
		}
		assert(m_nameIndex.size() == m_elementCount * m_elementSize);
		return;
	}

	if( domIDREF_array *idarray = src->getIDREF_array() )
	{
		const xsIDREFS &value = idarray->getValue();
		m_type = VT_IDREF;
		m_elementCount = static_cast<unsigned int>(value.getCount() / stride);
		for( size_t elem = 0; elem < m_elementCount; ++elem )
		{
			for( size_t sub = 0; sub < subElemDescs.size(); ++sub )
			{
				assert(1 == subElemDescs[sub].size);
				daeElement *elt = SafeResolve<daeElement>(value[elem*stride + subElemDescs[sub].offset]);
				m_rawData.insert(m_rawData.end(), (char *) &elt, (char *) (&elt + 1));
			}
		}
		return;
	}

	throw std::runtime_error(std::string("unsupported <source id=\"") + src->getId() + "\">");
}

SourceImpl::Type SourceImpl::GetType() const
{
	return m_type;
}

unsigned int SourceImpl::GetElementCount() const
{
	return m_elementCount;
}

unsigned int SourceImpl::GetElementSize() const
{
	return m_elementSize;
}

const float* SourceImpl::GetFloatArray() const
{
	assert(VT_FLOAT == GetType());
	return (const float *) &m_rawData[0];
}

const char* SourceImpl::GetName(size_t rawIdx) const
{
	assert(VT_NAME == GetType());
	return &m_rawData[0] + m_nameIndex[rawIdx];
}

daeElement* SourceImpl::GetRef(size_t rawIdx) const
{
	assert(VT_IDREF == GetType());
	return *(daeElement **) &m_rawData[rawIdx * sizeof(daeElement *)];
}

// end of file
