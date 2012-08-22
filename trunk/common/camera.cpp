// Camera object.

#include "lc_global.h"
#include "lc_math.h"
#include "lc_colors.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "opengl.h"
#include "globals.h"
#include "lc_file.h"
#include "camera.h"
#include "view.h"
#include "tr.h"

#define LC_CAMERA_SAVE_VERSION 6 // LeoCAD 0.73

GLuint Camera::m_nTargetList = 0;

static LC_OBJECT_KEY_INFO camera_key_info[LC_CK_COUNT] =
{
  { "Camera Position", 3, LC_CK_EYE },
  { "Camera Target", 3, LC_CK_TARGET },
  { "Camera Up Vector", 3, LC_CK_UP }
};

// =============================================================================
// CameraTarget class

CameraTarget::CameraTarget (Camera *pParent)
  : Object (LC_OBJECT_CAMERA_TARGET)
{
  m_pParent = pParent;
  /*
  strcpy (m_strName, pParent->GetName ());
  m_strName[LC_OBJECT_NAME_LEN-8] = '\0';
  strcat (m_strName, ".Target");
  */
}

CameraTarget::~CameraTarget ()
{
}

void CameraTarget::MinIntersectDist(lcClickLine* ClickLine)
{
	lcVector3 Min = lcVector3(-0.2f, -0.2f, -0.2f);
	lcVector3 Max = lcVector3(0.2f, 0.2f, 0.2f);

	lcMatrix44 WorldView = ((Camera*)m_pParent)->mWorldView;
	WorldView.SetTranslation(lcMul30(-((Camera*)m_pParent)->mTargetPosition, WorldView));

	lcVector3 Start = lcMul31(ClickLine->Start, WorldView);
	lcVector3 End = lcMul31(ClickLine->End, WorldView);

	float Dist;
	if (lcBoundingBoxRayMinIntersectDistance(Min, Max, Start, End, &Dist, NULL) && (Dist < ClickLine->MinDist))
	{
		ClickLine->Closest = this;
		ClickLine->MinDist = Dist;
	}
}

void CameraTarget::Select(bool bSelecting, bool bFocus, bool bMultiple)
{
	m_pParent->SelectTarget(bSelecting, bFocus, bMultiple);
}

const char* CameraTarget::GetName() const
{
	return m_pParent->GetName();
}

/////////////////////////////////////////////////////////////////////////////
// Camera construction/destruction

Camera::Camera(bool Simple)
	: Object(LC_OBJECT_CAMERA)
{
	Initialize();

	if (Simple)
		m_nState |= LC_CAMERA_SIMPLE;
}

