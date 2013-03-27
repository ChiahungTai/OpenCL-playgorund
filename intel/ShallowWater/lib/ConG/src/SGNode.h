/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGNode.h
// Node interface implementation

#pragma once

#include "mathstub.h"

#include "SceneGraph.h"
#include "SGImplBase.h"

// forward declarations
class TransformBaseImpl;
class PropertyImpl;

class NodeImpl : public BaseImpl<Node>
{
public:
	NodeImpl(Scene *s, NodeImpl *parent);
	virtual ~NodeImpl();

	void InvalidateTransformCache();
	const float* GetLocal() const;


	//
	// animation support
	//

	void OnInvalidateParentTransform();


	//
	// Base interface
	//

	virtual size_t GetPropertyCount() const;
	virtual Property* GetProperty(size_t idx);


	//
	// Node interface implementation
	//

	virtual NodeLayers GetLayers() const;
	virtual void SetLayers(NodeLayers layers);

	virtual const float* GetWorld(Scene *context) const;
	virtual const float* GetWorldI(Scene *context) const;

	virtual Node* GetParent(Scene *context) const;

	virtual size_t GetChildrenCount() const;
	virtual Node* GetChild(size_t index, Scene *context) const;

	virtual Transform* AppendTransform(Transform::Type type, const float *data);

	virtual void AddListener(INodeListener *listener);
	virtual void RemoveListener(INodeListener *listener);


private:
	NodeImpl *m_parent;
	NodeLayers m_layers;
	std::vector<NodeImpl *> m_children;
	PtrOwnerArray<TransformBaseImpl> m_transforms;
	std::vector<INodeListener *> m_callbacks;

	// TODO: add context cache
	mutable SSEUnaligned m_localU;  // cached local transform relative to the parent
	mutable SSEUnaligned m_worldU;  // cached world matrix composed with the parent's local transforms
	mutable float* m_local; //aligned
	mutable float* m_world; //aligned
	mutable float4x4 m_worldI; // cached inverse world matrix
	mutable bool m_isValidLocal;
	mutable bool m_isValidWorld;
	mutable bool m_isValidWorldI;
};

///////////////////////////////////////////////////////////////////////////////

class TransformBaseImpl
	: public Transform
	, public PropertyListener
{
public:
	TransformBaseImpl(NodeImpl *node);
	virtual ~TransformBaseImpl() = 0;

	// Transform interface implementation
	virtual const float* GetMatrix() const;

	// PropertyListener interface implementation
	virtual void OnPropertyChange(); // called by delegate when some parameter is changed

	// AnimationTarget interface implementation
	virtual Property* GetDelegate(const char *param) = 0;
	virtual bool HasBoundDelegates() const { return !m_delegates.empty(); }

protected:
	PropertyImpl* RegisterProperty(PropertyImpl *d);
	virtual void FillMatrix(float4x4 *m) const = 0;

private:
	NodeImpl * const m_node;
	PtrOwnerArray<PropertyImpl> m_delegates;

	mutable SSEUnaligned m_cachedMatrixU;
	mutable float* m_cachedMatrix;
	mutable bool m_isCacheValid;
};

///////////////////////////////////////////////////////////////////////////////

class TransformTranslate : public TransformBaseImpl
{
public:
	TransformTranslate(NodeImpl *node, const float *data3);

	// AnimationTarget interface implementation
	virtual Property* GetDelegate(const char *param);

protected:
	virtual void FillMatrix(float4x4 *m) const;

private:
	float m_translateX;
	float m_translateY;
	float m_translateZ;
};

///////////////////////////////////////////////////////////////////////////////

class TransformScale : public TransformBaseImpl
{
public:
	TransformScale(NodeImpl *node, const float *data3);

	// AnimationTarget interface implementation
	virtual Property* GetDelegate(const char *param);

protected:
	virtual void FillMatrix(float4x4 *m) const;

private:
	float m_scaleX;
	float m_scaleY;
	float m_scaleZ;
};

///////////////////////////////////////////////////////////////////////////////

class TransformRotate : public TransformBaseImpl
{
public:
	TransformRotate(NodeImpl *node, const float *data4);

	// AnimationTarget interface implementation
	virtual Property* GetDelegate(const char *param);

protected:
	virtual void FillMatrix(float4x4 *m) const;

private:
	float4 m_axisAndAngle;  // angle in degrees
};

///////////////////////////////////////////////////////////////////////////////

class TransformMatrix : public TransformBaseImpl
{
public:
	TransformMatrix(NodeImpl *node, const float *data4x4);

	// AnimationTarget interface implementation
	virtual Property* GetDelegate(const char *param);

protected:
	virtual void FillMatrix(float4x4 *m) const;

private:
	float m_matrix[16];
};

// end of file
