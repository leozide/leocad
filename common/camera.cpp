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

#define LC_CAMERA_SAVE_VERSION 6 // LeoCAD 0.73

static LC_OBJECT_KEY_INFO camera_key_info[LC_CK_COUNT] =
{
	{ "Camera Position", 3, LC_CK_EYE },
	{ "Camera Target", 3, LC_CK_TARGET },
	{ "Camera Up Vector", 3, LC_CK_UP }
};

lcCamera::lcCamera(bool Simple)
	: Object(LC_OBJECT_CAMERA)
{
	Initialize();

	if (Simple)
		mState |= LC_CAMERA_SIMPLE | LC_CAMERA_ORTHO;
	else
	{
		mPosition = lcVector3(-10.0f, -10.0f, 5.0f);
		mTargetPosition = lcVector3(0.0f, 0.0f, 0.0f);
		mOrthoTarget = mTargetPosition;
		mUpVector = lcVector3(-0.2357f, -0.2357f, 0.94281f);

		ChangeKey(1, true, mPosition, LC_CK_EYE);
		ChangeKey(1, true, mTargetPosition, LC_CK_TARGET);
		ChangeKey(1, true, mUpVector, LC_CK_UP);

		UpdatePosition(1);
	}
}

lcCamera::lcCamera(float ex, float ey, float ez, float tx, float ty, float tz)
	: Object(LC_OBJECT_CAMERA)
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

	float eye[3] = { ex, ey, ez }, target[3] = { tx, ty, tz };

	ChangeKey(1, true, eye, LC_CK_EYE);
	ChangeKey(1, true, target, LC_CK_TARGET);
	ChangeKey(1, true, UpVector, LC_CK_UP);

	UpdatePosition(1);
}

lcCamera::~lcCamera()
{
}

void lcCamera::Initialize()
{
	m_fovy = 30.0f;
	m_zNear = 1.0f;
	m_zFar = 500.0f;

	mState = 0;
	m_nType = LC_CAMERA_USER;

	m_pTR = NULL;
	memset(m_strName, 0, sizeof(m_strName));

	float *values[] = { mPosition, mTargetPosition, mUpVector };
	RegisterKeys(values, camera_key_info, LC_CK_COUNT);
}

void lcCamera::CreateName(const lcArray<Camera*>& Cameras)
{
	int i, max = 0;
	const char* Prefix = "Camera ";

	for (int CameraIdx = 0; CameraIdx < Cameras.GetSize(); CameraIdx++)
		if (strncmp(Cameras[CameraIdx]->m_strName, Prefix, strlen(Prefix)) == 0)
			if (sscanf(Cameras[CameraIdx]->m_strName + strlen(Prefix), " %d", &i) == 1)
				if (i > max)
					max = i;

	sprintf(m_strName, "%s %d", Prefix, max+1);
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
		if (!Object::FileLoad(file))
			return false;

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
		ChangeKey(1, true, f, LC_CK_EYE);

		file.ReadDoubles(d, 3);
		f[0] = (float)d[0];
		f[1] = (float)d[1];
		f[2] = (float)d[2];
		ChangeKey(1, true, f, LC_CK_TARGET);

		file.ReadDoubles(d, 3);
		f[0] = (float)d[0];
		f[1] = (float)d[1];
		f[2] = (float)d[2];
		ChangeKey(1, true, f, LC_CK_UP);
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
			ChangeKey(step, true, f, LC_CK_EYE);

			f[0] = (float)target[0];
			f[1] = (float)target[1];
			f[2] = (float)target[2];
			ChangeKey(step, true, f, LC_CK_TARGET);

			f[0] = (float)up[0];
			f[1] = (float)up[1];
			f[2] = (float)up[2];
			ChangeKey(step, true, f, LC_CK_UP);

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

				ChangeKey(time, true, param, type);
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

	return true;
}

