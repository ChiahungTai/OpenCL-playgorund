/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file ResourceWrapperD3D10Impl.h

#pragma once

#include "d3d10/DeviceManagerD3D10.h"
#include "SceneGraphBase.h"
#include "AutoPtr.h"

///////////////////////////////////////////////////////////////////////////////

// forward declarations

class DXObjectsFactory;
namespace collada
{
	struct AccessProviderD3D10;
	struct AccessorD3D10BufferRead;
}

///////////////////////////////////////////////////////////////////////////////

class DXObjectViewBase : public collada::IView
{
protected:
	DXObjectViewBase(DXObjectsFactory *factory);
	virtual ~DXObjectViewBase();

	DXObjectsFactory* GetFactory() const { return m_factory; }

protected:
	// IView
	virtual void OnDetach();

private:
	DXObjectsFactory *m_factory;

	DXObjectViewBase(const DXObjectViewBase&); // no copy
	DXObjectViewBase& operator = (const DXObjectViewBase&);
};

///////////////////////////////////////////////////////////////////////////////

class DXImageView
	: public DXObjectViewBase
	, private collada::IDeviceListenerD3D10
{
public:
	DXImageView(DXObjectsFactory *factory, const collada::Image *source);
	virtual ~DXImageView();

	ID3D10ShaderResourceView* GetSRV() const;

protected:
	// IDeviceListenerD3D10
	virtual HRESULT OnSetDevice(ID3D10Device *pd3dDevice);

private:
	const collada::Image *m_source;
	CComPtr<ID3D10ShaderResourceView> m_pSRV;
};

///////////////////////////////////////////////////////////////////////////////

class DXBatchView : public DXObjectViewBase
{
public:
	DXBatchView(DXObjectsFactory *factory, const collada::Batch *source);
	virtual ~DXBatchView();

	D3D10_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const;

	// index buffer
	ID3D10Buffer* GetIndexBuffer() const;
	UINT GetIndexCount() const;
	inline DXGI_FORMAT GetIndexBufferFormat() const
	{
		return m_idxBufferFormat;
	}

private:
	const collada::Batch *m_source;

	// index buffer
	DXGI_FORMAT m_idxBufferFormat;
	collada::AutoPtr<collada::AccessorD3D10BufferRead> m_pIndexBufferAccessor;
};

///////////////////////////////////////////////////////////////////////////////

class DXMeshView
	: public DXObjectViewBase
	, private collada::IDeviceListenerD3D10
{
public:
	DXMeshView(DXObjectsFactory *factory, const collada::Mesh *source);
	virtual ~DXMeshView();

	// shader vertex buffers
	inline const UINT* GetStrides() const
	{
		return &m_strides[0];
	}
	inline const UINT* GetOffsets() const
	{
		return &m_offsets[0];
	}
	inline UINT GetVertexBuffersCount() const
	{
		return (UINT) m_vertexBufferAccessors.size();
	}

	ID3D10Buffer*const* GetVertexBuffers() const;
	UINT GetVertexCount() const;

private:
	// IDeviceListenerD3D10
	virtual HRESULT OnSetDevice(ID3D10Device *pd3dDevice);

private:
	const collada::Mesh *m_source;
	typedef std::vector<collada::AutoPtr<collada::AccessorD3D10BufferRead>> AccessorsVector;
	AccessorsVector m_vertexBufferAccessors;

	mutable std::vector<ID3D10Buffer*> m_vertexBuffersCached10;
	std::vector<UINT> m_strides;
	std::vector<UINT> m_offsets;
};

///////////////////////////////////////////////////////////////////////////////

class DXMaterialBindingView
	: public DXObjectViewBase
	, private collada::IDeviceListenerD3D10
{
public:
	// create and destroy
	DXMaterialBindingView(DXObjectsFactory *factory, collada::MaterialBinding *source, collada::Scene *context);
	~DXMaterialBindingView();

	// input layout
	ID3D10InputLayout* GetInputLayout(class EffectManagerD3D10Impl *effectMgr);
	const D3D10_INPUT_ELEMENT_DESC* GetInputElementDescs() const;
	UINT GetInputElementCount() const;

private:
	// IDeviceListenerD3D10
	virtual HRESULT OnSetDevice(ID3D10Device *pd3dDevice);

private:
	collada::MaterialBinding *m_source;
	collada::Scene *m_context;
	std::vector<D3D10_INPUT_ELEMENT_DESC> m_inputElements;
	CComPtr<ID3D10InputLayout> m_d3dInputLayout;
};

///////////////////////////////////////////////////////////////////////////////

class DXObjectsFactory
	: public collada::IViewFactory
{
public:
	DXObjectsFactory(collada::AccessProviderD3D10 *providerD3D10);
	virtual ~DXObjectsFactory();

	collada::AccessProviderD3D10* GetProvider() const { return m_provider; }

	void AddImageTiming(LARGE_INTEGER qpcStart, LARGE_INTEGER qpcStop);
	LARGE_INTEGER GetImagesLoadTimeTotal() const { return m_imagesLoadTime; }


protected:
	// IViewFactory implementation
	virtual collada::IView* CreateView(collada::Scene *context, collada::Base *b, size_t thisViewId);
	virtual void OnDetach();

private:
	collada::AutoPtr<collada::AccessProviderD3D10> m_provider;
	LARGE_INTEGER m_imagesLoadTime;
};


// end of file
