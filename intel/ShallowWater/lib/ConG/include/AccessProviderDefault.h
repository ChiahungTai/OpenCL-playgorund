/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file AccessProviderDefault.h

#pragma once

#include "MemoryManager.h"

namespace collada
{
	struct AccessorWriteUpdate : public AccessorWrite
	{
		virtual void Update(const void *data) = 0;
	};

	struct AccessorWriteDefault : public AccessorWrite
	{
		virtual void* Map() = 0; ///< Discards previous content
		virtual void Unmap() = 0;
	};

	struct AccessorReadDefault : public AccessorRead
	{
		virtual const void* Map() = 0;
		virtual void Unmap() = 0;
	};

	struct AccessProviderLocal : public AccessProvider
	{
		virtual void CreateAccessorWriteUpdate(MemoryObject *resource, AccessorWriteUpdate **result, Processor *p) = 0;
		virtual void CreateAccessorWriteDefault(MemoryObject *resource, AccessorWriteDefault **result, Processor *p) = 0;
		virtual void CreateAccessorReadDefault(MemoryObject *resource, AccessorReadDefault **result) = 0;
	};

	void CreateAccessProviderLocal(AccessProviderLocal **ppResult);

///////////////////////////////////////////////////////////////////////////////

	template<class ElementType, class AccessorType>
	class MapHelper
	{
	public:
		const size_t count;
		// for performance reasons MapHelper will not addref the accessor
		MapHelper(AccessorType *accessor)
			: m_accessor((assert(accessor), accessor))
			, m_ptr(reinterpret_cast<ElementType*>(accessor->Map()))
			, count(accessor->GetMemoryObject()->GetBytesSize() / sizeof(ElementType))
		{
			assert(accessor->GetMemoryObject()->GetBytesSize() % sizeof(ElementType) == 0);
		}
		~MapHelper()
		{
			m_accessor->Unmap();
		}
		ElementType& operator[] (size_t idx) const
		{
			assert(idx < count);
			return m_ptr[idx];
		}
	private:
		ElementType * const m_ptr;
		AccessorType * const m_accessor;
		void operator = (const MapHelper&); // no copy
		MapHelper(const MapHelper&);
	};
} // end of namespace collada
// end of file