void lcCamera::FileSave(lcFile& file) const
{
	file.WriteU8(LC_CAMERA_SAVE_VERSION);

	Object::FileSave(file);

	lcuint8 ch = (unsigned char)strlen(m_strName);
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

void lcCamera::Move(unsigned short nTime, bool bAddKey, float dx, float dy, float dz)
{
	lcVector3 MoveVec(dx, dy, dz);

	if (IsSelected(LC_CAMERA_SECTION_POSITION))
	{
		mPosition += MoveVec;
		lcAlign(mOrthoTarget, mPosition, mTargetPosition);

		if (!IsSimple())
			ChangeKey(nTime, bAddKey, mPosition, LC_CK_EYE);
	}

	if (IsSelected(LC_CAMERA_SECTION_TARGET))
	{
		mTargetPosition += MoveVec;

		if (!IsSimple())
			ChangeKey(nTime, bAddKey, mTargetPosition, LC_CK_TARGET);
	}

	if (IsSelected(LC_CAMERA_SECTION_UPVECTOR))
	{
		mUpVector += MoveVec;
		mUpVector.Normalize();

		if (!IsSimple())
			ChangeKey(nTime, bAddKey, mTargetPosition, LC_CK_UP);
	}

	lcVector3 FrontVector(mTargetPosition - mPosition);
	lcVector3 SideVector = lcCross(FrontVector, mUpVector);

	if (fabsf(lcDot(mUpVector, SideVector)) > 0.99f)
		SideVector = lcVector3(1, 0, 0);

	mUpVector = lcCross(SideVector, FrontVector);
	mUpVector.Normalize();
}

void lcCamera::UpdatePosition(unsigned short nTime)
{
	if (!IsSimple())
		CalculateKeys(nTime);

	lcVector3 FrontVector(mPosition - mTargetPosition);
	lcVector3 SideVector = lcCross(FrontVector, mUpVector);
	mUpVector = lcNormalize(lcCross(SideVector, FrontVector));

	mWorldView = lcMatrix44LookAt(mPosition, mTargetPosition, mUpVector);
}

void lcCamera::CopyPosition(const Camera* camera)
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
		ObjectRayTest.ObjectSection.Object = const_cast<Camera*>(this);
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
		ObjectRayTest.ObjectSection.Object = const_cast<Camera*>(this);
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
		ObjectRayTest.ObjectSection.Object = const_cast<Camera*>(this);
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
		ObjectSection.Object = const_cast<Camera*>(this);
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
		ObjectSection.Object = const_cast<Camera*>(this);
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
		ObjectSection.Object = const_cast<Camera*>(this);
		ObjectSection.Section = LC_CAMERA_SECTION_UPVECTOR;
	}
}

