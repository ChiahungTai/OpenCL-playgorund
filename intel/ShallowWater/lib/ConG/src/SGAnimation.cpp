/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGAnimation.cpp

#include "stdafx.h"

#include "SGAnimation.h"

///////////////////////////////////////////////////////////////////////////////

AnimationImpl::AnimationImpl(Scene *s, const AnimationDataDesc dataDescs[], size_t dataDescCount,
                             const AnimationSamplerDesc samplerDescs[], size_t samplerDescCount)
  : BaseImpl(s)
  , m_dataDescs(dataDescCount)
  , m_samplerDescs(samplerDescs, samplerDescs + samplerDescCount)
  , m_timeStart(0)
  , m_timeEnd(0)
{
	size_t bytesRequired = 0;
	for( size_t i = 0; i < dataDescCount; ++i )
	{
		m_dataDescs[i].desc = dataDescs[i];
		m_dataDescs[i].bytesOffset = bytesRequired;
		m_dataDescs[i].locked = false;

		assert(ST_FLOAT == dataDescs[i].elementType || ST_UINT8 == dataDescs[i].elementType);
		size_t typeSize = (ST_FLOAT == dataDescs[i].elementType) ? 4 : 1;
		bytesRequired += typeSize * dataDescs[i].sampleCount * dataDescs[i].elementCount;
	}
	m_commonBuffer.resize(bytesRequired);


#ifdef _DEBUG
	for( size_t i = 0; i < samplerDescCount; ++i )
	{
		const AnimationSamplerDesc &sd = m_samplerDescs[i];

		assert(sd.in < dataDescCount);
		assert(sd.out < dataDescCount);
		assert(m_dataDescs[sd.in].desc.elementCount == 1); // time is 1D
		assert(m_dataDescs[sd.in].desc.sampleCount == m_dataDescs[sd.out].desc.sampleCount);

		if( -1 != sd.interpolation )
		{
			assert(sd.interpolation < dataDescCount);
			assert(m_dataDescs[sd.interpolation].desc.sampleCount == m_dataDescs[sd.in].desc.sampleCount);
			assert(m_dataDescs[sd.interpolation].desc.elementType == ST_UINT8);
		}
	}
#endif
}

size_t AnimationImpl::GetDataSourceCount() const
{
	return m_dataDescs.size();
}

void* AnimationImpl::LockDataSource(size_t index)
{
	assert(!m_dataDescs[index].locked);
	m_dataDescs[index].locked = true;
	return &m_commonBuffer[m_dataDescs[index].bytesOffset];
}

void AnimationImpl::UnlockDataSource(size_t index)
{
	assert(m_dataDescs[index].locked);
	m_dataDescs[index].locked = false;


	//
	// update time bounds
	//

	for( size_t samIdx = 0; samIdx < m_samplerDescs.size(); ++samIdx )
	{
		const float * const in = (float *) &m_commonBuffer[m_dataDescs[m_samplerDescs[samIdx].in].bytesOffset];
		const size_t last = m_dataDescs[m_samplerDescs[samIdx].in].desc.sampleCount - 1;
		if( samIdx )
		{
			if( m_timeStart > in[0] ) m_timeStart = in[0];
			if( m_timeEnd < in[last] ) m_timeEnd = in[last];
		}
		else
		{
			m_timeStart = in[0];
			m_timeEnd = in[last];
		}
	}
}

void AnimationImpl::AddTarget(Property *target, size_t samplerIdx)
{
	assert(target);
	assert(samplerIdx < m_samplerDescs.size());
	assert(target->GetSize() == m_dataDescs[m_samplerDescs[samplerIdx].out].desc.elementCount);

	m_targets.push_back(TargetDesc());
	m_targets.back().target = target;
	m_targets.back().samplerIdx = samplerIdx;
}

float AnimationImpl::GetTimeStart() const
{
	return m_timeStart;
}

float AnimationImpl::GetTimeEnd() const
{
	return m_timeEnd;
}

void AnimationImpl::SetTime(float time)
{
	for( size_t i = 0; i < m_targets.size(); ++i )
	{
		Sample(m_targets[i].samplerIdx, time, m_targets[i].target);
	}
}

