/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGGeometryInstance.h

#pragma once

#include "SGImplBase.h"
#include "SGMaterialBinding.h"

///////////////////////////////////////////////////////////////////////////////

template <class T, class EntityType>
class GeometryInstanceImpl : public InstanceImpl<T, EntityType>
{
public:
	GeometryInstanceImpl(Scene *context, EntityType *entity, Node *node)
	  : InstanceImpl(context, entity, node)
	{
	}

	// GeometryInstance interface
	virtual size_t GetMaterialBindingCount() const
	{
		return m_mtlBindings.size();
	}

	virtual MaterialBinding* GetMaterialBinding(size_t index, Scene *context) const
	{
		return SafeResolveContext<MaterialBinding>(context, m_mtlBindings[index]);
	}

	virtual MaterialBinding* CreateMaterialBinding(Batch *batch, size_t batchIdx, Material *src)
	{
		if( m_mtlBindings.size() <= batchIdx )
		{
			m_mtlBindings.resize(batchIdx+1, NULL);
		}
		if( m_mtlBindings[batchIdx] )
		{
			delete m_mtlBindings[batchIdx];
			m_mtlBindings[batchIdx] = NULL;
		}
		m_mtlBindings[batchIdx] = new MaterialBindingImpl(GetNativeContext(), this, batch, src);
		return m_mtlBindings[batchIdx];
	}

private:
	PtrOwnerArray<MaterialBindingImpl> m_mtlBindings; // materials binding for each batch
};

// end of file
