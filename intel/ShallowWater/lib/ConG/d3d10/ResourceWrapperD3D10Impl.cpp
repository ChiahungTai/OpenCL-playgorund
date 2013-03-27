/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file ResourceWrapperD3D10Impl.cpp

#include "stdafx.h"
#include "ResourceWrapperD3D10Impl.h"
#include "IndexCombinerPlugin.h"
#include "SceneGraph.h"
#include "AccessProviderDefault.h"
#include "TgaImage.h"

#include "d3d10/AccessProviderD3D10.h"

#include "EffectManagerD3D10Impl.h"


using namespace collada;

///////////////////////////////////////////////////////////////////////////////

static DXGI_FORMAT DXGIFormatFrom(unsigned int components, ScalarType type)
{
	if( components > 0 && components <= 4 && type < ST_COUNT )
	{
		// grep "enum ScalarType"
		static const DXGI_FORMAT formats[ST_COUNT][4] = {
			//    components = 1         components = 2            components = 3               components = 4
			{DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_R32G32B32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT},  // ST_FLOAT
			{DXGI_FORMAT_R16_FLOAT, DXGI_FORMAT_R16G16_FLOAT, DXGI_FORMAT_UNKNOWN,         DXGI_FORMAT_R16G16B16A16_FLOAT},  // ST_HALF
			{DXGI_FORMAT_R32_SINT,  DXGI_FORMAT_R32G32_SINT,  DXGI_FORMAT_R32G32B32_SINT,  DXGI_FORMAT_R32G32B32A32_SINT},   // ST_SINT32
			{DXGI_FORMAT_R32_UINT,  DXGI_FORMAT_R32G32_UINT,  DXGI_FORMAT_R32G32B32_UINT,  DXGI_FORMAT_R32G32B32A32_UINT},   // ST_UINT32
			{DXGI_FORMAT_R16_SINT,  DXGI_FORMAT_R16G16_SINT,  DXGI_FORMAT_UNKNOWN,         DXGI_FORMAT_R16G16B16A16_SINT},   // ST_SINT16
			{DXGI_FORMAT_R16_UINT,  DXGI_FORMAT_R16G16_UINT,  DXGI_FORMAT_UNKNOWN,         DXGI_FORMAT_R16G16B16A16_UINT},   // ST_UINT16
			{DXGI_FORMAT_R8_SINT,   DXGI_FORMAT_R8G8_SINT,    DXGI_FORMAT_UNKNOWN,         DXGI_FORMAT_R8G8B8A8_SINT},       // ST_SINT8
			{DXGI_FORMAT_R8_UINT,   DXGI_FORMAT_R8G8_UINT,    DXGI_FORMAT_UNKNOWN,         DXGI_FORMAT_R8G8B8A8_UINT},       // ST_UINT8
		};
		return formats[type][components-1];
	}
	return DXGI_FORMAT_UNKNOWN;
}

///////////////////////////////////////////////////////////////////////////////

DXObjectViewBase::DXObjectViewBase(DXObjectsFactory *factory)
  : m_factory(factory)
{
}

DXObjectViewBase::~DXObjectViewBase()
{
}

void DXObjectViewBase::OnDetach()
{
	delete this;
}

///////////////////////////////////////////////////////////////////////////////

DXImageView::DXImageView(DXObjectsFactory *factory, const collada::Image *source)
  : DXObjectViewBase(factory)
  , m_source(source)
{
	GetFactory()->GetProvider()->GetDeviceManager()->AddListener(this);
}

DXImageView::~DXImageView()
{
	GetFactory()->GetProvider()->GetDeviceManager()->RemoveListener(this);
}

ID3D10ShaderResourceView* DXImageView::GetSRV() const
{
	return m_pSRV.p;
}

