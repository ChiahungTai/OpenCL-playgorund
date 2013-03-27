/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file DeviceManagerD3D10Impl.h

#pragma once

#include "RefCountedImpl.h"
#include "d3d10/DeviceManagerD3D10.h"
#include <set>

using namespace collada;

namespace
{

class DeviceManagerD3D10Impl : public RefCountedImpl<DeviceManagerD3D10>
{
public:
	DeviceManagerD3D10Impl();
	virtual ~DeviceManagerD3D10Impl();

	// DeviceManagerD3D10
	virtual HRESULT AddListener(IDeviceListenerD3D10 *ls);
	virtual void RemoveListener(IDeviceListenerD3D10 *ls);
	virtual HRESULT SetDevice(ID3D10Device *pd3dDevice);
	virtual ID3D10Device* GetDevice();

private:
	typedef std::set<IDeviceListenerD3D10*> DeviceListeners;
	DeviceListeners m_listeners;
	CComPtr<ID3D10Device> m_device;
};

} // end of anonymous namespace
// end of file
