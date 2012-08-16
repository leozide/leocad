#include "lc_global.h"
#include "lc_math.h"
#include "lc_colors.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "light.h"
#include "globals.h"

GLuint Light::m_nSphereList = 0;
GLuint Light::m_nTargetList = 0;

static LC_OBJECT_KEY_INFO light_key_info[LC_LK_COUNT] =
{
	{ "Light Position", 3, LC_LK_POSITION },
	{ "Light Target", 3, LC_LK_TARGET },
	{ "Ambient Color", 3, LC_LK_AMBIENT_COLOR },
	{ "Diffuse Color", 3, LC_LK_DIFFUSE_COLOR },
	{ "Specular Color", 3, LC_LK_SPECULAR_COLOR },
	{ "Constant Attenuation", 1, LC_LK_CONSTANT_ATTENUATION },
	{ "Linear Attenuation", 1, LC_LK_LINEAR_ATTENUATION },
	{ "Quadratic Attenuation", 1, LC_LK_QUADRATIC_ATTENUATION },
	{ "Spot Cutoff", 1, LC_LK_SPOT_CUTOFF },
	{ "Spot Exponent", 1, LC_LK_SPOT_EXPONENT }
};

// =============================================================================
// LightTarget class

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

void LightTarget::MinIntersectDist(lcClickLine* ClickLine)
{
	lcVector3 Min = lcVector3(-0.2f, -0.2f, -0.2f);
	lcVector3 Max = lcVector3(0.2f, 0.2f, 0.2f);

	lcMatrix44 WorldLight = ((Light*)m_pParent)->mWorldLight;
	WorldLight.SetTranslation(lcMul30(-((Light*)m_pParent)->mTargetPosition, WorldLight));

	lcVector3 Start = lcMul31(ClickLine->Start, WorldLight);
	lcVector3 End = lcMul31(ClickLine->End, WorldLight);

	float Dist;
	if (lcBoundingBoxRayMinIntersectDistance(Min, Max, Start, End, &Dist, NULL) && (Dist < ClickLine->MinDist))
	{
		ClickLine->Closest = this;
		ClickLine->MinDist = Dist;
	}
}

void LightTarget::Select (bool bSelecting, bool bFocus, bool bMultiple)
{
	m_pParent->SelectTarget(bSelecting, bFocus, bMultiple);
}

const char* LightTarget::GetName() const
{
	return m_pParent->GetName();
}

// =============================================================================
// Light class

// New positional light
Light::Light (float px, float py, float pz)
  : Object (LC_OBJECT_LIGHT)
{
  Initialize ();

  float pos[] = { px, py, pz }, target[] = { 0, 0, 0 };

  ChangeKey (1, false, true, pos, LC_LK_POSITION);
  ChangeKey (1, false, true, target, LC_LK_TARGET);
  ChangeKey (1, true, true, pos, LC_LK_POSITION);
  ChangeKey (1, true, true, target, LC_LK_TARGET);

  UpdatePosition (1, false);
}

// New directional light
Light::Light (float px, float py, float pz, float tx, float ty, float tz)
  : Object (LC_OBJECT_LIGHT)
{
  Initialize ();

  float pos[] = { px, py, pz }, target[] = { tx, ty, tz };

  ChangeKey (1, false, true, pos, LC_LK_POSITION);
  ChangeKey (1, false, true, target, LC_LK_TARGET);
  ChangeKey (1, true, true, pos, LC_LK_POSITION);
  ChangeKey (1, true, true, target, LC_LK_TARGET);

  m_pTarget = new LightTarget (this);

  UpdatePosition (1, false);
}

