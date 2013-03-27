/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGMorphedMesh.h

#pragma once

#include "mathstub.h"

#include "SceneGraph.h"
#include "SGImplBase.h"
#include "SGGeometryInstance.h"
#include "AutoPtr.h"

using namespace collada;

class MorphControllerImpl : public BaseImpl<MorphController>
{
public:
	MorphControllerImpl(Scene *s);

	// MorphController interface
	virtual void SetBaseMesh(Mesh *baseMesh);
	virtual Mesh *GetBaseMesh(Scene *context) const;
	virtual void SetWeights(MemoryObject *weights);
	virtual MemoryObject* GetWeights() const;
	virtual void SetTargets(Mesh *const* ppMeshArray, size_t count);
	virtual size_t GetMeshCount() const;
	virtual Mesh* GetTargetMesh(size_t index);


private:
	Mesh *m_pBaseMesh;
	AutoPtr<MemoryObject> m_weights;
	std::vector<Mesh*> m_targets;
};

///////////////////////////////////////////////////////////////////////////////

class MorphControllerInstanceImpl
	: public GeometryInstanceImpl<MorphControllerInstance, MorphController>
{
public:
	MorphControllerInstanceImpl(Scene *s, Node *node, MorphController *morphCtrl);

private:
};

// end of file
