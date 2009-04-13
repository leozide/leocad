// Light object.

#include "lc_global.h"
#include "light.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "defines.h"
#include "globals.h"
#include "matrix.h"

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
// CameraTarget class

LightTarget::LightTarget(lcLight *pParent)
	: lcObject(LC_OBJECT_LIGHT_TARGET)
{
	m_pParent = pParent;
	m_Name = pParent->m_Name + ".Target";
}

LightTarget::~LightTarget()
{
}

void LightTarget::MinIntersectDist(LC_CLICKLINE* pLine)
{
	float dist = (float)BoundingBoxIntersectDist(pLine);

	if (dist < pLine->mindist)
	{
		pLine->mindist = dist;
		pLine->pClosest = this;
	}
}

void LightTarget::Select(bool bSelecting, bool bFocus, bool bMultiple)
{
	m_pParent->SelectTarget(bSelecting, bFocus, bMultiple);
}

// =============================================================================
// Light class

// New positional light
lcLight::lcLight(float px, float py, float pz)
	: lcObject(LC_OBJECT_LIGHT)
{
	Initialize();

	float pos[] = { px, py, pz }, target[] = { 0, 0, 0 };

	ChangeKey(1, false, true, pos, LC_LK_POSITION);
	ChangeKey(1, false, true, target, LC_LK_TARGET);
	ChangeKey(1, true, true, pos, LC_LK_POSITION);
	ChangeKey(1, true, true, target, LC_LK_TARGET);

	m_fPos[3] = 0.0f;

	UpdatePosition(1, false);
}

// New directional light
lcLight::lcLight(float px, float py, float pz, float tx, float ty, float tz)
	: lcObject(LC_OBJECT_LIGHT)
{
	Initialize();

	float pos[] = { px, py, pz }, target[] = { tx, ty, tz };

	ChangeKey(1, false, true, pos, LC_LK_POSITION);
	ChangeKey(1, false, true, target, LC_LK_TARGET);
	ChangeKey(1, true, true, pos, LC_LK_POSITION);
	ChangeKey(1, true, true, target, LC_LK_TARGET);

	m_pTarget = new LightTarget(this);
	m_fPos[3] = 1.0f;

	UpdatePosition(1, false);
}

