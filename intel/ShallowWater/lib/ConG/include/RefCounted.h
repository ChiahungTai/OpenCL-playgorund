/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2009 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file RefCounted.h

#pragma once

namespace collada
{
	/// Adds reference counting capability to an object.
	struct RefCounted
	{
		/// Increments the reference count of the object by one.
		virtual unsigned int AddRef() = 0;

		/// Decrements the reference count of the object by one.
		///
		/// When the final Release is called, object is destroyed.
		virtual unsigned int Release() = 0;
	};

	struct Watch : public RefCounted
	{
		virtual bool IsAlive() = 0;
	};

	/// Adds referring objects tracking capability to deal with cyclic references
	struct RefCountedCyclic : public RefCounted
	{
		virtual unsigned int GetRefCount() = 0;
		virtual unsigned int AddRefCyclic(RefCountedCyclic *referrer) = 0;
		virtual unsigned int ReleaseCyclic(RefCountedCyclic *referrer) = 0;

		virtual void GetWatch(Watch **ppResult) = 0;

		struct ReferrerVisitor
		{
			virtual bool Visit(RefCountedCyclic *p) = 0; // return false to stop visiting
		};
		virtual bool EnumReferrers(ReferrerVisitor *visitor) = 0; // return false if visitor returned false
	};

	template <class T>
	struct WeakPtr : public RefCounted
	{
		virtual bool GetPtr(T** ppResult) = 0;
	};

	template <class T>
	struct WeakTarget : public RefCounted
	{
		virtual void GetWeakPtr(WeakPtr<T> **ppResult) = 0;
	};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace collada
// end of file
