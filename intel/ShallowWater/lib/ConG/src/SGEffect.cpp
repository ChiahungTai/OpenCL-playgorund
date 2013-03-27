/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGEffect.cpp

#include "stdafx.h"

#include "SGEffect.h"
#include "SGProperty.h"

///////////////////////////////////////////////////////////////////////////////

// grep "enum Channel"
const char *EffectImpl::s_semantics[CH_COUNT] = 
{
	"TEX_EMISSION",    // CH_EMISSION
	"TEX_AMBIENT",     // CH_AMBIENT
	"TEX_DIFFUSE",     // CH_DIFFUSE
	"TEX_SPECULAR",    // CH_SPECULAR
	"TEX_TRANSPARENT", // CH_TRANSPARENT
	"TEX_BUMP",        // CH_BUMP
	"NORMAL",          // CH_REFLECTION  - does not require texture coordinates
};

EffectImpl::EffectImpl(Scene *s)
  : BaseImpl(s)
  , m_shader(SHADER_UNKNOWN)
  , m_tmode(OPAQUE_NONE)
  , m_shininess(1)
  , m_transparency(1)
  , m_isInputsValid(false)
{
	for( int i = 0; i < CH_COUNT; ++i )
	{
		m_textures[i].color.SetEffect(this);
	}
	m_shininess.SetEffect(this);
	m_transparency.SetEffect(this);
}

EffectImpl::~EffectImpl()
{
}

void EffectImpl::RequireInput(const std::string &input) const
{
	if( m_inputSemantics.end() == std::find(m_inputSemantics.begin(), m_inputSemantics.end(), input) )
	{
		m_inputSemantics.push_back(input);
	}
}

OpacityMode EffectImpl::GetOpacityMode() const
{
	if( !(m_transparency.HasBoundDelegates() || m_textures[CH_TRANSPARENT].color.HasBoundDelegates()
	      || m_textures[CH_TRANSPARENT].image) )
	{
		float t = m_transparency.GetValue();
		const float *c = m_textures[CH_TRANSPARENT].color.GetColor();
		const bool rgb0 = (m_tmode == OPAQUE_RGB_ZERO);
		if( !rgb0 && 1<=c[3]*t  ||  rgb0 && 0==c[0]*t && 0==c[1]*t && 0==c[2]*t )
		{
			return OPAQUE_NONE;
		}
	}
	return m_tmode;
}

void EffectImpl::SetOpacityMode(OpacityMode mode)
{
	m_tmode = mode;
}

AnimationTarget* EffectImpl::GetAnimationTarget(Channel ch)
{
	assert(ch < CH_COUNT);
	return &m_textures[ch].color;
}

const float* EffectImpl::GetColor(Channel ch) const // float4
{
	assert(ch < CH_COUNT);
	return m_textures[ch].color.GetColor();
}

Image* EffectImpl::GetImage(Channel ch, Scene *context) const
{
	assert(ch < CH_COUNT);
	return SafeResolveContext<Image>(context, m_textures[ch].image);
}

void EffectImpl::GetSamplerState(Channel ch, TexSamplerState &state) const
{
	state = m_textures[ch].sam;
}

void EffectImpl::SetColor(Channel ch, const float *color, size_t compCount) // float4
{
	assert(ch < CH_COUNT);
	memcpy(m_textures[ch].color.GetColor(), color, compCount * sizeof(float));
}

void EffectImpl::SetImage(Channel ch, Image *img)
{
	assert(ch < CH_COUNT);
	m_textures[ch].image = img;
	m_isInputsValid = false;
}

void EffectImpl::SetSamplerState(Channel ch, const TexSamplerState &state)
{
	assert(ch < CH_COUNT);
	static_cast<TexSamplerState&>(m_textures[ch].sam) = state;
}

size_t EffectImpl::GetInputCount() const
{
	ValidateInputs();
	return m_inputSemantics.size();
}

const char* EffectImpl::GetInputSemantic(size_t index) const
{
	ValidateInputs();
	return m_inputSemantics[index].c_str();
}

Shader EffectImpl::GetShader() const
{
	return m_shader;
}

void EffectImpl::SetShader(Shader shader)
{
	assert(shader < SHADER_COUNT);
	m_shader = shader;
	m_isInputsValid = false;
}

float EffectImpl::GetShininess() const
{
	return m_shininess.GetValue();
}

void EffectImpl::SetShininess(float s)
{
	m_shininess.GetValue() = s;
}

float EffectImpl::GetTransparency() const
{
	return m_transparency.GetValue();
}

void EffectImpl::SetTransparency(float t)
{
	m_transparency.GetValue() = t;
}

AnimationTarget* EffectImpl::GetTransparencyTarget()
{
	return &m_transparency;
}

void EffectImpl::ValidateInputs() const
{
	if( !m_isInputsValid )
	{
		m_inputSemantics.clear();
		switch( m_shader )
		{
			case SHADER_LAMBERT:
			case SHADER_BLINN_TORRANCE_SPARROW:
			case SHADER_BLINN_PHONG:
			case SHADER_PHONG:
				RequireInput("NORMAL");
			case SHADER_CONSTANT:
				RequireInput("POSITION");
				break;
			default:
				assert(0); // unknown shader
		}

		for( size_t i = 0; i < CH_COUNT; ++i )
		{
			if( m_textures[i].image )
			{
				RequireInput(s_semantics[i]);
				if( CH_BUMP == i )
				{
					RequireInput("TEXTANGENT");
					RequireInput("TEXBINORMAL");
				}
			}
		}

		m_isInputsValid = true;
	}
}

///////////////////////////////////////////////////////////////////////////////

EffectImpl::EffectColorImpl::EffectColorImpl()
  : m_color(0,0,0,1)
  , m_pEffect(NULL)
{
}

EffectImpl::EffectColorImpl::~EffectColorImpl()
{
}

void EffectImpl::EffectColorImpl::OnPropertyChange()
{
	assert(m_pEffect);
}

PropertyImpl* EffectImpl::EffectColorImpl::RegisterProperty(PropertyImpl *d)
{
	m_delegates.push_back(d);
	return d;
}

Property* EffectImpl::EffectColorImpl::GetDelegate(const char *param)
{
	if( param )
	{
		if( 0 == _stricmp("R", param) )
		{
			return RegisterProperty(new PropertyImpl(this, &m_color.x, 1, "r"));
		}
		if( 0 == _stricmp("G", param) )
		{
			return RegisterProperty(new PropertyImpl(this, &m_color.y, 1, "g"));
		}
		if( 0 == _stricmp("B", param) )
		{
			return RegisterProperty(new PropertyImpl(this, &m_color.z, 1, "b"));
		}
		if( 0 == _stricmp("A", param) )
		{
			return RegisterProperty(new PropertyImpl(this, &m_color.w, 1, "a"));
		}
	}
	else
	{
		// return delegate to entire contents
		return RegisterProperty(new PropertyImpl(this, m_color, 4, ""));
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////


EffectImpl::EffectFloatImpl::EffectFloatImpl(float value)
  : m_value(value)
  , m_pEffect(NULL)
{
}

EffectImpl::EffectFloatImpl::~EffectFloatImpl()
{
}

void EffectImpl::EffectFloatImpl::OnPropertyChange()
{
	assert(m_pEffect);
}

Property* EffectImpl::EffectFloatImpl::GetDelegate(const char *param)
{
	if( !m_delegate.get() && !param )
	{
		m_delegate.reset(new PropertyImpl(this, &m_value, 1, ""));
	}
	return m_delegate.get();
}

// end of file
