// Camera object.

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "opengl.h"
#include "globals.h"
#include "defines.h"
#include "vector.h"
#include "matrix.h"
#include "file.h"
#include "camera.h"
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

void CameraTarget::MinIntersectDist (LC_CLICKLINE* pLine)
{
  double dist = BoundingBoxIntersectDist (pLine);

  if (dist < pLine->mindist)
  {
    pLine->mindist = dist;
    pLine->pClosest = this;
  }
}

void CameraTarget::Select (bool bSelecting, bool bFocus, bool bMultiple)
{
  m_pParent->SelectTarget (bSelecting, bFocus, bMultiple);
}

/////////////////////////////////////////////////////////////////////////////
// Camera construction/destruction

Camera::Camera ()
  : Object (LC_OBJECT_CAMERA)
{
  Initialize();
}

// Start with a standard camera.
Camera::Camera (unsigned char nType, Camera* pPrev)
  : Object (LC_OBJECT_CAMERA)
{
  if (nType > 7)
    nType = 8;

  char names[8][7] = { "Front", "Back",  "Top",  "Under", "Left", "Right", "Main", "User" };
  float eyes[8][3] = { { 50,0,0 }, { -50,0,0 }, { 0,0,50 }, { 0,0,-50 },
		       { 0,50,0 }, { 0,-50,0 }, { 10,10,5}, { 0,5,0 } };
  float ups [8][3] = {  { 0,0,1 }, { 0,0,1 }, { 1,0,0 }, { -1,0,0 }, { 0,0,1 },
			{ 0,0,1 }, {-0.2357f, -0.2357f, 0.94281f }, { 0,0,1 } };

  Initialize();

  ChangeKey (1, false, true, eyes[nType], LC_CK_EYE);
  ChangeKey (1, false, true, ups[nType], LC_CK_UP);
  ChangeKey (1, true, true, eyes[nType], LC_CK_EYE);
  ChangeKey (1, true, true, ups[nType], LC_CK_UP);

  strcpy (m_strName, names[nType]);
  if (nType != 8)
    m_nState = LC_CAMERA_HIDDEN;
  m_nType = nType;

  if (pPrev)
    pPrev->m_pNext = this;

  UpdatePosition(1, false);
}

// From OnMouseMove(), case LC_ACTION_ROTATE_VIEW
Camera::Camera (const float *eye, const float *target, const float *up, Camera* pCamera)
  : Object (LC_OBJECT_CAMERA)
{
  // Fix the up vector
  Vector upvec(up), frontvec(eye[0]-target[0], eye[1]-target[1], eye[2]-target[2]), sidevec;
  frontvec.Normalize();
  sidevec.Cross(frontvec, upvec);
  upvec.Cross(sidevec, frontvec);
  upvec.Normalize();

  Initialize();

  ChangeKey (1, false, true, eye, LC_CK_EYE);
  ChangeKey (1, false, true, target, LC_CK_TARGET);
  ChangeKey (1, false, true, upvec, LC_CK_UP);
  ChangeKey (1, true, true, eye, LC_CK_EYE);
  ChangeKey (1, true, true, target, LC_CK_TARGET);
  ChangeKey (1, true, true, upvec, LC_CK_UP);

  int i, max = 0;

  for (;;)
  {
    if (strncmp (pCamera->m_strName, "Camera ", 7) == 0)
      if (sscanf(pCamera->m_strName, "Camera %d", &i) == 1)
	if (i > max) 
	  max = i;

    if (pCamera->m_pNext == NULL)
    {
      sprintf(m_strName, "Camera %d", max+1);
      pCamera->m_pNext = this;
      break;
    }
    else
      pCamera = pCamera->m_pNext;
  }

  UpdatePosition (1, false);
}

