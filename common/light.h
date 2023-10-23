#pragma once

#include "object.h"
#include "lc_math.h"

#define LC_LIGHT_HIDDEN            0x0001
#define LC_LIGHT_DISABLED          0x0002

enum lcLightSection : quint32
{
	LC_LIGHT_SECTION_INVALID = ~0U,
	LC_LIGHT_SECTION_POSITION = 0,
	LC_LIGHT_SECTION_TARGET
};

enum class lcLightType
{
	Point,
	Spot,
	Directional,
	Area,
	Count
};

enum class lcLightAreaShape
{
	Rectangle,
	Square,
	Disk,
	Ellipse,
	Count
};

enum lcLightProperty
{
	LC_LIGHT_DIFFUSE,
	LC_LIGHT_SPECULAR,
	LC_LIGHT_EXPONENT,
	LC_LIGHT_CUTOFF,
	LC_LIGHT_USE_CUTOFF,
	LC_LIGHT_POVRAY
};

struct lcLightProperties
{
	float     mLightDiffuse;
	float     mLightSpecular;
	float     mSpotExponent;
	float     mSpotCutoff;
	bool      mEnableCutoff;
	bool      mPOVRayLight;
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
	static QString GetAreaShapeString(lcLightAreaShape LightAreaShape);

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

	bool SetLightType(lcLightType LightType);

	bool IsSelected() const override
	{
		return mSelected;
	}

	bool IsSelected(quint32 Section) const override
	{
		Q_UNUSED(Section);

		return mSelected;
	}

	void SetSelected(bool Selected) override
	{
		mSelected = Selected;

		if (!Selected)
			mFocusedSection = LC_LIGHT_SECTION_INVALID;
	}

	void SetSelected(quint32 Section, bool Selected) override
	{
		Q_UNUSED(Section);

		mSelected = Selected;

		if (!Selected)
			mFocusedSection = LC_LIGHT_SECTION_INVALID;
	}

	bool IsFocused() const override
	{
		return mFocusedSection != LC_LIGHT_SECTION_INVALID;
	}

	bool IsFocused(quint32 Section) const override
	{
		return mFocusedSection == Section;
	}

	void SetFocused(quint32 Section, bool Focused) override
	{
		if (Focused)
		{
			mFocusedSection = Section;
			mSelected = true;
		}
		else
			mFocusedSection = LC_LIGHT_SECTION_INVALID;
	}

	quint32 GetFocusSection() const override
	{
		return mFocusedSection;
	}

	quint32 GetAllowedTransforms() const override
	{
		if (IsPointLight())
			return LC_OBJECT_TRANSFORM_MOVE_XYZ;

		const quint32 Section = GetFocusSection();

		if (Section == LC_LIGHT_SECTION_POSITION || Section == LC_LIGHT_SECTION_INVALID)
			return LC_OBJECT_TRANSFORM_MOVE_XYZ | LC_OBJECT_TRANSFORM_ROTATE_XYZ;

		if (Section == LC_LIGHT_SECTION_TARGET)
			return LC_OBJECT_TRANSFORM_MOVE_XYZ;

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
		if (Section == LC_LIGHT_SECTION_POSITION)
			return mWorldMatrix.GetTranslation();

		if (Section == LC_LIGHT_SECTION_TARGET)
			return lcMul31(lcVector3(0.0f, 0.0f, -mTargetDistance), mWorldMatrix);

		return lcVector3(0.0f, 0.0f, 0.0f);
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
		const quint32 Section = GetFocusSection();

		if (Section == LC_LIGHT_SECTION_POSITION || Section == LC_LIGHT_SECTION_INVALID)
		{
			return mWorldMatrix.GetTranslation();
		}
		else
		{
			return lcMul31(lcVector3(0.0f, 0.0f, -mTargetDistance), mWorldMatrix);
		}
	}

	lcVector3 GetPosition() const
	{
		return mWorldMatrix.GetTranslation();
	}

	lcVector3 GetDirection() const
	{
		return -lcVector3(mWorldMatrix[2]);
	}

