// Curve class, used to represent all flexible objects
//

#include "lc_global.h"
#include "lc_math.h"
#include "lc_colors.h"
#include <stdlib.h>
#include <math.h>
#include "globals.h"
#include "curve.h"
#include "opengl.h"

#define LC_CURVE_SAVE_VERSION 1 // LeoCAD 0.73
#define LC_CURVE_POINT_SAVE_VERSION 1 // LeoCAD 0.73

GLuint CurvePoint::m_nArrowList = 0;
GLuint CurvePoint::m_nSphereList = 0;

static LC_OBJECT_KEY_INFO curve_point_key_info[LC_CURVE_POINT_KEY_COUNT] =
{
  { "Control Point Position", 3, LC_CURVE_POINT_KEY_POSITION },
  { "Control Point Direction 1", 3, LC_CURVE_POINT_KEY_DIRECTION1 },
  { "Control Point Direction 2", 3, LC_CURVE_POINT_KEY_DIRECTION2 },
  { "Control Point Angle", 1, LC_CURVE_POINT_KEY_ANGLE }
};

// =============================================================================
// CurvePoint class

CurvePoint::CurvePoint (Curve *pParent)
  : Object (LC_OBJECT_CURVE_POINT)
{
  m_pParent = pParent;

  /*
    FIXME
  strcpy (m_strName, pParent->GetName ());
  m_strName[LC_OBJECT_NAME_LEN-8] = '\0';
  strcat (m_strName, ".Target");
  */

  Initialize ();
}

CurvePoint::CurvePoint (Curve *pParent, const float *pos, const float *dir)
  : Object (LC_OBJECT_CURVE_POINT)
{
  m_pParent = pParent;

  /*
    FIXME
  strcpy (m_strName, pParent->GetName ());
  m_strName[LC_OBJECT_NAME_LEN-8] = '\0';
  strcat (m_strName, ".Target");
  */

  Initialize ();

  float angle[1] = { 0 };

  ChangeKey (1, false, true, pos, LC_CURVE_POINT_KEY_POSITION);
  ChangeKey (1, false, true, dir, LC_CURVE_POINT_KEY_DIRECTION1);
  ChangeKey (1, false, true, dir, LC_CURVE_POINT_KEY_DIRECTION2);
  ChangeKey (1, false, true, angle, LC_CURVE_POINT_KEY_ANGLE);
  ChangeKey (1, true, true, pos, LC_CURVE_POINT_KEY_POSITION);
  ChangeKey (1, true, true, dir, LC_CURVE_POINT_KEY_DIRECTION1);
  ChangeKey (1, true, true, dir, LC_CURVE_POINT_KEY_DIRECTION2);
  ChangeKey (1, true, true, angle, LC_CURVE_POINT_KEY_ANGLE);

  UpdatePosition (1, false);
}

void CurvePoint::Initialize ()
{
  if (m_nSphereList == 0)
  {
    m_nSphereList = glGenLists (1);
    glNewList (m_nSphereList, GL_COMPILE);

    float radius = 0.2f;
    int slices = 6, stacks = 6;
    float rho, drho, theta, dtheta;
    float x, y, z;
    int i, j, imin, imax;
    drho = 3.1415926536f/(float)stacks;
    dtheta = 2.0f*3.1415926536f/(float)slices;

    // draw +Z end as a triangle fan
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0, 0.0, radius);
    for (j = 0; j <= slices; j++) 
    {
      theta = (j == slices) ? 0.0f : j * dtheta;
      x = (float)(-sin(theta) * sin(drho));
      y = (float)(cos(theta) * sin(drho));
      z = (float)(cos(drho));
      glVertex3f(x*radius, y*radius, z*radius);
    }
    glEnd();

    imin = 1;
    imax = stacks-1;

    for (i = imin; i < imax; i++)
    {
      rho = i * drho;
      glBegin(GL_QUAD_STRIP);
      for (j = 0; j <= slices; j++)
      {
	theta = (j == slices) ? 0.0f : j * dtheta;
	x = (float)(-sin(theta) * sin(rho));
	y = (float)(cos(theta) * sin(rho));
	z = (float)(cos(rho));
	glVertex3f(x*radius, y*radius, z*radius);
	x = (float)(-sin(theta) * sin(rho+drho));
	y = (float)(cos(theta) * sin(rho+drho));
	z = (float)(cos(rho+drho));
	glVertex3f(x*radius, y*radius, z*radius);
      }
      glEnd();
    }

    // draw -Z end as a triangle fan
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0, 0.0, -radius);
    rho = 3.1415926536f - drho;
    for (j = slices; j >= 0; j--)
    {
      theta = (j==slices) ? 0.0f : j * dtheta;
      x = (float)(-sin(theta) * sin(rho));
      y = (float)(cos(theta) * sin(rho));
      z = (float)(cos(rho));
      glVertex3f(x*radius, y*radius, z*radius);
    }
    glEnd();
    glEndList();
  }

  m_nState = LC_CURVE_POINT_CONTINUOUS;

  float *values[] = { m_fPos, m_fDir1, m_fDir2, &m_fAngle };
  RegisterKeys (values, curve_point_key_info, LC_CURVE_POINT_KEY_COUNT);
}