void lcLight::Initialize()
{
	m_bEnabled = true;
	m_nState = 0;
	m_pTarget = NULL;
	m_Name = "";

	m_fAmbient[3] = 1.0f;
	m_fDiffuse[3] = 1.0f;
	m_fSpecular[3] = 1.0f;

	float *values[] = { m_fPos, m_fTarget, m_fAmbient, m_fDiffuse, m_fSpecular,
	                    &m_fConstant, &m_fLinear, &m_fQuadratic, &m_fCutoff, &m_fExponent };
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

lcLight::~lcLight()
{
	delete m_pTarget;
}

void lcLight::CreateName(const lcLight* pLight)
{
	int i, max = 0;

	for (; pLight; pLight = (lcLight*)pLight->m_Next)
	{
		if (strncmp(pLight->m_Name, "Light ", 6) == 0)
		{
			if (sscanf((char*)pLight->m_Name + 6, " #%d", &i) == 1)
			{
				if (i > max)
					max = i;
			}
		}
	}

	char Name[256];
	sprintf(Name, "Light #%.2d", max+1);
	m_Name = Name;
}

void lcLight::Select(bool bSelecting, bool bFocus, bool bMultiple)
{
	if (bSelecting == true)
	{
		if (bFocus == true)
		{
			m_nState |= (LC_LIGHT_FOCUSED|LC_LIGHT_SELECTED);

			if (m_pTarget != NULL)
				m_pTarget->Select(false, true, bMultiple);
		}
		else
			m_nState |= LC_LIGHT_SELECTED;

		if (bMultiple == false)
			if (m_pTarget != NULL)
				m_pTarget->Select(false, false, bMultiple);
	}
	else
	{
		if (bFocus == true)
			m_nState &= ~(LC_LIGHT_FOCUSED);
		else
			m_nState &= ~(LC_LIGHT_SELECTED|LC_LIGHT_FOCUSED);
	}
}

void lcLight::SelectTarget(bool bSelecting, bool bFocus, bool bMultiple)
{
	// TODO: the target should handle this

	if (bSelecting == true)
	{
		if (bFocus == true)
		{
			m_nState |= (LC_LIGHT_TARGET_FOCUSED|LC_LIGHT_TARGET_SELECTED);

			Select(false, true, bMultiple);
		}
		else
			m_nState |= LC_LIGHT_TARGET_SELECTED;

		if (bMultiple == false)
			Select(false, false, bMultiple);
	}
	else
	{
		if (bFocus == true)
			m_nState &= ~(LC_LIGHT_TARGET_FOCUSED);
		else
			m_nState &= ~(LC_LIGHT_TARGET_SELECTED|LC_LIGHT_TARGET_FOCUSED);
	}
}

void lcLight::MinIntersectDist(LC_CLICKLINE* pLine)
{
	float dist;

	if (m_nState & LC_LIGHT_HIDDEN)
		return;

	dist = (float)BoundingBoxIntersectDist(pLine);
	if (dist < pLine->mindist)
	{
		pLine->mindist = dist;
		pLine->pClosest = this;
	}

	if (m_pTarget != NULL)
		m_pTarget->MinIntersectDist(pLine);
}

void lcLight::Move(unsigned short nTime, bool bAnimation, bool bAddKey, float dx, float dy, float dz)
{
	if (IsEyeSelected())
	{
		m_fPos[0] += dx;
		m_fPos[1] += dy;
		m_fPos[2] += dz;

		ChangeKey(nTime, bAnimation, bAddKey, m_fPos, LC_LK_POSITION);
	}

	if (IsTargetSelected())
	{
		m_fTarget[0] += dx;
		m_fTarget[1] += dy;
		m_fTarget[2] += dz;

		ChangeKey(nTime, bAnimation, bAddKey, m_fTarget, LC_LK_TARGET);
	}
}

void lcLight::UpdatePosition(unsigned short nTime, bool bAnimation)
{
	CalculateKeys(nTime, bAnimation);
	BoundingBoxCalculate(m_fPos);

	if (m_pTarget != NULL)
		m_pTarget->BoundingBoxCalculate(m_fTarget);
}

void lcLight::Render(float fLineWidth)
{
	if (m_pTarget != NULL)
	{
		if (IsEyeSelected())
		{
			glLineWidth(fLineWidth*2);
			int Color = (m_nState & LC_LIGHT_FOCUSED) != 0 ? LC_COL_FOCUSED : LC_COL_SELECTED;
			glColor4ub(FlatColorArray[Color][0], FlatColorArray[Color][1], FlatColorArray[Color][2], 255);
			DrawCone();
			glLineWidth(fLineWidth);
		}
		else
		{
			glColor4f(0.5f, 0.8f, 0.5f, 1.0f);
			DrawCone();
		}

		if (IsTargetSelected())
		{
			glLineWidth(fLineWidth*2);
			int Color = (m_nState & LC_LIGHT_TARGET_FOCUSED) != 0 ? LC_COL_FOCUSED : LC_COL_SELECTED;
			glColor4ub(FlatColorArray[Color][0], FlatColorArray[Color][1], FlatColorArray[Color][2], 255);
			DrawTarget();
			glLineWidth(fLineWidth);
		}
		else
		{
			glColor4f(0.5f, 0.8f, 0.5f, 1.0f);
			DrawTarget();
		}

		glColor4f(0.5f, 0.8f, 0.5f, 1.0f);

		float Line[2][3] = 
		{
			{ m_fPos[0], m_fPos[1], m_fPos[2] },
			{ m_fTarget[0], m_fTarget[1], m_fTarget[2] }
		};

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, Line);
		glDrawArrays(GL_LINES, 0, 2);

		if (IsSelected())
		{
			Matrix44 projection, modelview;
			Vector3 frontvec(m_fTarget[0]-m_fPos[0], m_fTarget[1]-m_fPos[1], m_fTarget[2]-m_fPos[2]);
			Vector3 up(1, 1, 1);
			float len = Length(frontvec);

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

			glPushMatrix();

			modelview = CreateLookAtMatrix(Vector3(m_fPos[0], m_fPos[1], m_fPos[2]), Vector3(m_fTarget[0], m_fTarget[1], m_fTarget[2]), up);
			modelview = RotTranInverse(modelview);
			glMultMatrixf(modelview);

			projection = CreatePerspectiveMatrix(2*m_fCutoff, 1.0f, 0.01f, len);
			projection = Inverse(projection);
			glMultMatrixf(projection);

			// Draw the light cone.
			float Verts[16][3] =
			{
				{  0.5f,   1.0f,  1.0f },
				{  1.0f,   0.5f,  1.0f },
				{  1.0f,  -0.5f,  1.0f },
				{  0.5f,  -1.0f,  1.0f },
				{ -0.5f,  -1.0f,  1.0f },
				{ -1.0f,  -0.5f,  1.0f },
				{ -1.0f,   0.5f,  1.0f },
				{ -0.5f,   1.0f,  1.0f },
				{  1.0f,   1.0f, -1.0f },
				{  0.75f,  0.75f, 1.0f },
				{ -1.0f,   1.0f, -1.0f },
				{ -0.75f,  0.75f, 1.0f },
				{ -1.0f,  -1.0f, -1.0f },
				{ -0.75f, -0.75f, 1.0f },
				{  1.0f,  -1.0f, -1.0f },
				{  0.75f, -0.75f, 1.0f }
			};

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, Verts);
			glDrawArrays(GL_LINE_LOOP, 0, 8);
			glDrawArrays(GL_LINES, 8, 8);

			glPopMatrix();
		}
	}
	else
	{
		glPushMatrix();
		glTranslatef(m_fPos[0], m_fPos[1], m_fPos[2]);

		if (IsEyeSelected())
		{
			glLineWidth(fLineWidth*2);
			int Color = (m_nState & LC_LIGHT_FOCUSED) != 0 ? LC_COL_FOCUSED : LC_COL_SELECTED;
			glColor4ub(FlatColorArray[Color][0], FlatColorArray[Color][1], FlatColorArray[Color][2], 255);
			DrawSphere();
			glLineWidth(fLineWidth);
		}
		else
		{
			glColor4f(0.5f, 0.8f, 0.5f, 1.0f);
			DrawSphere();
		}

		glPopMatrix();
	}

	glDisableClientState(GL_VERTEX_ARRAY);
}

