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
		mPosition = lcVector3(-250.0f, -250.0f, 75.0f);
		mTargetPosition = lcVector3(0.0f, 0.0f, 0.0f);
		mUpVector = lcVector3(-0.2357f, -0.2357f, 0.94281f);

		ChangeKey(mPositionKeys, mPosition, 1, true);
		ChangeKey(mTargetPositionKeys, mTargetPosition, 1, true);
		ChangeKey(mUpVectorKeys, mUpVector, 1, true);

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

	ChangeKey(mPositionKeys, lcVector3(ex, ey, ez), 1, true);
	ChangeKey(mTargetPositionKeys, lcVector3(tx, ty, tz), 1, true);
	ChangeKey(mUpVectorKeys, UpVector, 1, true);

	UpdatePosition(1);
}

lcCamera::~lcCamera()
{
}

void lcCamera::Initialize()
{
	m_fovy = 30.0f;
	m_zNear = 25.0f;
	m_zFar = 50000.0f;
	mState = 0;
	memset(m_strName, 0, sizeof(m_strName));
}

void lcCamera::CreateName(const lcArray<lcCamera*>& Cameras)
{
	if (m_strName[0])
	{
		bool Found = false;
		for (int CameraIdx = 0; CameraIdx < Cameras.GetSize(); CameraIdx++)
		{
			if (!strcmp(Cameras[CameraIdx]->m_strName, m_strName))
			{
				Found = true;
				break;
			}
		}

		if (!Found)
			return;
	}

	int i, max = 0;
	const char* Prefix = "Camera ";

	for (int CameraIdx = 0; CameraIdx < Cameras.GetSize(); CameraIdx++)
		if (strncmp(Cameras[CameraIdx]->m_strName, Prefix, strlen(Prefix)) == 0)
			if (sscanf(Cameras[CameraIdx]->m_strName + strlen(Prefix), " %d", &i) == 1)
				if (i > max)
					max = i;

	sprintf(m_strName, "%s %d", Prefix, max+1);
}

