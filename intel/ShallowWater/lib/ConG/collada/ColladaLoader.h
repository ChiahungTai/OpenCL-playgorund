/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file ColladaLoader.h

#pragma once

#include "ColladaDataFlow.h"
#include "MsgListenerWrapper.h"
#include "SceneGraphBase.h"

#include <set>

using namespace collada;

///////////////////////////////////////////////////////////////////////////////

class ColladaLoader
{
public:
	// create and destroy
	ColladaLoader(IMessageListener *ls);
	~ColladaLoader();

	void Load(EditableScene *s, const TCHAR *fileName);   // throws an exception in case of failure

private:
	ColladaLoader(const ColladaLoader &);
	ColladaLoader& operator = (const ColladaLoader &);


	EditableScene *m_scene;

	struct MeshExtra
	{
		std::vector<std::string> materials;
	};

	std::list<MeshExtra> m_meshExtras;
	std::map<Mesh*, MeshExtra*> m_meshExtrasMap;

	struct SkinCtrlExtra
	{
		SourceCache sourceCache;
		SourceImpl *jointSource;
		SourceImpl *invBindMatrixSource;

		SkinCtrlExtra()
		  : sourceCache(NULL)
		  , jointSource(NULL)
		  , invBindMatrixSource(NULL)
		{
		}
	};

	PtrOwnerArray<SkinCtrlExtra> m_skinCtrlExtras;
	std::map<SkinController *, SkinCtrlExtra*> m_skinCtrlExtrasMap;

	typedef std::map<std::string, NodeLayers> LayersRegistry;
	LayersRegistry m_layersRegistry;
	std::set<Node*> m_orphanNodes; // nodes w/o layer


	struct EffectExtra
	{
		typedef std::map<std::string, std::string> InputAliasMap;
		InputAliasMap aliases; // external -> internal
	};
	std::map<const Effect *, EffectExtra> m_effectExtrasMap;


	std::set<Animation*> m_orphanAnimations;
	Material *m_pDefaultMaterial;
	MsgListenerWrapper m_msg;

	void ProcessLibraryVisualScenes(const domLibrary_visual_scenes_Array &vsLibs);
	void ProcessLibraryAnimations(const domLibrary_animations_Array &animationLibs);
	void ProcessLibraryAnimationClips(const domLibrary_animation_clips_Array &animationClipLibs);

protected:
	// modify user data
	Node* ProcessNodeP(domNode *src, Node *parent, bool instanced);
	Node* ProcessNode(domNode *src); // uses default parent
	Camera* ProcessCamera(domCamera *src);
	Light* ProcessLight(domLight *src);
	EditableMesh* ProcessMesh(domGeometry *src);
	Material* ProcessMaterial(domMaterial *src);
	Material* CreateDefaultMaterial(bool supportNormals);
	Effect* ProcessEffect(domEffect *src);
	SkinController* ProcessSkinController(domController *src);
	SkinControllerInstance* ProcessSkinnedMeshInst(Node *node, domInstance_controller *src);
	MorphController* ProcessMorphController(domController *src);
	MorphControllerInstance* ProcessMorphedMeshInst(Node *node, domInstance_controller *src);

	void FillEffectFromCommonProfile(Effect *e, const domProfile_COMMON *common);
	void FillEffectWithPlainColor(Effect *e, bool supportNormals);

	Animation* ProcessAnimation(domAnimation *src); // may return NULL
	AnimationClip* ProcessAnimationClip(domAnimation_clip *src); // may return NULL

	void ProcessRegularTexture(Effect *effect, domCommon_color_or_texture_type *tex, Channel channel);
	void ProcessReflectionTexture(Effect *effect, domCommon_color_or_texture_type *tex, domCommon_float_or_param_type *factor);
	void ProcessTransparencyTexture(Effect *effect, domCommon_transparent_type *tex, domCommon_float_or_param_type *factor);
	void ProcessFColladaBumpTexture(Effect *effect, daeElement *bumpTex);

	template <class InstanceType>
	void CreateMaterialBindingTable(Mesh *mesh, GeometryInstance *geomInstance,
		const InstanceType *srcInst, const std::vector<std::string> &materials);
};

// end of file
