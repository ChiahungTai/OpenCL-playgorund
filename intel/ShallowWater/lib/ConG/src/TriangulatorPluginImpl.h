/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file TriangulatorPluginImpl.h

#pragma once

#include "TriangulatorPlugin.h"

#include "SceneGraph.h"
#include "SGMeshAdapter.h"
#include "SGSceneAdapter.h"
#include "SGGeometryInstanceAdapter.h"

using namespace collada;
namespace collada
{
	struct AccessProviderLocal;
}

///////////////////////////////////////////////////////////////////////////////

class TriangleBatchImpl : public BatchAdapter
{
public:
	TriangleBatchImpl(Batch *src, AccessProviderLocal *providerLocal, Scene *context);

	// Batch interface override
	virtual MemoryObject* GetIndexData() const;
	virtual MemoryObject* GetCountData() const;
	virtual PrimitiveTopology GetPrimitiveTopology() const;

private:
	AutoPtr<MemoryObject> m_idxbuf;
};

///////////////////////////////////////////////////////////////////////////////

class TriangleBatchViewImpl : public ITriangleBatchView
{
public:
	TriangleBatchViewImpl(Batch *srcBatch, AccessProviderLocal *providerLocal, Scene *context);

	// ITriangleBatchView interface implementation
	virtual Batch* GetTriangleBatch();

	// IView interface implementation
	virtual void OnDetach();

private:
	Batch *m_source;
	std::auto_ptr<TriangleBatchImpl> m_batch;
};

///////////////////////////////////////////////////////////////////////////////

class TriangulatorPluginFactory : public collada::IViewFactory
{
public:
	TriangulatorPluginFactory(AccessProviderLocal *providerLocal);

	// IViewFactory interface implementation
	virtual IView* CreateView(Scene *context, Base *b, size_t thisViewId);
	virtual void OnDetach();

private:
	AutoPtr<AccessProviderLocal> m_provider;
};

///////////////////////////////////////////////////////////////////////////////

class TriangulatedSceneAdapter : public SceneAdapter
{
public:
	TriangulatedSceneAdapter(AccessProviderLocal *providerLocal, Scene *src, size_t triPluginId);

	// Scene interface override
	virtual Base* ResolveContext(Base *target);
};

// end of file