void lcLight::DrawCone()
{
	glPushMatrix();
	glTranslatef(m_fPos[0], m_fPos[1], m_fPos[2]);

	Vector3 frontvec(m_fTarget[0]-m_fPos[0], m_fTarget[1]-m_fPos[1], m_fTarget[2]-m_fPos[2]);
	float len = Length(frontvec), up[3] = { 1, 1, 1 };

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

	Matrix mat;
	mat.CreateLookat(m_fPos, m_fTarget, up);
	mat.Invert();
	mat.SetTranslation(0, 0, 0);

	glMultMatrixf(mat.m);

	glEnableClientState(GL_VERTEX_ARRAY);
	float verts[16*3];
	for (int i = 0; i < 8; i++)
	{
		verts[i*6] = verts[i*6+3] = (float)cos((float)i/4 * PI) * 0.3f;
		verts[i*6+1] = verts[i*6+4] = (float)sin((float)i/4 * PI) * 0.3f;
		verts[i*6+2] = 0.3f;
		verts[i*6+5] = -0.3f;
	}
	glVertexPointer(3, GL_FLOAT, 0, verts);
	glDrawArrays(GL_LINES, 0, 16);
	glVertexPointer(3, GL_FLOAT, 6*sizeof(float), verts);
	glDrawArrays(GL_LINE_LOOP, 0, 8);
	glVertexPointer(3, GL_FLOAT, 6*sizeof(float), &verts[3]);
	glDrawArrays(GL_LINE_LOOP, 0, 8);

	float Lines[4][3] =
	{
		{ -0.5f, -0.5f, -0.3f },
		{  0.5f, -0.5f, -0.3f },
		{  0.5f,  0.5f, -0.3f },
		{ -0.5f,  0.5f, -0.3f }
	};

	glVertexPointer(3, GL_FLOAT, 0, Lines);
	glDrawArrays(GL_LINE_LOOP, 0, 4);

	glTranslatef(0, 0, -len);
}

void lcLight::DrawTarget()
{
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
	glVertexPointer(3, GL_FLOAT, 0, box);
	glDrawArrays(GL_LINES, 0, 24);
	glPopMatrix();
}