// Start with a standard camera.
Camera::Camera()
  : Object(LC_OBJECT_CAMERA)
{
	unsigned char nType = 7;

	mPosition = lcVector3(-10.0f, -10.0f, 5.0f);
	mTargetPosition = lcVector3(0.0f, 0.0f, 0.0f);
	mUpVector = lcVector3(-0.2357f, -0.2357f, 0.94281f);

	Initialize();

	ChangeKey(1, false, true, mPosition, LC_CK_EYE);
	ChangeKey(1, false, true, mTargetPosition, LC_CK_TARGET);
	ChangeKey(1, false, true, mUpVector, LC_CK_UP);
	ChangeKey(1, true, true, mPosition, LC_CK_EYE);
	ChangeKey(1, true, true, mTargetPosition, LC_CK_TARGET);
	ChangeKey(1, true, true, mUpVector, LC_CK_UP);

	if (nType != 8)
		m_nState = LC_CAMERA_HIDDEN;
	m_nType = nType;

	UpdatePosition(1, false);
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

Camera::~Camera()
{
  if (m_nList != 0)
    glDeleteLists (m_nList, 1);

  delete m_pTarget;
}

void Camera::Initialize()
{
  m_fovy = 30;
  m_zNear = 1;
  m_zFar = 500;

  m_nState = 0;
  m_nList = 0;
  m_nType = LC_CAMERA_USER;
  m_nList = 0;

  m_pTR = NULL;
  memset(m_strName, 0, sizeof(m_strName));

  float *values[] = { mPosition, mTargetPosition, mUpVector };
  RegisterKeys (values, camera_key_info, LC_CK_COUNT);

  m_pTarget = new CameraTarget (this);
}

void Camera::CreateName(const PtrArray<Camera>& Cameras)
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

bool Camera::FileLoad(lcFile& file)
{
  lcuint8 version, ch;

  version = file.ReadU8();

  if (version > LC_CAMERA_SAVE_VERSION)
    return false;

  if (version > 5)
    if (!Object::FileLoad (file))
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
    ChangeKey (1, false, true, f, LC_CK_EYE);
    ChangeKey (1, true, true, f, LC_CK_EYE);

    file.ReadDoubles(d, 3);
    f[0] = (float)d[0];
    f[1] = (float)d[1];
    f[2] = (float)d[2];
    ChangeKey (1, false, true, f, LC_CK_TARGET);
    ChangeKey (1, true, true, f, LC_CK_TARGET);

    file.ReadDoubles(d, 3);
    f[0] = (float)d[0];
    f[1] = (float)d[1];
    f[2] = (float)d[2];
    ChangeKey (1, false, true, f, LC_CK_UP);
    ChangeKey (1, true, true, f, LC_CK_UP);
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
      ChangeKey (step, false, true, f, LC_CK_EYE);
      ChangeKey (step, true, true, f, LC_CK_EYE);

      f[0] = (float)target[0];
      f[1] = (float)target[1];
      f[2] = (float)target[2];
      ChangeKey (step, false, true, f, LC_CK_TARGET);
      ChangeKey (step, true, true, f, LC_CK_TARGET);

      f[0] = (float)up[0];
      f[1] = (float)up[1];
      f[2] = (float)up[2];
      ChangeKey (step, false, true, f, LC_CK_UP);
      ChangeKey (step, true, true, f, LC_CK_UP);

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

        ChangeKey (time, false, true, param, type);
      }

      n = file.ReadS32();
      while (n--)
      {
        file.ReadU16(&time, 1);
        file.ReadFloats(param, 3);
        file.ReadU8(&type, 1);

        ChangeKey (time, true, true, param, type);
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

    show = file.ReadU32();
//			if (version > 2)
    user = file.ReadS32();
    if (show == 0)
      m_nState |= LC_CAMERA_HIDDEN;
  }

  return true;
}

