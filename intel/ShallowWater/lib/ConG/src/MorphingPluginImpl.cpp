/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2010 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

// MorphingPluginImpl.cpp

#include "stdafx.h"

#include "SGMesh.h"
#include "MorphingPluginImpl.h"
#include "MorphingAlgorithm.h"
//----------------------------------------------------------------------------------------------------------------
#include "AccessProviderDefault.h"
#include "SceneUtils.h"
//----------------------------------------------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////


MorphMeshImpl::MorphMeshImpl(IMorphingAlgorithm *morphAlgorithm, MorphController *morphCtrl, AccessProviderLocal *providerLocal, Scene *context)
: MeshAdapter(morphCtrl->GetBaseMesh(context->GetPrevContext()), context)
{
	size_t meshCount = morphCtrl->GetWeights()->GetBytesSize()/sizeof(float);
	for( size_t i = 0; i < GetPrimaryVertexElementCount(); ++i )
	{
		MemoryObject *bufferBase = FindVertexBuffer(morphCtrl->GetBaseMesh(context->GetPrevContext()), GetPrimaryVertexElementDescs()[i].semanticName);
		std::vector< float3 > buffersTarget;

		for ( size_t k = 0; k < meshCount; k++ )
		{
			MemoryObject *buffersResource = FindVertexBuffer(morphCtrl->GetTargetMesh(k), GetPrimaryVertexElementDescs()[i].semanticName);
			
			AutoPtr<AccessorReadDefault> accessorRead;
			providerLocal->CreateAccessorReadDefault(buffersResource, &accessorRead);
			MapHelper<const float3, AccessorReadDefault> buffersTmp(accessorRead);
			
			for( size_t j = 0; j < buffersTmp.count; j++ )
			{
				buffersTarget.push_back(buffersTmp[j]);
			}
		}
		AutoPtr< MemoryObject > targetsBuffers;
		GetMemoryManager()->Allocate(meshCount*bufferBase->GetBytesSize(), &buffersTarget.front(), &targetsBuffers);

		buffersTarget.clear();
		
		AutoPtr< MemoryObject > buffer;
		GetMemoryManager()->Allocate(bufferBase->GetBytesSize(), NULL, &buffer);
		m_AllBuffers.push_back(buffer);
		
	
		AlgorithmResource inputs[] = 
		{
			{ IMorphingAlgorithm::IN_BASEBUFFER, bufferBase },
			{ IMorphingAlgorithm::IN_TARGETSBUFFERS, targetsBuffers },
			{ IMorphingAlgorithm::IN_WEIGHTS, morphCtrl->GetWeights() },
			{ -1, NULL } // terminator
		};
		AlgorithmResource outputs[] = 
		{
			{ IMorphingAlgorithm::OUT_BUFFER, m_AllBuffers[i] },
			{ -1, NULL } // terminator
		};
		AutoPtr<Processor>(new ProcessorSimple(morphAlgorithm, inputs, outputs));
	}

	// override batches and search normals in original batches' extra attributes
	m_batches.reserve(GetSource()->GetBatchCount());
	for( size_t batchIdx = 0; batchIdx < GetSource()->GetBatchCount(); ++batchIdx )
	{
		Batch *batchOriginal = GetSource()->GetBatch(batchIdx, context->GetPrevContext());
		m_batches.push_back(new MorphedBatchImpl(batchOriginal, this, context));
	}
}

MemoryObject* MorphMeshImpl::GetVertexBuffer(size_t bufferIdx) const
{
	if( m_AllBuffers.size() > bufferIdx )
	{
		return m_AllBuffers[bufferIdx];
	}
	return MeshAdapter::GetVertexBuffer(bufferIdx);
}

size_t MorphMeshImpl::GetBatchCount() const
{
	return m_batches.size();
}

Batch* MorphMeshImpl::GetBatch(size_t batchIdx, Scene *context) const
{
	return SafeResolveContext<Batch>(context, m_batches[batchIdx]);
}

MorphedBatchImpl::MorphedBatchImpl(Batch *srcBatch, MorphMeshImpl *mesh, Scene *context)
  : BatchAdapter(srcBatch, context)
  , m_mesh(mesh)
{
}

Mesh* MorphedBatchImpl::GetMesh(Scene *context) const
{
	return SafeResolveContext<Mesh>(context, m_mesh);
}

///////////////////////////////////////////////////////////////////////////////

MorphedMeshViewImpl::MorphedMeshViewImpl(IMorphingAlgorithm *algorithm, MorphController *morphCtrl, AccessProviderLocal *providerLocal, Scene *context)
  : m_source(morphCtrl)
  , m_mesh(new MorphMeshImpl(algorithm, morphCtrl, providerLocal, context))
{
}

Mesh* MorphedMeshViewImpl::GetMorphedMesh()
{
	return m_mesh.get();
}

void MorphedMeshViewImpl::OnDetach()
{
	delete this;
}