	const lcMatrix44& GetWorldMatrix() const
	{
		return mWorldMatrix;
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

	bool SetAreaShape(lcLightAreaShape LightAreaShape);

	lcLightAreaShape GetAreaShape() const
	{
		return mAreaShape;
	}

	void SetSize(lcVector2 Size, lcStep Step, bool AddKey);

	lcVector2 GetSize() const
	{
		return mSize;
	}

	void SetPower(float Power, lcStep Step, bool AddKey);

	float GetPower() const
	{
		return mPower;
	}

	bool SetCastShadow(bool CastShadow);

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
	void MoveSelected(lcStep Step, bool AddKey, const lcVector3& Distance, bool FirstMove);
	void Rotate(lcStep Step, bool AddKey, const lcMatrix33& RotationMatrix, const lcVector3& Center, const lcMatrix33& RotationFrame);
	bool Setup(int LightIndex);
	void CreateName(const lcArray<lcLight*>& Lights);
	void UpdateLight(lcStep Step, lcLightProperties Props, int Property);

	lcLightProperties GetLightProperties() const
	{
		lcLightProperties props;
		props.mLightDiffuse = mLightDiffuse;
		props.mLightSpecular = mLightSpecular;
		props.mSpotExponent = mSpotExponent;
		props.mSpotCutoff = mSpotCutoff;
		props.mPOVRayLight = mPOVRayLight;
		props.mEnableCutoff = mEnableCutoff;
		return props;
	}

	lcMatrix44 mWorldMatrix;

	lcVector3 mAttenuation;
	bool mSpotBlendSet;
	bool mSpotCutoffSet;
	bool mEnableCutoff;
	bool mPOVRayLight;
	float mLightDiffuse;
	float mLightSpecular;
	float mSpotCutoff;
	float mSpotExponent;
	float mPOVRayExponent;

protected:
	void UpdateLightType();

	void DrawPointLight(lcContext* Context) const;
	void DrawSpotLight(lcContext* Context) const;
	void DrawDirectionalLight(lcContext* Context) const;
	void DrawAreaLight(lcContext* Context) const;

	void SetupLightMatrix(lcContext* Context) const;
	void DrawSphere(lcContext* Context, const lcVector3& Center, float Radius) const;
	void DrawCylinder(lcContext* Context, float Radius, float Height) const;
	void DrawTarget(lcContext* Context) const;
	void DrawCone(lcContext* Context, float TargetDistance) const;

	QString mName;
	lcLightType mLightType = lcLightType::Point;
	bool mCastShadow = true;
	lcVector3 mColor = lcVector3(1.0f, 1.0f, 1.0f);
	lcVector2 mSize = lcVector2(0.0f, 0.0f);
	float mPower = 1.0f;
	float mSpotConeAngle = 80.0f;
	float mSpotPenumbraAngle = 0.0f;
	float mSpotTightness = 0.0f;
	lcLightAreaShape mAreaShape = lcLightAreaShape::Rectangle;

	quint32 mState = 0;
	bool mSelected = false;
	quint32 mFocusedSection = LC_LIGHT_SECTION_INVALID;
	lcVector3 mTargetMovePosition = lcVector3(0.0f, 0.0f, 0.0f);

	lcObjectKeyArray<lcVector3> mPositionKeys;
	lcObjectKeyArray<lcMatrix33> mRotationKeys;
	lcObjectKeyArray<lcVector3> mColorKeys;
	lcObjectKeyArray<lcVector2> mSizeKeys;
	lcObjectKeyArray<float> mPowerKeys;
	lcObjectKeyArray<float> mSpotConeAngleKeys;
	lcObjectKeyArray<float> mSpotPenumbraAngleKeys;
	lcObjectKeyArray<float> mSpotTightnessKeys;

	lcObjectKeyArray<lcVector3> mAttenuationKeys;
	lcObjectKeyArray<float> mLightSpecularKeys;
	lcObjectKeyArray<float> mLightDiffuseKeys;
	lcObjectKeyArray<float> mSpotCutoffKeys;
	lcObjectKeyArray<float> mSpotExponentKeys;

	static constexpr float mTargetDistance = 50.0f;
};
