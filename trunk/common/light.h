#ifndef _LIGHT_H_
#define _LIGHT_H_

#include "object.h"
#include "lc_math.h"

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

class lcLight : public lcObject
{
public:
	lcLight(float px, float py, float pz);
	lcLight(float px, float py, float pz, float tx, float ty, float tz);
	virtual ~lcLight();

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

	virtual bool IsSelected(lcuint32 Section) const
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

	virtual void SetSelected(lcuint32 Section, bool Selected)
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

	virtual bool IsFocused(lcuint32 Section) const
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

	virtual void SetFocused(lcuint32 Section, bool Focused)
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

	virtual lcuint32 GetFocusSection() const
	{
		if (mState & LC_LIGHT_POSITION_FOCUSED)
			return LC_LIGHT_SECTION_POSITION;

		if (!IsPointLight() && (mState & LC_LIGHT_TARGET_FOCUSED))
			return LC_LIGHT_SECTION_TARGET;

		return ~0;
	}

	virtual lcVector3 GetSectionPosition(lcuint32 Section) const
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

	void SaveLDraw(QTextStream& Stream) const;

public:
	virtual void RayTest(lcObjectRayTest& ObjectRayTest) const;
	virtual void BoxTest(lcObjectBoxTest& ObjectBoxTest) const;
	virtual void DrawInterface(lcContext* Context, const lcMatrix44& ViewMatrix) const;

	void InsertTime(lcStep Start, lcStep Time);
	void RemoveTime(lcStep Start, lcStep Time);

	bool IsVisible() const
	{ return (mState & LC_LIGHT_HIDDEN) == 0; }

	const char* GetName()
	{ return m_strName; }

	const char* GetName() const
	{ return m_strName; }

	void RenderCone(const lcMatrix44& ViewMatrix) const;
	void RenderTarget() const;
	void RenderSphere() const;

	void CompareBoundingBox(float box[6]);
	void UpdatePosition(lcStep Step);
	void Move(lcStep Step, bool AddKey, const lcVector3& Distance);
	bool Setup(int LightIndex);
	void CreateName(const lcArray<lcLight*>& Lights);

	// Temporary parameters
	lcMatrix44 mWorldLight;
	lcVector3 mPosition;
	lcVector3 mTargetPosition;
	lcVector4 mAmbientColor;
	lcVector4 mDiffuseColor;
	lcVector4 mSpecularColor;
	lcVector3 mAttenuation;
	float mSpotCutoff;
	float mSpotExponent;

protected:
	lcArray<lcObjectKey<lcVector3>> mPositionKeys;
	lcArray<lcObjectKey<lcVector3>> mTargetPositionKeys;
	lcArray<lcObjectKey<lcVector4>> mAmbientColorKeys;
	lcArray<lcObjectKey<lcVector4>> mDiffuseColorKeys;
	lcArray<lcObjectKey<lcVector4>> mSpecularColorKeys;
	lcArray<lcObjectKey<lcVector3>> mAttenuationKeys;
	lcArray<lcObjectKey<float>> mSpotCutoffKeys;
	lcArray<lcObjectKey<float>> mSpotExponentKeys;

	void Initialize(const lcVector3& Position, const lcVector3& TargetPosition);

	float m_fCone;
	lcuint32 mState;
	char m_strName[81];
};

#endif // _LIGHT_H_