CurvePoint::~CurvePoint ()
{
}

void CurvePoint::MinIntersectDist (LC_CLICKLINE* pLine)
{
  float dist = (float)BoundingBoxIntersectDist (pLine);

  if (dist < pLine->mindist)
  {
    pLine->mindist = dist;
    pLine->pClosest = this;

    m_nLastHit = 1;
  }

  m_nLastHit = 0;
  // FIXME: check arrows
}

void CurvePoint::UpdatePosition (unsigned short nTime, bool bAnimation)
{
  CalculateKeys (nTime, bAnimation);
}

bool CurvePoint::FileLoad(lcFile& file)
{
  // FIXME
  return true;
}

void CurvePoint::FileSave(lcFile& file) const
{
  // FIXME
}

void CurvePoint::Move (unsigned short nTime, bool bAnimation, bool bAddKey, float dx, float dy, float dz)
{
  /*
  if (m_nState & LC_CURVE_POINT_ARROW1_FOCUS)
  if (m_nState & LC_CURVE_POINT_ARROW2_FOCUS)
  if (m_nState & LC_CURVE_POINT_CONTINUOUS)
    ;
  */
}

void CurvePoint::Select (bool bSelecting, bool bFocus, bool bMultiple)
{
  // FIXME: select arrows, use m_nLastHit

  if (bSelecting == true)
  {
    m_nState |= LC_CURVE_POINT_SELECTED;

    if (bFocus == true)
    {
      m_nState |= LC_CURVE_POINT_FOCUSED;

      m_pParent->DeselectOtherPoints (this, bMultiple);
    }
  }
  else
  {
    if (bFocus == true)
      m_nState &= ~(LC_CURVE_POINT_SELECTED|LC_CURVE_POINT_FOCUSED);
    else
      m_nState &= ~(LC_CURVE_POINT_SELECTED);
  }
}

void CurvePoint::Render (LC_RENDER_INFO* pInfo)
{
  if (m_nState & LC_CURVE_POINT_FOCUSED)
    lcSetColorFocused();
  else if (m_nState & LC_CURVE_POINT_SELECTED)
    lcSetColorSelected();
  else
    lcSetColorCamera(); // FIXME: same as camera color

  glPushMatrix ();
  glTranslatef (m_fPos[0], m_fPos[1], m_fPos[2]);
  glCallList (m_nSphereList);

  // FIXME: create and use arrow display list
  //      if (m_pPoints[i].m_nFlags & LC_CURVE_POINT_FOCUSED)
      {
	glBegin (GL_LINES);
        glVertex3f ( m_fDir1[0]/5,  m_fDir1[1]/5,  m_fDir1[2]/5);
        glVertex3f (-m_fDir2[0]/5, -m_fDir2[1]/5, -m_fDir2[2]/5);
	glEnd ();
      }

  glPopMatrix ();
}

// =============================================================================
// Curve class

Curve::Curve ()
  : Object (LC_OBJECT_CURVE)
{
  Initialize ();
}

