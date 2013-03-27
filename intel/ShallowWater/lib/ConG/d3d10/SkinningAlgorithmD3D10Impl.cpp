/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SkinningAlgorithmD3D10Impl.cpp

#include "stdafx.h"
#include "mathstub.h"
#include "SkinningAlgorithmD3D10Impl.h"
#include "AccessProviderDefault.h"
#include "SceneGraph.h"
#include "ProcessorBaseImpl.h"

#include "d3d10/AccessProviderD3D10.h"


static const char g_shaderSkinning[] =
	#include "SkinningD3D10.fx.h"
;


CommonResourcePool::CommonResourcePool(AccessProviderD3D10 *providerD3D10, IMessageListener *msg)
  : m_msg(msg)
  , m_provider(providerD3D10)
  , m_techniqueComputeMatrices(NULL)
  , m_techniqueTransformPositions(NULL)
  , m_techniqueTransformNormals(NULL)
  , m_techniqueSerialize3(NULL)
  , m_techniqueSerialize16(NULL)
  , m_bindShapeVar(NULL)
  , m_jointsVar(NULL)
  , m_weightsVar(NULL)
{
	HRESULT hr = m_provider->GetDeviceManager()->AddListener(this);
	if( FAILED(hr) )
	{
		throw std::runtime_error("failed to initialize SkinningAlgorithmD3D10Impl");
	}
}

CommonResourcePool::~CommonResourcePool()
{
	m_provider->GetDeviceManager()->RemoveListener(this);
	assert(!m_effect);
}

