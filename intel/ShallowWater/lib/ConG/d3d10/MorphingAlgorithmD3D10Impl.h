/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file MorphingAlgorithmD3D10Impl.h


#pragma once

#include "AutoPtr.h"
#include "MsgListenerWrapper.h"
#include "RefCountedImpl.h"

#include "d3d10/AccessProviderD3D10.h"
#include "d3d10/MorphingAlgorithmD3D10.h"
#include "d3d10/DeviceManagerD3D10.h"

namespace collada
{
	struct AccessProviderD3D10;
}

using namespace collada;

namespace
{	class CommonResourcePool
		: public RefCountedImpl<RefCounted>
		, private IDeviceListenerD3D10
	{
	public:
		CommonResourcePool(AccessProviderD3D10 *providerD3D10, IMessageListener *msg);
		virtual ~CommonResourcePool();

		AccessProviderD3D10* GetAccessProvider() const { return m_provider; }
		ID3D10InputLayout* GetInputLayoutMorphing() const { return m_inputLayoutMorphing; }
		ID3D10EffectTechnique* GetTechniqueMorphing() const { return m_techniqueMorphing; }

		ID3D10EffectShaderResourceVariable* GetTargetsVar() const { return m_targetsVar; }
		ID3D10EffectShaderResourceVariable* GetWeightsVar() const { return m_weightsVar; }
		ID3D10EffectScalarVariable* GetNumBSVar() const { return m_numBS; }

		ID3D10StateBlock* GetStateBlock() const { return m_stateBlock; }

	private:
		CommonResourcePool(const CommonResourcePool&); // no copy
		CommonResourcePool& operator = (const CommonResourcePool&);

		MsgListenerWrapper m_msg;
		AutoPtr<AccessProviderD3D10> m_provider;
		CComPtr<ID3D10StateBlock> m_stateBlock;
		CComPtr<ID3D10Effect> m_effect;
		CComPtr<ID3D10InputLayout> m_inputLayoutMorphing;
		ID3D10EffectTechnique *m_techniqueMorphing;
		ID3D10EffectShaderResourceVariable *m_targetsVar;
		ID3D10EffectShaderResourceVariable *m_weightsVar;
		ID3D10EffectScalarVariable *m_numBS; // blend shapes number;

		// IDeviceListenerD3D10
		virtual HRESULT OnSetDevice(ID3D10Device *pd3dDevice);
	};

	class MorphingAlgorithmD3D10Impl: public RefCountedImpl<IMorphingAlgorithm>
	{
	public:
		MorphingAlgorithmD3D10Impl(AccessProviderD3D10 *providerD3D10, AccessProviderLocal *providerLocal, IMessageListener *msg);
		virtual ~MorphingAlgorithmD3D10Impl();

		// IAlgorithmSimple
		virtual void DoProcess(const AlgorithmResourceAccessor *accessors) const;

		// IAlgorithm
		virtual void CreateAccessors(const AlgorithmResource *inputs, IResourceTracker *tracker, Processor *p) const;

	private:
		MsgListenerWrapper m_msg;
		AutoPtr<CommonResourcePool> m_resPool;
		AutoPtr<AccessProviderLocal> m_providerLocal;
	};
} // end of unnamed namespace

// end of file