// From LC_ACTION_CAMERA
Camera::Camera (float ex, float ey, float ez, float tx, float ty, float tz, Camera* pCamera)
  : Object (LC_OBJECT_CAMERA)
{
  // Fix the up vector
  Vector upvec(0,0,1), frontvec(ex-tx, ey-ty, ez-tz), sidevec;
  frontvec.Normalize();
  if (frontvec == upvec)
    sidevec.FromFloat(1,0,0);
  else
    sidevec.Cross(frontvec, upvec);
  upvec.Cross(sidevec, frontvec);
  upvec.Normalize();

  Initialize();

  float eye[3] = { ex, ey, ez }, target[3] = { tx, ty, tz };

  ChangeKey (1, false, true, eye, LC_CK_EYE);
  ChangeKey (1, false, true, target, LC_CK_TARGET);
  ChangeKey (1, false, true, upvec, LC_CK_UP);
  ChangeKey (1, true, true, eye, LC_CK_EYE);
  ChangeKey (1, true, true, target, LC_CK_TARGET);
  ChangeKey (1, true, true, upvec, LC_CK_UP);

  int i, max = 0;

  if (pCamera)
    for (;;)
    {
      if (strncmp (pCamera->m_strName, "Camera ", 7) == 0)
	if (sscanf(pCamera->m_strName, "Camera %d", &i) == 1)
	  if (i > max) 
	    max = i;

      if (pCamera->m_pNext == NULL)
      {
	sprintf(m_strName, "Camera %d", max+1);
	pCamera->m_pNext = this;
	break;
      }
      else
	pCamera = pCamera->m_pNext;
    }

  UpdatePosition (1, false);
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
  m_zFar = 100;

  m_pNext = NULL;
  m_nState = 0;
  m_nList = 0;
  m_nType = LC_CAMERA_USER;

  m_pTR = NULL;
  for (unsigned char i = 0 ; i < sizeof(m_strName) ; i++ )
    m_strName[i] = 0;

  float *values[] = { m_fEye, m_fTarget, m_fUp };
  RegisterKeys (values, camera_key_info, LC_CK_COUNT);

  m_pTarget = new CameraTarget (this);
}

/////////////////////////////////////////////////////////////////////////////
// Camera save/load

bool Camera::FileLoad (File& file)
{
  unsigned char version, ch;

  file.ReadByte (&version, 1);

  if (version > LC_CAMERA_SAVE_VERSION)
    return false;

  if (version > 5)
    if (!Object::FileLoad (file))
      return false;

  if (version == 4)
  {
    file.Read(m_strName, 80);
    m_strName[80] = 0;
  }
  else
  {
    file.Read(&ch, 1);
    if (ch == 0xFF)
      return false; // don't read CString
    file.Read(m_strName, ch);
    m_strName[ch] = 0;
  }

  if (version < 3)
  {
    double d[3];
    float f[3];

    file.ReadDouble (d, 3);
    f[0] = (float)d[0];
    f[1] = (float)d[1];
    f[2] = (float)d[2];
    ChangeKey (1, false, true, f, LC_CK_EYE);
    ChangeKey (1, true, true, f, LC_CK_EYE);

    file.ReadDouble (d, 3);
    f[0] = (float)d[0];
    f[1] = (float)d[1];
    f[2] = (float)d[2];
    ChangeKey (1, false, true, f, LC_CK_TARGET);
    ChangeKey (1, true, true, f, LC_CK_TARGET);

    file.ReadDouble (d, 3);
    f[0] = (float)d[0];
    f[1] = (float)d[1];
    f[2] = (float)d[2];
    ChangeKey (1, false, true, f, LC_CK_UP);
    ChangeKey (1, true, true, f, LC_CK_UP);
  }

  if (version == 3)
  {
    file.Read(&ch, 1);

    while (ch--)
    {
      unsigned char step;
      double eye[3], target[3], up[3];
      float f[3];

      file.ReadDouble (eye, 3);
      file.ReadDouble (target, 3);
      file.ReadDouble (up, 3);
      file.ReadByte (&step, 1);

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

      int snapshot; // BOOL under Windows
      int cam;
      file.ReadLong (&snapshot, 1);
      file.ReadLong (&cam, 1);
//			if (cam == -1)
//				node->pCam = NULL;
//			else
//				node->pCam = pDoc->GetCamera(i);
    }
  }

  if (version < 4)
  {
    double d;
    file.ReadDouble (&d, 1); m_fovy = (float)d;
    file.ReadDouble (&d, 1); m_zFar = (float)d;
    file.ReadDouble (&d, 1); m_zNear= (float)d;
  }
  else
  {
    int n;

    if (version < 6)
    {
      unsigned short time;
      float param[4];
      unsigned char type;

      file.ReadLong (&n, 1);
      while (n--)
      {
        file.ReadShort (&time, 1);
        file.ReadFloat (param, 3);
        file.ReadByte (&type, 1);

        ChangeKey (time, false, true, param, type);
      }

      file.ReadLong (&n, 1);
      while (n--)
      {
        file.ReadShort (&time, 1);
        file.ReadFloat (param, 3);
        file.ReadByte (&type, 1);

        ChangeKey (time, true, true, param, type);
      }
    }

    file.ReadFloat (&m_fovy, 1);
    file.ReadFloat (&m_zFar, 1);
    file.ReadFloat (&m_zNear, 1);

    if (version < 5)
    {
      file.ReadLong (&n, 1);
      if (n != 0)
	m_nState |= LC_CAMERA_HIDDEN;
    }
    else
    {
      file.ReadByte (&m_nState, 1);
      file.ReadByte (&m_nType, 1);
    }
  }

  if ((version > 1) && (version < 4))
  {
    unsigned long show;
    int user;

    file.ReadLong (&show, 1);
//			if (version > 2)
    file.ReadLong (&user, 1);
    if (show == 0)
      m_nState |= LC_CAMERA_HIDDEN;
  }

  return true;
}

