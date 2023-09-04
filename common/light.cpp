#include "lc_global.h"
#include "lc_math.h"
#include "lc_colors.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "light.h"
#include "lc_application.h"
#include "lc_context.h"

#define LC_LIGHT_SPHERE_RADIUS 5.0f
#define LC_LIGHT_TARGET_EDGE 5.0f
#define LC_LIGHT_SPOT_CONE_HEIGHT 10.0f
#define LC_LIGHT_SPOT_CONE_RADIUS 7.5f
#define LC_LIGHT_DIRECTIONAL_RADIUS 5.0f
#define LC_LIGHT_DIRECTIONAL_HEIGHT 7.5f

#define LC_LIGHT_POSITION_EDGE 7.5f

static const std::array<QLatin1String, static_cast<int>(lcLightType::Count)> gLightTypes = { QLatin1String("POINT"), QLatin1String("SPOT"), QLatin1String("DIRECTIONAL"), QLatin1String("AREA") };

lcLight::lcLight(const lcVector3& Position, const lcVector3& TargetPosition, lcLightType LightType)
	: lcObject(lcObjectType::Light), mLightType(LightType)
{
	mState = 0;

	mPosition = Position;
	mTargetPosition = TargetPosition;
	mUpVector = lcVector3(0, 0, 1);

	if (IsAreaLight())
	{
		lcVector3 FrontVector = lcNormalize(TargetPosition - Position), SideVector;

		if (FrontVector == mUpVector)
			SideVector = lcVector3(1, 0, 0);
		else
			SideVector = lcCross(FrontVector, mUpVector);

		mUpVector = lcCross(SideVector, FrontVector);
		mUpVector.Normalize();
	}

	mPOVRayLight = false;
	mEnableCutoff = false;
	mAttenuation = lcVector3(1.0f, 0.0f, 0.0f);
	mLightFactor[0] = LightType == lcLightType::Directional ? 11.4f : 0.25f;
	mLightFactor[1] = LightType == lcLightType::Area ? 0.25f : LightType == lcLightType::Spot ? 0.150f : 0.0f;
	mLightDiffuse = 1.0f;
	mLightSpecular = 1.0f;
	mSpotExponent = 10.0f;
	mPOVRayExponent = 1.0f;
	mSpotSize = 75.0f;
	mSpotCutoff = LightType != lcLightType::Directional ? 40.0f : 0.0f;
	mSpotFalloff = 45.0f;
	mSpotTightness = 0;
	mAreaGrid = lcVector2(10.0f, 10.0f);
	mAreaSize = lcVector2(200.0f, 200.0f);
	mLightShape = LC_LIGHT_SHAPE_SQUARE;

	mPositionKeys.ChangeKey(mPosition, 1, true);
	mTargetPositionKeys.ChangeKey(mTargetPosition, 1, true);
	mUpVectorKeys.ChangeKey(mUpVector, 1, true);
	mColorKeys.ChangeKey(mColor, 1, true);
	mAttenuationKeys.ChangeKey(mAttenuation, 1, true);
	mLightFactorKeys.ChangeKey(mLightFactor, 1, true);
	mLightDiffuseKeys.ChangeKey(mLightDiffuse, 1, true);
	mLightSpecularKeys.ChangeKey(mLightSpecular, 1, true);
	mSpotCutoffKeys.ChangeKey(mSpotCutoff, 1, true);
	mSpotFalloffKeys.ChangeKey(mSpotFalloff, 1, true);
	mSpotExponentKeys.ChangeKey(mSpotExponent, 1, true);
	mSpotSizeKeys.ChangeKey(mSpotSize, 1, true);
	mSpotTightnessKeys.ChangeKey(mSpotTightness, 1, true);
	mAreaGridKeys.ChangeKey(mAreaGrid, 1, true);

	UpdatePosition(1);
}

QString lcLight::GetLightTypeString(lcLightType LightType)
{
	switch (LightType)
	{
	case lcLightType::Point:
		return QT_TRANSLATE_NOOP("Light Names", "Point Light");

	case lcLightType::Spot:
		return QT_TRANSLATE_NOOP("Light Names", "Spotlight");

	case lcLightType::Directional:
		return QT_TRANSLATE_NOOP("Light Names", "Directional Light");

	case lcLightType::Area:
		return QT_TRANSLATE_NOOP("Light Names", "Area Light");

	case lcLightType::Count:
		break;
	}

	return QString();
}

