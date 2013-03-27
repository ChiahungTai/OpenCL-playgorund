/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file NodeControlPluginImpl.h

#pragma once

#include "NodeControlPlugin.h"

#include "mathstub.h"
#include "SceneGraphBase.h"

///////////////////////////////////////////////////////////////////////////////

class NodeControlViewImpl : public collada::INodeControlView
{
public:
	NodeControlViewImpl(collada::Node *node, collada::Scene *context);
	~NodeControlViewImpl();

	// INodeControlView
	virtual void Reset();
	virtual void TranslateDelta(float x, float y, float z);
	virtual void RotateDeltaFP(float yaw, float pitch, float roll);
	virtual void RotateDelta(float yaw, float pitch, float roll);
	virtual void RemoveRoll(const float *pCOI3);

	// IView
	virtual void OnDetach();

private:
	collada::Scene *m_context;
	collada::Node *m_node;
	collada::Property *m_transform;
	float3 m_coi;
	float m_scale;
};

///////////////////////////////////////////////////////////////////////////////

class NodeControlViewFactory : public collada::IViewFactory
{
public:
	NodeControlViewFactory();
	~NodeControlViewFactory();

	// IViewFactory
	collada::IView* CreateView(collada::Scene *context, collada::Base *b, size_t thisViewId);
	void OnDetach();
};


// end of file
