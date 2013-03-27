/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file RefCountedImpl.h
///
/// implementation of reference counting functionality

#pragma once

#include "RefCounted.h"
#include "AutoPtr.h"

#include <cassert>
#include <map>
#include <set>


namespace collada
{

struct RefCountedImplDummy {};

template <class T1, class T2 = RefCountedImplDummy>
class RefCountedImpl : public T1, public T2
{
	RefCountedImpl(const RefCountedImpl&); // no copy
	RefCountedImpl& operator = (const RefCountedImpl&);

	unsigned int m_refCount;

protected:
	RefCountedImpl()
		: m_refCount(0)
	{
	}
	virtual ~RefCountedImpl()
	{
		assert(0 == m_refCount);
	}


public:
	//
	// RefCounted
	//
	virtual unsigned int AddRef()
	{
		return ++m_refCount;
	}
	virtual unsigned int Release()
	{
		if( 0 == --m_refCount )
		{
			delete this;
			return 0;
		}
		return m_refCount;
	}
};

template <class T1, class T2 = RefCountedImplDummy>
class RefCountedCyclicImpl : public T1, public T2
{
	RefCountedCyclicImpl(const RefCountedCyclicImpl&); // no copy
	RefCountedCyclicImpl& operator = (const RefCountedCyclicImpl&);

	typedef std::map<RefCountedCyclic*, unsigned int> Referrers;

	class WatchImpl : public RefCountedImpl<Watch>
	{
	public:
		bool alive;
		WatchImpl()
			: alive(true)
		{
		}

		// Watch
		virtual bool IsAlive()
		{
			return alive;
		}
	};
	AutoPtr<WatchImpl> m_watch;
/*
	class SharedRefCounterImpl : public RefCountedImpl<RefCounted>
	{
	public:
		unsigned int m_refCountShared;
		SharedRefCounterImpl()
			: m_refCountShared(-1)
		{
		}
	};
	AutoPtr<SharedRefCounterImpl> m_sharedCounter;
*/
	unsigned int m_refCount;
	Referrers m_referrers;

	class MyRefTracer
	{
	public:
		MyRefTracer(RefCountedCyclic *initial)
			: m_visitor(initial)
			, m_result(initial->EnumReferrers(&m_visitor))
		{
		}
		operator bool () const
		{
			return m_result;
		}
	private:
		class Visitor : public T1::ReferrerVisitor
		{
		public:
			Visitor(RefCountedCyclic *initial)
			{
				m_visited.insert(initial);
			}
			// ReferrerVisitor
			virtual bool Visit(RefCountedCyclic *p)
			{
				if( m_visited.insert(p).second )
				{
					if( p->GetRefCount() || !p->EnumReferrers(this) )
					{
						return false;
					}
				}
				return true;
			}
			std::set<RefCountedCyclic*> m_visited;
		};
		Visitor m_visitor;
		bool m_result; // must be initialized last
	};

protected:
	RefCountedCyclicImpl()
		: m_refCount(-1)
		, m_watch(new WatchImpl())
//		, m_sharedCounter(new SharedRefCounterImpl())
	{
	}
	virtual ~RefCountedCyclicImpl()
	{
		assert(0 == m_refCount);
	}

	RefCountedCyclic* GetThis()
	{
		return static_cast<T1*>(this);
	}

public:

	//
	// RefCounted
	//
	virtual unsigned int AddRef()
	{
		assert(m_watch->alive);
		m_refCount = m_refCount == -1 ? 0 : m_refCount;
		return ++m_refCount;
	}
	virtual unsigned int Release()
	{
		assert(m_watch->alive);
		assert(m_refCount > 0 && m_refCount != -1);
		if( 0 == --m_refCount )
		{
			if( MyRefTracer(GetThis()) )
			{
				m_watch->alive = false;
				delete this;
			}
			return 0;
		}
		return m_refCount;
	}

	//
	// RefCountedCyclic
	//
	virtual unsigned int GetRefCount()
	{
		return m_refCount;
	}
	virtual unsigned int AddRefCyclic(RefCountedCyclic *referrer)
	{
		assert(m_watch->alive);
		return ++m_referrers[referrer];
	}
	virtual unsigned int ReleaseCyclic(RefCountedCyclic *referrer)
	{
		Referrers::iterator it = m_referrers.find(referrer);
		assert(m_watch->alive);
		assert(m_referrers.end() != it);
		if( 0 == --it->second )
		{
			m_referrers.erase(it);
			if( !m_refCount && MyRefTracer(GetThis()) )
			{
				m_watch->alive = false;
				delete this;
				return 0;
			}
		}
		return static_cast<unsigned int>(m_referrers.size());
	}
	virtual void GetWatch(Watch **ppResult)
	{
		AutoPtr<Watch> result(m_watch);
		DetachRawPtr(result, ppResult);
	}
	virtual bool EnumReferrers(typename T1::ReferrerVisitor *visitor)
	{
		for( Referrers::iterator it = m_referrers.begin(); it != m_referrers.end(); ++it )
		{
			if( !visitor->Visit(it->first) )
				return false;
		}
		return true;
	}
};

///////////////////////////////////////////////////////////////////////////////

template <class T, class PtrType>
class WeakTargetImpl;

template <class T, class PtrType>
class WeakPtrImpl : public RefCountedImpl<collada::WeakPtr<PtrType> >
{
public:
	// WeakPtr
	virtual bool GetPtr(PtrType** ppResult)
	{
		assert(!ppResult || NULL == *ppResult);
		if( m_ptr )
		{
			if( ppResult )
			{
				m_ptr->AddRef();
				*ppResult = m_ptr;
			}
			return true;
		}
		return false;
	}
private:
	friend class WeakTargetImpl<T, PtrType>;
	WeakPtrImpl(PtrType *ptr)
		: m_ptr(ptr)
	{
	}
	PtrType *m_ptr;
};

#pragma warning (push)
#pragma warning (disable: 4355) // 'this' : used in base member initializer list

template <class T, class PtrType>
class WeakTargetImpl : public RefCountedImpl<T>
{
public:
	// WeakTarget
	virtual void GetWeakPtr(collada::WeakPtr<PtrType> **ppResult)
	{
		assert(NULL == *ppResult);
		*ppResult = m_weakPtr;
		(*ppResult)->AddRef();
	}
protected:
	WeakTargetImpl()
	  : m_weakPtr(new WeakPtrImpl<T, PtrType>(this))
	{
	}
	virtual ~WeakTargetImpl()
	{
		m_weakPtr->m_ptr = NULL;
	}
private:
	collada::AutoPtr<WeakPtrImpl<T, PtrType> > m_weakPtr;
};

#pragma warning (pop)

}; // end of namespace collada

// end of file
