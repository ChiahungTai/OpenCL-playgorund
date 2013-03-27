/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGMorphedMesh.cpp

#include "stdafx.h"

#include "SGMorphedMesh.h"

MorphControllerImpl::MorphControllerImpl(Scene *s)
  : BaseImpl(s)
{
}

void MorphControllerImpl::SetBaseMesh(Mesh *baseMesh)
{
	m_pBaseMesh = baseMesh;
}

Mesh* MorphControllerImpl::GetBaseMesh(Scene *context) const
{
	return m_pBaseMesh;
}

void MorphControllerImpl::SetWeights(MemoryObject *weights)
{
	m_weights = weights;
}

MemoryObject* MorphControllerImpl::GetWeights() const
{
	return m_weights;
}

void MorphControllerImpl::SetTargets(Mesh *const* ppMeshArray, size_t count)
{
	m_targets.assign(ppMeshArray, ppMeshArray + count);
}

size_t MorphControllerImpl::GetMeshCount() const
{
	return m_targets.size();
}

Mesh* MorphControllerImpl::GetTargetMesh(size_t index)
{
	return m_targets[index];
}

///////////////////////////////////////////////////////////////////////////////

MorphControllerInstanceImpl::MorphControllerInstanceImpl(Scene *s, Node *node, MorphController *morphCtrl)
  : GeometryInstanceImpl(s, morphCtrl, node)
{
}


// end of file
