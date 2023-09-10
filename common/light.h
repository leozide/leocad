#pragma once

#include "object.h"
#include "lc_math.h"

#define LC_LIGHT_HIDDEN            0x0001
#define LC_LIGHT_DISABLED          0x0002
#define LC_LIGHT_POSITION_SELECTED 0x0010
#define LC_LIGHT_POSITION_FOCUSED  0x0020

#define LC_LIGHT_SELECTION_MASK    LC_LIGHT_POSITION_SELECTED
#define LC_LIGHT_FOCUS_MASK        LC_LIGHT_POSITION_FOCUSED

enum lcLightSection
{
	LC_LIGHT_SECTION_POSITION
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
	bool      mEnableCutoff;
	bool      mPOVRayLight;
	int       mLightShape;
};

class lcLight : public lcObject
{
public:
	lcLight(const lcVector3& Position, lcLightType LightType);
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

	bool IsSpotLight() const
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
		}

		return false;
	}

	void SetSelected(bool Selected) override
	{
		if (Selected)
			mState |= LC_LIGHT_POSITION_SELECTED;
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
		}
	}

	quint32 GetFocusSection() const override
	{
		if (mState & LC_LIGHT_POSITION_FOCUSED)
			return LC_LIGHT_SECTION_POSITION;

		return ~0U;
	}

	quint32 GetAllowedTransforms() const override
	{
		if (IsPointLight())
			return LC_OBJECT_TRANSFORM_MOVE_XYZ;

		const quint32 Section = GetFocusSection();

		if (Section == LC_LIGHT_SECTION_POSITION)
			return LC_OBJECT_TRANSFORM_MOVE_XYZ | LC_OBJECT_TRANSFORM_ROTATE_XYZ;

		return 0;
	}

	lcMatrix33 GetRelativeRotation() const
	{
		const quint32 Section = GetFocusSection();

		if (Section == LC_LIGHT_SECTION_POSITION)
			return lcMatrix33(mWorldMatrix);
		else
			return lcMatrix33Identity();
	}

	lcVector3 GetSectionPosition(quint32 Section) const override
	{
		Q_UNUSED(Section);

		return mWorldMatrix.GetTranslation();
	}

	void SetPosition(const lcVector3& Position, lcStep Step, bool AddKey)
	{
		mPositionKeys.ChangeKey(Position, Step, AddKey);
	}

	void SetRotation(const lcMatrix33& Rotation, lcStep Step, bool AddKey)
	{
		mRotationKeys.ChangeKey(Rotation, Step, AddKey);
	}

	lcVector3 GetRotationCenter() const
	{
		return mWorldMatrix.GetTranslation();
	}

	lcVector3 GetPosition() const
	{
		return mWorldMatrix.GetTranslation();
	}

	lcVector3 GetDirection() const
	{
		return -lcVector3(mWorldMatrix[2]);
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

	void SetSpotConeAngle(float Angle, lcStep Step, bool AddKey);

	float GetSpotConeAngle() const
	{
		return mSpotConeAngle;
	}

	void SetSpotPenumbraAngle(float Angle, lcStep Step, bool AddKey);

	float GetSpotPenumbraAngle() const
	{
		return mSpotPenumbraAngle;
	}

	void SetSpotTightness(float Angle, lcStep Step, bool AddKey);

	float GetSpotTightness() const
	{
		return mSpotTightness;
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
	void Rotate(lcStep Step, bool AddKey, const lcMatrix33& RotationMatrix, const lcVector3& Center, const lcMatrix33& RotationFrame);
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
		props.mPOVRayLight = mPOVRayLight;
		props.mEnableCutoff = mEnableCutoff;
		props.mAreaGrid = mAreaGrid;
		props.mLightShape = mLightShape;
		return props;
	}

	lcMatrix44 mWorldMatrix;

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
	float mSpotCutoff;
	float mSpotExponent;
	float mPOVRayExponent;
	QString mName;

protected:
	void DrawPointLight(lcContext* Context) const;
	void DrawSpotLight(lcContext* Context) const;
	void DrawDirectionalLight(lcContext* Context) const;
	void DrawAreaLight(lcContext* Context) const;

	void SetupLightMatrix(lcContext* Context) const;
	void DrawSphere(lcContext* Context, float Radius) const;
	void DrawCylinder(lcContext* Context, float Radius, float Height) const;
	void DrawTarget(lcContext* Context, float TargetDistance) const;
	void DrawCone(lcContext* Context, float TargetDistance) const;

	quint32 mState = 0;
	lcLightType mLightType;
	bool mCastShadow = true;
	lcVector3 mColor = lcVector3(1.0f, 1.0f, 1.0f);
	float mSpotConeAngle = 80.0f;
	float mSpotPenumbraAngle = 0.0f;
	float mSpotTightness = 0.0f;

	int mLightShape;
	lcObjectKeyArray<lcVector3> mPositionKeys;
	lcObjectKeyArray<lcMatrix33> mRotationKeys;
	lcObjectKeyArray<lcVector3> mColorKeys;
	lcObjectKeyArray<float> mSpotConeAngleKeys;
	lcObjectKeyArray<float> mSpotPenumbraAngleKeys;
	lcObjectKeyArray<float> mSpotTightnessKeys;

	lcObjectKeyArray<lcVector3> mAttenuationKeys;
	lcObjectKeyArray<lcVector2> mLightFactorKeys;
	lcObjectKeyArray<lcVector2> mAreaGridKeys;
	lcObjectKeyArray<float> mLightSpecularKeys;
	lcObjectKeyArray<float> mLightDiffuseKeys;
	lcObjectKeyArray<float> mSpotCutoffKeys;
	lcObjectKeyArray<float> mSpotExponentKeys;
};
