/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file ColladaLoader.cpp

#include "ColladaUtilsMath.h"

#include "SceneGraph.h"
#include "ColladaLoader.h"
#include "ColladaLoaderExport.h"
#include "AutoPtr.h"
#include "MemoryManager.h"

#include <dae.h>
#include <dom/domCOLLADA.h>
#include <dom/domProfile_COMMON.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <set>


#define ToRadian(x)   ((x) / 180.0f * M_PI)


// grep "enum ScalarType"
static const size_t g_typeSize[ST_COUNT] = 
{
	4, // ST_FLOAT
	2, // ST_HALF
	4, // ST_SINT32
	4, // ST_UINT32
	2, // ST_SINT16
	2, // ST_UINT16
	1, // ST_SINT8
	1, // ST_UINT8
};


// grep "enum Channel"
static const char *g_semantics[CH_COUNT] = 
{
	"TEX_EMISSION",    // CH_EMISSION
	"TEX_AMBIENT",     // CH_AMBIENT
	"TEX_DIFFUSE",     // CH_DIFFUSE
	"TEX_SPECULAR",    // CH_SPECULAR
	"TEX_TRANSPARENT", // CH_TRANSPARENT
	"TEX_BUMP",        // CH_BUMP
	"NORMAL",          // CH_REFLECTION  - does not require texture coordinates
};


///////////////////////////////////////////////////////////////////////////////

template <class InstanceType>
static void ColladaLoader::CreateMaterialBindingTable(Mesh *mesh, GeometryInstance *geomInstance, const InstanceType *srcInst, 
                                                      const std::vector<std::string> &materials)
{
	assert(mesh);
	assert(srcInst);
	assert(geomInstance);

	typedef std::pair<domInstance_material*, Material *> MatPair;
	typedef std::map<std::string, MatPair> StringToMatMap;
	StringToMatMap nameToMaterial;


	//
	// setup the symbol name to material mapping
	//
	
	if( domBind_material *bindMaterial = srcInst->getBind_material() )
	{
		domBind_material::domTechnique_common *tech = bindMaterial->getTechnique_common();
		domInstance_material_Array &materials = tech->getInstance_material_array();
		for( size_t i = 0; i < materials.getCount(); ++i )
		{
			domInstance_material *srcmat = materials[i];
			Material *mat = ProcessMaterial(SafeResolve<domMaterial>(srcmat->getTarget()));
			nameToMaterial[srcmat->getSymbol()] = MatPair(srcmat, mat);
		}
	}
	else
	{
		m_msg(MSG_SEVERITY_WARNING) << "instance that refers to "
			<< srcInst->getUrl().getOriginalURI()
			<< " does not have any material bindings";
	}


	//
	// associate mapped materials with matId of batches
	//
	assert(mesh->GetBatchCount() == materials.size());
	for( size_t i = 0; i < materials.size(); ++i )
	{
		StringToMatMap::const_iterator it = nameToMaterial.find(materials[i]);
		if( nameToMaterial.end() == it )
		{
			if( !materials[i].empty() )
			{
				m_msg(MSG_SEVERITY_WARNING) << "instance that refers to "
					<< srcInst->getUrl().getOriginalURI()
					<< " has no binding for symbol '" << materials[i] << "'";
			}
			if( !m_pDefaultMaterial )
			{
				m_msg(MSG_SEVERITY_INFO) << "created default material";
				m_pDefaultMaterial = CreateDefaultMaterial(true); // TODO: analyze normals support
			}
			it = nameToMaterial.insert(StringToMatMap::value_type(materials[i], MatPair(NULL, m_pDefaultMaterial))).first;
		}

		const Effect *effect = it->second.second->GetEffect(m_scene);
		const EffectExtra::InputAliasMap &aliases = m_effectExtrasMap[effect].aliases;

		MaterialBinding *mb = geomInstance->CreateMaterialBinding(mesh->GetBatch(i, m_scene), i, it->second.second);
		mb->SetTag((std::string(it->second.second->GetTag()) +"@"+mesh->GetTag()).c_str());
		if( it->second.first )
		{
			const domInstance_material::domBind_vertex_input_Array &vtxBindings = it->second.first->getBind_vertex_input_array();
			for( size_t j = 0; j < vtxBindings.getCount(); ++j )
			{
				MaterialBinding::Target t;
				t.semantic = vtxBindings[j]->getInput_semantic();
				t.index = (unsigned int) vtxBindings[j]->getInput_set();

				EffectExtra::InputAliasMap::const_iterator aliasIt = aliases.find(vtxBindings[j]->getSemantic());
				if( aliases.end() != aliasIt )
				{
					mb->SetBinding(aliasIt->second.c_str(), &t);
				}
				else
				{
					mb->SetBinding(vtxBindings[j]->getSemantic(), &t);
					m_msg(MSG_SEVERITY_DEBUG)
						<< "alias for input semantic '" << vtxBindings[j]->getSemantic()
						<< "' not found in effect '" << effect->GetTag() << "'";
				}
			}
		}
	}
} // CreateMaterialBindingTable

static int ToMS(const LARGE_INTEGER &perfCounter0, const LARGE_INTEGER &perfCounter1)
{
	LARGE_INTEGER perfFreq;
	QueryPerformanceFrequency(&perfFreq);
	return int((double) (perfCounter1.QuadPart - perfCounter0.QuadPart) / (double) perfFreq.QuadPart * 1000 + 0.5);
}

///////////////////////////////////////////////////////////////////////////////
// ColladaMeshFile implementation

ColladaLoader::ColladaLoader(IMessageListener *ls)
  : m_scene(NULL)
  , m_msg(ls, "COLLADA loader")
  , m_pDefaultMaterial(NULL)
{
}

ColladaLoader::~ColladaLoader()
{
}

void ColladaLoader::Load(EditableScene *s, const TCHAR *fileName)   // throws an exception in case of failure
{
	assert(!m_scene);
	m_scene = s;

#ifdef UNICODE
	size_t count = 0;
	wcstombs_s(&count, NULL, 0, fileName, 0);
	std::string buf(count, '\0');
	wcstombs_s(&count, &buf[0], buf.size(), fileName, buf.size());
#else
	const char *buf = fileName;
#endif

	LARGE_INTEGER perfTotal0, perfTotal1;
	QueryPerformanceCounter(&perfTotal0);


	LARGE_INTEGER perfCounter0, perfCounter1;


	//
	// load .dae into the DOM tree
	//

	QueryPerformanceCounter(&perfCounter0);

	DAE dae;
	domCOLLADA *root = dae.open(buf);
	if( !root )
	{
		throw std::runtime_error("Could not read xml file");
	}

	QueryPerformanceCounter(&perfCounter1);

	m_msg(MSG_SEVERITY_INFO) << "xml file parsed in " << ToMS(perfCounter0, perfCounter1) << " ms";



	//
	// create scene graph structures
	//

	QueryPerformanceCounter(&perfCounter0);

	// get up-axis
	if( root->getAsset() && root->getAsset()->getUp_axis() )
	{
		domUpAxisType up = root->getAsset()->getUp_axis()->getValue();
		switch( up )
		{
			case UPAXISTYPE_X_UP: m_scene->SetWorldUpAxis(UPAXIS_X); break;
			case UPAXISTYPE_Y_UP: m_scene->SetWorldUpAxis(UPAXIS_Y); break;
			case UPAXISTYPE_Z_UP: m_scene->SetWorldUpAxis(UPAXIS_Z); break;
			default:
				throw std::runtime_error("Invalid up-axis");
		}
	}

	ProcessLibraryVisualScenes(root->getLibrary_visual_scenes_array());

	// put all orphan nodes into the default layer
	if( !m_orphanNodes.empty() )
	{
		NodeLayers defaultLayer = m_scene->RegisterLayer(NULL);
		for( std::set<Node*>::const_iterator it = m_orphanNodes.begin(); m_orphanNodes.end() != it; ++it )
		{
			assert(!(*it)->GetLayers());
			(*it)->SetLayers(defaultLayer);
		}
	}

	QueryPerformanceCounter(&perfCounter1);

	m_msg(MSG_SEVERITY_INFO) << "scene graph created in " << ToMS(perfCounter0, perfCounter1) << " ms";


	//
	// parse animations
	//

	QueryPerformanceCounter(&perfCounter0);

	ProcessLibraryAnimations(root->getLibrary_animations_array());
	ProcessLibraryAnimationClips(root->getLibrary_animation_clips_array());

	// put all orphan animations into the default animation clip
	if( !m_orphanAnimations.empty() )
	{
		AnimationClip *clip = m_scene->CreateAnimationClip();
		clip->SetTag("<default>");
		float timeStart = 0;
		float timeEnd = 0;
		for( std::set<Animation*>::const_iterator it = m_orphanAnimations.begin(); it != m_orphanAnimations.end(); ++it )
		{
			if( m_orphanAnimations.begin() == it )
			{
				if( timeStart > (*it)->GetTimeStart() )
					timeStart = (*it)->GetTimeStart();
				if( timeEnd < (*it)->GetTimeEnd() )
					timeEnd = (*it)->GetTimeEnd();
			}
			else
			{
				timeStart = (*it)->GetTimeStart();
				timeEnd = (*it)->GetTimeEnd();
			}
			clip->AddAnimation(*it);
		}
		clip->SetTimeBounds(timeStart, timeEnd);
		m_orphanAnimations.clear();
	}

	QueryPerformanceCounter(&perfCounter1);

	m_msg(MSG_SEVERITY_INFO) << "animation parsed in " << ToMS(perfCounter0, perfCounter1) << " ms";

	QueryPerformanceCounter(&perfTotal1);
	m_msg(MSG_SEVERITY_INFO) << "DONE. Total loading time is " << ToMS(perfTotal0, perfTotal1) << " ms";
}

