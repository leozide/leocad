// Light object.

#include <stdlib.h>
#include <string.h>
#include "light.h"
#include "defines.h"
#include "globals.h"
#include "vector.h"
#include "matrix.h"

GLuint Light::m_nSphereList = 0;
GLuint Light::m_nTargetList = 0;

// =============================================================================
// Static functions

static LC_LIGHT_KEY* AddNode (LC_LIGHT_KEY *node, unsigned short nTime, unsigned char nType)
{
  LC_LIGHT_KEY* newnode = (LC_LIGHT_KEY*)malloc (sizeof (LC_LIGHT_KEY));

  if (node)
  {
    newnode->next = node->next;
    node->next = newnode;
  }
  else
    newnode->next = NULL;

  newnode->type = nType;
  newnode->time = nTime;
  newnode->param[0] = newnode->param[1] = newnode->param[2] = 0;

  return newnode;
}

// =============================================================================
// CameraTarget class

LightTarget::LightTarget (Light *pParent)
  : Object (LC_OBJECT_LIGHT_TARGET)
{
  m_pParent = pParent;
  /*
  strcpy (m_strName, pParent->GetName ());
  m_strName[LC_OBJECT_NAME_LEN-8] = '\0';
  strcat (m_strName, ".Target");
  */
}

LightTarget::~LightTarget ()
{
}

void LightTarget::MinIntersectDist (LC_CLICKLINE* pLine)
{
  double dist = BoundingBoxIntersectDist (pLine);

  if (dist < pLine->mindist)
  {
    pLine->mindist = dist;
    pLine->pClosest = this;
  }
}

// =============================================================================
// Light class

Light::Light (float px, float py, float pz)
  : Object (LC_OBJECT_LIGHT)
{
  LC_LIGHT_KEY *node;

  Initialize ();

  m_pAnimationKeys = node = AddNode (NULL, 1, LK_POSITION);
  node->param[0] = px;
  node->param[1] = py;
  node->param[2] = pz;
  node = AddNode (node, 1, LK_COLOR);
  node->param[0] = 1.0f;
  node->param[1] = 1.0f;
  node->param[2] = 1.0f;

  m_pInstructionKeys = node = AddNode (NULL, 1, LK_POSITION);
  node->param[0] = px;
  node->param[1] = py;
  node->param[2] = pz;
  node = AddNode (node, 1, LK_COLOR);
  node->param[0] = 1.0f;
  node->param[1] = 1.0f;
  node->param[2] = 1.0f;

  m_fPos[3] = 0.0f;
  UpdatePosition (1, false);
}

Light::Light (float px, float py, float pz, float tx, float ty, float tz)
  : Object (LC_OBJECT_LIGHT)
{
  LC_LIGHT_KEY *node;

  Initialize ();

  m_pAnimationKeys = node = AddNode (NULL, 1, LK_POSITION);
  node->param[0] = px;
  node->param[1] = py;
  node->param[2] = pz;
  node = AddNode (node, 1, LK_TARGET);
  node->param[0] = tx;
  node->param[1] = ty;
  node->param[2] = tz;
  node = AddNode (node, 1, LK_COLOR);
  node->param[0] = 1.0f;
  node->param[1] = 1.0f;
  node->param[2] = 1.0f;

  m_pInstructionKeys = node = AddNode (NULL, 1, LK_POSITION);
  node->param[0] = px;
  node->param[1] = py;
  node->param[2] = pz;
  node = AddNode (node, 1, LK_TARGET);
  node->param[0] = tx;
  node->param[1] = ty;
  node->param[2] = tz;
  node = AddNode (node, 1, LK_COLOR);
  node->param[0] = 1.0f;
  node->param[1] = 1.0f;
  node->param[2] = 1.0f;

  m_pTarget = new LightTarget (this);
  m_fPos[3] = 1.0f;

  UpdatePosition (1, false);
}

