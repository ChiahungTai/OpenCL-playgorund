/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file MemoryManagerImpl.cpp

#include "stdafx.h"

#include "MemoryManagerImpl.h"
#include "ProviderBinding.h"
#include "AutoPtr.h"
#include "Processor.h"

static const size_t g_align = 16;

template <class T>
T* AlignPointer(T *p)
{
	return (T*) (((size_t) p + g_align - 1) & ~(g_align - 1));
}

MemoryObjectImpl::MemoryObjectImpl(MemoryManager *manager, size_t bytesSize, const void *initialData)
  : m_version(0)
  , m_processor(Referrer(GetThis()))
  , m_manager(manager)
  , m_bytesSize(bytesSize)
  , m_lockedThrough(NULL)
#ifndef NDEBUG
  , m_mappedForWrite(false)
#endif
{
	assert(bytesSize);
	if( initialData )
	{
		m_rawBytesDataBackup.resize(bytesSize + g_align - 1);
		memcpy(AlignPointer(&m_rawBytesDataBackup[0]), initialData, bytesSize);
	}
}

MemoryObjectImpl::~MemoryObjectImpl()
{
	assert(!m_mappedForWrite);
	assert(m_versionListeners.empty());
}

void MemoryObjectImpl::Dispose(PrivateProviderData *what)
{
	for( ProviderToDataMap::iterator it = m_providers.begin(); it != m_providers.end(); ++it )
	{
		if( it->second == what )
		{
			m_providers.erase(it);
			return;
		}
	}
	assert(false); // data not found
}

bool MemoryObjectImpl::NeedBackup(PrivateProviderData *source) const
{
	return m_rawBytesDataBackup.empty();
}

void MemoryObjectImpl::Backup(PrivateProviderData *source, const void *data)
{
	m_rawBytesDataBackup.resize(m_bytesSize + g_align - 1);
	memcpy(AlignPointer(&m_rawBytesDataBackup[0]), data, m_bytesSize);
}

const void* MemoryObjectImpl::GetBackupData()
{
	assert(!m_mappedForWrite);
	return m_rawBytesDataBackup.empty() ? NULL : AlignPointer(&m_rawBytesDataBackup[0]);
}

void MemoryObjectImpl::UpdateBinding(const void *data)
{
	assert(!m_mappedForWrite);

	assert(false);
}

void* MemoryObjectImpl::MapBinding()
{
	assert(!m_mappedForWrite);
#ifndef NDEBUG
	m_mappedForWrite = true;
#endif

	for( ProviderToDataMap::const_iterator it = m_providers.begin(); it != m_providers.end(); ++it )
	{
		if( void *locked = it->second->MapPrivate() )
		{
			m_lockedThrough = it->second;
			return locked;
		}
	}

	m_lockedThrough = NULL;
	m_rawBytesDataBackup.resize(m_bytesSize + g_align - 1);
	return AlignPointer(&m_rawBytesDataBackup[0]);
}

void MemoryObjectImpl::UnmapBinding()
{
	assert(m_mappedForWrite);
	if( m_lockedThrough )
	{
		m_lockedThrough->UnmapPrivate();
		m_lockedThrough = NULL;
	}
#ifndef NDEBUG
	m_mappedForWrite = false;
#endif
}

unsigned int MemoryObjectImpl::GetCommonAccessPattern(const PrivateProviderData *exclude) const
{
	unsigned int result = 0;
	for( ProviderToDataMap::const_iterator it = m_providers.begin(); it != m_providers.end(); ++it )
	{
		if( it->second != exclude )
		{
			result |= it->second->GetAccessPattern();
		}
	}
	return result;
}

void MemoryObjectImpl::IncrementContentVersion()
{
	++m_version;
	std::for_each(m_versionListeners.begin(), m_versionListeners.end(),
		std::bind2nd(std::mem_fun1(&IVersionListener::OnChangeContent), this));
}

unsigned long long MemoryObjectImpl::GetContentVersion() const throw()
{
	return m_version;
}

void MemoryObjectImpl::AddListener(IVersionListener *ls)
{
	assert(!m_versionListeners.count(ls));
	m_versionListeners.insert(ls);
}

void MemoryObjectImpl::RemoveListener(IVersionListener *ls)
{
	assert(m_versionListeners.count(ls));
	m_versionListeners.erase(ls);
}

size_t MemoryObjectImpl::GetBytesSize() const throw()
{
	return m_bytesSize;
}

MemoryManager* MemoryObjectImpl::GetManager() const throw()
{
	return m_manager;
}

Processor* MemoryObjectImpl::GetProcessor() const throw()
{
	return m_processor;
}

ProviderBinding* MemoryObjectImpl::GetProviderBinding() throw()
{
	return this;
}

MemoryObject* MemoryObjectImpl::GetMemoryObject() throw()
{
	return this;
}

void MemoryObjectImpl::SetProcessor(Processor *p)
{
	m_processor = p;
}

void MemoryObjectImpl::GetPrivateData(AccessProvider *p, IPrivateDataFactory *initializer, PrivateProviderData **ppResult)
{
	ProviderToDataMap::mapped_type &stored = m_providers[p];
	if( !stored )
	{
		assert(initializer);
		stored = initializer->NewPrivateData(this);
	}
	AutoPtr<PrivateProviderData> result(stored);
	DetachRawPtr(result, ppResult);
}

///////////////////////////////////////////////////////////////////////////////

void MemoryManagerImpl::Allocate(size_t bytesSize, const void *initialData, MemoryObject **ppResult)
{
	AutoPtr<MemoryObject> result(new MemoryObjectImpl(this, bytesSize, initialData));
	DetachRawPtr(result, ppResult);
}


namespace collada
{
	void CreateMemoryManager(MemoryManager **ppResult)
	{
		AutoPtr<MemoryManager> result(new MemoryManagerImpl());
		DetachRawPtr(result, ppResult);
	}
}

// end of file
