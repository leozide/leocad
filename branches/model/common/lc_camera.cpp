#include "lc_global.h"
#include "lc_camera.h"
#include "lc_mesh.h"
#include "lc_colors.h"
#include "lc_application.h"
#include "project.h"

lcCamera::lcCamera(bool Simple) :
	lcObject(LC_OBJECT_TYPE_CAMERA)
{
	mFOV = 30.0f;
	mNear = 1.0f;
	mFar = 500.0f;

	mState = Simple ? LC_CAMERA_SIMPLE : 0;
	mName[0] = 0;

	mMesh = NULL;
}

lcCamera::~lcCamera()
{
}

void lcCamera::Update()
{
	mWorldView = lcMatrix44LookAt(mPosition, mTargetPosition, mUpVector);
}

void lcCamera::ClosestHitTest(lcObjectHitTest& HitTest)
{
	lcVector3 Min = lcVector3(-0.3f, -0.3f, -0.3f);
	lcVector3 Max = lcVector3(0.3f, 0.3f, 0.3f);

	lcVector3 Start = lcMul31(HitTest.Start, mWorldView);
	lcVector3 End = lcMul31(HitTest.End, mWorldView);

	float Distance;
	if (lcBoundingBoxRayMinIntersectDistance(Min, Max, Start, End, &Distance, NULL) && (Distance < HitTest.Distance))
	{
		HitTest.ObjectSection.Object = this;
		HitTest.ObjectSection.Section = LC_CAMERA_POSITION;
		HitTest.Distance = Distance;
	}

	Min = lcVector3(-0.2f, -0.2f, -0.2f);
	Max = lcVector3(0.2f, 0.2f, 0.2f);

	lcMatrix44 WorldView = mWorldView;
	WorldView.SetTranslation(lcMul30(-mTargetPosition, WorldView));

	Start = lcMul31(HitTest.Start, WorldView);
	End = lcMul31(HitTest.End, WorldView);

	if (lcBoundingBoxRayMinIntersectDistance(Min, Max, Start, End, &Distance, NULL) && (Distance < HitTest.Distance))
	{
		HitTest.ObjectSection.Object = this;
		HitTest.ObjectSection.Section = LC_CAMERA_TARGET;
		HitTest.Distance = Distance;
	}

	lcMatrix44 ViewWorld = lcMatrix44AffineInverse(mWorldView);
	lcVector3 UpVectorPosition = lcMul31(lcVector3(0, 1, 0), ViewWorld);

	WorldView = mWorldView;
	WorldView.SetTranslation(lcMul30(-UpVectorPosition, WorldView));

	Start = lcMul31(HitTest.Start, WorldView);
	End = lcMul31(HitTest.End, WorldView);

	if (lcBoundingBoxRayMinIntersectDistance(Min, Max, Start, End, &Distance, NULL) && (Distance < HitTest.Distance))
	{
		HitTest.ObjectSection.Object = this;
		HitTest.ObjectSection.Section = LC_CAMERA_UPVECTOR;
		HitTest.Distance = Distance;
	}
}

