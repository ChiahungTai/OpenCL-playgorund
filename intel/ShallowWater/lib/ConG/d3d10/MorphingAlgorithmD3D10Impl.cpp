/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file MorphingAlgorithmD3D10Impl.cpp

#include "stdafx.h"
#include "mathstub.h"
#include "MorphingAlgorithmD3D10Impl.h"
#include "AccessProviderDefault.h"

#include "d3d10/AccessProviderD3D10.h"

static const char g_shaderMorphing[] =
	#include "MorphingD3D10.fx.h"
;

CommonResourcePool::CommonResourcePool( AccessProviderD3D10 *providerD3D10, IMessageListener *msg )
	: m_msg(msg)
	, m_provider(providerD3D10)
	, m_effect(NULL)
	, m_techniqueMorphing(NULL)
	, m_targetsVar(NULL)
	, m_weightsVar(NULL)
	, m_numBS(NULL)
{
	HRESULT hr = m_provider->GetDeviceManager()->AddListener(this);
	if( FAILED(hr) )
	{
		throw std::runtime_error("failed to initialize MorphingAlgorithmD3D10Impl");
	}
}

CommonResourcePool::~CommonResourcePool()
{
	m_provider->GetDeviceManager()->RemoveListener(this);
	assert(!m_effect);
}