Curve::Curve (PieceInfo *pInfo, const float *pos, unsigned char color)
  : Object (LC_OBJECT_CURVE)
{
	/*
  Initialize ();

  // FIXME: set the curve type and length based on the PieceInfo
  m_fLength = 5;

  m_nCurveType = LC_CURVE_TYPE_HOSE;
  m_nColor = color;

  float dir[3] = { 0, 20, 0 }, pos2[3] = { pos[0] + m_fLength, pos[1], pos[2] };

  m_fUp0[0] = 0;
  m_fUp0[1] = 0;
  m_fUp0[2] = 1;

  CurvePoint *pt;
  pt = new CurvePoint (this, pos, dir);
  m_Points.Add (pt);

  dir[1] = 0;
  dir[2] = -5;

  pt = new CurvePoint (this, pos2, dir);
  m_Points.Add (pt);

  pos2[0] += 5;
  dir[2] = 5;

  pt = new CurvePoint (this, pos2, dir);
  m_Points.Add (pt);

  UpdatePosition (1, false);
*/
}

Curve::~Curve ()
{
	/*
  for (int i = 0; i < m_Points.GetSize (); i++)
    delete m_Points[i];
  glDeleteLists (m_nDisplayList, 1);
	*/
}

void Curve::Initialize ()
{
  m_nCurveType = (LC_CURVE_TYPE)0;
  m_nState = 0;
  m_nColor = 0;
  m_nDisplayList = glGenLists (1);
}

bool Curve::FileLoad(lcFile& file)
{
  // FIXME
  return true;
}

void Curve::FileSave(lcFile& file) const
{
  // FIXME
}

void Curve::MinIntersectDist (LC_CLICKLINE* pLine)
{
  // FIXME
}

void Curve::UpdatePosition (unsigned short nTime, bool bAnimation)
{
  for (int i = 0; i < m_Points.GetSize (); i++)
    m_Points[i]->UpdatePosition (nTime, bAnimation);

  glNewList (m_nDisplayList, GL_COMPILE);

  switch (m_nCurveType)
  {
  case LC_CURVE_TYPE_HOSE:
    TesselateHose ();
    break;
  }

  glEndList ();
}

void Curve::Move (unsigned short nTime, bool bAnimation, bool bAddKey, float dx, float dy, float dz)
{
  for (int i = 0; i < m_Points.GetSize (); i++)
    m_Points[i]->Move (nTime, bAnimation, bAddKey, dx, dy, dz);
}

void Curve::Select (bool bSelecting, bool bFocus, bool bMultiple)
{
  if (bSelecting == true)
  {
    if (bFocus == true)
    {
      m_nState |= (LC_CURVE_SELECTED|LC_CURVE_FOCUSED);

      for (int i = 0; i < m_Points.GetSize (); i++)
        m_Points[i]->Select (false, true, bMultiple);
    }
    else
      m_nState |= LC_CURVE_SELECTED;
  }
  else
  {
    if (bFocus == true)
      m_nState &= ~(LC_CURVE_SELECTED|LC_CURVE_FOCUSED);
    else
      m_nState &= ~(LC_CURVE_SELECTED);

    for (int i = 0; i < m_Points.GetSize (); i++)
      m_Points[i]->Select (false, bFocus, bMultiple);
  }
}

void Curve::DeselectOtherPoints (CurvePoint *pSender, bool bFocusOnly)
{
  CurvePoint *pt;

  for (int i = 0; i < m_Points.GetSize (); i++)
  {
    pt = m_Points[i];
    if (pt != pSender)
      pt->Select (false, bFocusOnly, true);
  }
}