void lcCamera::BoxTest(const lcVector4* BoxPlanes, lcArray<lcObjectSection>& ObjectSections)
{
	lcVector3 Min(-0.3f, -0.3f, -0.3f);
	lcVector3 Max(0.3f, 0.3f, 0.3f);

	lcVector4 LocalPlanes[6];

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		lcVector3 Normal = lcMul30(BoxPlanes[PlaneIdx], mWorldView);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, BoxPlanes[PlaneIdx][3] - lcDot3(mWorldView[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		lcObjectSection& ObjectSection = ObjectSections.Add();
		ObjectSection.Object = this;
		ObjectSection.Section = LC_CAMERA_POSITION;
	}

	Min = lcVector3(-0.2f, -0.2f, -0.2f);
	Max = lcVector3(0.2f, 0.2f, 0.2f);

	lcMatrix44 WorldView = mWorldView;
	WorldView.SetTranslation(lcMul30(-mTargetPosition, WorldView));

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		lcVector3 Normal = lcMul30(BoxPlanes[PlaneIdx], WorldView);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, BoxPlanes[PlaneIdx][3] - lcDot3(WorldView[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		lcObjectSection& ObjectSection = ObjectSections.Add();
		ObjectSection.Object = this;
		ObjectSection.Section = LC_CAMERA_TARGET;
	}

	lcMatrix44 ViewWorld = lcMatrix44AffineInverse(mWorldView);
	lcVector3 UpVectorPosition = lcMul31(lcVector3(0, 1, 0), ViewWorld);

	WorldView = mWorldView;
	WorldView.SetTranslation(lcMul30(-UpVectorPosition, WorldView));

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		lcVector3 Normal = lcMul30(BoxPlanes[PlaneIdx], WorldView);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, BoxPlanes[PlaneIdx][3] - lcDot3(WorldView[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		lcObjectSection& ObjectSection = ObjectSections.Add();
		ObjectSection.Object = this;
		ObjectSection.Section = LC_CAMERA_UPVECTOR;
	}
}

void lcCamera::RenderExtra(View* View) const
{
//		if ((Camera == View->mCamera) || !Camera->IsVisible())
//			continue;

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

	float LineWidth = lcGetActiveProject()->m_fLineWidth;

	// Camera.
	glPushMatrix();
	glMultMatrixf(ViewWorld);

	if (mState & LC_CAMERA_POSITION_SELECTED)
	{
		glLineWidth(2.0f * LineWidth);

		if (mState & LC_CAMERA_POSITION_FOCUSED)
			lcSetColorFocused();
		else
			lcSetColorSelected();
	}
	else
	{
		glLineWidth(LineWidth);
		lcSetColorCamera();
	}

	glVertexPointer(3, GL_FLOAT, 0, Verts);
	glDrawArrays(GL_LINES, 0, 24);
	glDrawArrays(GL_LINE_STRIP, 24, 10);

	glPopMatrix();

	// Target.
	glPushMatrix();
	lcMatrix44 TargetMat = ViewWorld;
	TargetMat.SetTranslation(mTargetPosition);
	glMultMatrixf(TargetMat);

	if (mState & LC_CAMERA_TARGET_SELECTED)
	{
		glLineWidth(2.0f * LineWidth);

		if (mState & LC_CAMERA_TARGET_FOCUSED)
			lcSetColorFocused();
		else
			lcSetColorSelected();
	}
	else
	{
		glLineWidth(LineWidth);
		lcSetColorCamera();
	}

	glDrawArrays(GL_LINES, 34, 24);
	glPopMatrix();

	// Up vector
	glPushMatrix();
	lcMatrix44 UpVectorMat = ViewWorld;
	UpVectorMat.SetTranslation(UpVectorPosition);
	glMultMatrixf(UpVectorMat);

	if (mState & LC_CAMERA_UPVECTOR_SELECTED)
	{
		glLineWidth(2.0f * LineWidth);

		if (mState & LC_CAMERA_UPVECTOR_FOCUSED)
			lcSetColorFocused();
		else
			lcSetColorSelected();
	}
	else
	{
		glLineWidth(LineWidth);
		lcSetColorCamera();
	}

	glDrawArrays(GL_LINES, 34, 24);
	glPopMatrix();

	// Lines.
	lcSetColorCamera();
	glLineWidth(1.0f);
	glDrawArrays(GL_LINES, 34 + 24, 4);

	// Frustum.
	if (mState & LC_CAMERA_SELECTION_MASK)
	{
		glPushMatrix();
		glMultMatrixf(ViewWorld);

		float Dist = lcLength(mTargetPosition - mPosition);
		lcMatrix44 Projection = lcMatrix44Perspective(mFOV, 1.33f, 0.01f, Dist);
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

		glPopMatrix();
	}
}

void lcModel::SaveCheckpoint(lcFile& File)
{
	File.WriteFloats(mPosition, 3);
	File.WriteFloats(mTargetPosition, 3);
	File.WriteFloats(mUpVector, 3);
}

void lcModel::LoadCheckpoint(lcFile& File)
{
}




/*
Camera::Camera(bool Simple)
	: Object(LC_OBJECT_CAMERA)
{
	Initialize();

	if (Simple)
		m_nState |= LC_CAMERA_SIMPLE;
	else
	{
		mPosition = lcVector3(-10.0f, -10.0f, 5.0f);
		mTargetPosition = lcVector3(0.0f, 0.0f, 0.0f);
		mUpVector = lcVector3(-0.2357f, -0.2357f, 0.94281f);

		ChangeKey(1, false, true, mPosition, LC_CK_EYE);
		ChangeKey(1, false, true, mTargetPosition, LC_CK_TARGET);
		ChangeKey(1, false, true, mUpVector, LC_CK_UP);
		ChangeKey(1, true, true, mPosition, LC_CK_EYE);
		ChangeKey(1, true, true, mTargetPosition, LC_CK_TARGET);
		ChangeKey(1, true, true, mUpVector, LC_CK_UP);

		UpdatePosition(1, false);
	}
}

Camera::Camera(float ex, float ey, float ez, float tx, float ty, float tz)
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

	ChangeKey(1, false, true, eye, LC_CK_EYE);
	ChangeKey(1, false, true, target, LC_CK_TARGET);
	ChangeKey(1, false, true, UpVector, LC_CK_UP);
	ChangeKey(1, true, true, eye, LC_CK_EYE);
	ChangeKey(1, true, true, target, LC_CK_TARGET);
	ChangeKey(1, true, true, UpVector, LC_CK_UP);

	UpdatePosition(1, false);
}

void Camera::Initialize()
{
	m_fovy = 30.0f;
	m_zNear = 1.0f;
	m_zFar = 500.0f;

	m_nState = 0;
	m_nType = LC_CAMERA_USER;

	m_pTR = NULL;
	memset(m_strName, 0, sizeof(m_strName));

	float *values[] = { mPosition, mTargetPosition, mUpVector };
	RegisterKeys(values, camera_key_info, LC_CK_COUNT);
}

/////////////////////////////////////////////////////////////////////////////
// Camera save/load

bool Camera::FileLoad(lcFile& file)
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
		ChangeKey(1, false, true, f, LC_CK_EYE);
		ChangeKey(1, true, true, f, LC_CK_EYE);

		file.ReadDoubles(d, 3);
		f[0] = (float)d[0];
		f[1] = (float)d[1];
		f[2] = (float)d[2];
		ChangeKey(1, false, true, f, LC_CK_TARGET);
		ChangeKey(1, true, true, f, LC_CK_TARGET);

		file.ReadDoubles(d, 3);
		f[0] = (float)d[0];
		f[1] = (float)d[1];
		f[2] = (float)d[2];
		ChangeKey(1, false, true, f, LC_CK_UP);
		ChangeKey(1, true, true, f, LC_CK_UP);
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
			ChangeKey(step, false, true, f, LC_CK_EYE);
			ChangeKey(step, true, true, f, LC_CK_EYE);

			f[0] = (float)target[0];
			f[1] = (float)target[1];
			f[2] = (float)target[2];
			ChangeKey(step, false, true, f, LC_CK_TARGET);
			ChangeKey(step, true, true, f, LC_CK_TARGET);

			f[0] = (float)up[0];
			f[1] = (float)up[1];
			f[2] = (float)up[2];
			ChangeKey(step, false, true, f, LC_CK_UP);
			ChangeKey(step, true, true, f, LC_CK_UP);

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

				ChangeKey(time, false, true, param, type);
			}

			n = file.ReadS32();
			while (n--)
			{
				file.ReadU16(&time, 1);
				file.ReadFloats(param, 3);
				file.ReadU8(&type, 1);

				ChangeKey(time, true, true, param, type);
			}
		}

		file.ReadFloats(&m_fovy, 1);
		file.ReadFloats(&m_zFar, 1);
		file.ReadFloats(&m_zNear, 1);

		if (version < 5)
		{
			n = file.ReadS32();
			if (n != 0)
				m_nState |= LC_CAMERA_HIDDEN;
		}
		else
		{
			m_nState = file.ReadU8();
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
			m_nState |= LC_CAMERA_HIDDEN;
	}

	return true;
}

void Camera::FileSave(lcFile& file) const
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
	file.WriteU8(m_nState);
	file.WriteU8(m_nType);
}

/////////////////////////////////////////////////////////////////////////////
// Camera operations

void Camera::Move(unsigned short nTime, bool bAnimation, bool bAddKey, float dx, float dy, float dz)
{
	lcVector3 MoveVec(dx, dy, dz);

	if (IsEyeSelected())
	{
		mPosition += MoveVec;

		if (!IsSimple())
			ChangeKey(nTime, bAnimation, bAddKey, mPosition, LC_CK_EYE);
	}

	if (IsTargetSelected())
	{
		mTargetPosition += MoveVec;

		if (!IsSimple())
			ChangeKey(nTime, bAnimation, bAddKey, mTargetPosition, LC_CK_TARGET);
	}

	// Fix the up vector
	lcVector3 FrontVector(mTargetPosition - mPosition);
	lcVector3 SideVector = lcCross(FrontVector, mUpVector);
	mUpVector = lcNormalize(lcCross(SideVector, FrontVector));

	if (!IsSimple())
		ChangeKey(nTime, bAnimation, bAddKey, mUpVector, LC_CK_UP);
}

void Camera::Select(bool bSelecting, bool bFocus, bool bMultiple)
{
	if (bSelecting == true)
	{
		if (bFocus == true)
		{
			m_nState |= (LC_CAMERA_FOCUSED|LC_CAMERA_SELECTED);

			m_pTarget->Select(false, true, bMultiple);
		}
		else
			m_nState |= LC_CAMERA_SELECTED;

		if (bMultiple == false)
			m_pTarget->Select(false, false, bMultiple);
	}
	else
	{
		if (bFocus == true)
			m_nState &= ~(LC_CAMERA_FOCUSED);
		else
			m_nState &= ~(LC_CAMERA_SELECTED|LC_CAMERA_FOCUSED);
	}
}

void Camera::SelectTarget(bool bSelecting, bool bFocus, bool bMultiple)
{
	// FIXME: the target should handle this

	if (bSelecting == true)
	{
		if (bFocus == true)
		{
			m_nState |= (LC_CAMERA_TARGET_FOCUSED|LC_CAMERA_TARGET_SELECTED);

			Select(false, true, bMultiple);
		}
		else
			m_nState |= LC_CAMERA_TARGET_SELECTED;

		if (bMultiple == false)
			Select(false, false, bMultiple);
	}
	else
	{
		if (bFocus == true)
			m_nState &= ~(LC_CAMERA_TARGET_FOCUSED);
		else
			m_nState &= ~(LC_CAMERA_TARGET_SELECTED|LC_CAMERA_TARGET_FOCUSED);
	}
}

void Camera::UpdatePosition(unsigned short nTime, bool bAnimation)
{
	if (!IsSimple())
		CalculateKeys(nTime, bAnimation);

	lcVector3 FrontVector(mPosition - mTargetPosition);
	lcVector3 SideVector = lcCross(FrontVector, mUpVector);
	mUpVector = lcNormalize(lcCross(SideVector, FrontVector));

	mWorldView = lcMatrix44LookAt(mPosition, mTargetPosition, mUpVector);
}

void Camera::CopyPosition(const Camera* camera)
{
	m_fovy = camera->m_fovy;
	m_zNear = camera->m_zNear;
	m_zFar = camera->m_zFar;

	mWorldView = camera->mWorldView;
	mPosition = camera->mPosition;
	mTargetPosition = camera->mTargetPosition;
	mUpVector = camera->mUpVector;
}

bool Camera::IntersectsVolume(const lcVector4 Planes[6]) const
{
}

void Camera::LoadProjection(float fAspect)
{
	if (m_pTR != NULL)
		m_pTR->BeginTile();
	else
	{
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(lcMatrix44Perspective(m_fovy, fAspect, m_zNear, m_zFar));
#if 0
		ymax = 10;//(m_zFar-m_zNear)*tan(DTOR*m_fovy)/3;
		ymin = -ymax;
		xmin = ymin * fAspect;
		xmax = ymax * fAspect;
		znear = -60;
		zfar = 60;
		glOrtho(xmin, xmax, ymin, ymax, znear, zfar);
#endif
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(mWorldView);
}

void Camera::ZoomExtents(View* view, const lcVector3& Center, const lcVector3* Points, int NumPoints, unsigned short nTime, bool bAnimation, bool bAddKey)
{
	int Viewport[4] = { 0, 0, view->mWidth, view->mHeight };

	float Aspect = (float)Viewport[2]/(float)Viewport[3];

	lcVector3 Position(mPosition + Center - mTargetPosition);

	lcMatrix44 Projection = lcMatrix44Perspective(m_fovy, Aspect, m_zNear, m_zFar);

	mPosition = lcZoomExtents(Position, mWorldView, Projection, Points, NumPoints);
	mTargetPosition = Center;

	if (!IsSimple())
	{
		ChangeKey(nTime, bAnimation, bAddKey, mPosition, LC_CK_EYE);
		ChangeKey(nTime, bAnimation, bAddKey, mTargetPosition, LC_CK_TARGET);
	}

	UpdatePosition(nTime, bAnimation);
}

void Camera::ZoomRegion(View* view, float Left, float Right, float Bottom, float Top, unsigned short nTime, bool bAnimation, bool bAddKey)
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
		ChangeKey(nTime, bAnimation, bAddKey, mPosition, LC_CK_EYE);
		ChangeKey(nTime, bAnimation, bAddKey, mTargetPosition, LC_CK_TARGET);
	}

	UpdatePosition(nTime, bAnimation);
}

void Camera::DoZoom(int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey)
{
	lcVector3 FrontVector(mPosition - mTargetPosition);
	FrontVector.Normalize();
	FrontVector *= -2.0f * dy / (21 - mouse);

	// TODO: option to move eye, target or both
	mPosition += FrontVector;
	mTargetPosition += FrontVector;

	if (!IsSimple())
	{
		ChangeKey(nTime, bAnimation, bAddKey, mPosition, LC_CK_EYE);
		ChangeKey(nTime, bAnimation, bAddKey, mTargetPosition, LC_CK_TARGET);
	}

	UpdatePosition(nTime, bAnimation);
}

void Camera::DoPan(int dx, int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey)
{
	lcVector3 FrontVector(mPosition - mTargetPosition);
	lcVector3 SideVector = lcNormalize(lcCross(FrontVector, mUpVector));

	lcVector3 MoveVec = (SideVector * (2.0f * dx / (21 - mouse))) + (mUpVector * (-2.0f * dy / (21 - mouse)));
	mPosition += MoveVec;
	mTargetPosition += MoveVec;

	if (!IsSimple())
	{
		ChangeKey(nTime, bAnimation, bAddKey, mPosition, LC_CK_EYE);
		ChangeKey(nTime, bAnimation, bAddKey, mTargetPosition, LC_CK_TARGET);
	}

	UpdatePosition(nTime, bAnimation);
}

void Camera::DoRotate(int dx, int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey, float* center)
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
	mUpVector = lcMul31(mUpVector, transform);

	if (!IsSimple())
	{
		ChangeKey(nTime, bAnimation, bAddKey, mPosition, LC_CK_EYE);
		ChangeKey(nTime, bAnimation, bAddKey, mUpVector, LC_CK_UP);
	}

	UpdatePosition(nTime, bAnimation);
}

void Camera::DoRoll(int dx, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey)
{
	lcVector3 FrontVector(mPosition - mTargetPosition);
	lcMatrix44 Rotation = lcMatrix44FromAxisAngle(FrontVector, 2.0f * dx / (21 - mouse) * LC_DTOR);

	mUpVector = lcMul30(mUpVector, Rotation);

	if (!IsSimple())
		ChangeKey(nTime, bAnimation, bAddKey, mUpVector, LC_CK_UP);

	UpdatePosition(nTime, bAnimation);
}

void Camera::SetViewpoint(LC_VIEWPOINT Viewpoint, unsigned short nTime, bool bAnimation, bool bAddKey)
{
	lcVector3 Positions[] =
	{
		lcVector3(  0.0f, -50.0f,   0.0f), // LC_VIEWPOINT_FRONT
		lcVector3(  0.0f,  50.0f,   0.0f), // LC_VIEWPOINT_BACK
		lcVector3(  0.0f,   0.0f,  50.0f), // LC_VIEWPOINT_TOP
		lcVector3(  0.0f,   0.0f, -50.0f), // LC_VIEWPOINT_BOTTOM
		lcVector3( 50.0f,   0.0f,   0.0f), // LC_VIEWPOINT_LEFT
		lcVector3(-50.0f,   0.0f,   0.0f), // LC_VIEWPOINT_RIGHT
		lcVector3(-10.0f, -10.0f,   5.0f)  // LC_VIEWPOINT_HOME
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
	mUpVector = Ups[Viewpoint];

	if (!IsSimple())
	{
		ChangeKey(nTime, bAnimation, bAddKey, mPosition, LC_CK_EYE);
		ChangeKey(nTime, bAnimation, bAddKey, mTargetPosition, LC_CK_TARGET);
		ChangeKey(nTime, bAnimation, bAddKey, mUpVector, LC_CK_UP);
	}

	UpdatePosition(nTime, bAnimation);
}

void Camera::StartTiledRendering(int tw, int th, int iw, int ih, float fAspect)
{
	m_pTR = new TiledRender();
	m_pTR->TileSize(tw, th, 0);
	m_pTR->ImageSize(iw, ih);
	m_pTR->Perspective(m_fovy, fAspect, m_zNear, m_zFar);
}

void Camera::GetTileInfo(int* row, int* col, int* width, int* height)
{
	if (m_pTR != NULL)
	{
		*row = m_pTR->m_Rows - m_pTR->m_CurrentRow - 1;
		*col = m_pTR->m_CurrentColumn;
		*width = m_pTR->m_CurrentTileWidth;
		*height = m_pTR->m_CurrentTileHeight;
	}
}

bool Camera::EndTile()
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
  */