/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SceneUtils.h

#pragma once

#include "SceneGraphBase.h"

namespace collada
{
	/// Creates a camera instance that observe whole scene.
	CameraInstance* CreateDefaultCamera(EditableScene *es, const char *camTag, const char *nodeTag, const char *instTag);

	/// Locate vertex data by semantic string.
	MemoryObject* FindVertexBuffer(Mesh *mesh, const char *semantic);
	MemoryObject* FindVertexBuffer(Batch *batch, Scene *context, const char *semantic);

	class GeometryInstanceIterator
	{
	public:
		GeometryInstanceIterator(Scene *s);

		bool IsValid() const;
		void StepNext();

		GeometryInstance* GetInst();
		Mesh* GetMesh();
		Node* GetNode();
		Batch* GetBatch();
		MaterialBinding* GetMaterialBinding();

	private:
		Scene *m_scene;
		size_t m_instIdx;
		size_t m_batchIdx;

		mutable Mesh *m_currentMesh;
	};

}

// end of file
