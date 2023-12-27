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
#define LC_LIGHT_TARGET_RADIUS 2.5f
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

	UpdateLightType();

	mPositionKeys.ChangeKey(mWorldMatrix.GetTranslation(), 1, true);
	mRotationKeys.ChangeKey(lcMatrix33(mWorldMatrix), 1, true);
	mColorKeys.ChangeKey(mColor, 1, true);
	mPowerKeys.ChangeKey(mPower, 1, true);
	mAttenuationDistanceKeys.ChangeKey(mAttenuationDistance, 1, true);
	mAttenuationPowerKeys.ChangeKey(mAttenuationPower, 1, true);
	mSpotConeAngleKeys.ChangeKey(mSpotConeAngle, 1, true);
	mSpotPenumbraAngleKeys.ChangeKey(mSpotPenumbraAngle, 1, true);
	mSpotTightnessKeys.ChangeKey(mSpotTightness, 1, true);
	mAreaGridKeys.ChangeKey(mAreaGrid, 1, true);

	UpdatePosition(1);
}

void lcLight::UpdateLightType()
{
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

	mSizeKeys.Reset(mSize);
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

	if (!mCastShadow)
		Stream << QLatin1String("0 !LEOCAD LIGHT SHADOWLESS") << LineEnding;

	const float* Matrix = mWorldMatrix;
	const float Numbers[12] = { Matrix[12], -Matrix[14], Matrix[13], Matrix[0], -Matrix[8], Matrix[4], -Matrix[2], Matrix[10], -Matrix[6], Matrix[1], -Matrix[9], Matrix[5] };

	if (mPositionKeys.GetSize() > 1)
		mPositionKeys.SaveKeysLDraw(Stream, "LIGHT", "POSITION");
	else
		Stream << QLatin1String("0 !LEOCAD LIGHT POSITION ") << Numbers[0] << ' ' << Numbers[1] << ' ' << Numbers[2] << LineEnding;

	if (!IsPointLight())
	{
		if (mRotationKeys.GetSize() > 1)
			mRotationKeys.SaveKeysLDraw(Stream, "LIGHT", "ROTATION");
		else
			Stream << QLatin1String("0 !LEOCAD LIGHT ROTATION ") << Numbers[3] << ' ' << Numbers[4] << ' ' << Numbers[5] << ' ' << Numbers[6] << ' ' << Numbers[7] << ' ' << Numbers[8] << ' ' << Numbers[9] << ' ' << Numbers[10] << ' ' << Numbers[11] << LineEnding;
	}

	SaveAttribute(Stream, mColor, mColorKeys, "LIGHT", "COLOR");
	SaveAttribute(Stream, mSize, mSizeKeys, "LIGHT", "SIZE");
	SaveAttribute(Stream, mPower, mPowerKeys, "LIGHT", "POWER");
	SaveAttribute(Stream, mAttenuationDistance, mAttenuationDistanceKeys, "LIGHT", "ATTENUATION_DISTANCE");
	SaveAttribute(Stream, mAttenuationPower, mAttenuationPowerKeys, "LIGHT", "ATTENUATION_POWER");

	switch (mLightType)
	{
	case lcLightType::Count:
	case lcLightType::Point:
		break;

	case lcLightType::Spot:
		SaveAttribute(Stream, mSpotConeAngle, mSpotConeAngleKeys, "LIGHT", "SPOT_CONE_ANGLE");
		SaveAttribute(Stream, mSpotPenumbraAngle, mSpotPenumbraAngleKeys, "LIGHT", "SPOT_PENUMBRA_ANGLE");
		SaveAttribute(Stream, mSpotTightness, mSpotTightnessKeys, "LIGHT", "SPOT_TIGHTNESS");
		break;

	case lcLightType::Directional:
		break;

	case lcLightType::Area:
		Stream << QLatin1String("0 !LEOCAD LIGHT AREA_SHAPE ") << gLightAreaShapes[static_cast<int>(mAreaShape)] << LineEnding;
		SaveAttribute(Stream, mAreaGrid, mAreaGridKeys, "LIGHT", "AREA_GRID");
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
		else if (LoadAttribute(Stream, Token, mColor, mColorKeys, "COLOR"))
			continue;
		else if (LoadAttribute(Stream, Token, mSize, mSizeKeys, "SIZE"))
			continue;
		else if (LoadAttribute(Stream, Token, mPower, mPowerKeys, "POWER"))
			continue;
		else if (LoadAttribute(Stream, Token, mAttenuationDistance, mAttenuationDistanceKeys, "ATTENUATION_DISTANCE"))
			continue;
		else if (LoadAttribute(Stream, Token, mAttenuationPower, mAttenuationPowerKeys, "ATTENUATION_POWER"))
			continue;
		else if (LoadAttribute(Stream, Token, mSpotConeAngle, mSpotConeAngleKeys, "SPOT_CONE_ANGLE"))
			continue;
		else if (LoadAttribute(Stream, Token, mSpotPenumbraAngle, mSpotPenumbraAngleKeys, "SPOT_PENUMBRA_ANGLE"))
			continue;
		else if (LoadAttribute(Stream, Token, mSpotTightness, mSpotTightnessKeys, "SPOT_TIGHTNESS"))
			continue;
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
		else if (LoadAttribute(Stream, Token, mAreaGrid, mAreaGridKeys, "AREA_GRID"))
			continue;
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
		else if (Token == QLatin1String("SHADOWLESS"))
		{
			mCastShadow = false;
		}
		else if (Token == QLatin1String("NAME"))
		{
			mName = Stream.readAll().trimmed();
			mName.replace("\"", "");

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

	if (IsSelected())
	{
		if (lcSphereRayMinIntersectDistance(lcMul31(lcVector3(0,0,-mTargetDistance), mWorldMatrix), LC_LIGHT_TARGET_RADIUS, ObjectRayTest.Start, ObjectRayTest.End, &Distance) && (Distance < ObjectRayTest.Distance))
		{
			ObjectRayTest.ObjectSection.Object = const_cast<lcLight*>(this);
			ObjectRayTest.ObjectSection.Section = LC_LIGHT_SECTION_TARGET;
			ObjectRayTest.Distance = Distance;
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

void lcLight::MoveSelected(lcStep Step, bool AddKey, const lcVector3& Distance, bool FirstMove)
{
	const quint32 Section = GetFocusSection();

	if (Section == LC_LIGHT_SECTION_POSITION || Section == LC_LIGHT_SECTION_INVALID)
	{
		const lcVector3 Position = mWorldMatrix.GetTranslation() + Distance;

		SetPosition(Position, Step, AddKey);

		mWorldMatrix.SetTranslation(Position);
	}
	else
	{
		if (FirstMove)
			mTargetMovePosition = lcMul31(lcVector3(0.0f, 0.0f, -mTargetDistance), mWorldMatrix);

		mTargetMovePosition += Distance;

		lcVector3 CurrentDirection = -lcNormalize(mTargetMovePosition - mWorldMatrix.GetTranslation());
		lcMatrix33 WorldMatrix;

		WorldMatrix.r[0] = lcCross(lcVector3(mWorldMatrix.r[1]), CurrentDirection);
		WorldMatrix.r[1] = lcCross(CurrentDirection, WorldMatrix.r[0]);
		WorldMatrix.r[2] = CurrentDirection;

		WorldMatrix.Orthonormalize();

		SetRotation(WorldMatrix, Step, AddKey);
			
		mWorldMatrix = lcMatrix44(WorldMatrix, mWorldMatrix.GetTranslation());
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

void lcLight::SetAttenuationDistance(float Distance, lcStep Step, bool AddKey)
{
	mAttenuationDistanceKeys.ChangeKey(Distance, Step, AddKey);
}

void lcLight::SetAttenuationPower(float Power, lcStep Step, bool AddKey)
{
	mAttenuationPowerKeys.ChangeKey(Power, Step, AddKey);
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

bool lcLight::SetAreaGrid(lcVector2i AreaGrid, lcStep Step, bool AddKey)
{
	mAreaGridKeys.ChangeKey(AreaGrid, Step, AddKey);

	return true;
}

void lcLight::SetSize(lcVector2 Size, lcStep Step, bool AddKey)
{
	if (mLightType == lcLightType::Area && (mAreaShape == lcLightAreaShape::Square || mAreaShape == lcLightAreaShape::Disk))
		Size[1] = Size[0];

	mSizeKeys.ChangeKey(Size, Step, AddKey);
}

void lcLight::SetPower(float Power, lcStep Step, bool AddKey)
{
	mPowerKeys.ChangeKey(Power, Step, AddKey);
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
	mAreaGridKeys.InsertTime(Start, Time);
	mSizeKeys.InsertTime(Start, Time);
	mPowerKeys.InsertTime(Start, Time);
	mAttenuationDistanceKeys.InsertTime(Start, Time);
	mAttenuationPowerKeys.InsertTime(Start, Time);
}

void lcLight::RemoveTime(lcStep Start, lcStep Time)
{
	mPositionKeys.RemoveTime(Start, Time);
	mRotationKeys.RemoveTime(Start, Time);
	mColorKeys.RemoveTime(Start, Time);
	mSpotConeAngleKeys.RemoveTime(Start, Time);
	mSpotPenumbraAngleKeys.RemoveTime(Start, Time);
	mSpotTightnessKeys.RemoveTime(Start, Time);
	mAreaGridKeys.RemoveTime(Start, Time);
	mSizeKeys.RemoveTime(Start, Time);
	mPowerKeys.RemoveTime(Start, Time);
	mAttenuationDistanceKeys.RemoveTime(Start, Time);
	mAttenuationPowerKeys.RemoveTime(Start, Time);
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
	mAreaGrid = mAreaGridKeys.CalculateKey(Step);
	mSize = mSizeKeys.CalculateKey(Step);
	mPower = mPowerKeys.CalculateKey(Step);
	mAttenuationDistance = mAttenuationDistanceKeys.CalculateKey(Step);
	mAttenuationPower = mAttenuationPowerKeys.CalculateKey(Step);
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

	DrawSphere(Context, lcVector3(0.0f, 0.0f, 0.0f), LC_LIGHT_SPHERE_RADIUS);
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

	if (IsSelected())
	{
		DrawCone(Context, mTargetDistance);
		DrawTarget(Context);
	}
}

void lcLight::DrawDirectionalLight(lcContext* Context) const
{
	SetupLightMatrix(Context);

	DrawCylinder(Context, LC_LIGHT_DIRECTIONAL_RADIUS, LC_LIGHT_DIRECTIONAL_HEIGHT);

	if (IsSelected())
		DrawTarget(Context);
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

	if (IsSelected())
		DrawTarget(Context);
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

void lcLight::DrawSphere(lcContext* Context, const lcVector3& Center, float Radius) const
{
	constexpr int Slices = 6;
	constexpr int NumIndices = 3 * Slices + 6 * Slices * (Slices - 2) + 3 * Slices;
	constexpr int NumVertices = (Slices - 1) * Slices + 2;
	lcVector3 Vertices[NumVertices];
	quint16 Indices[NumIndices];

	lcVector3* Vertex = Vertices;
	quint16* Index = Indices;

	*Vertex++ = Center + lcVector3(0, 0, Radius);

	for (int i = 1; i < Slices; i++)
	{
		const float r0 = Radius * sinf(i * (LC_PI / Slices));
		const float z0 = Radius * cosf(i * (LC_PI / Slices));

		for (int j = 0; j < Slices; j++)
		{
			const float x0 = r0 * sinf(j * (LC_2PI / Slices));
			const float y0 = r0 * cosf(j * (LC_2PI / Slices));

			*Vertex++ = Center + lcVector3(x0, y0, z0);
		}
	}

	*Vertex++ = Center + lcVector3(0, 0, -Radius);

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

void lcLight::DrawTarget(lcContext* Context) const
{
	float Verts[2 * 3];
	float* CurVert = Verts;

	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = 0.0f;
	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = -mTargetDistance;

	Context->SetVertexBufferPointer(Verts);
	Context->SetVertexFormatPosition(3);

	const GLushort Indices[2] =
	{
		0, 1
	};

	Context->SetIndexBufferPointer(Indices);

	Context->DrawIndexedPrimitives(GL_LINES, 2, GL_UNSIGNED_SHORT, 0);

	const lcPreferences& Preferences = lcGetPreferences();
	const float LineWidth = Preferences.mLineWidth;

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
		const lcVector4 LightColor = lcVector4FromColor(Preferences.mLightColor);

		Context->SetLineWidth(LineWidth);
		Context->SetColor(LightColor);
	}

	DrawSphere(Context, lcVector3(0.0f, 0.0f, -mTargetDistance), LC_LIGHT_TARGET_RADIUS);
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
	mPositionKeys.Reset(mWorldMatrix.GetTranslation());
	mRotationKeys.Reset(lcMatrix33(mWorldMatrix));
	mColorKeys.Reset(mColor);
	mSpotConeAngleKeys.Reset(mSpotConeAngle);
	mSpotPenumbraAngleKeys.Reset(mSpotPenumbraAngle);
	mSpotTightnessKeys.Reset(mSpotTightness);
	mAreaGridKeys.Reset(mAreaGrid);
	mSizeKeys.Reset(mSize);
	mPowerKeys.Reset(mPower);
	mAttenuationDistanceKeys.Reset(mAttenuationDistance);
	mAttenuationPowerKeys.Reset(mAttenuationPower);
}
