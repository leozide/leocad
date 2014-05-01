#ifndef _LIGHT_H_
#define _LIGHT_H_

#include "object.h"
#include "lc_math.h"

class Light;
class LightTarget;
class View;

#define LC_LIGHT_HIDDEN            0x0001
#define LC_LIGHT_DISABLED          0x0002
#define LC_LIGHT_SPOT              0x0004
#define LC_LIGHT_DIRECTIONAL       0x0008
#define LC_LIGHT_POSITION_SELECTED 0x0010
#define LC_LIGHT_POSITION_FOCUSED  0x0020
#define LC_LIGHT_TARGET_SELECTED   0x0040
#define LC_LIGHT_TARGET_FOCUSED    0x0080

#define LC_LIGHT_SELECTION_MASK    (LC_LIGHT_POSITION_SELECTED | LC_LIGHT_TARGET_SELECTED)
#define LC_LIGHT_FOCUS_MASK        (LC_LIGHT_POSITION_FOCUSED | LC_LIGHT_TARGET_FOCUSED)

enum lcLightSection
{
	LC_LIGHT_SECTION_POSITION,
	LC_LIGHT_SECTION_TARGET
};

enum LC_LK_TYPES
{
	LC_LK_POSITION,
	LC_LK_TARGET,
	LC_LK_AMBIENT_COLOR,
	LC_LK_DIFFUSE_COLOR,
	LC_LK_SPECULAR_COLOR,
	LC_LK_CONSTANT_ATTENUATION,
	LC_LK_LINEAR_ATTENUATION,
	LC_LK_QUADRATIC_ATTENUATION,
	LC_LK_SPOT_CUTOFF,
	LC_LK_SPOT_EXPONENT,
	LC_LK_COUNT
};

class Light : public Object
{
public:
	Light(float px, float py, float pz);
	Light(float px, float py, float pz, float tx, float ty, float tz);
	virtual ~Light();

	bool IsPointLight() const
	{
		return (mState & (LC_LIGHT_SPOT | LC_LIGHT_DIRECTIONAL)) == 0;
	}

	bool IsSpotLight() const
	{
		return (mState & LC_LIGHT_SPOT) != 0;
	}

	bool IsDirectionalLight() const
	{
		return (mState & LC_LIGHT_DIRECTIONAL) != 0;
	}

	virtual bool IsSelected() const
	{
		return (mState & LC_LIGHT_SELECTION_MASK) != 0;
	}

	virtual bool IsSelected(lcuintptr Section) const
	{
		switch (Section)
		{
		case LC_LIGHT_SECTION_POSITION:
			return (mState & LC_LIGHT_POSITION_SELECTED) != 0;
			break;

		case LC_LIGHT_SECTION_TARGET:
			return (mState & LC_LIGHT_TARGET_SELECTED) != 0;
			break;
		}
		return false;
	}

	virtual void SetSelected(bool Selected)
	{
		if (Selected)
		{
			if (IsPointLight())
				mState |= LC_LIGHT_POSITION_SELECTED;
			else
				mState |= LC_LIGHT_SELECTION_MASK;
		}
		else
			mState &= ~(LC_LIGHT_SELECTION_MASK | LC_LIGHT_FOCUS_MASK);
	}

	virtual void SetSelected(lcuintptr Section, bool Selected)
	{
		switch (Section)
		{
		case LC_LIGHT_SECTION_POSITION:
			if (Selected)
				mState |= LC_LIGHT_POSITION_SELECTED;
			else
				mState &= ~(LC_LIGHT_POSITION_SELECTED | LC_LIGHT_POSITION_FOCUSED);
			break;

		case LC_LIGHT_SECTION_TARGET:
			if (Selected)
			{
				if (!IsPointLight())
					mState |= LC_LIGHT_TARGET_SELECTED;
			}
			else
				mState &= ~(LC_LIGHT_TARGET_SELECTED | LC_LIGHT_TARGET_FOCUSED);
			break;
		}
	}

	virtual bool IsFocused() const
	{
		return (mState & LC_LIGHT_FOCUS_MASK) != 0;
	}

	virtual bool IsFocused(lcuintptr Section) const
	{
		switch (Section)
		{
		case LC_LIGHT_SECTION_POSITION:
			return (mState & LC_LIGHT_POSITION_FOCUSED) != 0;
			break;

		case LC_LIGHT_SECTION_TARGET:
			return (mState & LC_LIGHT_TARGET_FOCUSED) != 0;
			break;
		}
		return false;
	}

	virtual void SetFocused(lcuintptr Section, bool Focused)
	{
		switch (Section)
		{
		case LC_LIGHT_SECTION_POSITION:
			if (Focused)
				mState |= LC_LIGHT_POSITION_SELECTED | LC_LIGHT_POSITION_FOCUSED;
			else
				mState &= ~(LC_LIGHT_POSITION_SELECTED | LC_LIGHT_POSITION_FOCUSED);
			break;

		case LC_LIGHT_SECTION_TARGET:
			if (Focused)
			{
				if (!IsPointLight())
					mState |= LC_LIGHT_TARGET_SELECTED | LC_LIGHT_TARGET_FOCUSED;
			}
			else
				mState &= ~(LC_LIGHT_TARGET_SELECTED | LC_LIGHT_TARGET_FOCUSED);
			break;
		}
	}

	virtual lcuintptr GetFocusSection() const
	{
		if (mState & LC_LIGHT_POSITION_FOCUSED)
			return LC_LIGHT_SECTION_POSITION;

		if (!IsPointLight() && (mState & LC_LIGHT_TARGET_FOCUSED))
			return LC_LIGHT_SECTION_TARGET;

		return ~0;
	}

	virtual lcVector3 GetSectionPosition(lcuintptr Section) const
	{
		switch (Section)
		{
		case LC_LIGHT_SECTION_POSITION:
			return mPosition;

		case LC_LIGHT_SECTION_TARGET:
			return mTargetPosition;
		}

		return lcVector3(0.0f, 0.0f, 0.0f);
	}

public:
	virtual void RayTest(lcObjectRayTest& ObjectRayTest) const;
	virtual void BoxTest(lcObjectBoxTest& ObjectBoxTest) const;

	bool IsVisible() const
	{ return (mState & LC_LIGHT_HIDDEN) == 0; }

	const char* GetName()
	{ return m_strName; }

	const char* GetName() const
	{ return m_strName; }

	void Render(View* View);
	void RenderCone(const lcMatrix44& ViewMatrix);
	void RenderTarget();
	void RenderSphere();

	void UpdatePosition(unsigned short nTime);
	void Move(unsigned short nTime, bool bAddKey, float dx, float dy, float dz);
	bool Setup(int LightIndex);
	void CreateName(const lcArray<Light*>& Lights);

	// Temporary parameters
	lcMatrix44 mWorldLight;
	lcVector3 mPosition;
	lcVector3 mTargetPosition;
	lcVector4 mAmbientColor;
	lcVector4 mDiffuseColor;
	lcVector4 mSpecularColor;
	float mConstantAttenuation;
	float mLinearAttenuation;
	float mQuadraticAttenuation;
	float mSpotCutoff;
	float mSpotExponent;

protected:
	void Initialize();

	float m_fCone;
	lcuint32 mState;
	char m_strName[81];
};

#endif // _LIGHT_H_
