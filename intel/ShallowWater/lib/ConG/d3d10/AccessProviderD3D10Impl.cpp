/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2009 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file AccessProviderD3D10Impl.cpp

#include "stdafx.h"
#include "d3d10/DeviceManagerD3D10.h"
#include "AccessProviderD3D10Impl.h"
#include "Processor.h"

///////////////////////////////////////////////////////////////////////////////

AccessorD3D10BufferSRVImpl::AccessorD3D10BufferSRVImpl(PrivateD3D10ProviderData *data, DXGI_FORMAT format, D3D10_BUFFER_SRV desc)
  : AccessorD3D10BaseImpl<AccessorD3D10BufferSRV>(data)
{
	m_desc.Format = format;
	m_desc.ViewDimension = D3D10_SRV_DIMENSION_BUFFER;
	m_desc.Buffer = desc;
}

ID3D10ShaderResourceView* AccessorD3D10BufferSRVImpl::Map()
{
	assert(!m_presence.GetPrivateData()->m_lockedForWrite);
	if( Processor *p = m_presence.GetPrivateData()->GetBinding()->GetProcessor() )
	{
		// ensure up to date
		p->DoProcess();
	}
	if( !m_srv )
	{
		CComPtr<ID3D10Device> device;
		m_presence.GetPrivateData()->GetBuffer()->GetDevice(&device);
		if( FAILED(device->CreateShaderResourceView(m_presence.GetPrivateData()->GetBuffer(), &m_desc, &m_srv)) )
		{
			throw std::runtime_error("CreateShaderResourceView failed");
		}
	}
	assert(++m_presence.GetPrivateData()->m_lockedForReadCount);
	return m_srv;
}

void AccessorD3D10BufferSRVImpl::Unmap()
{
	assert(!m_presence.GetPrivateData()->m_lockedForWrite);
	assert(m_presence.GetPrivateData()->m_lockedForReadCount--);
}

UINT AccessorD3D10BufferSRVImpl::GetBindFlags() const
{
	return D3D10_BIND_SHADER_RESOURCE;
}

///////////////////////////////////////////////////////////////////////////////

AccessorD3D10BufferReadImpl::AccessorD3D10BufferReadImpl(PrivateD3D10ProviderData *data, UINT d3dBindFlags)
  : AccessorD3D10BaseImpl<AccessorD3D10BufferRead>(data)
  , m_d3dBindFlags(d3dBindFlags)
{
}

ID3D10Buffer* AccessorD3D10BufferReadImpl::Map()
{
	assert(!m_presence.GetPrivateData()->m_lockedForWrite);
	if( Processor *p = m_presence.GetPrivateData()->GetBinding()->GetProcessor() )
	{
		// ensure up to date
		p->DoProcess();
	}
//	++m_parent->m_lockedForReadCount;
	return m_presence.GetPrivateData()->GetBuffer();
}

void AccessorD3D10BufferReadImpl::Unmap()
{
	assert(!m_presence.GetPrivateData()->m_lockedForWrite);
//	assert(m_parent->m_lockedForReadCount > 0);
//	--m_parent->m_lockedForReadCount;
}

UINT AccessorD3D10BufferReadImpl::GetBindFlags() const
{
	return m_d3dBindFlags;
}

///////////////////////////////////////////////////////////////////////////////

AccessorD3D10BufferWriteImpl::AccessorD3D10BufferWriteImpl(PrivateD3D10ProviderData *data, UINT d3dBindFlags)
  : AccessorD3D10BaseImpl<AccessorD3D10BufferWrite>(data)
  , m_d3dBindFlags(d3dBindFlags)
{
}

ID3D10Buffer* AccessorD3D10BufferWriteImpl::Map()
{
	assert(!m_presence.GetPrivateData()->m_lockedForWrite);
	assert(!m_presence.GetPrivateData()->m_lockedForReadCount);
	ID3D10Buffer *result = m_presence.GetPrivateData()->GetBuffer();
#ifndef NDEBUG
	m_presence.GetPrivateData()->m_lockedForWrite = true;
#endif
	return result;
}

void AccessorD3D10BufferWriteImpl::Unmap()
{
	assert(m_presence.GetPrivateData()->m_lockedForWrite);
	assert(!m_presence.GetPrivateData()->m_lockedForReadCount);
#ifndef NDEBUG
	m_presence.GetPrivateData()->m_lockedForWrite = false;
#endif
	m_presence.GetPrivateData()->GetBinding()->IncrementContentVersion();
}

UINT AccessorD3D10BufferWriteImpl::GetBindFlags() const
{
	return m_d3dBindFlags;
}

///////////////////////////////////////////////////////////////////////////////

PrivateD3D10ProviderData::PrivateD3D10ProviderData(DeviceManagerD3D10 *deviceManager, ProviderD3D10Impl *providerD3D10, ProviderBinding *binding)
  : m_providerBinding(Referrer(GetThis()), binding)
  , m_d3dDeviceManager(deviceManager)
  , m_provider(providerD3D10)
  , m_needFlagsRefresh(false)
