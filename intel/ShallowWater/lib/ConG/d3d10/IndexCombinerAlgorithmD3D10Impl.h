/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
        Copyright (C) 2009-2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file IndexCombinerAlgorithmD3D10Impl.h

#pragma once

#include "AutoPtr.h"
#include "MsgListenerWrapper.h"

#include "RefCountedImpl.h"

#include "d3d10/IndexCombinerAlgorithmD3D10.h"
#include "d3d10/AccessProviderD3D10.h"
#include "d3d10/DeviceManagerD3D10.h"

namespace collada
{
	struct AccessProviderD3D10;
}

using namespace collada;

namespace
{
	class IndexCombinerAlgorithmD3D10Impl
		: public RefCountedImpl<IIndexCombinerAlgorithm>
		, public IDeviceListenerD3D10
	{
	public:
		IndexCombinerAlgorithmD3D10Impl(AccessProviderD3D10 *providerD3D10, IMessageListener *msg);
		virtual ~IndexCombinerAlgorithmD3D10Impl();

		// IIndexCombinerAlgorithm
		virtual void Execute(const AlgorithmResourceAccessor *accessors, unsigned long *vertexOffsets, size_t taskCount, unsigned int vertexStride);

		// IAlgorithm
		virtual void CreateAccessors(const AlgorithmResource *inputs, IResourceTracker *tracker, Processor *p) const;

	private:
		// IDeviceListenerD3D10
		virtual HRESULT OnSetDevice(ID3D10Device *pd3dDevice);

	private:
		MsgListenerWrapper m_msg;
		AutoPtr<AccessProviderD3D10> m_provider;
		CComPtr<ID3D10StateBlock> m_stateBlock;
		CComPtr<ID3D10InputLayout> m_layouts[4];
		CComPtr<ID3D10Effect> m_effect;
		ID3D10EffectTechnique *m_technique;
	};
}

// end of file
