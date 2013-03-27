/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGMaterialBinding.h

#pragma once

#include "SceneGraph.h"
#include "SGImplBase.h"

#include <map>
#include <string>

using namespace collada;

///////////////////////////////////////////////////////////////////////////////

class MaterialBindingImpl
	: public BaseImpl<MaterialBinding>
	, public ISceneListener
{
public:
	// Construction
	MaterialBindingImpl(Scene *s, GeometryInstance *parentInstance, Batch *batch, Material *mat);

	// ISceneListener
	virtual void OnSceneDestroy(Scene *which);

	// MaterialBinding interface
	virtual GeometryInstance* GetParentInstance(Scene *context) const;
	virtual Batch* GetBatch(Scene *context) const;
	virtual Material* GetMaterial(Scene *context) const;
	virtual void SetMaterial(Material*pMtl);

	virtual void SetBinding(const char *name, const Target *target);
	virtual size_t GetBindingCount() const;
	virtual const char* GetBindingName(size_t index) const;
	virtual bool GetBindingTarget(const char* name, Target *result) const;

	virtual size_t GetRequiredInputCount(Scene *context) const;
	virtual const VertexElementDesc* GetRequiredInputs(Scene *context) const;

private:
	Material *m_material;
	Batch *m_batch;
	GeometryInstance *m_parentInstance;
	typedef std::pair<std::string, int> MyTarget;
	typedef std::map<std::string, MyTarget> BindingMap;
	BindingMap m_binding; // material semantic -> vertex attribute

	typedef std::map<Scene *, std::vector<VertexElementDesc> > InputsCache;
	mutable InputsCache m_cachedInputs;
	mutable bool m_isInputsValid;

	void FlushInternalState() const;
};

// end of file