/*
void Curve::TesselateHose ()
{
  float x, y, z, t, t2, t3, cx[4], cy[4], cz[4];
  const float *p1, *p2, *r1, *r2, *u1, *u2;

  glEnableClientState (GL_VERTEX_ARRAY);

  for (unsigned int i = 0; i < m_Points.GetSize () - 1; i++)
  {
    p1 = m_Points[i]->GetPosition ();
    p2 = m_Points[i+1]->GetPosition ();
    r1 = m_Points[i]->GetDirection1 ();
    r2 = m_Points[i+1]->GetDirection2 ();
    u1 = m_Points[i]->GetUpVector ();
    u2 = m_Points[i+1]->GetUpVector ();

    cx[0] = 2*p1[0] - 2*p2[0] + r1[0] + r2[0];
    cx[1] = -3*p1[0] + 3*p2[0] - 2*r1[0] - r2[0];
    cx[2] = r1[0];
    cx[3] = p1[0];

    cy[0] = 2*p1[1] - 2*p2[1] + r1[1] + r2[1];
    cy[1] = -3*p1[1] + 3*p2[1] - 2*r1[1] - r2[1];
    cy[2] = r1[1];
    cy[3] = p1[1];

    cz[0] = 2*p1[2] - 2*p2[2] + r1[2] + r2[2];
    cz[1] = -3*p1[2] + 3*p2[2] - 2*r1[2] - r2[2];
    cz[2] = r1[2];
    cz[3] = p1[2];

    int steps1 = 16, steps2 = 6, j, k;
    float* verts = (float*)malloc ((steps1+1) * steps2 * 3 * sizeof (float));
    float a, b, c;
    float ux, uy, uz;

    for (t = 0, j = 0; j <= steps1; j++, t += 1.0f/steps1)
    {
      t2 = t*t;
      t3 = t2*t;

      // position
      x = cx[0]*t3 + cx[1]*t2 + cx[2]*t + cx[3];
      y = cy[0]*t3 + cy[1]*t2 + cy[2]*t + cy[3];
      z = cz[0]*t3 + cz[1]*t2 + cz[2]*t + cz[3];

      // tangent
      a = 3*cx[0]*t2 + 2*cx[1]*t + cx[2];
      b = 3*cy[0]*t2 + 2*cy[1]*t + cy[2];
      c = 3*cz[0]*t2 + 2*cz[1]*t + cz[2];

      // gradient
      ux = 6*cx[0]*t + 2*cx[1];
      uy = 6*cy[0]*t + 2*cy[1];
      uz = 6*cz[0]*t + 2*cz[1];

      Vector side, front (a, b, c);
      Vector up (ux, uy, uz);
      side.Cross (front, up);
      up.Cross (side, front);
      up.Normalize ();
      front.Normalize ();
      side.Normalize ();

      float f[16];
#define M(row,col)  f[col*4+row]
      M(0,0) = side[0]; M(0,1) = up[0]; M(0,2) = front[0]; M(0,3) = x;
      M(1,0) = side[1]; M(1,1) = up[1]; M(1,2) = front[1]; M(1,3) = y;
      M(2,0) = side[2]; M(2,1) = up[2]; M(2,2) = front[2]; M(2,3) = z;
      M(3,0) = 0.0;     M(3,1) = 0.0;   M(3,2) = 0.0;      M(3,3) = 1.0;
#undef M

      float v[3];
      Matrix m;
      m.FromFloat (f);

      for (int k = 0; k < steps2; k++)
      {
	float *o = &verts[(j*steps2+k)*3];
	v[0] = cos (2.0 * M_PI * k / steps2) * 0.15f;
	v[1] = sin (2.0 * M_PI * k / steps2) * 0.15f;
	v[2] = 0;
	m.TransformPoint (o, v);
        glVertex3fv (o);
      }
    }

    GLuint *index = (GLuint*)malloc (2 * (steps2+1) * sizeof (GLuint));
    glVertexPointer (3, GL_FLOAT, 0, verts);
    for (j = 0; j < steps1; j++)
    {
      for (k = 0; k < steps2; k++)
      {
	index[k*2] = j*steps2+k;
	index[k*2+1] = (j+1)*steps2+k;
      }
      index[k*2] = index[0];
      index[k*2+1] = index[1];
      glDrawElements (GL_TRIANGLE_STRIP, 2*(steps2+1), GL_UNSIGNED_INT, index);
    }

    free (index);
    free (verts);
  }
}
*/

