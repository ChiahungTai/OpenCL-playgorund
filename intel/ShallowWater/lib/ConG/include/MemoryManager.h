/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2009 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file MemoryManager.h

#pragma once

#include "SceneGraphBase.h"
#include "RefCounted.h"

namespace collada
{
	struct ProviderBinding;
	struct Processor;

struct MemoryObject : public RefCountedCyclic // WeakTarget<MemoryObject>
{
	virtual size_t GetBytesSize() const throw() = 0;
	virtual MemoryManager* GetManager() const throw() = 0;
	virtual ProviderBinding* GetProviderBinding() throw() = 0;

	struct IVersionListener
	{
		virtual void OnChangeContent(MemoryObject *which) = 0;
	};
	virtual unsigned long long GetContentVersion() const throw() = 0;
	virtual void AddListener(IVersionListener *ls) = 0;
	virtual void RemoveListener(IVersionListener *ls) = 0;

	/// several memory objects may share the same processor
	/// memory object becomes dirty when its processor become dirty
	virtual Processor* GetProcessor() const throw() = 0;
};

struct Accessor : public RefCountedCyclic
{
	virtual MemoryObject* GetMemoryObject() = 0;
};

struct AccessorWrite : public Accessor
{
};

struct AccessorRead : public Accessor
{
};

struct AccessProvider : public RefCounted
{
};

struct MemoryManager : public RefCounted
{
	virtual void Allocate(size_t bytesSize, const void *initialData, MemoryObject **result) = 0;
};

void CreateMemoryManager(MemoryManager **ppResult);

} // end of namespace collada
// end of file
