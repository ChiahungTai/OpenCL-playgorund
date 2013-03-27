/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGAnimation.h

#pragma once

#include "SceneGraph.h"
#include "SGImplBase.h"
using namespace collada;

///////////////////////////////////////////////////////////////////////////////

class AnimationImpl : public BaseImpl<Animation>
{
public:
	AnimationImpl(Scene *s,
		const AnimationDataDesc dataDescs[], size_t dataDescCount,
		const AnimationSamplerDesc samplerDescs[], size_t samplerDescCount);

	// Animation interface
	virtual size_t GetDataSourceCount() const;
	virtual void* LockDataSource(size_t index);
	virtual void UnlockDataSource(size_t index);
	virtual void AddTarget(Property *target, size_t samplerIdx);
	virtual float GetTimeStart() const;
	virtual float GetTimeEnd() const;
	virtual void SetTime(float time);

private:
	struct InternalBufferDesc
	{
		AnimationDataDesc desc;
		size_t bytesOffset;
		bool locked;
	};

	struct TargetDesc
	{
		Property *target;
		size_t samplerIdx;
	};

	std::vector<char> m_commonBuffer;
	std::vector<InternalBufferDesc> m_dataDescs;
	std::vector<AnimationSamplerDesc> m_samplerDescs;
	std::vector<TargetDesc> m_targets;

	float m_timeStart;
	float m_timeEnd;

	void Sample(size_t samplerIdx, float time, Property *target);
};

///////////////////////////////////////////////////////////////////////////////

class AnimationClipImpl : public BaseImpl<AnimationClip>
{
public:
	AnimationClipImpl(Scene *s);

	// AnimationClip interface
	virtual void AddAnimation(Animation *animation);
	virtual void SetTimeBounds(float start, float end);
	virtual float GetLength() const;
	virtual void SetTimePointer(float time);
	virtual float GetTimePointer() const;

private:
	std::vector<Animation*> m_animations;
	float m_timeStart;
	float m_timeEnd;
	float m_time;
};

// end of file
