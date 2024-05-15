#include "lc_global.h"
#include "lc_math.h"
#include "lc_colors.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "lc_file.h"
#include "camera.h"
#include "lc_application.h"
#include "lc_context.h"

#define LC_CAMERA_POSITION_EDGE 7.5f
#define LC_CAMERA_TARGET_EDGE 7.5f

#define LC_CAMERA_SAVE_VERSION 7 // LeoCAD 0.80

lcCamera::lcCamera(bool Simple)
	: lcObject(lcObjectType::Camera)
{
	Initialize();

	if (Simple)
		mState |= LC_CAMERA_SIMPLE;
	else
	{
		mPosition.SetValue(lcVector3(-250.0f, -250.0f, 75.0f));
		mTargetPosition.SetValue(lcVector3(0.0f, 0.0f, 0.0f));
		mUpVector.SetValue(lcVector3(-0.2357f, -0.2357f, 0.94281f));

		UpdatePosition(1);
	}
}

lcCamera::lcCamera(float ex, float ey, float ez, float tx, float ty, float tz)
	: lcObject(lcObjectType::Camera)
{
	// Fix the up vector
	lcVector3 UpVector(0, 0, 1), FrontVector(ex - tx, ey - ty, ez - tz), SideVector;
	FrontVector.Normalize();
	if (FrontVector == UpVector)
		SideVector = lcVector3(1, 0, 0);
	else
		SideVector = lcCross(FrontVector, UpVector);
	UpVector = lcCross(SideVector, FrontVector);
	UpVector.Normalize();

	Initialize();

	mPosition.SetValue(lcVector3(ex, ey, ez));
	mTargetPosition.SetValue(lcVector3(tx, ty, tz));
	mUpVector.SetValue(UpVector);

	UpdatePosition(1);
}

lcCamera::~lcCamera()
{
}

QString lcCamera::GetCameraTypeString(lcCameraType CameraType)
{
	switch (CameraType)
	{
	case lcCameraType::Perspective:
		return QT_TRANSLATE_NOOP("Camera Type", "Perspective");

	case lcCameraType::Orthographic:
		return QT_TRANSLATE_NOOP("Camera Type", "Orthographic");

	case lcCameraType::Count:
		break;
	}

	return QString();
}

QStringList lcCamera::GetCameraTypeStrings()
{
	QStringList CameraType;

	for (int CameraTypeIndex = 0; CameraTypeIndex < static_cast<int>(lcCameraType::Count); CameraTypeIndex++)
		CameraType.push_back(GetCameraTypeString(static_cast<lcCameraType>(CameraTypeIndex)));

	return CameraType;
}

lcViewpoint lcCamera::GetViewpoint(const QString& ViewpointName)
{
	const QLatin1String ViewpointNames[] =
	{
		QLatin1String("front"),
		QLatin1String("back"),
		QLatin1String("top"),
		QLatin1String("bottom"),
		QLatin1String("left"),
		QLatin1String("right"),
		QLatin1String("home")
	};

	LC_ARRAY_SIZE_CHECK(ViewpointNames, lcViewpoint::Count);

	for (int ViewpointIndex = 0; ViewpointIndex < static_cast<int>(lcViewpoint::Count); ViewpointIndex++)
		if (ViewpointNames[ViewpointIndex] == ViewpointName)
			return static_cast<lcViewpoint>(ViewpointIndex);

	return lcViewpoint::Count;
}

void lcCamera::Initialize()
{
	m_fovy = 30.0f;
	m_zNear = 25.0f;
	m_zFar = 50000.0f;
	mState = 0;
}

bool lcCamera::SetName(const QString& Name)
{
	if (mName == Name)
		return false;

	mName = Name;

	return true;
}

void lcCamera::CreateName(const lcArray<lcCamera*>& Cameras)
{
	if (!mName.isEmpty())
	{
		bool Found = false;

		for (const lcCamera* Camera : Cameras)
		{
			if (Camera->GetName() == mName)
			{
				Found = true;
				break;
			}
		}

		if (!Found)
			return;
	}

	int MaxCameraNumber = 0;
	const QLatin1String Prefix("Camera ");

	for (const lcCamera* Camera : Cameras)
	{
		QString CameraName = Camera->GetName();

		if (CameraName.startsWith(Prefix))
		{
			bool Ok = false;
			int CameraNumber = CameraName.mid(Prefix.size()).toInt(&Ok);

			if (Ok && CameraNumber > MaxCameraNumber)
				MaxCameraNumber = CameraNumber;
		}
	}

	mName = Prefix + QString::number(MaxCameraNumber + 1);
}

bool lcCamera::SetCameraType(lcCameraType CameraType)
{
	if (static_cast<int>(CameraType) < 0 || CameraType >= lcCameraType::Count)
		return false;

	if (GetCameraType() == CameraType)
		return false;

	SetOrtho(CameraType == lcCameraType::Orthographic);

	return true;
}

void lcCamera::SaveLDraw(QTextStream& Stream) const
{
	const QLatin1String LineEnding("\r\n");

	Stream << QLatin1String("0 !LEOCAD CAMERA FOV ") << m_fovy << QLatin1String(" ZNEAR ") << m_zNear << QLatin1String(" ZFAR ") << m_zFar << LineEnding;

	mPosition.Save(Stream, "CAMERA", "POSITION", true);
	mTargetPosition.Save(Stream, "CAMERA", "TARGET_POSITION", true);
	mUpVector.Save(Stream, "CAMERA", "UP_VECTOR", true);

	Stream << QLatin1String("0 !LEOCAD CAMERA ");

	if (IsHidden())
		Stream << QLatin1String("HIDDEN");

	if (IsOrtho())
		Stream << QLatin1String("ORTHOGRAPHIC ");

	Stream << QLatin1String("NAME ") << mName << LineEnding;
}

