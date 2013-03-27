/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGGeometryInstanceAdapter.h

#pragma once

#include "SceneGraph.h"
#include "SGGeometryInstance.h"

namespace collada
{

template <class InstanceType, class TargetType, class EntityType>
class GeometryInstanceAdapter : public BaseImpl<InstanceType>
{
public:
	GeometryInstanceAdapter(TargetType *target, Scene *context);
	GeometryInstance* GetTarget() const;

	// GeometryInstance interface
	virtual size_t GetMaterialBindingCount() const;
	virtual MaterialBinding* GetMaterialBinding(size_t index, Scene *context) const;
	virtual MaterialBinding* CreateMaterialBinding(Batch *batch, size_t batchIdx, Material *src);

	// Instance interface
	virtual EntityType* GetEntity(Scene *context) const;
	virtual Node* GetNode(Scene *context) const;

private:
	TargetType * const m_target;
};

} // end of namespace collada
// end of file
