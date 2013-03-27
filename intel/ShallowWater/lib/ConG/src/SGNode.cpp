/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGNode.cpp

#include "stdafx.h"

#include "mathstub.h"
#include "SSEOps.h"

#include "SGNode.h"
#include "SGProperty.h"


///////////////////////////////////////////////////////////////////////////////

TransformBaseImpl::TransformBaseImpl(NodeImpl *node)
  : m_node(node)
  , m_isCacheValid(false)
{
	m_cachedMatrix = SSEAlign( m_cachedMatrixU );
}

TransformBaseImpl::~TransformBaseImpl()
{
}

PropertyImpl* TransformBaseImpl::RegisterProperty(PropertyImpl *d)
{
	m_delegates.push_back(d);
	return d;
}

void TransformBaseImpl::OnPropertyChange()
{
	if( m_isCacheValid )
	{
		m_isCacheValid = false;
		m_node->InvalidateTransformCache();
	}
}

const float* TransformBaseImpl::GetMatrix() const
{
	if( !m_isCacheValid )
	{
		float4x4 m;
		FillMatrix(&m);
		memcpy( m_cachedMatrix, (float*)m, SSEMatSize );
		m_isCacheValid = true;
	}
	return m_cachedMatrix;
}

///////////////////////////////////////////////////////////////////////////////

// translation
TransformTranslate::TransformTranslate(NodeImpl *node, const float *data3)
  : TransformBaseImpl(node)
  , m_translateX(data3[0])
  , m_translateY(data3[1])
  , m_translateZ(data3[2])
{
}

void TransformTranslate::FillMatrix(float4x4 *m) const
{
	MatrixTranslation(m, m_translateX, m_translateY, m_translateZ);
}

