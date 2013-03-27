/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2009 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file AccessProviderD3D10.h

#pragma once

#include "MemoryManager.h"

namespace collada
{
struct DeviceManagerD3D10;

// may be read with:
//  index buffer
//  vertex buffer
//  constant buffer
//  CopyResource
struct AccessorD3D10BufferRead : public AccessorRead
{
	virtual ID3D10Buffer* Map() = 0;
	virtual void Unmap() = 0;
};

// may be read with:
//  shader resource
struct AccessorD3D10BufferSRV : public AccessorRead
{
	virtual ID3D10ShaderResourceView* Map() = 0;
	virtual void Unmap() = 0;
};

// may be written with:
//  stream out
//  CopyResource
//  UpdateSubresource
struct AccessorD3D10BufferWrite : public AccessorWrite
{
	virtual ID3D10Buffer* Map() = 0;
	virtual void Unmap() = 0;
};

struct AccessProviderD3D10 : public AccessProvider
{
	virtual DeviceManagerD3D10* GetDeviceManager() const throw() = 0;

	virtual void CreateAccessorD3D10BufferSRV(MemoryObject *resource, AccessorD3D10BufferSRV **result, DXGI_FORMAT format, D3D10_BUFFER_SRV desc) = 0;
	virtual void CreateAccessorD3D10BufferRead(MemoryObject *resource, AccessorD3D10BufferRead **result, UINT d3dBindFlags) = 0;
	virtual void CreateAccessorD3D10BufferWrite(MemoryObject *resource, AccessorD3D10BufferWrite **result, UINT d3dBindFlags, Processor *p) = 0;
};

/// You may specify an existing device manager or default one will be created otherwise
void CreateAccessProviderD3D10(AccessProviderD3D10 **ppResult, DeviceManagerD3D10 *deviceManager = NULL);

} // end of namespace collada
// end of file
