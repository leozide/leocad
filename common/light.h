#pragma once

#include "object.h"
#include "lc_math.h"

#define LC_LIGHT_HIDDEN            0x0001
#define LC_LIGHT_DISABLED          0x0002
#define LC_LIGHT_POSITION_SELECTED 0x0010
#define LC_LIGHT_POSITION_FOCUSED  0x0020
#define LC_LIGHT_TARGET_SELECTED   0x0040
#define LC_LIGHT_TARGET_FOCUSED    0x0080
#define LC_LIGHT_UPVECTOR_SELECTED 0x0100
#define LC_LIGHT_UPVECTOR_FOCUSED  0x0200

#define LC_LIGHT_SELECTION_MASK    (LC_LIGHT_POSITION_SELECTED | LC_LIGHT_TARGET_SELECTED | LC_LIGHT_UPVECTOR_SELECTED)
#define LC_LIGHT_FOCUS_MASK        (LC_LIGHT_POSITION_FOCUSED | LC_LIGHT_TARGET_FOCUSED | LC_LIGHT_UPVECTOR_FOCUSED)

enum lcLightSection
{
	LC_LIGHT_SECTION_POSITION,
	LC_LIGHT_SECTION_TARGET,
	LC_LIGHT_SECTION_UPVECTOR
};

enum class lcLightType
{
	Point,
	Spot,
	Directional,
	Area,
	Count
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
	LC_LIGHT_TYPE,
	LC_LIGHT_FACTOR,
	LC_LIGHT_DIFFUSE,
	LC_LIGHT_SPECULAR,
	LC_LIGHT_EXPONENT,
	LC_LIGHT_AREA_GRID,
	LC_LIGHT_SPOT_SIZE,
	LC_LIGHT_SPOT_FALLOFF,
	LC_LIGHT_SPOT_TIGHTNESS,
	LC_LIGHT_CUTOFF,
	LC_LIGHT_USE_CUTOFF,
	LC_LIGHT_POVRAY
};

struct lcLightProperties
{
	lcVector2 mLightFactor;
	lcVector2 mAreaGrid;
	float     mLightDiffuse;
	float     mLightSpecular;
	float     mSpotExponent;
	float     mSpotCutoff;
	float     mSpotFalloff;
	float     mSpotTightness;
	float     mSpotSize;
	bool      mEnableCutoff;
	bool      mPOVRayLight;
	int       mLightShape;
};

class lcLight : public lcObject
{
public:
	lcLight(const lcVector3& Position, const lcVector3& TargetPosition, lcLightType LightType);
	virtual ~lcLight() = default;

	lcLight(const lcLight&) = delete;
	lcLight(lcLight&&) = delete;
	lcLight& operator=(const lcLight&) = delete;
	lcLight& operator=(lcLight&&) = delete;

	static QString GetLightTypeString(lcLightType LightType);

	bool IsPointLight() const
	{
		return mLightType == lcLightType::Point;
	}

	bool IsSpotlight() const
	{
		return mLightType == lcLightType::Spot;
	}

	bool IsDirectionalLight() const
	{
		return mLightType == lcLightType::Directional;
	}

	bool IsAreaLight() const
	{
		return mLightType == lcLightType::Area;
	}

	lcLightType GetLightType() const
	{
		return mLightType;
	}

	void SetLightType(lcLightType LightType);

