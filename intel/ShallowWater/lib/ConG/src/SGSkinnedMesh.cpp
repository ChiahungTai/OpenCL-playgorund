/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGSkinnedMesh.cpp

#include "stdafx.h"

#include "SGSkinnedMesh.h"

///////////////////////////////////////////////////////////////////////////////

SkinControllerImpl::SkinControllerImpl(Scene *s, size_t weightedJointCount, size_t bindingCount)
  : BaseImpl(s)
  , m_pBaseMesh(NULL)
  , m_isBindingLocked(false)
  , m_isJointsLocked(false)
{
	assert(bindingCount && weightedJointCount);
	MatrixIdentity(&m_bindShape);
	m_weightedJoints.resize(weightedJointCount);
	m_weightBinding.resize(bindingCount);
}

void SkinControllerImpl::SetBaseMesh(Mesh *baseMesh)
{
	m_pBaseMesh = baseMesh;
}

const WeightedJoint* SkinControllerImpl::GetWeightedJoints() const
{
	assert(!m_isJointsLocked);
	return &m_weightedJoints[0];
}

size_t SkinControllerImpl::GetWeightedJointCount() const
{
	return m_weightedJoints.size();
}

const VertexWeightsBinding* SkinControllerImpl::GetBindings() const
{
	assert(!m_isBindingLocked);
	return &m_weightBinding[0];
}

size_t SkinControllerImpl::GetBindingCount() const
{
	return m_weightBinding.size();
}

VertexWeightsBinding* SkinControllerImpl::LockBinding()
{
	assert(!m_isBindingLocked);
	m_isBindingLocked = true;
	return &m_weightBinding[0];
}

void SkinControllerImpl::UnlockBinding()
{
	assert(m_isBindingLocked);
	m_isBindingLocked = false;
}

WeightedJoint* SkinControllerImpl::LockWeightedJoints()
{
	assert(!m_isJointsLocked);
	m_isJointsLocked = true;
	return &m_weightedJoints[0];
}

void SkinControllerImpl::UnlockWeightedJoints()
{
	assert(m_isJointsLocked);
	m_isJointsLocked = false;
}

const float* SkinControllerImpl::GetBindShape() const // float4x4
{
	return m_bindShape;
}

void SkinControllerImpl::SetBindShape(const float *data4x4)
{
	memcpy(m_bindShape, data4x4, sizeof(m_bindShape));
}

Mesh* SkinControllerImpl::GetBaseMesh(Scene *context) const
{
	assert(m_pBaseMesh);
	return SafeResolveContext<Mesh>(context, m_pBaseMesh);
}

///////////////////////////////////////////////////////////////////////////////

SkinControllerInstanceImpl::SkinControllerInstanceImpl(Scene *s, Node *node, SkinController *skinCtrl)
  : GeometryInstanceImpl(s, skinCtrl, node)
{
}


//
// SkinnedMeshInstance interface
//

MemoryObject* SkinControllerInstanceImpl::GetJoints() const
{
	return m_joints;
}

void SkinControllerInstanceImpl::SetJoints(MemoryObject *joints)
{
	m_joints = joints;
}

// end of file