void lcLight::SaveLDraw(QTextStream& Stream) const
{
	const QLatin1String LineEnding("\r\n");

	if (mPOVRayLight)
		Stream << QLatin1String("0 !LEOCAD LIGHT POV_RAY") << LineEnding;

	if (!mCastShadow)
		Stream << QLatin1String("0 !LEOCAD LIGHT SHADOWLESS") << LineEnding;

	if (mPositionKeys.GetSize() > 1)
		mPositionKeys.SaveKeysLDraw(Stream, "LIGHT POSITION_KEY ");
	else
		Stream << QLatin1String("0 !LEOCAD LIGHT POSITION ") << mPosition[0] << ' ' << mPosition[1] << ' ' << mPosition[2] << LineEnding;

	if (mLightType != lcLightType::Point)
	{
		if (mTargetPositionKeys.GetSize() > 1)
			mTargetPositionKeys.SaveKeysLDraw(Stream, "LIGHT TARGET_POSITION_KEY ");
		else
			Stream << QLatin1String("0 !LEOCAD LIGHT TARGET_POSITION ") << mTargetPosition[0] << ' ' << mTargetPosition[1] << ' ' << mTargetPosition[2] << LineEnding;
	}

	if (mLightType == lcLightType::Area)
	{
		if (mUpVectorKeys.GetSize() > 1)
			mUpVectorKeys.SaveKeysLDraw(Stream, "LIGHT UP_VECTOR_KEY ");
		else
			Stream << QLatin1String("0 !LEOCAD LIGHT UP_VECTOR ") << mUpVector[0] << ' ' << mUpVector[1] << ' ' << mUpVector[2] << LineEnding;
	}

	if (mColorKeys.GetSize() > 1)
		mColorKeys.SaveKeysLDraw(Stream, "LIGHT COLOR_KEY ");
	else
		Stream << QLatin1String("0 !LEOCAD LIGHT COLOR ") << mColor[0] << ' ' << mColor[1] << ' ' << mColor[2] << LineEnding;

	if (!mPOVRayLight)
	{
		if (mLightDiffuseKeys.GetSize() > 1)
			mLightDiffuseKeys.SaveKeysLDraw(Stream, "LIGHT DIFFUSE_KEY ");
		else
			Stream << QLatin1String("0 !LEOCAD LIGHT DIFFUSE ") << mLightDiffuse << LineEnding;

		if (mLightSpecularKeys.GetSize() > 1)
			mLightSpecularKeys.SaveKeysLDraw(Stream, "LIGHT SPECULAR_KEY ");
		else
			Stream << QLatin1String("0 !LEOCAD LIGHT SPECULAR ") << mLightSpecular << LineEnding;
	}

	if (mSpotExponentKeys.GetSize() > 1)
		mSpotExponentKeys.SaveKeysLDraw(Stream, "LIGHT POWER_KEY ");
	else
		Stream << QLatin1String("0 !LEOCAD LIGHT POWER ") << (mPOVRayLight ? mPOVRayExponent : mSpotExponent) << LineEnding;

	if (mEnableCutoff && !mPOVRayLight)
	{
		if (mSpotCutoffKeys.GetSize() > 1)
			mSpotCutoffKeys.SaveKeysLDraw(Stream, "LIGHT CUTOFF_DISTANCE_KEY ");
		else
			Stream << QLatin1String("0 !LEOCAD LIGHT CUTOFF_DISTANCE ") << mSpotCutoff << LineEnding;
	}

	switch (mLightType)
	{
	case lcLightType::Count:
		break;

	case lcLightType::Point:
		if (!mPOVRayLight)
		{
			if (mLightFactorKeys.GetSize() > 1)
				mLightFactorKeys.SaveKeysLDraw(Stream, "LIGHT RADIUS_KEY ");
			else
				Stream << QLatin1String("0 !LEOCAD LIGHT RADIUS ") << mLightFactor[0] << LineEnding;
		}
		break;

	case lcLightType::Spot:
		if (mPOVRayLight)
		{
			if (mLightFactorKeys.GetSize() > 1)
				mLightFactorKeys.SaveKeysLDraw(Stream, "LIGHT RADIUS_KEY ");
			else
				Stream << QLatin1String("0 !LEOCAD LIGHT RADIUS ") << (mSpotSize - mSpotFalloff) << LineEnding;
			if (mSpotFalloffKeys.GetSize() > 1)
				mSpotFalloffKeys.SaveKeysLDraw(Stream, "LIGHT SPOT_FALLOFF_KEY ");
			else
				Stream << QLatin1String("0 !LEOCAD LIGHT SPOT_FALLOFF ") << mSpotFalloff << LineEnding;
			if (mSpotTightnessKeys.GetSize() > 1)
				mSpotTightnessKeys.SaveKeysLDraw(Stream, "SPOT_TIGHTNESS_KEY ");
			else
				Stream << QLatin1String("0 !LEOCAD LIGHT SPOT_TIGHTNESS ") << mSpotTightness << LineEnding;
		}
		else
		{
			if (mSpotSizeKeys.GetSize() > 1)
				mSpotSizeKeys.SaveKeysLDraw(Stream, "LIGHT SPOT_SIZE_KEY ");
			else
				Stream << QLatin1String("0 !LEOCAD LIGHT SPOT_SIZE ") << mSpotSize << LineEnding;

			if (mLightFactorKeys.GetSize() > 1)
				mLightFactorKeys.SaveKeysLDraw(Stream, "LIGHT RADIUS_AND_SPOT_BLEND_KEY ");
			else
			{
				Stream << QLatin1String("0 !LEOCAD LIGHT RADIUS ") << mLightFactor[0] << LineEnding;
				Stream << QLatin1String("0 !LEOCAD LIGHT SPOT_BLEND ") << mLightFactor[1] << LineEnding;
			}
		}
		break;

	case lcLightType::Directional:
		if (mSpotExponentKeys.GetSize() > 1)
			mSpotExponentKeys.SaveKeysLDraw(Stream, "LIGHT STRENGTH_KEY ");
		else
			Stream << QLatin1String("0 !LEOCAD LIGHT STRENGTH ") << mSpotExponent << LineEnding;

		if (mLightFactorKeys.GetSize() > 1)
			mLightFactorKeys.SaveKeysLDraw(Stream, "LIGHT ANGLE_KEY ");
		else
			Stream << QLatin1String("0 !LEOCAD LIGHT ANGLE ") << mLightFactor[0] << LineEnding;
		break;

	case lcLightType::Area:
		if (mPOVRayLight)
		{
			if (mAreaGridKeys.GetSize() > 1)
				mAreaGridKeys.SaveKeysLDraw(Stream, "LIGHT AREA_GRID_KEY ");
			else
				Stream << QLatin1String("0 !LEOCAD LIGHT AREA_ROWS ") << mAreaGrid[0] << QLatin1String(" AREA_COLUMNS ") << mAreaGrid[1] << LineEnding;
		}
		if (mLightFactorKeys.GetSize() > 1)
			mLightFactorKeys.SaveKeysLDraw(Stream, "LIGHT SIZE_KEY ");
		else
		{
			if (mPOVRayLight)
			{
				Stream << QLatin1String("0 !LEOCAD LIGHT WIDTH ") << mAreaSize[0] << QLatin1String(" HEIGHT ") << mAreaSize[1] << LineEnding;
			}
			else
			{
				if (mLightShape == LC_LIGHT_SHAPE_RECTANGLE || mLightShape == LC_LIGHT_SHAPE_ELLIPSE || mLightFactor[1] > 0)
					Stream << QLatin1String("0 !LEOCAD LIGHT WIDTH ") << mLightFactor[0] << QLatin1String(" HEIGHT ") << mLightFactor[1] << LineEnding;
				else
					Stream << QLatin1String("0 !LEOCAD LIGHT SIZE ") << mLightFactor[0] << LineEnding;
			}
		}

		Stream << QLatin1String("0 !LEOCAD LIGHT SHAPE ");

		QString Shape = QLatin1String("UNDEFINED ");

		switch (mLightShape)
		{
		case LC_LIGHT_SHAPE_SQUARE:
			Shape = QLatin1String("SQUARE ");
			break;
		case LC_LIGHT_SHAPE_DISK:
			Shape = QLatin1String("DISK ");
			break;
		case LC_LIGHT_SHAPE_RECTANGLE:
			Shape = QLatin1String("RECTANGLE ");
			break;
		case LC_LIGHT_SHAPE_ELLIPSE:
			Shape = QLatin1String("ELLIPSE ");
			break;
		}

		Stream << QLatin1String("0 !LEOCAD LIGHT SHAPE ") << Shape << LineEnding;

		break;
	}

	Stream << QLatin1String("0 !LEOCAD LIGHT TYPE ") << gLightTypes[static_cast<int>(mLightType)] << QLatin1String(" NAME ") << mName << LineEnding;
}

void lcLight::CreateName(const lcArray<lcLight*>& Lights)
{
	if (!mName.isEmpty())
	{
		bool Found = false;

		for (const lcLight* Light : Lights)
		{
			if (Light->GetName() == mName)
			{
				Found = true;
				break;
			}
		}

		if (!Found)
			return;
	}

	int MaxLightNumber = 0;

	QString Prefix;

	switch (mLightType)
	{
	case lcLightType::Point:
		Prefix = QLatin1String("Point Light ");
		break;

	case lcLightType::Spot:
		Prefix = QLatin1String("Spotlight ");
		break;

	case lcLightType::Directional:
		Prefix = QLatin1String("Directional Light ");
		break;

	case lcLightType::Area:
		Prefix = QLatin1String("Area Light ");
		break;

	case lcLightType::Count:
		break;
	}

	for (const lcLight* Light : Lights)
	{
		QString LightName = Light->GetName();

		if (LightName.startsWith(Prefix))
		{
			bool Ok = false;
			int LightNumber = LightName.mid(Prefix.size()).toInt(&Ok);

			if (Ok && LightNumber > MaxLightNumber)
				MaxLightNumber = LightNumber;
		}
	}

	mName = Prefix + QString::number(MaxLightNumber + 1);
}

