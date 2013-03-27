/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
        Copyright (C) 2009-2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file AccessProviderD3D10Impl.h

#pragma once

#include "RefCountedImpl.h"
#include "ProviderBinding.h"

#include "AccessProviderDefault.h"
#include "AutoPtr.h"

#include "d3d10/AccessProviderD3D10.h"
#include "d3d10/DeviceManagerD3D10.h"

#include <set>

using namespace collada;

namespace
{

class ProviderD3D10Impl;
template<class T> class AccessorD3D10BaseImpl;

struct AccessorD3D10
{
	virtual void FreeDeviceResources() = 0;
	virtual UINT GetBindFlags() const = 0;
};

class PrivateD3D10ProviderData
	: public RefCountedCyclicImpl<PrivateProviderData>
	, private IDeviceListenerD3D10
{
	typedef std::set<AccessorD3D10*> AccessorsSet;

public:
	PrivateD3D10ProviderData(DeviceManagerD3D10 *deviceManager, ProviderD3D10Impl *providerD3D10, ProviderBinding *memObj);
	virtual ~PrivateD3D10ProviderData();

	class AccessorPresence
	{
	public:
		AccessorPresence(PrivateD3D10ProviderData *data, AccessorD3D10 *accessor, RefCountedCyclic *referrer)
			: m_data(Referrer(referrer), data)
			, m_location(data->m_accessors.insert(accessor).first)
		{
			m_data->GetWatch(&m_dataWatch);
			m_data->m_needFlagsRefresh = true;
		}
		~AccessorPresence()
		{
			if( m_dataWatch->IsAlive() )
			{
				m_data->m_accessors.erase(m_location);
				m_data->m_needFlagsRefresh = true;
			}
		}
		PrivateD3D10ProviderData* GetPrivateData() const throw() { return m_data; }
	private:
		AutoPtrCyclic<PrivateD3D10ProviderData> m_data;
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

private:
	// IDeviceListenerD3D10
	virtual HRESULT OnSetDevice(ID3D10Device *pd3dDevice);

private:
	friend class AccessorD3D10UpdateImpl;
	friend class AccessorD3D10ReadImpl;
	friend class AccessorD3D10WriteImpl;
	friend class AccessorD3D10BufferSRVImpl;
	friend class AccessorD3D10BufferReadImpl;
	friend class AccessorD3D10BufferWriteImpl;

	AutoPtrCyclic<ProviderBinding> m_providerBinding;
	AutoPtr<Watch> m_providerBindingWatch;
	AutoPtr<DeviceManagerD3D10> m_d3dDeviceManager;
	AutoPtr<ProviderD3D10Impl> m_provider;

	ID3D10Buffer* GetBuffer();
	CComPtr<ID3D10Buffer> m_d3dBuffer;

	AccessorsSet m_accessors;
	bool m_needFlagsRefresh;

#ifndef NDEBUG
	unsigned int m_lockedForReadCount;
	bool m_lockedForWrite;
#endif
};

///////////////////////////////////////////////////////////////////////////////

#pragma warning (push)
#pragma warning (disable: 4355) // 'this' : used in base member initializer list

template <class BaseType>
class AccessorD3D10BaseImpl
	: public RefCountedCyclicImpl<BaseType>
	, private AccessorD3D10
{
public:
	AccessorD3D10BaseImpl(PrivateD3D10ProviderData *data)
		: m_presence(data, this, this)
	{
	}

	// interface Accessor
	virtual MemoryObject* GetMemoryObject()
	{
		return m_presence.GetPrivateData()->GetBinding()->GetMemoryObject();
	}

protected:
	PrivateD3D10ProviderData::AccessorPresence m_presence;
};

#pragma warning (pop)

//
// default accessors
//

class AccessorD3D10UpdateImpl : public AccessorD3D10BaseImpl<AccessorWriteUpdate>
{
public:
	AccessorD3D10UpdateImpl(PrivateD3D10ProviderData *data);

	// AccessorWriteUpdate
	virtual void Update(const void *data);

	// AccessorD3D10
	virtual void FreeDeviceResources() {}
	virtual UINT GetBindFlags() const;
};

class AccessorD3D10WriteImpl : public AccessorD3D10BaseImpl<AccessorWriteDefault>
{
public:
	AccessorD3D10WriteImpl(PrivateD3D10ProviderData *data);

	// AccessorWriteDefault
	virtual void* Map();
	virtual void Unmap();

	// AccessorD3D10
	virtual void FreeDeviceResources() {}
	virtual UINT GetBindFlags() const;
};

class AccessorD3D10ReadImpl : public AccessorD3D10BaseImpl<AccessorReadDefault>
{
public:
	AccessorD3D10ReadImpl(PrivateD3D10ProviderData *data);

	// AccessorReadDefault
	virtual const void* Map();
	virtual void Unmap();

	// AccessorD3D10
	virtual void FreeDeviceResources() {}
	virtual UINT GetBindFlags() const;
};

//
// d3d10 specific accessors
//

class AccessorD3D10BufferSRVImpl : public AccessorD3D10BaseImpl<AccessorD3D10BufferSRV>
{
public:
	AccessorD3D10BufferSRVImpl(PrivateD3D10ProviderData *data, DXGI_FORMAT format, D3D10_BUFFER_SRV desc);

	// AccessorD3D10BufferSRV
	virtual ID3D10ShaderResourceView* Map();
	virtual void Unmap();

	// AccessorD3D10
	virtual void FreeDeviceResources() { m_srv.Release(); }
	virtual UINT GetBindFlags() const;

private:
	CComPtr<ID3D10ShaderResourceView> m_srv;
	D3D10_SHADER_RESOURCE_VIEW_DESC m_desc;
};

class AccessorD3D10BufferReadImpl : public AccessorD3D10BaseImpl<AccessorD3D10BufferRead>
{
public:
	AccessorD3D10BufferReadImpl(PrivateD3D10ProviderData *data, UINT d3dBindFlags);

	// AccessorD3D10BufferRead
	virtual ID3D10Buffer* Map();
	virtual void Unmap();

	// AccessorD3D10
	virtual void FreeDeviceResources() {}
	virtual UINT GetBindFlags() const;

private:
	UINT m_d3dBindFlags;
};

class AccessorD3D10BufferWriteImpl : public AccessorD3D10BaseImpl<AccessorD3D10BufferWrite>
{
public:
	AccessorD3D10BufferWriteImpl(PrivateD3D10ProviderData *data, UINT d3dBindFlags);

	// AccessorD3D10BufferWrite
	virtual ID3D10Buffer* Map();
	virtual void Unmap();

	// AccessorD3D10
	virtual void FreeDeviceResources() {}
	virtual UINT GetBindFlags() const;

private:
	UINT m_d3dBindFlags;
};

///////////////////////////////////////////////////////////////////////////////

class ProviderD3D10Impl
	: public RefCountedImpl<AccessProviderD3D10>
	, private IPrivateDataFactory
{
public:
	ProviderD3D10Impl(DeviceManagerD3D10 *deviceManager);
	virtual ~ProviderD3D10Impl();

	// AccessProviderD3D10 implementation
	virtual DeviceManagerD3D10* GetDeviceManager() const throw();
	virtual void CreateAccessorD3D10BufferSRV(MemoryObject *resource, AccessorD3D10BufferSRV **result, DXGI_FORMAT format, D3D10_BUFFER_SRV desc);
	virtual void CreateAccessorD3D10BufferRead(MemoryObject *resource, AccessorD3D10BufferRead **result, UINT d3dBindFlags);
	virtual void CreateAccessorD3D10BufferWrite(MemoryObject *resource, AccessorD3D10BufferWrite **result, UINT d3dBindFlags, Processor *p);

private:
	// IPrivateDataFactory
	virtual PrivateProviderData* NewPrivateData(ProviderBinding *target);

private:
	AutoPtr<PrivateD3D10ProviderData> GetMyData(MemoryObject *m);
	AutoPtr<DeviceManagerD3D10> m_d3dDeviceManager;
};


} // end of anonymous namespace
// end of file
