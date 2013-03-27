/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGMaterialBinding.cpp

#include "stdafx.h"
#include "SGMaterialBinding.h"

///////////////////////////////////////////////////////////////////////////////

MaterialBindingImpl::MaterialBindingImpl(Scene *s, GeometryInstance *parentInstance, Batch *batch, Material *mat)
  : BaseImpl(s)
  , m_parentInstance(parentInstance)
  , m_batch(batch)
  , m_material(mat)
  , m_isInputsValid(false)
{
}

void MaterialBindingImpl::FlushInternalState() const
{
	if( m_isInputsValid )
	{
		return;
	}

	for( InputsCache::iterator it = m_cachedInputs.begin(); m_cachedInputs.end() != it; ++it )
	{
		it->second.clear();
		Scene *context = it->first;

		const Batch *batch = GetBatch(context);
		const Effect *effect = GetMaterial(context)->GetEffect(context);

		for( size_t i = 0; i < effect->GetInputCount(); ++i )
		{
			const char *sem = effect->GetInputSemantic(i);

			MaterialBinding::Target target;
			if( !GetBindingTarget(sem, &target) )
			{
				// explicit binding is not specified, use semantic as is
				target.index = -1; // target should be located at primary attribute set
				target.semantic = sem;
			}


			bool found = false;
			const VertexElementDesc *desc = NULL;

			// search primary
			for( size_t j = 0; j < batch->GetMesh(context)->GetPrimaryVertexElementCount() && !found; ++j )
			{
				desc = batch->GetMesh(context)->GetPrimaryVertexElementDescs() + j;
				if( 0 == strcmp(desc->semanticName, target.semantic) 
					&& (-1 == target.index || batch->GetPrimarySet() == target.index) )
				{
					found = true;
				}
			}

			// search extra
			for( size_t j = 0; j < batch->GetExtraVertexElementCount() && !found; ++j )
			{
				const VertexElementDescEx *descEx = batch->GetExtraVertexElementDescs() + j;
				if( 0 == strcmp(descEx->semanticName, target.semantic) 
					&& (-1 == target.index || descEx->semanticIndex == target.index) )
				{
					desc = descEx;
					found = true;
				}
			}

			if( !found )
			{
				printf("WARNING! semantic \"%s/%d\" not found in vertex declaration\n", target.semantic, target.index);
				// TODO: search for any compatible vertex attribute
			}

			assert(desc);
			if( desc )
			{
				VertexElementDesc requiredInput;
				requiredInput.componentType = desc->componentType;
				requiredInput.componentCount = desc->componentCount;
				requiredInput.inputSlot = desc->inputSlot;
				requiredInput.semanticName = sem;
				it->second.push_back(requiredInput);
			}
		}
	}

	m_isInputsValid = true;
}

GeometryInstance* MaterialBindingImpl::GetParentInstance(Scene *context) const
{
	return SafeResolveContext<GeometryInstance>(context, m_parentInstance);
}

Batch* MaterialBindingImpl::GetBatch(Scene *context) const
{
	return SafeResolveContext<Batch>(context, m_batch);
}

void MaterialBindingImpl::SetMaterial(Material*pMtl){
	m_material = pMtl;
}
Material* MaterialBindingImpl::GetMaterial(Scene *context) const
{
	return SafeResolveContext<Material>(context, m_material);
}

//bool MaterialBindingImpl::GetTarget(const char *sem, Target *result) const
//{
//	std::map<std::string, MyTarget>::const_iterator it = m_binding.find(sem);
//	if( m_binding.end() == it )
//	{
//		return false;
//	}
//	result->semantic = it->second.first.c_str();
//	result->index = it->second.second;
//	return true;
//}
	
size_t MaterialBindingImpl::GetBindingCount() const
{
	return m_binding.size();
}

const char* MaterialBindingImpl::GetBindingName(size_t index) const
{
	assert(index<GetBindingCount());
	BindingMap::const_iterator it = m_binding.begin();
	while(index--)
	{
		++it;
	}
	return (it!=m_binding.end())?it->first.c_str():NULL;
}

bool MaterialBindingImpl::GetBindingTarget(const char* name, Target *result) const
{
	BindingMap::const_iterator it = m_binding.find(name);

	if(it!=m_binding.end())
	{
		result->semantic = it->second.first.c_str();
		result->index = it->second.second;
	}

	return (it!=m_binding.end());
}

void MaterialBindingImpl::SetBinding(const char *sem, const Target *target)
{
	//assert(0 == m_binding.count(sem));
	//MyTarget(target->semantic, target->index);
	if(target)
	{
		m_binding[sem] = MyTarget(target->semantic, target->index);
	}
	else
	{
		BindingMap::iterator it = m_binding.find(sem);
		if(it != m_binding.end())
			m_binding.erase(it);
	}
	//BindingMap::_Pairib pairib = m_binding.insert(BindingMap::value_type(sem, toInsert));
	//if(!pairib.second){
	//	pairib.first->second = toInsert;
	//}
	m_isInputsValid = false;
}

size_t MaterialBindingImpl::GetRequiredInputCount(Scene *context) const
{
	if( m_cachedInputs.insert(InputsCache::value_type(context, std::vector<VertexElementDesc>())).second )
	{
		// TODO: remove const cast
		const_cast<Scene *>(context)->AddListener(const_cast<MaterialBindingImpl*>(this));
		m_isInputsValid = false;
	}
	FlushInternalState();
	return m_cachedInputs[context].size();
}

const VertexElementDesc* MaterialBindingImpl::GetRequiredInputs(Scene *context) const
{
	if( m_cachedInputs.insert(InputsCache::value_type(context, std::vector<VertexElementDesc>())).second )
	{
		// TODO: remove cast
		const_cast<Scene *>(context)->AddListener(const_cast<MaterialBindingImpl*>(this));
		m_isInputsValid = false;
	}
	FlushInternalState();
	return &m_cachedInputs[context][0];
}

void MaterialBindingImpl::OnSceneDestroy(Scene *which)
{
	assert(m_cachedInputs.count(which));
	m_cachedInputs.erase(which);
	which->RemoveListener(this);
}

// end of file