bool lcLight::ParseLDrawLine(QTextStream& Stream)
{
	while (!Stream.atEnd())
	{
		QString Token;
		Stream >> Token;

		if (Token == QLatin1String("POSITION"))
		{
			Stream >> mPosition[0] >> mPosition[1] >> mPosition[2];
			mPositionKeys.ChangeKey(mPosition, 1, true);
		}
		else if (Token == QLatin1String("POSITION_KEY"))
			mPositionKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("TARGET_POSITION"))
		{
			Stream >> mTargetPosition[0] >> mTargetPosition[1] >> mTargetPosition[2];
			mTargetPositionKeys.ChangeKey(mTargetPosition, 1, true);
		}
		else if (Token == QLatin1String("TARGET_POSITION_KEY"))
			mTargetPositionKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("UP_VECTOR"))
		{
			Stream >> mUpVector[0] >> mUpVector[1] >> mUpVector[2];
			mUpVectorKeys.ChangeKey(mUpVector, 1, true);
		}
		else if (Token == QLatin1String("UP_VECTOR_KEY"))
			mUpVectorKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("COLOR"))
		{
			Stream >> mColor[0] >> mColor[1] >> mColor[2];
			mColorKeys.ChangeKey(mColor, 1, true);
		}
		else if (Token == QLatin1String("COLOR_KEY"))
			mColorKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("POWER") || Token == QLatin1String("STRENGTH"))
		{
			if (mPOVRayLight)
			{
				Stream >> mPOVRayExponent;
				mSpotExponentKeys.ChangeKey(mPOVRayExponent, 1, true);
			}
			else
			{
				Stream >> mSpotExponent;
				mSpotExponentKeys.ChangeKey(mSpotExponent, 1, true);
			}
		}
		else if (Token == QLatin1String("RADIUS") || Token == QLatin1String("SIZE") || Token == QLatin1String("WIDTH") || (mHeightSet = Token == QLatin1String("HEIGHT")) || (mSpotBlendSet = Token == QLatin1String("SPOT_BLEND")) || (mAngleSet = Token == QLatin1String("ANGLE")))
		{
			if (mPOVRayLight)
			{
				if (Token == QLatin1String("WIDTH"))
					Stream >> mAreaSize[0];
				else if (Token == QLatin1String("HEIGHT"))
					Stream >> mAreaSize[1];
				mLightFactorKeys.ChangeKey(mAreaSize, 1, true);
			}
			else
			{
				if(Token == QLatin1String("HEIGHT") || Token == QLatin1String("SPOT_BLEND"))
					Stream >> mLightFactor[1];
				else
					Stream >> mLightFactor[0];
				mLightFactorKeys.ChangeKey(mLightFactor, 1, true);
			}
		}
		else if (Token == QLatin1String("AREA_ROWS"))
		{
			mPOVRayLight = true;
			Stream >> mAreaGrid[0];
			mAreaGridKeys.ChangeKey(mAreaGrid, 1, true);
		}
		else if (Token == QLatin1String("AREA_COLUMNS"))
		{
			mPOVRayLight = true;
			Stream >> mAreaGrid[1];
			mAreaGridKeys.ChangeKey(mAreaGrid, 1, true);
		}
		else if (Token == QLatin1String("SPOT_FALLOFF"))
		{
			mPOVRayLight = true;
			Stream >> mSpotFalloff;
			mSpotFalloffKeys.ChangeKey(mSpotFalloff, 1, true);
		}
		else if (Token == QLatin1String("SPOT_TIGHTNESS"))
		{
			mPOVRayLight = true;
			Stream >> mSpotTightness;
			mSpotTightnessKeys.ChangeKey(mSpotTightness, 1, true);
		}
		else if (Token == QLatin1String("SPOT_SIZE"))
		{
			Stream >> mSpotSize;
			mSpotSizeKeys.ChangeKey(mSpotSize, 1, true);
		}
		else if (Token == QLatin1String("SHAPE"))
		{
			QString Shape;
			Stream >> Shape;
			Shape.replace("\"", "");

			if (Shape == QLatin1String("SQUARE"))
				mLightShape = LC_LIGHT_SHAPE_SQUARE;
			else if (Shape == QLatin1String("DISK") || Shape == QLatin1String("CIRCLE"))
				mLightShape = LC_LIGHT_SHAPE_DISK;
			else if (Shape == QLatin1String("RECTANGLE"))
				mLightShape = LC_LIGHT_SHAPE_RECTANGLE;
			else if (Shape == QLatin1String("ELLIPSE"))
				mLightShape = LC_LIGHT_SHAPE_ELLIPSE;
		}
		else if (Token == QLatin1String("DIFFUSE"))
		{
			Stream >>mLightDiffuse;
			mLightDiffuseKeys.ChangeKey(mLightDiffuse, 1, true);
		}
		else if (Token == QLatin1String("SPECULAR"))
		{
			Stream >>mLightSpecular;
			mLightSpecularKeys.ChangeKey(mLightSpecular, 1, true);
		}
		else if ((mSpotCutoffSet = Token == QLatin1String("CUTOFF_DISTANCE")))
		{
			mEnableCutoff = true;
			Stream >> mSpotCutoff;
			mSpotCutoffKeys.ChangeKey(mSpotCutoff, 1, true);
		}
		else if (Token == QLatin1String("TYPE"))
		{
			QString Type;
			Stream >> Type;

			for (size_t TypeIndex = 0; TypeIndex < gLightTypes.size(); TypeIndex++)
			{
				if (Type == gLightTypes[TypeIndex])
				{
					mLightType = static_cast<lcLightType>(TypeIndex);
					break;
				}
			}
		}
		else if (Token == QLatin1String("POV_RAY"))
		{
			mPOVRayLight = true;
		}
		else if (Token == QLatin1String("SHADOWLESS"))
		{
			mCastShadow = false;
		}
		else if ((Token == QLatin1String("POWER_KEY")) || (Token == QLatin1String("STRENGTH_KEY")))
			mSpotExponentKeys.LoadKeysLDraw(Stream);
		else if ((Token == QLatin1String("ANGLE_KEY")) || (Token == QLatin1String("RADIUS_KEY")) || (Token == QLatin1String("SIZE_KEY")) || (Token == QLatin1String("RADIUS_AND_SPOT_BLEND_KEY")))
			mLightFactorKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("SPOT_SIZE_KEY"))
			mSpotSizeKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("SPOT_FALLOFF_KEY"))
			mSpotFalloffKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("SPOT_TIGHTNESS_KEY"))
			mSpotTightnessKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("AREA_GRID_KEY"))
			mAreaGridKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("DIFFUSE_KEY"))
			mLightDiffuseKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("SPECULAR_KEY"))
			mLightSpecularKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("CUTOFF_DISTANCE_KEY"))
			mSpotCutoffKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("POSITION_KEY"))
			mPositionKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("TARGET_POSITION_KEY"))
			mTargetPositionKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("NAME"))
		{
			mName = Stream.readAll().trimmed();
			mName.replace("\"", "");

			// Set default settings per light type
			switch (mLightType)
			{
			case lcLightType::Point:
				break;

			case lcLightType::Spot:
				if (!mSpotBlendSet)
				{
					mLightFactor[1] = 0.15f;
					mLightFactorKeys.ChangeKey(mLightFactor, 1, true);
				}
				break;

			case lcLightType::Directional:
				if (!mAngleSet)
				{
					mLightFactor[0] = 11.4f;
					mLightFactorKeys.ChangeKey(mLightFactor, 1, true);
				}

				if (!mSpotCutoffSet)
				{
					mSpotCutoff = 0.0f;
					mSpotCutoffKeys.ChangeKey(mSpotCutoff, 1, true);
				}
				break;

			case lcLightType::Area:
				if (mLightShape == LC_LIGHT_SHAPE_RECTANGLE || mLightShape == LC_LIGHT_SHAPE_ELLIPSE)
				{
					if (!mHeightSet)
					{
						mLightFactor[1] = 0.25f;
						mLightFactorKeys.ChangeKey(mLightFactor, 1, true);
					}
				}
				break;

			case lcLightType::Count:
				break;
			}

			return true;
		}
	}

	return false;
}

void lcLight::CompareBoundingBox(lcVector3& Min, lcVector3& Max)
{
	const lcVector3 Points[2] =
	{
		mPosition, mTargetPosition
	};

	for (int i = 0; i < (IsPointLight() ? 1 : 2); i++)
	{
		const lcVector3& Point = Points[i];

		// TODO: this should check the entire mesh

		Min = lcMin(Point, Min);
		Max = lcMax(Point, Max);
	}
}