Node* ColladaLoader::ProcessNodeP(domNode *src, Node *parent, bool instanced)
{
	assert(src);
	assert(parent || !instanced); // instanced node must have parent

	Node *node = (Node *) src->getUserData();

	if( node && !instanced )
	{
		return node;
	}

	node = m_scene->CreateNode(parent);

	if( instanced )
	{
		node->SetTag((std::string(src->getId() ? src->getId() : "") + "@" + parent->GetTag()).c_str());
	}
	else
	{
		src->setUserData(node);
		node->SetTag(src->getId() ? src->getId() : "");
	}

//	m_idToNode.insert(IdToNodeMMap::value_type(node->GetTag(), node));


	//
	// extract transformation data
	//

	for( size_t i = 0; i < src->getContents().getCount(); ++i )
	{
		daeElement *elem = src->getContents()[i];
		Transform *t = NULL;

		switch( elem->getElementType() )
		{
			case COLLADA_TYPE::LOOKAT:
				// TODO: implement
				break;
			case COLLADA_TYPE::SKEW:
				// TODO: implement
				break;
			case COLLADA_TYPE::MATRIX:
				t = node->AppendTransform( Transform::MATRIX,
					ArrayConvert<4*4>(static_cast<const domMatrix *>(elem)->getValue()));
				break;
			case COLLADA_TYPE::ROTATE:
				t = node->AppendTransform( Transform::ROTATE,
					ArrayConvert<4>(static_cast<const domRotate *>(elem)->getValue()));
				break;
			case COLLADA_TYPE::SCALE:
				t = node->AppendTransform( Transform::SCALE,
					ArrayConvert<3>(static_cast<const domScale *>(elem)->getValue()));
				break;
			case COLLADA_TYPE::TRANSLATE:
				t = node->AppendTransform( Transform::TRANSLATE,
					ArrayConvert<3>(static_cast<const domTranslate *>(elem)->getValue()));
				break;
			default:
				continue;
		} // end of switch( type )

		if( t && !instanced )
		{
			elem->setUserData(t);
		}
	}

	if( MatrixDeterminant((const float4x4 *) node->GetWorld(m_scene)) < 0 )
	{
		m_msg(MSG_SEVERITY_WARNING) << "Node " << node->GetTag() << " has negative scale";
	}


	//
	// extract layers
	//

	NodeLayers layers = 0;
	for( size_t i = 0; i < src->getLayer().getCount(); ++i )
	{
		LayersRegistry::iterator it = m_layersRegistry.find(src->getLayer()[i]);
		if( m_layersRegistry.end() == it )
		{
			NodeLayers newLayer = m_scene->RegisterLayer(src->getLayer()[i]);
			it = m_layersRegistry.insert(LayersRegistry::value_type(src->getLayer()[i], newLayer)).first;
		}
		layers |= it->second;
	}
	if( layers )
	{
		node->SetLayers(layers);
	}
	else
	{
		m_orphanNodes.insert(node);
	}


	//
	// process direct children
	//

	const domNode_Array &children = src->getNode_array();
	for( size_t i = 0; i < children.getCount(); ++i )
	{
		ProcessNodeP(children[i], node, instanced); // inherit instanced flag
	}


	//
	// process instanced children
	//

	const domInstance_node_Array &childrenInst = src->getInstance_node_array();
	for( size_t i = 0; i < childrenInst.getCount(); ++i )
	{
		ProcessNodeP(SafeResolve<domNode>(childrenInst[i]->getUrl()), node, true);
	}


	//
	// extract cameras
	//

	for( size_t i = 0; i < src->getInstance_camera_array().getCount(); ++i )
	{
		const domInstance_camera *cameraInst = src->getInstance_camera_array()[i];
		Camera *cam = ProcessCamera(SafeResolve<domCamera>(cameraInst->getUrl()));
		CameraInstance *inst = m_scene->CreateCameraInstance(node, cam);
		inst->SetTag((std::string(cam->GetTag()) + "@" + node->GetTag()).c_str());
	}


	//
	// extract lights
	//

	for( size_t i = 0; i < src->getInstance_light_array().getCount(); ++i )
	{
		const domInstance_light *lightInst = src->getInstance_light_array()[i];
		Light *light = ProcessLight(SafeResolve<domLight>(lightInst->getUrl()));
		LightInstance *inst = m_scene->CreateLightInstance(node, light);
		inst->SetTag((std::string(light->GetTag()) + "@" + node->GetTag()).c_str());
	}


	//
	// extract static geometry data
	//

	for( size_t i = 0; i < src->getInstance_geometry_array().getCount(); ++i )
	{
		domInstance_geometry *geomInst = src->getInstance_geometry_array()[i];
		domGeometry *geomEntity = SafeResolve<domGeometry>(geomInst->getUrl());
		if( geomEntity->getMesh() )
		{
			if( Mesh *mesh = ProcessMesh(geomEntity) )
			{
				assert(m_meshExtrasMap.count(mesh));
				MeshInstance *inst = m_scene->CreateRigidMeshInstance(node, mesh);
				inst->SetTag((std::string(mesh->GetTag()) + "@" + node->GetTag()).c_str());
				CreateMaterialBindingTable(mesh, inst, geomInst, m_meshExtrasMap[mesh]->materials);
			}
		}
	}


	//
	// extract controllers
	//

	for( size_t i = 0; i < src->getInstance_controller_array().getCount(); ++i )
	{
		domInstance_controller *ctrlInst = src->getInstance_controller_array()[i];
		domController *ctrlEntity = SafeResolve<domController>(ctrlInst->getUrl());
		if( ctrlEntity->getSkin() )
		{
			ProcessSkinnedMeshInst(node, src->getInstance_controller_array()[i]);
		}
		else if( ctrlEntity->getMorph() )
		{
			ProcessMorphedMeshInst(node, src->getInstance_controller_array()[i]);
		}
	}

	return node;
}

MorphControllerInstance* ColladaLoader::ProcessMorphedMeshInst(Node *node, domInstance_controller *src)
{
	MorphController *morphCtrl = ProcessMorphController(SafeResolve<domController>(src->getUrl()));
	assert(m_meshExtrasMap.count(morphCtrl->GetBaseMesh(NULL)));

	MorphControllerInstance *inst = m_scene->CreateMorphCtrlInstance(node, morphCtrl);
	inst->SetTag((std::string(morphCtrl->GetTag()) + "(" + morphCtrl->GetBaseMesh(NULL)->GetTag() + ")@" + node->GetTag()).c_str());
	CreateMaterialBindingTable(morphCtrl->GetBaseMesh(NULL), inst, src, m_meshExtrasMap[morphCtrl->GetBaseMesh(NULL)]->materials);

	return inst;
}

SkinControllerInstance* ColladaLoader::ProcessSkinnedMeshInst(Node *node, domInstance_controller *src)
{
	SkinController *skinCtrl = ProcessSkinController(SafeResolve<domController>(src->getUrl()));

	assert(m_skinCtrlExtrasMap.count(skinCtrl));
	SourceImpl *jointSource = m_skinCtrlExtrasMap[skinCtrl]->jointSource;
	SourceImpl *invBindMatrixSource = m_skinCtrlExtrasMap[skinCtrl]->invBindMatrixSource;

	SkinControllerInstance *inst = m_scene->CreateSkinCtrlInstance(node, skinCtrl);
	inst->SetTag((std::string(skinCtrl->GetTag()) + "(" + skinCtrl->GetBaseMesh(NULL)->GetTag() + ")@" + node->GetTag()).c_str());
	assert(m_meshExtrasMap.count(skinCtrl->GetBaseMesh(NULL)));
	CreateMaterialBindingTable(skinCtrl->GetBaseMesh(NULL), inst, src, m_meshExtrasMap[skinCtrl->GetBaseMesh(NULL)]->materials);

	//
	// fill joints array
	//

	assert(jointSource->GetElementSize() == 1);
	assert(invBindMatrixSource->GetElementSize() == 16); // 4x4 matrix

	std::vector<Joint> jointsDst(jointSource->GetElementCount());
	for( size_t jointIdx = 0; jointIdx < jointSource->GetElementCount(); ++jointIdx )
	{
		Node *jointNode = NULL;

		if( SourceImpl::VT_NAME == jointSource->GetType() )
		{
			const char *jointSid = jointSource->GetName(jointIdx);
			// find this sid in all skeleton roots
			for( size_t skeletonIdx = 0; skeletonIdx < src->getSkeleton_array().getCount(); ++skeletonIdx )
			{
				daeElement *root = src->getSkeleton_array()[skeletonIdx]->getValue().getElement();
				if( !root )
				{
					throw std::runtime_error(std::string("skeleton root with id=\"") + 
						src->getSkeleton_array()[skeletonIdx]->getValue().getOriginalURI() + "\" not found");
				}

				// Find the joint inside the skeleton root. The "./" part is necessary.
				daeSIDResolver resolver(root, (std::string("./") + jointSid).c_str());
				if( resolver.getElement() )
				{
					jointNode = ProcessNode(SafeResolve<domNode>(resolver));
					break;
				}
			}

			if( !jointNode )
			{
				throw std::runtime_error(std::string("skeleton node with sid=\"") + jointSid + "\" not found");
			}
		}
		else
		{
			jointNode = ProcessNode(jointSource->GetRefT<domNode>(jointIdx));
		}
		assert(jointNode);

		// get the inverse bind matrix and register new joint
		const float *m = invBindMatrixSource->GetFloatArray() + jointIdx * 16;
		jointsDst[jointIdx].node = jointNode;

		//unroll SSEAlign function here
//		unsigned int mask = -16;
//		jointsDst[jointIdx].invBindMatrix = (float*)( ( ( (unsigned int)jointsDst[jointIdx].invBindMatrixU )+15 ) & mask );
		memcpy(jointsDst[jointIdx].invBindMatrix, Matrix4FromDomFloatArray(m), sizeof(float) * 16);
	}
	AutoPtr<MemoryObject> jointsBuffer;
	m_scene->GetMemoryManager()->Allocate(jointsDst.size() * sizeof(Joint), &jointsDst[0], &jointsBuffer);
	inst->SetJoints(jointsBuffer);
	return inst;
}

