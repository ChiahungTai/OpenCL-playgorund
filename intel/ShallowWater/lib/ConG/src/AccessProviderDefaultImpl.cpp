/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2009 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file AccessProviderDefaultImpl.cpp

#include "stdafx.h"

#include "AccessProviderDefaultImpl.h"
#include "AutoPtr.h"
#include "Processor.h"

///////////////////////////////////////////////////////////////////////////////

AccessorLocalUpdateImpl::AccessorLocalUpdateImpl(PrivateLocalProviderData *parent)
  : AccessorLocalBaseImpl<AccessorWriteUpdate>(parent)
{
}

void AccessorLocalUpdateImpl::Update(const void *ptr)
{
	assert(!m_presence.GetData()->m_mappedForReadCount);
	m_presence.GetData()->GetBinding()->UpdateBinding(ptr);
	m_presence.GetData()->GetBinding()->IncrementContentVersion();
}

unsigned int AccessorLocalUpdateImpl::GetAccessPattern() const
{
	return ACCESS_PATTERN_UPDATE;
}

///////////////////////////////////////////////////////////////////////////////

AccessorLocalWriteImpl::AccessorLocalWriteImpl(PrivateLocalProviderData *parent)
  : AccessorLocalBaseImpl<AccessorWriteDefault>(parent)
{
}

void* AccessorLocalWriteImpl::Map()
{
	assert(!m_presence.GetData()->m_mappedForReadCount);
	return m_presence.GetData()->GetBinding()->MapBinding();
}

void AccessorLocalWriteImpl::Unmap()
{
	assert(!m_presence.GetData()->m_mappedForReadCount);
	m_presence.GetData()->GetBinding()->UnmapBinding();
	m_presence.GetData()->GetBinding()->IncrementContentVersion();
}

unsigned int AccessorLocalWriteImpl::GetAccessPattern() const
{
	return ACCESS_PATTERN_WRITE;
}


///////////////////////////////////////////////////////////////////////////////

AccessorLocalReadImpl::AccessorLocalReadImpl(PrivateLocalProviderData *parent)
  : AccessorLocalBaseImpl<AccessorReadDefault>(parent)
{
}

const void* AccessorLocalReadImpl::Map()
{
	if( Processor *p = m_presence.GetData()->GetBinding()->GetProcessor() )
	{
		// ensure up to date
		p->DoProcess();
	}
	assert(++m_presence.GetData()->m_mappedForReadCount);
	assert(m_presence.GetData()->GetBinding()->GetBackupData());
	return m_presence.GetData()->GetBinding()->GetBackupData();
}

void AccessorLocalReadImpl::Unmap()
{
	assert(m_presence.GetData()->m_mappedForReadCount--);
}

unsigned int AccessorLocalReadImpl::GetAccessPattern() const
{
	return ACCESS_PATTERN_READ;
}

///////////////////////////////////////////////////////////////////////////////

ProviderLocalImpl::ProviderLocalImpl()
{
}

ProviderLocalImpl::~ProviderLocalImpl()
{
}

void ProviderLocalImpl::CreateAccessorWriteUpdate(MemoryObject *resource, AccessorWriteUpdate **result, Processor *p)
{
	AutoPtr<PrivateLocalProviderData> data;
	resource->GetProviderBinding()->GetPrivateData(this, this, (PrivateProviderData **) (PrivateLocalProviderData **) &data);
	AutoPtr<AccessorLocalUpdateImpl> tmp(new AccessorLocalUpdateImpl(data));
	DetachRawPtr(tmp, result);
	assert(!data->GetBinding()->GetProcessor());
	data->GetBinding()->SetProcessor(p);
}

void ProviderLocalImpl::CreateAccessorWriteDefault(MemoryObject *resource, AccessorWriteDefault **result, Processor *p)
{
	AutoPtr<PrivateLocalProviderData> data;
	resource->GetProviderBinding()->GetPrivateData(this, this, (PrivateProviderData **) (PrivateLocalProviderData **) &data);
	AutoPtr<AccessorLocalWriteImpl> tmp(new AccessorLocalWriteImpl(data));
	DetachRawPtr(tmp, result);
	assert(!data->GetBinding()->GetProcessor());
	data->GetBinding()->SetProcessor(p);
}

void ProviderLocalImpl::CreateAccessorReadDefault(MemoryObject *resource, AccessorReadDefault **result)
{
	AutoPtr<PrivateLocalProviderData> data;
	resource->GetProviderBinding()->GetPrivateData(this, this, (PrivateProviderData **) (PrivateLocalProviderData **) &data);
	AutoPtr<AccessorLocalReadImpl> tmp(new AccessorLocalReadImpl(data));
	DetachRawPtr(tmp, result);
}

PrivateProviderData* ProviderLocalImpl::NewPrivateData(ProviderBinding *target)
{
	return new PrivateLocalProviderData(this, target);
}

///////////////////////////////////////////////////////////////////////////////

PrivateLocalProviderData::PrivateLocalProviderData(ProviderLocalImpl *provider, ProviderBinding *binding)
  : m_provider(provider)
  , m_providerBinding(Referrer(GetThis()), binding)
#ifndef NDEBUG
  , m_mappedForReadCount(0)
#endif
{
	assert(provider);
	binding->GetWatch(&m_providerBindingWatch);
}

PrivateLocalProviderData::~PrivateLocalProviderData()
{
	if( m_providerBindingWatch->IsAlive() )
		m_providerBinding->Dispose(this);
	assert(!m_mappedForReadCount);
}

unsigned int PrivateLocalProviderData::GetAccessPattern() const
{
	unsigned int result = 0;
	for( AccessorsSet::const_iterator it = m_accessors.begin(); it != m_accessors.end(); ++it )
	{
		result |= (*it)->GetAccessPattern();
	}
	return result;
}

void* PrivateLocalProviderData::MapPrivate()
{
	return NULL;
}

void PrivateLocalProviderData::UnmapPrivate()
{
	// you should not call this UnmapPrivate() function since MapPrivate() always fails
	assert(false);
}

///////////////////////////////////////////////////////////////////////////////

namespace collada
{
	void CreateAccessProviderLocal(AccessProviderLocal **ppResult)
	{
		AutoPtr<AccessProviderLocal> result(new ProviderLocalImpl());
		DetachRawPtr(result, ppResult);
	}
}
// end of file
