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
	mPosition.SetValue(Position);

	UpdatePosition(1);
}

QString lcLight::GetLightTypeString(lcLightType LightType)
{
	switch (LightType)
	{
	case lcLightType::Point:
		return QT_TRANSLATE_NOOP("Light Types", "Point Light");

	case lcLightType::Spot:
		return QT_TRANSLATE_NOOP("Light Types", "Spot Light");

	case lcLightType::Directional:
		return QT_TRANSLATE_NOOP("Light Types", "Directional Light");

	case lcLightType::Area:
		return QT_TRANSLATE_NOOP("Light Types", "Area Light");

	case lcLightType::Count:
		break;
	}

	return QString();
}

QStringList lcLight::GetLightTypeStrings()
{
	QStringList LightTypes;

	for (int LightTypeIndex = 0; LightTypeIndex < static_cast<int>(lcLightType::Count); LightTypeIndex++)
		LightTypes.push_back(GetLightTypeString(static_cast<lcLightType>(LightTypeIndex)));

	return LightTypes;
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

QStringList lcLight::GetAreaShapeStrings()
{
	QStringList AreaShapes;

	for (int AreaShapeIndex = 0; AreaShapeIndex < static_cast<int>(lcLightAreaShape::Count); AreaShapeIndex++)
		AreaShapes.push_back(GetAreaShapeString(static_cast<lcLightAreaShape>(AreaShapeIndex)));

	return AreaShapes;
}

void lcLight::SaveLDraw(QTextStream& Stream) const
{
	const QLatin1String LineEnding("\r\n");

	if (!mCastShadow)
		Stream << QLatin1String("0 !LEOCAD LIGHT SHADOWLESS") << LineEnding;

	mPosition.Save(Stream, "LIGHT", "POSITION", true);
	mRotation.Save(Stream, "LIGHT", "ROTATION", true);
	mColor.Save(Stream, "LIGHT", "COLOR", true);
	mPointBlenderRadius.Save(Stream, "LIGHT", "BLENDER_POINT_RADIUS", true);
	mSpotBlenderRadius.Save(Stream, "LIGHT", "BLENDER_SPOT_RADIUS", true);
	mDirectionalBlenderAngle.Save(Stream, "LIGHT", "BLENDER_DIRECTIONAL_ANGLE", true);
	mAreaSize.Save(Stream, "LIGHT", "AREA_SIZE", true);
	mBlenderPower.Save(Stream, "LIGHT", "BLENDER_POWER", true);
	mPOVRayPower.Save(Stream, "LIGHT", "POVRAY_POWER", true);
	mPOVRayFadeDistance.Save(Stream, "LIGHT", "POVRAY_FADE_DISTANCE", true);
	mPOVRayFadePower.Save(Stream, "LIGHT", "POVRAY_FADE_POWER", true);

	switch (mLightType)
	{
	case lcLightType::Count:
	case lcLightType::Point:
		break;

	case lcLightType::Spot:
		mSpotConeAngle.Save(Stream, "LIGHT", "SPOT_CONE_ANGLE", true);
		mSpotPenumbraAngle.Save(Stream, "LIGHT", "SPOT_PENUMBRA_ANGLE", true);
		mSpotPOVRayTightness.Save(Stream, "LIGHT", "POVRAY_SPOT_TIGHTNESS", true);
		break;

	case lcLightType::Directional:
		break;

	case lcLightType::Area:
		Stream << QLatin1String("0 !LEOCAD LIGHT AREA_SHAPE ") << gLightAreaShapes[static_cast<int>(mAreaShape)] << LineEnding;
		mAreaPOVRayGrid.Save(Stream, "LIGHT", "POVRAY_AREA_GRID", true);
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

		if (mPosition.Load(Stream, Token, "POSITION"))
			continue;
		else if (mRotation.Load(Stream, Token, "ROTATION"))
			continue;
		else if (mColor.Load(Stream, Token, "COLOR"))
			continue;
		else if (mPointBlenderRadius.Load(Stream, Token, "BLENDER_POINT_RADIUS"))
			continue;
		else if (mSpotBlenderRadius.Load(Stream, Token, "BLENDER_SPOT_RADIUS"))
			continue;
		else if (mDirectionalBlenderAngle.Load(Stream, Token, "BLENDER_DIRECTIONAL_ANGLE"))
			continue;
		else if (mAreaSize.Load(Stream, Token, "AREA_SIZE"))
			continue;
		else if (mBlenderPower.Load(Stream, Token, "BLENDER_POWER"))
			continue;
		else if (mPOVRayPower.Load(Stream, Token, "POVRAY_POWER"))
			continue;
		else if (mPOVRayFadeDistance.Load(Stream, Token, "POVRAY_FADE_DISTANCE"))
			continue;
		else if (mPOVRayFadePower.Load(Stream, Token, "POVRAY_FADE_POWER"))
			continue;
		else if (mSpotConeAngle.Load(Stream, Token, "SPOT_CONE_ANGLE"))
			continue;
		else if (mSpotPenumbraAngle.Load(Stream, Token, "SPOT_PENUMBRA_ANGLE"))
			continue;
		else if (mSpotPOVRayTightness.Load(Stream, Token, "POVRAY_SPOT_TIGHTNESS"))
			continue;
		else if (mAreaPOVRayGrid.Load(Stream, Token, "POVRAY_AREA_GRID"))
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
			const lcVector2& Size = mAreaSize;

			if (fabsf(x) < Size.x / 2.0f && fabsf(y) < Size.y / 2.0f)
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

bool lcLight::SetName(const QString& Name)
{
	if (mName == Name)
		return false;

	mName = Name;

	return true;
}

bool lcLight::SetLightType(lcLightType LightType)
{
	if (static_cast<int>(LightType) < 0 || LightType >= lcLightType::Count)
		return false;

	if (mLightType == LightType)
		return false;

	mLightType = LightType;

	return true;
}

bool lcLight::SetColor(const lcVector3& Color, lcStep Step, bool AddKey)
{
	return mColor.ChangeKey(Color, Step, AddKey);
}

void lcLight::SetPOVRayFadeDistance(float Distance, lcStep Step, bool AddKey)
{
	mPOVRayFadeDistance.ChangeKey(Distance, Step, AddKey);
}

void lcLight::SetPOVRayFadePower(float Power, lcStep Step, bool AddKey)
{
	mPOVRayFadePower.ChangeKey(Power, Step, AddKey);
}

void lcLight::SetSpotConeAngle(float Angle, lcStep Step, bool AddKey)
{
	mSpotConeAngle.ChangeKey(Angle, Step, AddKey);
}

void lcLight::SetSpotPenumbraAngle(float Angle, lcStep Step, bool AddKey)
{
	mSpotPenumbraAngle.ChangeKey(Angle, Step, AddKey);
}

void lcLight::SetSpotPOVRayTightness(float Tightness, lcStep Step, bool AddKey)
{
	mSpotPOVRayTightness.ChangeKey(Tightness, Step, AddKey);
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

bool lcLight::SetAreaPOVRayGrid(lcVector2i AreaGrid, lcStep Step, bool AddKey)
{
	mAreaPOVRayGrid.ChangeKey(AreaGrid, Step, AddKey);

	return true;
}

void lcLight::SetPointBlenderRadius(float Radius, lcStep Step, bool AddKey)
{
	mPointBlenderRadius.ChangeKey(Radius, Step, AddKey);
}

void lcLight::SetSpotBlenderRadius(float Radius, lcStep Step, bool AddKey)
{
	mSpotBlenderRadius.ChangeKey(Radius, Step, AddKey);
}

void lcLight::SetDirectionalBlenderAngle(float Angle, lcStep Step, bool AddKey)
{
	mDirectionalBlenderAngle.ChangeKey(Angle, Step, AddKey);
}

void lcLight::SetAreaSize(lcVector2 Size, lcStep Step, bool AddKey)
{
	if (mAreaShape == lcLightAreaShape::Square || mAreaShape == lcLightAreaShape::Disk)
		Size[1] = Size[0];

	mAreaSize.ChangeKey(Size, Step, AddKey);
}

void lcLight::SetBlenderPower(float Power, lcStep Step, bool AddKey)
{
	mBlenderPower.ChangeKey(Power, Step, AddKey);
}

void lcLight::SetPOVRayPower(float Power, lcStep Step, bool AddKey)
{
	mPOVRayPower.ChangeKey(Power, Step, AddKey);
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
	mPosition.InsertTime(Start, Time);
	mRotation.InsertTime(Start, Time);
	mColor.InsertTime(Start, Time);
	mSpotConeAngle.InsertTime(Start, Time);
	mSpotPenumbraAngle.InsertTime(Start, Time);
	mSpotPOVRayTightness.InsertTime(Start, Time);
	mAreaPOVRayGrid.InsertTime(Start, Time);
	mPointBlenderRadius.InsertTime(Start, Time);
	mSpotBlenderRadius.InsertTime(Start, Time);
	mDirectionalBlenderAngle.InsertTime(Start, Time);
	mAreaSize.InsertTime(Start, Time);
	mBlenderPower.InsertTime(Start, Time);
	mPOVRayPower.InsertTime(Start, Time);
	mPOVRayFadeDistance.InsertTime(Start, Time);
	mPOVRayFadePower.InsertTime(Start, Time);
}

void lcLight::RemoveTime(lcStep Start, lcStep Time)
{
	mPosition.RemoveTime(Start, Time);
	mRotation.RemoveTime(Start, Time);
	mColor.RemoveTime(Start, Time);
	mSpotConeAngle.RemoveTime(Start, Time);
	mSpotPenumbraAngle.RemoveTime(Start, Time);
	mSpotPOVRayTightness.RemoveTime(Start, Time);
	mAreaPOVRayGrid.RemoveTime(Start, Time);
	mPointBlenderRadius.RemoveTime(Start, Time);
	mSpotBlenderRadius.RemoveTime(Start, Time);
	mDirectionalBlenderAngle.RemoveTime(Start, Time);
	mAreaSize.RemoveTime(Start, Time);
	mBlenderPower.RemoveTime(Start, Time);
	mPOVRayPower.RemoveTime(Start, Time);
	mPOVRayFadeDistance.RemoveTime(Start, Time);
	mPOVRayFadePower.RemoveTime(Start, Time);
}

void lcLight::UpdatePosition(lcStep Step)
{
	mPosition.Update(Step);
	mRotation.Update(Step);
	mColor.Update(Step);
	mSpotConeAngle.Update(Step);
	mSpotPenumbraAngle.Update(Step);
	mSpotPOVRayTightness.Update(Step);
	mAreaPOVRayGrid.Update(Step);
	mPointBlenderRadius.Update(Step);
	mSpotBlenderRadius.Update(Step);
	mDirectionalBlenderAngle.Update(Step);
	mAreaSize.Update(Step);
	mBlenderPower.Update(Step);
	mPOVRayPower.Update(Step);
	mPOVRayFadeDistance.Update(Step);
	mPOVRayFadePower.Update(Step);

	if (IsPointLight())
	{
		mWorldMatrix = lcMatrix44Translation(mPosition);
	}
	else
	{
		mWorldMatrix = lcMatrix44(mRotation, mPosition);
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
		const lcVector2& Size = mAreaSize;

		*CurVert++ = -Size.x / 2.0f;
		*CurVert++ = -Size.y / 2.0f;
		*CurVert++ = 0.0f;

		*CurVert++ =  Size.x / 2.0f;
		*CurVert++ = -Size.y / 2.0f;
		*CurVert++ = 0.0f;

		*CurVert++ =  Size.x / 2.0f;
		*CurVert++ =  Size.y / 2.0f;
		*CurVert++ = 0.0f;

		*CurVert++ = -Size.x / 2.0f;
		*CurVert++ =  Size.y / 2.0f;
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
			const lcVector2& Size = mAreaSize;
			float c = cosf((float)EdgeIndex / CircleEdges * LC_2PI) * Size.x / 2.0f;
			float s = sinf((float)EdgeIndex / CircleEdges * LC_2PI) * Size.y / 2.0f;

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

QVariant lcLight::GetPropertyValue(lcObjectPropertyId PropertyId) const
{
	switch (PropertyId)
	{
	case lcObjectPropertyId::PieceId:
	case lcObjectPropertyId::PieceColor:
	case lcObjectPropertyId::PieceStepShow:
	case lcObjectPropertyId::PieceStepHide:
	case lcObjectPropertyId::CameraName:
	case lcObjectPropertyId::CameraType:
	case lcObjectPropertyId::CameraFOV:
	case lcObjectPropertyId::CameraNear:
	case lcObjectPropertyId::CameraFar:
	case lcObjectPropertyId::CameraPositionX:
	case lcObjectPropertyId::CameraPositionY:
	case lcObjectPropertyId::CameraPositionZ:
	case lcObjectPropertyId::CameraTargetX:
	case lcObjectPropertyId::CameraTargetY:
	case lcObjectPropertyId::CameraTargetZ:
	case lcObjectPropertyId::CameraUpX:
	case lcObjectPropertyId::CameraUpY:
	case lcObjectPropertyId::CameraUpZ:
		break;

	case lcObjectPropertyId::LightName:
		return GetName();

	case lcObjectPropertyId::LightType:
		return static_cast<int>(GetLightType());

	case lcObjectPropertyId::LightColor:
		return QVariant::fromValue<lcVector3>(GetColor());

	case lcObjectPropertyId::LightBlenderPower:
	case lcObjectPropertyId::LightPOVRayPower:
		break;

	case lcObjectPropertyId::LightCastShadow:
		return GetCastShadow();

	case lcObjectPropertyId::LightPOVRayFadeDistance:
	case lcObjectPropertyId::LightPOVRayFadePower:
	case lcObjectPropertyId::LightPointBlenderRadius:
	case lcObjectPropertyId::LightSpotBlenderRadius:
	case lcObjectPropertyId::LightDirectionalBlenderAngle:
	case lcObjectPropertyId::LightAreaSize:
	case lcObjectPropertyId::LightAreaSizeX:
	case lcObjectPropertyId::LightAreaSizeY:
	case lcObjectPropertyId::LightSpotConeAngle:
	case lcObjectPropertyId::LightSpotPenumbraAngle:
	case lcObjectPropertyId::LightSpotPOVRayTightness:
		break;

	case lcObjectPropertyId::LightAreaShape:
		return static_cast<int>(GetAreaShape());

	case lcObjectPropertyId::LightAreaPOVRayGridX:
		return GetAreaPOVRayGrid().x;

	case lcObjectPropertyId::LightAreaPOVRayGridY:
		return GetAreaPOVRayGrid().y;

	case lcObjectPropertyId::ObjectPositionX:
	case lcObjectPropertyId::ObjectPositionY:
	case lcObjectPropertyId::ObjectPositionZ:
	case lcObjectPropertyId::ObjectRotationX:
	case lcObjectPropertyId::ObjectRotationY:
	case lcObjectPropertyId::ObjectRotationZ:
	case lcObjectPropertyId::Count:
		break;
	}

	return QVariant();
}

bool lcLight::SetPropertyValue(lcObjectPropertyId PropertyId, lcStep Step, bool AddKey, QVariant Value)
{
	switch (PropertyId)
	{
	case lcObjectPropertyId::PieceId:
	case lcObjectPropertyId::PieceColor:
	case lcObjectPropertyId::PieceStepShow:
	case lcObjectPropertyId::PieceStepHide:
	case lcObjectPropertyId::CameraName:
	case lcObjectPropertyId::CameraType:
	case lcObjectPropertyId::CameraFOV:
	case lcObjectPropertyId::CameraNear:
	case lcObjectPropertyId::CameraFar:
	case lcObjectPropertyId::CameraPositionX:
	case lcObjectPropertyId::CameraPositionY:
	case lcObjectPropertyId::CameraPositionZ:
	case lcObjectPropertyId::CameraTargetX:
	case lcObjectPropertyId::CameraTargetY:
	case lcObjectPropertyId::CameraTargetZ:
	case lcObjectPropertyId::CameraUpX:
	case lcObjectPropertyId::CameraUpY:
	case lcObjectPropertyId::CameraUpZ:
		break;

	case lcObjectPropertyId::LightName:
		return SetName(Value.toString());

	case lcObjectPropertyId::LightType:
		return SetLightType(static_cast<lcLightType>(Value.toInt()));

	case lcObjectPropertyId::LightColor:
		return SetColor(Value.value<lcVector3>(), Step, AddKey);
		
	case lcObjectPropertyId::LightBlenderPower:
	case lcObjectPropertyId::LightPOVRayPower:
		break;

	case lcObjectPropertyId::LightCastShadow:
		return SetCastShadow(Value.toBool());

	case lcObjectPropertyId::LightPOVRayFadeDistance:
	case lcObjectPropertyId::LightPOVRayFadePower:
	case lcObjectPropertyId::LightPointBlenderRadius:
	case lcObjectPropertyId::LightSpotBlenderRadius:
	case lcObjectPropertyId::LightDirectionalBlenderAngle:
	case lcObjectPropertyId::LightAreaSize:
	case lcObjectPropertyId::LightAreaSizeX:
	case lcObjectPropertyId::LightAreaSizeY:
	case lcObjectPropertyId::LightSpotConeAngle:
	case lcObjectPropertyId::LightSpotPenumbraAngle:
	case lcObjectPropertyId::LightSpotPOVRayTightness:
		break;

	case lcObjectPropertyId::LightAreaShape:
		return SetAreaShape(static_cast<lcLightAreaShape>(Value.toInt()));

	case lcObjectPropertyId::LightAreaPOVRayGridX:
		return SetAreaPOVRayGrid(lcVector2i(Value.toInt(), GetAreaPOVRayGrid().y), Step, AddKey);

	case lcObjectPropertyId::LightAreaPOVRayGridY:
		return SetAreaPOVRayGrid(lcVector2i(GetAreaPOVRayGrid().x, Value.toInt()), Step, AddKey);

	case lcObjectPropertyId::ObjectPositionX:
	case lcObjectPropertyId::ObjectPositionY:
	case lcObjectPropertyId::ObjectPositionZ:
	case lcObjectPropertyId::ObjectRotationX:
	case lcObjectPropertyId::ObjectRotationY:
	case lcObjectPropertyId::ObjectRotationZ:
	case lcObjectPropertyId::Count:
		break;
	}

	return false;
}

bool lcLight::HasKeyFrame(lcObjectPropertyId PropertyId, lcStep Time) const
{
	switch (PropertyId)
	{
	case lcObjectPropertyId::PieceId:
	case lcObjectPropertyId::PieceColor:
	case lcObjectPropertyId::PieceStepShow:
	case lcObjectPropertyId::PieceStepHide:
	case lcObjectPropertyId::CameraName:
	case lcObjectPropertyId::CameraType:
	case lcObjectPropertyId::CameraFOV:
	case lcObjectPropertyId::CameraNear:
	case lcObjectPropertyId::CameraFar:
	case lcObjectPropertyId::CameraPositionX:
	case lcObjectPropertyId::CameraPositionY:
	case lcObjectPropertyId::CameraPositionZ:
	case lcObjectPropertyId::CameraTargetX:
	case lcObjectPropertyId::CameraTargetY:
	case lcObjectPropertyId::CameraTargetZ:
	case lcObjectPropertyId::CameraUpX:
	case lcObjectPropertyId::CameraUpY:
	case lcObjectPropertyId::CameraUpZ:
	case lcObjectPropertyId::LightName:
	case lcObjectPropertyId::LightType:
		return false;

	case lcObjectPropertyId::LightColor:
		return mColor.HasKeyFrame(Time);

	case lcObjectPropertyId::LightBlenderPower:
		return mBlenderPower.HasKeyFrame(Time);

	case lcObjectPropertyId::LightPOVRayPower:
		return mPOVRayPower.HasKeyFrame(Time);

	case lcObjectPropertyId::LightCastShadow:
		return false;

	case lcObjectPropertyId::LightPOVRayFadeDistance:
		return mPOVRayFadeDistance.HasKeyFrame(Time);

	case lcObjectPropertyId::LightPOVRayFadePower:
		return mPOVRayFadePower.HasKeyFrame(Time);

	case lcObjectPropertyId::LightPointBlenderRadius:
		return mPointBlenderRadius.HasKeyFrame(Time);

	case lcObjectPropertyId::LightSpotBlenderRadius:
		return mSpotBlenderRadius.HasKeyFrame(Time);

	case lcObjectPropertyId::LightDirectionalBlenderAngle:
		return mDirectionalBlenderAngle.HasKeyFrame(Time);

	case lcObjectPropertyId::LightAreaSize:
	case lcObjectPropertyId::LightAreaSizeX:
	case lcObjectPropertyId::LightAreaSizeY:
		return mAreaSize.HasKeyFrame(Time);

	case lcObjectPropertyId::LightSpotConeAngle:
		return mSpotConeAngle.HasKeyFrame(Time);

	case lcObjectPropertyId::LightSpotPenumbraAngle:
		return mSpotPenumbraAngle.HasKeyFrame(Time);

	case lcObjectPropertyId::LightSpotPOVRayTightness:
		return mSpotPOVRayTightness.HasKeyFrame(Time);

	case lcObjectPropertyId::LightAreaShape:
		return false;

	case lcObjectPropertyId::LightAreaPOVRayGridX:
	case lcObjectPropertyId::LightAreaPOVRayGridY:
		return mAreaPOVRayGrid.HasKeyFrame(Time);

	case lcObjectPropertyId::ObjectPositionX:
	case lcObjectPropertyId::ObjectPositionY:
	case lcObjectPropertyId::ObjectPositionZ:
		return mPosition.HasKeyFrame(Time);

	case lcObjectPropertyId::ObjectRotationX:
	case lcObjectPropertyId::ObjectRotationY:
	case lcObjectPropertyId::ObjectRotationZ:
		return mRotation.HasKeyFrame(Time);

	case lcObjectPropertyId::Count:
		return false;
	}

	return false;
}

bool lcLight::SetKeyFrame(lcObjectPropertyId PropertyId, lcStep Time, bool KeyFrame)
{
	switch (PropertyId)
	{
	case lcObjectPropertyId::PieceId:
	case lcObjectPropertyId::PieceColor:
	case lcObjectPropertyId::PieceStepShow:
	case lcObjectPropertyId::PieceStepHide:
	case lcObjectPropertyId::CameraName:
	case lcObjectPropertyId::CameraType:
	case lcObjectPropertyId::CameraFOV:
	case lcObjectPropertyId::CameraNear:
	case lcObjectPropertyId::CameraFar:
	case lcObjectPropertyId::CameraPositionX:
	case lcObjectPropertyId::CameraPositionY:
	case lcObjectPropertyId::CameraPositionZ:
	case lcObjectPropertyId::CameraTargetX:
	case lcObjectPropertyId::CameraTargetY:
	case lcObjectPropertyId::CameraTargetZ:
	case lcObjectPropertyId::CameraUpX:
	case lcObjectPropertyId::CameraUpY:
	case lcObjectPropertyId::CameraUpZ:
	case lcObjectPropertyId::LightName:
	case lcObjectPropertyId::LightType:
		return false;

	case lcObjectPropertyId::LightColor:
		return mColor.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::LightBlenderPower:
		return mBlenderPower.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::LightPOVRayPower:
		return mPOVRayPower.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::LightCastShadow:
		return false;

	case lcObjectPropertyId::LightPOVRayFadeDistance:
		return mPOVRayFadeDistance.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::LightPOVRayFadePower:
		return mPOVRayFadePower.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::LightPointBlenderRadius:
		return mPointBlenderRadius.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::LightSpotBlenderRadius:
		return mSpotBlenderRadius.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::LightDirectionalBlenderAngle:
		return mDirectionalBlenderAngle.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::LightAreaSize:
	case lcObjectPropertyId::LightAreaSizeX:
	case lcObjectPropertyId::LightAreaSizeY:
		return mAreaSize.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::LightSpotConeAngle:
		return mSpotConeAngle.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::LightSpotPenumbraAngle:
		return mSpotPenumbraAngle.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::LightSpotPOVRayTightness:
		return mSpotPOVRayTightness.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::LightAreaShape:
		return false;

	case lcObjectPropertyId::LightAreaPOVRayGridX:
	case lcObjectPropertyId::LightAreaPOVRayGridY:
		return mAreaPOVRayGrid.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::ObjectPositionX:
	case lcObjectPropertyId::ObjectPositionY:
	case lcObjectPropertyId::ObjectPositionZ:
		return mPosition.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::ObjectRotationX:
	case lcObjectPropertyId::ObjectRotationY:
	case lcObjectPropertyId::ObjectRotationZ:
		return mRotation.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::Count:
		return false;
	}

	return false;
}

void lcLight::RemoveKeyFrames()
{
	mPosition.RemoveAllKeys();
	mRotation.RemoveAllKeys();
	mColor.RemoveAllKeys();
	mSpotConeAngle.RemoveAllKeys();
	mSpotPenumbraAngle.RemoveAllKeys();
	mSpotPOVRayTightness.RemoveAllKeys();
	mAreaPOVRayGrid.RemoveAllKeys();
	mPointBlenderRadius.RemoveAllKeys();
	mSpotBlenderRadius.RemoveAllKeys();
	mDirectionalBlenderAngle.RemoveAllKeys();
	mAreaSize.RemoveAllKeys();
	mBlenderPower.RemoveAllKeys();
	mPOVRayPower.RemoveAllKeys();
	mPOVRayFadeDistance.RemoveAllKeys();
	mPOVRayFadePower.RemoveAllKeys();
}