void lcLight::UpdateLight(lcStep Step, lcLightProperties Props, int Property)
{
	switch(Property)
	{
	case LC_LIGHT_SHAPE:
		mLightShape = Props.mLightShape;
		break;
	case LC_LIGHT_FACTOR:
		if (Props.mPOVRayLight && mLightType == lcLightType::Area)
		{
			mAreaSize = Props.mLightFactor;
			mLightFactorKeys.ChangeKey(mAreaSize, 1, true);
		}
		else
		{
			mLightFactor = Props.mLightFactor;
			mLightFactorKeys.ChangeKey(mLightFactor, 1, true);
		}
		break;
	case LC_LIGHT_DIFFUSE:
		mLightDiffuse = Props.mLightDiffuse;
		mLightDiffuseKeys.ChangeKey(mLightDiffuse, Step, false);
		break;
	case LC_LIGHT_SPECULAR:
		mLightSpecular = Props.mLightSpecular;
		mLightSpecularKeys.ChangeKey(mLightSpecular, Step, false);
		break;
	case LC_LIGHT_EXPONENT:
		if (Props.mPOVRayLight)
		{
			mPOVRayExponent = Props.mSpotExponent;
			mSpotExponentKeys.ChangeKey(mPOVRayExponent, Step, false);
		}
		else
		{
			mSpotExponent = Props.mSpotExponent;
			mSpotExponentKeys.ChangeKey(mSpotExponent, Step, false);
		}
		break;
	case LC_LIGHT_AREA_GRID:
		mAreaGrid = Props.mAreaGrid;
		mAreaGridKeys.ChangeKey(mAreaGrid, Step, false);
		break;
	case LC_LIGHT_SPOT_SIZE:
		mSpotSize = Props.mSpotSize;
		mSpotSizeKeys.ChangeKey(mSpotSize, Step, false);
		break;
	case LC_LIGHT_SPOT_FALLOFF:
		mSpotFalloff = Props.mSpotFalloff;
		mSpotFalloffKeys.ChangeKey(mSpotFalloff, Step, false);
		break;
	case LC_LIGHT_SPOT_TIGHTNESS:
		mSpotTightness = Props.mSpotTightness;
		mSpotTightnessKeys.ChangeKey(mSpotTightness, Step, false);
		break;
	case LC_LIGHT_CUTOFF:
		mSpotCutoff = Props.mSpotCutoff;
		mSpotCutoffKeys.ChangeKey(mSpotCutoff, Step, false);
		break;
	case LC_LIGHT_USE_CUTOFF:
		mEnableCutoff = Props.mEnableCutoff;
		break;
	case LC_LIGHT_POVRAY:
		mPOVRayLight = Props.mPOVRayLight;
		break;
	}
	UpdatePosition(Step);
}

void lcLight::RayTest(lcObjectRayTest& ObjectRayTest) const
{
	if (IsPointLight())
	{
		float Distance;

		if (lcSphereRayMinIntersectDistance(mPosition, LC_LIGHT_SPHERE_RADIUS, ObjectRayTest.Start, ObjectRayTest.End, &Distance) && (Distance < ObjectRayTest.Distance))
		{
			ObjectRayTest.ObjectSection.Object = const_cast<lcLight*>(this);
			ObjectRayTest.ObjectSection.Section = LC_LIGHT_SECTION_POSITION;
			ObjectRayTest.Distance = Distance;
		}

		return;
	}

	if (mLightType == lcLightType::Spot)
	{
		float Distance;
		lcVector3 Direction = lcNormalize(mTargetPosition - mPosition);

		if (lcConeRayMinIntersectDistance(mPosition - Direction * LC_LIGHT_SPOT_CONE_HEIGHT, Direction, LC_LIGHT_SPOT_CONE_RADIUS, LC_LIGHT_SPOT_CONE_HEIGHT, ObjectRayTest.Start, ObjectRayTest.End, &Distance) && (Distance < ObjectRayTest.Distance))
		{
			ObjectRayTest.ObjectSection.Object = const_cast<lcLight*>(this);
			ObjectRayTest.ObjectSection.Section = LC_LIGHT_SECTION_POSITION;
			ObjectRayTest.Distance = Distance;
		}
	}
	else if (mLightType == lcLightType::Area)
	{
		lcVector3 FrontVector = mTargetPosition - mPosition;
		lcVector4 Plane(FrontVector, -lcDot(FrontVector, mPosition));
		lcVector3 Intersection;

		if (lcLineSegmentPlaneIntersection(&Intersection, ObjectRayTest.Start, ObjectRayTest.End, Plane))
		{
			lcVector3 UpVector(1, 1, 1);

			if (fabs(FrontVector[0]) < fabs(FrontVector[1]))
			{
				if (fabs(FrontVector[0]) < fabs(FrontVector[2]))
					UpVector[0] = -(UpVector[1] * FrontVector[1] + UpVector[2] * FrontVector[2]);
				else
					UpVector[2] = -(UpVector[0] * FrontVector[0] + UpVector[1] * FrontVector[1]);
			}
			else
			{
				if (fabs(FrontVector[1]) < fabs(FrontVector[2]))
					UpVector[1] = -(UpVector[0] * FrontVector[0] + UpVector[2] * FrontVector[2]);
				else
					UpVector[2] = -(UpVector[0] * FrontVector[0] + UpVector[1] * FrontVector[1]);
			}

			lcVector3 XAxis = lcNormalize(lcCross(FrontVector, UpVector));
			lcVector3 YAxis = lcNormalize(lcCross(FrontVector, XAxis));
			lcVector3 IntersectionDirection = Intersection - mPosition;

			float x = lcDot(IntersectionDirection, XAxis);
			float y = lcDot(IntersectionDirection, YAxis);

			if (fabsf(x) < mAreaSize.x / 2.0f && fabsf(y) < mAreaSize.y / 2.0f)
			{
				float Distance = lcLength(Intersection - ObjectRayTest.Start);

				if (Distance < ObjectRayTest.Distance)
				{
					ObjectRayTest.ObjectSection.Object = const_cast<lcLight*>(this);
					ObjectRayTest.ObjectSection.Section = LC_LIGHT_SECTION_POSITION;
					ObjectRayTest.Distance = Distance;
				}
			}
		}
	}

	lcVector3 Start = lcMul31(ObjectRayTest.Start, mWorldLight);
	lcVector3 End = lcMul31(ObjectRayTest.End, mWorldLight);

	float Distance;
	lcVector3 Plane;

	if (mLightType == lcLightType::Directional)
	{
		if (lcCylinderRayMinIntersectDistance(LC_LIGHT_DIRECTIONAL_RADIUS, LC_LIGHT_DIRECTIONAL_HEIGHT, Start, End, &Distance) && (Distance < ObjectRayTest.Distance))
		{
			ObjectRayTest.ObjectSection.Object = const_cast<lcLight*>(this);
			ObjectRayTest.ObjectSection.Section = LC_LIGHT_SECTION_POSITION;
			ObjectRayTest.Distance = Distance;
			ObjectRayTest.PieceInfoRayTest.Plane = Plane;
		}
	}

	lcVector3 Min = lcVector3(-LC_LIGHT_TARGET_EDGE, -LC_LIGHT_TARGET_EDGE, -LC_LIGHT_TARGET_EDGE);
	lcVector3 Max = lcVector3( LC_LIGHT_TARGET_EDGE,  LC_LIGHT_TARGET_EDGE,  LC_LIGHT_TARGET_EDGE);

	lcMatrix44 WorldTarget = mWorldLight;
	WorldTarget.SetTranslation(lcMul30(-mTargetPosition, WorldTarget));

	Start = lcMul31(ObjectRayTest.Start, WorldTarget);
	End = lcMul31(ObjectRayTest.End, WorldTarget);

	if (lcBoundingBoxRayIntersectDistance(Min, Max, Start, End, &Distance, nullptr, &Plane) && (Distance < ObjectRayTest.Distance))
	{
		ObjectRayTest.ObjectSection.Object = const_cast<lcLight*>(this);
		ObjectRayTest.ObjectSection.Section = LC_LIGHT_SECTION_TARGET;
		ObjectRayTest.Distance = Distance;
		ObjectRayTest.PieceInfoRayTest.Plane = Plane;
	}

	if (IsAreaLight())
	{
		const lcMatrix44 LightWorld = lcMatrix44AffineInverse(mWorldLight);
		const lcVector3 UpVectorPosition = lcMul31(lcVector3(0, 25, 0), LightWorld);

		lcMatrix44 WorldLight = mWorldLight;
		WorldLight.SetTranslation(lcMul30(-UpVectorPosition, WorldLight));

		Start = lcMul31(ObjectRayTest.Start, WorldLight);
		End = lcMul31(ObjectRayTest.End, WorldLight);

		if (lcBoundingBoxRayIntersectDistance(Min, Max, Start, End, &Distance, nullptr, &Plane) && (Distance < ObjectRayTest.Distance))
		{
			ObjectRayTest.ObjectSection.Object = const_cast<lcLight*>(this);
			ObjectRayTest.ObjectSection.Section = LC_LIGHT_SECTION_UPVECTOR;
			ObjectRayTest.Distance = Distance;
			ObjectRayTest.PieceInfoRayTest.Plane = Plane;
		}
	}
}