Node* ColladaLoader::ProcessNode(domNode *src)
{
	assert(src);
	domNode *parent = daeSafeCast<domNode>(src->getParent());
	return ProcessNodeP(src, parent ? ProcessNode(parent) : NULL, false);
}

Camera* ColladaLoader::ProcessCamera(domCamera *src)
{
	assert(src);
	Camera *cam = (Camera *) src->getUserData();
	if( !cam )
	{
		cam = m_scene->CreateCamera();

		typedef domCamera::domOptics::domTechnique_common::domPerspective PerspectiveType;
		typedef domCamera::domOptics::domTechnique_common::domOrthographic OrthographicType;

		if( PerspectiveType *p = src->getOptics()->getTechnique_common()->getPerspective() )
		{
			cam->SetType(Camera::PERSPECTIVE);

			if( p->getZnear() )
				cam->SetNear((float) p->getZnear()->getValue());
			if( p->getZfar() )
				cam->SetFar((float) p->getZfar()->getValue());
			if( p->getYfov() )
				cam->SetYFov((float) ToRadian((float) p->getYfov()->getValue()));
			if( p->getXfov() )
				cam->SetXFov((float) ToRadian((float) p->getXfov()->getValue()));
//			if( p->getAspect_ratio() )
//				cam->SetAspect((float) p->getAspect_ratio()->getValue());

			if( p->getYfov() && p->getAspect_ratio() )
				cam->SetFixedValues(Camera::FIXED_YFOV_ASPECT);
			else if( p->getXfov() && p->getAspect_ratio() )
				cam->SetFixedValues(Camera::FIXED_XFOV_ASPECT);
			else if( p->getXfov() && p->getYfov() )
				cam->SetFixedValues(Camera::FIXED_XFOV_YFOV);
			else
			{
				m_msg(MSG_SEVERITY_WARNING) << "Camera '" << src->getId() << "' has undefined attributes";
			}
		}
		else
		if( OrthographicType *o = src->getOptics()->getTechnique_common()->getOrthographic() )
		{
			cam->SetType(Camera::ORTHOGRAPHIC);

			o->getZnear();
			o->getZfar();
			o->getXmag();
			o->getYmag();
		}
		else
		{
			throw std::runtime_error("invalid camera type");
		}

		cam->SetTag(src->getId() ? src->getId() : "");
		src->setUserData(cam);
	}
	return cam;
}

Light* ColladaLoader::ProcessLight(domLight *src)
{
	assert(src);
	Light *light = (Light *) src->getUserData();
	if( !light )
	{
		light = m_scene->CreateLight();

		float intencity = 1;
#ifndef COLLADA_STRICT
		if( daeElement *elt = const_cast<domLight *>(src)->getDescendant("intensity") )
		{
			intencity = (float) atof(elt->getCharData().c_str());
		}
#endif

		if( domLight::domTechnique_common::domAmbient *ambient = src->getTechnique_common()->getAmbient() )
		{
			light->SetType(Light::AMBIENT);
			light->SetColor(Vector3FromDomVector(ambient->getColor()->getValue()) * intencity, 3);
		}
		else
		if( domLight::domTechnique_common::domPoint *point = src->getTechnique_common()->getPoint() )
		{
			light->SetType(Light::POINT);
			light->SetColor(Vector3FromDomVector(point->getColor()->getValue()) * intencity, 3);
			light->SetAttenuation0(point->getConstant_attenuation() ? (float) point->getConstant_attenuation()->getValue() : 1);
			light->SetAttenuation1(point->getLinear_attenuation() ? (float) point->getLinear_attenuation()->getValue() : 0);
			light->SetAttenuation2(point->getQuadratic_attenuation() ? (float) point->getQuadratic_attenuation()->getValue() : 0);
		}
		else
		if( domLight::domTechnique_common::domDirectional *dir = src->getTechnique_common()->getDirectional() )
		{
			light->SetType(Light::DIRECT);
			light->SetColor(Vector3FromDomVector(dir->getColor()->getValue()) * intencity, 3);
		}
		else
		if( domLight::domTechnique_common::domSpot *spot = src->getTechnique_common()->getSpot() )
		{
			light->SetType(Light::SPOT);
			light->SetColor(Vector3FromDomVector(spot->getColor()->getValue()) * intencity, 3);
			light->SetAttenuation0(spot->getConstant_attenuation() ? (float) spot->getConstant_attenuation()->getValue() : 1);
			light->SetAttenuation1(spot->getLinear_attenuation() ? (float) spot->getLinear_attenuation()->getValue() : 0);
			light->SetAttenuation2(spot->getQuadratic_attenuation() ? (float) spot->getQuadratic_attenuation()->getValue() : 0);
			light->SetFalloffAngle(spot->getFalloff_angle() ? (float) (spot->getFalloff_angle()->getValue() / 180.0 * M_PI) : (float) M_PI);
			light->SetFalloffExponent(spot->getFalloff_exponent() ? (float) spot->getFalloff_exponent()->getValue() : 0);
		}
		else
		{
			throw std::runtime_error("invalid or unknown light type");
		}

		light->SetTag(src->getId() ? src->getId() : "");
		src->setUserData(light);
	}
	return light;
}


namespace
{
	struct IdxCompare : public std::binary_function<size_t, size_t, bool>
	{
		const domListOfUInts &p; // this disables default constructor and cmp operators
		size_t stride;
		IdxCompare(const domListOfUInts &_p, size_t _stride)
			: p(_p)
			, stride(_stride)
		{}

		bool operator () (size_t left, size_t right) const
		{
			for( size_t i = 0; i < stride; ++i )
			{
				domUint leftIdx = p[stride*left + i];
				domUint rightIdx = p[stride*right + i];
				if( leftIdx < rightIdx )
					return true;   // lt
				if( leftIdx > rightIdx )
					return false;  // gt
			}
			return false;          // eq
		}
	};
}

struct SharedInput
{
	const SourceImpl *source;
	xsNMTOKEN semantic;
};

struct LocalInput : public SharedInput
{
	size_t offset;
	size_t set;
};

struct Primitive
{
	std::vector<LocalInput> localInputs;
	//    <p> xxx xxx xxx xxx   xxx xxx xxx xxx   xxx xxx xxx xxx  ... </p>
	// offset  0   1   2   3     0   1   2   3     0   1   2   3   ...
	// oldIdx        0                 1                 2         ...
	//
	const domListOfUInts *p; // contents of collada <p> element
	unsigned int indexStride;      // how much indices from <p> belong to one vertex
	domUint baseOffset;       // offset of VERTEX input
	domUint baseSet;
	size_t pcount;     // triangle or polygon count
	domListOfUInts *vcount; // vertex count per polygon

	const daeElement *sourceElem;
	const char *material;
};


template <class DomPrimitive>
static size_t ProcessPrimitive(Primitive &pending, const DomPrimitive *sourceElem, SourceCache &sourceCache)
{
	domUint maxOffset = 0;
	pending.sourceElem = sourceElem;
	pending.material = sourceElem->getMaterial();
	pending.baseOffset = -1;
	pending.baseSet = -1;
	pending.vcount = NULL;
	pending.pcount = (size_t) sourceElem->getCount();
	for( size_t inputIdx = 0; inputIdx < sourceElem->getInput_array().getCount(); ++inputIdx )
	{
		const domInputLocalOffset *input = sourceElem->getInput_array()[inputIdx];
		if( maxOffset < input->getOffset() )
			maxOffset = input->getOffset();
		daeElement *elt = SafeResolve<daeElement>(input->getSource());
		if( domSource *source = daeSafeCast<domSource>(elt) )
		{
			LocalInput li;
			li.source   = sourceCache.LocateKey(source);
			li.semantic = input->getSemantic();
			li.set      = (size_t) input->getSet();
			li.offset   = (size_t) input->getOffset();
			pending.localInputs.push_back(li);
		}
		else
		{
			assert(daeSafeCast<domVertices>(elt));
			assert(-1 == pending.baseOffset); // only one VERTICES input is allowed
			pending.baseOffset = input->getOffset();
			pending.baseSet = input->getSet();
		}
	}
	assert(-1 != pending.baseOffset); // VERTICES input is required
	pending.indexStride = static_cast<unsigned int>(maxOffset + 1);
	pending.p = &sourceElem->getP()->getValue();

	assert(0 == pending.p->getCount() % pending.indexStride);
	return pending.p->getCount() / pending.indexStride;
}

