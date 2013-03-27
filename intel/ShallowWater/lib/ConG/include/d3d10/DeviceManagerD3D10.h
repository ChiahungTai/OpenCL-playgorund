/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file DeviceManagerD3D10.h

#pragma once

#include "RefCounted.h"

namespace collada
{
	struct IDeviceListenerD3D10
	{
		virtual HRESULT OnSetDevice(ID3D10Device *pd3dDevice) = 0;
	};

	struct DeviceManagerD3D10 : public RefCounted
	{
		virtual HRESULT AddListener(IDeviceListenerD3D10 *ls) = 0;  // TODO: use exceptions for error handling
		virtual void RemoveListener(IDeviceListenerD3D10 *ls) = 0;
		virtual HRESULT SetDevice(ID3D10Device *pd3dDevice) = 0;    // TODO: use exceptions for error handling
		virtual ID3D10Device* GetDevice() = 0;
	};

	void CreateDeviceManagerD3D10(DeviceManagerD3D10 **ppResult);
}

// end of file
