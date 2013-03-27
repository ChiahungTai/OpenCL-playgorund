/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
        Copyright (C) 2009-2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file IndexCombinerAlgorithmD3D10Impl.cpp

#include "stdafx.h"
#include "IndexCombinerAlgorithmD3D10Impl.h"
#include "d3d10/AccessProviderD3D10.h"

static const char g_shaderIndexCombiner[] =
	#include "IndexCombinerD3D10.fx.h"
;


IndexCombinerAlgorithmD3D10Impl::IndexCombinerAlgorithmD3D10Impl(AccessProviderD3D10 *provider, IMessageListener *msg)
  : m_msg(msg)
  , m_provider(provider)
  , m_technique(NULL)
{
	ZeroMemory(m_layouts, sizeof(m_layouts));
	HRESULT hr = m_provider->GetDeviceManager()->AddListener(this);
	if( FAILED(hr) )
	{
		throw std::runtime_error("failed to initialize IndexCombinerAlgorithmD3D10Impl");
	}
}

IndexCombinerAlgorithmD3D10Impl::~IndexCombinerAlgorithmD3D10Impl()
{
	m_provider->GetDeviceManager()->RemoveListener(this);
	assert(!m_effect);
}

void IndexCombinerAlgorithmD3D10Impl::Execute(const AlgorithmResourceAccessor *accessors, unsigned long *vertexOffsets, size_t taskCount, unsigned int vertexStride)
{
	assert(m_technique);
	assert(taskCount);

	FindAccessor<AccessorD3D10BufferRead> inVertexBufferAccessor(accessors, IN_VERTEX_BUFFER);
	FindAccessor<AccessorD3D10BufferWrite> outVertexBufferAccessor(accessors, OUT_VERTEX_BUFFER);

	ID3D10Buffer *vtxBufferIn = inVertexBufferAccessor->Map();
	ID3D10Buffer *vtxBufferOut = outVertexBufferAccessor->Map();

	UINT vtxStride = vertexStride;
	UINT vtxOffset = 0;

	CComPtr<ID3D10Device> device = m_provider->GetDeviceManager()->GetDevice();
	m_stateBlock->Capture();
	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
	device->IASetVertexBuffers(0, 1, &vtxBufferIn, &vtxStride, &vtxOffset);

	// setup shaders and data width
	unsigned int nFloats = vtxStride / 4;
	assert(vtxStride % 4 == 0);
	assert(nFloats >= 1 && nFloats <= 4);
	m_technique->GetPassByIndex(0)->Apply(0);
	m_technique->GetPassByIndex(nFloats)->Apply(0);
	device->IASetInputLayout(m_layouts[nFloats - 1]);

	for( unsigned int taskIdx = 0; taskIdx < taskCount; ++taskIdx )
	{
		FindAccessor<AccessorD3D10BufferRead> inIndexBufferAccessor(accessors, IN_INDEX_BUFFER, taskIdx);
		ID3D10Buffer *idxBuffer = inIndexBufferAccessor->Map();

		// set input data
		device->IASetIndexBuffer(idxBuffer, DXGI_FORMAT_R32_UINT, 0);

		// set output data
		UINT dstBytesOffset = vertexOffsets[taskIdx] * vtxStride;
		device->SOSetTargets(1, &vtxBufferOut, &dstBytesOffset);

		// execute algorithm
		UINT idxCount = static_cast<UINT>(inIndexBufferAccessor->GetMemoryObject()->GetBytesSize() / sizeof(unsigned long));
		device->DrawIndexed(idxCount, 0, 0);
	}

	m_stateBlock->Apply(); // restore device state
	outVertexBufferAccessor->Unmap();
}

void IndexCombinerAlgorithmD3D10Impl::CreateAccessors(const AlgorithmResource *inputs, IResourceTracker *tracker, Processor *p) const
{
	for( ;inputs->semanticId != -1; ++inputs )
	{
		switch( inputs->semanticId )
		{
		case IN_VERTEX_BUFFER:
			{
				AutoPtr<AccessorD3D10BufferRead> reader;
				m_provider->CreateAccessorD3D10BufferRead(inputs->data, &reader, D3D10_BIND_VERTEX_BUFFER);
				tracker->OnCreateReadAccessor(inputs->semanticId, reader);
			}
			break;
		case IN_INDEX_BUFFER:
			{
				AutoPtr<AccessorD3D10BufferRead> reader;
				m_provider->CreateAccessorD3D10BufferRead(inputs->data, &reader, D3D10_BIND_INDEX_BUFFER);
				tracker->OnCreateReadAccessor(inputs->semanticId, reader);
			}
			break;
		case OUT_VERTEX_BUFFER:
			{
				AutoPtr<AccessorD3D10BufferWrite> writer;
				m_provider->CreateAccessorD3D10BufferWrite(inputs->data, &writer, D3D10_BIND_STREAM_OUTPUT, p);
				tracker->OnCreateWriteAccessor(inputs->semanticId, writer);
			}
			break;
		default:
			assert(false);
		}
	}
}