void Camera::FileSave (File& file) const
{
  unsigned char ch = LC_CAMERA_SAVE_VERSION;

  file.WriteByte (&ch, 1);

  Object::FileSave (file);

  ch = (unsigned char)strlen(m_strName);
  file.Write (&ch, 1);
  file.Write (m_strName, ch);

  file.WriteFloat (&m_fovy, 1);
  file.WriteFloat (&m_zFar, 1);
  file.WriteFloat (&m_zNear, 1);
  // version 5
  file.WriteByte (&m_nState, 1);
  file.WriteByte (&m_nType, 1);
}

/////////////////////////////////////////////////////////////////////////////
// Camera operations

void Camera::Move (unsigned short nTime, bool bAnimation, bool bAddKey, float dx, float dy, float dz)
{
  if (IsSide())
  {
    m_fEye[0] += dx;
    m_fEye[1] += dy;
    m_fEye[2] += dz;
    m_fTarget[0] += dx;
    m_fTarget[1] += dy;
    m_fTarget[2] += dz;

    ChangeKey(nTime, bAnimation, bAddKey, m_fEye, LC_CK_EYE);
    ChangeKey(nTime, bAnimation, bAddKey, m_fTarget, LC_CK_TARGET);
  }
  else
  {
    if (IsEyeSelected())
    {
      m_fEye[0] += dx;
      m_fEye[1] += dy;
      m_fEye[2] += dz;

      ChangeKey(nTime, bAnimation, bAddKey, m_fEye, LC_CK_EYE);
    }

    if (IsTargetSelected())
    {
      m_fTarget[0] += dx;
      m_fTarget[1] += dy;
      m_fTarget[2] += dz;

      ChangeKey(nTime, bAnimation, bAddKey, m_fTarget, LC_CK_TARGET);
    }

    // Fix the up vector
    Vector upvec(m_fUp), sidevec;
    Vector frontvec(m_fTarget[0]-m_fEye[0], m_fTarget[1]-m_fEye[1], m_fTarget[2]-m_fEye[2]);
    sidevec.Cross(frontvec, upvec);
    upvec.Cross(sidevec, frontvec);
    upvec.Normalize();
    upvec.ToFloat(m_fUp);

    ChangeKey(nTime, bAnimation, bAddKey, m_fUp, LC_CK_UP);
  }
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

void Camera::UpdatePosition (unsigned short nTime, bool bAnimation)
{
  CalculateKeys (nTime, bAnimation);

  // Fix the up vector
  Vector frontvec(m_fEye[0]-m_fTarget[0], m_fEye[1]-m_fTarget[1], m_fEye[2]-m_fTarget[2]);
  Vector upvec(m_fUp), sidevec;

  sidevec.Cross(frontvec, upvec);
  upvec.Cross(sidevec, frontvec);
  upvec.Normalize();
  upvec.ToFloat(m_fUp);

  float len = frontvec.Length();

  Matrix mat;
  mat.CreateLookat (m_fEye, m_fTarget, m_fUp);
  mat.Invert ();

  mat.SetTranslation (m_fEye[0], m_fEye[1], m_fEye[2]);
  BoundingBoxCalculate (&mat);
  mat.SetTranslation (m_fTarget[0], m_fTarget[1], m_fTarget[2]);
  m_pTarget->BoundingBoxCalculate (&mat);
  mat.SetTranslation (0, 0, 0);

  if (m_nList == 0)
    m_nList = glGenLists(1);

  glNewList(m_nList, GL_COMPILE);

  glPushMatrix();
  glTranslatef(m_fEye[0], m_fEye[1], m_fEye[2]);
  glMultMatrixf(mat.m);

  glEnableClientState(GL_VERTEX_ARRAY);
  float verts[34][3] = {
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
    {  0.3f,  0.3f, -0.6f }, { -0.3f,  0.3f, -0.6f } };
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
    float box[24][3] = { 
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
      {  0.2f, -0.2f,  0.2f }, {  0.2f, -0.2f, -0.2f } };
    glVertexPointer (3, GL_FLOAT, 0, box);
    glDrawArrays(GL_LINES, 0, 24);
    glPopMatrix();
    glEndList();
  }
}

