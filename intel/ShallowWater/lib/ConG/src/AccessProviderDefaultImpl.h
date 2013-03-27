/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2009 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file AccessProviderDefaultImpl.h

#pragma once

#include "AccessProviderDefault.h"
#include "ProviderBinding.h"
#include "RefCountedImpl.h"
#include "AutoPtr.h"

#include <cassert>
#include <vector>
#include <set>

using namespace collada;

namespace
{
class ProviderLocalImpl;

struct AccessorLocal
{
	virtual unsigned int GetAccessPattern() const = 0;
};

class PrivateLocalProviderData : public RefCountedCyclicImpl<PrivateProviderData>
{
	typedef std::set<AccessorLocal*> AccessorsSet;

public:
	PrivateLocalProviderData(ProviderLocalImpl *provider, ProviderBinding *binding);
	virtual ~PrivateLocalProviderData();

	class AccessorPresence
	{
	public:
		AccessorPresence(PrivateLocalProviderData *data, AccessorLocal *accessor, RefCountedCyclic *referrer)
			: m_data(Referrer(referrer), data)
			, m_location(data->m_accessors.insert(accessor).first)
		{
			m_data->GetWatch(&m_dataWatch);
		}
		~AccessorPresence()
		{
			if( m_dataWatch->IsAlive() )
			{
				m_data->m_accessors.erase(m_location);
			}
		}
		PrivateLocalProviderData* GetData() const throw() { return m_data; }
	private:
		AutoPtrCyclic<PrivateLocalProviderData> m_data;
		AutoPtr<Watch> m_dataWatch;
		AccessorsSet::iterator m_location;
	};

	ProviderBinding* GetBinding() const throw()
	{
		assert(m_providerBindingWatch->IsAlive());
		return m_providerBinding;
	}

	// PrivateProviderData
	virtual unsigned int GetAccessPattern() const;
	virtual void* MapPrivate();
	virtual void UnmapPrivate();

//private:
	AutoPtr<ProviderLocalImpl> m_provider;
#ifndef NDEBUG
	unsigned int m_mappedForReadCount;
#endif

private:
	AutoPtrCyclic<ProviderBinding> m_providerBinding;
	AutoPtr<Watch> m_providerBindingWatch;
	AccessorsSet m_accessors;
}; // class PrivateLocalProviderData

///////////////////////////////////////////////////////////////////////////////

#pragma warning (push)
#pragma warning (disable: 4355) // 'this' : used in base member initializer list
template <class BaseType>
class AccessorLocalBaseImpl
	: public RefCountedCyclicImpl<BaseType>
	, public AccessorLocal
{
public:
	AccessorLocalBaseImpl(PrivateLocalProviderData *data)
		: m_presence(data, this, this)
	{
	}

	// Accessor
	virtual MemoryObject* GetMemoryObject()
	{
		return m_presence.GetData()->GetBinding()->GetMemoryObject();
	}

protected:
	PrivateLocalProviderData::AccessorPresence m_presence;
};
#pragma warning (pop)

class AccessorLocalUpdateImpl : public AccessorLocalBaseImpl<AccessorWriteUpdate>
{
public:
	AccessorLocalUpdateImpl(PrivateLocalProviderData *data);

	// AccessorWriteUpdate
	virtual void Update(const void *ptr);

	// AccessorLocal
	virtual unsigned int GetAccessPattern() const;
};

class AccessorLocalWriteImpl : public AccessorLocalBaseImpl<AccessorWriteDefault>
{
public:
	AccessorLocalWriteImpl(PrivateLocalProviderData *data);

	// AccessorWriteDefault
	virtual void* Map();
	virtual void Unmap();

	// AccessorLocal
	virtual unsigned int GetAccessPattern() const;
};

class AccessorLocalReadImpl : public AccessorLocalBaseImpl<AccessorReadDefault>
{
public:
	AccessorLocalReadImpl(PrivateLocalProviderData *data);

	// AccessorReadDefault
	virtual const void* Map();
	virtual void Unmap();

	// AccessorLocal
	virtual unsigned int GetAccessPattern() const;
};

///////////////////////////////////////////////////////////////////////////////

class ProviderLocalImpl
	: public RefCountedImpl<AccessProviderLocal>
	, private IPrivateDataFactory
{
public:
	ProviderLocalImpl();
	virtual ~ProviderLocalImpl();

	// AccessProviderLocal implementation
	virtual void CreateAccessorWriteUpdate(MemoryObject *resource, AccessorWriteUpdate **result, Processor *p);
	virtual void CreateAccessorWriteDefault(MemoryObject *resource, AccessorWriteDefault **result, Processor *p);
	virtual void CreateAccessorReadDefault(MemoryObject *resource, AccessorReadDefault **result);

private:
	// IPrivateDataFactory
	virtual PrivateProviderData* NewPrivateData(ProviderBinding *target);
};

} // end of anonymous namespace
// end of file
