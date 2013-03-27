/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SceneGraph.h

#pragma once

#include "SceneGraphBase.h"
#include "RefCounted.h"
#include "MemoryManager.h"

#include <cassert>


namespace collada
{
///////////////////////////////////////////////////////////////////////////////

/// Property interface gives you access to a single property of an object.
struct Property
{
	virtual const char* GetSemantic() const = 0; ///< Returns semantic string which may describe the property type
	virtual size_t GetSize() const = 0; ///< @return number of elements stored within property

	/// Gets value of property.
	/// @arg result must point to a float array with at least the number of elements returned by GetSize()
	virtual void GetValue(float *result) const = 0;

	/// Sets value of property.
	/// @arg result must point to a float array with at least the number of elements returned by GetSize()
	virtual void SetValue(const float *floatArray) = 0;
};

/// Parent interface for objects which may expose properties used in animation
struct AnimationTarget
{
	/// Remember that every time you call this function, a new Property object may be allocated.
	/// @arg param String describes part of the object that property will be bound to. See description of derived interfaces.
	virtual Property* GetDelegate(const char *param) = 0;

	/// Determine whether object has any bound properties.
	virtual bool HasBoundDelegates() const = 0;
};

///////////////////////////////////////////////////////////////////////////////

/// Parent interface for most scene graph objects
struct Base
{
	virtual MemoryManager* GetMemoryManager() const = 0;
	virtual Scene* GetNativeContext() const = 0;

	/// @return user-defined tag that was set by SetTag
	virtual const char* GetTag() const = 0;

	/// Set user-defined tag that can be used for any purpose in application
	virtual void SetTag(const char *tag) = 0;

	/// @return IView
	virtual IView* GetView(size_t id, Scene *context) = 0;

	/// Detach associated view and free all related resources. The view will be created again next time you call GetView.
	/// @arg id Identifier of corresponding IViewFactory registered with Scene::RegisterViewFactory
	virtual void DetachView(size_t id) = 0;

	template <class T> T* GetViewT(size_t id, Scene *context)
	{
		assert(dynamic_cast<T*>(GetView(id, context)));
		return static_cast<T*>(GetView(id, context));
	}

	/// @return Number of properties of this object.
	virtual size_t GetPropertyCount() const = 0;
	/// Gives assess to the collection of object's properties.
	virtual Property* GetProperty(size_t idx) = 0;
};

///////////////////////////////////////////////////////////////////////////////

/// The actual data representation of an object might be stored only once. However, the object can appear in a
/// scene more than once. The object may be transformed in various ways each time it appears. Each
/// appearance in the scene is called an instance of the object. The family of Instance interfaces enables a
/// scene graph to instantiate corresponding objects.
/// Each instance inherits the local coordinate system from its node.
struct Instance : public Base
{
	/// @return Pointer to Node where the object is instantiated.
	virtual Node* GetNode(Scene *context) const = 0;

	/// @return Pointer to instantiated object.
	virtual Base* GetEntity(Scene *context) const = 0;
};

///////////////////////////////////////////////////////////////////////////////

/// Represents image file
struct Image : public Base
{
	virtual const TCHAR* GetFileName() const = 0;
	virtual void SetFileName(const TCHAR *fileName) = 0;
};

///////////////////////////////////////////////////////////////////////////////

/// Describes the visual appearance of a geometric object.
struct Effect : public Base
{
	virtual Shader GetShader() const = 0;
	virtual void SetShader(Shader shader) = 0;

	virtual size_t GetInputCount() const = 0;
	virtual const char* GetInputSemantic(size_t index) const = 0;

	virtual AnimationTarget* GetAnimationTarget(Channel ch) = 0;

	virtual const float* GetColor(Channel ch) const = 0; // float4
	virtual Image* GetImage(Channel ch, Scene *context) const = 0;
	virtual void GetSamplerState(Channel ch, TexSamplerState &state) const = 0;

	virtual void SetColor(Channel ch, const float *color, size_t compCount) = 0; // float4
	virtual void SetImage(Channel ch, Image *img) = 0;
	virtual void SetSamplerState(Channel ch, const TexSamplerState &state) = 0;