HRESULT IndexCombinerAlgorithmD3D10Impl::OnSetDevice(ID3D10Device *pd3dDevice)
{
	m_stateBlock.Release();
	m_effect.Release();
	for( unsigned int i = 0; i < 4; ++i )
	{
		m_layouts[i].Release();
	}

	if( pd3dDevice )
	{
		//
		// compile effect
		//

		CComPtr<ID3D10Blob> pBlob;
		HRESULT hr = D3DX10CreateEffectFromMemory(
			&g_shaderIndexCombiner,             // pData
			sizeof(g_shaderIndexCombiner),      // DataLength
			"IndexCombinerD3D10.fx.h",          // pSrcFileName
			NULL,                               // pDefines
			NULL,                               // pInclude
			"fx_4_0",                           // pProfile
			D3D10_SHADER_ENABLE_STRICTNESS,     // HLSLFlags
			0,                                  // FXFlags
			pd3dDevice,                         // pDevice
			NULL,                               // pEffectPool
			NULL,                               // pPump
			&m_effect,                          // [out] ppEffect
			&pBlob,                             // [out] ppErrors
			NULL                                // [out] pHResult
		);
		if( pBlob )
		{
			m_msg(MSG_SEVERITY_WARNING) << "Shader compiler says:\n" << (const char *) pBlob->GetBufferPointer();
		}
		assert(SUCCEEDED(hr));
		assert(m_effect->IsValid());


		//
		// acquire technique and variables
		//

		m_technique = m_effect->GetTechniqueByName("DoProcess");
		assert(m_technique && m_technique->IsValid());


		//
		// create input layouts for various data widths
		//

		D3D10_INPUT_ELEMENT_DESC inputDescs[4] = {
		// SemanticName, SemanticIndex, Format,     InputSlot, AlignedByteOffset, InputSlotClass, InstanceDataStepRate
			{"DATA",     0,  DXGI_FORMAT_R32_UINT,          0, 0,    D3D10_INPUT_PER_VERTEX_DATA, 0},
			{"DATA",     0,  DXGI_FORMAT_R32G32_UINT,       0, 0,    D3D10_INPUT_PER_VERTEX_DATA, 0},
			{"DATA",     0,  DXGI_FORMAT_R32G32B32_UINT,    0, 0,    D3D10_INPUT_PER_VERTEX_DATA, 0},
			{"DATA",     0,  DXGI_FORMAT_R32G32B32A32_UINT, 0, 0,    D3D10_INPUT_PER_VERTEX_DATA, 0}
		};
		for( UINT i = 0; i < 4; ++i )
		{
			D3D10_PASS_DESC pd;
			m_technique->GetPassByIndex(i + 1)->GetDesc(&pd);
			pd3dDevice->CreateInputLayout(inputDescs + i, 1, pd.pIAInputSignature, pd.IAInputSignatureSize, &m_layouts[i]);
		}

		D3D10_STATE_BLOCK_MASK mask = {0};
		m_technique->ComputeStateBlockMask(&mask);
		V(D3D10StateBlockMaskEnableCapture(&mask, D3D10_DST_IA_INDEX_BUFFER, 0, 1));
		V(D3D10StateBlockMaskEnableCapture(&mask, D3D10_DST_IA_INPUT_LAYOUT, 0, 1));
		V(D3D10StateBlockMaskEnableCapture(&mask, D3D10_DST_IA_PRIMITIVE_TOPOLOGY, 0, 1));
		V(D3D10StateBlockMaskEnableCapture(&mask, D3D10_DST_SO_BUFFERS, 0, 1));
		V(D3D10StateBlockMaskEnableCapture(&mask, D3D10_DST_IA_VERTEX_BUFFERS, 0, 1));
		V(D3D10CreateStateBlock(pd3dDevice, &mask, &m_stateBlock));
	}
	else
	{
		m_technique = NULL;
	}

	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////

// factory
namespace collada
{
	void CreateIndexCombinerAlgorithmD3D10(AccessProviderD3D10 *providerD3D10, IMessageListener *msg, IIndexCombinerAlgorithm **ppResult)
	{
		AutoPtr<IIndexCombinerAlgorithm> result(new IndexCombinerAlgorithmD3D10Impl(providerD3D10, msg));
		DetachRawPtr(result, ppResult);
	}
}


// end of file
