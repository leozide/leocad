#include "lc_global.h"
#include "lc_math.h"
#include "lc_colors.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "opengl.h"
#include "lc_file.h"
#include "camera.h"
#include "view.h"
#include "tr.h"
#include "lc_application.h"
#include "lc_context.h"

#define LC_CAMERA_SAVE_VERSION 7 // LeoCAD 0.80

lcCamera::lcCamera(bool Simple)
	: lcObject(LC_OBJECT_CAMERA)
{
	Initialize();

	if (Simple)
		mState |= LC_CAMERA_SIMPLE;
	else
	{
		mPosition = lcVector3(-250.0f, -250.0f, 75.0f);
		mTargetPosition = lcVector3(0.0f, 0.0f, 0.0f);
		mOrthoTarget = mTargetPosition;
		mUpVector = lcVector3(-0.2357f, -0.2357f, 0.94281f);

		ChangeKey(mPositionKeys, mPosition, 1, true);
		ChangeKey(mTargetPositionKeys, mTargetPosition, 1, true);
		ChangeKey(mUpVectorKeys, mUpVector, 1, true);

		UpdatePosition(1);
	}
}

lcCamera::lcCamera(float ex, float ey, float ez, float tx, float ty, float tz)
	: lcObject(LC_OBJECT_CAMERA)
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
	m_zFar = 12500.0f;

	mState = 0;
	m_nType = LC_CAMERA_USER;

	m_pTR = NULL;
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
			Stream >> mPosition[0] >> mPosition[1] >> mPosition[2];
		else if (Token == QLatin1String("TARGET_POSITION"))
			Stream >> mTargetPosition[0] >> mTargetPosition[1] >> mTargetPosition[2];
		else if (Token == QLatin1String("UP_VECTOR"))
			Stream >> mUpVector[0] >> mUpVector[1] >> mUpVector[2];
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
	lcuint8 version, ch;

	version = file.ReadU8();

	if (version > LC_CAMERA_SAVE_VERSION)
		return false;

	if (version > 5)
	{
		if (file.ReadU8() != 1)
			return false;

		lcuint16 time;
		float param[4];
		lcuint8 type;
		lcuint32 n;

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
			lcuint8 step;
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
		lcint32 n;

		if (version < 6)
		{
			lcuint16 time;
			float param[4];
			lcuint8 type;

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
			m_nType = file.ReadU8();
		}
	}

	if ((version > 1) && (version < 4))
	{
		lcuint32 show;
		lcint32 user;

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

void lcCamera::FileSave(lcFile& file) const
{
	file.WriteU8(LC_CAMERA_SAVE_VERSION);

	file.WriteU8(1);
	file.WriteU32(mPositionKeys.GetSize() + mTargetPositionKeys.GetSize() + mUpVectorKeys.GetSize());

	for (int KeyIdx = 0; KeyIdx < mPositionKeys.GetSize(); KeyIdx++)
	{
		lcObjectKey<lcVector3>& Key = mPositionKeys[KeyIdx];

		lcuint16 Step = lcMin(Key.Step, 0xFFFFU);
		file.WriteU16(Step);
		file.WriteFloats(Key.Value, 3);
		file.WriteFloat(0);
		file.WriteU8(0);
	}

	for (int KeyIdx = 0; KeyIdx < mTargetPositionKeys.GetSize(); KeyIdx++)
	{
		lcObjectKey<lcVector3>& Key = mTargetPositionKeys[KeyIdx];

		lcuint16 Step = lcMin(Key.Step, 0xFFFFU);
		file.WriteU16(Step);
		file.WriteFloats(Key.Value, 3);
		file.WriteFloat(0);
		file.WriteU8(1);
	}

	for (int KeyIdx = 0; KeyIdx < mUpVectorKeys.GetSize(); KeyIdx++)
	{
		lcObjectKey<lcVector3>& Key = mUpVectorKeys[KeyIdx];

		lcuint16 Step = lcMin(Key.Step, 0xFFFFU);
		file.WriteU16(Step);
		file.WriteFloats(Key.Value, 3);
		file.WriteFloat(0);
		file.WriteU8(2);
	}

	file.WriteU32(0);

	lcuint8 ch = (lcuint8)strlen(m_strName);
	file.WriteU8(ch);
	file.WriteBuffer(m_strName, ch);

	file.WriteFloat(m_fovy);
	file.WriteFloat(m_zFar);
	file.WriteFloat(m_zNear);
	// version 5
	file.WriteU8(mState & LC_CAMERA_HIDDEN ? 1 : 0);
	file.WriteU8(m_nType);
}

/////////////////////////////////////////////////////////////////////////////
// Camera operations

void lcCamera::Move(lcStep Step, bool AddKey, const lcVector3& Distance)
{
	if (IsSimple())
		AddKey = false;

	if (IsSelected(LC_CAMERA_SECTION_POSITION))
	{
		mPosition += Distance;
		lcAlign(mOrthoTarget, mPosition, mTargetPosition);
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

void lcCamera::UpdatePosition(lcStep Step)
{
	mPosition = CalculateKey(mPositionKeys, Step);
	mTargetPosition = CalculateKey(mTargetPositionKeys, Step);
	mUpVector = CalculateKey(mUpVectorKeys, Step);

	lcVector3 FrontVector(mPosition - mTargetPosition);
	lcVector3 SideVector = lcCross(FrontVector, mUpVector);
	mUpVector = lcNormalize(lcCross(SideVector, FrontVector));

	mWorldView = lcMatrix44LookAt(mPosition, mTargetPosition, mUpVector);
}

void lcCamera::CopyPosition(const lcCamera* camera)
{
	m_fovy = camera->m_fovy;
	m_zNear = camera->m_zNear;
	m_zFar = camera->m_zFar;

	mWorldView = camera->mWorldView;
	mPosition = camera->mPosition;
	mTargetPosition = camera->mTargetPosition;
	mOrthoTarget = camera->mOrthoTarget;
	mUpVector = camera->mUpVector;
}

void lcCamera::Render(View* View)
{
	float LineWidth = lcGetPreferences().mLineWidth;
	const lcMatrix44& ViewMatrix = View->mCamera->mWorldView;
	lcContext* Context = View->mContext;

	lcMatrix44 ViewWorld = lcMatrix44AffineInverse(mWorldView);
	lcVector3 UpVectorPosition = lcMul31(lcVector3(0, 1, 0), ViewWorld);

	float Verts[34 + 24 + 4][3] =
	{
		{  0.3f,  0.3f,  0.3f }, { -0.3f,  0.3f,  0.3f },
		{ -0.3f,  0.3f,  0.3f }, { -0.3f, -0.3f,  0.3f },
		{ -0.3f, -0.3f,  0.3f }, {  0.3f, -0.3f,  0.3f },
		{  0.3f, -0.3f,  0.3f }, {  0.3f,  0.3f,  0.3f },
		{  0.3f,  0.3f, -0.3f }, { -0.3f,  0.3f, -0.3f },
		{ -0.3f,  0.3f, -0.3f }, { -0.3f, -0.3f, -0.3f },
		{ -0.3f, -0.3f, -0.3f }, {  0.3f, -0.3f, -0.3f },
		{  0.3f, -0.3f, -0.3f }, {  0.3f,  0.3f, -0.3f },
		{  0.3f,  0.3f,  0.3f }, {  0.3f,  0.3f, -0.3f },
		{ -0.3f,  0.3f,  0.3f }, { -0.3f,  0.3f, -0.3f },
		{ -0.3f, -0.3f,  0.3f }, { -0.3f, -0.3f, -0.3f },
		{  0.3f, -0.3f,  0.3f }, {  0.3f, -0.3f, -0.3f },
		{ -0.3f, -0.3f, -0.6f }, { -0.3f,  0.3f, -0.6f },
		{  0.0f,  0.0f, -0.3f }, { -0.3f, -0.3f, -0.6f },
		{  0.3f, -0.3f, -0.6f }, {  0.0f,  0.0f, -0.3f },
		{  0.3f,  0.3f, -0.6f }, {  0.3f, -0.3f, -0.6f },
		{  0.3f,  0.3f, -0.6f }, { -0.3f,  0.3f, -0.6f },

		{  0.2f,  0.2f,  0.2f }, { -0.2f,  0.2f,  0.2f },
		{ -0.2f,  0.2f,  0.2f }, { -0.2f, -0.2f,  0.2f },
		{ -0.2f, -0.2f,  0.2f }, {  0.2f, -0.2f,  0.2f },
		{  0.2f, -0.2f,  0.2f }, {  0.2f,  0.2f,  0.2f },
		{  0.2f,  0.2f, -0.2f }, { -0.2f,  0.2f, -0.2f },
		{ -0.2f,  0.2f, -0.2f }, { -0.2f, -0.2f, -0.2f },
		{ -0.2f, -0.2f, -0.2f }, {  0.2f, -0.2f, -0.2f },
		{  0.2f, -0.2f, -0.2f }, {  0.2f,  0.2f, -0.2f },
		{  0.2f,  0.2f,  0.2f }, {  0.2f,  0.2f, -0.2f },
		{ -0.2f,  0.2f,  0.2f }, { -0.2f,  0.2f, -0.2f },
		{ -0.2f, -0.2f,  0.2f }, { -0.2f, -0.2f, -0.2f },
		{  0.2f, -0.2f,  0.2f }, {  0.2f, -0.2f, -0.2f },

		{ mPosition[0], mPosition[1], mPosition[2] },
		{ mTargetPosition[0], mTargetPosition[1], mTargetPosition[2] },
		{ mPosition[0], mPosition[1], mPosition[2] },
		{ UpVectorPosition[0], UpVectorPosition[1], UpVectorPosition[2] },
	};

	Context->SetWorldViewMatrix(lcMul(ViewWorld, ViewMatrix));

	if (IsSelected(LC_CAMERA_SECTION_POSITION))
	{
		Context->SetLineWidth(2.0f * LineWidth);
		if (IsFocused(LC_CAMERA_SECTION_POSITION))
			lcSetColorFocused();
		else
			lcSetColorSelected();
	}
	else
	{
		Context->SetLineWidth(LineWidth);
		lcSetColorCamera();
	}

	glVertexPointer(3, GL_FLOAT, 0, Verts);
	glDrawArrays(GL_LINES, 0, 24);
	glDrawArrays(GL_LINE_STRIP, 24, 10);

	lcMatrix44 TargetMat = ViewWorld;
	TargetMat.SetTranslation(mTargetPosition);
	Context->SetWorldViewMatrix(lcMul(TargetMat, ViewMatrix));

	if (IsSelected(LC_CAMERA_SECTION_TARGET))
	{
		Context->SetLineWidth(2.0f * LineWidth);
		if (IsFocused(LC_CAMERA_SECTION_TARGET))
			lcSetColorFocused();
		else
			lcSetColorSelected();
	}
	else
	{
		Context->SetLineWidth(LineWidth);
		lcSetColorCamera();
	}

	glDrawArrays(GL_LINES, 34, 24);

	lcMatrix44 UpVectorMat = ViewWorld;
	UpVectorMat.SetTranslation(UpVectorPosition);
	Context->SetWorldViewMatrix(lcMul(UpVectorMat, ViewMatrix));

	if (IsSelected(LC_CAMERA_SECTION_UPVECTOR))
	{
		Context->SetLineWidth(2.0f * LineWidth);
		if (IsFocused(LC_CAMERA_SECTION_UPVECTOR))
			lcSetColorFocused();
		else
			lcSetColorSelected();
	}
	else
	{
		Context->SetLineWidth(LineWidth);
		lcSetColorCamera();
	}

	glDrawArrays(GL_LINES, 34, 24);

	Context->SetWorldViewMatrix(ViewMatrix);

	lcSetColorCamera();
	Context->SetLineWidth(LineWidth);

	glDrawArrays(GL_LINES, 34 + 24, 4);

	if (IsSelected())
	{
		Context->SetWorldViewMatrix(lcMul(ViewWorld, ViewMatrix));

		float Dist = lcLength(mTargetPosition - mPosition);
		lcMatrix44 Projection = lcMatrix44Perspective(m_fovy, 1.33f, 0.01f, Dist);
		Projection = lcMatrix44Inverse(Projection);
		glMultMatrixf(Projection);

		float ProjVerts[16][3] =
		{
			{  1,  1,  1 }, { -1,  1, 1 },
			{ -1,  1,  1 }, { -1, -1, 1 },
			{ -1, -1,  1 }, {  1, -1, 1 },
			{  1, -1,  1 }, {  1,  1, 1 },
			{  1,  1, -1 }, {  1,  1, 1 },
			{ -1,  1, -1 }, { -1,  1, 1 },
			{ -1, -1, -1 }, { -1, -1, 1 },
			{  1, -1, -1 }, {  1, -1, 1 },
		};

		glVertexPointer(3, GL_FLOAT, 0, ProjVerts);
		glDrawArrays(GL_LINES, 0, 16);
	}
}

void lcCamera::RayTest(lcObjectRayTest& ObjectRayTest) const
{
	lcVector3 Min = lcVector3(-0.3f, -0.3f, -0.3f);
	lcVector3 Max = lcVector3(0.3f, 0.3f, 0.3f);

	lcVector3 Start = lcMul31(ObjectRayTest.Start, mWorldView);
	lcVector3 End = lcMul31(ObjectRayTest.End, mWorldView);

	float Distance;
	if (lcBoundingBoxRayMinIntersectDistance(Min, Max, Start, End, &Distance, NULL) && (Distance < ObjectRayTest.Distance))
	{
		ObjectRayTest.ObjectSection.Object = const_cast<lcCamera*>(this);
		ObjectRayTest.ObjectSection.Section = LC_CAMERA_SECTION_POSITION;
		ObjectRayTest.Distance = Distance;
	}

	Min = lcVector3(-0.2f, -0.2f, -0.2f);
	Max = lcVector3(0.2f, 0.2f, 0.2f);

	lcMatrix44 WorldView = mWorldView;
	WorldView.SetTranslation(lcMul30(-mTargetPosition, WorldView));

	Start = lcMul31(ObjectRayTest.Start, WorldView);
	End = lcMul31(ObjectRayTest.End, WorldView);

	if (lcBoundingBoxRayMinIntersectDistance(Min, Max, Start, End, &Distance, NULL) && (Distance < ObjectRayTest.Distance))
	{
		ObjectRayTest.ObjectSection.Object = const_cast<lcCamera*>(this);
		ObjectRayTest.ObjectSection.Section = LC_CAMERA_SECTION_TARGET;
		ObjectRayTest.Distance = Distance;
	}

	lcMatrix44 ViewWorld = lcMatrix44AffineInverse(mWorldView);
	lcVector3 UpVectorPosition = lcMul31(lcVector3(0, 1, 0), ViewWorld);

	WorldView = mWorldView;
	WorldView.SetTranslation(lcMul30(-UpVectorPosition, WorldView));

	Start = lcMul31(ObjectRayTest.Start, WorldView);
	End = lcMul31(ObjectRayTest.End, WorldView);

	if (lcBoundingBoxRayMinIntersectDistance(Min, Max, Start, End, &Distance, NULL) && (Distance < ObjectRayTest.Distance))
	{
		ObjectRayTest.ObjectSection.Object = const_cast<lcCamera*>(this);
		ObjectRayTest.ObjectSection.Section = LC_CAMERA_SECTION_UPVECTOR;
		ObjectRayTest.Distance = Distance;
	}
}

void lcCamera::BoxTest(lcObjectBoxTest& ObjectBoxTest) const
{
	lcVector3 Min(-0.3f, -0.3f, -0.3f);
	lcVector3 Max(0.3f, 0.3f, 0.3f);

	lcVector4 LocalPlanes[6];

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		lcVector3 Normal = lcMul30(ObjectBoxTest.Planes[PlaneIdx], mWorldView);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, ObjectBoxTest.Planes[PlaneIdx][3] - lcDot3(mWorldView[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		lcObjectSection& ObjectSection = ObjectBoxTest.ObjectSections.Add();
		ObjectSection.Object = const_cast<lcCamera*>(this);
		ObjectSection.Section = LC_CAMERA_SECTION_POSITION;
	}

	Min = lcVector3(-0.2f, -0.2f, -0.2f);
	Max = lcVector3(0.2f, 0.2f, 0.2f);

	lcMatrix44 WorldView = mWorldView;
	WorldView.SetTranslation(lcMul30(-mTargetPosition, WorldView));

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		lcVector3 Normal = lcMul30(ObjectBoxTest.Planes[PlaneIdx], WorldView);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, ObjectBoxTest.Planes[PlaneIdx][3] - lcDot3(WorldView[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		lcObjectSection& ObjectSection = ObjectBoxTest.ObjectSections.Add();
		ObjectSection.Object = const_cast<lcCamera*>(this);
		ObjectSection.Section = LC_CAMERA_SECTION_TARGET;
	}

	lcMatrix44 ViewWorld = lcMatrix44AffineInverse(mWorldView);
	lcVector3 UpVectorPosition = lcMul31(lcVector3(0, 1, 0), ViewWorld);

	WorldView = mWorldView;
	WorldView.SetTranslation(lcMul30(-UpVectorPosition, WorldView));

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		lcVector3 Normal = lcMul30(ObjectBoxTest.Planes[PlaneIdx], WorldView);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, ObjectBoxTest.Planes[PlaneIdx][3] - lcDot3(WorldView[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		lcObjectSection& ObjectSection = ObjectBoxTest.ObjectSections.Add();
		ObjectSection.Object = const_cast<lcCamera*>(this);
		ObjectSection.Section = LC_CAMERA_SECTION_UPVECTOR;
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

void lcCamera::ZoomExtents(float Aspect, const lcVector3& Center, const lcVector3* Points, int NumPoints, lcStep Step, bool AddKey)
{
	lcVector3 Position(mPosition + Center - mTargetPosition);

	lcMatrix44 Projection = lcMatrix44Perspective(m_fovy, Aspect, m_zNear, m_zFar);

	mPosition = lcZoomExtents(Position, mWorldView, Projection, Points, NumPoints);
	mTargetPosition = Center;
	mOrthoTarget = mTargetPosition;

	if (IsSimple())
		AddKey = false;

	ChangeKey(mPositionKeys, mPosition, Step, AddKey);
	ChangeKey(mTargetPositionKeys, mTargetPosition, Step, AddKey);

	UpdatePosition(Step);
}

void lcCamera::ZoomRegion(const lcVector3* Points, float RatioX, float RatioY, lcStep Step, bool AddKey)
{
	// Center camera.
	lcVector3 Eye = mPosition;
	Eye = Eye + (Points[0] - Points[1]);

	lcVector3 Target = mTargetPosition;
	Target = Target + (Points[0] - Points[1]);

	// Zoom in/out.
	float ZoomFactor = -lcMax(RatioX, RatioY) + 0.75f;

	lcVector3 Dir = Points[1] - Points[2];
	mPosition = Eye + Dir * ZoomFactor;
	mTargetPosition = Target + Dir * ZoomFactor;

	// Change the camera and redraw.
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
	FrontVector *= -Distance;

	// Don't zoom ortho in if it would cross the ortho focal plane.
	if (IsOrtho())
	{
		if ((Distance > 0) && (lcDot(mPosition + FrontVector - mOrthoTarget, mPosition - mOrthoTarget) <= 0))
			return;
	}

	mPosition += FrontVector;
	mTargetPosition += FrontVector;

	if (IsSimple())
		AddKey = false;

	ChangeKey(mPositionKeys, mPosition, Step, AddKey);
	ChangeKey(mTargetPositionKeys, mTargetPosition, Step, AddKey);

	UpdatePosition(Step);
}

void lcCamera::Pan(float DistanceX, float DistanceY, lcStep Step, bool AddKey)
{
	lcVector3 FrontVector(mPosition - mTargetPosition);
	lcVector3 SideVector = lcNormalize(lcCross(FrontVector, mUpVector));

	lcVector3 MoveVec = (SideVector * DistanceX) + (mUpVector * -DistanceY);
	mPosition += MoveVec;
	mTargetPosition += MoveVec;
	mOrthoTarget += MoveVec;

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
	if (isnan(Z[0]) || isnan(Z[1]))
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
	lcAlign(mOrthoTarget, mPosition, mTargetPosition);

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

void lcCamera::Center(lcVector3& point, lcStep Step, bool AddKey)
{
	lcAlign(mTargetPosition, mPosition, point);

	if (IsSimple())
		AddKey = false;

	ChangeKey(mPositionKeys, mPosition, Step, AddKey);
	ChangeKey(mTargetPositionKeys, mTargetPosition, Step, AddKey);

	UpdatePosition(Step);
}

void lcCamera::SetViewpoint(lcViewpoint Viewpoint)
{
	lcVector3 Positions[] =
	{
		lcVector3(    0.0f, -1250.0f,     0.0f), // LC_VIEWPOINT_FRONT
		lcVector3(    0.0f,  1250.0f,     0.0f), // LC_VIEWPOINT_BACK
		lcVector3(    0.0f,     0.0f,  1250.0f), // LC_VIEWPOINT_TOP
		lcVector3(    0.0f,     0.0f, -1250.0f), // LC_VIEWPOINT_BOTTOM
		lcVector3( 1250.0f,     0.0f,     0.0f), // LC_VIEWPOINT_LEFT
		lcVector3(-1250.0f,     0.0f,     0.0f), // LC_VIEWPOINT_RIGHT
		lcVector3( -375.0f,  -375.0f,   187.5f)  // LC_VIEWPOINT_HOME
	};

	lcVector3 Ups[] =
	{
		lcVector3( 0.0f, 0.0f, 1.0f),
		lcVector3( 0.0f, 0.0f, 1.0f),
		lcVector3( 0.0f, 1.0f, 0.0f),
		lcVector3( 0.0f,-1.0f, 0.0f),
		lcVector3( 0.0f, 0.0f, 1.0f),
		lcVector3( 0.0f, 0.0f, 1.0f),
		lcVector3(-0.2357f, -0.2357f, 0.94281f)
	};

	mPosition = Positions[Viewpoint];
	mTargetPosition = lcVector3(0, 0, 0);
	mOrthoTarget = mTargetPosition;
	mUpVector = Ups[Viewpoint];

	ChangeKey(mPositionKeys, mPosition, 1, false);
	ChangeKey(mTargetPositionKeys, mTargetPosition, 1, false);
	ChangeKey(mUpVectorKeys, mUpVector, 1, false);

	UpdatePosition(1);
}

void lcCamera::StartTiledRendering(int tw, int th, int iw, int ih, float AspectRatio)
{
	m_pTR = new TiledRender();
	m_pTR->TileSize(tw, th, 0);
	m_pTR->ImageSize(iw, ih);
	if (IsOrtho())
	{
		float f = (mPosition - mOrthoTarget).Length();
		float d = (m_fovy * f) * (LC_PI / 180.0f);
		float r = d / 2;

		float right = r * AspectRatio;
		m_pTR->Ortho(-right, right, -r, r, m_zNear, m_zFar * 4);
	}
	else
		m_pTR->Perspective(m_fovy, AspectRatio, m_zNear, m_zFar);
}

void lcCamera::GetTileInfo(int* row, int* col, int* width, int* height)
{
	if (m_pTR != NULL)
	{
		*row = m_pTR->m_Rows - m_pTR->m_CurrentRow - 1;
		*col = m_pTR->m_CurrentColumn;
		*width = m_pTR->m_CurrentTileWidth;
		*height = m_pTR->m_CurrentTileHeight;
	}
}

bool lcCamera::EndTile()
{
	if (m_pTR != NULL)
	{
		if (m_pTR->EndTile())
			return true;

		delete m_pTR;
		m_pTR = NULL;
	}

	return false;
}

void lcCamera::SetFocalPoint(const lcVector3& focus, lcStep Step, bool AddKey)
{
	if (IsOrtho())
	{
		lcVector3 FocusVector = focus;
		lcAlign(FocusVector, mPosition, mTargetPosition);
		lcAlign(mOrthoTarget, mPosition, mTargetPosition);
		lcVector3 TranslateVector = FocusVector - mOrthoTarget;
		mPosition += TranslateVector;
		mTargetPosition += TranslateVector;
		mOrthoTarget = FocusVector;
	}
	else
	{
		mOrthoTarget = focus;
	}

	if (IsSimple())
		AddKey = false;

	ChangeKey(mPositionKeys, mPosition, Step, AddKey);
	ChangeKey(mTargetPositionKeys, mTargetPosition, Step, AddKey);

	UpdatePosition(Step);
}
