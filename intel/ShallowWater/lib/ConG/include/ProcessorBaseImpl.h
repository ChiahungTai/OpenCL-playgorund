/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2009 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file ProcessorBaseImpl.h

#pragma once

#include "MemoryManager.h"
#include "AlgorithmBase.h"
#include "AutoPtr.h"
#include "RefCountedImpl.h"
#include "Processor.h"

#include <set>
#include <map>
#include <vector>

namespace collada
{
	struct IAlgorithm;
	struct AlgorithmResource;
	struct AlgorithmResourceAccessor;

class ProcessorBase
	: public RefCountedCyclicImpl<Processor>
	, private Processor::IDirtyListener
	, private MemoryObject::IVersionListener
	, private IResourceTracker
{
protected:
	ProcessorBase(IAlgorithm *algorithm);
	virtual ~ProcessorBase() = 0;

	IAlgorithm* GetAlgorithm() { return m_algorithm; }

	// Processor
	virtual void AddListener(IDirtyListener *ls);
	virtual void RemoveListener(IDirtyListener *ls);

protected:
	void SetData(const AlgorithmResource *inputs, const AlgorithmResource *outputs);
	const AlgorithmResourceAccessor* GetAccessors() const;

	bool GetDirty() const { return m_isDirty; }
	void SetDirty();
	void ClearDirtyFlag() { m_isDirty = false; }

private:
	// Processor::IDirtyListener
	virtual void OnDirty(Processor *which);

	// MemoryObject::IVersionListener
	virtual void OnChangeContent(MemoryObject *which);

	// IResourceTracker
	virtual void OnCreateReadAccessor(unsigned int semanticId, AccessorRead *accessor);
	virtual void OnCreateWriteAccessor(unsigned int semanticId, AccessorWrite *accessor);

private:
	ProcessorBase(const ProcessorBase&); // no copy
	ProcessorBase& operator = (const ProcessorBase &);

	void FreeAllResources();

	typedef std::set<AutoPtr<MemoryObject>> InputSet;
	typedef std::map<Processor *, unsigned int> ProcessorSet;
	AutoPtr<IAlgorithm> m_algorithm;
	InputSet m_inputs;
	ProcessorSet m_inputProcessors;
	std::set<IDirtyListener*> m_listeners;

	typedef AutoPtrCyclic<Accessor> AccessorPtr;
	std::vector<AccessorPtr> m_accessors;
	std::vector<AlgorithmResourceAccessor> m_accessorsCache;

	bool m_isDirty;
};


class ProcessorSimple: public ProcessorBase
{
public:
	ProcessorSimple(IAlgorithmSimple *algorithm, const AlgorithmResource *inputs, const AlgorithmResource *outputs);

	// Processor
	virtual bool DoProcess();
};

///////////////////////////////////////////////////////////////////////////////
} // end namespace collada
// end of file