	virtual OpacityMode GetOpacityMode() const = 0;
	virtual void SetOpacityMode(OpacityMode mode) = 0;

	// FIXME: workaround
	virtual float GetShininess() const = 0;
	virtual void SetShininess(float s) = 0;
	virtual float GetTransparency() const = 0;
	virtual void SetTransparency(float t) = 0;
	virtual AnimationTarget* GetTransparencyTarget() = 0;
};

///////////////////////////////////////////////////////////////////////////////

/// Describes the visual appearance of a geometric object.
/// In current implementation you may consider Material as synonym to Effect.
struct Material : public Base
{
	virtual Effect* GetEffect(Scene *context) const = 0;
	virtual void SetEffect(Effect *effect) = 0;
};

///////////////////////////////////////////////////////////////////////////////

struct MaterialBinding : public Base
{
	struct Target
	{
		const char *semantic;
		unsigned int index;
	};

	virtual GeometryInstance* GetParentInstance(Scene *context) const = 0;
	virtual Batch* GetBatch(Scene *context) const = 0;
	virtual Material* GetMaterial(Scene *context) const = 0;
	virtual void SetMaterial(Material*pMtl) = 0;

	virtual void SetBinding(const char *name, const Target *target) = 0;
	virtual size_t GetBindingCount() const = 0;
	virtual const char* GetBindingName(size_t index) const = 0;
	virtual bool GetBindingTarget(const char* name, Target *result) const = 0;

	// required inputs depend on batch so context is required
	virtual size_t GetRequiredInputCount(Scene *context) const = 0;
	virtual const VertexElementDesc* GetRequiredInputs(Scene *context) const = 0;
};

///////////////////////////////////////////////////////////////////////////////
// mesh subset (batch)

/// Represents subset of geometry within a mesh.
/// 
/// The entire subset has the same topology and material,
/// thus it might be sent to most graphics hardware with a single draw call.
struct Batch : public Base
{
	/// @return pointer to parent mesh
	virtual Mesh* GetMesh(Scene *context) const = 0;

	//@{
	virtual unsigned int GetIndexStride() const = 0;       ///< Number of indices per vertex
	virtual MemoryObject* GetIndexData() const = 0;
	//@}

	//@{
	virtual MemoryObject* GetCountData() const = 0;
	//@}

	// input layout
	virtual PrimitiveTopology GetPrimitiveTopology() const = 0;
	virtual const VertexElementDescEx* GetExtraVertexElementDescs() const = 0;
	virtual size_t GetExtraVertexElementCount() const = 0;

	// bounds
	//virtual const float* GetCenter(Scene *context = 0) const = 0; // float3
	//virtual const float* GetAABB(Scene *context = 0) const = 0; // float3