#ifndef NDEBUG
  , m_lockedForWrite(false)
  , m_lockedForReadCount(0)
#endif
{
	binding->GetWatch(&m_providerBindingWatch);
	m_d3dDeviceManager->AddListener(this);
}

PrivateD3D10ProviderData::~PrivateD3D10ProviderData()
{
	if( m_providerBindingWatch->IsAlive() )
		m_providerBinding->Dispose(this);
	m_d3dDeviceManager->RemoveListener(this);
	assert(!m_lockedForWrite);
	assert(!m_lockedForReadCount);
}

HRESULT PrivateD3D10ProviderData::OnSetDevice(ID3D10Device *pd3dDevice)
{
	assert(!m_lockedForWrite);
	assert(!m_lockedForReadCount);

	if( m_d3dBuffer )
	{
		CComPtr<ID3D10Device> currentDevice;
		m_d3dBuffer->GetDevice(&currentDevice);
		if( pd3dDevice == currentDevice )
		{
			// same device - no action required
			return S_OK;
		}


		//
		// device is being changed - backup data on host side if needed
		//

		if( m_providerBinding->NeedBackup(this) )
		{
			CComPtr<ID3D10Buffer> stagingBuffer;
			D3D10_BUFFER_DESC currentDesc;
			m_d3dBuffer->GetDesc(&currentDesc);
			if( currentDesc.CPUAccessFlags & D3D10_CPU_ACCESS_READ )
			{
				// direct copy
				stagingBuffer = m_d3dBuffer;
			}
			else
			{
				// copy via staging
				D3D10_BUFFER_DESC desc;
				desc.ByteWidth = static_cast<UINT>(m_providerBinding->GetMemoryObject()->GetBytesSize());
				desc.Usage = D3D10_USAGE_STAGING;
				desc.BindFlags = 0;
				desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
				desc.MiscFlags = 0;

				HRESULT hr = currentDevice->CreateBuffer(&desc, NULL, &stagingBuffer);
				if( FAILED(hr) )
				{
					return hr;
				}
				currentDevice->CopyResource(stagingBuffer, m_d3dBuffer);
			}

			void *data;
			HRESULT hr = stagingBuffer->Map(D3D10_MAP_READ, 0, &data);
			if( FAILED(hr) )
			{
				return hr;
			}

			try
			{
				m_providerBinding->Backup(this, data);
			}
			catch( const std::bad_alloc& )
			{
				stagingBuffer->Unmap();
				return E_OUTOFMEMORY;
			}

			stagingBuffer->Unmap();
		}
		m_d3dBuffer.Release();

		for( AccessorsSet::const_iterator it = m_accessors.begin(); it != m_accessors.end(); ++it )
		{
			(*it)->FreeDeviceResources();
		}
	}

	return S_OK;
}

ID3D10Buffer* PrivateD3D10ProviderData::GetBuffer()
{
	if( m_needFlagsRefresh )
	{
		OnSetDevice(NULL); // TODO: unnecessary if flags were not changed
	}

	if( !m_d3dBuffer )
	{
		assert(m_d3dDeviceManager->GetDevice());
		assert(!m_accessors.empty());

		D3D10_BUFFER_DESC desc;
		desc.ByteWidth = static_cast<UINT>(m_providerBinding->GetMemoryObject()->GetBytesSize());
		desc.BindFlags = 0;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		unsigned int commonAccessPattern = m_providerBinding->GetCommonAccessPattern(this);

		// collect bind flags
		for( AccessorsSet::const_iterator it = m_accessors.begin(); it != m_accessors.end(); ++it )
		{
			desc.BindFlags |= (*it)->GetBindFlags();
		}

		if( commonAccessPattern & ACCESS_PATTERN_READ )
		{
			desc.CPUAccessFlags |= D3D10_CPU_ACCESS_READ;
		}

		if( commonAccessPattern & ACCESS_PATTERN_WRITE )
		{
			desc.CPUAccessFlags |= D3D10_CPU_ACCESS_WRITE;
			desc.Usage = D3D10_USAGE_DYNAMIC;
		}
		else if( (commonAccessPattern & ACCESS_PATTERN_UPDATE)
		         || (desc.BindFlags & (D3D10_BIND_STREAM_OUTPUT|D3D10_BIND_RENDER_TARGET)) )
		{
			desc.Usage = D3D10_USAGE_DEFAULT;
		}
		else
		{
			desc.Usage = D3D10_USAGE_IMMUTABLE;
		}
/*
		switch( desc.Usage )
		{
		case D3D10_USAGE_DEFAULT:
			printf("0x%08X - D3D10_USAGE_DEFAULT\n", this);
			break;
		case D3D10_USAGE_IMMUTABLE:
			printf("0x%08X - D3D10_USAGE_IMMUTABLE\n", this);
			break;
		case D3D10_USAGE_DYNAMIC:
			printf("0x%08X - D3D10_USAGE_DYNAMIC\n", this);
			break;
		case D3D10_USAGE_STAGING:
			printf("0x%08X - D3D10_USAGE_STAGING\n", this);
			break;
		}
*/

		D3D10_SUBRESOURCE_DATA initial;
		initial.pSysMem = m_providerBinding->GetBackupData();
		initial.SysMemPitch = 0;
		initial.SysMemSlicePitch = 0;
		HRESULT hr = m_d3dDeviceManager->GetDevice()->CreateBuffer(&desc, initial.pSysMem ? &initial : NULL, &m_d3dBuffer);
		if( FAILED(hr) )
		{
			throw std::runtime_error("CreateBuffer failed");
		}

		m_needFlagsRefresh = false;

		// TODO: free host memory
	}
	return m_d3dBuffer;
}

