/* ************************************************************************* *\
				  INTEL CORPORATION PROPRIETARY INFORMATION
	 This software is supplied under the terms of a license agreement or
	 nondisclosure agreement with Intel Corporation and may not be copied
	 or disclosed except in accordance with the terms of that agreement.
		  Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file AutoPtr.h

#pragma once

#include "RefCounted.h"
#include <cassert>

namespace collada
{

///////////////////////////////////////////////////////////////////////////////

template <class T, class AddRefferType>
class AutoPtrBase
{
protected:
	T *_ptr;
	AddRefferType _addReffer;

	class NoAddRefRelease : public T
	{
		virtual unsigned int AddRef() = 0;
		virtual unsigned int Release() = 0;
		virtual ~NoAddRefRelease() = 0 {}
	};

	struct NoRawPtrCast
	{
		char placeholder[sizeof(AutoPtrBase)];
	};

	AutoPtrBase(const AutoPtrBase &other)
		: _ptr(other._ptr)
		, _addReffer(other._addReffer)
	{
		if( _ptr )
			_addReffer.AddRef(_ptr);
	}
	AutoPtrBase(T *f, const AddRefferType &addReffer)
		: _ptr(f)
		, _addReffer(addReffer)
	{
		if( _ptr )
			_addReffer.AddRef(_ptr);
	}
	~AutoPtrBase()
	{
		if( _ptr )
			_addReffer.Release(_ptr);
	}

public:


	//
	// dereference
	//

	NoAddRefRelease* operator -> () const throw()
	{
		assert(_ptr);
		return static_cast<NoAddRefRelease*>(_ptr);
	}

	operator T* () const throw()
	{
		return _ptr;
	}


	//
	// Pointer cast
	//

	operator const NoRawPtrCast&() const
	{
		return reinterpret_cast<const NoRawPtrCast&>(*this);
	}
	template <class TargetType>
	friend TargetType SafePtrCast(const NoRawPtrCast &r)
	{
		assert(dynamic_cast<TargetType>(reinterpret_cast<const AutoPtrBase&>(r)._ptr));
		return static_cast<TargetType>(reinterpret_cast<const AutoPtrBase&>(r)._ptr);
	}


	//
	// assignment
	//
	AutoPtrBase& operator = (T *p)
	{
		if( _ptr )
			_addReffer.Release(_ptr);
		_ptr = p;
		if( _ptr )
			_addReffer.AddRef(_ptr);
		return *this;
	}
	AutoPtrBase& operator = (const AutoPtrBase &p)
	{
		if( _ptr )
			_addReffer.Release(_ptr);
		_ptr = p._ptr;
		if( _ptr )
			_addReffer.AddRef(_ptr);
		return *this;
	}
/*	template <class U, class A>
	const AutoPtrBase& operator = (const AutoPtrBase<U,A> &p)
	{
		if( _ptr )
			_addReffer.Release(_ptr);
		_ptr = p; // uses operator U*
		if( _ptr )
			_addReffer.AddRef(_ptr);
		return *this;
	}
*/

	//
	// Equality and Inequality
	//
/*	template <class U>
	inline friend bool operator==(const AutoPtr &l, const U *r) throw()
	{
		return l._ptr == r;
	} */
	template <class U>
	inline friend bool operator==(const U *l, const AutoPtrBase &r) throw()
	{
		return l == r._ptr;
	}
	template <class U>
	inline friend bool operator!=(const AutoPtrBase &l, const U *r) throw()
	{
		return l._ptr != r;
	}
	template <class U>
	inline friend bool operator!=(const U *l, const AutoPtrBase &r) throw()
	{
		return l != r._ptr;
	}
	template <class U, class A> // for comparing safe pointers of different types
	bool operator==(const AutoPtrBase<U, A> &r) const throw()
	{
		return _ptr == r; // uses operator U*
	}
	template <class U, class A> // for comparing safe pointers of different types
	bool operator!=(const AutoPtrBase<U, A> &r) const throw()
	{
		return _ptr != r; // uses operator U*
	}
};


//
// enable instantiation of AutoPtr<void>
//

//template<> AutoPtrBase<void>::AutoPtrBase(void *f) : _ptr(NULL) {}
//template<> AutoPtrBase<void>::~AutoPtrBase() {}

