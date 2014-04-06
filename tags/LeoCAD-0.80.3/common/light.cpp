#include "lc_global.h"
#include "lc_math.h"
#include "lc_colors.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "light.h"

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

LightTarget::LightTarget(Light *pParent)
	: Object(LC_OBJECT_LIGHT_TARGET)
{
	m_pParent = pParent;
	/*
	strcpy(m_strName, pParent->GetName());
	m_strName[LC_OBJECT_NAME_LEN-8] = '\0';
	strcat(m_strName, ".Target");
	*/
}

LightTarget::~LightTarget()
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

void LightTarget::Select(bool bSelecting, bool bFocus, bool bMultiple)
{
	m_pParent->SelectTarget(bSelecting, bFocus, bMultiple);
}

const char* LightTarget::GetName() const
{
	return m_pParent->GetName();
}

// =============================================================================
// Light class

// New omni light.
Light::Light(float px, float py, float pz)
	: Object(LC_OBJECT_LIGHT)
{
	Initialize();

	float pos[] = { px, py, pz }, target[] = { 0, 0, 0 };

	ChangeKey(1, true, pos, LC_LK_POSITION);
	ChangeKey(1, true, target, LC_LK_TARGET);

	UpdatePosition(1);
}

// New directional or spot light.
Light::Light(float px, float py, float pz, float tx, float ty, float tz)
	: Object(LC_OBJECT_LIGHT)
{
	Initialize();

	float pos[] = { px, py, pz }, target[] = { tx, ty, tz };

	ChangeKey(1, true, pos, LC_LK_POSITION);
	ChangeKey(1, true, target, LC_LK_TARGET);

	m_pTarget = new LightTarget(this);

	UpdatePosition(1);
}

void Light::Initialize()
{
	m_bEnabled = true;
	m_pNext = NULL;
	m_nState = 0;
	m_pTarget = NULL;
	memset(m_strName, 0, sizeof(m_strName));

	mAmbientColor[3] = 1.0f;
	mDiffuseColor[3] = 1.0f;
	mSpecularColor[3] = 1.0f;

	float *values[] = { mPosition, mTargetPosition, mAmbientColor, mDiffuseColor, mSpecularColor,
	                    &mConstantAttenuation, &mLinearAttenuation, &mQuadraticAttenuation, &mSpotCutoff, &mSpotExponent };
	RegisterKeys(values, light_key_info, LC_LK_COUNT);

	// set the default values
	float ambient[] = { 0, 0, 0 }, diffuse[] = { 0.8f, 0.8f, 0.8f }, specular[] = { 1, 1, 1 };
	float constant = 1, linear = 0, quadratic = 0, cutoff = 30, exponent = 0;

	ChangeKey(1, true, ambient, LC_LK_AMBIENT_COLOR);
	ChangeKey(1, true, diffuse, LC_LK_DIFFUSE_COLOR);
	ChangeKey(1, true, specular, LC_LK_SPECULAR_COLOR);
	ChangeKey(1, true, &constant, LC_LK_CONSTANT_ATTENUATION);
	ChangeKey(1, true, &linear, LC_LK_LINEAR_ATTENUATION);
	ChangeKey(1, true, &quadratic, LC_LK_QUADRATIC_ATTENUATION);
	ChangeKey(1, true, &cutoff, LC_LK_SPOT_CUTOFF);
	ChangeKey(1, true, &exponent, LC_LK_SPOT_EXPONENT);
}

Light::~Light()
{
	delete m_pTarget;
}

void Light::CreateName(const Light* pLight)
{
	int i, max = 0;

	for (; pLight; pLight = pLight->m_pNext)
	{
		if (strncmp(pLight->m_strName, "Light ", 6) == 0)
		{
			if (sscanf(pLight->m_strName + 6, " #%d", &i) == 1)
			{
				if (i > max)
					max = i;
			}
		}
	}

	sprintf(m_strName, "Light #%.2d", max+1);
}

void Light::Select(bool bSelecting, bool bFocus, bool bMultiple)
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

void Light::SelectTarget(bool bSelecting, bool bFocus, bool bMultiple)
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

void Light::Move(unsigned short nTime, bool bAddKey, float dx, float dy, float dz)
{
	lcVector3 MoveVec(dx, dy, dz);

	if (IsEyeSelected())
	{
		mPosition += MoveVec;

		ChangeKey(nTime, bAddKey, mPosition, LC_LK_POSITION);
	}

	if (IsTargetSelected())
	{
		mTargetPosition += MoveVec;

		ChangeKey(nTime, bAddKey, mTargetPosition, LC_LK_TARGET);
	}
}

