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
#define LC_LIGHT_TARGET_EDGE 2.0f
#define LC_LIGHT_SPOT_CONE_HEIGHT 10.0f
#define LC_LIGHT_SPOT_CONE_RADIUS 7.5f
#define LC_LIGHT_DIRECTIONAL_RADIUS 5.0f
#define LC_LIGHT_DIRECTIONAL_HEIGHT 7.5f

#define LC_LIGHT_POSITION_EDGE 7.5f

static const std::array<QLatin1String, static_cast<int>(lcLightType::Count)> gLightTypes = { QLatin1String("POINT"), QLatin1String("SPOT"), QLatin1String("DIRECTIONAL"), QLatin1String("AREA") };
static const std::array<QLatin1String, static_cast<int>(lcLightAreaShape::Count)> gLightAreaShapes = { QLatin1String("RECTANGLE"), QLatin1String("SQUARE"), QLatin1String("DISK"), QLatin1String("ELLIPSE") };

lcLight::lcLight(const lcVector3& Position, lcLightType LightType)
	: lcObject(lcObjectType::Light), mLightType(LightType)
{
	mWorldMatrix = lcMatrix44Translation(Position);

	mPOVRayLight = false;
	mEnableCutoff = false;
	mAttenuation = lcVector3(1.0f, 0.0f, 0.0f);
	mLightDiffuse = 1.0f;
	mLightSpecular = 1.0f;
	mSpotExponent = 10.0f;
	mPOVRayExponent = 1.0f;
	mSpotCutoff = LightType != lcLightType::Directional ? 40.0f : 0.0f;
	mAreaGrid = lcVector2(10.0f, 10.0f);
	
	UpdateLightType();

	mPositionKeys.ChangeKey(mWorldMatrix.GetTranslation(), 1, true);
	mRotationKeys.ChangeKey(lcMatrix33(mWorldMatrix), 1, true);
	mColorKeys.ChangeKey(mColor, 1, true);
	mSpotConeAngleKeys.ChangeKey(mSpotConeAngle, 1, true);
	mSpotPenumbraAngleKeys.ChangeKey(mSpotPenumbraAngle, 1, true);
	mSpotTightnessKeys.ChangeKey(mSpotTightness, 1, true);

	mAttenuationKeys.ChangeKey(mAttenuation, 1, true);
	mLightDiffuseKeys.ChangeKey(mLightDiffuse, 1, true);
	mLightSpecularKeys.ChangeKey(mLightSpecular, 1, true);
	mSpotCutoffKeys.ChangeKey(mSpotCutoff, 1, true);
	mSpotExponentKeys.ChangeKey(mSpotExponent, 1, true);
	mAreaGridKeys.ChangeKey(mAreaGrid, 1, true);

	UpdatePosition(1);
}

void lcLight::UpdateLightType()
{
	mSizeKeys.RemoveAll();

	switch (mLightType)
	{
	case lcLightType::Point:
		mSize = lcVector2(0.0f, 0.0f);
		break;

	case lcLightType::Spot:
		mSize = lcVector2(0.0f, 0.0f);
		break;

	case lcLightType::Directional:
		mSize = lcVector2(0.00918f * LC_DTOR, 0.0f);
		break;

	case lcLightType::Area:
		mSize = lcVector2(200.0f, 200.0f);
		break;

	case lcLightType::Count:
		break;
	}

	mSizeKeys.ChangeKey(mSize, 1, true);
}

QString lcLight::GetLightTypeString(lcLightType LightType)
{
	switch (LightType)
	{
	case lcLightType::Point:
		return QT_TRANSLATE_NOOP("Light Names", "Point Light");

	case lcLightType::Spot:
		return QT_TRANSLATE_NOOP("Light Names", "Spot Light");

	case lcLightType::Directional:
		return QT_TRANSLATE_NOOP("Light Names", "Directional Light");

	case lcLightType::Area:
		return QT_TRANSLATE_NOOP("Light Names", "Area Light");

	case lcLightType::Count:
		break;
	}

	return QString();
}

