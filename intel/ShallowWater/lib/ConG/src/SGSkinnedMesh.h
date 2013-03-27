/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGSkinnedMesh.h

#pragma once

#include "mathstub.h"

#include "SceneGraph.h"
#include "SGImplBase.h"
#include "SGGeometryInstance.h"
#include "AutoPtr.h"

using namespace collada;

///////////////////////////////////////////////////////////////////////////////

class SkinControllerImpl : public BaseImpl<SkinController>
{
public:
	SkinControllerImpl(Scene *s, size_t weightedJointCount, size_t bindingCount);

	// SkinController interface
	virtual void SetBaseMesh(Mesh *baseMesh);
	virtual Mesh *GetBaseMesh(Scene *context) const;
	virtual const float* GetBindShape() const;        // float4x4 row major
	virtual void SetBindShape(const float *data4x4);  // float4x4 row major
	virtual const WeightedJoint* GetWeightedJoints() const;
	virtual size_t GetWeightedJointCount() const;
	virtual const VertexWeightsBinding* GetBindings() const;
	virtual size_t GetBindingCount() const;

	virtual VertexWeightsBinding* LockBinding();
	virtual void UnlockBinding();
	virtual WeightedJoint* LockWeightedJoints();
	virtual void UnlockWeightedJoints();

private:
	Mesh *m_pBaseMesh;

	std::vector<WeightedJoint> m_weightedJoints;
	std::vector<VertexWeightsBinding> m_weightBinding;

	float4x4 m_bindShape;

	bool m_isBindingLocked;
	bool m_isJointsLocked;
};

///////////////////////////////////////////////////////////////////////////////
// represents <controller_instance>

class SkinControllerInstanceImpl
	: public GeometryInstanceImpl<SkinControllerInstance, SkinController>
{
public:
	// create skinned mesh instance
	SkinControllerInstanceImpl(Scene *s, Node *node, SkinController *skinCtrl);

	// SkinnedMeshInstance interface
	virtual MemoryObject* GetJoints() const;
	virtual void SetJoints(MemoryObject *joints);

private:
	AutoPtr<MemoryObject> m_joints;
};

// end of file