void Light::UpdatePosition(unsigned short nTime)
{
	CalculateKeys(nTime);

	if (m_pTarget != NULL)
	{
		lcVector3 frontvec = mTargetPosition - mPosition;
		lcVector3 up(1, 1, 1);

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
}

void Light::Render(const lcMatrix44& ViewMatrix, float LineWidth)
{
	if (m_pTarget != NULL)
	{
		if (IsEyeSelected())
		{
			glLineWidth(LineWidth*2);
			if (m_nState & LC_LIGHT_FOCUSED)
				lcSetColorFocused();
			else
				lcSetColorSelected();
			RenderCone(ViewMatrix);
			glLineWidth(LineWidth);
		}
		else
		{
			lcSetColorLight();
			RenderCone(ViewMatrix);
		}

		if (IsTargetSelected())
		{
			glLineWidth(LineWidth*2);
			if (m_nState & LC_LIGHT_TARGET_FOCUSED)
				lcSetColorFocused();
			else
				lcSetColorSelected();
			RenderTarget();
			glLineWidth(LineWidth);
		}
		else
		{
			lcSetColorLight();
			RenderTarget();
		}

		glLoadMatrixf(ViewMatrix);

		lcSetColorLight();

		lcVector3 Line[2] = { mPosition, mTargetPosition };
		glVertexPointer(3, GL_FLOAT, 0, Line);

		glDrawArrays(GL_LINES, 0, 2);

		if (IsSelected())
		{
			lcMatrix44 ProjectionMatrix, LightMatrix;
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

			LightMatrix = lcMatrix44LookAt(mPosition, mTargetPosition, UpVector);
			LightMatrix = lcMatrix44AffineInverse(LightMatrix);
			ProjectionMatrix = lcMatrix44Perspective(2 * mSpotCutoff, 1.0f, 0.01f, Length);
			ProjectionMatrix = lcMatrix44Inverse(ProjectionMatrix);
			glLoadMatrixf(lcMul(ProjectionMatrix, lcMul(LightMatrix, ViewMatrix)));

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

			glVertexPointer(3, GL_FLOAT, 0, Verts);
			glDrawArrays(GL_LINE_LOOP, 0, 8);
			glDrawArrays(GL_LINES, 8, 8);
		}
	}
	else
	{
		glLoadMatrixf(lcMul(lcMatrix44Translation(mPosition), ViewMatrix));

		if (IsEyeSelected())
		{
			if (m_nState & LC_LIGHT_FOCUSED)
				lcSetColorFocused();
			else
				lcSetColorSelected();
		}
		else
			lcSetColorLight();

		RenderSphere();
	}
}

void Light::RenderCone(const lcMatrix44& ViewMatrix)
{
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

	lcMatrix44 LightMatrix = lcMatrix44LookAt(mPosition, mTargetPosition, UpVector);
	LightMatrix = lcMatrix44AffineInverse(LightMatrix);
	LightMatrix.SetTranslation(lcVector3(0, 0, 0));

	lcMatrix44 LightViewMatrix = lcMul(LightMatrix, lcMul(lcMatrix44Translation(mPosition), ViewMatrix));

	glLoadMatrixf(LightViewMatrix);

	float verts[16*3];
	for (int i = 0; i < 8; i++)
	{
		verts[i*6] = verts[i*6+3] = (float)cos((float)i/4 * LC_PI) * 0.3f;
		verts[i*6+1] = verts[i*6+4] = (float)sin((float)i/4 * LC_PI) * 0.3f;
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

	glLoadMatrixf(lcMul(lcMatrix44Translation(lcVector3(0, 0, -Length)), LightViewMatrix));
}

void Light::RenderTarget()
{
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
}

void Light::RenderSphere()
{
	const int Slices = 6;
	const int NumIndices = 3 * Slices + 6 * Slices * (Slices - 2) + 3 * Slices;
	const int NumVertices = (Slices - 1) * Slices + 2;
	const float Radius = 0.2f;
	lcVector3 Vertices[NumVertices];
	lcuint16 Indices[NumIndices];

	lcVector3* Vertex = Vertices;
	lcuint16* Index = Indices;

	*Vertex++ = lcVector3(0, 0, Radius);

	for (int i = 1; i < Slices; i++ )
	{
		float r0 = Radius * sinf(i * (LC_PI / Slices));
		float z0 = Radius * cosf(i * (LC_PI / Slices));

		for (int j = 0; j < Slices; j++)
		{
			float x0 = r0 * sinf(j * (LC_2PI / Slices));
			float y0 = r0 * cosf(j * (LC_2PI / Slices));

			*Vertex++ = lcVector3(x0, y0, z0);
		}
	}

	*Vertex++ = lcVector3(0, 0, -Radius);

	for (int i = 0; i < Slices - 1; i++ )
	{
		*Index++ = 0;
		*Index++ = 1 + i;
		*Index++ = 1 + i + 1;
	}

	*Index++ = 0;
	*Index++ = 1;
	*Index++ = 1 + Slices - 1;

	for (int i = 0; i < Slices - 2; i++ )
	{
		int Row1 = 1 + i * Slices;
		int Row2 = 1 + (i + 1) * Slices;

		for (int j = 0; j < Slices - 1; j++ )
		{
			*Index++ = Row1 + j;
			*Index++ = Row2 + j + 1;
			*Index++ = Row2 + j;

			*Index++ = Row1 + j;
			*Index++ = Row1 + j + 1;
			*Index++ = Row2 + j + 1;
		}

		*Index++ = Row1 + Slices - 1;
		*Index++ = Row2 + 0;
		*Index++ = Row2 + Slices - 1;

		*Index++ = Row1 + Slices - 1;
		*Index++ = Row2 + 0;
		*Index++ = Row1 + 0;
	}

	for (int i = 0; i < Slices - 1; i++ )
	{
		*Index++ = (Slices - 1) * Slices + 1;
		*Index++ = (Slices - 1) * (Slices - 1) + i;
		*Index++ = (Slices - 1) * (Slices - 1) + i + 1;
	}

	*Index++ = (Slices - 1) * Slices + 1;
	*Index++ = (Slices - 1) * (Slices - 1) + (Slices - 2) + 1;
	*Index++ = (Slices - 1) * (Slices - 1);

	glVertexPointer(3, GL_FLOAT, 0, Vertices);
	glDrawElements(GL_TRIANGLES, NumIndices, GL_UNSIGNED_SHORT, Indices);
}

void Light::Setup(int index)
{
	GLenum light = (GLenum)(GL_LIGHT0+index);

	if (!m_bEnabled)
	{
		glDisable(light);
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
