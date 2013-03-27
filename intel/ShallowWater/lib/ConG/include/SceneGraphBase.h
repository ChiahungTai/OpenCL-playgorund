/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SceneGraphBase.h

#pragma once

#include "SSEOps.h"

#if !defined(_CRTDBG_MAP_ALLOC) && (defined(DEBUG) || defined(_DEBUG))
#define _CRTDBG_MAP_ALLOC // Enable run-time memory check for debug builds.
#endif

#ifdef  _UNICODE
	#ifndef _TCHAR_DEFINED
	typedef wchar_t TCHAR, *PTCHAR;
	typedef wchar_t TBYTE , *PTBYTE;
	#define _TCHAR_DEFINED
	#endif /* !_TCHAR_DEFINED */
#else  /* _UNICODE */
	#ifndef _TCHAR_DEFINED
	typedef char TCHAR, *PTCHAR;
	typedef unsigned char TBYTE , *PTBYTE;
	#define _TCHAR_DEFINED
	#endif /* !_TCHAR_DEFINED */
#endif /* _UNICODE */

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif


namespace collada
{
	//
	// forward declarations of scene graph interfaces
	//

	struct Property;
	struct Base;
	struct Instance;
	struct Image;
	struct Effect;
	struct Material;
	struct MaterialBinding;
	struct Batch;
	struct Mesh;
	struct GeometryInstance;
	struct EditableMesh;
	struct MeshInstance;
	struct SkinController;
	struct SkinControllerInstance;
	struct MorphController;
	struct MorphControllerInstance;
	struct Light;
	struct LightInstance;
	struct Camera;
	struct CameraInstance;
	struct Transform;
	struct Node;
	struct Animation;
	struct AnimationClip;
	struct Scene;
	struct EditableScene;

	struct MemoryObject;
	struct MemoryManager;

	struct IMessageListener;



	//
	// common scene graph data types
	//

	/// Defines set of layers. Several layer sets can be combined with operator |
	typedef unsigned long long NodeLayers;


	/// Describes the coordinate system specifying which axis is considered upward,
	/// which is considered to the right, and which is considered inward.
	/// All coordinates are righthanded by definition.
	enum UpAxis
	{
		UPAXIS_X = 0, ///< Right - negative y; Up - positive x; In - positive z.
		UPAXIS_Y = 1, ///< Right - positive x; Up - positive y; In - positive z.
		UPAXIS_Z = 2, ///< Right - positive x; Up - positive z; In - negative y.
		UPAXIS_UNKNOWN = 0x7fffffff
	};

	enum Channel
	{
		CH_EMISSION       = 0,
		CH_AMBIENT        = 1,
		CH_DIFFUSE        = 2,
		CH_SPECULAR       = 3,
		CH_TRANSPARENT    = 4,
		CH_BUMP           = 5,
		CH_REFLECTION     = 6,
		//---------------------
		CH_COUNT,
		CH_UNKNOWN = 0x7fffffff
	};

	enum OpacityMode
	{
		OPAQUE_NONE,
		OPAQUE_A_ONE,
		OPAQUE_RGB_ZERO,
		OPAQUE_COUNT
	};

	/// Texture wrapping mode
	enum TexWrapMode
	{
		SAMPLER_WRAP_MODE_NONE,
		SAMPLER_WRAP_MODE_WRAP,
		SAMPLER_WRAP_MODE_MIRROR,
		SAMPLER_WRAP_MODE_CLAMP,
		SAMPLER_WRAP_MODE_BORDER,
		SAMPLER_WRAP_MODE_COUNT
	};

	/// Describes texture filtering mode
	enum TexFilterMode
	{
		SAMPLER_FILTER_CONST,
		SAMPLER_FILTER_UNIFORM,
		SAMPLER_FILTER_VARYING,
		SAMPLER_FILTER_STATIC,
		SAMPLER_FILTER_VOLATILE,
		SAMPLER_FILTER_EXTERN,
		SAMPLER_FILTER_SHARED,
	};

	/// Defines shading model
	enum Shader
	{
		SHADER_CONSTANT, ///< Produces a constantly shaded surface that is independent of lighting.
		SHADER_LAMBERT, ///< Produces a diffuse shaded surface.
		SHADER_PHONG, ///< Produces a specularly shaded surface where the specular reflection is shaded according the Phong BRDF approximation.
		SHADER_BLINN_PHONG,
		SHADER_BLINN_TORRANCE_SPARROW,
		//--------------------
		SHADER_COUNT,
		SHADER_UNKNOWN = 0x7fffffff
	};

	enum ScalarType
	{
		ST_FLOAT,   ///< 32 bit floating-point number
		ST_HALF,    ///< 16 bit floating-point number
		ST_SINT32,  ///< 32 bit signed integer
		ST_UINT32,  ///< 32 bit unsigned integer
		ST_SINT16,  ///< 16 bit signed integer
		ST_UINT16,  ///< 16 bit unsigned integer
		ST_SINT8,   ///< 8 bit signed integer
		ST_UINT8,   ///< 8 bit unsigned integer
		//-----------------
		ST_COUNT,
		ST_UNKNOWN = 0x7fffffff,
	};

