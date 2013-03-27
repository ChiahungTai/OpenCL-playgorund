/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file TriangulatorAlgorithmDefaultImpl.cpp

#include "stdafx.h"
#include "TriangulatorAlgorithmDefaultImpl.h"
#include "AccessProviderDefault.h"


TriangulatorAlgorithmDefaultImpl::TriangulatorAlgorithmDefaultImpl(AccessProviderLocal *providerLocal)
  : m_provider(providerLocal)
{
}

TriangulatorAlgorithmDefaultImpl::~TriangulatorAlgorithmDefaultImpl()
{
}

void TriangulatorAlgorithmDefaultImpl::Triangulate(const AlgorithmResourceAccessor *accessors)
{
}

void TriangulatorAlgorithmDefaultImpl::CreateAccessors(const AlgorithmResource *inputs, IResourceTracker *tracker, Processor *p) const
{
}

///////////////////////////////////////////////////////////////////////////////

// factory
namespace collada
{
	void CreateTriangulatorAlgorithmDefault(AccessProviderLocal *providerLocal, ITriangulatorAlgorithm **ppResult)
	{
		AutoPtr<ITriangulatorAlgorithm> tmp(new TriangulatorAlgorithmDefaultImpl(providerLocal));
		DetachRawPtr(tmp, ppResult);
	}
}

// end of file
