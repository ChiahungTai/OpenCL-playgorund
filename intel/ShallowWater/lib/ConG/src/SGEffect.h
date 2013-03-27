/* ************************************************************************* *\
                  INTEL CORPORATION PROPRIETARY INFORMATION
     This software is supplied under the terms of a license agreement or
     nondisclosure agreement with Intel Corporation and may not be copied
     or disclosed except in accordance with the terms of that agreement.
          Copyright (C) 2008 Intel Corporation. All Rights Reserved.
\* ************************************************************************* */

/// @file SGEffect.h

#pragma once

#include "mathstub.h"

#include "SceneGraph.h"
#include "SGImplBase.h"

#include <utility>
#include <string>

using namespace collada;

// forward declarations
class PropertyImpl;

///////////////////////////////////////////////////////////////////////////////

class EffectImpl : public BaseImpl<Effect>
{
public:
	EffectImpl(Scene *s);
	virtual ~EffectImpl();

	// effect interface
	virtual Shader GetShader() const;
	virtual void SetShader(Shader shader);

	virtual size_t GetInputCount() const;
	virtual const char* GetInputSemantic(size_t index) const;

	virtual OpacityMode GetOpacityMode() const;
	virtual void SetOpacityMode(OpacityMode mode);

	virtual AnimationTarget* GetAnimationTarget(Channel ch);
	virtual const float* GetColor(Channel ch) const; // float4
	virtual Image* GetImage(Channel ch, Scene *context) const;
	virtual void GetSamplerState(Channel ch, TexSamplerState &state) const;

	virtual void SetColor(Channel ch, const float *color, size_t compCount); // float4
	virtual void SetImage(Channel ch, Image *img);
	virtual void SetSamplerState(Channel ch, const TexSamplerState &state);


	// workaround
	virtual float GetShininess() const;
	virtual void SetShininess(float s);
	virtual float GetTransparency() const;
	virtual void SetTransparency(float t);
	virtual AnimationTarget* GetTransparencyTarget();

private:
	class EffectColorImpl
		: public AnimationTarget
		, public PropertyListener
	{
	public:
		EffectColorImpl();
		~EffectColorImpl();

		void SetEffect(EffectImpl *pEffect) { m_pEffect = pEffect; }
		const float* GetColor() const { return m_color; }
		float* GetColor() { return m_color; }

		// PropertyListener interface implementation
		virtual void OnPropertyChange(); // called by delegate when some parameter is changed

		// AnimationTarget interface implementation
		virtual Property* GetDelegate(const char *param);
		virtual bool HasBoundDelegates() const { return !m_delegates.empty(); }

	protected:
		PropertyImpl* RegisterProperty(PropertyImpl *d);

	private:
		float4 m_color;
		EffectImpl * m_pEffect;
		PtrOwnerArray<PropertyImpl> m_delegates;
	};

	///////////////////////////////////////////////////////////////////////////////

	class EffectFloatImpl
		: public AnimationTarget
		, public PropertyListener
	{
	public:
		EffectFloatImpl(float value = 0);
		~EffectFloatImpl();

		void SetEffect(EffectImpl *pEffect) { m_pEffect = pEffect; }
		float GetValue() const { return m_value; }
		float& GetValue() { return m_value; }

		// PropertyListener interface implementation
		virtual void OnPropertyChange(); // called by delegate when some parameter is changed

		// AnimationTarget interface implementation
		virtual Property* GetDelegate(const char *param);
		virtual bool HasBoundDelegates() const { return NULL != m_delegate.get(); }

	private:
		float m_value;
		EffectImpl *m_pEffect;
		std::auto_ptr<PropertyImpl> m_delegate;
	};


	Shader m_shader;
	OpacityMode m_tmode;
	EffectFloatImpl m_shininess;
	EffectFloatImpl m_transparency;

	struct MySamplerState : public TexSamplerState
	{
		MySamplerState()
		{
			wrapS = SAMPLER_WRAP_MODE_WRAP;
			wrapT = SAMPLER_WRAP_MODE_WRAP;
//			filter = ;
		}
	};

	struct TextureSlot
	{
		EffectColorImpl color;
		MySamplerState sam;
		Image *image;

		TextureSlot()
		  : image(NULL)
		{}
	};

	TextureSlot m_textures[CH_COUNT];

	mutable std::vector<std::string> m_inputSemantics;
	mutable bool m_isInputsValid;

	void RequireInput(const std::string &input) const;
	void ValidateInputs() const;

	static const char *s_semantics[CH_COUNT];
};

// end of file