void lcLight::BoxTest(lcObjectBoxTest& ObjectBoxTest) const
{
	if (IsPointLight())
	{
		for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
			if (lcDot3(mPosition, ObjectBoxTest.Planes[PlaneIdx]) + ObjectBoxTest.Planes[PlaneIdx][3] > LC_LIGHT_SPHERE_RADIUS)
				return;

		ObjectBoxTest.Objects.Add(const_cast<lcLight*>(this));
		return;
	}

	lcVector3 Min(-LC_LIGHT_POSITION_EDGE, -LC_LIGHT_POSITION_EDGE, -LC_LIGHT_POSITION_EDGE); // todo: fix light box test
	lcVector3 Max(LC_LIGHT_POSITION_EDGE, LC_LIGHT_POSITION_EDGE, LC_LIGHT_POSITION_EDGE);

	lcVector4 LocalPlanes[6];

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		const lcVector3 Normal = lcMul30(ObjectBoxTest.Planes[PlaneIdx], mWorldLight);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, ObjectBoxTest.Planes[PlaneIdx][3] - lcDot3(mWorldLight[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		ObjectBoxTest.Objects.Add(const_cast<lcLight*>(this));
		return;
	}

	Min = lcVector3(-LC_LIGHT_TARGET_EDGE, -LC_LIGHT_TARGET_EDGE, -LC_LIGHT_TARGET_EDGE);
	Max = lcVector3(LC_LIGHT_TARGET_EDGE, LC_LIGHT_TARGET_EDGE, LC_LIGHT_TARGET_EDGE);

	lcMatrix44 WorldTarget = mWorldLight;
	WorldTarget.SetTranslation(lcMul30(-mTargetPosition, WorldTarget));

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		const lcVector3 Normal = lcMul30(ObjectBoxTest.Planes[PlaneIdx], WorldTarget);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, ObjectBoxTest.Planes[PlaneIdx][3] - lcDot3(WorldTarget[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		ObjectBoxTest.Objects.Add(const_cast<lcLight*>(this));
		return;
	}
}

void lcLight::MoveSelected(lcStep Step, bool AddKey, const lcVector3& Distance)
{
	if (IsSelected(LC_LIGHT_SECTION_POSITION))
	{
		mPosition += Distance;
		mPositionKeys.ChangeKey(mPosition, Step, AddKey);
	}

	if (IsSelected(LC_LIGHT_SECTION_TARGET))
	{
		mTargetPosition += Distance;
		mTargetPositionKeys.ChangeKey(mTargetPosition, Step, AddKey);
	}
	else if (IsSelected(LC_LIGHT_SECTION_UPVECTOR))
	{
		mUpVector += Distance;
		mUpVector.Normalize();
		mUpVectorKeys.ChangeKey(mUpVector, Step, AddKey);
	}

	if (IsAreaLight())
	{
		const lcVector3 FrontVector(mTargetPosition - mPosition);
		lcVector3 SideVector = lcCross(FrontVector, mUpVector);

		if (fabsf(lcDot(mUpVector, SideVector)) > 0.99f)
			SideVector = lcVector3(1, 0, 0);

		mUpVector = lcCross(SideVector, FrontVector);
		mUpVector.Normalize();
	}
}

void lcLight::Rotate(lcStep Step, bool AddKey, const lcMatrix33& RotationMatrix, const lcMatrix33& RotationFrame)
{
	if (IsPointLight())
		return;

	if (GetFocusSection() != LC_LIGHT_SECTION_POSITION)
		return;

//	lcVector3 Direction = mTargetPosition - mPosition;
//
//	Direction = lcMul(Direction, RotationMatrix);
//
//	mTargetPosition = mPosition + Direction;
//	mTargetPositionKeys.ChangeKey(mTargetPosition, Step, AddKey);


	const lcMatrix33 LocalToWorldMatrix = lcMatrix33AffineInverse(lcMatrix33(mWorldLight));

	const lcMatrix33 LocalToFocusMatrix = lcMul(LocalToWorldMatrix, RotationFrame);
	lcMatrix33 NewLocalToWorldMatrix = lcMul(LocalToFocusMatrix, RotationMatrix);

	const lcMatrix33 WorldToLocalMatrix = lcMatrix33AffineInverse(LocalToWorldMatrix);

	NewLocalToWorldMatrix.Orthonormalize();

	lcVector3 Target = lcMul(lcVector3(0.0f, 0.0f, lcLength(mTargetPosition - mPosition)), NewLocalToWorldMatrix);
	mTargetPosition = mPosition - Target;
	mTargetPositionKeys.ChangeKey(mTargetPosition, Step, AddKey);

	if (IsAreaLight())
	{
		const lcVector3 FrontVector(mTargetPosition - mPosition);
		lcVector3 SideVector = lcCross(FrontVector, mUpVector);

		if (fabsf(lcDot(mUpVector, SideVector)) > 0.99f)
			SideVector = lcVector3(1, 0, 0);

		mUpVector = lcCross(SideVector, FrontVector);
		mUpVector.Normalize();
	}
}

void lcLight::SetLightType(lcLightType LightType)
{
	if (static_cast<int>(LightType) < 0 || LightType >= lcLightType::Count)
		return;

	mLightType = LightType;
}

void lcLight::SetColor(const lcVector3& Color, lcStep Step, bool AddKey)
{
	mColorKeys.ChangeKey(Color, Step, AddKey);
}

void lcLight::SetCastShadow(bool CastShadow)
{
	mCastShadow = CastShadow;
}

void lcLight::InsertTime(lcStep Start, lcStep Time)
{
	mPositionKeys.InsertTime(Start, Time);
	mTargetPositionKeys.InsertTime(Start, Time);
	mUpVectorKeys.InsertTime(Start, Time);
	mColorKeys.InsertTime(Start, Time);
	mAttenuationKeys.InsertTime(Start, Time);
	mLightFactorKeys.InsertTime(Start, Time);
	mLightDiffuseKeys.InsertTime(Start, Time);
	mLightSpecularKeys.InsertTime(Start, Time);
	mSpotSizeKeys.InsertTime(Start, Time);
	mSpotCutoffKeys.InsertTime(Start, Time);
	mSpotExponentKeys.InsertTime(Start, Time);
	mSpotFalloffKeys.InsertTime(Start, Time);
	mSpotTightnessKeys.InsertTime(Start, Time);
	mAreaGridKeys.InsertTime(Start, Time);
}

void lcLight::RemoveTime(lcStep Start, lcStep Time)
{
	mPositionKeys.RemoveTime(Start, Time);
	mTargetPositionKeys.RemoveTime(Start, Time);
	mUpVectorKeys.RemoveTime(Start, Time);
	mColorKeys.RemoveTime(Start, Time);
	mAttenuationKeys.RemoveTime(Start, Time);
	mLightFactorKeys.RemoveTime(Start, Time);
	mLightDiffuseKeys.RemoveTime(Start, Time);
	mLightSpecularKeys.RemoveTime(Start, Time);
	mSpotSizeKeys.RemoveTime(Start, Time);
	mSpotCutoffKeys.RemoveTime(Start, Time);
	mSpotExponentKeys.RemoveTime(Start, Time);
	mSpotFalloffKeys.RemoveTime(Start, Time);
	mSpotTightnessKeys.RemoveTime(Start, Time);
	mAreaGridKeys.RemoveTime(Start, Time);
}

void lcLight::UpdatePosition(lcStep Step)
{
	mPosition = mPositionKeys.CalculateKey(Step);
	mTargetPosition = mTargetPositionKeys.CalculateKey(Step);
	mUpVector = mUpVectorKeys.CalculateKey(Step);
	mColor = mColorKeys.CalculateKey(Step);
	mAttenuation = mAttenuationKeys.CalculateKey(Step);
	mLightFactor = mLightFactorKeys.CalculateKey(Step);
	mLightDiffuse = mLightDiffuseKeys.CalculateKey(Step);
	mLightSpecular = mLightSpecularKeys.CalculateKey(Step);
	mSpotSize = mSpotSizeKeys.CalculateKey(Step);
	mSpotCutoff = mSpotCutoffKeys.CalculateKey(Step);
	mSpotExponent = mSpotExponentKeys.CalculateKey(Step);
	mSpotFalloff = mSpotFalloffKeys.CalculateKey(Step);
	mSpotTightness = mSpotTightnessKeys.CalculateKey(Step);
	mAreaGrid = mAreaGridKeys.CalculateKey(Step);

	if (IsPointLight())
	{
		mWorldLight = lcMatrix44Identity();
		mWorldLight.SetTranslation(-mPosition);
	}
	else if (IsAreaLight())
	{
		lcVector3 UpVector(0, 0, 1), FrontVector(mPosition), SideVector;
		FrontVector.Normalize();
		if (fabsf(lcDot(UpVector, FrontVector)) > 0.99f)
			SideVector = lcVector3(-1, 0, 0);
		else
			SideVector = lcCross(FrontVector, UpVector);
		UpVector = lcCross(SideVector, FrontVector);
		UpVector.Normalize();
		mUpVector = UpVector;

		mWorldLight = lcMatrix44LookAt(mPosition, mTargetPosition, mUpVector);
	}
	else
	{
		lcVector3 FrontVector(mTargetPosition - mPosition);
		lcVector3 UpVector(1, 1, 1);

		if (fabs(FrontVector[0]) < fabs(FrontVector[1]))
		{
			if (fabs(FrontVector[0]) < fabs(FrontVector[2]))
				UpVector[0] = -(UpVector[1] * FrontVector[1] + UpVector[2] * FrontVector[2]);
			else
				UpVector[2] = -(UpVector[0] * FrontVector[0] + UpVector[1] * FrontVector[1]);
		}
		else
		{
			if (fabs(FrontVector[1]) < fabs(FrontVector[2]))
				UpVector[1] = -(UpVector[0] * FrontVector[0] + UpVector[2] * FrontVector[2]);
			else
				UpVector[2] = -(UpVector[0] * FrontVector[0] + UpVector[1] * FrontVector[1]);
		}

		mWorldLight = lcMatrix44LookAt(mPosition, mTargetPosition, UpVector);
	}
}

void lcLight::DrawInterface(lcContext* Context, const lcScene& Scene) const
{
	Q_UNUSED(Scene);
	Context->SetMaterial(lcMaterialType::UnlitColor);

	switch (mLightType)
	{
	case lcLightType::Point:
		DrawPointLight(Context);
		break;

	case lcLightType::Spot:
		DrawSpotLight(Context);
		break;

	case lcLightType::Directional:
		DrawDirectionalLight(Context);
		break;

	case lcLightType::Area:
		DrawAreaLight(Context);
		break;

	case lcLightType::Count:
		break;
	}
}

void lcLight::DrawPointLight(lcContext* Context) const
{
	Context->SetWorldMatrix(lcMatrix44Translation(mPosition));

	const lcPreferences& Preferences = lcGetPreferences();

	if (IsFocused(LC_LIGHT_SECTION_POSITION))
	{
		const lcVector4 FocusedColor = lcVector4FromColor(Preferences.mObjectFocusedColor);
		Context->SetColor(FocusedColor);
	}
	else if (IsSelected(LC_LIGHT_SECTION_POSITION))
	{
		const lcVector4 SelectedColor = lcVector4FromColor(Preferences.mObjectSelectedColor);
		Context->SetColor(SelectedColor);
	}
	else
	{
		const lcVector4 LightColor = lcVector4FromColor(Preferences.mLightColor);
		Context->SetColor(LightColor);
	}

	DrawSphere(Context, LC_LIGHT_SPHERE_RADIUS);
}

void lcLight::DrawSpotLight(lcContext* Context) const
{
	constexpr int ConeEdges = 8;
	float TargetDistance = SetupLightMatrix(Context);

	float Verts[(ConeEdges + 1) * 3];
	float* CurVert = Verts;

	for (int EdgeIndex = 0; EdgeIndex < ConeEdges; EdgeIndex++)
	{
		float c = cosf((float)EdgeIndex / ConeEdges * LC_2PI) * LC_LIGHT_SPOT_CONE_RADIUS;
		float s = sinf((float)EdgeIndex / ConeEdges * LC_2PI) * LC_LIGHT_SPOT_CONE_RADIUS;

		*CurVert++ = c;
		*CurVert++ = s;
		*CurVert++ = 0.0f;
	}

	*CurVert++ = 0.0f;
	*CurVert++ = 0.0f;
	*CurVert++ = LC_LIGHT_SPOT_CONE_HEIGHT;

	Context->SetVertexBufferPointer(Verts);
	Context->SetVertexFormatPosition(3);

	const GLushort Indices[(ConeEdges + 4) * 2] =
	{
		0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 0,
		0, 8, 2, 8, 4, 8, 6, 8,
	};

	Context->SetIndexBufferPointer(Indices);

	Context->DrawIndexedPrimitives(GL_LINES, (ConeEdges + 4) * 2, GL_UNSIGNED_SHORT, 0);

	DrawTarget(Context, TargetDistance);

	if (IsSelected())
		DrawCone(Context, TargetDistance);
}

void lcLight::DrawDirectionalLight(lcContext* Context) const
{
	float TargetDistance = SetupLightMatrix(Context);

	DrawCylinder(Context, LC_LIGHT_DIRECTIONAL_RADIUS, LC_LIGHT_DIRECTIONAL_HEIGHT);

	DrawTarget(Context, TargetDistance);
}

void lcLight::DrawAreaLight(lcContext* Context) const
{
	float TargetDistance = SetupLightMatrix(Context);

	if (mLightShape == LC_LIGHT_SHAPE_SQUARE || mLightShape == LC_LIGHT_SHAPE_RECTANGLE)
	{
		float Verts[4 * 3];
		float* CurVert = Verts;

		*CurVert++ = -mAreaSize.x / 2.0f;
		*CurVert++ = -mAreaSize.y / 2.0f;
		*CurVert++ = 0.0f;

		*CurVert++ =  mAreaSize.x / 2.0f;
		*CurVert++ = -mAreaSize.y / 2.0f;
		*CurVert++ = 0.0f;

		*CurVert++ =  mAreaSize.x / 2.0f;
		*CurVert++ =  mAreaSize.y / 2.0f;
		*CurVert++ = 0.0f;

		*CurVert++ = -mAreaSize.x / 2.0f;
		*CurVert++ =  mAreaSize.y / 2.0f;
		*CurVert++ = 0.0f;

		Context->SetVertexBufferPointer(Verts);
		Context->SetVertexFormatPosition(3);

		const GLushort Indices[(4 + 2) * 2] =
		{
			0, 1, 1, 2, 2, 3, 3, 0,
			0, 2, 1, 3,
		};

		Context->SetIndexBufferPointer(Indices);

		Context->DrawIndexedPrimitives(GL_LINES, (4 + 2) * 2, GL_UNSIGNED_SHORT, 0);
	}
	else
	{
		constexpr int CircleEdges = 16;

		float Verts[CircleEdges * 3];
		float* CurVert = Verts;

		for (int EdgeIndex = 0; EdgeIndex < CircleEdges; EdgeIndex++)
		{
			float c = cosf((float)EdgeIndex / CircleEdges * LC_2PI) * mAreaSize.x / 2.0f;
			float s = sinf((float)EdgeIndex / CircleEdges * LC_2PI) * mAreaSize.y / 2.0f;

			*CurVert++ = c;
			*CurVert++ = s;
			*CurVert++ = 0.0f;
		}

		Context->SetVertexBufferPointer(Verts);
		Context->SetVertexFormatPosition(3);

		const GLushort Indices[(CircleEdges + 2) * 2] =
		{
			0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8,
			8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 0,
			0, 8, 4, 12
		};

		Context->SetIndexBufferPointer(Indices);

		Context->DrawIndexedPrimitives(GL_LINES, (CircleEdges + 2) * 2, GL_UNSIGNED_SHORT, 0);
	}

	DrawTarget(Context, TargetDistance);

	float Verts[10 * 3];
	float* CurVert = Verts;

	*CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE + 25.0f; *CurVert++ =  LC_LIGHT_TARGET_EDGE;
	*CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE + 25.0f; *CurVert++ =  LC_LIGHT_TARGET_EDGE;
	*CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE + 25.0f; *CurVert++ =  LC_LIGHT_TARGET_EDGE;
	*CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE + 25.0f; *CurVert++ =  LC_LIGHT_TARGET_EDGE;
	*CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE + 25.0f; *CurVert++ = -LC_LIGHT_TARGET_EDGE;
	*CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE + 25.0f; *CurVert++ = -LC_LIGHT_TARGET_EDGE;
	*CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE + 25.0f; *CurVert++ = -LC_LIGHT_TARGET_EDGE;
	*CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE + 25.0f; *CurVert++ = -LC_LIGHT_TARGET_EDGE;

	*CurVert++ = 0.0f; *CurVert++ =  0.0f; *CurVert++ = 0.0f;
	*CurVert++ = 0.0f; *CurVert++ = 25.0f; *CurVert++ = 0.0f;

	Context->SetVertexBufferPointer(Verts);
	Context->SetVertexFormatPosition(3);

	const GLushort Indices[(12 + 1) * 2] =
	{
		0, 1, 1, 2, 2, 3, 3, 0,
		4, 5, 5, 6, 6, 7, 7, 4,
		0, 4, 1, 5, 2, 6, 3, 7,
		8, 9
	};

	Context->SetIndexBufferPointer(Indices);

	const lcPreferences& Preferences = lcGetPreferences();
	const float LineWidth = Preferences.mLineWidth;
	const lcVector4 LightColor = lcVector4FromColor(Preferences.mLightColor);

	if (IsSelected(LC_LIGHT_SECTION_UPVECTOR))
	{
		const lcVector4 SelectedColor = lcVector4FromColor(Preferences.mObjectSelectedColor);
		const lcVector4 FocusedColor = lcVector4FromColor(Preferences.mObjectFocusedColor);

		Context->SetLineWidth(2.0f * LineWidth);

		if (IsFocused(LC_LIGHT_SECTION_UPVECTOR))
			Context->SetColor(FocusedColor);
		else
			Context->SetColor(SelectedColor);
	}
	else
	{
		Context->SetLineWidth(LineWidth);
		Context->SetColor(LightColor);
	}

	Context->DrawIndexedPrimitives(GL_LINES, 12 * 2, GL_UNSIGNED_SHORT, 0);

	Context->SetLineWidth(LineWidth);
	Context->SetColor(LightColor);

	Context->DrawIndexedPrimitives(GL_LINES, 2, GL_UNSIGNED_SHORT, 12 * 2 * 2);
}

float lcLight::SetupLightMatrix(lcContext* Context) const
{
	lcVector3 FrontVector(mTargetPosition - mPosition);
	lcVector3 UpVector(1, 1, 1);

	if (IsAreaLight())
	{
		UpVector = mUpVector;
	}
	else
	{
		if (fabs(FrontVector[0]) < fabs(FrontVector[1]))
		{
			if (fabs(FrontVector[0]) < fabs(FrontVector[2]))
				UpVector[0] = -(UpVector[1] * FrontVector[1] + UpVector[2] * FrontVector[2]);
			else
				UpVector[2] = -(UpVector[0] * FrontVector[0] + UpVector[1] * FrontVector[1]);
		}
		else
		{
			if (fabs(FrontVector[1]) < fabs(FrontVector[2]))
				UpVector[1] = -(UpVector[0] * FrontVector[0] + UpVector[2] * FrontVector[2]);
			else
				UpVector[2] = -(UpVector[0] * FrontVector[0] + UpVector[1] * FrontVector[1]);
		}
	}

	lcMatrix44 LightMatrix = lcMatrix44LookAt(mPosition, mTargetPosition, UpVector);
	LightMatrix = lcMatrix44AffineInverse(LightMatrix);
	LightMatrix.SetTranslation(lcVector3(0, 0, 0));

	const lcMatrix44 LightViewMatrix = lcMul(LightMatrix, lcMatrix44Translation(mPosition));
	Context->SetWorldMatrix(LightViewMatrix);

	const lcPreferences& Preferences = lcGetPreferences();
	const float LineWidth = Preferences.mLineWidth;

	if (IsSelected(LC_LIGHT_SECTION_POSITION))
	{
		const lcVector4 SelectedColor = lcVector4FromColor(Preferences.mObjectSelectedColor);
		const lcVector4 FocusedColor = lcVector4FromColor(Preferences.mObjectFocusedColor);

		Context->SetLineWidth(2.0f * LineWidth);

		if (IsFocused(LC_LIGHT_SECTION_POSITION))
			Context->SetColor(FocusedColor);
		else
			Context->SetColor(SelectedColor);
	}
	else
	{
		const lcVector4 LightColor = lcVector4FromColor(Preferences.mLightColor);

		Context->SetLineWidth(LineWidth);
		Context->SetColor(LightColor);
	}

	return FrontVector.Length();
}

void lcLight::DrawSphere(lcContext* Context, float Radius) const
{
	constexpr int Slices = 6;
	constexpr int NumIndices = 3 * Slices + 6 * Slices * (Slices - 2) + 3 * Slices;
	constexpr int NumVertices = (Slices - 1) * Slices + 2;
	lcVector3 Vertices[NumVertices];
	quint16 Indices[NumIndices];

	lcVector3* Vertex = Vertices;
	quint16* Index = Indices;

	*Vertex++ = lcVector3(0, 0, Radius);

	for (int i = 1; i < Slices; i++)
	{
		const float r0 = Radius * sinf(i * (LC_PI / Slices));
		const float z0 = Radius * cosf(i * (LC_PI / Slices));

		for (int j = 0; j < Slices; j++)
		{
			const float x0 = r0 * sinf(j * (LC_2PI / Slices));
			const float y0 = r0 * cosf(j * (LC_2PI / Slices));

			*Vertex++ = lcVector3(x0, y0, z0);
		}
	}

	*Vertex++ = lcVector3(0, 0, -Radius);

	for (quint16 i = 0; i < Slices - 1; i++)
	{
		*Index++ = 0;
		*Index++ = 1 + i;
		*Index++ = 1 + i + 1;
	}

	*Index++ = 0;
	*Index++ = 1;
	*Index++ = 1 + Slices - 1;

	for (quint16 i = 0; i < Slices - 2; i++)
	{
		quint16 Row1 = 1 + i * Slices;
		quint16 Row2 = 1 + (i + 1) * Slices;

		for (quint16 j = 0; j < Slices - 1; j++)
		{
			*Index++ = Row1 + j;
			*Index++ = Row2 + j + 1;
			*Index++ = Row2 + j;

			*Index++ = Row1 + j;
			*Index++ = Row1 + j + 1;
			*Index++ = Row2 + j + 1;
		}

		*Index++ = Row1 + Slices - 1;
		*Index++ = Row2 + 0;
		*Index++ = Row2 + Slices - 1;

		*Index++ = Row1 + Slices - 1;
		*Index++ = Row2 + 0;
		*Index++ = Row1 + 0;
	}

	for (quint16 i = 0; i < Slices - 1; i++)
	{
		*Index++ = (Slices - 1) * Slices + 1;
		*Index++ = (Slices - 1) * (Slices - 1) + i;
		*Index++ = (Slices - 1) * (Slices - 1) + i + 1;
	}

	*Index++ = (Slices - 1) * Slices + 1;
	*Index++ = (Slices - 1) * (Slices - 1) + (Slices - 2) + 1;
	*Index++ = (Slices - 1) * (Slices - 1);

	Context->SetVertexBufferPointer(Vertices);
	Context->SetVertexFormatPosition(3);
	Context->SetIndexBufferPointer(Indices);

	Context->DrawIndexedPrimitives(GL_TRIANGLES, NumIndices, GL_UNSIGNED_SHORT, 0);
}

void lcLight::DrawCylinder(lcContext* Context, float Radius, float Height) const
{
	constexpr int Slices = 8;

	float Verts[(Slices * 2) * 3];
	float* CurVert = Verts;

	for (int EdgeIndex = 0; EdgeIndex < Slices; EdgeIndex++)
	{
		float c = cosf((float)EdgeIndex / Slices * LC_2PI) * Radius;
		float s = sinf((float)EdgeIndex / Slices * LC_2PI) * Radius;

		*CurVert++ = c;
		*CurVert++ = s;
		*CurVert++ = Height;
		*CurVert++ = c;
		*CurVert++ = s;
		*CurVert++ = 0.0f;
	}

	const GLushort Indices[48] =
	{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14, 0,
		1, 3, 3, 5, 5, 7, 7, 9, 9, 11, 11, 13, 13, 15, 15, 1,
	};

	Context->SetVertexBufferPointer(Verts);
	Context->SetVertexFormatPosition(3);
	Context->SetIndexBufferPointer(Indices);

	Context->DrawIndexedPrimitives(GL_LINES, 48, GL_UNSIGNED_SHORT, 0);
}

void lcLight::DrawTarget(lcContext* Context, float TargetDistance) const
{
	float Verts[10 * 3];
	float* CurVert = Verts;

	*CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE - TargetDistance;
	*CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE - TargetDistance;
	*CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE - TargetDistance;
	*CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE - TargetDistance;
	*CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE - TargetDistance;
	*CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE - TargetDistance;
	*CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE - TargetDistance;
	*CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE - TargetDistance;

	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = 0.0f;
	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = -TargetDistance;

	Context->SetVertexBufferPointer(Verts);
	Context->SetVertexFormatPosition(3);

	const GLushort Indices[(12 + 1) * 2] =
	{
		0, 1, 1, 2, 2, 3, 3, 0,
		4, 5, 5, 6, 6, 7, 7, 4,
		0, 4, 1, 5, 2, 6, 3, 7,
		8, 9
	};

	Context->SetIndexBufferPointer(Indices);

	const lcPreferences& Preferences = lcGetPreferences();
	const float LineWidth = Preferences.mLineWidth;
	const lcVector4 LightColor = lcVector4FromColor(Preferences.mLightColor);

	if (IsSelected(LC_LIGHT_SECTION_TARGET))
	{
		const lcVector4 SelectedColor = lcVector4FromColor(Preferences.mObjectSelectedColor);
		const lcVector4 FocusedColor = lcVector4FromColor(Preferences.mObjectFocusedColor);

		Context->SetLineWidth(2.0f * LineWidth);

		if (IsFocused(LC_LIGHT_SECTION_TARGET))
			Context->SetColor(FocusedColor);
		else
			Context->SetColor(SelectedColor);
	}
	else
	{
		Context->SetLineWidth(LineWidth);
		Context->SetColor(LightColor);
	}

	Context->DrawIndexedPrimitives(GL_LINES, 12 * 2, GL_UNSIGNED_SHORT, 0);

	Context->SetLineWidth(LineWidth);
	Context->SetColor(LightColor);

	Context->DrawIndexedPrimitives(GL_LINES, 2, GL_UNSIGNED_SHORT, 12 * 2 * 2);
}

void lcLight::DrawCone(lcContext* Context, float TargetDistance) const
{
	constexpr int ConeEdges = 16;
	const float Radius = tanf(LC_DTOR * mSpotCutoff) * TargetDistance;

	float Verts[(ConeEdges + 1) * 3];
	float* CurVert = Verts;

	for (int EdgeIndex = 0; EdgeIndex < ConeEdges; EdgeIndex++)
	{
		float c = cosf((float)EdgeIndex / ConeEdges * LC_2PI) * Radius;
		float s = sinf((float)EdgeIndex / ConeEdges * LC_2PI) * Radius;

		*CurVert++ = c;
		*CurVert++ = s;
		*CurVert++ = -TargetDistance;
	}

	*CurVert++ = 0.0f;
	*CurVert++ = 0.0f;
	*CurVert++ = 0.0f;

	Context->SetVertexBufferPointer(Verts);
	Context->SetVertexFormatPosition(3);

	const GLushort Indices[(ConeEdges + 4) * 2] =
	{
		0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8,
		8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 0,
		16, 0, 16, 4, 16, 8, 16, 12
	};

	Context->SetIndexBufferPointer(Indices);

	Context->DrawIndexedPrimitives(GL_LINES, (ConeEdges + 4) * 2, GL_UNSIGNED_SHORT, 0);
}

void lcLight::RemoveKeyFrames()
{
	mPositionKeys.RemoveAll();
	mPositionKeys.ChangeKey(mPosition, 1, true);

	mTargetPositionKeys.RemoveAll();
	mTargetPositionKeys.ChangeKey(mTargetPosition, 1, true);

	mUpVectorKeys.RemoveAll();
	mUpVectorKeys.ChangeKey(mUpVector, 1, true);

	mColorKeys.RemoveAll();
	mColorKeys.ChangeKey(mColor, 1, true);

	mAttenuationKeys.RemoveAll();
	mAttenuationKeys.ChangeKey(mAttenuation, 1, true);

	mLightFactorKeys.RemoveAll();
	mLightFactorKeys.ChangeKey(mLightFactor, 1, true);

	mLightDiffuseKeys.RemoveAll();
	mLightDiffuseKeys.ChangeKey(mLightDiffuse, 1, true);

	mLightSpecularKeys.RemoveAll();
	mLightSpecularKeys.ChangeKey(mLightSpecular, 1, true);

	mSpotSizeKeys.RemoveAll();
	mSpotSizeKeys.ChangeKey(mSpotSize, 1, false);

	mSpotCutoffKeys.RemoveAll();
	mSpotCutoffKeys.ChangeKey(mSpotCutoff, 1, true);

	mSpotExponentKeys.RemoveAll();
	mSpotExponentKeys.ChangeKey(mSpotExponent, 1, true);

	mSpotFalloffKeys.RemoveAll();
	mSpotFalloffKeys.ChangeKey(mSpotFalloff, 1, true);

	mSpotTightnessKeys.RemoveAll();
	mSpotTightnessKeys.ChangeKey(mSpotTightness, 1, true);

	mAreaGridKeys.RemoveAll();
	mAreaGridKeys.ChangeKey(mAreaGrid, 1, true);
}

bool lcLight::Setup(int LightIndex)
{
	Q_UNUSED(LightIndex);

	return true;
}
