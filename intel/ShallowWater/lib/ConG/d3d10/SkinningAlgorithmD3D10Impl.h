/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SkinningAlgorithmD3D10Impl.h


#pragma once

#include "AutoPtr.h"
#include "MsgListenerWrapper.h"
#include "RefCountedImpl.h"

#include "d3d10/AccessProviderD3D10.h"
#include "d3d10/SkinningAlgorithmD3D10.h"
#include "d3d10/DeviceManagerD3D10.h"

namespace collada
{
	struct AccessProviderD3D10;
}

using namespace collada;

namespace
{
	class CommonResourcePool
		: public RefCountedImpl<RefCounted>
		, private IDeviceListenerD3D10
	{
	public:
		CommonResourcePool(AccessProviderD3D10 *providerD3D10, IMessageListener *msg);
		virtual ~CommonResourcePool();

		AccessProviderD3D10* GetAccessProvider() const { return m_provider; }
		ID3D10InputLayout* GetInputLayoutMatrices() const { return m_inputLayoutMatrices; }
		ID3D10InputLayout* GetInputLayoutTransform() const { return m_inputLayoutTransform; }
		ID3D10InputLayout* GetInputLayoutSerialize3() const { return m_inputLayoutSerialize3; }
		ID3D10InputLayout* GetInputLayoutSerialize16() const { return m_inputLayoutSerialize16; }
		ID3D10EffectTechnique* GetTechniqueComputeMatrices() const { return m_techniqueComputeMatrices; }
		ID3D10EffectTechnique* GetTechniqueTransformPositions() const { return m_techniqueTransformPositions; }
		ID3D10EffectTechnique* GetTechniqueTransformNormals() const { return m_techniqueTransformNormals; }
		ID3D10EffectTechnique* GetTechniqueSerialize3() const { return m_techniqueSerialize3; }
		ID3D10EffectTechnique* GetTechniqueSerialize16() const { return m_techniqueSerialize16; }

		ID3D10EffectMatrixVariable* GetBindShapeVar() const { return m_bindShapeVar; }
		ID3D10EffectShaderResourceVariable* GetJointsVar() const { return m_jointsVar; }
		ID3D10EffectConstantBuffer* GetWeightsVar() const { return m_weightsVar; }

		ID3D10StateBlock* GetStateBlock() const { return m_stateBlock; }

	private:
		CommonResourcePool(const CommonResourcePool&); // no copy
		CommonResourcePool& operator = (const CommonResourcePool&);

		MsgListenerWrapper m_msg;
		AutoPtr<AccessProviderD3D10> m_provider;
		CComPtr<ID3D10StateBlock> m_stateBlock;
		CComPtr<ID3D10Effect> m_effect;
		CComPtr<ID3D10InputLayout> m_inputLayoutMatrices;
		CComPtr<ID3D10InputLayout> m_inputLayoutTransform;
		CComPtr<ID3D10InputLayout> m_inputLayoutSerialize3;
		CComPtr<ID3D10InputLayout> m_inputLayoutSerialize16;
		ID3D10EffectTechnique *m_techniqueComputeMatrices;
		ID3D10EffectTechnique *m_techniqueTransformPositions;
		ID3D10EffectTechnique *m_techniqueTransformNormals;
		ID3D10EffectTechnique *m_techniqueSerialize3;
		ID3D10EffectTechnique *m_techniqueSerialize16;
		ID3D10EffectMatrixVariable *m_bindShapeVar;
		ID3D10EffectShaderResourceVariable *m_jointsVar;
		ID3D10EffectConstantBuffer *m_weightsVar;

		// IDeviceListenerD3D10
		virtual HRESULT OnSetDevice(ID3D10Device *pd3dDevice);
	};

	class AttributeSerializerAlgorithmD3D10Impl: public RefCountedImpl<IAlgorithmSimple>
	{
	public:
		AttributeSerializerAlgorithmD3D10Impl(CommonResourcePool *resourcePool, IMessageListener *msg, bool ifTrueThen3Else16);
		AccessProviderD3D10* GetProvider() const { return m_resPool->GetAccessProvider(); }

		// IAlgorithmSimple
		virtual void DoProcess(const AlgorithmResourceAccessor *accessors) const;

		// IAlgorithm
		virtual void CreateAccessors(const AlgorithmResource *inputs, IResourceTracker *tracker, Processor *p) const;

		enum
		{
			IN_INDEX,
			IN_DATA,
			OUT_DATA,
		};

	private:
		MsgListenerWrapper m_msg;
		AutoPtr<CommonResourcePool> m_resPool;
		bool m_IfTrueThen3Else16;
	};

	class SkinningAlgorithmD3D10Impl: public RefCountedImpl<ISkinningAlgorithm>
	{
	public:
		SkinningAlgorithmD3D10Impl(AccessProviderD3D10 *providerD3D10, AccessProviderLocal *providerLocal, IMessageListener *msg);
		virtual ~SkinningAlgorithmD3D10Impl();

		// ISkinningAlgorithm
		virtual void ComputeJoints(const AlgorithmResourceAccessor *accessors);
		virtual void ComputeSkinningMatrices(const AlgorithmResourceAccessor *accessors, const float* bindShape4x4);
		virtual void TransformPositions(const AlgorithmResourceAccessor *accessors);
		virtual void TransformNormals(const AlgorithmResourceAccessor *accessors);

		// IAlgorithm
		virtual void CreateAccessors(const AlgorithmResource *inputs, IResourceTracker *tracker, Processor *p) const;

	protected:
		AttributeSerializerAlgorithmD3D10Impl* GetSerializer3() const;
		AttributeSerializerAlgorithmD3D10Impl* GetSerializer16() const;

	private:
		MsgListenerWrapper m_msg;
		AutoPtr<CommonResourcePool> m_resPool;
		AutoPtr<AccessProviderLocal> m_providerLocal;
		mutable AutoPtr<AttributeSerializerAlgorithmD3D10Impl> m_serializer3; // created on demand
		mutable AutoPtr<AttributeSerializerAlgorithmD3D10Impl> m_serializer16; // created on demand
	};

} // end of unnamed namespace


// end of file