HRESULT CommonResourcePool::OnSetDevice( ID3D10Device *pd3dDevice )
{
	m_inputLayoutMorphing.Release();
	m_effect.Release();
	m_stateBlock.Release();

	if( pd3dDevice )
	{
		//
		// compile effect
		//

		CComPtr<ID3D10Blob> pBlob;
		HRESULT hr = D3DX10CreateEffectFromMemory(
			&g_shaderMorphing,                  // pData
			sizeof(g_shaderMorphing),           // DataLength
			"MorphingD3D10.fx.h",               // pSrcFileName
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
		// acquire techniques and variables
		//

		m_techniqueMorphing = m_effect->GetTechniqueByName("Morphing");
		m_targetsVar = m_effect->GetVariableByName("g_targets")->AsShaderResource();
		m_weightsVar = m_effect->GetVariableByName("g_weights")->AsShaderResource();
		m_numBS = m_effect->GetVariableByName("numBS")->AsScalar();

		assert(m_techniqueMorphing && m_techniqueMorphing->IsValid());
		assert(m_targetsVar && m_targetsVar->IsValid());
		assert(m_weightsVar && m_weightsVar->IsValid());

		//
		// create input layouts
		//
		{
			const D3D10_INPUT_ELEMENT_DESC elements[] =
			{
				{"DATA", 0, DXGI_FORMAT_R32G32B32_UINT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0},
			};
			D3D10_PASS_DESC pd;
			m_techniqueMorphing->GetPassByIndex(0)->GetDesc(&pd);
			pd3dDevice->CreateInputLayout(elements, sizeof(elements) / sizeof(elements[0]), 
				pd.pIAInputSignature, pd.IAInputSignatureSize, &m_inputLayoutMorphing);
		}

		//
		// create state blocks
		//

		D3D10_STATE_BLOCK_MASK mask = {0};
		D3D10StateBlockMaskEnableCapture(&mask, D3D10_DST_SO_BUFFERS, 0, 1);
		D3D10StateBlockMaskEnableCapture(&mask, D3D10_DST_IA_PRIMITIVE_TOPOLOGY, 0, 1);
		D3D10StateBlockMaskEnableCapture(&mask, D3D10_DST_IA_INDEX_BUFFER, 0, 1);
		D3D10StateBlockMaskEnableCapture(&mask, D3D10_DST_IA_INPUT_LAYOUT, 0, 1);
		D3D10StateBlockMaskEnableCapture(&mask, D3D10_DST_IA_VERTEX_BUFFERS, 0, 2);
		D3D10CreateStateBlock(pd3dDevice, &mask, &m_stateBlock);

	}
	else
	{
		m_techniqueMorphing = NULL;
		m_targetsVar = NULL;
		m_weightsVar = NULL;
		m_numBS = NULL;
	}

	return S_OK;
}

MorphingAlgorithmD3D10Impl::MorphingAlgorithmD3D10Impl( AccessProviderD3D10 *providerD3D10, AccessProviderLocal *providerLocal, IMessageListener *msg )
	: m_msg(msg)
	, m_resPool(new CommonResourcePool(providerD3D10, msg))
	, m_providerLocal(providerLocal)
{
}

MorphingAlgorithmD3D10Impl::~MorphingAlgorithmD3D10Impl()
{
}

void MorphingAlgorithmD3D10Impl::DoProcess( const AlgorithmResourceAccessor *accessors ) const
{
	FindAccessor<AccessorD3D10BufferRead> inBaseBufferAccessor(accessors, IN_BASEBUFFER);
	FindAccessor<AccessorD3D10BufferSRV> inTargetsAccessor(accessors, IN_TARGETSBUFFERS);
	FindAccessor<AccessorD3D10BufferSRV> inWeightsAccessor(accessors, IN_WEIGHTS);
	FindAccessor<AccessorD3D10BufferWrite> outBufferAccessor(accessors, OUT_BUFFER);

	ID3D10Buffer *baseBuffer = inBaseBufferAccessor->Map();
	ID3D10ShaderResourceView *targetsSRV = inTargetsAccessor->Map();
	ID3D10ShaderResourceView *weightsSRV = inWeightsAccessor->Map();
	ID3D10Buffer *outBuffer = outBufferAccessor->Map();

	UINT vtxStride = sizeof(float3);
	UINT vtxOffset = 0;

	// capture device state to restore it later
	m_resPool->GetStateBlock()->Capture();

	m_resPool->GetTargetsVar()->SetResource(targetsSRV);
	m_resPool->GetWeightsVar()->SetResource(weightsSRV);
	size_t count = inBaseBufferAccessor->GetMemoryObject()->GetBytesSize() / sizeof(float3);
	m_resPool->GetNumBSVar()->SetInt(count);
	m_resPool->GetTechniqueMorphing()->GetPassByIndex(0)->Apply(0);

	ID3D10Device *device = m_resPool->GetAccessProvider()->GetDeviceManager()->GetDevice();
	device->SOSetTargets(1, &outBuffer, &vtxOffset);
	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
	device->IASetVertexBuffers(0, 1, &baseBuffer, &vtxStride, &vtxOffset);
	device->IASetInputLayout(m_resPool->GetInputLayoutMorphing());
	device->Draw(static_cast<UINT>(count), 0);

	// restore device state
	m_resPool->GetStateBlock()->Apply();

	// commit modified data
	static_cast<AccessorD3D10BufferWrite*>(outBufferAccessor)->Unmap();
}

void MorphingAlgorithmD3D10Impl::CreateAccessors( const AlgorithmResource *inputs, IResourceTracker *tracker, Processor *p ) const
{
	for( const AlgorithmResource *it = inputs; it->semanticId != -1; ++it )
	{
		switch( it->semanticId )
		{
		case IN_BASEBUFFER:
			{
				AutoPtr<AccessorD3D10BufferRead> reader;
				m_resPool->GetAccessProvider()->CreateAccessorD3D10BufferRead(it->data, &reader, D3D10_BIND_VERTEX_BUFFER);
				tracker->OnCreateReadAccessor(it->semanticId, reader);
			}
			break;
		case IN_TARGETSBUFFERS:
			{
				AutoPtr<AccessorD3D10BufferSRV> reader;
				D3D10_BUFFER_SRV desc;
				desc.ElementOffset = 0;
				desc.ElementWidth = static_cast<UINT>(it->data->GetBytesSize() / sizeof(float3));
				m_resPool->GetAccessProvider()->CreateAccessorD3D10BufferSRV(it->data, &reader, DXGI_FORMAT_R32G32B32_FLOAT, desc);
				tracker->OnCreateReadAccessor(it->semanticId, reader);
			}
			break;
		case IN_WEIGHTS:
			{
				AutoPtr<AccessorD3D10BufferSRV> reader;
				D3D10_BUFFER_SRV desc;
				desc.ElementOffset = 0;
				desc.ElementWidth = static_cast<UINT>(it->data->GetBytesSize() / sizeof(float));
				m_resPool->GetAccessProvider()->CreateAccessorD3D10BufferSRV(it->data, &reader, DXGI_FORMAT_R32_FLOAT, desc);
				tracker->OnCreateReadAccessor(it->semanticId, reader);
			}
			break;
		case OUT_BUFFER:
			{
				AutoPtr<AccessorD3D10BufferWrite> writer;
				m_resPool->GetAccessProvider()->CreateAccessorD3D10BufferWrite(it->data, &writer, D3D10_BIND_STREAM_OUTPUT, p);
				tracker->OnCreateWriteAccessor(it->semanticId, writer);
			}
			break;
		default:
			assert(false);
		}
	}
}

// factory
namespace collada
{
	void CreateMorphingAlgorithmD3D10( AccessProviderD3D10 *providerD3D10, AccessProviderLocal *providerLocal, IMessageListener *msg, IMorphingAlgorithm **ppResult )
	{
		AutoPtr<IMorphingAlgorithm> tmp(new MorphingAlgorithmD3D10Impl(providerD3D10, providerLocal, msg));
		DetachRawPtr(tmp, ppResult);
	}
}

// end of file