void lcCamera::LoadProjection(const lcProjection& projection)
{
	if (m_pTR != NULL)
		m_pTR->BeginTile();
	else
	{
		glMatrixMode(GL_PROJECTION);
		mProjection = projection;
		glLoadMatrixf((lcMatrix44&)mProjection);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(mWorldView);
}

void lcCamera::ZoomExtents(View* view, const lcVector3& Center, const lcVector3* Points, int NumPoints, unsigned short nTime, bool bAddKey)
{
	int Viewport[4] = { 0, 0, view->mWidth, view->mHeight };

	float Aspect = (float)Viewport[2]/(float)Viewport[3];

	lcVector3 Position(mPosition + Center - mTargetPosition);

	lcMatrix44 Projection = lcMatrix44Perspective(m_fovy, Aspect, m_zNear, m_zFar);

	mPosition = lcZoomExtents(Position, mWorldView, Projection, Points, NumPoints);
	mTargetPosition = Center;
	mOrthoTarget = mTargetPosition;

	if (!IsSimple())
	{
		ChangeKey(nTime, bAddKey, mPosition, LC_CK_EYE);
		ChangeKey(nTime, bAddKey, mTargetPosition, LC_CK_TARGET);
	}

	UpdatePosition(nTime);
}

void lcCamera::ZoomRegion(View* view, float Left, float Right, float Bottom, float Top, unsigned short nTime, bool bAddKey)
{
	int Viewport[4] = { 0, 0, view->mWidth, view->mHeight };
	float Aspect = (float)Viewport[2]/(float)Viewport[3];

	const lcMatrix44& ModelView = mWorldView;
	lcMatrix44 Projection = lcMatrix44Perspective(m_fovy, Aspect, m_zNear, m_zFar);

	// Unproject screen points to world space.
	lcVector3 Points[3] =
	{
		lcVector3((Left + Right) / 2, (Top + Bottom) / 2, 0.9f),
		lcVector3((float)Viewport[2] / 2.0f, (float)Viewport[3] / 2.0f, 0.9f),
		lcVector3((float)Viewport[2] / 2.0f, (float)Viewport[3] / 2.0f, 0.1f),
	};

	lcUnprojectPoints(Points, 3, ModelView, Projection, Viewport);

	// Center camera.
	lcVector3 Eye = mPosition;
	Eye = Eye + (Points[0] - Points[1]);

	lcVector3 Target = mTargetPosition;
	Target = Target + (Points[0] - Points[1]);

	// Zoom in/out.
	float RatioX = (Right - Left) / Viewport[2];
	float RatioY = (Top - Bottom) / Viewport[3];
	float ZoomFactor = -lcMax(RatioX, RatioY) + 0.75f;

	lcVector3 Dir = Points[1] - Points[2];
	mPosition = Eye + Dir * ZoomFactor;
	mTargetPosition = Target + Dir * ZoomFactor;

	// Change the camera and redraw.
	if (!IsSimple())
	{
		ChangeKey(nTime, bAddKey, mPosition, LC_CK_EYE);
		ChangeKey(nTime, bAddKey, mTargetPosition, LC_CK_TARGET);
	}

	UpdatePosition(nTime);
}

void lcCamera::DoZoom(int dy, int mouse, unsigned short nTime, bool bAddKey)
{
	lcVector3 FrontVector(mPosition - mTargetPosition);
	FrontVector.Normalize();
	FrontVector *= -2.0f * dy / (21 - mouse);

	// Don't zoom ortho in if it would cross the ortho focal plane.
	if (IsOrtho())
	{
		if ((dy > 0) && (lcDot(mPosition + FrontVector - mOrthoTarget, mPosition - mOrthoTarget) <= 0))
			return;
	}

	mPosition += FrontVector;
	mTargetPosition += FrontVector;

	if (!IsSimple())
	{
		ChangeKey(nTime, bAddKey, mPosition, LC_CK_EYE);
		ChangeKey(nTime, bAddKey, mTargetPosition, LC_CK_TARGET);
	}

	UpdatePosition(nTime);
}

void lcCamera::DoPan(int dx, int dy, int mouse, unsigned short nTime, bool bAddKey)
{
	lcVector3 FrontVector(mPosition - mTargetPosition);
	lcVector3 SideVector = lcNormalize(lcCross(FrontVector, mUpVector));

	lcVector3 MoveVec = (SideVector * (2.0f * dx / (21 - mouse))) + (mUpVector * (-2.0f * dy / (21 - mouse)));
	mPosition += MoveVec;
	mTargetPosition += MoveVec;
	mOrthoTarget += MoveVec;

	if (!IsSimple())
	{
		ChangeKey(nTime, bAddKey, mPosition, LC_CK_EYE);
		ChangeKey(nTime, bAddKey, mTargetPosition, LC_CK_TARGET);
	}

	UpdatePosition(nTime);
}

void lcCamera::DoRotate(int dx, int dy, int mouse, unsigned short nTime, bool bAddKey, float* center)
{
	lcVector3 FrontVector(mPosition - mTargetPosition);
	lcVector3 CenterPosition(center[0], center[1], center[2]);

	lcVector3 Z(lcNormalize(lcVector3(FrontVector[0], FrontVector[1], 0)));
	if (isnan(Z[0]) || isnan(Z[1]))
		Z = lcNormalize(lcVector3(mUpVector[0], mUpVector[1], 0));

	if (mUpVector[2] < 0)
	{
		Z[0] = -Z[0];
		Z[1] = -Z[1];
		dx = -dx;
	}
 
	lcMatrix44 YRot(lcVector4(Z[0], Z[1], 0.0f, 0.0f), lcVector4(-Z[1], Z[0], 0.0f, 0.0f), lcVector4(0.0f, 0.0f, 1.0f, 0.0f), lcVector4(0.0f, 0.0f, 0.0f, 1.0f));
	lcMatrix44 transform = lcMul(lcMul(lcMul(lcMatrix44AffineInverse(YRot), lcMatrix44RotationY(0.1f * dy / (21 - mouse))), YRot), lcMatrix44RotationZ(-0.1f * dx / (21 - mouse)));

	mPosition = lcMul31(mPosition - CenterPosition, transform) + CenterPosition;
	mTargetPosition = lcMul31(mTargetPosition - CenterPosition, transform) + CenterPosition;
	lcAlign(mOrthoTarget, mPosition, mTargetPosition);

	mUpVector = lcMul31(mUpVector, transform);

	if (!IsSimple())
	{
		ChangeKey(nTime, bAddKey, mPosition, LC_CK_EYE);
		ChangeKey(nTime, bAddKey, mUpVector, LC_CK_UP);
	}

	UpdatePosition(nTime);
}

void lcCamera::DoRoll(int dx, int mouse, unsigned short nTime, bool bAddKey)
{
	lcVector3 FrontVector(mPosition - mTargetPosition);
	lcMatrix44 Rotation = lcMatrix44FromAxisAngle(FrontVector, 2.0f * dx / (21 - mouse) * LC_DTOR);

	mUpVector = lcMul30(mUpVector, Rotation);

	if (!IsSimple())
		ChangeKey(nTime, bAddKey, mUpVector, LC_CK_UP);

	UpdatePosition(nTime);
}

void lcCamera::DoCenter(lcVector3& point, unsigned short nTime, bool bAddKey)
{
	lcAlign(mTargetPosition, mPosition, point);

	if (!IsSimple())
	{
		ChangeKey(nTime, bAddKey, mPosition, LC_CK_EYE);
		ChangeKey(nTime, bAddKey, mTargetPosition, LC_CK_TARGET);
	}

	UpdatePosition(nTime);
}

void lcCamera::SetViewpoint(LC_VIEWPOINT Viewpoint, unsigned short nTime, bool bAddKey)
{
	lcVector3 Positions[] =
	{
		lcVector3(  0.0f, -50.0f,   0.0f), // LC_VIEWPOINT_FRONT
		lcVector3(  0.0f,  50.0f,   0.0f), // LC_VIEWPOINT_BACK
		lcVector3(  0.0f,   0.0f,  50.0f), // LC_VIEWPOINT_TOP
		lcVector3(  0.0f,   0.0f, -50.0f), // LC_VIEWPOINT_BOTTOM
		lcVector3( 50.0f,   0.0f,   0.0f), // LC_VIEWPOINT_LEFT
		lcVector3(-50.0f,   0.0f,   0.0f), // LC_VIEWPOINT_RIGHT
		lcVector3(-15.0f, -15.0f,   7.5f)  // LC_VIEWPOINT_HOME
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

	if (!IsSimple())
	{
		ChangeKey(nTime, bAddKey, mPosition, LC_CK_EYE);
		ChangeKey(nTime, bAddKey, mTargetPosition, LC_CK_TARGET);
		ChangeKey(nTime, bAddKey, mUpVector, LC_CK_UP);
	}

	UpdatePosition(nTime);
}

void lcCamera::StartTiledRendering(int tw, int th, int iw, int ih, float fAspect)
{
	m_pTR = new TiledRender();
	m_pTR->TileSize(tw, th, 0);
	m_pTR->ImageSize(iw, ih);
	m_pTR->Perspective(m_fovy, fAspect, m_zNear, m_zFar);
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

void lcCamera::SetFocalPoint(const lcVector3& focus, unsigned short nTime, bool bAddKey)
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

	if (!IsSimple())
	{
		ChangeKey(nTime, bAddKey, mPosition, LC_CK_EYE);
		ChangeKey(nTime, bAddKey, mTargetPosition, LC_CK_TARGET);
	}

	UpdatePosition(nTime);
}
