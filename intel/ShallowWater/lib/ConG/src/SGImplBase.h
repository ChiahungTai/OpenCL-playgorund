/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGImplBase.h

#pragma once

#include "SceneGraphBase.h"
#include "SGUtils.h"

namespace collada
{
	#ifdef _UNICODE
		typedef std::wstring string_t;
	#else _UNICODE
		typedef std::string string_t;
	#endif _UNICODE
}

using namespace collada;

///////////////////////////////////////////////////////////////////////////////

struct PropertyListener
{
	virtual void OnPropertyChange() = 0; // called by delegate when some parameter is changed
};

///////////////////////////////////////////////////////////////////////////////

template <class T, void (T::*onUpdate)(collada::Property *)>
class StaticPropertyFloat1 : public collada::Property
{
public:
	StaticPropertyFloat1(T *owner, float value, const char *semantic = NULL)
	  : m_owner(owner)
	  , m_value(value)
	  , m_semantic(semantic)
	{
	}

	virtual const char* GetSemantic() const
	{
		return m_semantic;
	}

	virtual size_t GetSize() const
	{
		return 1;
	}

	virtual void GetValue(float *result) const
	{
		*result = m_value;
	}

	virtual void SetValue(const float *floatArray)
	{
		m_value = *floatArray;
		(m_owner->*onUpdate)(this);
	}

	inline float GetFloat() const { return m_value; }
	inline void RawSetFloat(float value) { m_value = value; }

protected:
	float m_value;
	T * const m_owner;
	const char *m_semantic;
};

///////////////////////////////////////////////////////////////////////////////

// implementation of basic functionality
template <class T>
class BaseImpl : public T
{
public:
	//
	// Base interface implementation
	//

	virtual MemoryManager* GetMemoryManager() const
	{
		return m_context->GetMemoryManager();
	}

	virtual Scene* GetNativeContext() const
	{
		return m_context;
	}

	virtual const char* GetTag() const
	{
		return m_id.c_str();
	}
	virtual void SetTag(const char *tag)
	{
		m_id = tag;
	}

	virtual IView* GetView(size_t id, Scene *context)
	{
		if( m_connectedViews.size() <= id )
		{
			m_connectedViews.resize(id + 1, ((IView *) 1));
		}
		if( ((IView *) 1) == m_connectedViews[id] )
		{
			// Create view on demand
			m_connectedViews[id] = m_context->CreateView(context, this, id);
		}
		return m_connectedViews[id];
	}

	virtual void DetachView(size_t id)
	{
		if( id < m_connectedViews.size() )
		{
			if( IsPointerValid(m_connectedViews[id]) )
			{
				m_context->ReleaseView(this, id);
				m_connectedViews[id]->OnDetach();
			}
			m_connectedViews[id] = ((IView *) 1); // create on demand next time
		}
	}

	virtual size_t GetPropertyCount() const { return 0; }
	virtual Property* GetProperty(size_t idx) { return NULL; }

protected:
	BaseImpl(const BaseImpl &src)
	  : m_context(src.m_context)
	  , m_id(src.m_id)
	{
	}

	BaseImpl(Scene *context)
	  : m_context(context)
	{
		assert(context);
	}

	virtual ~BaseImpl()
	{
		for( std::vector<IView *>::const_iterator it = m_connectedViews.begin(); it != m_connectedViews.end(); ++it )
		{
			if( IsPointerValid(*it) )
			{
				(*it)->OnDetach();
			}
		}
//		m_scene->OnDestroyObject(this); // TODO: this is not needed until we remove objects at runtime
	}

private:
	BaseImpl& operator = (const BaseImpl&); // no copy

	std::string m_id;
	Scene *m_context;
	std::vector<IView *> m_connectedViews; // views can be connected on demand

	inline static size_t IsPointerValid(void *ptr)
	{
		return (~ (size_t) 1) & (size_t) ptr;
	}
};

///////////////////////////////////////////////////////////////////////////////

template <class T, class EntityType>
class InstanceImpl : public BaseImpl<T>
{
	EntityType *m_entity;
	Node *m_node;

public:
	InstanceImpl(Scene *context, EntityType *entity, Node *node)
	  : BaseImpl(context)
	  , m_entity(entity)
	  , m_node(node)
	{
	}

	// Instance interface
	virtual EntityType* GetEntity(Scene *context) const
	{
		return SafeResolveContext<EntityType>(context, m_entity);
	}

	virtual Node* GetNode(Scene *context) const
	{
		return SafeResolveContext<Node>(context, m_node);
	}
};

// end of file
