/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
        Copyright (C) 2009-2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file IndexCombinerAlgorithm.h

#pragma once

#include "SceneGraphBase.h"
#include "AlgorithmBase.h"
#include "AutoPtr.h"

#include <vector>

namespace collada
{

struct AccessorRead;
struct AccessorWrite;
struct AccessProviderLocal;

struct IndexCombinerAlgorithmData
{
	struct Task
	{
		AutoPtr<AccessorRead> inIndexBufferAccessor;
		unsigned long dstVertexOffset; // in elements
	};
	std::vector<Task> tasks;

	unsigned int vertexStride;
	AutoPtr<AccessorRead> inVertexBufferAccessor;
};

struct IIndexCombinerAlgorithm : public IAlgorithm
{
	enum ResourceSemantics
	{
		IN_INDEX_BUFFER,
		IN_VERTEX_BUFFER,
		OUT_VERTEX_BUFFER,
	};
	virtual void Execute(const AlgorithmResourceAccessor *accessors, unsigned long *vertexOffsets, size_t taskCount, unsigned int vertexStride) = 0;
};


// default CPU-based implementation
void CreateIndexCombinerAlgorithmDefault(AccessProviderLocal *providerLocal, IIndexCombinerAlgorithm **ppResult);

} // end of namespace collada

// end of file