HRESULT DXImageView::OnSetDevice(ID3D10Device *pd3dDevice)
{
	m_pSRV.Release();
	if( pd3dDevice )
	{
		assert(!m_pSRV);

		LARGE_INTEGER start, stop;
		QueryPerformanceCounter(&start);

		//
		// try one of standard formats
		//
		std::basic_string<TCHAR> sFileName = m_source->GetFileName();
		bool isTGA = 0==_tcsicmp(sFileName.substr(sFileName.length()-4).c_str(), _T(".tga"));

	/*	D3DX10_IMAGE_LOAD_INFO loadInfo;
		loadInfo.Width = D3DX10_DEFAULT;
		loadInfo.Height = D3DX10_DEFAULT;
		loadInfo.Depth = D3DX10_DEFAULT;
		loadInfo.FirstMipLevel = D3DX10_DEFAULT;
		loadInfo.MipLevels = D3DX10_DEFAULT;
		loadInfo.Usage = D3D10_USAGE_IMMUTABLE;
		loadInfo.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		loadInfo.CpuAccessFlags = 0;
		loadInfo.MiscFlags = 0;
		loadInfo.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		loadInfo.Filter = D3DX10_FILTER_NONE | D3DX10_FILTER_SRGB;
		loadInfo.MipFilter = D3DX10_DEFAULT;
		loadInfo.pSrcInfo = NULL;*/
		if( SUCCEEDED(D3DX10CreateShaderResourceViewFromFile(pd3dDevice, m_source->GetFileName(), /*&loadInfo*/NULL, NULL, &m_pSRV, NULL)) )
		{
			QueryPerformanceCounter(&stop);
			GetFactory()->AddImageTiming(start, stop);
			_tprintf(_T("image '%s' loaded with DirectX\n"), m_source->GetFileName());
			return S_OK;
		}

		//
		// try load as tga image
		//

		if( isTGA ) try
		{
			TgaImage img(m_source->GetFileName(), 0);

			D3D10_TEXTURE2D_DESC td;
			td.Width = img.GetWidth();
			td.Height = img.GetHeight();
			td.MipLevels = 1;
			td.ArraySize = 1;
			td.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			td.SampleDesc.Count = 1;
			td.SampleDesc.Quality = 0;
			td.Usage = D3D10_USAGE_DEFAULT;
			td.BindFlags = D3D10_BIND_SHADER_RESOURCE|D3D10_BIND_RENDER_TARGET;
			td.CPUAccessFlags = 0;
			td.MiscFlags = D3D10_RESOURCE_MISC_GENERATE_MIPS;

			D3D10_SUBRESOURCE_DATA data;
			data.pSysMem = img.GetData();
			data.SysMemPitch = img.GetWidth() * img.GetBpp() / 8;
			data.SysMemSlicePitch = 0;

			CComPtr<ID3D10Texture2D> pTex;
			CComPtr<ID3D10Texture2D> pMipTex;

			if( FAILED(pd3dDevice->CreateTexture2D(&td, &data, &pTex)) )
			{
				throw std::runtime_error("CreateTexture2D failed");
			}
			td.MipLevels = 0;
			if( FAILED(pd3dDevice->CreateTexture2D(&td, NULL, &pMipTex)) )
			{
				throw std::runtime_error("CreateTexture2D failed");
			}
			if( FAILED(pd3dDevice->CreateShaderResourceView(pMipTex, NULL, &m_pSRV)) )
			{
				throw std::runtime_error("CreateShaderResourceView failed");
			}
			pd3dDevice->CopySubresourceRegion(pMipTex, 0,0,0,0, pTex,0,NULL);
			pd3dDevice->GenerateMips(m_pSRV);

			QueryPerformanceCounter(&stop);
			GetFactory()->AddImageTiming(start, stop);
			_tprintf(_T("image '%s' loaded with TGA loader\n"), m_source->GetFileName());
			return S_OK;
		}
		catch( const std::exception &e )
		{
			printf("%s\n", e.what());
		}

		_tprintf(_T("Could not load image file %s\n"), m_source->GetFileName());
		return S_FALSE;
	}
	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////

DXBatchView::DXBatchView(DXObjectsFactory *factory, const Batch *source)
  : DXObjectViewBase(factory)
  , m_source(source)
  , m_idxBufferFormat(DXGIFormatFrom(1, ST_UINT32))
{
	assert(1 == source->GetIndexStride()); // unified batch expected
	if( AutoPtr<MemoryObject> tmp = source->GetIndexData() )
	{
		factory->GetProvider()->CreateAccessorD3D10BufferRead(tmp, &m_pIndexBufferAccessor, D3D10_BIND_INDEX_BUFFER);
	}
}

DXBatchView::~DXBatchView()
{
}

D3D10_PRIMITIVE_TOPOLOGY DXBatchView::GetPrimitiveTopology() const
{
	switch( m_source->GetPrimitiveTopology() )
	{
	case PRIMITIVE_TOPOLOGY_LINELIST:
		return D3D10_PRIMITIVE_TOPOLOGY_LINELIST;
	case PRIMITIVE_TOPOLOGY_TRIANGLELIST:
		return D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}

	return D3D10_PRIMITIVE_TOPOLOGY_POINTLIST;
}

ID3D10Buffer* DXBatchView::GetIndexBuffer() const
{
	if( m_pIndexBufferAccessor )
	{
		return m_pIndexBufferAccessor->Map();
	}
	return NULL;
}

UINT DXBatchView::GetIndexCount() const
{
	return (UINT) m_source->GetIndexData()->GetBytesSize() / sizeof(unsigned long);
}

///////////////////////////////////////////////////////////////////////////////

DXMeshView::DXMeshView(DXObjectsFactory *factory, const collada::Mesh *source)
  : DXObjectViewBase(factory)
  , m_source(source)
{
	for( size_t i = 0; i < source->GetVertexBufferCount(); ++i )
	{
		m_offsets.push_back(0);
		m_strides.push_back(static_cast<UINT>(source->GetRawVertexSize(i)));
		m_vertexBufferAccessors.push_back(AccessorsVector::value_type());
		factory->GetProvider()->CreateAccessorD3D10BufferRead(
			m_source->GetVertexBuffer(i), &m_vertexBufferAccessors[i], D3D10_BIND_VERTEX_BUFFER);
	}
	GetFactory()->GetProvider()->GetDeviceManager()->AddListener(this);
}

DXMeshView::~DXMeshView()
{
	GetFactory()->GetProvider()->GetDeviceManager()->RemoveListener(this);
}

HRESULT DXMeshView::OnSetDevice(ID3D10Device *pd3dDevice)
{
	m_vertexBuffersCached10.clear();
	return S_OK;
}

ID3D10Buffer*const* DXMeshView::GetVertexBuffers() const
{
	assert(!m_vertexBufferAccessors.empty());
	m_vertexBuffersCached10.resize(m_vertexBufferAccessors.size());
	for( size_t i = 0; i < m_vertexBufferAccessors.size(); ++i )
	{
		m_vertexBuffersCached10[i] = m_vertexBufferAccessors[i]->Map();
		assert(m_vertexBuffersCached10[i]);
	}
	return &m_vertexBuffersCached10[0];
}

UINT DXMeshView::GetVertexCount() const
{
	// all buffers should have the same number of elements so we may pick any buffer
	return static_cast<UINT>(m_source->GetVertexBuffer(0)->GetBytesSize() / m_source->GetRawVertexSize(0));
}

///////////////////////////////////////////////////////////////////////////////

DXMaterialBindingView::DXMaterialBindingView(DXObjectsFactory *factory, MaterialBinding *source, Scene *context)
  : DXObjectViewBase(factory)
  , m_source(source)
  , m_context(context)
{
	GetFactory()->GetProvider()->GetDeviceManager()->AddListener(this);
	for( size_t i = 0; i < source->GetRequiredInputCount(context); ++i )
	{
		const VertexElementDesc *desc = source->GetRequiredInputs(context) + i;

		D3D10_INPUT_ELEMENT_DESC dxdesc;
		dxdesc.SemanticName = desc->semanticName;
		dxdesc.SemanticIndex = 0;
		dxdesc.Format = DXGIFormatFrom(desc->componentCount, desc->componentType);
		dxdesc.InputSlot = desc->inputSlot;
		dxdesc.AlignedByteOffset = 0;  // we have one buffer per attribute
		dxdesc.InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
		dxdesc.InstanceDataStepRate = 0;
		m_inputElements.push_back(dxdesc);
	}
}

DXMaterialBindingView::~DXMaterialBindingView()
{
	GetFactory()->GetProvider()->GetDeviceManager()->RemoveListener(this);
	assert(!m_d3dInputLayout);
}

const D3D10_INPUT_ELEMENT_DESC* DXMaterialBindingView::GetInputElementDescs() const
{
	return &m_inputElements[0];
}

ID3D10InputLayout* DXMaterialBindingView::GetInputLayout(EffectManagerD3D10Impl *effectMgr)
{
	if( !m_d3dInputLayout )
	{
		// create input layout
		// TODO: group similar input layouts
		D3D10_PASS_DESC desc;
		HRESULT hr;
		V(effectMgr->GetTechnique(m_source, m_context)->GetPassByIndex(0)->GetDesc(&desc));
		V(GetFactory()->GetProvider()->GetDeviceManager()->GetDevice()->CreateInputLayout(
			GetInputElementDescs(), GetInputElementCount(), desc.pIAInputSignature, desc.IAInputSignatureSize, &m_d3dInputLayout));
	}
	return m_d3dInputLayout;
}

UINT DXMaterialBindingView::GetInputElementCount() const
{
	return static_cast<UINT>(m_inputElements.size());
}

HRESULT DXMaterialBindingView::OnSetDevice(ID3D10Device *pd3dDevice)
{
	m_d3dInputLayout.Release();
	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////

DXObjectsFactory::DXObjectsFactory(AccessProviderD3D10 *providerD3D10)
  : m_provider(providerD3D10)
{
	m_imagesLoadTime.QuadPart = 0;
}

DXObjectsFactory::~DXObjectsFactory()
{
}

IView* DXObjectsFactory::CreateView(Scene *context, Base *b, size_t thisViewId)
{
	DXObjectViewBase *result = NULL;
	if( const Image *img = dynamic_cast<const Image *>(b) )
	{
		result = new DXImageView(this, img);
	}
	else
	if( const Batch *batch = dynamic_cast<const Batch *>(b) )
	{
		result = new DXBatchView(this, batch);
	}
	else
	if( const Mesh *mesh = dynamic_cast<const Mesh *>(b) )
	{
		result = new DXMeshView(this, mesh);
	}
	else
	if( MaterialBinding *binding = dynamic_cast<MaterialBinding *>(b) )
	{
		result = new DXMaterialBindingView(this, binding, context);
	}
	return result;
}

void DXObjectsFactory::OnDetach()
{ // do nothing
}

void DXObjectsFactory::AddImageTiming(LARGE_INTEGER qpcStart, LARGE_INTEGER qpcStop)
{
	m_imagesLoadTime.QuadPart += qpcStop.QuadPart - qpcStart.QuadPart;
}


// end of file