void lcLight::DrawSphere()
{
	glEnableClientState(GL_VERTEX_ARRAY);

	const float radius = 0.2f;
	const int slices = 6, stacks = 6;
	float rho, drho, theta, dtheta;
	int i, j, imin, imax;
	drho = 3.1415926536f/(float)stacks;
	dtheta = 2.0f*3.1415926536f/(float)slices;

	// draw +Z end as a triangle fan
	float Cap[slices+2][3];

	Cap[0][0] = 0.0f;
	Cap[0][1] = 0.0f;
	Cap[0][2] = radius;

	for (j = 0; j <= slices; j++) 
	{
		theta = (j == slices) ? 0.0f : j * dtheta;
		Cap[j+1][0] = (float)(-sin(theta) * sin(drho)) * radius;
		Cap[j+1][1] = (float)(cos(theta) * sin(drho)) * radius;
		Cap[j+1][2] = (float)(cos(drho)) * radius;
	}

	glVertexPointer(3, GL_FLOAT, 0, Cap);
	glDrawArrays(GL_TRIANGLE_FAN, 0, slices+2);

	imin = 1;
	imax = stacks-1;

	float Center[(slices+1)*2][3];
	glVertexPointer(3, GL_FLOAT, 0, Center);

	for (i = imin; i < imax; i++)
	{
		rho = i * drho;

		for (j = 0; j <= slices; j++)
		{
			theta = (j == slices) ? 0.0f : j * dtheta;
			Center[j*2][0] = (float)(-sin(theta) * sin(rho)) * radius;
			Center[j*2][1] = (float)(cos(theta) * sin(rho)) * radius;
			Center[j*2][2] = (float)(cos(rho)) * radius;
			Center[j*2+1][0] = (float)(-sin(theta) * sin(rho+drho)) * radius;
			Center[j*2+1][1] = (float)(cos(theta) * sin(rho+drho)) * radius;
			Center[j*2+1][2] = (float)(cos(rho+drho)) * radius;
		}

		glDrawArrays(GL_TRIANGLE_STRIP, 0, (slices+1)*2);
	}

	// draw -Z end as a triangle fan
	Cap[0][0] = 0.0f;
	Cap[0][1] = 0.0f;
	Cap[0][2] = -radius;

	rho = 3.1415926536f - drho;
	for (j = slices; j >= 0; j--)
	{
		theta = (j==slices) ? 0.0f : j * dtheta;
		Cap[j+1][0] = (float)(-sin(theta) * sin(rho)) * radius;
		Cap[j+1][1] = (float)(cos(theta) * sin(rho)) * radius;
		Cap[j+1][2] = (float)(cos(rho)) * radius;
	}

	glVertexPointer(3, GL_FLOAT, 0, Cap);
	glDrawArrays(GL_TRIANGLE_FAN, 0, slices+2);
}

void lcLight::Setup(int index)
{
	GLenum light = (GLenum)(GL_LIGHT0+index);

	if (!m_bEnabled)
	{
		glDisable(light);
		return;
	}

	bool Omni = (m_pTarget == NULL);
	bool Spot = (m_pTarget != NULL) && (m_fCutoff != 180.0f);

	glEnable(light);

	glLightfv(light, GL_AMBIENT, m_fAmbient);
	glLightfv(light, GL_DIFFUSE, m_fDiffuse);
	glLightfv(light, GL_SPECULAR, m_fSpecular);

	if (Omni || Spot)
	{
		glLightf(light, GL_CONSTANT_ATTENUATION, m_fConstant);
		glLightf(light, GL_LINEAR_ATTENUATION, m_fLinear);
		glLightf(light, GL_QUADRATIC_ATTENUATION, m_fQuadratic);

		m_fPos[3] = 1.0f;
		glLightfv(light, GL_POSITION, m_fPos);
	}
	else
	{
		glLightf(light, GL_CONSTANT_ATTENUATION, 1.0f);
		glLightf(light, GL_LINEAR_ATTENUATION, 0.0f);
		glLightf(light, GL_QUADRATIC_ATTENUATION, 0.0f);

		m_fPos[3] = 0.0f;
		glLightfv(light, GL_POSITION, m_fPos);
	}

	if (Omni)
	{
		Vector3 dir(0.0f, 0.0f, 0.0f);

		glLightf(light, GL_SPOT_CUTOFF, 180.0f);
		glLightf(light, GL_SPOT_EXPONENT, m_fExponent);
		glLightfv(light, GL_SPOT_DIRECTION, dir);
	}
	else
	{
		Vector3 dir(m_fTarget[0]-m_fPos[0], m_fTarget[1]-m_fPos[1], m_fTarget[2]-m_fPos[2]);
		dir = Normalize(dir);

		glLightf(light, GL_SPOT_CUTOFF, m_fCutoff);
		glLightf(light, GL_SPOT_EXPONENT, m_fExponent);
		glLightfv(light, GL_SPOT_DIRECTION, dir);
	}
}