bool lcCamera::ParseLDrawLine(QTextStream& Stream)
{
	while (!Stream.atEnd())
	{
		QString Token;
		Stream >> Token;

		if (Token == QLatin1String("HIDDEN"))
			SetHidden(true);
		else if (Token == QLatin1String("ORTHOGRAPHIC"))
			SetOrtho(true);
		else if (Token == QLatin1String("FOV"))
			Stream >> m_fovy;
		else if (Token == QLatin1String("ZNEAR"))
			Stream >> m_zNear;
		else if (Token == QLatin1String("ZFAR"))
			Stream >> m_zFar;
		else if (mPosition.Load(Stream, Token, "POSITION"))
			continue;
		else if (mTargetPosition.Load(Stream, Token, "TARGET_POSITION"))
			continue;
		else if (mUpVector.Load(Stream, Token, "UP_VECTOR"))
			continue;
		else if (Token == QLatin1String("NAME"))
		{
			mName = Stream.readAll().trimmed();
			return true;
		}
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////
// Camera save/load

bool lcCamera::FileLoad(lcFile& file)
{
	quint8 version, ch;

	version = file.ReadU8();

	if (version > LC_CAMERA_SAVE_VERSION)
		return false;

	if (version > 5)
	{
		if (file.ReadU8() != 1)
			return false;

		quint16 time;
		float param[4];
		quint8 type;
		quint32 n;

		file.ReadU32(&n, 1);
		while (n--)
		{
			file.ReadU16(&time, 1);
			file.ReadFloats(param, 4);
			file.ReadU8(&type, 1);
		}

		file.ReadU32(&n, 1);
		while (n--)
		{
			file.ReadU16(&time, 1);
			file.ReadFloats(param, 4);
			file.ReadU8(&type, 1);
		}
	}

	if (version == 4)
	{
		char Name[81];
		file.ReadBuffer(Name, 80);
	}
	else
	{
		ch = file.ReadU8();
		if (ch == 0xFF)
			return false;
		char Name[81];
		file.ReadBuffer(Name, ch);
	}

	if (version < 3)
	{
		double d[3];

		file.ReadDoubles(d, 3);
		file.ReadDoubles(d, 3);
		file.ReadDoubles(d, 3);
	}

	if (version == 3)
	{
		ch = file.ReadU8();

		while (ch--)
		{
			quint8 step;
			double eye[3], target[3], up[3];

			file.ReadDoubles(eye, 3);
			file.ReadDoubles(target, 3);
			file.ReadDoubles(up, 3);
			file.ReadU8(&step, 1);

			file.ReadS32(); // snapshot
			file.ReadS32(); // cam
		}
	}

	if (version < 4)
	{
		file.ReadDouble(); // m_fovy
		file.ReadDouble(); // m_zFar
		file.ReadDouble(); // m_zNear
	}
	else
	{
		qint32 n;

		if (version < 6)
		{
			quint16 time;
			float param[4];
			quint8 type;

			n = file.ReadS32();
			while (n--)
			{
				file.ReadU16(&time, 1);
				file.ReadFloats(param, 3);
				file.ReadU8(&type, 1);
			}

			n = file.ReadS32();
			while (n--)
			{
				file.ReadU16(&time, 1);
				file.ReadFloats(param, 3);
				file.ReadU8(&type, 1);
			}
		}

		float f;
		file.ReadFloats(&f, 1); // m_fovy
		file.ReadFloats(&f, 1); // m_zFar
		file.ReadFloats(&f, 1); // m_zNear

		if (version < 5)
		{
			n = file.ReadS32();
		}
		else
		{
			ch = file.ReadU8();
			file.ReadU8();
		}
	}

	if ((version > 1) && (version < 4))
	{
		quint32 show;
		qint32 user;

		file.ReadU32(&show, 1);
//		if (version > 2)
		file.ReadS32(&user, 1);
	}

	return true;
}

void lcCamera::CompareBoundingBox(lcVector3& Min, lcVector3& Max)
{
	const lcVector3 Points[2] =
	{
		mPosition, mTargetPosition
	};

	for (int i = 0; i < 2; i++)
	{
		const lcVector3& Point = Points[i];

		// TODO: this should check the entire mesh

		Min = lcMin(Point, Min);
		Max = lcMax(Point, Max);
	}
}

void lcCamera::MoveSelected(lcStep Step, bool AddKey, const lcVector3& Distance)
{
	if (IsSimple())
		AddKey = false;

	if (IsSelected(LC_CAMERA_SECTION_POSITION))
	{
		mPosition.ChangeKey(mPosition + Distance, Step, AddKey);
	}

	if (IsSelected(LC_CAMERA_SECTION_TARGET))
	{
		mTargetPosition.ChangeKey(mTargetPosition + Distance, Step, AddKey);
	}
	else if (IsSelected(LC_CAMERA_SECTION_UPVECTOR))
	{
		mUpVector.ChangeKey(lcNormalize(mUpVector + Distance), Step, AddKey);
	}
}

void lcCamera::MoveRelative(const lcVector3& Distance, lcStep Step, bool AddKey)
{
	if (IsSimple())
		AddKey = false;

	const lcVector3 Relative = lcMul30(Distance, lcMatrix44Transpose(mWorldView)) * 5.0f;

	mPosition.ChangeKey(mPosition + Relative, Step, AddKey);
	mTargetPosition.ChangeKey(mTargetPosition + Relative, Step, AddKey);

	UpdatePosition(Step);
}

void lcCamera::UpdatePosition(lcStep Step)
{
	mPosition.Update(Step);
	mTargetPosition.Update(Step);
	mUpVector.Update(Step);

	const lcVector3 FrontVector(mPosition - mTargetPosition);
	const lcVector3 SideVector = lcCross(FrontVector, mUpVector);
	const lcVector3 UpVector = lcNormalize(lcCross(SideVector, FrontVector));

	mWorldView = lcMatrix44LookAt(mPosition, mTargetPosition, UpVector);
}

void lcCamera::CopyPosition(const lcCamera* Camera)
{
	m_fovy = Camera->m_fovy;
	m_zNear = Camera->m_zNear;
	m_zFar = Camera->m_zFar;

	mWorldView = Camera->mWorldView;
	mPosition = Camera->mPosition;
	mTargetPosition = Camera->mTargetPosition;
	mUpVector = Camera->mUpVector;
	mState |= (Camera->mState & LC_CAMERA_ORTHO);
}

void lcCamera::CopySettings(const lcCamera* camera)
{
	m_fovy = camera->m_fovy;
	m_zNear = camera->m_zNear;
	m_zFar = camera->m_zFar;

	mState |= (camera->mState & LC_CAMERA_ORTHO);
}

void lcCamera::DrawInterface(lcContext* Context, const lcScene& Scene) const
{
	Q_UNUSED(Scene);
	Context->SetMaterial(lcMaterialType::UnlitColor);

	lcMatrix44 ViewWorldMatrix = lcMatrix44AffineInverse(mWorldView);
	ViewWorldMatrix.SetTranslation(lcVector3(0, 0, 0));

	const lcMatrix44 CameraViewMatrix = lcMul(ViewWorldMatrix, lcMatrix44Translation(mPosition));
	Context->SetWorldMatrix(CameraViewMatrix);

	float Verts[(12 + 8 + 8 + 3 + 4) * 3];
	float* CurVert = Verts;

	float Length = lcLength(mPosition - mTargetPosition);

	*CurVert++ =  LC_CAMERA_POSITION_EDGE; *CurVert++ =  LC_CAMERA_POSITION_EDGE; *CurVert++ =  LC_CAMERA_POSITION_EDGE;
	*CurVert++ = -LC_CAMERA_POSITION_EDGE; *CurVert++ =  LC_CAMERA_POSITION_EDGE; *CurVert++ =  LC_CAMERA_POSITION_EDGE;
	*CurVert++ = -LC_CAMERA_POSITION_EDGE; *CurVert++ = -LC_CAMERA_POSITION_EDGE; *CurVert++ =  LC_CAMERA_POSITION_EDGE;
	*CurVert++ =  LC_CAMERA_POSITION_EDGE; *CurVert++ = -LC_CAMERA_POSITION_EDGE; *CurVert++ =  LC_CAMERA_POSITION_EDGE;
	*CurVert++ =  LC_CAMERA_POSITION_EDGE; *CurVert++ =  LC_CAMERA_POSITION_EDGE; *CurVert++ = -LC_CAMERA_POSITION_EDGE;
	*CurVert++ = -LC_CAMERA_POSITION_EDGE; *CurVert++ =  LC_CAMERA_POSITION_EDGE; *CurVert++ = -LC_CAMERA_POSITION_EDGE;
	*CurVert++ = -LC_CAMERA_POSITION_EDGE; *CurVert++ = -LC_CAMERA_POSITION_EDGE; *CurVert++ = -LC_CAMERA_POSITION_EDGE;
	*CurVert++ =  LC_CAMERA_POSITION_EDGE; *CurVert++ = -LC_CAMERA_POSITION_EDGE; *CurVert++ = -LC_CAMERA_POSITION_EDGE;
	*CurVert++ =  LC_CAMERA_POSITION_EDGE; *CurVert++ =  LC_CAMERA_POSITION_EDGE; *CurVert++ = -LC_CAMERA_POSITION_EDGE * 2;
	*CurVert++ = -LC_CAMERA_POSITION_EDGE; *CurVert++ =  LC_CAMERA_POSITION_EDGE; *CurVert++ = -LC_CAMERA_POSITION_EDGE * 2;
	*CurVert++ = -LC_CAMERA_POSITION_EDGE; *CurVert++ = -LC_CAMERA_POSITION_EDGE; *CurVert++ = -LC_CAMERA_POSITION_EDGE * 2;
	*CurVert++ =  LC_CAMERA_POSITION_EDGE; *CurVert++ = -LC_CAMERA_POSITION_EDGE; *CurVert++ = -LC_CAMERA_POSITION_EDGE * 2;

	*CurVert++ =  LC_CAMERA_TARGET_EDGE; *CurVert++ =  LC_CAMERA_TARGET_EDGE; *CurVert++ =  LC_CAMERA_TARGET_EDGE - Length;
	*CurVert++ = -LC_CAMERA_TARGET_EDGE; *CurVert++ =  LC_CAMERA_TARGET_EDGE; *CurVert++ =  LC_CAMERA_TARGET_EDGE - Length;
	*CurVert++ = -LC_CAMERA_TARGET_EDGE; *CurVert++ = -LC_CAMERA_TARGET_EDGE; *CurVert++ =  LC_CAMERA_TARGET_EDGE - Length;
	*CurVert++ =  LC_CAMERA_TARGET_EDGE; *CurVert++ = -LC_CAMERA_TARGET_EDGE; *CurVert++ =  LC_CAMERA_TARGET_EDGE - Length;
	*CurVert++ =  LC_CAMERA_TARGET_EDGE; *CurVert++ =  LC_CAMERA_TARGET_EDGE; *CurVert++ = -LC_CAMERA_TARGET_EDGE - Length;
	*CurVert++ = -LC_CAMERA_TARGET_EDGE; *CurVert++ =  LC_CAMERA_TARGET_EDGE; *CurVert++ = -LC_CAMERA_TARGET_EDGE - Length;
	*CurVert++ = -LC_CAMERA_TARGET_EDGE; *CurVert++ = -LC_CAMERA_TARGET_EDGE; *CurVert++ = -LC_CAMERA_TARGET_EDGE - Length;
	*CurVert++ =  LC_CAMERA_TARGET_EDGE; *CurVert++ = -LC_CAMERA_TARGET_EDGE; *CurVert++ = -LC_CAMERA_TARGET_EDGE - Length;

	*CurVert++ =  LC_CAMERA_TARGET_EDGE; *CurVert++ =  LC_CAMERA_TARGET_EDGE + 25.0f; *CurVert++ =  LC_CAMERA_TARGET_EDGE;
	*CurVert++ = -LC_CAMERA_TARGET_EDGE; *CurVert++ =  LC_CAMERA_TARGET_EDGE + 25.0f; *CurVert++ =  LC_CAMERA_TARGET_EDGE;
	*CurVert++ = -LC_CAMERA_TARGET_EDGE; *CurVert++ = -LC_CAMERA_TARGET_EDGE + 25.0f; *CurVert++ =  LC_CAMERA_TARGET_EDGE;
	*CurVert++ =  LC_CAMERA_TARGET_EDGE; *CurVert++ = -LC_CAMERA_TARGET_EDGE + 25.0f; *CurVert++ =  LC_CAMERA_TARGET_EDGE;
	*CurVert++ =  LC_CAMERA_TARGET_EDGE; *CurVert++ =  LC_CAMERA_TARGET_EDGE + 25.0f; *CurVert++ = -LC_CAMERA_TARGET_EDGE;
	*CurVert++ = -LC_CAMERA_TARGET_EDGE; *CurVert++ =  LC_CAMERA_TARGET_EDGE + 25.0f; *CurVert++ = -LC_CAMERA_TARGET_EDGE;
	*CurVert++ = -LC_CAMERA_TARGET_EDGE; *CurVert++ = -LC_CAMERA_TARGET_EDGE + 25.0f; *CurVert++ = -LC_CAMERA_TARGET_EDGE;
	*CurVert++ =  LC_CAMERA_TARGET_EDGE; *CurVert++ = -LC_CAMERA_TARGET_EDGE + 25.0f; *CurVert++ = -LC_CAMERA_TARGET_EDGE;

	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = 0.0f;
	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = -Length;
	*CurVert++ = 0.0f; *CurVert++ = 25.0f; *CurVert++ = 0.0f;

	const GLushort Indices[40 + 24 + 24 + 4 + 16] = 
	{
		0, 1, 1, 2, 2, 3, 3, 0,
		4, 5, 5, 6, 6, 7, 7, 4,
		0, 4, 1, 5, 2, 6, 3, 7,
		8, 9, 9, 10, 10, 11, 11, 8,
		8, 28, 9, 28, 10, 28, 11, 28,
		12, 13, 13, 14, 14, 15, 15, 12,
		16, 17, 17, 18, 18, 19, 19, 16,
		12, 16, 13, 17, 14, 18, 15, 19,
		20, 21, 21, 22, 22, 23, 23, 20,
		24, 25, 25, 26, 26, 27, 27, 24,
		20, 24, 21, 25, 22, 26, 23, 27,
		28, 29, 28, 30,
		31, 32, 32, 33, 33, 34, 34, 31,
		28, 31, 28, 32, 28, 33, 28, 34
	};

	Context->SetVertexBufferPointer(Verts);
	Context->SetVertexFormatPosition(3);
	Context->SetIndexBufferPointer(Indices);

	const lcPreferences& Preferences = lcGetPreferences();
	const float LineWidth = Preferences.mLineWidth;
	const lcVector4 SelectedColor = lcVector4FromColor(Preferences.mObjectSelectedColor);
	const lcVector4 FocusedColor = lcVector4FromColor(Preferences.mObjectFocusedColor);
	const lcVector4 CameraColor = lcVector4FromColor(Preferences.mCameraColor);

	if (!IsSelected())
	{
		Context->SetLineWidth(LineWidth);
		Context->SetColor(CameraColor);

		Context->DrawIndexedPrimitives(GL_LINES, 40 + 24 + 24 + 4, GL_UNSIGNED_SHORT, 0);
	}
	else
	{
		if (IsSelected(LC_CAMERA_SECTION_POSITION))
		{
			Context->SetLineWidth(2.0f * LineWidth);
			if (IsFocused(LC_CAMERA_SECTION_POSITION))
				Context->SetColor(FocusedColor);
			else
				Context->SetColor(SelectedColor);
		}
		else
		{
			Context->SetLineWidth(LineWidth);
			Context->SetColor(CameraColor);
		}

		Context->DrawIndexedPrimitives(GL_LINES, 40, GL_UNSIGNED_SHORT, 0);

		if (IsSelected(LC_CAMERA_SECTION_TARGET))
		{
			Context->SetLineWidth(2.0f * LineWidth);
			if (IsFocused(LC_CAMERA_SECTION_TARGET))
				Context->SetColor(FocusedColor);
			else
				Context->SetColor(SelectedColor);
		}
		else
		{
			Context->SetLineWidth(LineWidth);
			Context->SetColor(CameraColor);
		}

		Context->DrawIndexedPrimitives(GL_LINES, 24, GL_UNSIGNED_SHORT, 40 * 2);

		if (IsSelected(LC_CAMERA_SECTION_UPVECTOR))
		{
			Context->SetLineWidth(2.0f * LineWidth);
			if (IsFocused(LC_CAMERA_SECTION_UPVECTOR))
				Context->SetColor(FocusedColor);
			else
				Context->SetColor(SelectedColor);
		}
		else
		{
			Context->SetLineWidth(LineWidth);
			Context->SetColor(CameraColor);
		}

		Context->DrawIndexedPrimitives(GL_LINES, 24, GL_UNSIGNED_SHORT, (40 + 24) * 2);

		Context->SetColor(CameraColor);
		Context->SetLineWidth(LineWidth);

		float SizeY = tanf(LC_DTOR * m_fovy / 2) * Length;
		float SizeX = SizeY * 1.333f;

		*CurVert++ =  SizeX; *CurVert++ =  SizeY; *CurVert++ = -Length;
		*CurVert++ = -SizeX; *CurVert++ =  SizeY; *CurVert++ = -Length;
		*CurVert++ = -SizeX; *CurVert++ = -SizeY; *CurVert++ = -Length;
		*CurVert++ =  SizeX; *CurVert++ = -SizeY; *CurVert++ = -Length;

		Context->DrawIndexedPrimitives(GL_LINES, 4 + 16, GL_UNSIGNED_SHORT, (40 + 24 + 24) * 2);
	}
}

QVariant lcCamera::GetPropertyValue(lcObjectPropertyId PropertyId) const
{
	switch (PropertyId)
	{
	case lcObjectPropertyId::PieceId:
	case lcObjectPropertyId::PieceColor:
	case lcObjectPropertyId::PieceStepShow:
	case lcObjectPropertyId::PieceStepHide:
		break;

	case lcObjectPropertyId::CameraName:
		return GetName();

	case lcObjectPropertyId::CameraType:
		return static_cast<int>(GetCameraType());

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
	case lcObjectPropertyId::LightColor:
	case lcObjectPropertyId::LightBlenderPower:
	case lcObjectPropertyId::LightPOVRayPower:
	case lcObjectPropertyId::LightCastShadow:
	case lcObjectPropertyId::LightPOVRayFadeDistance:
	case lcObjectPropertyId::LightPOVRayFadePower:
	case lcObjectPropertyId::LightPointBlenderRadius:
	case lcObjectPropertyId::LightSpotBlenderRadius:
	case lcObjectPropertyId::LightDirectionalBlenderAngle:
	case lcObjectPropertyId::LightAreaSizeX:
	case lcObjectPropertyId::LightAreaSizeY:
	case lcObjectPropertyId::LightSpotConeAngle:
	case lcObjectPropertyId::LightSpotPenumbraAngle:
	case lcObjectPropertyId::LightSpotPOVRayTightness:
	case lcObjectPropertyId::LightAreaShape:
	case lcObjectPropertyId::LightAreaPOVRayGridX:
	case lcObjectPropertyId::LightAreaPOVRayGridY:
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

bool lcCamera::SetPropertyValue(lcObjectPropertyId PropertyId, lcStep Step, bool AddKey, QVariant Value)
{
	Q_UNUSED(Step);
	Q_UNUSED(AddKey);

	switch (PropertyId)
	{
	case lcObjectPropertyId::PieceId:
	case lcObjectPropertyId::PieceColor:
	case lcObjectPropertyId::PieceStepShow:
	case lcObjectPropertyId::PieceStepHide:
		break;

	case lcObjectPropertyId::CameraName:
		return SetName(Value.toString());

	case lcObjectPropertyId::CameraType:
		return SetCameraType(static_cast<lcCameraType>(Value.toInt()));

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
	case lcObjectPropertyId::LightColor:
	case lcObjectPropertyId::LightBlenderPower:
	case lcObjectPropertyId::LightPOVRayPower:
	case lcObjectPropertyId::LightCastShadow:
	case lcObjectPropertyId::LightPOVRayFadeDistance:
	case lcObjectPropertyId::LightPOVRayFadePower:
	case lcObjectPropertyId::LightPointBlenderRadius:
	case lcObjectPropertyId::LightSpotBlenderRadius:
	case lcObjectPropertyId::LightDirectionalBlenderAngle:
	case lcObjectPropertyId::LightAreaSizeX:
	case lcObjectPropertyId::LightAreaSizeY:
	case lcObjectPropertyId::LightSpotConeAngle:
	case lcObjectPropertyId::LightSpotPenumbraAngle:
	case lcObjectPropertyId::LightSpotPOVRayTightness:
	case lcObjectPropertyId::LightAreaShape:
	case lcObjectPropertyId::LightAreaPOVRayGridX:
	case lcObjectPropertyId::LightAreaPOVRayGridY:
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

bool lcCamera::HasKeyFrame(lcObjectPropertyId PropertyId, lcStep Time) const
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
		return false;

	case lcObjectPropertyId::CameraPositionX:
	case lcObjectPropertyId::CameraPositionY:
	case lcObjectPropertyId::CameraPositionZ:
		return mPosition.HasKeyFrame(Time);

	case lcObjectPropertyId::CameraTargetX:
	case lcObjectPropertyId::CameraTargetY:
	case lcObjectPropertyId::CameraTargetZ:
		return mTargetPosition.HasKeyFrame(Time);

	case lcObjectPropertyId::CameraUpX:
	case lcObjectPropertyId::CameraUpY:
	case lcObjectPropertyId::CameraUpZ:
		return mUpVector.HasKeyFrame(Time);

	case lcObjectPropertyId::LightName:
	case lcObjectPropertyId::LightType:
	case lcObjectPropertyId::LightColor:
	case lcObjectPropertyId::LightBlenderPower:
	case lcObjectPropertyId::LightPOVRayPower:
	case lcObjectPropertyId::LightCastShadow:
	case lcObjectPropertyId::LightPOVRayFadeDistance:
	case lcObjectPropertyId::LightPOVRayFadePower:
	case lcObjectPropertyId::LightPointBlenderRadius:
	case lcObjectPropertyId::LightSpotBlenderRadius:
	case lcObjectPropertyId::LightDirectionalBlenderAngle:
	case lcObjectPropertyId::LightAreaSizeX:
	case lcObjectPropertyId::LightAreaSizeY:
	case lcObjectPropertyId::LightSpotConeAngle:
	case lcObjectPropertyId::LightSpotPenumbraAngle:
	case lcObjectPropertyId::LightSpotPOVRayTightness:
	case lcObjectPropertyId::LightAreaShape:
	case lcObjectPropertyId::LightAreaPOVRayGridX:
	case lcObjectPropertyId::LightAreaPOVRayGridY:
	case lcObjectPropertyId::ObjectPositionX:
	case lcObjectPropertyId::ObjectPositionY:
	case lcObjectPropertyId::ObjectPositionZ:
	case lcObjectPropertyId::ObjectRotationX:
	case lcObjectPropertyId::ObjectRotationY:
	case lcObjectPropertyId::ObjectRotationZ:
	case lcObjectPropertyId::Count:
		return false;
	}

	return false;
}

bool lcCamera::SetKeyFrame(lcObjectPropertyId PropertyId, lcStep Time, bool KeyFrame)
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
		return false;

	case lcObjectPropertyId::CameraPositionX:
	case lcObjectPropertyId::CameraPositionY:
	case lcObjectPropertyId::CameraPositionZ:
		return mPosition.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::CameraTargetX:
	case lcObjectPropertyId::CameraTargetY:
	case lcObjectPropertyId::CameraTargetZ:
		return mTargetPosition.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::CameraUpX:
	case lcObjectPropertyId::CameraUpY:
	case lcObjectPropertyId::CameraUpZ:
		return mUpVector.SetKeyFrame(Time, KeyFrame);

	case lcObjectPropertyId::LightName:
	case lcObjectPropertyId::LightType:
	case lcObjectPropertyId::LightColor:
	case lcObjectPropertyId::LightBlenderPower:
	case lcObjectPropertyId::LightPOVRayPower:
	case lcObjectPropertyId::LightCastShadow:
	case lcObjectPropertyId::LightPOVRayFadeDistance:
	case lcObjectPropertyId::LightPOVRayFadePower:
	case lcObjectPropertyId::LightPointBlenderRadius:
	case lcObjectPropertyId::LightSpotBlenderRadius:
	case lcObjectPropertyId::LightDirectionalBlenderAngle:
	case lcObjectPropertyId::LightAreaSizeX:
	case lcObjectPropertyId::LightAreaSizeY:
	case lcObjectPropertyId::LightSpotConeAngle:
	case lcObjectPropertyId::LightSpotPenumbraAngle:
	case lcObjectPropertyId::LightSpotPOVRayTightness:
	case lcObjectPropertyId::LightAreaShape:
	case lcObjectPropertyId::LightAreaPOVRayGridX:
	case lcObjectPropertyId::LightAreaPOVRayGridY:
	case lcObjectPropertyId::ObjectPositionX:
	case lcObjectPropertyId::ObjectPositionY:
	case lcObjectPropertyId::ObjectPositionZ:
	case lcObjectPropertyId::ObjectRotationX:
	case lcObjectPropertyId::ObjectRotationY:
	case lcObjectPropertyId::ObjectRotationZ:
	case lcObjectPropertyId::Count:
		return false;
	}

	return false;
}

void lcCamera::RemoveKeyFrames()
{
	mPosition.RemoveAllKeys();
	mTargetPosition.RemoveAllKeys();
	mUpVector.RemoveAllKeys();
}

void lcCamera::RayTest(lcObjectRayTest& ObjectRayTest) const
{
	lcVector3 Min = lcVector3(-LC_CAMERA_POSITION_EDGE, -LC_CAMERA_POSITION_EDGE, -LC_CAMERA_POSITION_EDGE);
	lcVector3 Max = lcVector3(LC_CAMERA_POSITION_EDGE, LC_CAMERA_POSITION_EDGE, LC_CAMERA_POSITION_EDGE);

	lcVector3 Start = lcMul31(ObjectRayTest.Start, mWorldView);
	lcVector3 End = lcMul31(ObjectRayTest.End, mWorldView);

	float Distance;
	lcVector3 Plane;

	if (lcBoundingBoxRayIntersectDistance(Min, Max, Start, End, &Distance, nullptr, &Plane) && (Distance < ObjectRayTest.Distance))
	{
		ObjectRayTest.ObjectSection.Object = const_cast<lcCamera*>(this);
		ObjectRayTest.ObjectSection.Section = LC_CAMERA_SECTION_POSITION;
		ObjectRayTest.Distance = Distance;
		ObjectRayTest.PieceInfoRayTest.Plane = Plane;
	}

	Min = lcVector3(-LC_CAMERA_TARGET_EDGE, -LC_CAMERA_TARGET_EDGE, -LC_CAMERA_TARGET_EDGE);
	Max = lcVector3(LC_CAMERA_TARGET_EDGE, LC_CAMERA_TARGET_EDGE, LC_CAMERA_TARGET_EDGE);

	lcMatrix44 WorldView = mWorldView;
	WorldView.SetTranslation(lcMul30(-mTargetPosition, WorldView));

	Start = lcMul31(ObjectRayTest.Start, WorldView);
	End = lcMul31(ObjectRayTest.End, WorldView);

	if (lcBoundingBoxRayIntersectDistance(Min, Max, Start, End, &Distance, nullptr, &Plane) && (Distance < ObjectRayTest.Distance))
	{
		ObjectRayTest.ObjectSection.Object = const_cast<lcCamera*>(this);
		ObjectRayTest.ObjectSection.Section = LC_CAMERA_SECTION_TARGET;
		ObjectRayTest.Distance = Distance;
		ObjectRayTest.PieceInfoRayTest.Plane = Plane;
	}

	const lcMatrix44 ViewWorld = lcMatrix44AffineInverse(mWorldView);
	const lcVector3 UpVectorPosition = lcMul31(lcVector3(0, 25, 0), ViewWorld);

	WorldView = mWorldView;
	WorldView.SetTranslation(lcMul30(-UpVectorPosition, WorldView));

	Start = lcMul31(ObjectRayTest.Start, WorldView);
	End = lcMul31(ObjectRayTest.End, WorldView);

	if (lcBoundingBoxRayIntersectDistance(Min, Max, Start, End, &Distance, nullptr, &Plane) && (Distance < ObjectRayTest.Distance))
	{
		ObjectRayTest.ObjectSection.Object = const_cast<lcCamera*>(this);
		ObjectRayTest.ObjectSection.Section = LC_CAMERA_SECTION_UPVECTOR;
		ObjectRayTest.Distance = Distance;
		ObjectRayTest.PieceInfoRayTest.Plane = Plane;
	}
}

void lcCamera::BoxTest(lcObjectBoxTest& ObjectBoxTest) const
{
	lcVector3 Min(-LC_CAMERA_POSITION_EDGE, -LC_CAMERA_POSITION_EDGE, -LC_CAMERA_POSITION_EDGE);
	lcVector3 Max(LC_CAMERA_POSITION_EDGE, LC_CAMERA_POSITION_EDGE, LC_CAMERA_POSITION_EDGE);

	lcVector4 LocalPlanes[6];

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		const lcVector3 Normal = lcMul30(ObjectBoxTest.Planes[PlaneIdx], mWorldView);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, ObjectBoxTest.Planes[PlaneIdx][3] - lcDot3(mWorldView[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		ObjectBoxTest.Objects.Add(const_cast<lcCamera*>(this));
		return;
	}

	Min = lcVector3(-LC_CAMERA_TARGET_EDGE, -LC_CAMERA_TARGET_EDGE, -LC_CAMERA_TARGET_EDGE);
	Max = lcVector3(LC_CAMERA_TARGET_EDGE, LC_CAMERA_TARGET_EDGE, LC_CAMERA_TARGET_EDGE);

	lcMatrix44 WorldView = mWorldView;
	WorldView.SetTranslation(lcMul30(-mTargetPosition, WorldView));

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		const lcVector3 Normal = lcMul30(ObjectBoxTest.Planes[PlaneIdx], WorldView);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, ObjectBoxTest.Planes[PlaneIdx][3] - lcDot3(WorldView[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		ObjectBoxTest.Objects.Add(const_cast<lcCamera*>(this));
		return;
	}

	const lcMatrix44 ViewWorld = lcMatrix44AffineInverse(mWorldView);
	const lcVector3 UpVectorPosition = lcMul31(lcVector3(0, 25, 0), ViewWorld);

	WorldView = mWorldView;
	WorldView.SetTranslation(lcMul30(-UpVectorPosition, WorldView));

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		const lcVector3 Normal = lcMul30(ObjectBoxTest.Planes[PlaneIdx], WorldView);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, ObjectBoxTest.Planes[PlaneIdx][3] - lcDot3(WorldView[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		ObjectBoxTest.Objects.Add(const_cast<lcCamera*>(this));
		return;
	}
}

void lcCamera::InsertTime(lcStep Start, lcStep Time)
{
	mPosition.InsertTime(Start, Time);
	mTargetPosition.InsertTime(Start, Time);
	mUpVector.InsertTime(Start, Time);
}

void lcCamera::RemoveTime(lcStep Start, lcStep Time)
{
	mPosition.RemoveTime(Start, Time);
	mTargetPosition.RemoveTime(Start, Time);
	mUpVector.RemoveTime(Start, Time);
}

void lcCamera::ZoomExtents(float AspectRatio, const lcVector3& Center, const std::vector<lcVector3>& Points, lcStep Step, bool AddKey)
{
	lcVector3 Position, TargetPosition;

	if (IsOrtho())
	{
		float MinX = FLT_MAX, MaxX = -FLT_MAX, MinY = FLT_MAX, MaxY = -FLT_MAX;

		for (lcVector3 Point : Points)
		{
			Point = lcMul30(Point, mWorldView);

			MinX = lcMin(MinX, Point.x);
			MinY = lcMin(MinY, Point.y);
			MaxX = lcMax(MaxX, Point.x);
			MaxY = lcMax(MaxY, Point.y);
		}

		const lcVector3 ViewCenter = lcMul30(Center, mWorldView);
		float Width = qMax(fabsf(MaxX - ViewCenter.x), fabsf(ViewCenter.x - MinX)) * 2;
		float Height = qMax(fabsf(MaxY - ViewCenter.y), fabsf(ViewCenter.y - MinY)) * 2;

		if (Width > Height * AspectRatio)
			Height = Width / AspectRatio;

		const float f = Height / (m_fovy * (LC_PI / 180.0f));

		const lcVector3 FrontVector(mTargetPosition - mPosition);
		Position = Center - lcNormalize(FrontVector) * f;
		TargetPosition = Center;
	}
	else
	{
		const lcVector3 CenterPosition(mPosition + Center - mTargetPosition);
		const lcMatrix44 ProjectionMatrix = lcMatrix44Perspective(m_fovy, AspectRatio, m_zNear, m_zFar);

		std::tie(Position, std::ignore) = lcZoomExtents(CenterPosition, mWorldView, ProjectionMatrix, Points.data(), Points.size());
		TargetPosition = Center;
	}

	if (IsSimple())
		AddKey = false;

	mPosition.ChangeKey(Position, Step, AddKey);
	mTargetPosition.ChangeKey(TargetPosition, Step, AddKey);

	UpdatePosition(Step);
}

void lcCamera::ZoomRegion(float AspectRatio, const lcVector3& Position, const lcVector3& TargetPosition, const lcVector3* Corners, lcStep Step, bool AddKey)
{
	lcVector3 NewPosition;

	if (IsOrtho())
	{
		float MinX = FLT_MAX, MaxX = -FLT_MAX, MinY = FLT_MAX, MaxY = -FLT_MAX;

		for (int PointIdx = 0; PointIdx < 2; PointIdx++)
		{
			const lcVector3 Point = lcMul30(Corners[PointIdx], mWorldView);

			MinX = lcMin(MinX, Point.x);
			MinY = lcMin(MinY, Point.y);
			MaxX = lcMax(MaxX, Point.x);
			MaxY = lcMax(MaxY, Point.y);
		}

		float Width = MaxX - MinX;
		float Height = MaxY - MinY;

		if (Width > Height * AspectRatio)
			Height = Width / AspectRatio;

		const float f = Height / (m_fovy * (LC_PI / 180.0f));

		const lcVector3 FrontVector(mTargetPosition - mPosition);
		NewPosition = TargetPosition - lcNormalize(FrontVector) * f;
	}
	else
	{
		const lcMatrix44 WorldView = lcMatrix44LookAt(Position, TargetPosition, mUpVector);
		const lcMatrix44 ProjectionMatrix = lcMatrix44Perspective(m_fovy, AspectRatio, m_zNear, m_zFar);

		std::tie(NewPosition, std::ignore) = lcZoomExtents(Position, WorldView, ProjectionMatrix, Corners, 2);
	}

	if (IsSimple())
		AddKey = false;

	mPosition.ChangeKey(NewPosition, Step, AddKey);
	mTargetPosition.ChangeKey(TargetPosition, Step, AddKey);

	UpdatePosition(Step);
}

void lcCamera::Zoom(float Distance, lcStep Step, bool AddKey)
{
	lcVector3 FrontVector(mPosition - mTargetPosition);
	FrontVector.Normalize();
	FrontVector *= -5.0f * Distance;

	// Don't zoom ortho in if it would cross the ortho focal plane.
	if (IsOrtho())
	{
		if ((Distance > 0) && (lcDot(mPosition + FrontVector - mTargetPosition, mPosition - mTargetPosition) <= 0))
			return;
	}

	if (IsSimple())
		AddKey = false;

	mPosition.ChangeKey(mPosition + FrontVector, Step, AddKey);

	if (!IsOrtho())
		mTargetPosition.ChangeKey(mTargetPosition + FrontVector, Step, AddKey);

	UpdatePosition(Step);
}

void lcCamera::Pan(const lcVector3& Distance, lcStep Step, bool AddKey)
{
	if (IsSimple())
		AddKey = false;

	mPosition.ChangeKey(mPosition + Distance, Step, AddKey);
	mTargetPosition.ChangeKey(mTargetPosition + Distance, Step, AddKey);

	UpdatePosition(Step);
}

void lcCamera::Orbit(float DistanceX, float DistanceY, const lcVector3& CenterPosition, lcStep Step, bool AddKey)
{
	lcVector3 FrontVector(mPosition - mTargetPosition);
	lcVector3 UpVector = mUpVector;

	lcVector3 Z(lcNormalize(lcVector3(FrontVector[0], FrontVector[1], 0)));
	if (qIsNaN(Z[0]) || qIsNaN(Z[1]))
		Z = lcNormalize(lcVector3(UpVector[0], UpVector[1], 0));

	if (UpVector[2] < 0)
	{
		Z[0] = -Z[0];
		Z[1] = -Z[1];
	}
 
	const lcMatrix44 YRot(lcVector4(Z[0], Z[1], 0.0f, 0.0f), lcVector4(-Z[1], Z[0], 0.0f, 0.0f), lcVector4(0.0f, 0.0f, 1.0f, 0.0f), lcVector4(0.0f, 0.0f, 0.0f, 1.0f));
	const lcMatrix44 Transform = lcMul(lcMul(lcMul(lcMatrix44AffineInverse(YRot), lcMatrix44RotationY(DistanceY)), YRot), lcMatrix44RotationZ(-DistanceX));

	lcVector3 Position = lcMul31(mPosition - CenterPosition, Transform) + CenterPosition;
	lcVector3 TargetPosition = lcMul31(mTargetPosition - CenterPosition, Transform) + CenterPosition;
	UpVector = lcMul31(mUpVector, Transform);

	if (IsSimple())
		AddKey = false;

	mPosition.ChangeKey(Position, Step, AddKey);
	mTargetPosition.ChangeKey(TargetPosition, Step, AddKey);
	mUpVector.ChangeKey(UpVector, Step, AddKey);

	UpdatePosition(Step);
}

void lcCamera::Roll(float Distance, lcStep Step, bool AddKey)
{
	const lcVector3 FrontVector(mPosition - mTargetPosition);
	const lcMatrix44 Rotation = lcMatrix44FromAxisAngle(FrontVector, Distance);

	const lcVector3 UpVector = lcMul30(mUpVector, Rotation);

	if (IsSimple())
		AddKey = false;

	mUpVector.ChangeKey(UpVector, Step, AddKey);

	UpdatePosition(Step);
}

void lcCamera::Center(const lcVector3& NewCenter, lcStep Step, bool AddKey)
{
	const lcMatrix44 Inverse = lcMatrix44AffineInverse(mWorldView);
	const lcVector3 Direction = -lcVector3(Inverse[2]);

//	float Yaw, Pitch, Roll;
	float Roll;

	if (fabsf(Direction.z) < 0.9999f)
	{
//		Yaw = atan2f(Direction.y, Direction.x);
//		Pitch = asinf(Direction.z);
		Roll = atan2f(Inverse[0][2], Inverse[1][2]);
	}
	else
	{
//		Yaw = 0.0f;
//		Pitch = asinf(Direction.z);
		Roll = atan2f(Inverse[0][1], Inverse[1][1]);
	}

	lcVector3 FrontVector(mPosition - NewCenter);
	const lcMatrix44 Rotation = lcMatrix44FromAxisAngle(FrontVector, Roll);

	lcVector3 UpVector(0, 0, 1), SideVector;
	FrontVector.Normalize();
	if (fabsf(lcDot(UpVector, FrontVector)) > 0.99f)
		SideVector = lcVector3(-1, 0, 0);
	else
		SideVector = lcCross(FrontVector, UpVector);
	UpVector = lcCross(SideVector, FrontVector);
	UpVector.Normalize();
	UpVector = lcMul30(UpVector, Rotation);

	if (IsSimple())
		AddKey = false;

	mTargetPosition.ChangeKey(NewCenter, Step, AddKey);
	mUpVector.ChangeKey(UpVector, Step, AddKey);

	UpdatePosition(Step);
}

void lcCamera::SetViewpoint(lcViewpoint Viewpoint)
{
	const lcVector3 Positions[] =
	{
		lcVector3(    0.0f, -1250.0f,     0.0f), // lcViewpoint::Front
		lcVector3(    0.0f,  1250.0f,     0.0f), // lcViewpoint::Back
		lcVector3(    0.0f,     0.0f,  1250.0f), // lcViewpoint::Top
		lcVector3(    0.0f,     0.0f, -1250.0f), // lcViewpoint::Bottom
		lcVector3( 1250.0f,     0.0f,     0.0f), // lcViewpoint::Left
		lcVector3(-1250.0f,     0.0f,     0.0f), // lcViewpoint::Right
		lcVector3(  375.0f,  -375.0f,   187.5f)  // lcViewpoint::Home
	};

	const lcVector3 Ups[] =
	{
		lcVector3(0.0f, 0.0f, 1.0f),
		lcVector3(0.0f, 0.0f, 1.0f),
		lcVector3(0.0f, 1.0f, 0.0f),
		lcVector3(0.0f,-1.0f, 0.0f),
		lcVector3(0.0f, 0.0f, 1.0f),
		lcVector3(0.0f, 0.0f, 1.0f),
		lcVector3(0.2357f, -0.2357f, 0.94281f)
	};

	mPosition.ChangeKey(Positions[static_cast<int>(Viewpoint)], 1, false);
	mTargetPosition.ChangeKey(lcVector3(0, 0, 0), 1, false);
	mUpVector.ChangeKey(Ups[static_cast<int>(Viewpoint)], 1, false);

	UpdatePosition(1);
}

void lcCamera::SetViewpoint(const lcVector3& Position)
{
	lcVector3 UpVector(0, 0, 1), FrontVector(Position), SideVector;
	FrontVector.Normalize();
	if (fabsf(lcDot(UpVector, FrontVector)) > 0.99f)
		SideVector = lcVector3(-1, 0, 0);
	else
		SideVector = lcCross(FrontVector, UpVector);
	UpVector = lcCross(SideVector, FrontVector);
	UpVector.Normalize();

	mPosition.ChangeKey(Position, 1, false);
	mTargetPosition.ChangeKey(lcVector3(0, 0, 0), 1, false);
	mUpVector.ChangeKey(UpVector, 1, false);

	UpdatePosition(1);
}

void lcCamera::SetViewpoint(const lcVector3& Position, const lcVector3& TargetPosition, const lcVector3& Up)
{
	const lcVector3 Direction = TargetPosition - Position;
	lcVector3 UpVector, SideVector;
	SideVector = lcCross(Direction, Up);
	UpVector = lcCross(SideVector, Direction);
	UpVector.Normalize();

	mPosition.ChangeKey(Position, 1, false);
	mTargetPosition.ChangeKey(TargetPosition, 1, false);
	mUpVector.ChangeKey(UpVector, 1, false);

	UpdatePosition(1);
}

void lcCamera::SetAngles(float Latitude, float Longitude, float Distance)
{
	lcVector3 Position = lcVector3(0, -1, 0);
	lcVector3 TargetPosition = lcVector3(0, 0, 0);
	lcVector3 UpVector = lcVector3(0, 0, 1);

	const lcMatrix33 LongitudeMatrix = lcMatrix33RotationZ(LC_DTOR * Longitude);
	Position = lcMul(Position, LongitudeMatrix);

	const lcVector3 SideVector = lcMul(lcVector3(-1, 0, 0), LongitudeMatrix);
	const lcMatrix33 LatitudeMatrix = lcMatrix33FromAxisAngle(SideVector, LC_DTOR * Latitude);
	Position = lcMul(Position, LatitudeMatrix) * Distance;
	UpVector = lcMul(UpVector, LatitudeMatrix);

	mPosition.ChangeKey(Position, 1, false);
	mTargetPosition.ChangeKey(TargetPosition, 1, false);
	mUpVector.ChangeKey(UpVector, 1, false);

	UpdatePosition(1);
}

void lcCamera::GetAngles(float& Latitude, float& Longitude, float& Distance) const
{
	lcVector3 FrontVector(mPosition - mTargetPosition);
	const lcVector3 X(1, 0, 0);
	const lcVector3 Y(0, 1, 0);
	const lcVector3 Z(0, 0, 1);

	FrontVector.Normalize();
	Latitude = acos(lcDot(-FrontVector, Z)) * LC_RTOD - 90.0f;

	const lcVector3 CameraXY = -lcNormalize(lcVector3(FrontVector.x, FrontVector.y, 0.0f));
	Longitude = acos(lcDot(CameraXY, Y)) * LC_RTOD;

	if (lcDot(CameraXY, X) > 0)
		Longitude = -Longitude;

	Distance = lcLength(mPosition);
}
