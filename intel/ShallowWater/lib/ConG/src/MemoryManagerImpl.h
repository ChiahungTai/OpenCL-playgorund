/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file MemoryManagerImpl.h

#pragma once

#include "MemoryManager.h"
#include "ProviderBinding.h"
#include "RefCountedImpl.h"

#include <vector>
#include <map>
#include <set>

using namespace collada;

namespace
{

class MemoryObjectImpl : public RefCountedCyclicImpl<MemoryObject, ProviderBinding>
{
	typedef std::map<AutoPtr<AccessProvider>, PrivateProviderData*> ProviderToDataMap;

public:
	MemoryObjectImpl(MemoryManager *manager, size_t bytesSize, const void *initialData);
	virtual ~MemoryObjectImpl();

	// MemoryObject
	virtual size_t GetBytesSize() const throw();
	virtual MemoryManager* GetManager() const throw();
	virtual unsigned long long GetContentVersion() const throw();
	virtual void AddListener(MemoryObject::IVersionListener *ls);
	virtual void RemoveListener(MemoryObject::IVersionListener *ls);
	virtual Processor* GetProcessor() const throw();
	virtual ProviderBinding* GetProviderBinding() throw();

	// ProviderBinding
	virtual void GetPrivateData(AccessProvider *p, IPrivateDataFactory *initializer, PrivateProviderData **ppResult);
	virtual void Dispose(PrivateProviderData *what);
	virtual unsigned int GetCommonAccessPattern(const PrivateProviderData *exclude) const;
	virtual void IncrementContentVersion();
	virtual void UpdateBinding(const void *data);
	virtual void* MapBinding();
	virtual void UnmapBinding();
	virtual bool NeedBackup(PrivateProviderData *source) const;
	virtual void Backup(PrivateProviderData *source, const void *data);
	virtual const void* GetBackupData();
	virtual void SetProcessor(Processor *p);
	virtual MemoryObject* GetMemoryObject() throw();

private:
	size_t m_bytesSize;
	std::vector<char> m_rawBytesDataBackup;
	unsigned long long m_version;
	AutoPtrCyclic<Processor> m_processor;
	AutoPtr<MemoryManager> m_manager;
	std::set<MemoryObject::IVersionListener *> m_versionListeners;
	ProviderToDataMap m_providers;
	PrivateProviderData *m_lockedThrough;
#ifndef NDEBUG
	bool m_mappedForWrite;
#endif
};

///////////////////////////////////////////////////////////////////////////////

class MemoryManagerImpl : public RefCountedImpl<MemoryManager>
{
public:
	virtual void Allocate(size_t bytesSize, const void *initialData, MemoryObject **result);
};

///////////////////////////////////////////////////////////////////////////////
} // end of unnamed namespace
// end of file