void Camera::FileSave(lcFile& file) const
{
  file.WriteU8(LC_CAMERA_SAVE_VERSION);

  Object::FileSave (file);

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
	lcVector3 Move(dx, dy, dz);

	if (IsEyeSelected())
	{
		mPosition += Move;

		if (!IsSimple())
			ChangeKey(nTime, bAnimation, bAddKey, mPosition, LC_CK_EYE);
	}

	if (IsTargetSelected())
	{
		mTargetPosition += Move;

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

void Camera::Select (bool bSelecting, bool bFocus, bool bMultiple)
{
  if (bSelecting == true)
  {
    if (bFocus == true)
    {
      m_nState |= (LC_CAMERA_FOCUSED|LC_CAMERA_SELECTED);

      m_pTarget->Select (false, true, bMultiple);
    }
    else
      m_nState |= LC_CAMERA_SELECTED;

    if (bMultiple == false)
      m_pTarget->Select (false, false, bMultiple);
  }
  else
  {
    if (bFocus == true)
      m_nState &= ~(LC_CAMERA_FOCUSED);
    else
      m_nState &= ~(LC_CAMERA_SELECTED|LC_CAMERA_FOCUSED);
  } 
}

void Camera::SelectTarget (bool bSelecting, bool bFocus, bool bMultiple)
{
  // FIXME: the target should handle this

  if (bSelecting == true)
  {
    if (bFocus == true)
    {
      m_nState |= (LC_CAMERA_TARGET_FOCUSED|LC_CAMERA_TARGET_SELECTED);

      Select (false, true, bMultiple);
    }
    else
      m_nState |= LC_CAMERA_TARGET_SELECTED;

    if (bMultiple == false)
      Select (false, false, bMultiple);
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

	UpdateBoundingBox();
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

	UpdateBoundingBox();
}

void Camera::UpdateBoundingBox()
{
	// Fix the up vector
	lcVector3 FrontVector(mPosition - mTargetPosition);
	float len = FrontVector.Length();

	lcMatrix44 Mat = lcMatrix44AffineInverse(mWorldView);
	Mat.SetTranslation(lcVector3(0, 0, 0));

	if (!m_nList)
		return;

	glNewList(m_nList, GL_COMPILE);

	glPushMatrix();
	glTranslatef(mPosition[0], mPosition[1], mPosition[2]);
	glMultMatrixf(Mat);

	glEnableClientState(GL_VERTEX_ARRAY);
	float verts[34][3] =
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
		{  0.3f,  0.3f, -0.6f }, { -0.3f,  0.3f, -0.6f }
	};

	glVertexPointer (3, GL_FLOAT, 0, verts);
	glDrawArrays(GL_LINES, 0, 24);
	glDrawArrays(GL_LINE_STRIP, 24, 10);

//	glBegin(GL_LINES);
//	glVertex3f(0,0,0);
//	glVertex3f(0,0,len);
//	glEnd();

	glTranslatef(0, 0, -len);

	glEndList();

	if (m_nTargetList == 0)
	{
		m_nTargetList = glGenLists(1);
		glNewList (m_nTargetList, GL_COMPILE);

		glEnableClientState(GL_VERTEX_ARRAY);

		float box[24][3] =
		{
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
			{  0.2f, -0.2f,  0.2f }, {  0.2f, -0.2f, -0.2f }
		};

		glVertexPointer (3, GL_FLOAT, 0, box);
		glDrawArrays(GL_LINES, 0, 24);
		glPopMatrix();
		glEndList();
	}
}

void Camera::Render(float fLineWidth)
{
	// Create the display lists if this is the first time we're rendered.
	if (!m_nList)
	{
		m_nList = glGenLists(1);
		UpdateBoundingBox();
	}

  if (IsEyeSelected())
  {
    glLineWidth(fLineWidth*2);
    if (m_nState & LC_CAMERA_FOCUSED)
      lcSetColorFocused();
    else
      lcSetColorSelected();
    glCallList(m_nList);
    glLineWidth(fLineWidth);
  }
  else
  {
    lcSetColorCamera();
    glCallList(m_nList);
  }

  if (IsTargetSelected())
  {
    glLineWidth(fLineWidth*2);
    if (m_nState & LC_CAMERA_TARGET_FOCUSED)
      lcSetColorFocused();
    else
      lcSetColorSelected();
    glCallList(m_nTargetList);
    glLineWidth(fLineWidth);
  }
  else
  {
    lcSetColorCamera();
    glCallList(m_nTargetList);
  }

  lcSetColorCamera();
  glBegin(GL_LINES);
  glVertex3fv(mPosition);
  glVertex3fv(mTargetPosition);
  glEnd();

  if (IsSelected())
  {
    lcVector3 FrontVector(mTargetPosition - mPosition);
    float len = FrontVector.Length();

    glPushMatrix ();

	lcMatrix44 ViewWorld = lcMatrix44AffineInverse(mWorldView);
    glMultMatrixf(ViewWorld);

	lcMatrix44 InvProjection = lcMatrix44Inverse(lcMatrix44Perspective(m_fovy, 1.33f, 0.01f, len));
    glMultMatrixf(InvProjection);

    // draw the viewing frustum
    glBegin(GL_LINE_LOOP);
    glVertex3i(1, 1, 1);
    glVertex3i(-1, 1, 1);
    glVertex3i(-1, -1, 1);
    glVertex3i(1, -1, 1);
    glEnd();

    glBegin(GL_LINES);
    glVertex3i(1, 1, -1);
    glVertex3i(1, 1, 1);
    glVertex3i(-1, 1, -1);
    glVertex3i(-1, 1, 1);
    glVertex3i(-1, -1, -1);
    glVertex3i(-1, -1, 1);
    glVertex3i(1, -1, -1);
    glVertex3i(1, -1, 1);
    glEnd();

    glPopMatrix();
  }
}

void Camera::MinIntersectDist(lcClickLine* ClickLine)
{
	lcVector3 Min = lcVector3(-0.3f, -0.3f, -0.3f);
	lcVector3 Max = lcVector3(0.3f, 0.3f, 0.3f);

	lcVector3 Start = lcMul31(ClickLine->Start, mWorldView);
	lcVector3 End = lcMul31(ClickLine->End, mWorldView);

	float Dist;
	if (lcBoundingBoxRayMinIntersectDistance(Min, Max, Start, End, &Dist, NULL) && (Dist < ClickLine->MinDist))
	{
		ClickLine->Closest = this;
		ClickLine->MinDist = Dist;
	}

	m_pTarget->MinIntersectDist(ClickLine);
}

void Camera::LoadProjection(float fAspect)
{
  if (m_pTR != NULL)
    m_pTR->BeginTile();
  else
  {
    glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(lcMatrix44Perspective(m_fovy, fAspect, m_zNear, m_zFar));
/*
		ymax = 10;//(m_zFar-m_zNear)*tan(DTOR*m_fovy)/3;
		ymin = -ymax;
		xmin = ymin * fAspect;
		xmax = ymax * fAspect;
		znear = -60;
		zfar = 60;
		glOrtho(xmin, xmax, ymin, ymax, znear, zfar);
*/
  }

  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(mWorldView);
}

void Camera::ZoomExtents(View* view, const lcVector3& Center, const lcVector3* Points, int NumPoints, unsigned short nTime, bool bAnimation, bool bAddKey)
{
	int Viewport[4] = { 0, 0, view->GetWidth(), view->GetHeight() };

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

void Camera::DoZoom(int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey)
{
	lcVector3 FrontVector(mPosition - mTargetPosition);
	FrontVector.Normalize();
	FrontVector *= 2.0f * dy / (21 - mouse);

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

	lcVector3 Move = (SideVector * (2.0f * dx / (21 - mouse))) + (mUpVector * (-2.0f * dy / (21 - mouse)));
	mPosition += Move;
	mTargetPosition += Move;

	if (!IsSimple())
	{
		ChangeKey(nTime, bAnimation, bAddKey, mPosition, LC_CK_EYE);
		ChangeKey(nTime, bAnimation, bAddKey, mTargetPosition, LC_CK_TARGET);
	}

	UpdatePosition(nTime, bAnimation);
}

void Camera::DoRotate(int dx, int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey, float* /*center*/)
{
	lcVector3 FrontVector(mPosition - mTargetPosition);
	lcVector3 SideVector = lcNormalize(lcCross(FrontVector, mUpVector));

	// TODO: option to move eye or target
	float len = FrontVector.Length();
	FrontVector += (SideVector * (2.0f * dx / (21 - mouse))) + (mUpVector * (-2.0f * dy / (21 - mouse)));
	FrontVector.Normalize();
	mPosition = (FrontVector * len) + mTargetPosition;

	// Calculate new up
	FrontVector = mPosition - mTargetPosition;
	SideVector = lcCross(FrontVector, mUpVector);
	mUpVector = lcNormalize(lcCross(SideVector, FrontVector));

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
		lcVector3(50,0,0), lcVector3(-50,0,0), lcVector3(0,0,50), lcVector3(0,0,-50),
		lcVector3(0,50,0), lcVector3(0,-50,0), lcVector3(-10,-10,5)
	};

	lcVector3 Ups[] = { lcVector3(0,0,1), lcVector3(0,0,1), lcVector3(1,0,0), lcVector3(-1,0,0), lcVector3(0,0,1), lcVector3(0,0,1), lcVector3(-0.2357f, -0.2357f, 0.94281f) };

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
