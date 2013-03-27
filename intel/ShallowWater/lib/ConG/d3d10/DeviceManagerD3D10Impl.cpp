/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file DeviceManagerD3D10Impl.cpp

#include "stdafx.h"

#include "DeviceManagerD3D10Impl.h"


DeviceManagerD3D10Impl::DeviceManagerD3D10Impl()
{
}

DeviceManagerD3D10Impl::~DeviceManagerD3D10Impl()
{
	assert(m_listeners.empty());
}

HRESULT DeviceManagerD3D10Impl::AddListener(IDeviceListenerD3D10 *ls)
{
	assert(!m_listeners.count(ls));
	HRESULT hr = S_OK;
	if( m_device )
	{
		hr = ls->OnSetDevice(m_device);
		if( FAILED(hr) )
		{
			return hr; // don't add failed listener
		}
	}
	m_listeners.insert(ls);
	return hr;
}

void DeviceManagerD3D10Impl::RemoveListener(IDeviceListenerD3D10 *ls)
{
	assert(m_listeners.count(ls));
	m_listeners.erase(ls);
	if( m_device )
	{
		HRESULT hr = ls->OnSetDevice(NULL);
		assert(SUCCEEDED(hr)); // detach device should not fail
	}
}

HRESULT DeviceManagerD3D10Impl::SetDevice(ID3D10Device *pd3dDevice)
{
	for( DeviceListeners::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it )
	{
		HRESULT result = (*it)->OnSetDevice(pd3dDevice);
		if( FAILED(result) && pd3dDevice )
		{
			while( m_listeners.begin() != it )
			{
				--it;
				HRESULT hr = (*it)->OnSetDevice(NULL);
				assert(SUCCEEDED(hr)); // detach device should not fail
			}
			return result;
		}
	}
	m_device = pd3dDevice;
	return S_OK;
}

ID3D10Device* DeviceManagerD3D10Impl::GetDevice()
{
	return m_device;
}

///////////////////////////////////////////////////////////////////////////////

// factory
namespace collada
{
	void CreateDeviceManagerD3D10(DeviceManagerD3D10 **ppResult)
	{
		AutoPtr<DeviceManagerD3D10> result(new DeviceManagerD3D10Impl());
		DetachRawPtr(result, ppResult);
	}
}


// end of file