void AnimationImpl::Sample(size_t samplerIdx, float time, Property *target)
{
	const AnimationSamplerDesc &sd = m_samplerDescs[samplerIdx];

	const size_t stride = m_dataDescs[sd.out].desc.elementCount;
	const float * const out = (float *) &m_commonBuffer[m_dataDescs[sd.out].bytesOffset];
	const float * const in = (float *) &m_commonBuffer[m_dataDescs[sd.in].bytesOffset];
	const size_t keyFrameCnt = m_dataDescs[sd.in].desc.sampleCount;

	float buffer[16];
	assert(stride <= 16);


	//
	// find key frame. input sequence must be sorted
	//

	const float *in1 = std::lower_bound(in, in + keyFrameCnt, time);
	if( in == in1 )
	{
		memcpy(buffer, out, sizeof(float) * stride);
	}
	else
	if( in + keyFrameCnt == in1 )
	{
		--in1;
		memcpy(buffer, out + (keyFrameCnt - 1) * stride, sizeof(float) * stride);
	}
	else
	{
		const float *in0(in1 - 1);
		size_t nkey = (in0 - in);
		size_t offset = nkey * stride; // key_frame_number * stride

		int interp = (-1 != sd.interpolation) ?
			((const unsigned char *) &m_commonBuffer[m_dataDescs[sd.interpolation].bytesOffset])[nkey] : INTERPOLATION_LINEAR;

		switch( interp )
		{
		case INTERPOLATION_STEP:
			for( size_t i = 0; i < stride; ++i )
			{
				buffer[i] = out[offset + i];
			}
			break;
		case INTERPOLATION_LINEAR:
		{
			assert(time >= *in0);
			float slope = (time - *in0) / (*in1 - *in0);
			for( size_t i = 0; i < stride; ++i )
			{
				buffer[i] = out[offset + i] + (out[offset + i + stride] - out[offset + i]) * slope;
			}
			break;
		}
		case INTERPOLATION_BEZIER:
		{
			assert(m_dataDescs[sd.out].desc.elementCount * 2 == m_dataDescs[sd.tangentsIn].desc.elementCount);
			assert(m_dataDescs[sd.out].desc.elementCount * 2 == m_dataDescs[sd.tangentsOut].desc.elementCount);
			const float * const itan = (float *) &m_commonBuffer[m_dataDescs[sd.tangentsIn].bytesOffset];
			const float * const otan = (float *) &m_commonBuffer[m_dataDescs[sd.tangentsOut].bytesOffset];
			for( size_t i = 0; i < stride; ++i )
			{
				float sa = 0, sb = 1;
				float c0 = otan[(offset+i)*2]*3;
				float c1 = itan[(offset+stride)*2+i]*3;
				for( int n = 0; n < 30; ++n )
				{
					float s = (sa + sb) / 2;
					float si = 1 - s;
					float t = *in0*si*si*si + c0*s*si*si + c1*s*s*si + *in1*s*s*s;
					(t < time ? sa : sb) = s;
				}
				sb = 1-sa;
				c0 = otan[(offset+i)*2+1]*3;
				c1 = itan[(offset+stride+i)*2+1]*3;
				buffer[i] = out[offset+i]*sb*sb*sb + c0*sa*sb*sb + c1*sa*sa*sb + out[offset+i+stride]*sa*sa*sa;
			}
			break;
		}
		case INTERPOLATION_CARDINAL:
			printf("cardinal interpolation not supported\n");
			break;
		case INTERPOLATION_HERMITE:
			printf("hermite interpolation not supported\n");
			break;
		case INTERPOLATION_BSPLINE:
			printf("bspline interpolation not supported\n");
			break;
		default:
			assert(false);
		}
	}

	// apply result
	target->SetValue(buffer);
}

///////////////////////////////////////////////////////////////////////////////

AnimationClipImpl::AnimationClipImpl(Scene *s)
  : BaseImpl(s)
  , m_timeStart(0)
  , m_timeEnd(0)
  , m_time(0)
{
}

void AnimationClipImpl::AddAnimation(Animation *animation)
{
	assert(animation);
	m_animations.push_back(animation);
}

void AnimationClipImpl::SetTimeBounds(float start, float end)
{
	m_timeStart = start;
	m_timeEnd = end;
}

float AnimationClipImpl::GetLength() const
{
	return m_timeEnd - m_timeStart;
}

void AnimationClipImpl::SetTimePointer(float time)
{
	if( m_time != time )
	{
		m_time = time;
		for( size_t i = 0; i < m_animations.size(); ++i )
		{
			m_animations[i]->SetTime(m_timeStart + time);
		}
	}
}

float AnimationClipImpl::GetTimePointer() const
{
	return m_time;
}


// end of file