	/// Defines interpolation type used within an animation sampler.
	enum Interpolation
	{
		INTERPOLATION_STEP,
		INTERPOLATION_LINEAR,
		INTERPOLATION_BEZIER,
		INTERPOLATION_CARDINAL,
		INTERPOLATION_HERMITE,
		INTERPOLATION_BSPLINE,
		//----------------
		INTERPOLATION_COUNT
	};

	/// Defines the primitive topology of a Batch
	enum PrimitiveTopology
	{
		PRIMITIVE_TOPOLOGY_UNDEFINED,
		PRIMITIVE_TOPOLOGY_LINELIST, ///< Each pair of vertices make up a line segment.
		PRIMITIVE_TOPOLOGY_TRIANGLELIST, ///< Each triple of vertices make up a triangle.
		PRIMITIVE_TOPOLOGY_POLYLIST, ///< A number of vertices make up a polygon w/o holes.
		//----------------
		PRIMITIVE_TOPOLOGY_COUNT
	};

	struct TexSamplerState
	{
		TexWrapMode wrapS; ///< Defines how a texture wraps along S direction.
		TexWrapMode wrapT; ///< Defines how a texture wraps along T direction.
		TexFilterMode filter; // Defines texture filtering mode.
	};

	struct AnimationDataDesc
	{
		unsigned int sampleCount;        ///< total number of samples in the animation curve
		unsigned int elementCount;       ///< sample element dimension
		ScalarType elementType;    ///< ST_FLOAT for numeric data or ST_UINT8 for interpolation data.
	};

	/// Describes a sampler. An Animation may host one or more samplers.
	struct AnimationSamplerDesc
	{
		size_t interpolation;  ///< index of the AnimationDataDesc describing the data source of ST_UINT8 elements.
		size_t in;
		size_t out;
		size_t tangentsIn;
		size_t tangentsOut;
	};

	/// Comprises joint and its weight information.
	struct WeightedJoint
	{
		unsigned int jointIdx; ///< Index of the node. Actual pointers to nodes are stored within a SkinControllerInstance object.
		float weight; ///< The sum of joints' weights of a particular vertex should be equal to 1.
	};

	/// Binds a set of joints to a vertex
	struct VertexWeightsBinding
	{
		unsigned int begin; ///< Starting index in the array of WeightedJoint structures.
		unsigned int end;   ///< 
	};

	/// Describes a joint which is a combination of a Node and a bind matrix.
	struct Joint
	{
		Node *node; ///< FIXME: get rid of poiner here
		float invBindMatrix[4][4];
//		SSEUnaligned invBindMatrixU;
	};

	/// Describes a single vertex element such as normal or position.
	/// Only non-indexed primitive such as point cloud may use this structure directly.
	/// Indexed primitives should use VertexElementDescEx
	struct VertexElementDesc
	{
		ScalarType componentType; ///< Type of scalar component.
		unsigned int componentCount; ///< Number of scalar components or vector element dimension.
		unsigned int inputSlot; ///< Index of a vertex buffer where actual is stored.
		const char* semanticName; ///< Pointer to a buffer containing string semantic name.
		                          ///  There are several valid semantics defined:
		                          ///    POSITION
		                          ///    NORMAL
		                          ///    COLOR
		                          ///    TEXCOORD
		                          ///    TEXNORMAL
		                          ///    TEXBINORMAL
	};

	/// Adds index information to VertexElementDesc
	struct VertexElementDescEx : public VertexElementDesc
	{
		unsigned int indexOffset; ///< Offset within index buffer stride.
		unsigned int semanticIndex; /// When multiple vertex elements share the same semantic, their semanticIndex must have different.
	};

	/// Describes a vertex buffer
	struct VertexBufferDesc
	{
		unsigned int bytesStride;  ///< number of bytes per element
	};


	//
	// plug-ins support
	//

	/// Implement this interface to attach custom data to any scene graph object.
	/// Because the framework is not aware of the nature of you implementation you must
	/// provide implementation of IViewFactory interface.
	struct IView
	{
		/// Called when the framework is going to detach your from the object.
		virtual void OnDetach() = 0;
	};

	/// The framework uses this interface to create custom IView interfaces. You must provide
	/// your own implementation of IViewFactory for each set of your IView's
	struct IViewFactory
	{
		/// @return Pointer to you implementation of IView interface which may be specific to
		///         scene graph object type.
		virtual IView* CreateView(Scene *context, Base *b, size_t thisViewId) = 0;

		/// Called when the framework is going to detach your factory from the scene graph.
		virtual void OnDetach() = 0;
	};


	/// Reserved for future use
	struct IClockListener
	{
		virtual void Tick(int frame, double time, float dt) = 0;
	};

	/// Reserved for future use
	struct IClock
	{
	//	virtual float GetFrameRate() = 0; // 0 means floating frame rate
		virtual double GetTime() = 0; ///< @return current time in seconds
		virtual int GetFrameNumber() = 0; ///< @return current frame number

		virtual void AddListener(IClockListener *ls) = 0;
		virtual void RemoveListener(IClockListener *ls) = 0;
	};
}


// end of file