QString lcLight::GetAreaShapeString(lcLightAreaShape LightAreaShape)
{
	switch (LightAreaShape)
	{
	case lcLightAreaShape::Rectangle:
		return QT_TRANSLATE_NOOP("Light Shapes", "Rectangle");

	case lcLightAreaShape::Square:
		return QT_TRANSLATE_NOOP("Light Shapes", "Square");

	case lcLightAreaShape::Disk:
		return QT_TRANSLATE_NOOP("Light Shapes", "Disk");

	case lcLightAreaShape::Ellipse:
		return QT_TRANSLATE_NOOP("Light Shapes", "Ellipse");

	case lcLightAreaShape::Count:
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

	const float* Matrix = mWorldMatrix;
	const float Numbers[12] = { Matrix[12], -Matrix[14], Matrix[13], Matrix[0], -Matrix[8], Matrix[4], -Matrix[2], Matrix[10], -Matrix[6], Matrix[1], -Matrix[9], Matrix[5] };

	if (mPositionKeys.GetSize() > 1)
		mPositionKeys.SaveKeysLDraw(Stream, "LIGHT POSITION_KEY ");
	else
		Stream << QLatin1String("0 !LEOCAD LIGHT POSITION ") << Numbers[0] << ' ' << Numbers[1] << ' ' << Numbers[2] << LineEnding;

	if (!IsPointLight())
	{
		if (mRotationKeys.GetSize() > 1)
			mRotationKeys.SaveKeysLDraw(Stream, "LIGHT ROTATION_KEY ");
		else
			Stream << QLatin1String("0 !LEOCAD LIGHT ROTATION ") << Numbers[3] << ' ' << Numbers[4] << ' ' << Numbers[5] << ' ' << Numbers[6] << ' ' << Numbers[7] << ' ' << Numbers[8] << ' ' << Numbers[9] << ' ' << Numbers[10] << ' ' << Numbers[11] << LineEnding;
	}

	if (mColorKeys.GetSize() > 1)
		mColorKeys.SaveKeysLDraw(Stream, "LIGHT COLOR_KEY ");
	else
		Stream << QLatin1String("0 !LEOCAD LIGHT COLOR ") << mColor[0] << ' ' << mColor[1] << ' ' << mColor[2] << LineEnding;

	if (mSizeKeys.GetSize() > 1)
		mSizeKeys.SaveKeysLDraw(Stream, "LIGHT SIZE_KEY ");
	else
		Stream << QLatin1String("0 !LEOCAD LIGHT SIZE ") << mSize[0] << mSize[1] << LineEnding;

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
	case lcLightType::Point:
		break;

	case lcLightType::Spot:
		if (mSpotConeAngleKeys.GetSize() > 1)
			mSpotConeAngleKeys.SaveKeysLDraw(Stream, "LIGHT SPOT_CONE_ANGLE_KEY ");
		else
			Stream << QLatin1String("0 !LEOCAD LIGHT SPOT_CONE_ANGLE ") << mSpotConeAngle << LineEnding;

		if (mSpotPenumbraAngleKeys.GetSize() > 1)
			mSpotPenumbraAngleKeys.SaveKeysLDraw(Stream, "LIGHT SPOT_PENUMBRA_ANGLE_KEY ");
		else
			Stream << QLatin1String("0 !LEOCAD LIGHT SPOT_PENUMBRA_ANGLE ") << mSpotPenumbraAngle << LineEnding;

		if (mSpotTightnessKeys.GetSize() > 1)
			mSpotTightnessKeys.SaveKeysLDraw(Stream, "SPOT_TIGHTNESS_KEY ");
		else
			Stream << QLatin1String("0 !LEOCAD LIGHT SPOT_TIGHTNESS ") << mSpotTightness << LineEnding;

		break;

	case lcLightType::Directional:
		if (mSpotExponentKeys.GetSize() > 1)
			mSpotExponentKeys.SaveKeysLDraw(Stream, "LIGHT STRENGTH_KEY ");
		else
			Stream << QLatin1String("0 !LEOCAD LIGHT STRENGTH ") << mSpotExponent << LineEnding;

		break;

	case lcLightType::Area:
		if (mPOVRayLight)
		{
			if (mAreaGridKeys.GetSize() > 1)
				mAreaGridKeys.SaveKeysLDraw(Stream, "LIGHT AREA_GRID_KEY ");
			else
				Stream << QLatin1String("0 !LEOCAD LIGHT AREA_ROWS ") << mAreaGrid[0] << QLatin1String(" AREA_COLUMNS ") << mAreaGrid[1] << LineEnding;
		}

		Stream << QLatin1String("0 !LEOCAD LIGHT AREA_SHAPE ") << gLightAreaShapes[static_cast<int>(mAreaShape)] << LineEnding;

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
		Prefix = QLatin1String("Spot Light ");
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
			lcVector3 Position;
			Stream >> Position[0] >> Position[1] >> Position[2];

			Position = lcVector3LDrawToLeoCAD(Position);
			mWorldMatrix.SetTranslation(Position);
			mPositionKeys.ChangeKey(Position, 1, true);
		}
		else if (Token == QLatin1String("POSITION_KEY"))
			mPositionKeys.LoadKeysLDraw(Stream); // todo: convert from ldraw
		else if (Token == QLatin1String("ROTATION"))
		{
			float Numbers[9];

			for (int TokenIdx = 0; TokenIdx < 9; TokenIdx++)
				Stream >> Numbers[TokenIdx];

			float* Matrix = mWorldMatrix;

			Matrix[0] =  Numbers[0];
			Matrix[8] = -Numbers[1];
			Matrix[4] =  Numbers[2];
			Matrix[2] = -Numbers[3];
			Matrix[10] = Numbers[4];
			Matrix[6] = -Numbers[5];
			Matrix[1] =  Numbers[6];
			Matrix[9] = -Numbers[7];
			Matrix[5] =  Numbers[8];
		
			mRotationKeys.ChangeKey(lcMatrix33(mWorldMatrix), 1, true);
		}
		else if (Token == QLatin1String("ROTATION_KEY"))
			mRotationKeys.LoadKeysLDraw(Stream); // todo: convert from ldraw
		else if (Token == QLatin1String("COLOR"))
		{
			Stream >> mColor[0] >> mColor[1] >> mColor[2];
			mColorKeys.ChangeKey(mColor, 1, true);
		}
		else if (Token == QLatin1String("COLOR_KEY"))
			mColorKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("SPOT_CONE_ANGLE"))
		{
			Stream >> mSpotConeAngle;
			mSpotConeAngleKeys.ChangeKey(mSpotConeAngle, 1, true);
		}
		else if (Token == QLatin1String("SPOT_CONE_ANGLE_KEY"))
			mSpotConeAngleKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("SPOT_PENUMBRA_ANGLE"))
		{
			Stream >> mSpotPenumbraAngle;
			mSpotPenumbraAngleKeys.ChangeKey(mSpotPenumbraAngle, 1, true);
		}
		else if (Token == QLatin1String("SPOT_PENUMBRA_ANGLE_KEY"))
			mSpotPenumbraAngleKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("SPOT_TIGHTNESS"))
		{
			mPOVRayLight = true;
			Stream >> mSpotTightness;
			mSpotTightnessKeys.ChangeKey(mSpotTightness, 1, true);
		}
		else if (Token == QLatin1String("SPOT_TIGHTNESS_KEY"))
			mSpotTightnessKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("AREA_SHAPE"))
		{
			QString AreaShape;
			Stream >> AreaShape;

			for (size_t ShapeIndex = 0; ShapeIndex < gLightAreaShapes.size(); ShapeIndex++)
			{
				if (AreaShape == gLightAreaShapes[ShapeIndex])
				{
					mAreaShape = static_cast<lcLightAreaShape>(ShapeIndex);
					break;
				}
			}
		}
		else if (Token == QLatin1String("SIZE"))
		{
			Stream >> mSize[0] >> mSize[1];
			mSizeKeys.ChangeKey(mSize, 1, true);
		}
		else if (Token == QLatin1String("SIZE_KEY"))
			mSizeKeys.LoadKeysLDraw(Stream);

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
		else if (Token == QLatin1String("AREA_GRID_KEY"))
			mAreaGridKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("DIFFUSE_KEY"))
			mLightDiffuseKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("SPECULAR_KEY"))
			mLightSpecularKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("CUTOFF_DISTANCE_KEY"))
			mSpotCutoffKeys.LoadKeysLDraw(Stream);
		else if (Token == QLatin1String("NAME"))
		{
			mName = Stream.readAll().trimmed();
			mName.replace("\"", "");

			// Set default settings per light type
			switch (mLightType)
			{
			case lcLightType::Point:
			case lcLightType::Spot:
				break;

			case lcLightType::Directional:
				if (!mSpotCutoffSet)
				{
					mSpotCutoff = 0.0f;
					mSpotCutoffKeys.ChangeKey(mSpotCutoff, 1, true);
				}
				break;

			case lcLightType::Area:
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
	const lcVector3 Point = mWorldMatrix.GetTranslation();

	// TODO: this should check the entire mesh

	Min = lcMin(Point, Min);
	Max = lcMax(Point, Max);
}

void lcLight::UpdateLight(lcStep Step, lcLightProperties Props, int Property)
{
	switch(Property)
	{
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

		if (lcSphereRayMinIntersectDistance(mWorldMatrix.GetTranslation(), LC_LIGHT_SPHERE_RADIUS, ObjectRayTest.Start, ObjectRayTest.End, &Distance) && (Distance < ObjectRayTest.Distance))
		{
			ObjectRayTest.ObjectSection.Object = const_cast<lcLight*>(this);
			ObjectRayTest.ObjectSection.Section = LC_LIGHT_SECTION_POSITION;
			ObjectRayTest.Distance = Distance;
		}

		return;
	}

	if (mLightType == lcLightType::Spot)
	{
		const lcVector3 Direction = -lcVector3(mWorldMatrix[2]);
		const lcVector3 Position = mWorldMatrix.GetTranslation() - Direction * LC_LIGHT_SPOT_CONE_HEIGHT;
		float Distance;

		if (lcConeRayMinIntersectDistance(Position, Direction, LC_LIGHT_SPOT_CONE_RADIUS, LC_LIGHT_SPOT_CONE_HEIGHT, ObjectRayTest.Start, ObjectRayTest.End, &Distance) && (Distance < ObjectRayTest.Distance))
		{
			ObjectRayTest.ObjectSection.Object = const_cast<lcLight*>(this);
			ObjectRayTest.ObjectSection.Section = LC_LIGHT_SECTION_POSITION;
			ObjectRayTest.Distance = Distance;
		}
	}
	else if (mLightType == lcLightType::Area)
	{
		const lcVector3 Direction = -lcVector3(mWorldMatrix[2]);
		const lcVector3 Position = mWorldMatrix.GetTranslation();
		const lcVector4 Plane(Direction, -lcDot(Direction, Position));
		lcVector3 Intersection;

		if (lcLineSegmentPlaneIntersection(&Intersection, ObjectRayTest.Start, ObjectRayTest.End, Plane))
		{
			const lcVector3 XAxis = lcVector3(mWorldMatrix[0]);
			const lcVector3 YAxis = lcVector3(mWorldMatrix[1]);
			lcVector3 IntersectionDirection = Intersection - Position;

			float x = lcDot(IntersectionDirection, XAxis);
			float y = lcDot(IntersectionDirection, YAxis);

			if (fabsf(x) < mSize.x / 2.0f && fabsf(y) < mSize.y / 2.0f)
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

	const lcMatrix44 InverseWorldMatrix = lcMatrix44AffineInverse(mWorldMatrix);
	lcVector3 Start = lcMul31(ObjectRayTest.Start, InverseWorldMatrix);
	lcVector3 End = lcMul31(ObjectRayTest.End, InverseWorldMatrix);

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
}

void lcLight::BoxTest(lcObjectBoxTest& ObjectBoxTest) const
{
	if (IsPointLight())
	{
		for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
			if (lcDot3(mWorldMatrix.GetTranslation(), ObjectBoxTest.Planes[PlaneIdx]) + ObjectBoxTest.Planes[PlaneIdx][3] > LC_LIGHT_SPHERE_RADIUS)
				return;

		ObjectBoxTest.Objects.Add(const_cast<lcLight*>(this));
		return;
	}
	
	lcVector3 Min(-LC_LIGHT_POSITION_EDGE, -LC_LIGHT_POSITION_EDGE, -LC_LIGHT_POSITION_EDGE); // todo: fix light box test
	lcVector3 Max( LC_LIGHT_POSITION_EDGE,  LC_LIGHT_POSITION_EDGE,  LC_LIGHT_POSITION_EDGE);

	lcVector4 LocalPlanes[6];

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		const lcVector3 Normal = lcMul30(ObjectBoxTest.Planes[PlaneIdx], mWorldMatrix);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, ObjectBoxTest.Planes[PlaneIdx][3] - lcDot3(mWorldMatrix[3], Normal));
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
		const lcVector3 Position = mWorldMatrix.GetTranslation() + Distance;

		SetPosition(Position, Step, AddKey);

		mWorldMatrix.SetTranslation(Position);
	}
}

void lcLight::Rotate(lcStep Step, bool AddKey, const lcMatrix33& RotationMatrix, const lcVector3& Center, const lcMatrix33& RotationFrame)
{
	if (IsPointLight())
		return;

	if (GetFocusSection() != LC_LIGHT_SECTION_POSITION)
		return;

	lcVector3 Distance = mWorldMatrix.GetTranslation() - Center;
	const lcMatrix33 LocalToWorldMatrix = lcMatrix33(mWorldMatrix);

	const lcMatrix33 LocalToFocusMatrix = lcMul(LocalToWorldMatrix, RotationFrame);
	lcMatrix33 NewLocalToWorldMatrix = lcMul(LocalToFocusMatrix, RotationMatrix);

	const lcMatrix33 WorldToLocalMatrix = lcMatrix33AffineInverse(LocalToWorldMatrix);

	Distance = lcMul(Distance, WorldToLocalMatrix);
	Distance = lcMul(Distance, NewLocalToWorldMatrix);

	NewLocalToWorldMatrix.Orthonormalize();

	SetPosition(Center + Distance, Step, AddKey);
	SetRotation(NewLocalToWorldMatrix, Step, AddKey);
}

bool lcLight::SetLightType(lcLightType LightType)
{
	if (static_cast<int>(LightType) < 0 || LightType >= lcLightType::Count)
		return false;

	if (mLightType == LightType)
		return false;

	mLightType = LightType;

	UpdateLightType();

	return true;
}

void lcLight::SetColor(const lcVector3& Color, lcStep Step, bool AddKey)
{
	mColorKeys.ChangeKey(Color, Step, AddKey);
}

void lcLight::SetSpotConeAngle(float Angle, lcStep Step, bool AddKey)
{
	mSpotConeAngleKeys.ChangeKey(Angle, Step, AddKey);
}

void lcLight::SetSpotPenumbraAngle(float Angle, lcStep Step, bool AddKey)
{
	mSpotPenumbraAngleKeys.ChangeKey(Angle, Step, AddKey);
}

void lcLight::SetSpotTightness(float Tightness, lcStep Step, bool AddKey)
{
	mSpotTightnessKeys.ChangeKey(Tightness, Step, AddKey);
}

bool lcLight::SetAreaShape(lcLightAreaShape AreaShape)
{
	if (static_cast<int>(AreaShape) < 0 || AreaShape >= lcLightAreaShape::Count)
		return false;

	if (mAreaShape != AreaShape)
	{
		mAreaShape = AreaShape;
		return true;
	}

	return false;
}

void lcLight::SetSize(lcVector2 Size, lcStep Step, bool AddKey)
{
	if (mLightType == lcLightType::Area && (mAreaShape == lcLightAreaShape::Square || mAreaShape == lcLightAreaShape::Disk))
		Size[1] = Size[0];

	mSizeKeys.ChangeKey(Size, Step, AddKey);
}

bool lcLight::SetCastShadow(bool CastShadow)
{
	if (mCastShadow != CastShadow)
	{
		mCastShadow = CastShadow;
		return true;
	}

	return false;
}

void lcLight::InsertTime(lcStep Start, lcStep Time)
{
	mPositionKeys.InsertTime(Start, Time);
	mRotationKeys.InsertTime(Start, Time);
	mColorKeys.InsertTime(Start, Time);
	mSpotConeAngleKeys.InsertTime(Start, Time);
	mSpotPenumbraAngleKeys.InsertTime(Start, Time);
	mSpotTightnessKeys.InsertTime(Start, Time);
	mSizeKeys.InsertTime(Start, Time);

	mAttenuationKeys.InsertTime(Start, Time);
	mLightDiffuseKeys.InsertTime(Start, Time);
	mLightSpecularKeys.InsertTime(Start, Time);
	mSpotCutoffKeys.InsertTime(Start, Time);
	mSpotExponentKeys.InsertTime(Start, Time);
	mAreaGridKeys.InsertTime(Start, Time);
}

void lcLight::RemoveTime(lcStep Start, lcStep Time)
{
	mPositionKeys.RemoveTime(Start, Time);
	mRotationKeys.RemoveTime(Start, Time);
	mColorKeys.RemoveTime(Start, Time);
	mSpotConeAngleKeys.RemoveTime(Start, Time);
	mSpotPenumbraAngleKeys.RemoveTime(Start, Time);
	mSpotTightnessKeys.RemoveTime(Start, Time);
	mSizeKeys.RemoveTime(Start, Time);

	mAttenuationKeys.RemoveTime(Start, Time);
	mLightDiffuseKeys.RemoveTime(Start, Time);
	mLightSpecularKeys.RemoveTime(Start, Time);
	mSpotCutoffKeys.RemoveTime(Start, Time);
	mSpotExponentKeys.RemoveTime(Start, Time);
	mAreaGridKeys.RemoveTime(Start, Time);
}

void lcLight::UpdatePosition(lcStep Step)
{
	const lcVector3 Position = mPositionKeys.CalculateKey(Step);

	if (IsPointLight())
	{
		mWorldMatrix = lcMatrix44Translation(Position);
	}
	else
	{
		const lcMatrix33 Rotation = mRotationKeys.CalculateKey(Step);

		mWorldMatrix = lcMatrix44(Rotation, Position);
	}

	mColor = mColorKeys.CalculateKey(Step);
	mSpotConeAngle = mSpotConeAngleKeys.CalculateKey(Step);
	mSpotPenumbraAngle = mSpotPenumbraAngleKeys.CalculateKey(Step);
	mSpotTightness = mSpotTightnessKeys.CalculateKey(Step);
	mSize = mSizeKeys.CalculateKey(Step);

	mAttenuation = mAttenuationKeys.CalculateKey(Step);
	mLightDiffuse = mLightDiffuseKeys.CalculateKey(Step);
	mLightSpecular = mLightSpecularKeys.CalculateKey(Step);
	mSpotCutoff = mSpotCutoffKeys.CalculateKey(Step);
	mSpotExponent = mSpotExponentKeys.CalculateKey(Step);
	mAreaGrid = mAreaGridKeys.CalculateKey(Step);
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
	SetupLightMatrix(Context);

	DrawSphere(Context, LC_LIGHT_SPHERE_RADIUS);
}

void lcLight::DrawSpotLight(lcContext* Context) const
{
	SetupLightMatrix(Context);

	constexpr int ConeEdges = 8;
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

	const float TargetDistance = 250.0f;

	DrawTarget(Context, TargetDistance);

	if (IsSelected())
		DrawCone(Context, TargetDistance);
}

void lcLight::DrawDirectionalLight(lcContext* Context) const
{
	SetupLightMatrix(Context);

	DrawCylinder(Context, LC_LIGHT_DIRECTIONAL_RADIUS, LC_LIGHT_DIRECTIONAL_HEIGHT);

	const float TargetDistance = 25.0f;

	DrawTarget(Context, TargetDistance);
}

void lcLight::DrawAreaLight(lcContext* Context) const
{
	SetupLightMatrix(Context);

	if (mAreaShape == lcLightAreaShape::Square || mAreaShape == lcLightAreaShape::Rectangle)
	{
		float Verts[4 * 3];
		float* CurVert = Verts;

		*CurVert++ = -mSize.x / 2.0f;
		*CurVert++ = -mSize.y / 2.0f;
		*CurVert++ = 0.0f;

		*CurVert++ =  mSize.x / 2.0f;
		*CurVert++ = -mSize.y / 2.0f;
		*CurVert++ = 0.0f;

		*CurVert++ =  mSize.x / 2.0f;
		*CurVert++ =  mSize.y / 2.0f;
		*CurVert++ = 0.0f;

		*CurVert++ = -mSize.x / 2.0f;
		*CurVert++ =  mSize.y / 2.0f;
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
			float c = cosf((float)EdgeIndex / CircleEdges * LC_2PI) * mSize.x / 2.0f;
			float s = sinf((float)EdgeIndex / CircleEdges * LC_2PI) * mSize.y / 2.0f;

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

	const float TargetDistance = 25.0f;

	DrawTarget(Context, TargetDistance);
}

void lcLight::SetupLightMatrix(lcContext* Context) const
{
	Context->SetWorldMatrix(mWorldMatrix);

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

	Context->DrawIndexedPrimitives(GL_LINES, (12 + 1) * 2, GL_UNSIGNED_SHORT, 0);
}

void lcLight::DrawCone(lcContext* Context, float TargetDistance) const
{
	constexpr int ConeEdges = 16;
	const float OuterRadius = tanf(LC_DTOR * mSpotConeAngle / 2.0f) * TargetDistance;

	float Verts[(ConeEdges * 2 + 1) * 3];
	float* CurVert = Verts;

	for (int EdgeIndex = 0; EdgeIndex < ConeEdges; EdgeIndex++)
	{
		const float c = cosf((float)EdgeIndex / ConeEdges * LC_2PI);
		const float s = sinf((float)EdgeIndex / ConeEdges * LC_2PI);

		*CurVert++ = c * OuterRadius;
		*CurVert++ = s * OuterRadius;
		*CurVert++ = -TargetDistance;
	}

	*CurVert++ = 0.0f;
	*CurVert++ = 0.0f;
	*CurVert++ = 0.0f;

	const bool DrawPenumbra = mSpotPenumbraAngle > 1.0f;

	if (DrawPenumbra)
	{
		const float InnerRadius = tanf(LC_DTOR * (mSpotConeAngle / 2.0f - mSpotPenumbraAngle)) * TargetDistance;

		for (int EdgeIndex = 0; EdgeIndex < ConeEdges; EdgeIndex++)
		{
			const float c = cosf((float)EdgeIndex / ConeEdges * LC_2PI);
			const float s = sinf((float)EdgeIndex / ConeEdges * LC_2PI);

			*CurVert++ = c * InnerRadius;
			*CurVert++ = s * InnerRadius;
			*CurVert++ = -TargetDistance;
		}
	}

	Context->SetVertexBufferPointer(Verts);
	Context->SetVertexFormatPosition(3);

	constexpr GLushort Indices[(ConeEdges * 2 + 4) * 2] =
	{
		0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8,
		8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 0,
		16, 0, 16, 4, 16, 8, 16, 12,
		17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23, 24, 24, 25,
		25, 26, 26, 27, 27, 28, 28, 29, 29, 30, 30, 31, 31, 32, 32, 17
	};

	Context->SetIndexBufferPointer(Indices);

	Context->DrawIndexedPrimitives(GL_LINES, DrawPenumbra ? (ConeEdges * 2 + 4) * 2 : (ConeEdges + 4) * 2, GL_UNSIGNED_SHORT, 0);
}

void lcLight::RemoveKeyFrames()
{
	mPositionKeys.RemoveAll();
	mPositionKeys.ChangeKey(mWorldMatrix.GetTranslation(), 1, true);

	mRotationKeys.RemoveAll();
	mRotationKeys.ChangeKey(lcMatrix33(mWorldMatrix), 1, true);

	mColorKeys.RemoveAll();
	mColorKeys.ChangeKey(mColor, 1, true);

	mSpotConeAngleKeys.RemoveAll();
	mSpotConeAngleKeys.ChangeKey(mSpotConeAngle, 1, false);

	mSpotPenumbraAngleKeys.RemoveAll();
	mSpotPenumbraAngleKeys.ChangeKey(mSpotPenumbraAngle, 1, true);

	mSpotTightnessKeys.RemoveAll();
	mSpotTightnessKeys.ChangeKey(mSpotTightness, 1, true);

	mSizeKeys.RemoveAll();
	mSizeKeys.ChangeKey(mSize, 1, true);

	mAttenuationKeys.RemoveAll();
	mAttenuationKeys.ChangeKey(mAttenuation, 1, true);

	mLightDiffuseKeys.RemoveAll();
	mLightDiffuseKeys.ChangeKey(mLightDiffuse, 1, true);

	mLightSpecularKeys.RemoveAll();
	mLightSpecularKeys.ChangeKey(mLightSpecular, 1, true);

	mSpotCutoffKeys.RemoveAll();
	mSpotCutoffKeys.ChangeKey(mSpotCutoff, 1, true);

	mSpotExponentKeys.RemoveAll();
	mSpotExponentKeys.ChangeKey(mSpotExponent, 1, true);

	mAreaGridKeys.RemoveAll();
	mAreaGridKeys.ChangeKey(mAreaGrid, 1, true);
}

bool lcLight::Setup(int LightIndex)
{
	Q_UNUSED(LightIndex);

	return true;
}