void Camera::Render(float fLineWidth)
{
  if (IsEyeSelected())
  {
    glLineWidth(fLineWidth*2);
    glColor3ubv(FlatColorArray[(m_nState & LC_CAMERA_FOCUSED) != 0 ? LC_COL_FOCUSED : LC_COL_SELECTED]);
    glCallList(m_nList);
    glLineWidth(fLineWidth);
  }
  else
  {
    glColor3f(0.5f, 0.8f, 0.5f);
    glCallList(m_nList);
  }

  if (IsTargetSelected())
  {
    glLineWidth(fLineWidth*2);
    glColor3ubv(FlatColorArray[(m_nState & LC_CAMERA_TARGET_FOCUSED) != 0 ? LC_COL_FOCUSED : LC_COL_SELECTED]);
    glCallList(m_nTargetList);
    glLineWidth(fLineWidth);
  }
  else
  {
    glColor3f(0.5f, 0.8f, 0.5f);
    glCallList(m_nTargetList);
  }

  glColor3f(0.5f, 0.8f, 0.5f);
  glBegin(GL_LINES);
  glVertex3fv(m_fEye);
  glVertex3fv(m_fTarget);
  glEnd();

  if (IsSelected())
  {
    Matrix projection, modelview;
    Vector frontvec(m_fTarget[0]-m_fEye[0], m_fTarget[1]-m_fEye[1], m_fTarget[2]-m_fEye[2]);
    float len = frontvec.Length();

    glPushMatrix ();

    modelview.CreateLookat (m_fEye, m_fTarget, m_fUp);
    modelview.Invert ();
    glMultMatrixf (modelview.m);

    projection.CreatePerspective (m_fovy, 1.33f, 0.01f, len);
    projection.Invert ();
    glMultMatrixf (projection.m);

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

void Camera::MinIntersectDist(LC_CLICKLINE* pLine)
{
  double dist;

  if (m_nState & LC_CAMERA_HIDDEN)
    return;

  dist = BoundingBoxIntersectDist (pLine);
  if (dist < pLine->mindist)
  {
    pLine->mindist = dist;
    pLine->pClosest = this;
  }

  m_pTarget->MinIntersectDist (pLine);
}

void Camera::LoadProjection(float fAspect)
{
  if (m_pTR != NULL)
    m_pTR->BeginTile();
  else
  {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(m_fovy, fAspect, m_zNear, m_zFar);
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
  glLoadIdentity();
  gluLookAt(m_fEye[0], m_fEye[1], m_fEye[2], m_fTarget[0], m_fTarget[1], m_fTarget[2], m_fUp[0], m_fUp[1], m_fUp[2]);
}

void Camera::DoZoom(int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey)
{
  Vector frontvec(m_fEye[0]-m_fTarget[0], m_fEye[1]-m_fTarget[1], m_fEye[2]-m_fTarget[2]);
  frontvec.Normalize();
  frontvec *= 2.0f*dy/(21-mouse);

  // TODO: option to move eye, target or both
  m_fEye[0] += frontvec.X();
  m_fEye[1] += frontvec.Y();
  m_fEye[2] += frontvec.Z();
  m_fTarget[0] += frontvec.X();
  m_fTarget[1] += frontvec.Y();
  m_fTarget[2] += frontvec.Z();

  ChangeKey(nTime, bAnimation, bAddKey, m_fEye, LC_CK_EYE);
  ChangeKey(nTime, bAnimation, bAddKey, m_fTarget, LC_CK_TARGET);
  UpdatePosition(nTime, bAnimation);
}

void Camera::DoPan(int dx, int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey)
{
  Vector upvec(m_fUp), frontvec(m_fEye[0]-m_fTarget[0], m_fEye[1]-m_fTarget[1], m_fEye[2]-m_fTarget[2]), sidevec;
  sidevec.Cross(frontvec, upvec);
  sidevec.Normalize();
  sidevec *= 2.0f*dx/(21-mouse);
  upvec.Normalize();
  upvec *= -2.0f*dy/(21-mouse);

  m_fEye[0] += upvec.X() + sidevec.X();
  m_fEye[1] += upvec.Y() + sidevec.Y();
  m_fEye[2] += upvec.Z() + sidevec.Z();
  m_fTarget[0] += upvec.X() + sidevec.X();
  m_fTarget[1] += upvec.Y() + sidevec.Y();
  m_fTarget[2] += upvec.Z() + sidevec.Z();

  ChangeKey(nTime, bAnimation, bAddKey, m_fEye, LC_CK_EYE);
  ChangeKey(nTime, bAnimation, bAddKey, m_fTarget, LC_CK_TARGET);
  UpdatePosition(nTime, bAnimation);
}

void Camera::DoRotate(int dx, int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey, float* /*center*/)
{
  Vector upvec(m_fUp), frontvec(m_fEye[0]-m_fTarget[0], m_fEye[1]-m_fTarget[1], m_fEye[2]-m_fTarget[2]), sidevec;
  sidevec.Cross(frontvec, upvec);
  sidevec.Normalize();
  sidevec *= 2.0f*dx/(21-mouse);
  upvec.Normalize();
  upvec *= -2.0f*dy/(21-mouse);

  // TODO: option to move eye or target
  float len = frontvec.Length();
  frontvec.Add(upvec.X() + sidevec.X(), upvec.Y() + sidevec.Y(), upvec.Z() + sidevec.Z());
  frontvec.Normalize();
  frontvec *= len;
  frontvec.Add(m_fTarget);
  frontvec.ToFloat(m_fEye);

  // Calculate new up
  upvec.FromFloat(m_fUp);
  frontvec.FromFloat(m_fEye[0]-m_fTarget[0], m_fEye[1]-m_fTarget[1], m_fEye[2]-m_fTarget[2]);
  sidevec.Cross(frontvec, upvec);
  upvec.Cross(sidevec, frontvec);
  upvec.Normalize();
  upvec.ToFloat(m_fUp);

  ChangeKey(nTime, bAnimation, bAddKey, m_fEye, LC_CK_EYE);
  ChangeKey(nTime, bAnimation, bAddKey, m_fUp, LC_CK_UP);
  UpdatePosition(nTime, bAnimation);
}

void Camera::DoRoll(int dx, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey)
{
  Matrix mat;
  float front[3] = { m_fEye[0]-m_fTarget[0], m_fEye[1]-m_fTarget[1], m_fEye[2]-m_fTarget[2] };

  mat.FromAxisAngle(front, 2.0f*dx/(21-mouse));
  mat.TransformPoints(m_fUp, 1);

  ChangeKey(nTime, bAnimation, bAddKey, m_fUp, LC_CK_UP);
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