void lcCamera::SaveLDraw(QTextStream& Stream) const
{
	QLatin1String LineEnding("\r\n");

	Stream << QLatin1String("0 !LEOCAD CAMERA FOV ") << m_fovy << QLatin1String(" ZNEAR ") << m_zNear << QLatin1String(" ZFAR ") << m_zFar << LineEnding;

	if (mPositionKeys.GetSize() > 1)
		SaveKeysLDraw(Stream, mPositionKeys, "CAMERA POSITION_KEY ");
	else
		Stream << QLatin1String("0 !LEOCAD CAMERA POSITION ") << mPosition[0] << ' ' << mPosition[1] << ' ' << mPosition[2] << LineEnding;

	if (mTargetPositionKeys.GetSize() > 1)
		SaveKeysLDraw(Stream, mTargetPositionKeys, "CAMERA TARGET_POSITION_KEY ");
	else
		Stream << QLatin1String("0 !LEOCAD CAMERA TARGET_POSITION ") << mTargetPosition[0] << ' ' << mTargetPosition[1] << ' ' << mTargetPosition[2] << LineEnding;

	if (mUpVectorKeys.GetSize() > 1)
		SaveKeysLDraw(Stream, mUpVectorKeys, "CAMERA UP_VECTOR_KEY ");
	else
		Stream << QLatin1String("0 !LEOCAD CAMERA UP_VECTOR ") << mUpVector[0] << ' ' << mUpVector[1] << ' ' << mUpVector[2] << LineEnding;

	Stream << QLatin1String("0 !LEOCAD CAMERA ");

	if (IsHidden())
		Stream << QLatin1String("HIDDEN");

	if (IsOrtho())
		Stream << QLatin1String("ORTHOGRAPHIC ");

	Stream << QLatin1String("NAME ") << m_strName << LineEnding;
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
		else if (Token == QLatin1String("POSITION"))
		{
			Stream >> mPosition[0] >> mPosition[1] >> mPosition[2];
			ChangeKey(mPositionKeys, mPosition, 1, true);
		}
		else if (Token == QLatin1String("TARGET_POSITION"))
		{
			Stream >> mTargetPosition[0] >> mTargetPosition[1] >> mTargetPosition[2];
			ChangeKey(mTargetPositionKeys, mTargetPosition, 1, true);
		}
		else if (Token == QLatin1String("UP_VECTOR"))
		{
			Stream >> mUpVector[0] >> mUpVector[1] >> mUpVector[2];
			ChangeKey(mUpVectorKeys, mUpVector, 1, true);
		}
		else if (Token == QLatin1String("POSITION_KEY"))
			LoadKeysLDraw(Stream, mPositionKeys);
		else if (Token == QLatin1String("TARGET_POSITION_KEY"))
			LoadKeysLDraw(Stream, mTargetPositionKeys);
		else if (Token == QLatin1String("UP_VECTOR_KEY"))
			LoadKeysLDraw(Stream, mUpVectorKeys);
		else if (Token == QLatin1String("NAME"))
		{
			QString Name = Stream.readAll().trimmed();
			QByteArray NameUtf = Name.toUtf8(); // todo: replace with qstring
			strncpy(m_strName, NameUtf.constData(), sizeof(m_strName));
			m_strName[sizeof(m_strName) - 1] = 0;
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

			if (type == 0)
				ChangeKey(mPositionKeys, lcVector3(param[0], param[1], param[2]), time, true);
			else if (type == 1)
				ChangeKey(mTargetPositionKeys, lcVector3(param[0], param[1], param[2]), time, true);
			else if (type == 2)
				ChangeKey(mUpVectorKeys, lcVector3(param[0], param[1], param[2]), time, true);
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
		file.ReadBuffer(m_strName, 80);
		m_strName[80] = 0;
	}
	else
	{
		ch = file.ReadU8();
		if (ch == 0xFF)
			return false; // don't read CString
		file.ReadBuffer(m_strName, ch);
		m_strName[ch] = 0;
	}

	if (version < 3)
	{
		double d[3];
		float f[3];

		file.ReadDoubles(d, 3);
		f[0] = (float)d[0];
		f[1] = (float)d[1];
		f[2] = (float)d[2];
		ChangeKey(mPositionKeys, lcVector3(f[0], f[1], f[2]), 1, true);

		file.ReadDoubles(d, 3);
		f[0] = (float)d[0];
		f[1] = (float)d[1];
		f[2] = (float)d[2];
		ChangeKey(mTargetPositionKeys, lcVector3(f[0], f[1], f[2]), 1, true);

		file.ReadDoubles(d, 3);
		f[0] = (float)d[0];
		f[1] = (float)d[1];
		f[2] = (float)d[2];
		ChangeKey(mUpVectorKeys, lcVector3(f[0], f[1], f[2]), 1, true);
	}

	if (version == 3)
	{
		ch = file.ReadU8();

		while (ch--)
		{
			quint8 step;
			double eye[3], target[3], up[3];
			float f[3];

			file.ReadDoubles(eye, 3);
			file.ReadDoubles(target, 3);
			file.ReadDoubles(up, 3);
			file.ReadU8(&step, 1);

			if (up[0] == 0 && up[1] == 0 && up[2] == 0)
				up[2] = 1;

			f[0] = (float)eye[0];
			f[1] = (float)eye[1];
			f[2] = (float)eye[2];
			ChangeKey(mPositionKeys, lcVector3(f[0], f[1], f[2]), step, true);

			f[0] = (float)target[0];
			f[1] = (float)target[1];
			f[2] = (float)target[2];
			ChangeKey(mTargetPositionKeys, lcVector3(f[0], f[1], f[2]), step, true);

			f[0] = (float)up[0];
			f[1] = (float)up[1];
			f[2] = (float)up[2];
			ChangeKey(mUpVectorKeys, lcVector3(f[0], f[1], f[2]), step, true);

			file.ReadS32(); // snapshot
			file.ReadS32(); // cam
		}
	}

	if (version < 4)
	{
		m_fovy = (float)file.ReadDouble();
		m_zFar = (float)file.ReadDouble();
		m_zNear= (float)file.ReadDouble();
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

				if (type == 0)
					ChangeKey(mPositionKeys, lcVector3(param[0], param[1], param[2]), time, true);
				else if (type == 1)
					ChangeKey(mTargetPositionKeys, lcVector3(param[0], param[1], param[2]), time, true);
				else if (type == 2)
					ChangeKey(mUpVectorKeys, lcVector3(param[0], param[1], param[2]), time, true);
			}

			n = file.ReadS32();
			while (n--)
			{
				file.ReadU16(&time, 1);
				file.ReadFloats(param, 3);
				file.ReadU8(&type, 1);
			}
		}

		file.ReadFloats(&m_fovy, 1);
		file.ReadFloats(&m_zFar, 1);
		file.ReadFloats(&m_zNear, 1);

		if (version < 5)
		{
			n = file.ReadS32();
			if (n != 0)
				mState |= LC_CAMERA_HIDDEN;
		}
		else
		{
			ch = file.ReadU8();
			if (ch & 1)
				mState |= LC_CAMERA_HIDDEN;
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
		if (show == 0)
			mState |= LC_CAMERA_HIDDEN;
	}

	if (version < 7)
	{
		m_zFar *= 25.0f;
		m_zNear *= 25.0f;

		for (int KeyIdx = 0; KeyIdx < mPositionKeys.GetSize(); KeyIdx++)
			mPositionKeys[KeyIdx].Value *= 25.0f;

		for (int KeyIdx = 0; KeyIdx < mTargetPositionKeys.GetSize(); KeyIdx++)
			mTargetPositionKeys[KeyIdx].Value *= 25.0f;
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
		mPosition += Distance;
		ChangeKey(mPositionKeys, mPosition, Step, AddKey);
	}

	if (IsSelected(LC_CAMERA_SECTION_TARGET))
	{
		mTargetPosition += Distance;
		ChangeKey(mTargetPositionKeys, mTargetPosition, Step, AddKey);
	}
	else if (IsSelected(LC_CAMERA_SECTION_UPVECTOR))
	{
		mUpVector += Distance;
		mUpVector.Normalize();
		ChangeKey(mUpVectorKeys, mUpVector, Step, AddKey);
	}

	lcVector3 FrontVector(mTargetPosition - mPosition);
	lcVector3 SideVector = lcCross(FrontVector, mUpVector);

	if (fabsf(lcDot(mUpVector, SideVector)) > 0.99f)
		SideVector = lcVector3(1, 0, 0);

	mUpVector = lcCross(SideVector, FrontVector);
	mUpVector.Normalize();
}