HRESULT CommonResourcePool::OnSetDevice(ID3D10Device *pd3dDevice)
{
	m_inputLayoutMatrices.Release();
	m_inputLayoutTransform.Release();
	m_inputLayoutSerialize3.Release();
	m_inputLayoutSerialize16.Release();
	m_effect.Release();
	m_stateBlock.Release();

	if( pd3dDevice )
	{
		//
		// compile effect
		//

		CComPtr<ID3D10Blob> pBlob;
		HRESULT hr = D3DX10CreateEffectFromMemory(
			&g_shaderSkinning,                  // pData
			sizeof(g_shaderSkinning),           // DataLength
			"SkinningD3D10.fx.h",               // pSrcFileName
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

		m_techniqueComputeMatrices = m_effect->GetTechniqueByName("ComputeMatrices");
		m_techniqueTransformPositions = m_effect->GetTechniqueByName("TransformPositions");
		m_techniqueTransformNormals = m_effect->GetTechniqueByName("TransformNormals");
		m_techniqueSerialize3 = m_effect->GetTechniqueByName("Serialize3");
		m_techniqueSerialize16 = m_effect->GetTechniqueByName("Serialize16");
		m_bindShapeVar = m_effect->GetVariableByName("g_bindShape")->AsMatrix();
		m_jointsVar = m_effect->GetVariableByName("g_joints")->AsShaderResource();
		m_weightsVar = m_effect->GetConstantBufferByName("WeightedJoints");

		assert(m_techniqueComputeMatrices && m_techniqueComputeMatrices->IsValid());
		assert(m_techniqueTransformPositions && m_techniqueTransformPositions->IsValid());
		assert(m_techniqueTransformNormals && m_techniqueTransformNormals->IsValid());
		assert(m_techniqueSerialize3 && m_techniqueSerialize3->IsValid());
		assert(m_techniqueSerialize16 && m_techniqueSerialize16->IsValid());
		assert(m_bindShapeVar && m_bindShapeVar->IsValid());
		assert(m_jointsVar && m_jointsVar->IsValid());
		assert(m_weightsVar && m_weightsVar->IsValid());


		//
		// create input layouts
		//

		{
			const D3D10_INPUT_ELEMENT_DESC elements[] =
			{
				{"BINDINGS",  0, DXGI_FORMAT_R32G32_UINT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0},
			};
			D3D10_PASS_DESC pd;
			m_techniqueComputeMatrices->GetPassByIndex(0)->GetDesc(&pd);
			pd3dDevice->CreateInputLayout(elements, sizeof(elements) / sizeof(elements[0]), 
				pd.pIAInputSignature, pd.IAInputSignatureSize, &m_inputLayoutMatrices);
		}
		{
			const D3D10_INPUT_ELEMENT_DESC elements[] =
			{
				{"DATA",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0},
				{"TRANSFORM", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0},
				{"TRANSFORM", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0},
				{"TRANSFORM", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0},
				{"TRANSFORM", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0},
			};
			D3D10_PASS_DESC pd;
			m_techniqueTransformPositions->GetPassByIndex(0)->GetDesc(&pd);
			pd3dDevice->CreateInputLayout(elements, sizeof(elements) / sizeof(elements[0]), 
				pd.pIAInputSignature, pd.IAInputSignatureSize, &m_inputLayoutTransform);
		}
		{
			const D3D10_INPUT_ELEMENT_DESC elements[] =
			{
				{"DATA", 0, DXGI_FORMAT_R32G32B32_UINT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0},
			};
			D3D10_PASS_DESC pd;
			m_techniqueSerialize3->GetPassByIndex(0)->GetDesc(&pd);
			pd3dDevice->CreateInputLayout(elements, sizeof(elements) / sizeof(elements[0]), 
				pd.pIAInputSignature, pd.IAInputSignatureSize, &m_inputLayoutSerialize3);
		}
		{
			const D3D10_INPUT_ELEMENT_DESC elements[] =
			{
				{"DATA", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0},
				{"DATA", 1, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0},
				{"DATA", 2, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0},
				{"DATA", 3, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0},
			};
			D3D10_PASS_DESC pd;
			m_techniqueSerialize16->GetPassByIndex(0)->GetDesc(&pd);
			pd3dDevice->CreateInputLayout(elements, sizeof(elements) / sizeof(elements[0]), 
				pd.pIAInputSignature, pd.IAInputSignatureSize, &m_inputLayoutSerialize16);
		}


		//
		// create state blocks
		//

		D3D10_STATE_BLOCK_MASK mask = {0};
		m_techniqueComputeMatrices->ComputeStateBlockMask(&mask);
		m_techniqueTransformPositions->ComputeStateBlockMask(&mask);
		m_techniqueTransformNormals->ComputeStateBlockMask(&mask);
		m_techniqueSerialize3->ComputeStateBlockMask(&mask);
		m_techniqueSerialize16->ComputeStateBlockMask(&mask);
		D3D10StateBlockMaskEnableCapture(&mask, D3D10_DST_SO_BUFFERS, 0, 1);
		D3D10StateBlockMaskEnableCapture(&mask, D3D10_DST_IA_PRIMITIVE_TOPOLOGY, 0, 1);
		D3D10StateBlockMaskEnableCapture(&mask, D3D10_DST_IA_INDEX_BUFFER, 0, 1);
		D3D10StateBlockMaskEnableCapture(&mask, D3D10_DST_IA_INPUT_LAYOUT, 0, 1);
		D3D10StateBlockMaskEnableCapture(&mask, D3D10_DST_IA_VERTEX_BUFFERS, 0, 2);
		D3D10CreateStateBlock(pd3dDevice, &mask, &m_stateBlock);
	}
	else
	{
		m_techniqueComputeMatrices = NULL;
		m_techniqueTransformPositions = NULL;
		m_techniqueTransformNormals = NULL;
		m_techniqueSerialize3 = NULL;
		m_techniqueSerialize16 = NULL;
		m_bindShapeVar = NULL;
		m_jointsVar = NULL;
		m_weightsVar = NULL;
	}

	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////

AttributeSerializerAlgorithmD3D10Impl::AttributeSerializerAlgorithmD3D10Impl(CommonResourcePool *resourcePool, 
                                                                   IMessageListener *msg,
                                                                   bool ifTrueThen3Else16)
  : m_msg(msg)
  , m_resPool(resourcePool)
  , m_IfTrueThen3Else16(ifTrueThen3Else16)
{
}

void AttributeSerializerAlgorithmD3D10Impl::DoProcess(const AlgorithmResourceAccessor *accessors) const
{
	FindAccessor<AccessorD3D10BufferRead> inIndexAccessor(accessors, IN_INDEX);
	FindAccessor<AccessorD3D10BufferRead> inDataAccessor(accessors, IN_DATA);
	FindAccessor<AccessorD3D10BufferWrite> outDataAccessor(accessors, OUT_DATA);

	assert(inIndexAccessor->GetMemoryObject()->GetBytesSize() % sizeof(unsigned long) == 0);

	ID3D10Buffer *vtxBufferOut = outDataAccessor->Map();
	ID3D10Buffer *vtxBufferInput = inDataAccessor->Map();
	ID3D10Buffer *idxBuffer = inIndexAccessor->Map();

	UINT vtxStride = m_IfTrueThen3Else16 ? sizeof(float3) : sizeof(float4x4);
	UINT vtxOffset = 0;

	// capture device state to restore it later
	m_resPool->GetStateBlock()->Capture();

	(m_IfTrueThen3Else16 ? m_resPool->GetTechniqueSerialize3() : m_resPool->GetTechniqueSerialize16())->GetPassByIndex(0)->Apply(0);

	ID3D10Device *device = m_resPool->GetAccessProvider()->GetDeviceManager()->GetDevice();
	device->SOSetTargets(1, &vtxBufferOut, &vtxOffset);
	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
	device->IASetIndexBuffer(idxBuffer, DXGI_FORMAT_R32_UINT, 0);
	device->IASetVertexBuffers(0, 1, &vtxBufferInput, &vtxStride, &vtxOffset);
	device->IASetInputLayout(m_IfTrueThen3Else16 ? m_resPool->GetInputLayoutSerialize3() : m_resPool->GetInputLayoutSerialize16());
	device->DrawIndexed(static_cast<UINT>(inIndexAccessor->GetMemoryObject()->GetBytesSize() / sizeof(unsigned long)), 0, 0);

	// restore device state
	m_resPool->GetStateBlock()->Apply();

	// commit modified data
	static_cast<AccessorD3D10BufferWrite*>(outDataAccessor)->Unmap();
}

void AttributeSerializerAlgorithmD3D10Impl::CreateAccessors(const AlgorithmResource *inputs, IResourceTracker *tracker, Processor *p) const
{
	for( ;inputs->semanticId != -1; ++inputs )
	{
		switch( inputs->semanticId )
		{
		case IN_INDEX:
			{
				AutoPtr<AccessorD3D10BufferRead> reader;
				m_resPool->GetAccessProvider()->CreateAccessorD3D10BufferRead(inputs->data, &reader, D3D10_BIND_INDEX_BUFFER);
				tracker->OnCreateReadAccessor(inputs->semanticId, reader);
			}
			break;
		case IN_DATA:
			{
				AutoPtr<AccessorD3D10BufferRead> reader;
				m_resPool->GetAccessProvider()->CreateAccessorD3D10BufferRead(inputs->data, &reader, D3D10_BIND_VERTEX_BUFFER);
				tracker->OnCreateReadAccessor(inputs->semanticId, reader);
			}
			break;
		case OUT_DATA:
			{
				AutoPtr<AccessorD3D10BufferWrite> writer;
				m_resPool->GetAccessProvider()->CreateAccessorD3D10BufferWrite(inputs->data, &writer, D3D10_BIND_STREAM_OUTPUT, p);
				tracker->OnCreateWriteAccessor(inputs->semanticId, writer);
			}
			break;
		default:
			assert(false);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

SkinningAlgorithmD3D10Impl::SkinningAlgorithmD3D10Impl(AccessProviderD3D10 *providerD3D10, AccessProviderLocal *providerLocal, IMessageListener *msg)
  : m_msg(msg)
  , m_resPool(new CommonResourcePool(providerD3D10, msg))
  , m_providerLocal(providerLocal)
{
}

SkinningAlgorithmD3D10Impl::~SkinningAlgorithmD3D10Impl()
{
}

void SkinningAlgorithmD3D10Impl::ComputeJoints(const AlgorithmResourceAccessor *accessors)
{
	MapHelper<const Joint, AccessorReadDefault> joints(FindAccessor<AccessorReadDefault>(accessors, IN_JOINTS));
	MapHelper<float4x4, AccessorWriteDefault> jointMatrices(FindAccessor<AccessorWriteDefault>(accessors, OUT_JOINT_MATRICES));
	for( size_t i = 0; i < jointMatrices.count; ++i )
	{
		jointMatrices[i] = *(const float4x4 *) joints[i].invBindMatrix * *(const float4x4 *) joints[i].node->GetWorld(NULL);
	}
}

void SkinningAlgorithmD3D10Impl::ComputeSkinningMatrices(const AlgorithmResourceAccessor *accessors, const float* bindShape4x4)
{
	FindAccessor<AccessorD3D10BufferSRV> inJointsAccessor(accessors, IN_JOINT_MATRICES);
	FindAccessor<AccessorD3D10BufferSRV> inWeightsAccessor(accessors, IN_WEIGHTED_JOINTS);
	FindAccessor<AccessorD3D10BufferRead> inBindingsAccessor(accessors, IN_WEIGHTS_BINDINGS);
	FindAccessor<AccessorD3D10BufferWrite> outAccessor(accessors, OUT_TRANSFORMS);

	ID3D10Buffer *vtxBufferOut = outAccessor->Map();
	ID3D10Buffer *vtxBufferBindings = inBindingsAccessor->Map();
	ID3D10ShaderResourceView *jointsSRV = inJointsAccessor->Map();
	ID3D10ShaderResourceView *weightsSRV = inWeightsAccessor->Map();

	UINT offset = 0;
	UINT stride = sizeof(VertexWeightsBinding);

	ID3D10Device *device = m_resPool->GetAccessProvider()->GetDeviceManager()->GetDevice();

	// save device state
	m_resPool->GetStateBlock()->Capture();

	m_resPool->GetBindShapeVar()->SetMatrix(const_cast<float*>(bindShape4x4));
	m_resPool->GetJointsVar()->SetResource(jointsSRV);
	m_resPool->GetWeightsVar()->SetTextureBuffer(weightsSRV);
	m_resPool->GetTechniqueComputeMatrices()->GetPassByIndex(0)->Apply(0);

	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
	device->IASetVertexBuffers(0, 1, &vtxBufferBindings, &stride, &offset);
	device->IASetInputLayout(m_resPool->GetInputLayoutMatrices());
	device->SOSetTargets(1, &vtxBufferOut, &offset);
	device->Draw(static_cast<UINT>(outAccessor->GetMemoryObject()->GetBytesSize() / sizeof(float4x4)), 0);

	// restore device state
	m_resPool->GetStateBlock()->Apply();

	// release resources
	outAccessor->Unmap();
	inWeightsAccessor->Unmap();
	inJointsAccessor->Unmap();
}

void SkinningAlgorithmD3D10Impl::TransformPositions(const AlgorithmResourceAccessor *accessors)
{
	FindAccessor<AccessorD3D10BufferRead> inPositionsAccessor(accessors, IN_POSITIONS);
	FindAccessor<AccessorD3D10BufferRead> inMatricesAccessor(accessors, IN_TRANSFORMS);
	FindAccessor<AccessorD3D10BufferWrite> outPositionsAccessor(accessors, OUT_POSITIONS);

	assert(inPositionsAccessor->GetMemoryObject()->GetBytesSize() == outPositionsAccessor->GetMemoryObject()->GetBytesSize());
	assert(inPositionsAccessor->GetMemoryObject()->GetBytesSize() % sizeof(float3) == 0);
	assert(inMatricesAccessor->GetMemoryObject()->GetBytesSize() % sizeof(float4x4) == 0);

	ID3D10Buffer *vtxBufferOut = outPositionsAccessor->Map();
	ID3D10Buffer *vtxBuffersInput[2] = { inPositionsAccessor->Map(), inMatricesAccessor->Map() };

	UINT vtxStrides[2] = {sizeof(float3), sizeof(float4x4)};
	UINT vtxOffsets[2] = {0, 0};

	// save device state
	m_resPool->GetStateBlock()->Capture();

	m_resPool->GetTechniqueTransformPositions()->GetPassByIndex(0)->Apply(0);

	ID3D10Device *device = m_resPool->GetAccessProvider()->GetDeviceManager()->GetDevice();
	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
	device->IASetVertexBuffers(0, 2, vtxBuffersInput, vtxStrides, vtxOffsets);
	device->IASetInputLayout(m_resPool->GetInputLayoutTransform());
	device->SOSetTargets(1, &vtxBufferOut, vtxOffsets);
	device->Draw(static_cast<UINT>(inPositionsAccessor->GetMemoryObject()->GetBytesSize() / sizeof(float3)), 0);

	// restore device state
	m_resPool->GetStateBlock()->Apply();

	// commit modified buffer
	outPositionsAccessor->Unmap();
}

void SkinningAlgorithmD3D10Impl::TransformNormals(const AlgorithmResourceAccessor *accessors)
{
	AccessorD3D10BufferRead *inMatricesAccessor;
	if( const AlgorithmResourceAccessor *res = FindResourceById(accessors, IN_TMP_TRANSFORMS_SERIALIZED) )
	{
		inMatricesAccessor = static_cast<AccessorD3D10BufferRead*>(res->accessor);
	}
 	else
	{
		inMatricesAccessor = static_cast<AccessorD3D10BufferRead*>(FindResourceById(accessors, IN_TRANSFORMS)->accessor);
	}

	FindAccessor<AccessorD3D10BufferWrite> outNormalsAccessor(accessors, OUT_NORMALS);
	FindAccessor<AccessorD3D10BufferRead> inNormalsAccessor(accessors, IN_NORMALS);

	assert(inNormalsAccessor->GetMemoryObject()->GetBytesSize() % sizeof(float3) == 0);
	assert(outNormalsAccessor->GetMemoryObject()->GetBytesSize() % sizeof(float3) == 0);
	assert(inMatricesAccessor->GetMemoryObject()->GetBytesSize() % sizeof(float4x4) == 0);

	assert(inNormalsAccessor->GetMemoryObject()->GetBytesSize() == outNormalsAccessor->GetMemoryObject()->GetBytesSize());

	ID3D10Buffer *vtxBufferOut = outNormalsAccessor->Map();
	ID3D10Buffer *vtxBuffersInput[2] = { inNormalsAccessor->Map(), inMatricesAccessor->Map() };
	UINT vtxStrides[2] = {sizeof(float3), sizeof(float4x4)};
	UINT vtxOffsets[2] = {0, 0};

	// capture device state to restore it later
	m_resPool->GetStateBlock()->Capture();

	m_resPool->GetTechniqueTransformNormals()->GetPassByIndex(0)->Apply(0);

	ID3D10Device *device = m_resPool->GetAccessProvider()->GetDeviceManager()->GetDevice();
	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
	device->SOSetTargets(1, &vtxBufferOut, vtxOffsets);
	device->IASetVertexBuffers(0, 2, vtxBuffersInput, vtxStrides, vtxOffsets);
	device->IASetInputLayout(m_resPool->GetInputLayoutTransform());
	device->Draw(static_cast<UINT>(outNormalsAccessor->GetMemoryObject()->GetBytesSize() / sizeof(float3)), 0);

	// restore device state
	m_resPool->GetStateBlock()->Apply();

	// commit modified buffer
	outNormalsAccessor->Unmap();
}

void CreateSerializedVertexBuffer(AttributeSerializerAlgorithmD3D10Impl *serializer,
                                  MemoryObject *src, unsigned int srcStride, MemoryObject *idx,
                                  unsigned int resourceId, IResourceTracker *tracker)
{
	assert(src->GetManager() == idx->GetManager());
	size_t vcount = idx->GetBytesSize() / sizeof(unsigned long);

	AutoPtr<MemoryObject> serializedResource;
	src->GetManager()->Allocate(srcStride * vcount, NULL, &serializedResource);

	AlgorithmResource serializerInputs[] =
	{
		{ AttributeSerializerAlgorithmD3D10Impl::IN_INDEX, idx },
		{ AttributeSerializerAlgorithmD3D10Impl::IN_DATA, src },
		{ -1, NULL } // terminator
	};
	AlgorithmResource serializerOutputs[] =
	{
		{ AttributeSerializerAlgorithmD3D10Impl::OUT_DATA, serializedResource },
		{ -1, NULL } // terminator
	};
	AutoPtr<RefCounted>(new ProcessorSimple(serializer, serializerInputs, serializerOutputs));

	AutoPtr<AccessorD3D10BufferRead> reader;
	serializer->GetProvider()->CreateAccessorD3D10BufferRead(serializedResource, &reader, D3D10_BIND_VERTEX_BUFFER);
	tracker->OnCreateReadAccessor(resourceId, reader);
}

AttributeSerializerAlgorithmD3D10Impl* SkinningAlgorithmD3D10Impl::GetSerializer3() const
{
	if( !m_serializer3 )
	{
		m_serializer3 = new AttributeSerializerAlgorithmD3D10Impl(m_resPool, m_msg.GetMessageListener(), true);
	}
	return m_serializer3;
}

AttributeSerializerAlgorithmD3D10Impl* SkinningAlgorithmD3D10Impl::GetSerializer16() const
{
	if( !m_serializer16 )
	{
		m_serializer16 = new AttributeSerializerAlgorithmD3D10Impl(m_resPool, m_msg.GetMessageListener(), false);
	}
	return m_serializer16;
}

void SkinningAlgorithmD3D10Impl::CreateAccessors(const AlgorithmResource *inputs, IResourceTracker *tracker, Processor *p) const
{
	for( const AlgorithmResource *it = inputs; it->semanticId != -1; ++it )
	{
		switch( it->semanticId )
		{
		case IN_REINDEX_NORMALS:
			CreateSerializedVertexBuffer(GetSerializer3(), FindResourceById(inputs, IN_NORMALS)->data,
				sizeof(float3), it->data, IN_NORMALS, tracker);
			break;
		case IN_REINDEX_TRANSFORMS:
			CreateSerializedVertexBuffer(GetSerializer16(), FindResourceById(inputs, IN_TRANSFORMS)->data,
				sizeof(float4x4), it->data, IN_TMP_TRANSFORMS_SERIALIZED, tracker);
			break;
		case IN_NORMALS:
			if( FindResourceById(inputs, IN_REINDEX_NORMALS) )
				break;
		case IN_POSITIONS:
		case IN_TRANSFORMS:
		case IN_WEIGHTS_BINDINGS: // (begin, end)
			{
				AutoPtr<AccessorD3D10BufferRead> reader;
				m_resPool->GetAccessProvider()->CreateAccessorD3D10BufferRead(it->data, &reader, D3D10_BIND_VERTEX_BUFFER);
				tracker->OnCreateReadAccessor(it->semanticId, reader);
			}
			break;
		case IN_JOINT_MATRICES:
			{
				AutoPtr<AccessorD3D10BufferSRV> reader;
				D3D10_BUFFER_SRV desc;
				desc.ElementOffset = 0;
				desc.ElementWidth = static_cast<UINT>(it->data->GetBytesSize() / sizeof(float4));
				m_resPool->GetAccessProvider()->CreateAccessorD3D10BufferSRV(it->data, &reader, DXGI_FORMAT_R32G32B32A32_FLOAT, desc);
				tracker->OnCreateReadAccessor(it->semanticId, reader);
			}
			break;
		case IN_WEIGHTED_JOINTS: // (joint_idx, weight)
			{
				AutoPtr<AccessorD3D10BufferSRV> reader;
				D3D10_BUFFER_SRV desc;
				desc.ElementOffset = 0;
				desc.ElementWidth = static_cast<UINT>(it->data->GetBytesSize() / sizeof(WeightedJoint));
				m_resPool->GetAccessProvider()->CreateAccessorD3D10BufferSRV(it->data, &reader, DXGI_FORMAT_R32G32_UINT, desc);
				tracker->OnCreateReadAccessor(it->semanticId, reader);
			}
			break;
		case IN_JOINTS:
			{
				AutoPtr<AccessorReadDefault> reader;
				m_providerLocal->CreateAccessorReadDefault(it->data, &reader);
				tracker->OnCreateReadAccessor(it->semanticId, reader);
			}
			break;
		case OUT_JOINT_MATRICES:
			{
				AutoPtr<AccessorWriteDefault> writer;
				m_providerLocal->CreateAccessorWriteDefault(it->data, &writer, p);
				tracker->OnCreateWriteAccessor(it->semanticId, writer);
			}
			break;
		case OUT_POSITIONS:
		case OUT_NORMALS:
		case OUT_TRANSFORMS:
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

///////////////////////////////////////////////////////////////////////////////

// factory
namespace collada
{
	void CreateSkinningAlgorithmD3D10(AccessProviderD3D10 *providerD3D10, AccessProviderLocal *providerLocal, IMessageListener *msg, ISkinningAlgorithm **ppResult)
	{
		AutoPtr<ISkinningAlgorithm> tmp(new SkinningAlgorithmD3D10Impl(providerD3D10, providerLocal, msg));
		DetachRawPtr(tmp, ppResult);
	}
}

// end of file