	// misc
	virtual bool GetDoubleSided() const = 0;
	virtual unsigned int GetPrimarySet() const = 0;
	virtual unsigned int GetPrimaryOffset() const = 0;
};

/// @return Number of primitives in batch
inline size_t CalcPrimitiveCount(const Batch *batch)
{
	if( MemoryObject *countData = batch->GetCountData() )
	{
		return countData->GetBytesSize() / sizeof(unsigned long);
	}

	unsigned int primitiveSize = 0;
	switch( batch->GetPrimitiveTopology() )
	{
		case PRIMITIVE_TOPOLOGY_LINELIST:      primitiveSize = 2; break;
		case PRIMITIVE_TOPOLOGY_TRIANGLELIST:  primitiveSize = 3; break;
	}
	if( MemoryObject *idxData = batch->GetIndexData() )
	{
		assert(primitiveSize > 0);
		assert(0 == idxData->GetBytesSize() % (batch->GetIndexStride() * primitiveSize));
		return idxData->GetBytesSize() / (batch->GetIndexStride() * primitiveSize);
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////

/// Extends Batch interface to enable write access to its structure
struct EditableBatch : public Batch
{
	/// 
	virtual void SetIndexData(MemoryObject *indexData) = 0;
	virtual void SetCountData(MemoryObject *countData) = 0; ///< optional for fixed size primitives

	/// extra attributes
	virtual void AddExtraVertexElement(const VertexElementDescEx *desc) = 0;

	// misc
	virtual void SetPrimitiveTopology(PrimitiveTopology pt) = 0;
	virtual void SetPrimarySet(unsigned int primarySet) = 0;
	virtual void SetPrimaryOffset(unsigned int primaryOffset) = 0;
	virtual void SetDoubleSided(bool value) = 0;
};

///////////////////////////////////////////////////////////////////////////////

/// Describes basic geometric meshes using vertex and primitive information.
///
/// Meshes embody a general form of geometric description that primarily includes 
/// vertex and primitive information.
/// Vertex information is the set of attributes associated with a point on the surface 
/// of the mesh. Each vertex includes data for attributes such as:
/// - Vertex position
/// - Vertex color
/// - Vertex normal
/// - Vertex texture coordinate
/// The mesh also includes a description of how the vertices are organized to form 
/// the geometric shape of the mesh. The mesh vertices are collated into geometric
/// primitives such as polygons, triangles, or lines.
///
/// This interface provides read-only access to mesh structures.
struct Mesh : public Base
{
	/// Primary vertex elements are common for all batches within a mesh.
	/// In addition each batch might have specific additional elements.
	virtual size_t GetPrimaryVertexElementCount() const = 0;
	virtual const VertexElementDesc* GetPrimaryVertexElementDescs() const = 0;

	/// @return Number of vertex buffers in the mesh.
	virtual size_t GetVertexBufferCount() const = 0;

	/// @return pointer to MemoryObject containing vertex data.
	virtual MemoryObject* GetVertexBuffer(size_t bufferIdx) const = 0;
	virtual unsigned int GetRawVertexSize(size_t bufferIdx) const = 0; // TODO: move to vertex element desc

	/// @return Number of Batch objects in the mesh.
	virtual size_t GetBatchCount() const = 0;

	/// @return Pointer to Batch object.
	virtual Batch* GetBatch(size_t batchIdx, Scene *context) const = 0;
};

///////////////////////////////////////////////////////////////////////////////

/// Extension to Mesh interface enables write access to mesh structures.
struct EditableMesh : public Mesh
{
	virtual void SetVertexBuffer(size_t bufferIdx, MemoryObject *data) = 0;

	/// write access to vertex layout
	virtual void AddPrimaryVertexElementDesc(const VertexElementDesc *desc) = 0;

	/// Create a new batch
	/// @return Pointer to newly created batch.
	virtual EditableBatch* CreateBatch() = 0;
};

///////////////////////////////////////////////////////////////////////////////

/// A geometry object is instantiated within the local coordinate system of its parent node.
/// Each geometry instance may have different set of materials.
struct GeometryInstance : public Instance
{
	/// @return Number of material bindings.
	virtual size_t GetMaterialBindingCount() const = 0;
	/// @return Pointer to stored material binding.
	virtual MaterialBinding* GetMaterialBinding(size_t index, Scene *context) const = 0;
	/// Creates a new material binding. You must create exactly one MaterialBinding for each Batch in the instanced Mesh.
	virtual MaterialBinding* CreateMaterialBinding(Batch *batch, size_t batchIdx, Material *src) = 0;
};

///////////////////////////////////////////////////////////////////////////////

/// Instantiates a Mesh object.
struct MeshInstance : public GeometryInstance
{
	virtual Mesh* GetEntity(Scene *context) const = 0;
};

///////////////////////////////////////////////////////////////////////////////

/// Contains vertex and primitive information sufficient to describe blend-weight skinning.
struct SkinController : public Base
{
	virtual void SetBaseMesh(Mesh *baseMesh) = 0;
	virtual Mesh *GetBaseMesh(Scene *context) const = 0;

	/// Provides extra information about the position and orientation of the base mesh before binding.
	virtual const float* GetBindShape() const = 0;        // float4x4 row major
	virtual void SetBindShape(const float *data4x4) = 0;  // float4x4 row major

	virtual const WeightedJoint* GetWeightedJoints() const = 0;
	virtual size_t GetWeightedJointCount() const = 0;

	/// Gives write access to the vertex binding information. The framework can not access binding data while it's locked.
	virtual const VertexWeightsBinding* GetBindings() const = 0;
	virtual size_t GetBindingCount() const = 0;

	/// Gives write access to the vertex binding information. The framework can not access binding data while it's locked.
	virtual VertexWeightsBinding* LockBinding() = 0;
	virtual void UnlockBinding() = 0;
	virtual WeightedJoint* LockWeightedJoints() = 0;
	virtual void UnlockWeightedJoints() = 0;
};

///////////////////////////////////////////////////////////////////////////////

struct SkinControllerInstance : public GeometryInstance
{
	virtual SkinController* GetEntity(Scene *context) const = 0;
	virtual MemoryObject* GetJoints() const = 0; ///< @return pointer to a buffer of Joint structures
	virtual void SetJoints(MemoryObject *joints) = 0;
};


///////////////////////////////////////////////////////////////////////////////

struct MorphController : public Base
{
	virtual void SetBaseMesh(Mesh *baseMesh) = 0;
	virtual Mesh* GetBaseMesh(Scene *context) const = 0;
	virtual void SetWeights(MemoryObject *weights) = 0;
	virtual MemoryObject* GetWeights() const = 0;
	virtual void SetTargets(Mesh *const* ppMeshArray, size_t count) = 0;
	virtual size_t GetMeshCount() const = 0;
	virtual Mesh* GetTargetMesh(size_t index) = 0;
};

///////////////////////////////////////////////////////////////////////////////

struct MorphControllerInstance : public GeometryInstance
{
	virtual MorphController* GetEntity(Scene *context) const = 0;
};

///////////////////////////////////////////////////////////////////////////////

/// Declares a light source that illuminates a scene
struct Light : public Base
{
	enum LightType
	{
		UNDEFINED,
		AMBIENT, ///< Ambient light source radiates light from all directions at once.
		         ///  The intensity of an ambient light source is not attenuated.
		POINT,   ///< Point light source radiates light in all directions from a known
		         ///  location in space. The intensity of a point light source is
		         ///  attenuated as the distance to the light source increases.
		DIRECT,  ///< Directional light source radiates light in one direction from
		         ///  a known direction in space that is infinitely far away.
		         ///  The intensity of a directional light source is not attenuated.
		SPOT,    ///< A spot light source radiates light in one direction from a known location
		         ///  in space. The light radiates from the spot light source in a cone shape.
		         ///  The intensity of the light is attenuated as the radiation angle increases
		         ///  away from the direction of the light source. The intensity of a spot light
		         ///  source is attenuated as the distance to the light source increases.
		LIGHT_TYPE_COUNT,
	};

	virtual LightType GetType() const = 0;
	virtual const float* GetColor() const = 0; // float4
	virtual float GetAttenuation0() const = 0; // constant
	virtual float GetAttenuation1() const = 0; // linear
	virtual float GetAttenuation2() const = 0; // quadratic
	virtual float GetFalloffAngle() const = 0;
	virtual float GetFalloffExponent() const = 0;

	virtual void SetType(LightType type) = 0;
	virtual void SetColor(const float data[], size_t compCount) = 0;
	virtual void SetAttenuation0(float a0) = 0; // constant
	virtual void SetAttenuation1(float a1) = 0; // linear
	virtual void SetAttenuation2(float a2) = 0; // quadratic
	virtual void SetFalloffAngle(float angle) = 0;
	virtual void SetFalloffExponent(float e) = 0;
};

///////////////////////////////////////////////////////////////////////////////

/// Puts a Light object the local coordinate system of its parent node.
struct LightInstance : public Instance
{
	/// @return Pointer to instantiated Light object.
	virtual Light* GetEntity(Scene *context) const = 0;
};

///////////////////////////////////////////////////////////////////////////////

/// The camera is defined such that the local +X axis is to the right, the lens looks
/// towards the local -Z axis, and the top of the camera is aligned with the local +Y axis.
struct Camera : public Base
{
	enum ProjectionType
	{
		UNDEFINED,
		PERSPECTIVE,
		ORTHOGRAPHIC,
	};

	enum FixedValues
	{
		FIXED_XFOV_ASPECT,
		FIXED_YFOV_ASPECT,
		FIXED_XFOV_YFOV,
	};

	virtual const float* GetProj() const = 0; // float4x4 row major; TODO: move to view class

	virtual ProjectionType GetType() const = 0;
	virtual float GetScreenAspect() const = 0;     //  width : height
	virtual float GetYFov() const = 0;
	virtual float GetXFov() const = 0;
	virtual float GetAspect() const = 0;
	virtual float GetNear() const = 0;
	virtual float GetFar() const = 0;
	virtual FixedValues GetFixedValues() const = 0;

	virtual void SetType(ProjectionType type) = 0;
	virtual void SetScreenAspect(float a) = 0;     //  width : height
	virtual void SetYFov(float yfov) = 0;
	virtual void SetXFov(float yfov) = 0;
	virtual void SetAspect(float a) = 0;
	virtual void SetNear(float near) = 0;
	virtual void SetFar(float zfar) = 0;
	virtual void SetFixedValues(FixedValues fv) = 0;
};

///////////////////////////////////////////////////////////////////////////////

/// Puts a Camera object the local coordinate system of its parent node.
/// The node determines its position, orientation, and scale.
/// Use Node::GetWorldI to get view matrix for the camera.
struct CameraInstance : public Instance
{
	/// @return Pointer to instantiated Camera object.
	virtual Camera* GetEntity(Scene *context) const = 0;
};

///////////////////////////////////////////////////////////////////////////////

/// Specifies how to transform an object.
struct Transform : public AnimationTarget
{
	enum Type
	{
		LOOKAT, ///< Contains a position and orientation transformation suitable for aiming a camera.
		        ///
		        ///  Positioning and orienting a camera or object in the scene is often complicated when using a matrix.
		        ///  A lookat transform is an intuitive way to specify an eye position, interest point, and orientation.
		        ///  The transform data comprises three 3d vectors of floats that describe:
		        ///   - The position of the object.
		        ///   - The position of the interest point.
		        ///   - The direction that points up.

		SKEW,   ///< Specifies how to deform an object along one axis.
		        ///  
		        ///  Contains an angle and two mathematical vectors that represent
		        ///  the axis of rotation and the axis of translation.

		MATRIX, ///< Describes transformations that embody mathematical changes to points within a coordinate system or the
		        ///  Contains a list of 16 floating-point values organized into a 4-by-4 row-major matrix.

		ROTATE, ///< Specifies how to rotate an object around an axis.
		        ///  Contains a column vector [X,Y,Z] specifying the axis of rotation followed by an angle in degrees.

		SCALE,  ///< Specifies how to change an object’s size.
		        ///  Contains a mathematical vector that represents the relative proportions
		        ///  of the x, y, and z axes of a coordinate system.

		TRANSLATE, ///< Changes the position of an object in a local coordinate system.
		           ///  Contains a mathematical vector that represents the distance along the x, y, and z axes.
	};

	/// @return array of 16 floats describing an equivalent transform matrix in row-major order
	virtual const float* GetMatrix() const = 0;
};

/// Implement this interface to get informed on what is happening with the Node object. You may
/// pass a pointer to this interface to the Node::AddListener and Node::RemoveListener functions.
struct INodeListener
{
	/// Called by framework when noed's coordinate system is changed.
	/// This is also affected by parent's coordinate system.
	virtual void OnNodeUpdate(Node *node) = 0;
};

/// Embodies the hierarchical relationship of elements in a scene.
struct Node : public Base
{
	/// @return Set of layers the node belongs to
	virtual NodeLayers GetLayers() const = 0;

	/// Sets layers the node will belong to
	virtual void SetLayers(NodeLayers layers) = 0;

	/// Defines a point of interests in 3d space
	virtual const float* GetWorld(Scene *context) const = 0; // float4x4
	virtual const float* GetWorldI(Scene *context) const = 0; // float4x4

	/// @return Pointer to parent node or NULL if the node does not have parent.
	virtual Node* GetParent(Scene *context) const = 0;

	virtual size_t GetChildrenCount() const = 0;
	virtual Node* GetChild(size_t index, Scene *context) const = 0;

	/// Adds a new transformation to the node's transformation stack
	virtual Transform* AppendTransform(Transform::Type type, const float data[]) = 0;

	virtual void AddListener(INodeListener *listener) = 0;
	virtual void RemoveListener(INodeListener *listener) = 0;
};

///////////////////////////////////////////////////////////////////////////////

/// Animation describes the transformation of an object or value over time.
/// The framework uses the key-frame animation technique.
struct Animation : public Base
{
	virtual size_t GetDataSourceCount() const = 0;
	virtual void* LockDataSource(size_t index) = 0;
	virtual void UnlockDataSource(size_t index) = 0;

	/// The target will receive updates each time you call SetTime.
	virtual void AddTarget(Property *target, size_t samplerIdx) = 0;

	virtual float GetTimeStart() const = 0;
	virtual float GetTimeEnd() const = 0;

	virtual void SetTime(float time) = 0;
};

///////////////////////////////////////////////////////////////////////////////

/// Defines a set of animations to be executed together
struct AnimationClip : public Base
{
	/// One animation may belong so several animation clips.
	virtual void AddAnimation(Animation *animation) = 0;
	virtual void SetTimeBounds(float start, float end) = 0;
	virtual float GetLength() const = 0;
	virtual void SetTimePointer(float time) = 0;
	virtual float GetTimePointer() const = 0;
};

///////////////////////////////////////////////////////////////////////////////

struct EditableScene;
struct AccessProviderLocal;

/// Implement this interface to get informed on what is happening with the Scene object.
/// You may pass pointer to this interface to the Scene::AddListener and Scene::RemoveListener functions.
__declspec(deprecated) struct ISceneListener
{
	/// Called by Scene object when the final Release is called and the scene object is about to be destroyed.
	virtual void OnSceneDestroy(Scene *which) = 0;
};

/// Root interface where all the scene graph objects live. It give read-only access to the scene graph structure.
struct Scene : public RefCounted
{
	virtual MemoryManager* GetMemoryManager() const throw() = 0;
	virtual IMessageListener* GetMessageListener() const throw () = 0;

	/// Queries for the extended interface which may be used for modifying of the existing scene graph.
	virtual bool QueryEditableScene(EditableScene **ppResult) = 0;

	virtual void AddListener(ISceneListener *sl) = 0;
	virtual void RemoveListener(ISceneListener *sl) = 0;

	virtual Animation* GetAnimation(size_t idx) = 0;
	virtual size_t GetAnimationCount() const = 0;

	virtual AnimationClip* GetAnimationClip(size_t idx) = 0;
	virtual size_t GetAnimationClipCount() const = 0;

	virtual Effect* GetEffect(size_t idx) = 0;
	virtual size_t GetEffectCount() const = 0;

	virtual Material* GetMaterial(size_t idx) = 0;
	virtual size_t GetMaterialCount() const = 0;

	virtual Node* GetNode(size_t idx) = 0;
	virtual size_t GetNodeCount() const = 0;

	virtual Image* GetImage(size_t idx) = 0;
	virtual size_t GetImageCount() const = 0;

	virtual MeshInstance* GetMeshInstance(size_t idx) = 0;
	virtual size_t GetMeshInstanceCount() const = 0;

	virtual Mesh* GetMesh(size_t idx) = 0;
	virtual size_t GetMeshCount() const = 0;

	virtual SkinControllerInstance* GetSkinCtrlInstance(size_t idx) = 0;
	virtual size_t GetSkinCtrlInstanceCount() const = 0;

	virtual MorphControllerInstance* GetMorphCtrlInstance(size_t idx) = 0;
	virtual size_t GetMorphCtrlInstanceCount() const = 0;

	virtual SkinController* GetSkinCtrl(size_t idx) = 0;
	virtual size_t GetSkinCtrlCount() const = 0;

	virtual MorphController* GetMorphCtrl(size_t idx) = 0;
	virtual size_t GetMorphCtrlCount() const = 0;

	virtual CameraInstance* GetCameraInstance(size_t idx) = 0;
	virtual size_t GetCameraInstanceCount() const = 0;

	virtual Camera* GetCamera(size_t idx) = 0;
	virtual size_t GetCameraCount() const = 0;

	virtual size_t GetLightInstanceCount() const = 0;
	virtual LightInstance* GetLightInstance(size_t index) = 0;

	virtual size_t GetLightCount() const = 0;
	virtual Light* GetLight(size_t index) = 0;

	/// Register a new layer with the specified name. The name is for user convenience only. It is not interpreted by framework and does not have to be unique.
	/// Once registered the layer can be assigned to nodes. You may combine several layers with operator |
	virtual NodeLayers RegisterLayer(const char *layerName) = 0;
	virtual size_t GetLayerCount() const = 0;
	virtual NodeLayers GetLayer(size_t index) const = 0;
	virtual const char* GetLayerName(size_t index) const = 0;

	virtual void SetWorldUpAxis(UpAxis axis) = 0;
	virtual void SetTargetUpAxis(const float *v3right, const float *v3up, const float *v3in) = 0;
	virtual const float* GetUpAxis() const = 0; //float4x4

	virtual const float* GetSceneCenter(NodeLayers incl, NodeLayers excl, AccessProviderLocal *providerLocal) const = 0; // float3
	virtual const float* GetSceneAABB(NodeLayers incl, NodeLayers excl, AccessProviderLocal *providerLocal) const = 0; // float3

	// plug-ins support
	virtual IViewFactory* GetViewFactory(size_t id) const = 0;
	virtual size_t RegisterViewFactory(IViewFactory *pFactory, Scene *contextImmediate) = 0; ///< if contextImmediate views will be created on demand
	virtual void UnregisterViewFactory(size_t id) = 0;
	virtual IView* CreateView(Scene *context, Base *o, size_t viewId) = 0;
	virtual void ReleaseView(Base *o, size_t viewId) = 0;
	virtual Base* ResolveContext(Base *target) = 0;
	virtual Scene* GetPrevContext() const = 0;
//	virtual void AddObject(Base *obj) = 0;


	// debug
	virtual const TCHAR* GetDebugInfo() const = 0;
};


//
// editing interface
//

/// Extends Scene interface to add editing capabilities.
struct EditableScene : public Scene
{
	virtual Animation* CreateAnimation(const AnimationDataDesc dataDescs[], size_t dataDescCount,
	                                   const AnimationSamplerDesc samplerDescs[], size_t samplerDescCount) = 0;
	virtual AnimationClip* CreateAnimationClip() = 0;

	virtual Node* CreateNode(Node *parent) = 0;
	virtual Image* CreateImage() = 0;
	virtual Effect* CreateEffect() = 0;
	virtual Material* CreateMaterial() = 0;
	virtual Camera* CreateCamera() = 0;
	virtual Light* CreateLight() = 0;
	virtual EditableMesh* CreateEditableMesh(const VertexBufferDesc *bufferDescs, size_t bufferDescCount) = 0;
	virtual SkinController* CreateSkinController(size_t weightedJointCount, size_t bindingCount) = 0;
	virtual MorphController* CreateMorphController() = 0;

	virtual CameraInstance* CreateCameraInstance(Node *node, Camera *camera) = 0;
	virtual LightInstance* CreateLightInstance(Node *node, Light *light) = 0;
	virtual MeshInstance* CreateRigidMeshInstance(Node *node, Mesh *mesh) = 0;
	virtual SkinControllerInstance* CreateSkinCtrlInstance(Node *node, SkinController *skinCtrl) = 0;
	virtual MorphControllerInstance* CreateMorphCtrlInstance(Node *node, MorphController *morphCtrl) = 0;
};

///////////////////////////////////////////////////////////////////////////////

/// Create empty scene that can be filled later.
void CreateScene(EditableScene **ppResult, IMessageListener *pMsgListener = NULL, MemoryManager *pMemManager = NULL);

///////////////////////////////////////////////////////////////////////////////
} // end of namespace collada
// end of file