void Light::Initialize ()
{
	m_bEnabled = true;
	m_pNext = NULL;
	m_nState = 0;
	m_pTarget = NULL;
	m_nList = 0;
	memset(m_strName, 0, sizeof (m_strName));

	mAmbientColor[3] = 1.0f;
	mDiffuseColor[3] = 1.0f;
	mSpecularColor[3] = 1.0f;

	float *values[] = { mPosition, mTargetPosition, mAmbientColor, mDiffuseColor, mSpecularColor,
	                    &mConstantAttenuation, &mLinearAttenuation, &mQuadraticAttenuation, &mSpotCutoff, &mSpotExponent };
	RegisterKeys(values, light_key_info, LC_LK_COUNT);

	// set the default values
	float ambient[] = { 0, 0, 0 }, diffuse[] = { 0.8f, 0.8f, 0.8f }, specular[] = { 1, 1, 1 };
	float constant = 1, linear = 0, quadratic = 0, cutoff = 30, exponent = 0;

	ChangeKey(1, false, true, ambient, LC_LK_AMBIENT_COLOR);
	ChangeKey(1, false, true, diffuse, LC_LK_DIFFUSE_COLOR);
	ChangeKey(1, false, true, specular, LC_LK_SPECULAR_COLOR);
	ChangeKey(1, false, true, &constant, LC_LK_CONSTANT_ATTENUATION);
	ChangeKey(1, false, true, &linear, LC_LK_LINEAR_ATTENUATION);
	ChangeKey(1, false, true, &quadratic, LC_LK_QUADRATIC_ATTENUATION);
	ChangeKey(1, false, true, &cutoff, LC_LK_SPOT_CUTOFF);
	ChangeKey(1, false, true, &exponent, LC_LK_SPOT_EXPONENT);
	ChangeKey(1, true, true, ambient, LC_LK_AMBIENT_COLOR);
	ChangeKey(1, true, true, diffuse, LC_LK_DIFFUSE_COLOR);
	ChangeKey(1, true, true, specular, LC_LK_SPECULAR_COLOR);
	ChangeKey(1, true, true, &constant, LC_LK_CONSTANT_ATTENUATION);
	ChangeKey(1, true, true, &linear, LC_LK_LINEAR_ATTENUATION);
	ChangeKey(1, true, true, &quadratic, LC_LK_QUADRATIC_ATTENUATION);
	ChangeKey(1, true, true, &cutoff, LC_LK_SPOT_CUTOFF);
	ChangeKey(1, true, true, &exponent, LC_LK_SPOT_EXPONENT);
}

Light::~Light ()
{
  if (m_nList != 0)
    glDeleteLists (m_nList, 1);

  delete m_pTarget;
}

void Light::CreateName(const Light* pLight)
{
	int i, max = 0;

	for (; pLight; pLight = pLight->m_pNext)
	{
		if (strncmp (pLight->m_strName, "Light ", 6) == 0)
		{
			if (sscanf(pLight->m_strName + 6, " #%d", &i) == 1)
			{
				if (i > max)
					max = i;
			}
		}
	}

	sprintf (m_strName, "Light #%.2d", max+1);
}

void Light::Select (bool bSelecting, bool bFocus, bool bMultiple)
{
  if (bSelecting == true)
  {
    if (bFocus == true)
    {
      m_nState |= (LC_LIGHT_FOCUSED|LC_LIGHT_SELECTED);

      if (m_pTarget != NULL)
        m_pTarget->Select (false, true, bMultiple);
    }
    else
      m_nState |= LC_LIGHT_SELECTED;

    if (bMultiple == false)
      if (m_pTarget != NULL)
        m_pTarget->Select (false, false, bMultiple);
  }
  else
  {
    if (bFocus == true)
      m_nState &= ~(LC_LIGHT_FOCUSED);
    else
      m_nState &= ~(LC_LIGHT_SELECTED|LC_LIGHT_FOCUSED);
  } 
}

void Light::SelectTarget (bool bSelecting, bool bFocus, bool bMultiple)
{
  // FIXME: the target should handle this

  if (bSelecting == true)
  {
    if (bFocus == true)
    {
      m_nState |= (LC_LIGHT_TARGET_FOCUSED|LC_LIGHT_TARGET_SELECTED);

      Select (false, true, bMultiple);
    }
    else
      m_nState |= LC_LIGHT_TARGET_SELECTED;

    if (bMultiple == false)
      Select (false, false, bMultiple);
  }
  else
  {
    if (bFocus == true)
      m_nState &= ~(LC_LIGHT_TARGET_FOCUSED);
    else
      m_nState &= ~(LC_LIGHT_TARGET_SELECTED|LC_LIGHT_TARGET_FOCUSED);
  } 
}

void Light::MinIntersectDist(lcClickLine* ClickLine)
{
	if (m_pTarget)
	{
		lcVector3 Min = lcVector3(-0.2f, -0.2f, -0.2f);
		lcVector3 Max = lcVector3(0.2f, 0.2f, 0.2f);

		lcVector3 Start = lcMul31(ClickLine->Start, mWorldLight);
		lcVector3 End = lcMul31(ClickLine->End, mWorldLight);

		float Dist;
		if (lcBoundingBoxRayMinIntersectDistance(Min, Max, Start, End, &Dist, NULL) && (Dist < ClickLine->MinDist))
		{
			ClickLine->Closest = this;
			ClickLine->MinDist = Dist;
		}

		m_pTarget->MinIntersectDist(ClickLine);
	}
	else
	{
		float Dist;
		if (lcSphereRayMinIntersectDistance(mPosition, 0.2f, ClickLine->Start, ClickLine->End, &Dist))
		{
			ClickLine->Closest = this;
			ClickLine->MinDist = Dist;
		}
	}
}