EditableMesh* ColladaLoader::ProcessMesh(domGeometry *src)
{
	assert(src);
	EditableMesh *mesh = (EditableMesh *) src->getUserData();
	if( !mesh )
	{
		SourceCache sourceCache(NULL);


		//
		// process primary attribute set
		//

		domVertices *vert = src->getMesh()->getVertices();
		assert(vert);

		std::vector<VertexElementDesc> primaryLayout(vert->getInput_array().getCount());

		for( size_t baseInputIdx = 0; baseInputIdx < vert->getInput_array().getCount(); ++baseInputIdx )
		{
			domInputLocal *input = vert->getInput_array()[baseInputIdx];
			domSource *source = SafeResolve<domSource>(input->getSource());

			SourceImpl *si = sourceCache.LocateKey(source);
			assert(si->GetType() == SourceImpl::VT_FLOAT);

			VertexElementDesc &ed = primaryLayout[baseInputIdx];
			ed.componentType = ST_FLOAT;              // TODO: support other types
			ed.componentCount = si->GetElementSize();
			ed.inputSlot = sourceCache.FindPointer(si);//  (unsigned int) it->second.bufIdx;
			ed.semanticName = input->getSemantic();
		}


		//
		// process primitive-specific inputs
		//

		std::vector<Primitive> pendingPrimitives;

		// line lists
		const domLines_Array &lines = src->getMesh()->getLines_array();
		for( size_t i = 0; i < lines.getCount(); ++i )
		{
			pendingPrimitives.push_back(Primitive());
			ProcessPrimitive(
				pendingPrimitives.back(),            // out
				lines[i].cast(),                     // in
				sourceCache
			);
		}

		// triangle lists
		const domTriangles_Array &triangles = src->getMesh()->getTriangles_array();
		for( size_t i = 0; i < triangles.getCount(); ++i )
		{
			pendingPrimitives.push_back(Primitive());
			ProcessPrimitive(
				pendingPrimitives.back(),            // out
				triangles[i].cast(),                 // in
				sourceCache
			);
		}

		// polylists
		const domPolylist_Array &polylists = src->getMesh()->getPolylist_array();
		for( size_t i = 0; i < polylists.getCount(); ++i )
		{
			pendingPrimitives.push_back(Primitive());
			ProcessPrimitive(
				pendingPrimitives.back(),            // out
				polylists[i].cast(),                 // in
				sourceCache
			);
			pendingPrimitives.back().vcount = &polylists[i]->getVcount()->getValue();
			if( pendingPrimitives.back().vcount->getCount() < pendingPrimitives.back().pcount )
			{
				throw std::runtime_error("wrong primitive count in polygonal mesh");
			}
		}

		// TODO: support next types
		if( src->getMesh()->getLinestrips_array().getCount() )
			m_msg(MSG_SEVERITY_WARNING) << "Mesh '" << src->getId() << "' contains " 
				<< src->getMesh()->getLinestrips_array().getCount() << " unsupported <linestrips> elements(s).";
		if( src->getMesh()->getPolygons_array().getCount() )
			m_msg(MSG_SEVERITY_WARNING) << "Mesh '" << src->getId() << "' contains " 
				<< src->getMesh()->getPolygons_array().getCount() << " unsupported <polygons> elements(s).";
		if( src->getMesh()->getTrifans_array().getCount() )
			m_msg(MSG_SEVERITY_WARNING) << "Mesh '" << src->getId() << "' contains " 
				<< src->getMesh()->getTrifans_array().getCount() << " unsupported <trifans> elements(s).";
		if( src->getMesh()->getTristrips_array().getCount() )
			m_msg(MSG_SEVERITY_WARNING) << "Mesh '" << src->getId() << "' contains " 
				<< src->getMesh()->getTristrips_array().getCount() << " unsupported <tristrips> elements(s).";


//		if( pendingPrimitives.empty() )
//		{
//			m_msg(MSG_SEVERITY_WARNING) << "Mesh '" << src->getId() << "' ignored - no supported geometry.";
//			return NULL;
//		}


		//
		// create mesh
		//

		std::vector<VertexBufferDesc> bufferDescs(sourceCache.GetSize());  // size == inputs.size()
		for( size_t i = 0; i < sourceCache.GetSize(); ++i )
		{
			assert(sourceCache.GetPointer(i)->GetType() == SourceImpl::VT_FLOAT);
			bufferDescs[i].bytesStride = sourceCache.GetPointer(i)->GetElementSize() * sizeof(float);
		}

		mesh = m_scene->CreateEditableMesh(&bufferDescs[0], bufferDescs.size());
		mesh->SetTag(src->getId() ? src->getId() : "");

		// register new extra data item
		m_meshExtras.push_back(MeshExtra());
		m_meshExtrasMap[mesh] = &m_meshExtras.back();
		MeshExtra &extra = m_meshExtras.back();


		// init primary vertex layout
		for( size_t i = 0; i < primaryLayout.size(); ++i )
		{
			mesh->AddPrimaryVertexElementDesc(&primaryLayout[i]);
		}


		bool doubleSided = false;

#ifndef COLLADA_STRICT
		// get extra double sided flag
		if( daeElement *elt = const_cast<domGeometry *>(src)->getDescendant("double_sided") )
		{
			doubleSided = (0 != atoi(elt->getCharData().c_str()));
		}
#endif

		// fill layouts and index buffers
		for( size_t i = 0; i < pendingPrimitives.size(); ++i )
		{
			const Primitive &pending = pendingPrimitives[i];
			if( 0 == pending.pcount )
			{
				m_msg(MSG_SEVERITY_INFO) << "mesh '" << src->getId() << "' has batch with zero primitive count";
			}

			if( !pending.material )
			{
				// TODO: select default material instead and just printing warning
				m_msg(MSG_SEVERITY_WARNING) << "mesh '" << src->getId() << "' has one or more batches with no material specified";
				extra.materials.push_back(std::string());
			}
			else
			{
				extra.materials.push_back(pending.material);
			}

			EditableBatch *batch = mesh->CreateBatch();
			batch->SetTag(pending.material);
			batch->SetDoubleSided(doubleSided);
			batch->SetPrimarySet(static_cast<unsigned int>(pending.baseSet));
			batch->SetPrimaryOffset(static_cast<unsigned int>(pending.baseOffset));

			AutoPtr<MemoryObject> countData;
			switch( pending.sourceElem->getElementType() )
			{
				case COLLADA_TYPE::LINES:
				{
					assert(0 == (pending.p->getCount() / pending.indexStride) % 2);
					batch->SetPrimitiveTopology(PRIMITIVE_TOPOLOGY_LINELIST);
					break;
				}
				case COLLADA_TYPE::TRIANGLES:
				{
					assert(0 == (pending.p->getCount() / pending.indexStride) % 3);
					batch->SetPrimitiveTopology(PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					break;
				}
				case COLLADA_TYPE::POLYLIST:
				{
					assert(pending.vcount);
					batch->SetPrimitiveTopology(PRIMITIVE_TOPOLOGY_POLYLIST);
					if( pending.vcount->getCount() )
					{
						std::vector<unsigned long> dst(pending.vcount->getCount());
						for( size_t polyIdx = 0; polyIdx < dst.size(); ++polyIdx )
						{
							dst[polyIdx] = (unsigned long) pending.vcount->get(polyIdx);
						}
						m_scene->GetMemoryManager()->Allocate(pending.pcount * sizeof(unsigned long), &dst[0], &countData);
					}
					break;
				}
				default:
					assert(!"unsupported topology");
			}
			batch->SetCountData(countData);

			// process local inputs
			for( std::vector<LocalInput>::const_iterator it = pending.localInputs.begin(); it != pending.localInputs.end(); ++it )
			{
				VertexElementDescEx ed;
				ed.componentType = ST_FLOAT;              // TODO: support other types
				ed.componentCount = (unsigned int) it->source->GetElementSize();
				ed.inputSlot = (unsigned int) sourceCache.FindPointer(it->source);// sd.bufIdx;
				ed.indexOffset = (unsigned int) it->offset;
				ed.semanticIndex = (unsigned int) it->set;
				ed.semanticName = it->semantic;
				batch->AddExtraVertexElement(&ed);
			}


			//
			// fill index buffer
			//

			if( pending.p->getCount() )
			{
				std::vector<unsigned long> dst(pending.p->getCount());
				for( size_t i = 0; i < dst.size(); ++i )
				{
					dst[i] = (unsigned long) (pending.p->get(i));
				}
				AutoPtr<MemoryObject> indexData;
				m_scene->GetMemoryManager()->Allocate(pending.p->getCount() * sizeof(unsigned long), &dst[0], &indexData);
				batch->SetIndexData(indexData);
			}
		} // end for( pendingPrimitives )


		//
		// submit data to mesh's vertex buffers
		//
		for( size_t i = 0; i < sourceCache.GetSize(); ++i )
		{
			SourceImpl *si = sourceCache.GetPointer(i);
			assert(si->GetType() == SourceImpl::VT_FLOAT);
			AutoPtr<MemoryObject> buf;
			m_scene->GetMemoryManager()->Allocate(si->GetElementCount() * si->GetElementSize() * sizeof(float), si->GetFloatArray(), &buf);
			mesh->SetVertexBuffer(i, buf);
		}

		src->setUserData(mesh);
	} // end if( !mesh )
	return mesh;
}

Material* ColladaLoader::ProcessMaterial(domMaterial *src)
{
	assert(src);
	Material *mat = (Material *) src->getUserData();
	if( !mat )
	{
		mat = m_scene->CreateMaterial();
		mat->SetTag(src->getId() ? src->getId() : "");

		// TODO: fill material data
		mat->SetEffect(ProcessEffect(SafeResolve<domEffect>(src->getInstance_effect()->getUrl())));

		src->setUserData(mat);
	}
	return mat;
}

Material* ColladaLoader::CreateDefaultMaterial(bool supportNormals)
{
	Effect *effect = m_scene->CreateEffect();
	effect->SetTag("-default-effect-");
	FillEffectWithPlainColor(effect, supportNormals);

	Material *mat = m_scene->CreateMaterial();
	mat->SetTag("-default-material-");
	mat->SetEffect(effect);

	return mat;
}

static TexWrapMode GetWrapMode(domFx_sampler_wrap_common w)
{
	// grep 'enum domFx_sampler_wrap_common'
	TexWrapMode wraps[FX_SAMPLER_WRAP_COMMON_COUNT] =
	{
		SAMPLER_WRAP_MODE_NONE,
		SAMPLER_WRAP_MODE_WRAP,
		SAMPLER_WRAP_MODE_MIRROR,
		SAMPLER_WRAP_MODE_CLAMP,
		SAMPLER_WRAP_MODE_BORDER,
	};
	assert(w < FX_SAMPLER_WRAP_COMMON_COUNT);
	return wraps[w];
}


static Image* ProcessImage(MsgListenerWrapper &msg, EditableScene *scene, domImage *src)
{
	assert(src);
	Image *image = (Image *) src->getUserData();
	if( !image )
	{
		image = scene->CreateImage();
		image->SetTag(src->getId() ? src->getId() : "");

		daeElement *elt = src->getChild("init_from");
		if( !elt )
		{
			throw std::runtime_error("init_from element is absent in <image>");
		}
		msg(MSG_SEVERITY_INFO) << "image: " << elt->getCharData();

		// set image data

		std::string fileName = cdom::uriToNativePath(src->getInit_from()->getValue().str());

#ifdef UNICODE
		size_t count = 0;
		mbstowcs_s(&count, NULL, 0, fileName.c_str(), 0);
		std::wstring buf_(count, '\0');
		mbstowcs_s(&count, &buf_[0], buf_.size(), fileName.c_str(), buf_.size());
		const wchar_t *buf = buf_.c_str();
#else
		const char *buf = fileName.c_str();
#endif

		image->SetFileName(buf);
		src->setUserData(image);
	}
	return image;
}

static void AssignImage(MsgListenerWrapper &msg, EditableScene *scene, Effect *effect, Channel ch, 
                        domCommon_color_or_texture_type_complexType::domTexture *tex)
{
	assert(effect);
	assert(ch < CH_COUNT);
	assert(tex);
	assert(tex->getParent());

	daeSIDResolver samplerResolver(tex->getParent(), tex->getTexture());
	for(;;)
	{
		if( daeSIDResolver::sid_success_element == samplerResolver.getState()
			|| NULL == samplerResolver.getContainer()->getParent() )
		{
			break;
		}
		samplerResolver.setContainer(samplerResolver.getContainer()->getParent());
	}
	domCommon_newparam_type *samplerNP = SafeResolve<domCommon_newparam_type>(samplerResolver);
	domFx_sampler2D_commonRef sampler = samplerNP->getSampler2D();
	assert(sampler);


	TexSamplerState ss;
	ss.wrapS = sampler->getWrap_s() ? GetWrapMode(sampler->getWrap_s()->getValue()) : SAMPLER_WRAP_MODE_WRAP;
	ss.wrapT = sampler->getWrap_t() ? GetWrapMode(sampler->getWrap_t()->getValue()) : SAMPLER_WRAP_MODE_WRAP;
	// TODO: other sampler fields
	effect->SetSamplerState(ch, ss);


	xsNCName surfaceSID = sampler->getSource()->getValue();
	daeSIDResolver surfaceResolver(tex->getParent(), surfaceSID);
	for(;;)
	{
		if( daeSIDResolver::sid_success_element == surfaceResolver.getState()
			|| NULL == surfaceResolver.getContainer()->getParent() )
		{
			break;
		}
		surfaceResolver.setContainer(surfaceResolver.getContainer()->getParent());
	}
	domCommon_newparam_type *surfaceNP = SafeResolve<domCommon_newparam_type>(surfaceResolver);

	domFx_surface_common *surface = surfaceNP->getSurface();
	// TODO: support domFx_surface_type_enum getType()

	const xsIDREF& idRef = surface->getFx_surface_init_common()->getInit_from_array()[0]->getValue();
	effect->SetImage(ch, ProcessImage(msg, scene, SafeResolve<domImage>(idRef)));
}

void ColladaLoader::ProcessRegularTexture(Effect *effect, domCommon_color_or_texture_type *tex, Channel ch)
{
	if( tex )
	{
		if( tex->getTexture() )
		{
			AssignImage(m_msg, m_scene, effect, ch, tex->getTexture());
			m_effectExtrasMap[effect].aliases[tex->getTexture()->getTexcoord() ? tex->getTexture()->getTexcoord() : "TEXCOORD"] = g_semantics[ch];
		}
		else if( tex->getColor() )
		{
			effect->SetColor(ch, ArrayConvert<4>(tex->getColor()->getValue()), 4);
			tex->getColor()->setUserData(effect->GetAnimationTarget(ch));
		}
	}
}

void ColladaLoader::ProcessReflectionTexture(Effect *effect,
                                             domCommon_color_or_texture_type *tex,
                                             domCommon_float_or_param_type *factor)
{
	if( tex )
	{
		if( tex->getTexture() )
		{
			AssignImage(m_msg, m_scene, effect, CH_REFLECTION, tex->getTexture());
//			m_effectExtrasMap[effect].aliases["NORMAL"] = g_semantics[CH_REFLECTION]; // TODO: redesign
		}

		float a = 1;
#ifndef COLLADA_STRICT
		// profile="MAX3D", <amount/>
		if( daeElement *elt = tex->getDescendant("amount") )
		{
			a = (float) atof(elt->getCharData().c_str());
		}
#endif

		if( factor && factor->getFloat() )
		{
			float c = (float) factor->getFloat()->getValue() * a;
			effect->SetColor(CH_REFLECTION, float4(c,c,c,1), 4);
		}
		else
		{
			effect->SetColor(CH_REFLECTION, float4(a,a,a,1), 4);
		}
	}
}

void ColladaLoader::ProcessTransparencyTexture(Effect *effect,
                                               domCommon_transparent_type *tex,
                                               domCommon_float_or_param_type *factor)
{
	static const OpacityMode modes[FX_OPAQUE_ENUM_COUNT] = 
	{
		OPAQUE_A_ONE,
		OPAQUE_RGB_ZERO,
	};

	if( tex )
	{
		effect->SetOpacityMode(modes[tex->getOpaque()]);

		if( tex->getTexture() )
		{
			AssignImage(m_msg, m_scene, effect, CH_TRANSPARENT, tex->getTexture());
			m_effectExtrasMap[effect].aliases[tex->getTexture()->getTexcoord() ? tex->getTexture()->getTexcoord() : "TEXCOORD"] = g_semantics[CH_TRANSPARENT];
		}
		else if( tex->getColor() )
		{
			effect->SetColor(CH_TRANSPARENT, ArrayConvert<4>(tex->getColor()->getValue()), 4);
			tex->getColor()->setUserData(effect->GetAnimationTarget(CH_TRANSPARENT));
		}
	}

	if( factor && factor->getFloat() )
	{
		effect->SetTransparency((float) factor->getFloat()->getValue());
		factor->getFloat()->setUserData(effect->GetTransparencyTarget());
	}
}

void ColladaLoader::ProcessFColladaBumpTexture(Effect *effect, daeElement *bumpTex)
{
	if( bumpTex )
	{
		if( daeElement *elt = bumpTex->getDescendant("texture") )
		{
			typedef domCommon_color_or_texture_type_complexType::domTexture domTexture;
			typedef domCommon_color_or_texture_type_complexType::domTextureRef domTextureRef;

			domTextureRef tr = domTextureRef::staticCast(domTexture::create(*elt->getDAE()));
			tr->setup(domTexture::registerElement(*elt->getDAE()));
			tr->setTexture(elt->getAttribute("texture").c_str());
			tr->setTexcoord(elt->getAttribute("texcoord").empty() ? "TEXCOORD" : elt->getAttribute("texcoord").c_str());

			bumpTex->add(tr);

			AssignImage(m_msg, m_scene, effect, CH_BUMP, tr);
			m_effectExtrasMap[effect].aliases[tr->getTexcoord() ? tr->getTexcoord() : "TEXCOORD"] = g_semantics[CH_BUMP];
		}
	}
}

void ColladaLoader::FillEffectFromCommonProfile(Effect *effect, const domProfile_COMMON *common)
{
	assert(effect);
	assert(common);

	domProfile_COMMON::domTechnique *tech = common->getTechnique();
	if( domProfile_COMMON::domTechnique::domConstant *constant = tech->getConstant() )
	{
		effect->SetShader(SHADER_CONSTANT);

		// declare textures
		ProcessRegularTexture(effect, constant->getEmission(), CH_EMISSION);
		ProcessReflectionTexture(effect, constant->getReflective(), constant->getReflectivity());
	//	constant->getIndex_of_refraction()

		ProcessTransparencyTexture(effect, constant->getTransparent(), constant->getTransparency());
	}
	else
	if( domProfile_COMMON::domTechnique::domLambert *lambert = tech->getLambert() )
	{
		effect->SetShader(SHADER_LAMBERT);

		// declare textures
		ProcessRegularTexture(effect, lambert->getEmission(),   CH_EMISSION);
		ProcessReflectionTexture(effect, lambert->getReflective(), lambert->getReflectivity());
	//	lambert->getIndex_of_refraction();

		ProcessRegularTexture(effect, lambert->getAmbient(), CH_AMBIENT);
		ProcessRegularTexture(effect, lambert->getDiffuse(), CH_DIFFUSE);
		ProcessTransparencyTexture(effect, lambert->getTransparent(), lambert->getTransparency());
	}
	else
	if( domProfile_COMMON::domTechnique::domBlinn *blinn = tech->getBlinn() )
	{
		// workaround
		float shininess = 1;
		if( blinn->getShininess() && blinn->getShininess()->getFloat() )
			shininess = (float) blinn->getShininess()->getFloat()->getValue();

		effect->SetShader((shininess > 1 ? SHADER_BLINN_PHONG : SHADER_BLINN_TORRANCE_SPARROW));
		effect->SetShininess(shininess);

		// declare textures
		ProcessRegularTexture(effect, blinn->getEmission(), CH_EMISSION);
		ProcessReflectionTexture(effect, blinn->getReflective(), blinn->getReflectivity());
	//	blinn->getIndex_of_refraction();

		ProcessRegularTexture(effect, blinn->getAmbient(), CH_AMBIENT);
		ProcessRegularTexture(effect, blinn->getDiffuse(), CH_DIFFUSE);
		ProcessRegularTexture(effect, blinn->getSpecular(), CH_SPECULAR);
		ProcessTransparencyTexture(effect, blinn->getTransparent(), blinn->getTransparency());
	}
	else
	if( domProfile_COMMON::domTechnique::domPhong *phong = tech->getPhong() )
	{
		effect->SetShader(SHADER_PHONG);

		// workaround
		if( phong->getShininess() && phong->getShininess()->getFloat() )
			effect->SetShininess((float) phong->getShininess()->getFloat()->getValue());

		// declare textures
		ProcessRegularTexture(effect, phong->getEmission(),   CH_EMISSION);
		ProcessReflectionTexture(effect, phong->getReflective(), phong->getReflectivity());
	//	phong->getIndex_of_refraction();

		ProcessRegularTexture(effect, phong->getAmbient(), CH_AMBIENT);
		ProcessRegularTexture(effect, phong->getDiffuse(), CH_DIFFUSE);
		ProcessRegularTexture(effect, phong->getSpecular(), CH_SPECULAR);
		ProcessTransparencyTexture(effect, phong->getTransparent(), phong->getTransparency());
	}
	else
	{
		assert(!"no shader found in common profile technique");
	}

	// profile="FCOLLADA", <spec_level/>
#ifndef COLLADA_STRICT
	if( daeElement *elt = tech->getDescendant("spec_level") ? tech->getDescendant("spec_level")->getChild("float") : NULL )
	{
		float spec_level = (float) atof(elt->getCharData().c_str());
		effect->SetColor(CH_SPECULAR, float4(effect->GetColor(CH_SPECULAR)) * spec_level, 3);
	}
#endif

#ifndef COLLADA_STRICT
	// profile="FCOLLADA", <bump/>
	ProcessFColladaBumpTexture(effect, tech->getDescendant("bump"));
#endif
}

void ColladaLoader::FillEffectWithPlainColor(Effect *effect, bool supportNormals)
{
	assert(effect);
	effect->SetShader(SHADER_LAMBERT);
	effect->SetColor(CH_DIFFUSE, float3(.5f, .5f, .5f), 3);
}

Effect* ColladaLoader::ProcessEffect(domEffect *src)
{
	assert(src);
	Effect *effect = (Effect *) src->getUserData();
	if( !effect )
	{
		effect = m_scene->CreateEffect();
		effect->SetTag(src->getId() ? src->getId() : "");

		//
		// fill effect
		//

		const domFx_profile_abstract_Array &profiles = src->getFx_profile_abstract_array();
		bool commonProfileFound = false;

		for( size_t i = 0; i < profiles.getCount(); ++i )
		{
			if( const domProfile_COMMON *common = daeSafeCast<const domProfile_COMMON>(profiles[i]) )
			{
				FillEffectFromCommonProfile(effect, common);
				commonProfileFound = true;
				break;
			}
		}

		if( !commonProfileFound )
		{
			m_msg(MSG_SEVERITY_WARNING) << "no common profile found in effect '" << src->getId() << "'";
			FillEffectWithPlainColor(effect, true);
		}

		src->setUserData(effect);
	}
	return effect;
}

MorphController* ColladaLoader::ProcessMorphController(domController *src)
{
	assert(src && src->getMorph());
	MorphController *morphCtrl = (MorphController *) src->getUserData();
	if( !morphCtrl )
	{
		const domMorph *morph = src->getMorph();

		Mesh *baseMesh = ProcessMesh(SafeResolve<domGeometry>(morph->getSource()));
		assert(baseMesh);

		domMorph::domTargets *targets = morph->getTargets();
		if( !targets )
			throw std::runtime_error("<targets> element not found in morph controller");

		std::auto_ptr<SourceImpl> morphTargets;
		std::auto_ptr<SourceImpl> morphWeights;

		for( size_t inputIdx = 0; inputIdx < targets->getInput_array().getCount(); ++inputIdx )
		{
			domInputLocal *input = targets->getInput_array()[inputIdx];
			if( 0 == strcmp(input->getSemantic(), "MORPH_TARGET") )
				morphTargets.reset(new SourceImpl(SafeResolve<domSource>(input->getSource()), NULL));
			else if( 0 == strcmp(input->getSemantic(), "MORPH_WEIGHT") )
				morphWeights.reset(new SourceImpl(SafeResolve<domSource>(input->getSource()), NULL));
			else
				m_msg(MSG_SEVERITY_WARNING) << "unknown semantic '" << input->getSemantic() << "'";
		}

		if( !morphTargets.get() )
			throw std::runtime_error("MORPH_TARGET input not found in morph controller");
		if( morphTargets->GetType() != SourceImpl::VT_IDREF )
			throw std::runtime_error("MORPH_TARGET is expected to be of IDREF type");
		if( !morphWeights.get() )
			throw std::runtime_error("MORPH_WEIGHT input not found in morph controller");
		if( morphWeights->GetType() != SourceImpl::VT_FLOAT || morphWeights->GetElementSize() != 1 )
			throw std::runtime_error("MORPH_WEIGHT is expected to be of float1 type");


		std::vector<Mesh*> bufTargets(morphTargets->GetElementCount());
		for( size_t i = 0; i < morphTargets->GetElementCount(); ++i )
		{
			bufTargets[i] = ProcessMesh(morphTargets->GetRefT<domGeometry>(i));
			assert(bufTargets[i]);
		}

		AutoPtr<MemoryObject> bufWeights;
		m_scene->GetMemoryManager()->Allocate(morphWeights->GetElementCount() * sizeof(float),
			morphWeights->GetFloatArray(), &bufWeights);


		//
		// create morph controller and fill it with data
		//

		morphCtrl = m_scene->CreateMorphController();
		morphCtrl->SetTag(src->getID() ? src->getID() : (std::string(baseMesh->GetTag()) + "_morphed").c_str());
		morphCtrl->SetBaseMesh(baseMesh);
		morphCtrl->SetWeights(bufWeights);
		morphCtrl->SetTargets(&bufTargets[0], bufTargets.size());


		// finished controller creation
		src->setUserData(morphCtrl);
	}
	return morphCtrl;
}

SkinController* ColladaLoader::ProcessSkinController(domController *src)
{
	assert(src && src->getSkin());
	SkinController *skinCtrl = (SkinController *) src->getUserData();
	if( !skinCtrl )
	{
		const domSkin *skin = src->getSkin();


		//
		// extract skeleton sids
		//

		m_skinCtrlExtras.push_back(new SkinCtrlExtra());
		SkinCtrlExtra &extra = *m_skinCtrlExtras.back();

		const domInputLocal *jointInput = NULL;
		const domInputLocal *invBindMatrixInput = NULL;


		const domSkin::domJoints *joints = skin->getJoints();
		assert(joints);

		for( size_t i = 0; i < joints->getInput_array().getCount(); ++i )
		{
			const domInputLocal *input = joints->getInput_array()[i];
			if( !jointInput && 0 == _stricmp("JOINT", input->getSemantic()) )
				jointInput = input;
			else if( !invBindMatrixInput && 0 == _stricmp("INV_BIND_MATRIX", input->getSemantic()) )
				invBindMatrixInput = input;
		}
		if( !jointInput )
		{
			throw std::string("JOINT input is absent in <joints/> of skin controller with id=\"") + (src->getId() ? src->getId() : "") + "\"";
		}
		if( !invBindMatrixInput )
		{
			throw std::string("INV_BIND_MATRIX input is absent in <joints/> of skin controller with id=\"") + (src->getId() ? src->getId() : "") + "\"";
		}

		extra.jointSource = extra.sourceCache.LocateKey(SafeResolve<domSource>(jointInput->getSource()));
		extra.invBindMatrixSource = extra.sourceCache.LocateKey(SafeResolve<domSource>(invBindMatrixInput->getSource()));

		assert(extra.jointSource->GetElementCount() == extra.invBindMatrixSource->GetElementCount());


		//
		// extract vertex weights
		//

		const domSkin::domVertex_weights *vw = skin->getVertex_weights();
		assert(vw);

		const domSkin::domVertex_weights::domVcount *vcountSrc = vw->getVcount();
		if( !vcountSrc )
		{
			throw std::string("<vcount> element is absent in skin controller with id=\"") + (src->getId() ? src->getId() : "") + "\"";
		}

		const domSkin::domVertex_weights::domV *vSrc = vw->getV();
		if( !vSrc )
		{
			throw std::string("<v> element is absent in skin controller with id=\"") + (src->getId() ? src->getId() : "") + "\"";
		}

		const domInputLocalOffset *jointInput1 = NULL;
		const domInputLocalOffset *weightInput = NULL;
		domUint maxOffset = 0;

		for( size_t i = 0; i < vw->getInput_array().getCount(); ++i )
		{
			const domInputLocalOffset *input = vw->getInput_array()[i];

			if( maxOffset < input->getOffset() )
				maxOffset = input->getOffset();

			if( !jointInput1 && 0 == _stricmp("JOINT", input->getSemantic()) )
				jointInput1 = input;
			else
				if( !weightInput && 0 == _stricmp("WEIGHT", input->getSemantic()) )
					weightInput = input;
		}

		if( !jointInput1 )
		{
			throw std::string("JOINT input is absent in <vertex_weights/> of skin controller with id=\"") + (src->getId() ? src->getId() : "") + "\"";
		}

		if( !weightInput )
		{
			throw std::string("WEIGHT input is absent in <vertex_weights/> of skin controller with id=\"") + (src->getId() ? src->getId() : "") + "\"";
		}


		const domSource *srcWeightSource = daeSafeCast<domSource>(weightInput->getSource().getElement());
		if( !srcWeightSource )
		{
			throw std::runtime_error(std::string("invalid uri: ") + weightInput->getSource().getURI());
		}

		std::auto_ptr<SourceImpl> weightSource(new SourceImpl(srcWeightSource, NULL));
		if( weightSource->GetElementSize() != 1 || weightSource->GetType() != SourceImpl::VT_FLOAT )
		{
			throw std::runtime_error(std::string("weight should be scalar float: ") + weightInput->getSource().getURI());
		}
		const float *weights = weightSource->GetFloatArray();

		const size_t vstride = (size_t) maxOffset + 1;

		const domListOfInts &v = vSrc->getValue();
		const domListOfUInts &vcount = vcountSrc->getValue();


		size_t weightedJointCount = 0;
		for( size_t vtxIdx = 0, cnt = 0; vtxIdx < vcount.getCount(); ++vtxIdx )
		{
			weightedJointCount += (size_t) vcount[vtxIdx];
		}


		Mesh *baseMesh = ProcessMesh(SafeResolve<domGeometry>(skin->getSource()));
		assert(baseMesh);

		// all primary vertex elements have the same index offset and same vertex count, so we may take any
		size_t weightCount = baseMesh->GetVertexBuffer(baseMesh->GetPrimaryVertexElementDescs()->inputSlot)->GetBytesSize()
			/ baseMesh->GetRawVertexSize(baseMesh->GetPrimaryVertexElementDescs()->inputSlot);
		assert(vcount.getCount() == weightCount);
		assert(m_meshExtrasMap.count(baseMesh));

		skinCtrl = m_scene->CreateSkinController(weightedJointCount, weightCount);
		skinCtrl->SetTag(src->getID() ? src->getID() : (std::string(baseMesh->GetTag()) + "_skinned").c_str());
		skinCtrl->SetBaseMesh(baseMesh);

		m_skinCtrlExtrasMap[skinCtrl] = &extra;


		//
		// extract bind shape matrix
		//

		if( skin->getBind_shape_matrix() )
		{
			skinCtrl->SetBindShape(Matrix4FromDomFloatArray(skin->getBind_shape_matrix()->getValue()));
		}


		//
		// write vertex weights and bindings to the controller
		//

		WeightedJoint *wj = skinCtrl->LockWeightedJoints();
		VertexWeightsBinding *vwb = skinCtrl->LockBinding();

		unsigned int vtxTotal = static_cast<unsigned int>(vcount.getCount());
		for( unsigned int vtxIdx = 0, cnt = 0; vtxIdx < vtxTotal; ++vtxIdx )
		{
			float wsum = 0;
			unsigned int influenceCount = static_cast<unsigned int>(vcount[vtxIdx]);

			vwb[vtxIdx].begin = cnt;
			vwb[vtxIdx].end = cnt + influenceCount;

			for( unsigned int i = 0; i < influenceCount; ++i, ++cnt )
			{
				unsigned int jointIdx = static_cast<unsigned int>(v[cnt * vstride + (size_t) jointInput1->getOffset()]);
				unsigned int weightIdx = static_cast<unsigned int>(v[cnt * vstride + (size_t) weightInput->getOffset()]);
				if( weightIdx >= weightSource->GetElementCount() )
				{
					throw std::runtime_error("weight index out of range");
				}
				if( jointIdx >= extra.jointSource->GetElementCount() )
				{
					throw std::runtime_error("joint index out of range");
				}
				wsum += weights[weightIdx];

				wj[cnt].jointIdx = jointIdx;
				wj[cnt].weight = weights[weightIdx];
			}

			// normalize weights
			for( size_t i = vwb[vtxIdx].begin; i < vwb[vtxIdx].end; ++i )
			{
				wj[i].weight /= wsum;
			}
		}

		skinCtrl->UnlockBinding();
		skinCtrl->UnlockWeightedJoints();


		// finished controller creation
		src->setUserData(skinCtrl);
	}
	return skinCtrl;
}

void ColladaLoader::ProcessLibraryVisualScenes(const domLibrary_visual_scenes_Array &vsLibs)
{
	if( 0 == vsLibs.getCount() )
	{
		m_msg(MSG_SEVERITY_INFO) << "no visual scenes defined";
	}

	for( size_t libIdx = 0; libIdx < vsLibs.getCount(); ++libIdx )
	{
		const domVisual_scene_Array &visualScenes = vsLibs[libIdx]->getVisual_scene_array();
		for( size_t vsIdx = 0; vsIdx < visualScenes.getCount(); ++vsIdx )
		{
			// Covert all the <node>s in the <visual_scene>.
			// This is a recursive process, so any child nodes will also be converted.
			const domNode_Array &nodes = visualScenes[vsIdx]->getNode_array();
			for( size_t i = 0; i < nodes.getCount(); ++i )
			{
				ProcessNode(nodes[i]);
			}
		}
	}
}

void ColladaLoader::ProcessLibraryAnimations(const domLibrary_animations_Array &animationLibs)
{
	for( size_t libIdx = 0; libIdx < animationLibs.getCount(); ++libIdx )
	{
		// fill the stack with top level animations
		const domAnimation_Array &animations = animationLibs[libIdx]->getAnimation_array();
		for( size_t animIdx = 0; animIdx < animations.getCount(); ++animIdx )
		{
			ProcessAnimation(animations[animIdx]);
		}
	}
}

void ColladaLoader::ProcessLibraryAnimationClips(const domLibrary_animation_clips_Array &animationClipLibs)
{
	for( size_t libIdx = 0; libIdx < animationClipLibs.getCount(); ++libIdx )
	{
		// fill the stack with top level animations
		const domAnimation_clip_Array &clips = animationClipLibs[libIdx]->getAnimation_clip_array();
		for( size_t clipIdx = 0; clipIdx < clips.getCount(); ++clipIdx )
		{
			ProcessAnimationClip(clips[clipIdx]);
		}
	}
}

AnimationClip* ColladaLoader::ProcessAnimationClip(domAnimation_clip *src)
{
	assert(src);
	AnimationClip *clip = (AnimationClip *) src->getUserData();
	if( !clip )
	{
		clip = m_scene->CreateAnimationClip();
		clip->SetTag(src->getId() ? src->getId() : "");
		clip->SetTimeBounds((float) src->getStart(), (float) src->getEnd());

		domInstanceWithExtra_Array &aa = src->getInstance_animation_array();
		for( size_t instIdx = 0; instIdx < aa.getCount(); ++instIdx )
		{
			if( Animation *anim = ProcessAnimation(SafeResolve<domAnimation>(aa[instIdx]->getUrl())) )
			{
				clip->AddAnimation(anim);
				m_orphanAnimations.erase(anim);
			}
		}

		// object creation finished
		src->setUserData(clip);
	}
	return clip;
}

Animation* ColladaLoader::ProcessAnimation(domAnimation *src)
{
	assert(src);
	Animation *animation = (Animation *) src->getUserData();
	if( (void *) -1 == animation )
	{
		return NULL; // there was an unsuccessful attempt to parse this animation.
	}
	if( !animation )
	{
		src->setUserData((void *) -1); // set unsuccessful flag for case of failure

		typedef std::pair<size_t, Property*> SamplerTargetPair;
		std::vector<SamplerTargetPair> boundChannels;

		SourceCache sourceCache(NULL);
		SamplerCache samplerCache(&sourceCache);


		//
		// get list of animation channels and theirs samplers
		//

		for( size_t chanIdx = 0; chanIdx < src->getChannel_array().getCount(); ++chanIdx ) 
		{
			const char *raw = src->getChannel_array()[chanIdx]->getTarget();
			assert(raw);

			daeSIDResolver sidResolver(src->getDocument()->getDomRoot(), raw);
			daeSIDResolver::ResolveState state = sidResolver.getState();
			daeElement *elem = sidResolver.getElement();

			if( AnimationTarget *t = elem ? (AnimationTarget *) elem->getUserData() : NULL )
			{
				const char *param = NULL;
				if( daeSIDResolver::sid_success_double == state )
				{
					// it can be either .x or (n)
					param = strrchr(raw, '.');
					if( param )
					{
						++param; // skip '.' symbol
					}
					else
					{
						param = strchr(raw, '(');
					}
					assert(param);
				}

				if( Property *linkedParam = t->GetDelegate(param) )
				{
					SamplerInfo *sampler = samplerCache.LocateKey(SafeResolve<domSampler>(src->getChannel_array()[chanIdx]->getSource()));

					if( linkedParam->GetSize() != sampler->GetOutput()->GetElementSize() )
					{
						m_msg(MSG_SEVERITY_WARNING) << "Sampler '" << sampler->GetSamplerTag()
							<< "' is incompatible with target '" << raw << "'";
					}
					else
					{
						size_t samIdx = samplerCache.FindPointer(sampler);
						boundChannels.push_back(SamplerTargetPair(samIdx, linkedParam));
					}
				}
				else
				{
					m_msg(MSG_SEVERITY_WARNING) << "target object '" << sidResolver.getTarget() 
						<< "' does not support parameter with name='" << param << "'";
				}
			}
			else
			{
				m_msg(MSG_SEVERITY_WARNING) << "target object '" << sidResolver.getTarget() << "' not found";
			}
		}

		std::vector<AnimationDataDesc> dataDescs(sourceCache.GetSize());
		for( size_t i = 0; i < dataDescs.size(); ++i )
		{
			SourceImpl *source = sourceCache.GetPointer(i);
			assert(SourceImpl::VT_FLOAT == source->GetType() || SourceImpl::VT_NAME == source->GetType());
			dataDescs[i].sampleCount = source->GetElementCount();
			dataDescs[i].elementCount = source->GetElementSize();
			dataDescs[i].elementType = (SourceImpl::VT_FLOAT == source->GetType()) ? ST_FLOAT : ST_UINT8;

			if( SourceImpl::VT_NAME == source->GetType() && 1 != source->GetElementSize() )
			{
				m_msg(MSG_SEVERITY_WARNING) << "INTERPOLATION source '" << source->GetSourceTag() << "' has wrong dimension";
				return NULL;
			}
		}

		std::vector<AnimationSamplerDesc> samplerDescs(samplerCache.GetSize());
		for( size_t i = 0; i < samplerDescs.size(); ++i )
		{
			SamplerInfo *sampler = samplerCache.GetPointer(i);
			AnimationSamplerDesc &sd = samplerDescs[i];

			sd.in = sourceCache.FindPointer(sampler->GetInput());
			sd.out = sourceCache.FindPointer(sampler->GetOutput());
			sd.interpolation = sourceCache.FindPointer(sampler->GetInterpolation());
			sd.tangentsIn = sourceCache.FindPointer(sampler->GetInT());
			sd.tangentsOut = sourceCache.FindPointer(sampler->GetOutT());

			if( -1 == sd.interpolation )
			{
				m_msg(MSG_SEVERITY_WARNING) << "Sampler '" << sampler->GetSamplerTag() << "' has no interpolation info. LINEAR interpolation will be used.";
			}
		}


		if( boundChannels.empty() )
		{
			m_msg(MSG_SEVERITY_INFO) << "Animation '" << src->getId() << "' has no bound channels and will be ignored.";
			return NULL;
		}


		//
		// create animation object
		//

		animation = m_scene->CreateAnimation(&dataDescs[0], dataDescs.size(), &samplerDescs[0], samplerDescs.size());
		m_orphanAnimations.insert(animation);

		animation->SetTag(src->getId() ? src->getId() : "");


		//
		// fill animation data
		//

		std::set<std::string> warnings;
		std::set<const SamplerInfo *> samWarnings;

		typedef std::map<std::string, Interpolation> InterpTable;
		InterpTable idata;
		idata["STEP"]     = INTERPOLATION_STEP;
		idata["LINEAR"]   = INTERPOLATION_LINEAR;
		idata["BEZIER"]   = INTERPOLATION_BEZIER;
		idata["CARDINAL"] = INTERPOLATION_CARDINAL;
		idata["HERMITE"]  = INTERPOLATION_HERMITE;
		idata["BSPLINE"]  = INTERPOLATION_BSPLINE;

		for( size_t srcIdx = 0; srcIdx < dataDescs.size(); ++srcIdx )
		{
			SourceImpl *source = sourceCache.GetPointer(srcIdx);

			switch( source->GetType() )
			{
				case SourceImpl::VT_FLOAT:
				{
					float *dst = (float *) animation->LockDataSource(srcIdx);
					memcpy(dst, source->GetFloatArray(), source->GetElementCount() * source->GetElementSize() * sizeof(float));
					animation->UnlockDataSource(srcIdx);
					break;
				}
				case SourceImpl::VT_NAME:
				{
					assert(1 == source->GetElementSize());
					unsigned char *dst = (unsigned char *) animation->LockDataSource(srcIdx);
					for( size_t j = 0; j < source->GetElementCount(); ++j )
					{
						InterpTable::const_iterator it = idata.find(source->GetName(j));
						if( idata.end() != it )
						{
							assert(it->second < INTERPOLATION_COUNT);
							dst[j] = (unsigned char) it->second;


							for( size_t samIdx = 0; samIdx < samplerDescs.size(); ++samIdx )
							{
								const AnimationSamplerDesc &sd = samplerDescs[samIdx];
								if( sd.interpolation == srcIdx )
								{
									switch( dst[j] )
									{
									case INTERPOLATION_BEZIER:
										if( -1 == sd.tangentsIn || -1 == sd.tangentsOut ||
											dataDescs[sd.out].elementCount * 2 != dataDescs[sd.tangentsIn].elementCount ||
											dataDescs[sd.out].elementCount * 2 != dataDescs[sd.tangentsOut].elementCount )
										{
											// replace the lame interpolator by linear one
											dst[j] = INTERPOLATION_LINEAR;

											// print warning only once for each sampler
											const SamplerInfo *sampler = samplerCache.GetPointer(samIdx);
											if( samWarnings.insert(sampler).second )
											{
												m_msg(MSG_SEVERITY_WARNING) 
													<< "Sampler '" << sampler->GetSamplerTag() 
													<< "' has wrong number of tangents. "
													<< "LINEAR interpolation will be used instead of BEZIER.";
											}
										}
										break;
									}
								}
							}
						}
						else
						{
							// print warning only once for each unknown type
							if( warnings.insert(source->GetName(j)).second )
							{
								m_msg(MSG_SEVERITY_WARNING) << "Unknown interpolator '" 
									<< source->GetName(j) << "' - replaced by LINEAR.";
							}
							dst[j] = (unsigned char) INTERPOLATION_LINEAR;
						}
					}
					animation->UnlockDataSource(srcIdx);
					break;
				}
				default:
					assert(false);
			}
		}


		//
		// register targets
		//

		for( size_t i = 0; i < boundChannels.size(); ++i )
		{
			animation->AddTarget(boundChannels[i].second, boundChannels[i].first);
		}


		//
		// process children
		//

		const domAnimation_Array &children = src->getAnimation_array();
		for( size_t i = 0; i < children.getCount(); ++i )
		{
			ProcessAnimation(children[i]);
		}

		// object creation finished
		src->setUserData(animation);
	}
	return animation;
}


//
// factory
//

namespace collada
{
	extern "C" void OpenColladaFile(const TCHAR *szFileName, EditableScene *pResult) throw(...)
	{
		try
		{
			assert(pResult);
			ColladaLoader loader(pResult->GetMessageListener());
			loader.Load(pResult, szFileName);
		}
		catch(...)
		{
			throw;
		}
	}
}

// end of file
