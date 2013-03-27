/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
        Copyright (C) 2009-2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file NodeControlPluginImpl.cpp

#include "stdafx.h"

#include "mathstub.h"
#include "NodeControlPluginImpl.h"
#include "SceneGraph.h"

static const float I[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};

NodeControlViewImpl::NodeControlViewImpl(collada::Node *node, collada::Scene *context)
  : m_node(node)
  , m_coi(0,0,0)
  , m_context(context)
{
	assert(m_node);
	float det = MatrixDeterminant((const float4x4 *) m_node->GetWorld(m_context));
	m_scale = 1.0f / pow(det, 1.0f/3);
	m_transform = m_node->AppendTransform(collada::Transform::MATRIX, I)->GetDelegate(NULL);
}

NodeControlViewImpl::~NodeControlViewImpl()
{
}

void NodeControlViewImpl::Reset()
{
	m_transform->SetValue(I);
}

void NodeControlViewImpl::TranslateDelta(float x, float y, float z)
{
	assert(m_transform->GetSize() == 16);
	float4x4 m;
	m_transform->GetValue(m);
	MatrixTranspose(&m, &m);
	float4x4 t;
	MatrixTranslation(&t, x * m_scale, y * m_scale, z * m_scale);
	MatrixTranspose(&m, &(t*m));
	m_transform->SetValue(m);
}

void NodeControlViewImpl::RotateDeltaFP(float yaw, float pitch, float roll)
{
	collada::Property* pProp = m_transform;
	if( pProp )
	{
		assert(pProp->GetSize()==16);
		float4x4 m;
		pProp->GetValue(m);
		MatrixTranspose(&m, &m);
		float4x4 ypr; 
		MatrixRotationYawPitchRoll(&ypr, yaw, pitch, roll);
		MatrixTranspose(&m, &(ypr*m));
		pProp->SetValue(m);

//		RemoveRoll();
	}
}

void NodeControlViewImpl::RotateDelta(float yaw, float pitch, float roll)
{
	collada::Property* pProp = m_transform;

	float3 coi = m_coi;

	float4x4 m;
	pProp->GetValue(m);
	MatrixTranspose(&m, &m);
	float4x4 w = m_node->GetWorld(NULL);
	float4x4 wI = m_node->GetWorldI(NULL);
	float4x4 mcoi(w);

	mcoi(3,0) = coi.x;
	mcoi(3,1) = coi.y;
	mcoi(3,2) = coi.z;

	float4x4 toCOI = mcoi*wI;
	float4x4 fromCOI;
	MatrixInverse(&fromCOI, 0, &toCOI);
	float4x4 ypr; 
	MatrixRotationYawPitchRoll(&ypr, yaw, pitch, roll);
	float4x4 result = fromCOI*ypr*toCOI;
	MatrixMultiply(&m, &result, &m);

	float4x4 m0 = m;
	MatrixTranspose(&m, &m);
	pProp->SetValue(m);

//	RemoveRoll(&coi);
}

void NodeControlViewImpl::RemoveRoll(const float *pCOI3)
{
	collada::Property* pProp = m_transform;

	float4x4 w2 = m_node->GetWorld(NULL);
	float4x4 upA = m_context->GetUpAxis();

	float4x4 w2Up = w2*upA;

	float4x4 rot;
	float xz2 = sqrt(w2Up._11*w2Up._11+w2Up._13*w2Up._13);
	float alpha = -atan(w2Up._12/xz2);

	MatrixRotationZ(&rot, alpha);

	float4x4 m0;
	pProp->GetValue(m0);
	MatrixTranspose(&m0, &m0);


	if( pCOI3 )
	{
		float4x4 mcoi(w2);

		mcoi(3,0) = pCOI3[0]; // x
		mcoi(3,1) = pCOI3[1]; // y
		mcoi(3,2) = pCOI3[2]; // z

		float4x4 wI = m_node->GetWorldI(NULL);
		float4x4 toCOI = mcoi*wI;
		float4x4 fromCOI;
		MatrixInverse(&fromCOI, 0, &toCOI);
		rot = fromCOI*rot*toCOI;
	}
	float4x4 m = rot*m0;
	MatrixTranspose(&m, &m);
	pProp->SetValue(m);
}

void NodeControlViewImpl::OnDetach()
{
	delete this;
}

///////////////////////////////////////////////////////////////////////////////

NodeControlViewFactory::NodeControlViewFactory()
{
}

NodeControlViewFactory::~NodeControlViewFactory()
{
}

collada::IView* NodeControlViewFactory::CreateView(collada::Scene *context, collada::Base *b, size_t thisViewId)
{
	if( collada::Node *node = dynamic_cast<collada::Node *>(b) )
	{
		return new NodeControlViewImpl(node, context);
	}
	return NULL;
}

void NodeControlViewFactory::OnDetach()
{
	delete this;
}


// factory

namespace collada
{
	IViewFactory* CreateDefaultNodeControlPlugin()
	{
		return new NodeControlViewFactory();
	}
}


// end of file