void Curve::TesselateHose ()
{
  float x, y, z, t, t2, t3, cx[4], cy[4], cz[4];
  const float *p1, *p2, *r1, *r2;

  float u[3] = { m_fUp0[0], m_fUp0[1], m_fUp0[2] };
  int steps1 = 16, steps2 = 6, j, k;
  float* verts = (float*)malloc ((steps1+1) * steps2 * 3 * sizeof (float));
  float a, b, c;

  glEnableClientState (GL_VERTEX_ARRAY);
  glVertexPointer (3, GL_FLOAT, 0, verts);

  for (int i = 0; i < m_Points.GetSize () - 1; i++)
  {
    float a1, a2, angle_step; // axial rotation

    p1 = m_Points[i]->GetPosition ();
    p2 = m_Points[i+1]->GetPosition ();
    r1 = m_Points[i]->GetDirection1 ();
    r2 = m_Points[i+1]->GetDirection2 ();
    a1 = m_Points[i]->GetAngle ();
    a2 = m_Points[i+1]->GetAngle ();

    angle_step = (a2 - a1) / steps1;
    if (fabs (angle_step) < 0.01f)
      angle_step = 0;

    cx[0] = 2*p1[0] - 2*p2[0] + r1[0] + r2[0];
    cx[1] = -3*p1[0] + 3*p2[0] - 2*r1[0] - r2[0];
    cx[2] = r1[0];
    cx[3] = p1[0];

    cy[0] = 2*p1[1] - 2*p2[1] + r1[1] + r2[1];
    cy[1] = -3*p1[1] + 3*p2[1] - 2*r1[1] - r2[1];
    cy[2] = r1[1];
    cy[3] = p1[1];

    cz[0] = 2*p1[2] - 2*p2[2] + r1[2] + r2[2];
    cz[1] = -3*p1[2] + 3*p2[2] - 2*r1[2] - r2[2];
    cz[2] = r1[2];
    cz[3] = p1[2];

    for (t = 0, j = 0; j <= steps1; j++, t += 1.0f/steps1)
    {
      t2 = t*t;
      t3 = t2*t;

      // position
      x = cx[0]*t3 + cx[1]*t2 + cx[2]*t + cx[3];
      y = cy[0]*t3 + cy[1]*t2 + cy[2]*t + cy[3];
      z = cz[0]*t3 + cz[1]*t2 + cz[2]*t + cz[3];

      // tangent
      a = 3*cx[0]*t2 + 2*cx[1]*t + cx[2];
      b = 3*cy[0]*t2 + 2*cy[1]*t + cy[2];
      c = 3*cz[0]*t2 + 2*cz[1]*t + cz[2];

      lcVector3 SideVector, FrontVector(a, b, c);
      lcVector3 UpVector(u[0], u[1], u[2]);
      SideVector = lcCross(FrontVector, UpVector);
      UpVector = lcCross(SideVector, FrontVector);
      UpVector.Normalize();
      FrontVector.Normalize();
      SideVector.Normalize();

      if (angle_step != 0)
      {
//        Matrix rot;
//        rot.FromAxisAngle(FrontVector, angle_step);
//        rot.TransformPoint(u, UpVector);
      }
//      else
//        UpVector.ToFloat(u);

      lcMatrix44 m(lcVector4(SideVector, 0.0f), lcVector4(UpVector, 0.0f), lcVector4(FrontVector, 0.0f), lcVector4(x, y, z, 1.0f));

      for (int k = 0; k < steps2; k++)
      {
			lcVector3 Pos((float)(cos (2.0 * M_PI * k / steps2) * 0.15), (float)(sin (2.0 * M_PI * k / steps2) * 0.15), 0.0f);
			Pos = lcMul31(Pos, m);
			verts[(j*steps2+k)*3+0] = Pos[0];
			verts[(j*steps2+k)*3+1] = Pos[1];
			verts[(j*steps2+k)*3+2] = Pos[2];
      }
    }

    GLuint *index = (GLuint*)malloc (2 * (steps2+1) * sizeof (GLuint));
    for (j = 0; j < steps1; j++)
    {
      for (k = 0; k < steps2; k++)
      {
	index[k*2] = j*steps2+k;
	index[k*2+1] = (j+1)*steps2+k;
      }
      index[k*2] = index[0];
      index[k*2+1] = index[1];
      glDrawElements (GL_TRIANGLE_STRIP, 2*(steps2+1), GL_UNSIGNED_INT, index);
    }

    free (index);
  }
  free (verts);
}