unsigned int PrivateD3D10ProviderData::GetAccessPattern() const
{
	assert(false);
	return ACCESS_PATTERN_NONE;
}

void* PrivateD3D10ProviderData::MapPrivate()
{
	assert(!m_lockedForWrite);
	assert(!m_lockedForReadCount);

	void *result = NULL;
	if( m_d3dBuffer )
	{
		if( FAILED(m_d3dBuffer->Map(D3D10_MAP_WRITE_DISCARD, 0, &result)) || !result )
		{
			throw std::runtime_error("failed to map d3d10 buffer");
		}
#ifndef NDEBUG
		m_lockedForWrite = true;
#endif
	}
	return result;
}

void PrivateD3D10ProviderData::UnmapPrivate()
{
	assert(m_lockedForWrite);
	assert(!m_lockedForReadCount);
	assert(m_d3dBuffer);
	m_d3dBuffer->Unmap();

#ifndef NDEBUG
	m_lockedForWrite = false;
#endif
//	GetMemoryObject()->IncrementContentVersion();
}

///////////////////////////////////////////////////////////////////////////////

ProviderD3D10Impl::ProviderD3D10Impl(DeviceManagerD3D10 *deviceManager)
  : m_d3dDeviceManager(deviceManager)
{
	assert(deviceManager);
}

ProviderD3D10Impl::~ProviderD3D10Impl()
{
}

DeviceManagerD3D10* ProviderD3D10Impl::GetDeviceManager() const throw()
{
	return m_d3dDeviceManager;
}

void ProviderD3D10Impl::CreateAccessorD3D10BufferSRV(MemoryObject *resource, AccessorD3D10BufferSRV **result, DXGI_FORMAT format, D3D10_BUFFER_SRV desc)
{
	AutoPtr<AccessorD3D10BufferSRVImpl> tmp(new AccessorD3D10BufferSRVImpl(GetMyData(resource), format, desc));
	DetachRawPtr(tmp, result);
}

void ProviderD3D10Impl::CreateAccessorD3D10BufferRead(MemoryObject *resource, AccessorD3D10BufferRead **result, UINT d3dBindFlags)
{
	assert(0 == (d3dBindFlags & D3D10_BIND_SHADER_RESOURCE)); // use CreateAccessorD3D10BufferSRV instead
	AutoPtr<AccessorD3D10BufferReadImpl> tmp(new AccessorD3D10BufferReadImpl(GetMyData(resource), d3dBindFlags));
	DetachRawPtr(tmp, result);
}

void ProviderD3D10Impl::CreateAccessorD3D10BufferWrite(MemoryObject *resource, AccessorD3D10BufferWrite **result, UINT d3dBindFlags, Processor *p)
{
	AutoPtr<AccessorD3D10BufferWriteImpl> tmp(new AccessorD3D10BufferWriteImpl(GetMyData(resource), d3dBindFlags));
	DetachRawPtr(tmp, result);

	assert(!resource->GetProcessor());
	resource->GetProviderBinding()->SetProcessor(p);
}

AutoPtr<PrivateD3D10ProviderData> ProviderD3D10Impl::GetMyData(MemoryObject *m)
{
	AutoPtr<PrivateProviderData> result;
	m->GetProviderBinding()->GetPrivateData(this, this, &result);
	return SafePtrCast<PrivateD3D10ProviderData*>(result);
}

PrivateProviderData* ProviderD3D10Impl::NewPrivateData(ProviderBinding *target)
{
	return new PrivateD3D10ProviderData(m_d3dDeviceManager, this, target);
}

///////////////////////////////////////////////////////////////////////////////

namespace collada
{
	void CreateAccessProviderD3D10(AccessProviderD3D10 **ppResult, DeviceManagerD3D10 *deviceManager)
	{
		AutoPtr<DeviceManagerD3D10> devman(deviceManager);
		if( !devman ) CreateDeviceManagerD3D10(&devman);
		AutoPtr<AccessProviderD3D10> result(new ProviderD3D10Impl(devman));
		DetachRawPtr(result, ppResult);
	}
}

// end of file
