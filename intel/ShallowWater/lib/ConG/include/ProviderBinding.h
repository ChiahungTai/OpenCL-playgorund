/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file ProviderBinding.h

#pragma once

#include "RefCounted.h"


namespace collada
{
	struct ProviderBinding;
	struct AccessProvider;
	struct Processor;
	struct MemoryObject;

enum AccessPattern
{
	ACCESS_PATTERN_NONE = 0,
	ACCESS_PATTERN_READ = 1,
	ACCESS_PATTERN_WRITE = 2,
	ACCESS_PATTERN_UPDATE = 4,
};

// access provider implementation stores this interface inside the attached memory object
struct PrivateProviderData : public RefCountedCyclic
{
	virtual unsigned int GetAccessPattern() const = 0;
	virtual void* MapPrivate() = 0;
	virtual void UnmapPrivate() = 0;
};

struct IPrivateDataFactory
{
	virtual PrivateProviderData* NewPrivateData(ProviderBinding *target) = 0;
};

struct ProviderBinding : public RefCountedCyclic
{
	// ProviderBinding does not addref PrivateProviderData
	virtual void GetPrivateData(AccessProvider *p, IPrivateDataFactory *initializer, PrivateProviderData **ppResult) = 0;
	virtual void Dispose(PrivateProviderData *what) = 0; /// called by framework when the private data is gone

	virtual unsigned int GetCommonAccessPattern(const PrivateProviderData *exclude) const = 0;

	virtual void IncrementContentVersion() = 0;
	virtual void UpdateBinding(const void *data) = 0;
	virtual void* MapBinding() = 0;
	virtual void UnmapBinding() = 0;

	virtual bool NeedBackup(PrivateProviderData *source) const = 0;
	virtual void Backup(PrivateProviderData *source, const void *data) = 0;
	virtual const void* GetBackupData() = 0;

	virtual void SetProcessor(Processor *p) = 0;
	virtual Processor* GetProcessor() const throw() = 0;

	virtual MemoryObject* GetMemoryObject() throw() = 0;
};

} // end of namespace collada
// end of file
