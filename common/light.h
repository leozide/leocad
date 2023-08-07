#pragma once

#include "object.h"
#include "lc_math.h"

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

enum lcLightType
{
	LC_UNDEFINED_LIGHT,
	LC_POINTLIGHT,
	LC_AREALIGHT,
	LC_SUNLIGHT,
	LC_SPOTLIGHT
};

enum lcLightShape
{
	LC_LIGHT_SHAPE_UNDEFINED = -1,
	LC_LIGHT_SHAPE_SQUARE,
	LC_LIGHT_SHAPE_DISK,
	LC_LIGHT_SHAPE_RECTANGLE,
	LC_LIGHT_SHAPE_ELLIPSE
};

enum lcLightProperty
{
	LC_LIGHT_NONE,
	LC_LIGHT_SHAPE,
	LC_LIGHT_COLOR,
	LC_LIGHT_TYPE,
	LC_LIGHT_FACTOR,
	LC_LIGHT_SPECULAR,
	LC_LIGHT_EXPONENT,
	LC_LIGHT_SPOT_SIZE,
	LC_LIGHT_CUTOFF,
	LC_LIGHT_USE_CUTOFF
};

struct lcLightProperties
{
	lcVector3 mLightColor;
	lcVector2 mLightFactor;
	float     mLightSpecular;
	float     mSpotExponent;
	float     mSpotCutoff;
	float     mSpotSize;
	bool      mEnableCutoff;
	int       mLightShape;
};

class lcLight : public lcObject
{
public:
	lcLight(float px, float py, float pz);
	lcLight(float px, float py, float pz, float tx, float ty, float tz, int LightType);
	~lcLight();

	lcLight(const lcLight&) = delete;
	lcLight(lcLight&&) = delete;
	lcLight& operator=(const lcLight&) = delete;
	lcLight& operator=(lcLight&&) = delete;

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

	bool IsSelected() const override
	{
		return (mState & LC_LIGHT_SELECTION_MASK) != 0;
	}

	bool IsSelected(quint32 Section) const override
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

	void SetSelected(bool Selected) override
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

	void SetSelected(quint32 Section, bool Selected) override
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

	bool IsFocused() const override
	{
		return (mState & LC_LIGHT_FOCUS_MASK) != 0;
	}

	bool IsFocused(quint32 Section) const override
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

	void SetFocused(quint32 Section, bool Focused) override
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

	quint32 GetFocusSection() const override
	{
		if (mState & LC_LIGHT_POSITION_FOCUSED)
			return LC_LIGHT_SECTION_POSITION;

		if (!IsPointLight() && (mState & LC_LIGHT_TARGET_FOCUSED))
			return LC_LIGHT_SECTION_TARGET;

		return ~0U;
	}

	quint32 GetAllowedTransforms() const override
	{
		return LC_OBJECT_TRANSFORM_MOVE_X | LC_OBJECT_TRANSFORM_MOVE_Y | LC_OBJECT_TRANSFORM_MOVE_Z;
	}

	lcVector3 GetSectionPosition(quint32 Section) const override
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
	bool ParseLDrawLine(QTextStream& Stream);

public:
	void RayTest(lcObjectRayTest& ObjectRayTest) const override;
	void BoxTest(lcObjectBoxTest& ObjectBoxTest) const override;
	void DrawInterface(lcContext* Context, const lcScene& Scene) const override;
	void RemoveKeyFrames() override;

	void InsertTime(lcStep Start, lcStep Time);
	void RemoveTime(lcStep Start, lcStep Time);

	bool IsVisible() const
	{ return (mState & LC_LIGHT_HIDDEN) == 0; }

	void SetName(const QString& Name)
	{
		mName = Name;
	}

	QString GetName() const override
	{
		return mName;
	}

	void CompareBoundingBox(lcVector3& Min, lcVector3& Max);
	void UpdatePosition(lcStep Step);
	void MoveSelected(lcStep Step, bool AddKey, const lcVector3& Distance);
	bool Setup(int LightIndex);
	void CreateName(const lcArray<lcLight*>& Lights);
	void UpdateLight(lcStep Step, lcLightProperties Props, int Property);
	lcLightProperties GetLightProperties() const
	{
		lcLightProperties props;
		props.mLightColor = mLightColor;
		props.mLightFactor = mLightFactor;
		props.mLightSpecular = mLightSpecular;
		props.mSpotExponent = mSpotExponent;
		props.mSpotCutoff = mSpotCutoff;
		props.mSpotSize = mSpotSize;
		props.mEnableCutoff = mEnableCutoff;
		props.mLightShape = mLightShape;
		return props;
	}

	// Temporary parameters
	lcMatrix44 mWorldLight;
	lcVector3 mPosition;
	lcVector3 mTargetPosition;
	lcVector4 mAmbientColor;
	lcVector4 mDiffuseColor;
	lcVector4 mSpecularColor;
	lcVector3 mAttenuation;
	lcVector3 mLightColor;
	lcVector2 mLightFactor;
	bool  mAngleSet;
	bool  mSpotBlendSet;
	bool  mSpotCutoffSet;
	bool  mHeightSet;
	bool  mEnableCutoff;
	int   mLightType;
	int   mLightShape;
	float mLightSpecular;
	float mSpotSize;
	float mSpotCutoff;
	float mSpotExponent;
	QString mName;

protected:
	lcObjectKeyArray<lcVector3> mPositionKeys;
	lcObjectKeyArray<lcVector3> mTargetPositionKeys;
	lcObjectKeyArray<lcVector4> mAmbientColorKeys;
	lcObjectKeyArray<lcVector4> mDiffuseColorKeys;
	lcObjectKeyArray<lcVector4> mSpecularColorKeys;
	lcObjectKeyArray<lcVector3> mAttenuationKeys;
	lcObjectKeyArray<lcVector3> mLightColorKeys;
	lcObjectKeyArray<lcVector2> mLightFactorKeys;
	lcObjectKeyArray<int> mLightTypeKeys;
	lcObjectKeyArray<int> mLightShapeKeys;
	lcObjectKeyArray<float> mLightSpecularKeys;
	lcObjectKeyArray<float> mLightSpotSizeKeys;
	lcObjectKeyArray<float> mSpotCutoffKeys;
	lcObjectKeyArray<float> mSpotExponentKeys;

	void Initialize(const lcVector3& Position, const lcVector3& TargetPosition, int LightType);

	void DrawDirectionalLight(lcContext* Context) const;
	void DrawPointLight(lcContext* Context) const;
	void DrawSpotLight(lcContext* Context) const;
	void SetLightState(int LightType);

	quint32 mState;
};