	int GetLightShape() const
	{
		return mLightShape;
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

		case LC_LIGHT_SECTION_UPVECTOR:
			return (mState & LC_LIGHT_UPVECTOR_SELECTED) != 0;
			break;
		}
		return false;
	}

	void SetSelected(bool Selected) override
	{
		if (Selected)
		{
			switch (mLightType)
			{
			case lcLightType::Point:
				mState |= LC_LIGHT_POSITION_SELECTED;
				break;

			case lcLightType::Spot:
			case lcLightType::Directional:
				mState |= LC_LIGHT_POSITION_SELECTED | LC_LIGHT_TARGET_SELECTED;
				break;

			case lcLightType::Area:
				mState |= LC_LIGHT_POSITION_SELECTED | LC_LIGHT_TARGET_SELECTED | LC_LIGHT_UPVECTOR_SELECTED;
				break;

			case lcLightType::Count:
				break;
			}
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

		case LC_LIGHT_SECTION_UPVECTOR:
			if (Selected)
			{
				if (IsAreaLight())
					mState |= LC_LIGHT_UPVECTOR_SELECTED;
			}
			else
				mState &= ~(LC_LIGHT_UPVECTOR_SELECTED | LC_LIGHT_UPVECTOR_FOCUSED);
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

		case LC_LIGHT_SECTION_UPVECTOR:
			return (mState & LC_LIGHT_UPVECTOR_FOCUSED) != 0;
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

		case LC_LIGHT_SECTION_UPVECTOR:
			if (Focused)
			{
				if (IsAreaLight())
					mState |= LC_LIGHT_UPVECTOR_SELECTED | LC_LIGHT_UPVECTOR_FOCUSED;
			}
			else
				mState &= ~(LC_LIGHT_UPVECTOR_SELECTED | LC_LIGHT_UPVECTOR_FOCUSED);
			break;
		}
	}

	quint32 GetFocusSection() const override
	{
		if (mState & LC_LIGHT_POSITION_FOCUSED)
			return LC_LIGHT_SECTION_POSITION;

		if (!IsPointLight() && (mState & LC_LIGHT_TARGET_FOCUSED))
			return LC_LIGHT_SECTION_TARGET;

		if (IsAreaLight() && (mState & LC_LIGHT_UPVECTOR_FOCUSED))
			return LC_LIGHT_SECTION_UPVECTOR;

		return ~0U;
	}

	quint32 GetAllowedTransforms() const override
	{
		if (IsPointLight())
			return LC_OBJECT_TRANSFORM_MOVE_XYZ;

		const quint32 Section = GetFocusSection();

		if (Section == LC_LIGHT_SECTION_POSITION)
			return LC_OBJECT_TRANSFORM_MOVE_XYZ | LC_OBJECT_TRANSFORM_ROTATE_XYZ;

		if (Section == LC_LIGHT_SECTION_TARGET || Section == LC_LIGHT_SECTION_UPVECTOR)
			return LC_OBJECT_TRANSFORM_MOVE_XYZ;

		return 0;
	}

	lcMatrix33 GetRelativeRotation() const
	{
		const quint32 Section = GetFocusSection();

		if (Section == LC_LIGHT_SECTION_POSITION)
		{
			return lcMatrix33AffineInverse(lcMatrix33(mWorldLight));
		}
		else
		{
			return lcMatrix33Identity();
		}
	}

	lcVector3 GetSectionPosition(quint32 Section) const override
	{
		switch (Section)
		{
		case LC_LIGHT_SECTION_POSITION:
			return mPosition;

		case LC_LIGHT_SECTION_TARGET:
			return mTargetPosition;

		case LC_LIGHT_SECTION_UPVECTOR:
			return lcMul31(lcVector3(0, 25, 0), lcMatrix44AffineInverse(mWorldLight));
		}

		return lcVector3(0.0f, 0.0f, 0.0f);
	}

	lcVector3 GetRotationCenter() const
	{
		const quint32 Section = GetFocusSection();

		switch (Section)
		{
		case LC_LIGHT_SECTION_POSITION:
			return mPosition;

		case LC_LIGHT_SECTION_TARGET:
		case LC_LIGHT_SECTION_UPVECTOR:
			return mTargetPosition;
		}

		return mPosition;
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
	{
		return (mState & LC_LIGHT_HIDDEN) == 0;
	}

	void SetColor(const lcVector3& Color, lcStep Step, bool AddKey);

	lcVector3 GetColor() const
	{
		return mColor;
	}

	void SetCastShadow(bool CastShadow);

	bool GetCastShadow() const
	{
		return mCastShadow;
	}

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
	void Rotate(lcStep Step, bool AddKey, const lcMatrix33& RotationMatrix, const lcMatrix33& RotationFrame);
	bool Setup(int LightIndex);
	void CreateName(const lcArray<lcLight*>& Lights);
	void UpdateLight(lcStep Step, lcLightProperties Props, int Property);
	lcLightProperties GetLightProperties() const
	{
		lcLightProperties props;
		props.mLightFactor = mLightFactor;
		props.mLightDiffuse = mLightDiffuse;
		props.mLightSpecular = mLightSpecular;
		props.mSpotExponent = mSpotExponent;
		props.mSpotCutoff = mSpotCutoff;
		props.mSpotFalloff = mSpotFalloff;
		props.mSpotTightness = mSpotTightness;
		props.mSpotSize = mSpotSize;
		props.mPOVRayLight = mPOVRayLight;
		props.mEnableCutoff = mEnableCutoff;
		props.mAreaGrid = mAreaGrid;
		props.mLightShape = mLightShape;
		return props;
	}

	lcMatrix44 mWorldLight;
	lcVector3 mPosition;
	lcVector3 mTargetPosition;
	lcVector3 mUpVector;

	lcVector3 mAttenuation;
	lcVector2 mLightFactor;
	lcVector2 mAreaGrid;
	lcVector2 mAreaSize;
	bool mAngleSet;
	bool mSpotBlendSet;
	bool mSpotCutoffSet;
	bool mHeightSet;
	bool mEnableCutoff;
	bool mPOVRayLight;
	float mLightDiffuse;
	float mLightSpecular;
	float mSpotSize;
	float mSpotCutoff;
	float mSpotFalloff;
	float mSpotTightness;
	float mSpotExponent;
	float mPOVRayExponent;
	QString mName;

protected:
	void DrawPointLight(lcContext* Context) const;
	void DrawSpotLight(lcContext* Context) const;
	void DrawDirectionalLight(lcContext* Context) const;
	void DrawAreaLight(lcContext* Context) const;

	float SetupLightMatrix(lcContext* Context) const;
	void DrawSphere(lcContext* Context, float Radius) const;
	void DrawCylinder(lcContext* Context, float Radius, float Height) const;
	void DrawTarget(lcContext* Context, float TargetDistance) const;
	void DrawCone(lcContext* Context, float TargetDistance) const;

	quint32 mState;
	lcLightType mLightType;
	lcVector3 mColor = lcVector3(1.0f, 1.0f, 1.0f);
	bool mCastShadow = true;

	int mLightShape;
	lcObjectKeyArray<lcVector3> mPositionKeys;
	lcObjectKeyArray<lcVector3> mTargetPositionKeys;
	lcObjectKeyArray<lcVector3> mUpVectorKeys;
	lcObjectKeyArray<lcVector3> mColorKeys;

	lcObjectKeyArray<lcVector3> mAttenuationKeys;
	lcObjectKeyArray<lcVector2> mLightFactorKeys;
	lcObjectKeyArray<lcVector2> mAreaGridKeys;
	lcObjectKeyArray<float> mLightSpecularKeys;
	lcObjectKeyArray<float> mLightDiffuseKeys;
	lcObjectKeyArray<float> mSpotSizeKeys;
	lcObjectKeyArray<float> mSpotCutoffKeys;
	lcObjectKeyArray<float> mSpotFalloffKeys;
	lcObjectKeyArray<float> mSpotExponentKeys;
	lcObjectKeyArray<float> mSpotTightnessKeys;
};