void Light::Move (unsigned short nTime, bool bAnimation, bool bAddKey, float dx, float dy, float dz)
{
	lcVector3 Move(dx, dy, dz);

	if (IsEyeSelected())
	{
		mPosition += Move;

		ChangeKey (nTime, bAnimation, bAddKey, mPosition, LC_LK_POSITION);
	}

	if (IsTargetSelected())
	{
		mTargetPosition += Move;

		ChangeKey (nTime, bAnimation, bAddKey, mTargetPosition, LC_LK_TARGET);
	}
}

void Light::UpdatePosition (unsigned short nTime, bool bAnimation)
{
	CalculateKeys(nTime, bAnimation);

	if (m_pTarget != NULL)
	{
		lcVector3 frontvec = mTargetPosition - mPosition;
		lcVector3 up(1, 1, 1);
//		float len = Length(frontvec);

		if (fabs(frontvec[0]) < fabs(frontvec[1]))
		{
			if (fabs(frontvec[0]) < fabs(frontvec[2]))
				up[0] = -(up[1]*frontvec[1] + up[2]*frontvec[2]);
			else
				up[2] = -(up[0]*frontvec[0] + up[1]*frontvec[1]);
		}
		else
		{
			if (fabs(frontvec[1]) < fabs(frontvec[2]))
				up[1] = -(up[0]*frontvec[0] + up[2]*frontvec[2]);
			else
				up[2] = -(up[0]*frontvec[0] + up[1]*frontvec[1]);
		}

		mWorldLight = lcMatrix44LookAt(mPosition, mTargetPosition, up);
	}
	else
	{
		mWorldLight = lcMatrix44Identity();
		mWorldLight.SetTranslation(-mPosition);
	}

	if (m_pTarget != NULL)
	{
		if (m_nList == 0)
			m_nList = glGenLists(1);

		glNewList(m_nList, GL_COMPILE);

		glPushMatrix();
		glTranslatef(mPosition[0], mPosition[1], mPosition[2]);

		lcVector3 FrontVector(mTargetPosition - mPosition);
		lcVector3 UpVector(1, 1, 1);
		float Length = FrontVector.Length();

		if (fabs (FrontVector[0]) < fabs (FrontVector[1]))
		{
			if (fabs(FrontVector[0]) < fabs(FrontVector[2]))
				UpVector[0] = -(UpVector[1] * FrontVector[1] + UpVector[2] * FrontVector[2]);
			else
				UpVector[2] = -(UpVector[0] * FrontVector[0] + UpVector[1] * FrontVector[1]);
		}
		else
		{
			if (fabs(FrontVector[1]) < fabs(FrontVector[2]))
				UpVector[1] = -(UpVector[0] * FrontVector[0] + UpVector[2] * FrontVector[2]);
			else
				UpVector[2] = -(UpVector[0] * FrontVector[0] + UpVector[1] * FrontVector[1]);
		}

		lcMatrix44 mat = lcMatrix44LookAt(mPosition, mTargetPosition, UpVector);
		mat = lcMatrix44AffineInverse(mat);
		mat.SetTranslation(lcVector3(0, 0, 0));

		glMultMatrixf(mat);

    glEnableClientState (GL_VERTEX_ARRAY);
    float verts[16*3];
    for (int i = 0; i < 8; i++)
    {
      verts[i*6] = verts[i*6+3] = (float)cos ((float)i/4 * LC_PI) * 0.3f;
      verts[i*6+1] = verts[i*6+4] = (float)sin ((float)i/4 * LC_PI) * 0.3f;
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

    glTranslatef(0, 0, -Length);
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
			if (m_nState & LC_LIGHT_FOCUSED)
				lcSetColorFocused();
			else
				lcSetColorSelected();
			glCallList(m_nList);
			glLineWidth(fLineWidth);
		}
		else
		{
			lcSetColorLight();
			glCallList(m_nList);
		}

		if (IsTargetSelected())
		{
			glLineWidth(fLineWidth*2);
			if (m_nState & LC_LIGHT_TARGET_FOCUSED)
				lcSetColorFocused();
			else
				lcSetColorSelected();
			glCallList(m_nTargetList);
			glLineWidth(fLineWidth);
		}
		else
		{
			lcSetColorLight();
			glCallList(m_nTargetList);
		}

		lcSetColorLight();
		glBegin(GL_LINES);
		glVertex3fv(mPosition);
		glVertex3fv(mTargetPosition);
		glEnd();

		if (IsSelected())
		{
			lcMatrix44 projection, modelview;
			lcVector3 FrontVector(mTargetPosition - mPosition);
			lcVector3 UpVector(1, 1, 1);
			float Length = FrontVector.Length();

			if (fabs(FrontVector[0]) < fabs(FrontVector[1]))
			{
				if (fabs(FrontVector[0]) < fabs(FrontVector[2]))
					UpVector[0] = -(UpVector[1] * FrontVector[1] + UpVector[2] * FrontVector[2]);
				else
					UpVector[2] = -(UpVector[0] * FrontVector[0] + UpVector[1] * FrontVector[1]);
			}
			else
			{
				if (fabs(FrontVector[1]) < fabs(FrontVector[2]))
					UpVector[1] = -(UpVector[0] * FrontVector[0] + UpVector[2] * FrontVector[2]);
				else
					UpVector[2] = -(UpVector[0] * FrontVector[0] + UpVector[1] * FrontVector[1]);
			}

			glPushMatrix();

			modelview = lcMatrix44LookAt(mPosition, mTargetPosition, UpVector);
			modelview = lcMatrix44AffineInverse(modelview);
			glMultMatrixf(modelview);

			projection = lcMatrix44Perspective(2*mSpotCutoff, 1.0f, 0.01f, Length);
			projection = lcMatrix44Inverse(projection);
			glMultMatrixf(projection);

			// draw the viewing frustum
			glBegin(GL_LINE_LOOP);
			glVertex3f( 0.5f,  1.0f, 1.0f);
			glVertex3f( 1.0f,  0.5f, 1.0f);
			glVertex3f( 1.0f, -0.5f, 1.0f);
			glVertex3f( 0.5f, -1.0f, 1.0f);
			glVertex3f(-0.5f, -1.0f, 1.0f);
			glVertex3f(-1.0f, -0.5f, 1.0f);
			glVertex3f(-1.0f,  0.5f, 1.0f);
			glVertex3f(-0.5f,  1.0f, 1.0f);
			glEnd();

			glBegin(GL_LINES);
			glVertex3f(1, 1, -1);
			glVertex3f(0.75f, 0.75f, 1);
			glVertex3f(-1, 1, -1);
			glVertex3f(-0.75f, 0.75f, 1);
			glVertex3f(-1, -1, -1);
			glVertex3f(-0.75f, -0.75f, 1);
			glVertex3f(1, -1, -1);
			glVertex3f(0.75f, -0.75f, 1);
			glEnd();

			glPopMatrix();
		}
	}
	else
	{
		glPushMatrix ();
		glTranslatef (mPosition[0], mPosition[1], mPosition[2]);

		if (IsEyeSelected ())
		{
			glLineWidth (fLineWidth*2);
			if (m_nState & LC_LIGHT_FOCUSED)
				lcSetColorFocused();
			else
				lcSetColorSelected();
			glCallList (m_nSphereList);
			glLineWidth (fLineWidth);
		}
		else
		{
			lcSetColorLight();
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

	bool Omni = (m_pTarget == NULL);
	bool Spot = (m_pTarget != NULL) && (mSpotCutoff != 180.0f);

	glEnable(light);

	glLightfv(light, GL_AMBIENT, mAmbientColor);
	glLightfv(light, GL_DIFFUSE, mDiffuseColor);
	glLightfv(light, GL_SPECULAR, mSpecularColor);

	if (Omni || Spot)
	{
		glLightf(light, GL_CONSTANT_ATTENUATION, mConstantAttenuation);
		glLightf(light, GL_LINEAR_ATTENUATION, mLinearAttenuation);
		glLightf(light, GL_QUADRATIC_ATTENUATION, mQuadraticAttenuation);

		lcVector4 Position(mPosition, 1.0f);
		glLightfv(light, GL_POSITION, Position);
	}
	else
	{
		glLightf(light, GL_CONSTANT_ATTENUATION, 1.0f);
		glLightf(light, GL_LINEAR_ATTENUATION, 0.0f);
		glLightf(light, GL_QUADRATIC_ATTENUATION, 0.0f);

		lcVector4 Position(mPosition, 0.0f);
		glLightfv(light, GL_POSITION, Position);
	}

	if (Omni)
	{
		lcVector3 Dir(0.0f, 0.0f, 0.0f);

		glLightf(light, GL_SPOT_CUTOFF, 180.0f);
		glLightf(light, GL_SPOT_EXPONENT, mSpotExponent);
		glLightfv(light, GL_SPOT_DIRECTION, Dir);
	}
	else
	{
		lcVector3 Dir(mTargetPosition - mPosition);
		Dir.Normalize();

		glLightf(light, GL_SPOT_CUTOFF, mSpotCutoff);
		glLightf(light, GL_SPOT_EXPONENT, mSpotExponent);
		glLightfv(light, GL_SPOT_DIRECTION, Dir);
	}
}
