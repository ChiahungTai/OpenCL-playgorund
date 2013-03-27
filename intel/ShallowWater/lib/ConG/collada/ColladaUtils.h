/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file ColladaUtils.h

#pragma once

#include "SceneGraphBase.h"

#include <dae.h> // for daeSafeCast
#include <dom/domCOLLADA.h>

#include <map>
#include <vector>

///////////////////////////////////////////////////////////////////////////////

// array of pointers; it calls delete on each stored pointer when the array is being destroyed
template <class T>
class PtrOwnerArray : public std::vector<T *>
{
public:
	PtrOwnerArray() {}
	~PtrOwnerArray()
	{
		for( iterator it = begin(); it != end(); ++it )
		{
			delete *it;
		}
	}
};

///////////////////////////////////////////////////////////////////////////////

template<size_t N>
class ArrayConvert
{
public:
	ArrayConvert(const domListOfFloats &src)
	{
		assert(src.getCount() >= N);
		for( size_t i = 0; i < N; ++i )
		{
			m_data[i] = (float) src[i];
		}
	}
	operator const float* () const
	{
		return m_data;
	}
private:
	float m_data[N];
};

///////////////////////////////////////////////////////////////////////////////

// helper class that allows using of zero-terminated string as map key
class StringRef
{
	const char *m_str;
public:
	StringRef(const char *str)
	  : m_str(str)
	{
	}
	bool operator < (const StringRef &r) const
	{
		return strcmp(m_str, r.m_str) < 0;
	}
};

///////////////////////////////////////////////////////////////////////////////

// resource cache

template
<
	class KeyTypeNoPtr, 
	class ValueType, 
	class Arg = void
>
class ResourceCache
{
	Arg * const m_arg;

	typedef std::map<KeyTypeNoPtr*, ValueType *> CacheType;
	CacheType m_cache;
	PtrOwnerArray<ValueType> m_storage;

	ResourceCache(const ResourceCache&); // protect from copy

public:
	ResourceCache(Arg *arg) throw()
	  : m_arg(arg)
	{
	}

	unsigned int FindPointer(const ValueType *ptr) const
	{
		for( size_t i = 0; i < m_storage.size(); ++i )
		{
			if( m_storage[i] == ptr )
			{
				return static_cast<unsigned int>(i);
			}
		}
		return -1;
	}

	ValueType* GetPointer(size_t index) const
	{
		return m_storage[index];
	}

	size_t GetSize() const
	{
		return m_storage.size();
	}

	ValueType* LocateKey(KeyTypeNoPtr* key)
	{
		CacheType::const_iterator it = m_cache.find(key);
		if( m_cache.end() != it )
		{
			// return existing
			return it->second;
		}

		// create new
		m_storage.push_back(new ValueType(key, m_arg));
		ValueType *result = m_storage.back();
		m_cache[key] = result;

		return result;
	}
};

///////////////////////////////////////////////////////////////////////////////

template <class T>
class SafeResolve
{
public:
	template <class Resolver>
	SafeResolve(Resolver &r)
		: m_resolved(NULL)
	{
		daeElement *tmp = r.getElement();

		if( !tmp )
		{
			throw std::runtime_error(std::string("invalid reference '") + GetId(r) + "' of " + typeid(Resolver).name() + " - target object not found");
		}

		m_resolved = Cast(tmp);
		if( !m_resolved )
		{
			std::ostringstream buf;
			buf << typeid(Resolver).name();
			buf << " reference '" << GetId(r) << "' points to incompatible object";
			buf << " (" << typeid(T).name() << " expected, got " << typeid(*tmp).name() << ")";
			throw std::runtime_error(buf.str());
		}
	}

	operator T* ()
	{
		return m_resolved;
	}

private:
	T *m_resolved;
	static T* Cast(daeElement *arg) { return daeSafeCast<T>(arg); }
	static const char* SafeStr(const char *s)         { return s ? s : ""; }
	static const char* GetId(const daeIDRef &r)       { return SafeStr(r.getID());           }
	static const char* GetId(const daeURI &r)         { return SafeStr(r.getOriginalURI());  }
	static const char* GetId(const daeSIDResolver &r) { return SafeStr(r.getTarget());       }
};

template <>
daeElement* SafeResolve<daeElement>::Cast(daeElement *arg)
{
	return arg;
}

// end of file