void Light::Initialize ()
{
  m_bEnabled = true;
  m_pNext = NULL;
  m_nState = 0;
  m_pAnimationKeys = NULL;
  m_pInstructionKeys = NULL;
  m_pTarget = NULL;
  m_nList = 0;
  memset (m_strName, 0, sizeof (m_strName));
  m_fColor[3] = 1.0f;
}

Light::~Light ()
{
  if (m_nList != 0)
    glDeleteLists (m_nList, 1);
  RemoveKeys ();
  delete m_pTarget;
}

void Light::RemoveKeys ()
{
  LC_LIGHT_KEY *node, *prev;

  for (node = m_pInstructionKeys; node;)
  {
    prev = node;
    node = node->next;
    free (prev);
  }

  for (node = m_pAnimationKeys; node;)
  {
    prev = node;
    node = node->next;
    free (prev);
  }
}

void Light::MinIntersectDist (LC_CLICKLINE* pLine)
{
  double dist;

  if (m_nState & LC_LIGHT_HIDDEN)
    return;

  dist = BoundingBoxIntersectDist (pLine);
  if (dist < pLine->mindist)
  {
    pLine->mindist = dist;
    pLine->pClosest = this;
  }

  if (m_pTarget != NULL)
    m_pTarget->MinIntersectDist (pLine);
}

void Light::ChangeKey (unsigned short nTime, bool bAnimation, bool bAddKey, float param[3], unsigned char nKeyType)
{
  LC_LIGHT_KEY *node, *poskey = NULL, *newpos = NULL;
  if (bAnimation)
    node = m_pAnimationKeys;
  else
    node = m_pInstructionKeys;

  while (node)
  {
    if ((node->time <= nTime) &&
	(node->type == nKeyType))
      poskey = node;

    node = node->next;
  }

  if (bAddKey)
  {
    if (poskey)
    {
      if (poskey->time != nTime)
	newpos = AddNode(poskey, nTime, nKeyType);
    }
    else
      newpos = AddNode(poskey, nTime, nKeyType);
  }

  if (newpos == NULL)
    newpos = poskey;

  newpos->param[0] = param[0];
  newpos->param[1] = param[1];
  newpos->param[2] = param[2];
}

void Light::Move (unsigned short nTime, bool bAnimation, bool bAddKey, float dx, float dy, float dz)
{
  if (IsEyeSelected())
  {
    m_fPos[0] += dx;
    m_fPos[1] += dy;
    m_fPos[2] += dz;

    ChangeKey (nTime, bAnimation, bAddKey, m_fPos, LK_POSITION);
  }

  if (IsTargetSelected())
  {
    m_fTarget[0] += dx;
    m_fTarget[1] += dy;
    m_fTarget[2] += dz;

    ChangeKey (nTime, bAnimation, bAddKey, m_fTarget, LK_TARGET);
  }
}

void Light::CalculatePosition (unsigned short nTime, bool bAnimation, float pos[3], float target[3], float color[3])
{
  LC_LIGHT_KEY *node, *pp = NULL, *np = NULL, *pt = NULL, *nt = NULL, *pc = NULL, *nc = NULL;
  if (bAnimation)
    node = m_pAnimationKeys;
  else
    node = m_pInstructionKeys;

  while (node && (!np || !nt || !nc))
  {
    if (node->time <= nTime)
    {
      switch (node->type)
      {
      case LK_POSITION: pp = node; break;
      case LK_TARGET: pt = node; break;
      case LK_COLOR: pc = node; break;
      }
    }
    else
    {
      switch (node->type)
      {
      case LK_POSITION: if (np == NULL) np = node; break;
      case LK_TARGET: if (nt == NULL) nt = node; break;
      case LK_COLOR: if (nc == NULL) nc = node; break;
      }
    }

    node = node->next;
  }

  // TODO: USE KEY IN/OUT WEIGHTS
  if (bAnimation && (np != NULL) && (pp->time != nTime))
  {
    float t = (float)(nTime - pp->time)/(np->time - pp->time);
    pos[0] = pp->param[0] + (np->param[0] - pp->param[0])*t;
    pos[1] = pp->param[1] + (np->param[1] - pp->param[1])*t;
    pos[2] = pp->param[2] + (np->param[2] - pp->param[2])*t;
  }
  else
    memcpy (pos, pp->param, sizeof(float[3]));

  if (m_pTarget != NULL)
  {
    if (bAnimation && (nt != NULL) && (pt->time != nTime))
    {
      float t = (float)(nTime - pt->time)/(nt->time - pt->time);
      target[0] = pt->param[0] + (nt->param[0] - pt->param[0])*t;
      target[1] = pt->param[1] + (nt->param[1] - pt->param[1])*t;
      target[2] = pt->param[2] + (nt->param[2] - pt->param[2])*t;
    }
    else
      memcpy (target, pt->param, sizeof(float[3]));
  }

  if (bAnimation && (nc != NULL) && (pc->time != nTime))
  {
    float t = (float)(nTime - pc->time)/(nc->time - pc->time);
    color[0] = pc->param[0] + (nc->param[0] - pc->param[0])*t;
    color[1] = pc->param[1] + (nc->param[1] - pc->param[1])*t;
    color[2] = pc->param[2] + (nc->param[2] - pc->param[2])*t;
  }
  else
    memcpy (color, pc->param, sizeof(float[3]));
}