MorphMeshInstanceViewImpl::MorphMeshInstanceViewImpl(IMorphingAlgorithm *morphAlgorithm, MorphControllerInstance *morphCtrlInst, AccessProviderLocal *providerLocal, Scene *context)
: m_inst(new MorphedMeshInstanceImpl(morphAlgorithm, morphCtrlInst, providerLocal, context))
{
}

MeshInstance* MorphMeshInstanceViewImpl::GetMeshInstance()
{
	return m_inst.get();
}

void MorphMeshInstanceViewImpl::OnDetach()
{
	delete this;
}

///////////////////////////////////////////////////////////////////////////////

MorphedMeshInstanceImpl::MorphedMeshInstanceImpl(IMorphingAlgorithm *morphAlgorithm, MorphControllerInstance *src, AccessProviderLocal *providerLocal, Scene *context)
  : GeometryInstanceAdapter(src, context)
  , m_mesh(new MorphMeshImpl(morphAlgorithm, src->GetEntity(context), providerLocal, context))
{
}

Mesh* MorphedMeshInstanceImpl::GetEntity(Scene *context) const
{
	return SafeResolveContext<Mesh>(context, m_mesh.get());
}

///////////////////////////////////////////////////////////////////////////////

// constructor
MorphPluginFactory::MorphPluginFactory(IMorphingAlgorithm *morphAlgorithm, AccessProviderLocal *providerLocal)
  : m_morphAlgorithm(morphAlgorithm)
  , m_provider(providerLocal)
{
}

//
// IViewFactory interface implementation
//

IView* MorphPluginFactory::CreateView(Scene *context, Base *b, size_t thisViewId)
{
	if( MorphControllerInstance *morphMesh = dynamic_cast<MorphControllerInstance *>(b) )
	{
		return new MorphMeshInstanceViewImpl(m_morphAlgorithm, morphMesh, m_provider, context);
	}
	return NULL;
}

void MorphPluginFactory::OnDetach()
{
	delete this;
}

///////////////////////////////////////////////////////////////////////////////

MorphSceneAdapter::MorphSceneAdapter(IMorphingAlgorithm *morphAlgorithm, AccessProviderLocal *providerLocal, Scene *src, size_t pluginId)
  : SceneAdapter(src, pluginId, &CViewFactoryCreatorProc2<IMorphingAlgorithm*, AccessProviderLocal*>(CreateMorphPlugin, morphAlgorithm, providerLocal))
  , m_morphCtrlInstCount(src->GetMorphCtrlInstanceCount())
{
}

MeshInstance* MorphSceneAdapter::GetMeshInstance(size_t idx)
{
	if( idx < m_morphCtrlInstCount )
	{
		return GetSource()->GetMorphCtrlInstance(idx)
			->GetViewT<IMorphedMeshInstanceView>(GetPluginId(), this)->GetMeshInstance();
	}
	return GetSource()->GetMeshInstance(idx - m_morphCtrlInstCount);
}

size_t MorphSceneAdapter::GetMeshInstanceCount() const
{
	return GetSource()->GetMeshInstanceCount() + m_morphCtrlInstCount;
}

Mesh* MorphSceneAdapter::GetMesh(size_t idx)
{
	//assert(false); // TODO: implement
	//return NULL;
	
	if( idx < m_morphCtrlCount )
	{
		return GetSource()->GetMorphCtrlInstance(idx)
			->GetViewT<IMorphedMeshInstanceView>(GetPluginId(), this)->GetMeshInstance()->GetEntity(this);
	}
	return GetSource()->GetMesh(idx - m_morphCtrlCount);
}

size_t MorphSceneAdapter::GetMeshCount() const
{
	assert(false); // TODO: implement
	return 0;//GetSource()->GetMeshCount() + m_meshCount;
}

MorphControllerInstance* MorphSceneAdapter::GetMorphCtrlInstance(size_t idx)
{
	assert(false);
	return NULL;
}

size_t MorphSceneAdapter::GetMorphCtrlInstanceCount() const
{
	return 0;
}

MorphController* MorphSceneAdapter::GetMorphCtrl(size_t idx)
{
	assert(false);
	return NULL;
}

size_t MorphSceneAdapter::GetMorphCtrlCount() const
{
	return 0;
}

// factory
namespace collada
{
	IViewFactory* CreateMorphPlugin(IMorphingAlgorithm *algorithm, AccessProviderLocal *providerLocal)
	{
		return new MorphPluginFactory(algorithm, providerLocal);
	}
	void CreateMorphingSceneAdapter(Scene *source, Scene **ppResult, IMorphingAlgorithm *algorithm, AccessProviderLocal *providerLocal, size_t pluginId)
	{
		AutoPtr<Scene> result(new MorphSceneAdapter(algorithm, providerLocal, source, pluginId));
		DetachRawPtr(result, ppResult);
	}
}

// end of file