Property* TransformTranslate::GetDelegate(const char *param)
{
	if( param )
	{
		if( 0 == _stricmp("X", param) || 0 == strcmp("(0)", param) )
		{
			return RegisterProperty(new PropertyImpl(this, &m_translateX, 1, "translate.x"));
		}
		if( 0 == _stricmp("Y", param) || 0 == strcmp("(1)", param) )
		{
			return RegisterProperty(new PropertyImpl(this, &m_translateY, 1, "translate.y"));
		}
		if( 0 == _stricmp("Z", param) || 0 == strcmp("(2)", param) )
		{
			return RegisterProperty(new PropertyImpl(this, &m_translateZ, 1, "translate.z"));
		}
	}
	else
	{
		// return delegate to entire contents
		return RegisterProperty(new PropertyImpl(this, &m_translateX, 3, "translate"));
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// scaling
TransformScale::TransformScale(NodeImpl *node, const float *data3)
  : TransformBaseImpl(node)
  , m_scaleX(data3[0])
  , m_scaleY(data3[1])
  , m_scaleZ(data3[2])
{
}

void TransformScale::FillMatrix(float4x4 *m) const
{
	MatrixScaling(m, m_scaleX, m_scaleY, m_scaleZ);
}

Property* TransformScale::GetDelegate(const char *param)
{
	if( param )
	{
		if( 0 == _stricmp("X", param) || 0 == strcmp("(0)", param) )
		{
			return RegisterProperty(new PropertyImpl(this, &m_scaleX, 1, "scale.x"));
		}
		if( 0 == _stricmp("Y", param) || 0 == strcmp("(1)", param) )
		{
			return RegisterProperty(new PropertyImpl(this, &m_scaleY, 1, "scale.y"));
		}
		if( 0 == _stricmp("Z", param) || 0 == strcmp("(2)", param) )
		{
			return RegisterProperty(new PropertyImpl(this, &m_scaleZ, 1, "scale.z"));
		}
	}
	else
	{
		// return delegate to entire contents
		return RegisterProperty(new PropertyImpl(this, &m_scaleX, 3, "scale"));
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// rotation
TransformRotate::TransformRotate(NodeImpl *node, const float *data4)
  : TransformBaseImpl(node)
  , m_axisAndAngle(data4)
{
}

void TransformRotate::FillMatrix(float4x4 *m) const
{	
	MatrixRotationAxis( m, (float3*)&m_axisAndAngle, (float) (m_axisAndAngle.w * M_PI / 180.0) );
}

Property* TransformRotate::GetDelegate(const char *param)
{
	if( param )
	{
		if( 0 == _stricmp("ANGLE", param) || 0 == strcmp("(3)", param)  )
		{
			return RegisterProperty(new PropertyImpl(this, &m_axisAndAngle.w, 1, "rotation.angle"));
		}
	}
	else
	{
		// return delegate to entire contents
		return RegisterProperty(new PropertyImpl(this, m_axisAndAngle, 4, "rotation"));
	}
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// matrix

TransformMatrix::TransformMatrix(NodeImpl *node, const float *data4x4)
  : TransformBaseImpl(node)
{
	memcpy(m_matrix, data4x4, sizeof(float) * 4*4);
}

void TransformMatrix::FillMatrix(float4x4 *m) const
{
	// matrix is transposed to comply with dx row-vectors
	*m = float4x4(m_matrix[0x0], m_matrix[0x4], m_matrix[0x8], m_matrix[0xC],
	                  m_matrix[0x1], m_matrix[0x5], m_matrix[0x9], m_matrix[0xD],
	                  m_matrix[0x2], m_matrix[0x6], m_matrix[0xA], m_matrix[0xE],
	                  m_matrix[0x3], m_matrix[0x7], m_matrix[0xB], m_matrix[0xF]);
}

Property* TransformMatrix::GetDelegate(const char *param)
{
	if( param )
	{
		// (x)(y)
		char bufCopy[6];
		if( sizeof(bufCopy) != strlen(param)
			|| param[0] != '(' || param[2] != ')' || param[3] != '(' || param[5] != ')' )
		{
			return NULL;
		}
		memcpy(bufCopy, param, sizeof(bufCopy));
		bufCopy[2] = 0;
		bufCopy[5] = 0;
		int x = atoi(bufCopy + 1);
		int y = atoi(bufCopy + 4);
		return RegisterProperty(new PropertyImpl(this, &m_matrix[x + 4*y], 1, "matrix.element"));
	}
	else
	{
		// return delegate to entire contents
		return RegisterProperty(new PropertyImpl(this, m_matrix, 16, "transform"));
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////

NodeImpl::NodeImpl(Scene *s, NodeImpl *parent)
  : BaseImpl(s)
  , m_parent(parent)
  , m_isValidLocal(false)
  , m_isValidWorld(false)
  , m_isValidWorldI(false)
  , m_layers(0)
{
	if( m_parent )
	{
		m_parent->m_children.push_back(this);
	}
	m_local = SSEAlign( m_localU );
	m_world = SSEAlign( m_worldU );
}

NodeImpl::~NodeImpl()
{
}

Transform* NodeImpl::AppendTransform(Transform::Type type, const float *data)
{
	TransformBaseImpl *t = NULL;

	switch( type )
	{
	case Transform::LOOKAT:
		// TODO: implement
		break;
	case Transform::SKEW:
		// TODO: implement
		break;
	case Transform::MATRIX:
		t = new TransformMatrix(this, data);
		break;
	case Transform::ROTATE:
		t = new TransformRotate(this, data);
		break;
	case Transform::SCALE:
		t = new TransformScale(this, data);
		break;
	case Transform::TRANSLATE:
		t = new TransformTranslate(this, data);
		break;
	} // end of switch( type )

	assert(t);
	m_transforms.push_back(t);
	InvalidateTransformCache();
	return t;
}

void NodeImpl::InvalidateTransformCache()
{
	if( m_isValidLocal )
	{
		m_isValidLocal = false;
		OnInvalidateParentTransform();
	}
}

void NodeImpl::OnInvalidateParentTransform()
{
	if( m_isValidWorld )
	{
		m_isValidWorld = false;
		m_isValidWorldI = false;
		for( size_t i = 0; i < m_children.size(); ++i )
		{
			m_children[i]->OnInvalidateParentTransform();
		}
		for( size_t i = 0; i < m_callbacks.size(); ++i )
		{
			m_callbacks[i]->OnNodeUpdate(this);
		}
	}
}

void NodeImpl::AddListener(INodeListener *listener)
{
	m_callbacks.push_back(listener);
}

void NodeImpl::RemoveListener(INodeListener *listener)
{
	std::vector<INodeListener *>::iterator it = std::find(m_callbacks.begin(), m_callbacks.end(), listener);
	assert(m_callbacks.end() != it);
	*it = *m_callbacks.rbegin();
	m_callbacks.pop_back();
}

NodeLayers NodeImpl::GetLayers() const
{
	return m_layers;
}

void NodeImpl::SetLayers(NodeLayers layers)
{
	m_layers = layers;
}

const float* NodeImpl::GetLocal() const
{
	if( !m_isValidLocal )
	{
		SSEId( m_local );
//		MatrixIdentity(&m_local);
		for( size_t i = 0; i < m_transforms.size(); ++i )
		{
//			MatrixMultiply((float4x4 *)&m_local, (const float4x4 *)m_transforms[i]->GetMatrix(), (const float4x4 *)&m_local);
			SSEMat r;
			SSEMulMM( m_transforms[i]->GetMatrix(), (const float*)m_local, r );
			memcpy( (float*)m_local, r, SSEMatSize );
		}
		m_isValidLocal = true;
	}
	return m_local;
}

const float* NodeImpl::GetWorld(Scene *context) const
{
	if( !m_isValidWorld )
	{
		if( GetParent(context) )
		{
//			MatrixMultiply((float4x4 *)&m_world, (const float4x4 *)GetLocal(), 
//				(const float4x4 *)GetParent(context)->GetWorld(context));
			SSEMulMM(GetLocal(), GetParent(context)->GetWorld(context), (float*) m_world);
		}
		else
		{
//			m_world = *(const float4x4 *) GetLocal();
			memcpy( m_world, GetLocal(), SSEMatSize );
		}
		m_isValidWorld = true;
	}
	return m_world;
}

const float* NodeImpl::GetWorldI(Scene *context) const
{
	if( !m_isValidWorldI )
	{
		MatrixInverse(&m_worldI, NULL, ((const float4x4 *) GetWorld(context)));
		m_isValidWorldI = true;
	}
	return m_worldI;
}

Node* NodeImpl::GetParent(Scene *context) const
{
	return SafeResolveContext<Node>(context, m_parent);
}

size_t NodeImpl::GetChildrenCount() const
{
	return m_children.size();
}

Node* NodeImpl::GetChild(size_t index, Scene *context) const
{
	return SafeResolveContext<Node>(context, m_children[index]);
}

size_t NodeImpl::GetPropertyCount() const
{
	return m_transforms.size();
}

Property* NodeImpl::GetProperty(size_t idx)
{
	return m_transforms[idx]->GetDelegate(NULL);
}


// end of file
