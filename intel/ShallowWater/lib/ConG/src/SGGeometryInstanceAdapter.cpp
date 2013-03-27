/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGGeometryInstanceAdapter.cpp

#include "stdafx.h"

#include "SGGeometryInstanceAdapter.h"
using namespace collada;

// force template instantiations
template class GeometryInstanceAdapter<MeshInstance, SkinControllerInstance, Mesh>;
template class GeometryInstanceAdapter<MeshInstance, MorphControllerInstance, Mesh>;
template class GeometryInstanceAdapter<MeshInstance, MeshInstance, Mesh>;

///////////////////////////////////////////////////////////////////////////////

template <class InstanceType, class TargetType, class EntityType>
GeometryInstanceAdapter<InstanceType, TargetType, EntityType>::GeometryInstanceAdapter(TargetType *target, Scene *context)
  : BaseImpl(context)
  , m_target(target)
{
}

template <class InstanceType, class TargetType, class EntityType>
GeometryInstance* GeometryInstanceAdapter<InstanceType, TargetType, EntityType>::GetTarget() const
{
	return m_target;
}


//
// GeometryInstance interface
//

template <class InstanceType, class TargetType, class EntityType>
size_t GeometryInstanceAdapter<InstanceType, TargetType, EntityType>::GetMaterialBindingCount() const
{
	return GetTarget()->GetMaterialBindingCount();
}

template <class InstanceType, class TargetType, class EntityType>
MaterialBinding* GeometryInstanceAdapter<InstanceType, TargetType, EntityType>::GetMaterialBinding(size_t index, Scene *context) const
{
	return GetTarget()->GetMaterialBinding(index, context);
}

template <class InstanceType, class TargetType, class EntityType>
MaterialBinding* GeometryInstanceAdapter<InstanceType, TargetType, EntityType>::CreateMaterialBinding(Batch *batch, size_t batchIdx, Material *src)
{
	return GetTarget()->CreateMaterialBinding(batch, batchIdx, src);
}


//
// Instance interface
//

template <class InstanceType, class TargetType, class EntityType>
EntityType* GeometryInstanceAdapter<InstanceType, TargetType, EntityType>::GetEntity(Scene *context) const
{
	return static_cast<EntityType*>(GetTarget()->GetEntity(context));
}

template <class InstanceType, class TargetType, class EntityType>
Node* GeometryInstanceAdapter<InstanceType, TargetType, EntityType>::GetNode(Scene *context) const
{
	return GetTarget()->GetNode(context);
}

// end of file