void Curve::Render (LC_RENDER_INFO* pInfo)
{
	if ((m_nState & LC_CURVE_HIDDEN) != 0)
		return;

	lcSetColor(m_nColor);

	if (lcIsColorTranslucent(m_nColor))
	{
		if (!pInfo->transparent)
		{
			pInfo->transparent = true;
			glEnable (GL_BLEND);
			glDepthMask (GL_FALSE);
			glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	}
	else
	{
		if (pInfo->transparent)
		{
			pInfo->transparent = false;
			glDepthMask (GL_TRUE);
			glDisable (GL_BLEND);
		}
	}

	//  glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	glCallList (m_nDisplayList);

	//  if (m_nState & LC_CURVE_SELECTED)
	{
		// turn off transparency to draw the control points
		if (pInfo->transparent)
		{
			pInfo->transparent = false;
			if (pInfo->transparent)
			{
				glDepthMask (GL_TRUE);
				glDisable (GL_BLEND);
			}
		}

		for (int i = 0; i < m_Points.GetSize (); i++)
			m_Points[i]->Render (pInfo);
	}

	/*
	if (IsSelected ())
	{

	for (int i = 0; i < m_nNumPoints; i++)
	{
	if (m_pPoints[i].m_nFlags & LC_CURVE_POINT_FOCUSED)
		lcSetColorFocused();
	else if (m_pPoints[i].m_nFlags & LC_CURVE_POINT_SELECTED)
		lcSetColorSelected();
	else
		lcSetColorCamera(); // FIXME: same as camera color

	glPushMatrix ();
	//    RenderSegment (m_pPoints[i].m_fPos, m_pPoints[i+1].m_fPos, m_pSegments[i].m_fR1, m_pSegments[i].m_fR2);
	glTranslatef (m_pPoints[i].m_fPos[0], m_pPoints[i].m_fPos[1], m_pPoints[i].m_fPos[2]);
	glCallList (m_nSphereList);

	if (m_pPoints[i].m_nFlags & LC_CURVE_POINT_FOCUSED)
	{
	glBegin (GL_LINES);
	if (i < m_nNumSegments)
	{
	glVertex3fv (m_pSegments[i].m_fR1);
	glVertex3f (0, 0, 0);
	}
	else if (i > 0)
	{
	glVertex3f (-m_pSegments[i-1].m_fR2[0], -m_pSegments[i-1].m_fR2[1], -m_pSegments[i-1].m_fR2[2]);
	glVertex3f (0, 0, 0);
	}
	glEnd ();
	}

	glPopMatrix ();
	}
	}
	*/


	/*
	if (m_nFlags & LC_CURVE_LOOP)
	{
	i = m_nNumPoints - 1;
	RenderSegment (m_pPoints[0].pos, m_pPoints[i].pos, m_pPoints[0].normal, m_pPoints[i].normal);
	}
	*/
}






#if 0

#define LC_CURVE_POINT_RADIUS 0.2f

// =============================================================================
// Static functions

#include <math.h>
/*
// TODO: optimize
static void RenderSegment (float p1[3], float p2[3], float r1[3], float r2[3])
{
  float x, y, z, t, t2, t3, cx[4], cy[4], cz[4];
  int i;

  cx[0] = 2*p1[0] - 2*p2[0] + r1[0] + r2[0];
  cx[1] = -3*p1[0] + 3*p2[0] - 2*r1[0] - r2[0];
  cx[2] = r1[0];
  cx[3] = p1[0];

  cy[0] = 2*p1[1] - 2*p2[1] + r1[1] + r2[1];
  cy[1] = -3*p1[1] + 3*p2[1] - 2*r1[1] - r2[1];
  cy[2] = r1[1];
  cy[3] = p1[1];

  cz[0] = 2*p1[2] - 2*p2[2] + r1[2] + r2[2];
  cz[1] = -3*p1[2] + 3*p2[2] - 2*r1[2] - r2[2];
  cz[2] = r1[2];
  cz[3] = p1[2];

  glColor3f (1,0,0);

  glBegin (GL_LINE_STRIP);
  glVertex3f (cx[3], cy[3], cz[3]);

  for (t = 0, i = 0; i < 10; i++)
  {
    t += 0.1f;
    t2 = t*t;
    t3 = t2*t;

    x = cx[0]*t3 + cx[1]*t2 + cx[2]*t + cx[3];
    y = cy[0]*t3 + cy[1]*t2 + cy[2]*t + cy[3];
    z = cz[0]*t3 + cz[1]*t2 + cz[2]*t + cz[3];
    glVertex3f (x, y, z);
  }

  glEnd ();


  glColor3f (0,0,0);

  float a, b, c;
  for (t = 0, i = 0; i <= 10; i++, t += 0.1f)
  {
    t2 = t*t;
    t3 = t2*t;

    x = cx[0]*t3 + cx[1]*t2 + cx[2]*t + cx[3];
    y = cy[0]*t3 + cy[1]*t2 + cy[2]*t + cy[3];
    z = cz[0]*t3 + cz[1]*t2 + cz[2]*t + cz[3];

    a = 3*cx[0]*t2 + 2*cx[1]*t + cx[2];
    b = 3*cy[0]*t2 + 2*cy[1]*t + cy[2];
    c = 3*cz[0]*t2 + 2*cz[1]*t + cz[2];

    Vector v1 (0, 0, 1);
    Vector v2 (a, b, c);
    Vector v3;
    v3.Cross (v1, v2);
    a = v1.Angle (v2);
    float v[3];
    v3.ToFloat (v);
    Matrix m;
    m.FromAxisAngle (v, a);

    glBegin (GL_LINE_LOOP);
    for (int j = 0; j < 16; j++)
    {
      float o[3];
      v[0] = cos (2.0 * M_PI * j / 16) * 0.15f;
      v[1] = sin (2.0 * M_PI * j / 16) * 0.15f;
      v[2] = 0;
      m.TransformPoint (o, v);
      glVertex3f (o[0]+x, o[1]+y, o[2]+z);
    }

    glEnd ();
  }

}
*/

// =============================================================================
// Curve class

void Curve::MinIntersectDist (LC_CLICKLINE* pLine)
{
  // FIXME: Curve segments and tangent arrows

  for (int i = 0; i < m_nNumPoints; i++)
  {
    double dist = pLine->PointDistance (m_pPoints[i].m_fPos);

    if ((dist < pLine->mindist) && (dist < LC_CURVE_POINT_RADIUS))
    {
      pLine->mindist = dist;
      pLine->pClosest = this;
      pLine->pParam = &m_pPoints[i];
    }
  }
}

void Curve::SetSelection (bool bSelect, void *pParam)
{
  Object::SetSelection (bSelect, pParam);

  if (pParam != NULL)
  {
    for (int i = 0; i < m_nNumPoints; i++)
      if (&m_pPoints[i] == pParam)
      {
	if (bSelect)
	  m_pPoints[i].m_nState |= LC_CURVE_POINT_SELECTED;
	else
	  m_pPoints[i].m_nState &= ~(LC_CURVE_POINT_SELECTED | LC_CURVE_POINT_FOCUSED);
      }
  }
  else
  {
    for (int i = 0; i < m_nNumPoints; i++)
      if (bSelect)
	m_pPoints[i].m_nState |= LC_CURVE_POINT_SELECTED;
      else
	m_pPoints[i].m_nState &= ~(LC_CURVE_POINT_SELECTED | LC_CURVE_POINT_FOCUSED);
  }
}

void Curve::SetFocus (bool bFocus, void *pParam)
{
  Object::SetFocus (bFocus, pParam);

  if (pParam != NULL)
  {
    for (int i = 0; i < m_nNumPoints; i++)
      if (&m_pPoints[i] == pParam)
      {
	if (bFocus)
	  m_pPoints[i].m_nState |= (LC_CURVE_POINT_SELECTED | LC_CURVE_POINT_FOCUSED);
	else
	  m_pPoints[i].m_nState &= ~LC_CURVE_POINT_FOCUSED;
      }
      else
      {
	m_pPoints[i].m_nState &= ~LC_CURVE_POINT_FOCUSED;
      }
  }
}


#endif