void Light::UpdatePosition (unsigned short nTime, bool bAnimation)
{
  CalculatePosition (nTime, bAnimation, m_fPos, m_fTarget, m_fColor);
  BoundingBoxCalculate (m_fPos);

  if (m_pTarget != NULL)
  {
    m_pTarget->BoundingBoxCalculate (m_fTarget);

    if (m_nList == 0)
      m_nList = glGenLists(1);

    glNewList (m_nList, GL_COMPILE);

    glPushMatrix ();
    glTranslatef (m_fPos[0], m_fPos[1], m_fPos[2]);

    Vector frontvec (m_fTarget[0]-m_fPos[0], m_fTarget[1]-m_fPos[1], m_fTarget[2]-m_fPos[2]);
    float len = frontvec.Length (), up[3] = { 1, 1, 1 };

    if (fabs (frontvec.X ()) < fabs (frontvec.Y ()))
    {
      if (fabs (frontvec.X ()) < fabs (frontvec.Z ()))
	up[0] = -(up[1]*frontvec.Y () + up[2]*frontvec.Z ());
      else
	up[2] = -(up[0]*frontvec.X () + up[1]*frontvec.Y ());
    }
    else
    {
      if (fabs (frontvec.Y ()) < fabs (frontvec.Z ()))
	up[1] = -(up[0]*frontvec.X () + up[2]*frontvec.Z ());
      else
	up[2] = -(up[0]*frontvec.X () + up[1]*frontvec.Y ());
    }

    Matrix mat;
    mat.CreateLookat (m_fPos, m_fTarget, up);
    mat.Invert ();
    mat.SetTranslation (0, 0, 0);

    glMultMatrixf (mat.m);

    glEnableClientState (GL_VERTEX_ARRAY);
    float verts[16*3];
    for (int i = 0; i < 8; i++)
    {
      verts[i*6] = verts[i*6+3] = cos ((float)i/4 * PI) * 0.3f;
      verts[i*6+1] = verts[i*6+4] = sin ((float)i/4 * PI) * 0.3f;
      verts[i*6+2] = 0.3f;
      verts[i*6+5] = -0.3f;
    }
    glVertexPointer (3, GL_FLOAT, 0, verts);
    glDrawArrays (GL_LINES, 0, 16);
    glVertexPointer (3, GL_FLOAT, 6*sizeof(float), verts);
    glDrawArrays (GL_LINE_LOOP, 0, 8);
    glVertexPointer (3, GL_FLOAT, 6*sizeof(float), &verts[3]);
    glDrawArrays (GL_LINE_LOOP, 0, 8);

    glBegin (GL_LINE_LOOP);
    glVertex3f (-0.5f, -0.5f, -0.3f);
    glVertex3f ( 0.5f, -0.5f, -0.3f);
    glVertex3f ( 0.5f,  0.5f, -0.3f);
    glVertex3f (-0.5f,  0.5f, -0.3f);
    glEnd ();

    glTranslatef(0, 0, -len);
    glEndList();

    if (m_nTargetList == 0)
    {
      m_nTargetList = glGenLists (1);
      glNewList (m_nTargetList, GL_COMPILE);

      glEnableClientState (GL_VERTEX_ARRAY);
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
      glDrawArrays (GL_LINES, 0, 24);
      glPopMatrix ();
      glEndList ();
    }
  }
  else
  {
    if (m_nSphereList == 0)
      m_nSphereList = glGenLists (1);
    glNewList (m_nSphereList, GL_COMPILE);

    const float radius = 0.2f;
    const int slices = 6, stacks = 6;
    float rho, drho, theta, dtheta;
    float x, y, z;
    int i, j, imin, imax;
    drho = 3.1415926536f/(float)stacks;
    dtheta = 2.0f*3.1415926536f/(float)slices;

    // draw +Z end as a triangle fan
    glBegin (GL_TRIANGLE_FAN);
    glVertex3f (0.0, 0.0, radius);
    for (j = 0; j <= slices; j++) 
    {
      theta = (j == slices) ? 0.0f : j * dtheta;
      x = (float)(-sin(theta) * sin(drho));
      y = (float)(cos(theta) * sin(drho));
      z = (float)(cos(drho));
      glVertex3f (x*radius, y*radius, z*radius);
    }
    glEnd ();

    imin = 1;
    imax = stacks-1;

    for (i = imin; i < imax; i++)
    {
      rho = i * drho;
      glBegin (GL_QUAD_STRIP);
      for (j = 0; j <= slices; j++)
      {
	theta = (j == slices) ? 0.0f : j * dtheta;
	x = (float)(-sin(theta) * sin(rho));
	y = (float)(cos(theta) * sin(rho));
	z = (float)(cos(rho));
	glVertex3f (x*radius, y*radius, z*radius);
	x = (float)(-sin(theta) * sin(rho+drho));
	y = (float)(cos(theta) * sin(rho+drho));
	z = (float)(cos(rho+drho));
	glVertex3f (x*radius, y*radius, z*radius);
      }
      glEnd ();
    }

    // draw -Z end as a triangle fan
    glBegin (GL_TRIANGLE_FAN);
    glVertex3f(0.0, 0.0, -radius);
    rho = 3.1415926536f - drho;
    for (j = slices; j >= 0; j--)
    {
      theta = (j==slices) ? 0.0f : j * dtheta;
      x = (float)(-sin(theta) * sin(rho));
      y = (float)(cos(theta) * sin(rho));
      z = (float)(cos(rho));
      glVertex3f (x*radius, y*radius, z*radius);
    }
    glEnd ();

    glEndList ();
  }
}