///////////////////////////////////////////////////////////////////////////////

template <class T>
class AddRefferDefault
{
public:
	unsigned int AddRef(T *what)
	{
		return what->AddRef();
	}
	unsigned int Release(T *what)
	{
		return what->Release();
	}
};


template <class T>
class AutoPtr : public AutoPtrBase<T, AddRefferDefault<T> >
{
	class RefT
	{
		AutoPtr m_me;
	public:
		operator T*const* () const
		{
			return &(m_me._ptr);
		}
		operator T** ()
		{
			assert(!m_me._ptr);
			return &(m_me._ptr);
		}
		operator AutoPtr* ()
		{
			return (AutoPtr*)&reinterpret_cast<int&>(m_me);
		}
		operator const AutoPtr* () const
		{
			return (const AutoPtr*)&reinterpret_cast<const int&>(m_me);
		}
	};

public:
	//
	// construction
	//
	AutoPtr() throw()
		: AutoPtrBase(NULL, AddRefferDefault<T>())
	{
	}
	AutoPtr(T *f) // implicit construction from raw pointer
		: AutoPtrBase(f, AddRefferDefault<T>())
	{
	}
	//	AutoPtr(const AutoPtr &f)
	//		: AutoPtrBase(f._ptr, AddRefferDefault<T>())
	//	{
	//	}
	template <class U>
	AutoPtr(const AutoPtr<U> &f) // initialize from another type of safe pointer
		: AutoPtrBase(f, AddRefferDefault<T>())
	{
	}


	//
	// dereference
	//

	RefT& operator & () throw()
	{
		return reinterpret_cast<RefT&>(*this);
	}

	const RefT& operator & () const throw()
	{
		return reinterpret_cast<const RefT&>(*this);
	}


	//
	// Direct access access to _ptr
	//
	template <class U>
	friend void DetachRawPtr(AutoPtr &r, U **pp) throw()
	{
		assert(NULL == *pp);
		*pp = r._ptr;
		r._ptr = NULL;
	}
	friend unsigned int SafeReleasePtr(AutoPtr &r)
	{
		if( T *tmp = r._ptr )
		{
			r._ptr = NULL; // for case when SafeReleasePtr is called again during Release()
			return tmp->Release();
		}
		return 0;
	}

	using AutoPtrBase<T, AddRefferDefault<T> >::operator =;
};

///////////////////////////////////////////////////////////////////////////////

template <class T>
class AddRefferCyclic
{
	RefCountedCyclic *_referrer;
	AutoPtr<Watch> _watch;
public:
	AddRefferCyclic(const AddRefferCyclic &other)
		: _referrer(other._referrer)
	{
	}
	AddRefferCyclic(RefCountedCyclic *referrer)
		: _referrer(referrer)
	{
		assert(_referrer);
	}
	unsigned int AddRef(T *what)
	{
		assert(_referrer);
		assert(!_watch);
		what->GetWatch(&_watch);
		return what->AddRefCyclic(_referrer);
	}
	unsigned int Release(T *what)
	{
		assert(_referrer);
		bool alive = _watch->IsAlive();
		SafeReleasePtr(_watch);
		return alive ? what->ReleaseCyclic(_referrer) : 0;
	}
};

class Referrer
{
public:
	explicit Referrer(RefCountedCyclic *r)
		: _r(r)
	{
	}
	RefCountedCyclic * _r;
};

template <class T>
class AutoPtrCyclic : public AutoPtrBase<T, AddRefferCyclic<T> >
{
public:
	//
	// construction
	//
	AutoPtrCyclic(const Referrer &referrer)
		: AutoPtrBase(NULL, referrer._r)
	{
	}
	AutoPtrCyclic(const Referrer &referrer, T *f)
		: AutoPtrBase(f, referrer._r)
	{
	}
	template <class U, class A>
	AutoPtrCyclic(const Referrer &referrer, const AutoPtrBase<U, A> &f) // initialize from another type of safe pointer
		: AutoPtrBase(f, referrer._r)
	{
	}

	//friend bool SafeCheckPtr(AutoPtrCyclic &r)
	//{
	//	return r ? r._addReffer.watch->IsAlive() : false;
	//}

	using AutoPtrBase<T, AddRefferCyclic<T> >::operator =;
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace collada
// end of file
