/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
        Copyright (C) 2009-2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file ProcessorBaseImpl.cpp

#include "stdafx.h"

#include "ProcessorBaseImpl.h"
#include "AlgorithmBase.h"

using namespace collada;

static const AlgorithmResourceAccessor g_terminator = {-1, NULL};

namespace collada
{
///////////////////////////////////////////////////////////////////////////////

ProcessorBase::ProcessorBase(IAlgorithm *algorithm)
  : m_algorithm(algorithm)
  , m_isDirty(true)
{
	assert(m_algorithm);
	m_accessorsCache.push_back(g_terminator);
}

ProcessorBase::~ProcessorBase()
{
	FreeAllResources();
}

void ProcessorBase::FreeAllResources()
{
	m_accessors.clear();
	m_accessorsCache.clear();
	m_accessorsCache.push_back(g_terminator);

	for( ProcessorSet::const_iterator it = m_inputProcessors.begin(); it != m_inputProcessors.end(); ++it )
	{
		it->first->RemoveListener(this);
	}
	for( InputSet::const_iterator it = m_inputs.begin(); it != m_inputs.end(); ++it )
	{
		if( !(*it)->GetProcessor() )
		{
			(*it)->RemoveListener(this);
		}
	}
	m_inputs.clear();
	assert(m_listeners.empty());
}

void ProcessorBase::SetData(const AlgorithmResource *inputs, const AlgorithmResource *outputs)
{
	assert(outputs);
	FreeAllResources();

	// merge inputs and outputs into a single list
	std::vector<AlgorithmResource> inoutsMerged;

	// append inputs
	if( inputs )
	{
		for( ;inputs->semanticId != -1; ++inputs )
		{
			if( inputs->data )
			{
				// register input resource
				assert(!m_inputs.count(inputs->data));
				m_inputs.insert(inputs->data);
				if( Processor *p = inputs->data->GetProcessor() )
				{
					ProcessorSet::_Pairib ib = m_inputProcessors.insert(ProcessorSet::value_type(p, 0));
					if( ib.second ) // if just added
					{
						(*ib.first).first->AddListener(this); // listen for OnDirty
					}
					++(*ib.first).second;
				}
				else
				{
					inputs->data->AddListener(this); // listen for OnChangeContent
				}
				inoutsMerged.push_back(*inputs);
			}
		}
	}

	// append outputs
	for( ;outputs->semanticId != -1; ++outputs )
	{
		inoutsMerged.push_back(*outputs);
	}

	// append terminator
	assert(-1 == outputs->semanticId);
	inoutsMerged.push_back(*outputs);

	// create accessors
	assert(m_accessors.empty());
	assert(-1 == m_accessorsCache[0].semanticId); // should contain terminator only
	m_algorithm->CreateAccessors(&inoutsMerged[0], this, this);
}

const AlgorithmResourceAccessor* ProcessorBase::GetAccessors() const
{
	return &m_accessorsCache[0];
}

void ProcessorBase::SetDirty()
{
	if( !m_isDirty )
	{
		m_isDirty = true;
		std::for_each(m_listeners.begin(), m_listeners.end(),
			std::bind2nd(std::mem_fun1(&IDirtyListener::OnDirty), this));
	}
}

void ProcessorBase::OnDirty(Processor *which)
{
	SetDirty();
}

void ProcessorBase::OnChangeContent(MemoryObject *which)
{
	SetDirty();
}

void ProcessorBase::OnCreateReadAccessor(unsigned int semanticId, AccessorRead *accessor)
{
	assert(accessor);
	m_accessors.push_back(AccessorPtr(Referrer(this), accessor));
	m_accessorsCache.back().semanticId = semanticId;
	m_accessorsCache.back().accessor = m_accessors.back();
	m_accessorsCache.push_back(g_terminator);
}

void ProcessorBase::OnCreateWriteAccessor(unsigned int semanticId, AccessorWrite *accessor)
{
	assert(accessor);
	m_accessors.push_back(AccessorPtr(Referrer(this), accessor));
	m_accessorsCache.back().semanticId = semanticId;
	m_accessorsCache.back().accessor = m_accessors.back();
	m_accessorsCache.push_back(g_terminator);
}

void ProcessorBase::AddListener(IDirtyListener *ls)
{
	assert(ls);
	assert(!m_listeners.count(ls));
	m_listeners.insert(ls);
}

void ProcessorBase::RemoveListener(IDirtyListener *ls)
{
	assert(m_listeners.count(ls));
	m_listeners.erase(ls);
}

///////////////////////////////////////////////////////////////////////////////

ProcessorSimple::ProcessorSimple(IAlgorithmSimple *algorithm, const AlgorithmResource *inputs, const AlgorithmResource *outputs)
  : ProcessorBase(algorithm)
{
	SetData(inputs, outputs);
}

bool ProcessorSimple::DoProcess()
{
	if( GetDirty() )
	{
		static_cast<IAlgorithmSimple*>(GetAlgorithm())->DoProcess(GetAccessors());
		ClearDirtyFlag();
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace collada
// end of file