void Light::Render (float fLineWidth)
{
  if (m_pTarget != NULL)
  {
    if (IsEyeSelected())
    {
      glLineWidth(fLineWidth*2);
      glColor3ubv(FlatColorArray[(m_nState & LC_LIGHT_FOCUSED) != 0 ? LC_COL_FOCUSED : LC_COL_SELECTED]);
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
      glColor3ubv(FlatColorArray[(m_nState & LC_LIGHT_TARGET_FOCUSED) != 0 ? LC_COL_FOCUSED : LC_COL_SELECTED]);
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
    glVertex3fv(m_fPos);
    glVertex3fv(m_fTarget);
    glEnd();

    if (IsSelected())
    {
      Matrix projection, modelview;
      Vector frontvec(m_fTarget[0]-m_fPos[0], m_fTarget[1]-m_fPos[1], m_fTarget[2]-m_fPos[2]);
      float len = frontvec.Length (), up[3] = { 1, 1, 1 };

      if (fabs (frontvec.X ()) < fabs (frontvec.Y ()))
      {
	if (fabs (frontvec.X ()) < fabs (frontvec.Z ()))
	  up[0] = -(up[1]*frontvec.Y () + up[2]*frontvec.Z ());
	else
	  up[2] = -(up[0]*frontvec.X () + up[1]*frontvec.Y ());
      }
      else
      {
	if (fabs (frontvec.Y ()) < fabs (frontvec.Z ()))
	  up[1] = -(up[0]*frontvec.X () + up[2]*frontvec.Z ());
	else
	  up[2] = -(up[0]*frontvec.X () + up[1]*frontvec.Y ());
      }

      glPushMatrix ();

      modelview.CreateLookat (m_fPos, m_fTarget, up);
      modelview.Invert ();
      glMultMatrixf (modelview.m);

      projection.CreatePerspective (90.0f, 1.0f, 0.01, len);
      projection.Invert ();
      glMultMatrixf (projection.m);

      // draw the viewing frustum
      glBegin (GL_LINE_LOOP);
      glVertex3f ( 0.5f,  1.0f, 1.0f);
      glVertex3f ( 1.0f,  0.5f, 1.0f);
      glVertex3f ( 1.0f, -0.5f, 1.0f);
      glVertex3f ( 0.5f, -1.0f, 1.0f);
      glVertex3f (-0.5f, -1.0f, 1.0f);
      glVertex3f (-1.0f, -0.5f, 1.0f);
      glVertex3f (-1.0f,  0.5f, 1.0f);
      glVertex3f (-0.5f,  1.0f, 1.0f);
      glEnd ();

      glBegin (GL_LINES);
      glVertex3f (1, 1, -1);
      glVertex3f (0.75f, 0.75f, 1);
      glVertex3f (-1, 1, -1);
      glVertex3f (-0.75f, 0.75f, 1);
      glVertex3f (-1, -1, -1);
      glVertex3f (-0.75f, -0.75f, 1);
      glVertex3f (1, -1, -1);
      glVertex3f (0.75f, -0.75f, 1);
      glEnd ();

      glPopMatrix();
    }
  }
  else
  {
    glPushMatrix ();
    glTranslatef (m_fPos[0], m_fPos[1], m_fPos[2]);

    if (IsEyeSelected ())
    {
      glLineWidth (fLineWidth*2);
      glColor3ubv (FlatColorArray[(m_nState & LC_LIGHT_FOCUSED) != 0 ? LC_COL_FOCUSED : LC_COL_SELECTED]);
      glCallList (m_nSphereList);
      glLineWidth (fLineWidth);
    }
    else
    {
      glColor3f (0.5f, 0.8f, 0.5f);
      glCallList (m_nSphereList);
    }

    glPopMatrix ();
  }
}

void Light::Setup (int index)
{
  GLenum light = (GLenum)(GL_LIGHT0+index);

  if (!m_bEnabled)
  {
    glDisable (light);
    return;
  }

  glEnable (light);
  glLightfv (light, GL_POSITION, m_fPos);
  //  glLightfv (light, GL_AMBIENT, m_fColor);
  //  glLightfv (light, GL_DIFFUSE, m_fColor);
  //  glLightfv (light, GL_SPECULAR, m_fColor);

  float amb[]={0,0,0,1};
  float diff[]={0,0,1,1};
  float spec[]={1,1,1,1};
  glLightfv (light, GL_AMBIENT, amb);
  glLightfv (light, GL_DIFFUSE, diff);
  glLightfv (light, GL_SPECULAR, spec);

  glLightf (light, GL_CONSTANT_ATTENUATION, 1);
  glLightf (light, GL_LINEAR_ATTENUATION, 0);
  glLightf (light, GL_QUADRATIC_ATTENUATION, 0);

  if (m_pTarget != NULL)
  {
    Vector dir (m_fTarget[0]-m_fPos[0], m_fTarget[1]-m_fPos[1], m_fTarget[2]-m_fPos[2]);
    dir.Normalize ();

    glLightf (light, GL_SPOT_CUTOFF, 30.0f);
    glLightfv (light, GL_SPOT_DIRECTION, dir);
  }
}
