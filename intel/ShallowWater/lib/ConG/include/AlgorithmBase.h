/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file AlgorithmBase.h

#pragma once

#include "SceneGraphBase.h"
#include "RefCounted.h"

#include <cassert>

namespace collada
{
	struct Accessor;
	struct AccessorRead;
	struct AccessorWrite;
	struct Processor;

	struct AlgorithmResource
	{
		unsigned int semanticId;
		MemoryObject *data;
	};

	struct AlgorithmResourceAccessor
	{
		unsigned int semanticId;
		Accessor *accessor;
	};

	/// Processor provides this interface to track resource allocation by algorithm
	struct IResourceTracker
	{
		virtual void OnCreateReadAccessor(unsigned int semanticId, AccessorRead *accessor) = 0;
		virtual void OnCreateWriteAccessor(unsigned int semanticId, AccessorWrite *accessor) = 0;
	};

	/// Note that algorithm may not have more than one accessor per resource
	struct IAlgorithm : public RefCounted
	{
		virtual void CreateAccessors(const AlgorithmResource *inputs, IResourceTracker *tracker, Processor *p) const = 0;
	};

	/// Use this interface when you don't need additional arguments for DoProcess function. Otherwise define your own algorithm interface.
	struct IAlgorithmSimple : public IAlgorithm
	{
		virtual void DoProcess(const AlgorithmResourceAccessor *accessors) const = 0;
	};

	//////////////////////////////////////////////////////////////////////////////

	template <class ResourceType>
	inline const ResourceType* FindResourceById(const ResourceType *reslist, unsigned int id, unsigned int index = 0)
	{
		assert(reslist);
		for(; reslist->semanticId != -1; ++reslist )
		{
			if( reslist->semanticId == id )
			{
				if( 0 == index )
				{
					return reslist;
				}
				--index;
			}
		}
		return NULL;
	}

	template <class AccessorType>
	class FindAccessor
	{
	public:
		FindAccessor(const AlgorithmResourceAccessor *accessors, unsigned int id, unsigned int index = 0)
		{
			assert(FindResourceById(accessors, id, index));
			assert(dynamic_cast<AccessorType*>(FindResourceById(accessors, id, index)->accessor));
			m_accessor = static_cast<AccessorType*>(FindResourceById(accessors, id, index)->accessor);
		}
		operator AccessorType* () const
		{
			return m_accessor;
		}
		AccessorType* operator -> () const
		{
			return m_accessor;
		}
	private:
		AccessorType *m_accessor;
	};

} // end of namespace collada
// end of file