void lcCamera::MoveRelative(const lcVector3& Distance, lcStep Step, bool AddKey)
{
	if (IsSimple())
		AddKey = false;

	lcVector3 Relative = lcMul30(Distance, lcMatrix44Transpose(mWorldView)) * 5.0f;

	mPosition += Relative;
	ChangeKey(mPositionKeys, mPosition, Step, AddKey);

	mTargetPosition += Relative;
	ChangeKey(mTargetPositionKeys, mTargetPosition, Step, AddKey);

	UpdatePosition(Step);
}

void lcCamera::UpdatePosition(lcStep Step)
{
	if (!IsSimple())
	{
		mPosition = CalculateKey(mPositionKeys, Step);
		mTargetPosition = CalculateKey(mTargetPositionKeys, Step);
		mUpVector = CalculateKey(mUpVectorKeys, Step);
	}

	lcVector3 FrontVector(mPosition - mTargetPosition);
	lcVector3 SideVector = lcCross(FrontVector, mUpVector);
	mUpVector = lcNormalize(lcCross(SideVector, FrontVector));

	mWorldView = lcMatrix44LookAt(mPosition, mTargetPosition, mUpVector);
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

	lcMatrix44 CameraViewMatrix = lcMul(ViewWorldMatrix, lcMatrix44Translation(mPosition));
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

	float LineWidth = lcGetPreferences().mLineWidth;

	if (!IsSelected())
	{
		Context->SetLineWidth(LineWidth);
		Context->SetInterfaceColor(LC_COLOR_CAMERA);

		Context->DrawIndexedPrimitives(GL_LINES, 40 + 24 + 24 + 4, GL_UNSIGNED_SHORT, 0);
	}
	else
	{
		if (IsSelected(LC_CAMERA_SECTION_POSITION))
		{
			Context->SetLineWidth(2.0f * LineWidth);
			if (IsFocused(LC_CAMERA_SECTION_POSITION))
				Context->SetInterfaceColor(LC_COLOR_FOCUSED);
			else
				Context->SetInterfaceColor(LC_COLOR_SELECTED);
		}
		else
		{
			Context->SetLineWidth(LineWidth);
			Context->SetInterfaceColor(LC_COLOR_CAMERA);
		}

		Context->DrawIndexedPrimitives(GL_LINES, 40, GL_UNSIGNED_SHORT, 0);

		if (IsSelected(LC_CAMERA_SECTION_TARGET))
		{
			Context->SetLineWidth(2.0f * LineWidth);
			if (IsFocused(LC_CAMERA_SECTION_TARGET))
				Context->SetInterfaceColor(LC_COLOR_FOCUSED);
			else
				Context->SetInterfaceColor(LC_COLOR_SELECTED);
		}
		else
		{
			Context->SetLineWidth(LineWidth);
			Context->SetInterfaceColor(LC_COLOR_CAMERA);
		}

		Context->DrawIndexedPrimitives(GL_LINES, 24, GL_UNSIGNED_SHORT, 40 * 2);

		if (IsSelected(LC_CAMERA_SECTION_UPVECTOR))
		{
			Context->SetLineWidth(2.0f * LineWidth);
			if (IsFocused(LC_CAMERA_SECTION_UPVECTOR))
				Context->SetInterfaceColor(LC_COLOR_FOCUSED);
			else
				Context->SetInterfaceColor(LC_COLOR_SELECTED);
		}
		else
		{
			Context->SetLineWidth(LineWidth);
			Context->SetInterfaceColor(LC_COLOR_CAMERA);
		}

		Context->DrawIndexedPrimitives(GL_LINES, 24, GL_UNSIGNED_SHORT, (40 + 24) * 2);

		Context->SetInterfaceColor(LC_COLOR_CAMERA);
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

void lcCamera::RemoveKeyFrames()
{
	mPositionKeys.RemoveAll();
	ChangeKey(mPositionKeys, mPosition, 1, true);

	mTargetPositionKeys.RemoveAll();
	ChangeKey(mTargetPositionKeys, mTargetPosition, 1, true);

	mUpVectorKeys.RemoveAll();
	ChangeKey(mUpVectorKeys, mUpVector, 1, true);
}

void lcCamera::RayTest(lcObjectRayTest& ObjectRayTest) const
{
	lcVector3 Min = lcVector3(-LC_CAMERA_POSITION_EDGE, -LC_CAMERA_POSITION_EDGE, -LC_CAMERA_POSITION_EDGE);
	lcVector3 Max = lcVector3(LC_CAMERA_POSITION_EDGE, LC_CAMERA_POSITION_EDGE, LC_CAMERA_POSITION_EDGE);

	lcVector3 Start = lcMul31(ObjectRayTest.Start, mWorldView);
	lcVector3 End = lcMul31(ObjectRayTest.End, mWorldView);

	float Distance;
	if (lcBoundingBoxRayIntersectDistance(Min, Max, Start, End, &Distance, nullptr) && (Distance < ObjectRayTest.Distance))
	{
		ObjectRayTest.ObjectSection.Object = const_cast<lcCamera*>(this);
		ObjectRayTest.ObjectSection.Section = LC_CAMERA_SECTION_POSITION;
		ObjectRayTest.Distance = Distance;
	}

	Min = lcVector3(-LC_CAMERA_TARGET_EDGE, -LC_CAMERA_TARGET_EDGE, -LC_CAMERA_TARGET_EDGE);
	Max = lcVector3(LC_CAMERA_TARGET_EDGE, LC_CAMERA_TARGET_EDGE, LC_CAMERA_TARGET_EDGE);

	lcMatrix44 WorldView = mWorldView;
	WorldView.SetTranslation(lcMul30(-mTargetPosition, WorldView));

	Start = lcMul31(ObjectRayTest.Start, WorldView);
	End = lcMul31(ObjectRayTest.End, WorldView);

	if (lcBoundingBoxRayIntersectDistance(Min, Max, Start, End, &Distance, nullptr) && (Distance < ObjectRayTest.Distance))
	{
		ObjectRayTest.ObjectSection.Object = const_cast<lcCamera*>(this);
		ObjectRayTest.ObjectSection.Section = LC_CAMERA_SECTION_TARGET;
		ObjectRayTest.Distance = Distance;
	}

	lcMatrix44 ViewWorld = lcMatrix44AffineInverse(mWorldView);
	lcVector3 UpVectorPosition = lcMul31(lcVector3(0, 25, 0), ViewWorld);

	WorldView = mWorldView;
	WorldView.SetTranslation(lcMul30(-UpVectorPosition, WorldView));

	Start = lcMul31(ObjectRayTest.Start, WorldView);
	End = lcMul31(ObjectRayTest.End, WorldView);

	if (lcBoundingBoxRayIntersectDistance(Min, Max, Start, End, &Distance, nullptr) && (Distance < ObjectRayTest.Distance))
	{
		ObjectRayTest.ObjectSection.Object = const_cast<lcCamera*>(this);
		ObjectRayTest.ObjectSection.Section = LC_CAMERA_SECTION_UPVECTOR;
		ObjectRayTest.Distance = Distance;
	}
}

void lcCamera::BoxTest(lcObjectBoxTest& ObjectBoxTest) const
{
	lcVector3 Min(-LC_CAMERA_POSITION_EDGE, -LC_CAMERA_POSITION_EDGE, -LC_CAMERA_POSITION_EDGE);
	lcVector3 Max(LC_CAMERA_POSITION_EDGE, LC_CAMERA_POSITION_EDGE, LC_CAMERA_POSITION_EDGE);

	lcVector4 LocalPlanes[6];

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		lcVector3 Normal = lcMul30(ObjectBoxTest.Planes[PlaneIdx], mWorldView);
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
		lcVector3 Normal = lcMul30(ObjectBoxTest.Planes[PlaneIdx], WorldView);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, ObjectBoxTest.Planes[PlaneIdx][3] - lcDot3(WorldView[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		ObjectBoxTest.Objects.Add(const_cast<lcCamera*>(this));
		return;
	}

	lcMatrix44 ViewWorld = lcMatrix44AffineInverse(mWorldView);
	lcVector3 UpVectorPosition = lcMul31(lcVector3(0, 25, 0), ViewWorld);

	WorldView = mWorldView;
	WorldView.SetTranslation(lcMul30(-UpVectorPosition, WorldView));

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		lcVector3 Normal = lcMul30(ObjectBoxTest.Planes[PlaneIdx], WorldView);
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
	lcObject::InsertTime(mPositionKeys, Start, Time);
	lcObject::InsertTime(mTargetPositionKeys, Start, Time);
	lcObject::InsertTime(mUpVectorKeys, Start, Time);
}

void lcCamera::RemoveTime(lcStep Start, lcStep Time)
{
	lcObject::RemoveTime(mPositionKeys, Start, Time);
	lcObject::RemoveTime(mTargetPositionKeys, Start, Time);
	lcObject::RemoveTime(mUpVectorKeys, Start, Time);
}

void lcCamera::ZoomExtents(float AspectRatio, const lcVector3& Center, const lcVector3* Points, int NumPoints, lcStep Step, bool AddKey)
{
	if (IsOrtho())
	{
		float MinX = FLT_MAX, MaxX = -FLT_MAX, MinY = FLT_MAX, MaxY = -FLT_MAX;

		for (int PointIdx = 0; PointIdx < NumPoints; PointIdx++)
		{
			lcVector3 Point = lcMul30(Points[PointIdx], mWorldView);

			MinX = lcMin(MinX, Point.x);
			MinY = lcMin(MinY, Point.y);
			MaxX = lcMax(MaxX, Point.x);
			MaxY = lcMax(MaxY, Point.y);
		}

		float Width = MaxX - MinX;
		float Height = MaxY - MinY;

		if (Width > Height * AspectRatio)
			Height = Width / AspectRatio;

		float f = Height / (m_fovy * (LC_PI / 180.0f));

		lcVector3 FrontVector(mTargetPosition - mPosition);
		mPosition = Center - lcNormalize(FrontVector) * f;
		mTargetPosition = Center;
	}
	else
	{
		lcVector3 Position(mPosition + Center - mTargetPosition);
		lcMatrix44 ProjectionMatrix = lcMatrix44Perspective(m_fovy, AspectRatio, m_zNear, m_zFar);

		std::tie(mPosition, std::ignore) = lcZoomExtents(Position, mWorldView, ProjectionMatrix, Points, NumPoints);
		mTargetPosition = Center;
	}

	if (IsSimple())
		AddKey = false;

	ChangeKey(mPositionKeys, mPosition, Step, AddKey);
	ChangeKey(mTargetPositionKeys, mTargetPosition, Step, AddKey);

	UpdatePosition(Step);
}

void lcCamera::ZoomRegion(float AspectRatio, const lcVector3& Position, const lcVector3& TargetPosition, const lcVector3* Corners, lcStep Step, bool AddKey)
{
	if (IsOrtho())
	{
		float MinX = FLT_MAX, MaxX = -FLT_MAX, MinY = FLT_MAX, MaxY = -FLT_MAX;

		for (int PointIdx = 0; PointIdx < 2; PointIdx++)
		{
			lcVector3 Point = lcMul30(Corners[PointIdx], mWorldView);

			MinX = lcMin(MinX, Point.x);
			MinY = lcMin(MinY, Point.y);
			MaxX = lcMax(MaxX, Point.x);
			MaxY = lcMax(MaxY, Point.y);
		}

		float Width = MaxX - MinX;
		float Height = MaxY - MinY;

		if (Width > Height * AspectRatio)
			Height = Width / AspectRatio;

		float f = Height / (m_fovy * (LC_PI / 180.0f));

		lcVector3 FrontVector(mTargetPosition - mPosition);
		mPosition = TargetPosition - lcNormalize(FrontVector) * f;
		mTargetPosition = TargetPosition;
	}
	else
	{
		lcMatrix44 WorldView = lcMatrix44LookAt(Position, TargetPosition, mUpVector);
		lcMatrix44 ProjectionMatrix = lcMatrix44Perspective(m_fovy, AspectRatio, m_zNear, m_zFar);

		std::tie(mPosition, std::ignore) = lcZoomExtents(Position, WorldView, ProjectionMatrix, Corners, 2);
		mTargetPosition = TargetPosition;
	}

	if (IsSimple())
		AddKey = false;

	ChangeKey(mPositionKeys, mPosition, Step, AddKey);
	ChangeKey(mTargetPositionKeys, mTargetPosition, Step, AddKey);

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

		mPosition += FrontVector;
	}
	else
	{
		mPosition += FrontVector;
		mTargetPosition += FrontVector;
	}

	if (IsSimple())
		AddKey = false;

	ChangeKey(mPositionKeys, mPosition, Step, AddKey);
	ChangeKey(mTargetPositionKeys, mTargetPosition, Step, AddKey);

	UpdatePosition(Step);
}

void lcCamera::Pan(const lcVector3& Distance, lcStep Step, bool AddKey)
{
	mPosition += Distance;
	mTargetPosition += Distance;

	if (IsSimple())
		AddKey = false;

	ChangeKey(mPositionKeys, mPosition, Step, AddKey);
	ChangeKey(mTargetPositionKeys, mTargetPosition, Step, AddKey);

	UpdatePosition(Step);
}

void lcCamera::Orbit(float DistanceX, float DistanceY, const lcVector3& CenterPosition, lcStep Step, bool AddKey)
{
	lcVector3 FrontVector(mPosition - mTargetPosition);

	lcVector3 Z(lcNormalize(lcVector3(FrontVector[0], FrontVector[1], 0)));
	if (qIsNaN(Z[0]) || qIsNaN(Z[1]))
		Z = lcNormalize(lcVector3(mUpVector[0], mUpVector[1], 0));

	if (mUpVector[2] < 0)
	{
		Z[0] = -Z[0];
		Z[1] = -Z[1];
	}
 
	lcMatrix44 YRot(lcVector4(Z[0], Z[1], 0.0f, 0.0f), lcVector4(-Z[1], Z[0], 0.0f, 0.0f), lcVector4(0.0f, 0.0f, 1.0f, 0.0f), lcVector4(0.0f, 0.0f, 0.0f, 1.0f));
	lcMatrix44 transform = lcMul(lcMul(lcMul(lcMatrix44AffineInverse(YRot), lcMatrix44RotationY(DistanceY)), YRot), lcMatrix44RotationZ(-DistanceX));

	mPosition = lcMul31(mPosition - CenterPosition, transform) + CenterPosition;
	mTargetPosition = lcMul31(mTargetPosition - CenterPosition, transform) + CenterPosition;

	mUpVector = lcMul31(mUpVector, transform);

	if (IsSimple())
		AddKey = false;

	ChangeKey(mPositionKeys, mPosition, Step, AddKey);
	ChangeKey(mTargetPositionKeys, mTargetPosition, Step, AddKey);
	ChangeKey(mUpVectorKeys, mUpVector, Step, AddKey);

	UpdatePosition(Step);
}

void lcCamera::Roll(float Distance, lcStep Step, bool AddKey)
{
	lcVector3 FrontVector(mPosition - mTargetPosition);
	lcMatrix44 Rotation = lcMatrix44FromAxisAngle(FrontVector, Distance);

	mUpVector = lcMul30(mUpVector, Rotation);

	if (IsSimple())
		AddKey = false;

	ChangeKey(mUpVectorKeys, mUpVector, Step, AddKey);

	UpdatePosition(Step);
}

void lcCamera::Center(const lcVector3& NewCenter, lcStep Step, bool AddKey)
{
	const lcMatrix44 Inverse = lcMatrix44AffineInverse(mWorldView);
	const lcVector3 Direction = -lcVector3(Inverse[2]);

	float Yaw, Pitch, Roll;

	if (fabsf(Direction.z) < 0.9999f)
	{
		Yaw = atan2f(Direction.y, Direction.x);
		Pitch = asinf(Direction.z);
		Roll = atan2f(Inverse[0][2], Inverse[1][2]);
	}
	else
	{
		Yaw = 0.0f;
		Pitch = asinf(Direction.z);
		Roll = atan2f(Inverse[0][1], Inverse[1][1]);
	}

	mTargetPosition = NewCenter;

	lcVector3 FrontVector(mPosition - mTargetPosition);
	lcMatrix44 Rotation = lcMatrix44FromAxisAngle(FrontVector, Roll);

	lcVector3 UpVector(0, 0, 1), SideVector;
	FrontVector.Normalize();
	if (fabsf(lcDot(UpVector, FrontVector)) > 0.99f)
		SideVector = lcVector3(-1, 0, 0);
	else
		SideVector = lcCross(FrontVector, UpVector);
	UpVector = lcCross(SideVector, FrontVector);
	UpVector.Normalize();
	mUpVector = lcMul30(UpVector, Rotation);

	if (IsSimple())
		AddKey = false;

	ChangeKey(mTargetPositionKeys, mTargetPosition, Step, AddKey);
	ChangeKey(mUpVectorKeys, mUpVector, Step, AddKey);

	UpdatePosition(Step);
}

void lcCamera::SetViewpoint(lcViewpoint Viewpoint)
{
	lcVector3 Positions[] =
	{
		lcVector3(    0.0f, -1250.0f,     0.0f), // lcViewpoint::Front
		lcVector3(    0.0f,  1250.0f,     0.0f), // lcViewpoint::Back
		lcVector3(    0.0f,     0.0f,  1250.0f), // lcViewpoint::Top
		lcVector3(    0.0f,     0.0f, -1250.0f), // lcViewpoint::Bottom
		lcVector3( 1250.0f,     0.0f,     0.0f), // lcViewpoint::Left
		lcVector3(-1250.0f,     0.0f,     0.0f), // lcViewpoint::Right
		lcVector3(  375.0f,  -375.0f,   187.5f)  // lcViewpoint::Home
	};

	lcVector3 Ups[] =
	{
		lcVector3(0.0f, 0.0f, 1.0f),
		lcVector3(0.0f, 0.0f, 1.0f),
		lcVector3(0.0f, 1.0f, 0.0f),
		lcVector3(0.0f,-1.0f, 0.0f),
		lcVector3(0.0f, 0.0f, 1.0f),
		lcVector3(0.0f, 0.0f, 1.0f),
		lcVector3(0.2357f, -0.2357f, 0.94281f)
	};

	mPosition = Positions[static_cast<int>(Viewpoint)];
	mTargetPosition = lcVector3(0, 0, 0);
	mUpVector = Ups[static_cast<int>(Viewpoint)];

	ChangeKey(mPositionKeys, mPosition, 1, false);
	ChangeKey(mTargetPositionKeys, mTargetPosition, 1, false);
	ChangeKey(mUpVectorKeys, mUpVector, 1, false);

	UpdatePosition(1);
}

void lcCamera::SetViewpoint(const lcVector3& Position)
{
	mPosition = Position;
	mTargetPosition = lcVector3(0, 0, 0);

	lcVector3 UpVector(0, 0, 1), FrontVector(Position), SideVector;
	FrontVector.Normalize();
	if (fabsf(lcDot(UpVector, FrontVector)) > 0.99f)
		SideVector = lcVector3(-1, 0, 0);
	else
		SideVector = lcCross(FrontVector, UpVector);
	UpVector = lcCross(SideVector, FrontVector);
	UpVector.Normalize();
	mUpVector = UpVector;

	ChangeKey(mPositionKeys, mPosition, 1, false);
	ChangeKey(mTargetPositionKeys, mTargetPosition, 1, false);
	ChangeKey(mUpVectorKeys, mUpVector, 1, false);

	UpdatePosition(1);
}

void lcCamera::SetViewpoint(const lcVector3& Position, const lcVector3& Target, const lcVector3& Up)
{
	mPosition = Position;
	mTargetPosition = Target;

	lcVector3 Direction = Target - Position;
	lcVector3 UpVector, SideVector;
	SideVector = lcCross(Direction, Up);
	UpVector = lcCross(SideVector, Direction);
	UpVector.Normalize();
	mUpVector = Up;

	ChangeKey(mPositionKeys, mPosition, 1, false);
	ChangeKey(mTargetPositionKeys, mTargetPosition, 1, false);
	ChangeKey(mUpVectorKeys, mUpVector, 1, false);

	UpdatePosition(1);
}

void lcCamera::SetAngles(float Latitude, float Longitude, float Distance)
{
	mPosition = lcVector3(0, -1, 0);
	mTargetPosition = lcVector3(0, 0, 0);
	mUpVector = lcVector3(0, 0, 1);

	lcMatrix33 LongitudeMatrix = lcMatrix33RotationZ(LC_DTOR * Longitude);
	mPosition = lcMul(mPosition, LongitudeMatrix);

	lcVector3 SideVector = lcMul(lcVector3(-1, 0, 0), LongitudeMatrix);
	lcMatrix33 LatitudeMatrix = lcMatrix33FromAxisAngle(SideVector, LC_DTOR * Latitude);
	mPosition = lcMul(mPosition, LatitudeMatrix) * Distance;
	mUpVector = lcMul(mUpVector, LatitudeMatrix);

	ChangeKey(mPositionKeys, mPosition, 1, false);
	ChangeKey(mTargetPositionKeys, mTargetPosition, 1, false);
	ChangeKey(mUpVectorKeys, mUpVector, 1, false);

	UpdatePosition(1);
}

void lcCamera::GetAngles(float& Latitude, float& Longitude, float& Distance) const
{
	lcVector3 FrontVector(mPosition - mTargetPosition);
	lcVector3 X(1, 0, 0);
	lcVector3 Y(0, 1, 0);
	lcVector3 Z(0, 0, 1);

	FrontVector.Normalize();
	Latitude = acos(lcDot(-FrontVector, Z)) * LC_RTOD - 90.0f;

	lcVector3 CameraXY = -lcNormalize(lcVector3(FrontVector.x, FrontVector.y, 0.0f));
	Longitude = acos(lcDot(CameraXY, Y)) * LC_RTOD;

	if (lcDot(CameraXY, X) > 0)
		Longitude = -Longitude;

	Distance = lcLength(mPosition);
}
