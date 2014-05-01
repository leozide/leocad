#include "lc_global.h"
#include "lc_math.h"
#include "lc_colors.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "light.h"
#include "camera.h"
#include "view.h"
#include "lc_application.h"
#include "lc_context.h"

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

	UpdatePosition(1);
}

void Light::Initialize()
{
	mState = 0;
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
}

void Light::CreateName(const lcArray<Light*>& Lights)
{
	int i, max = 0;

	for (int LightIdx = 0; LightIdx < Lights.GetSize(); LightIdx++)
	{
		Light* pLight = Lights[LightIdx];

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

void Light::RayTest(lcObjectRayTest& ObjectRayTest) const
{
	if (IsPointLight())
	{
		float Distance;

		if (lcSphereRayMinIntersectDistance(mPosition, 0.2f, ObjectRayTest.Start, ObjectRayTest.End, &Distance))
		{
			ObjectRayTest.ObjectSection.Object = const_cast<Light*>(this);
			ObjectRayTest.ObjectSection.Section = LC_LIGHT_SECTION_POSITION;
			ObjectRayTest.Distance = Distance;
		}

		return;
	}

	lcVector3 Min = lcVector3(-0.2f, -0.2f, -0.2f);
	lcVector3 Max = lcVector3(0.2f, 0.2f, 0.2f);

	lcVector3 Start = lcMul31(ObjectRayTest.Start, mWorldLight);
	lcVector3 End = lcMul31(ObjectRayTest.End, mWorldLight);

	float Distance;
	if (lcBoundingBoxRayMinIntersectDistance(Min, Max, Start, End, &Distance, NULL) && (Distance < ObjectRayTest.Distance))
	{
		ObjectRayTest.ObjectSection.Object = const_cast<Light*>(this);
		ObjectRayTest.ObjectSection.Section = LC_LIGHT_SECTION_POSITION;
		ObjectRayTest.Distance = Distance;
	}

	Min = lcVector3(-0.2f, -0.2f, -0.2f);
	Max = lcVector3(0.2f, 0.2f, 0.2f);

	lcMatrix44 WorldTarget = mWorldLight;
	WorldTarget.SetTranslation(lcMul30(-mTargetPosition, WorldTarget));

	Start = lcMul31(ObjectRayTest.Start, WorldTarget);
	End = lcMul31(ObjectRayTest.End, WorldTarget);

	if (lcBoundingBoxRayMinIntersectDistance(Min, Max, Start, End, &Distance, NULL) && (Distance < ObjectRayTest.Distance))
	{
		ObjectRayTest.ObjectSection.Object = const_cast<Light*>(this);
		ObjectRayTest.ObjectSection.Section = LC_LIGHT_SECTION_TARGET;
		ObjectRayTest.Distance = Distance;
	}
}

void Light::BoxTest(lcObjectBoxTest& ObjectBoxTest) const
{
	if (IsPointLight())
	{
		for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
			if (lcDot3(mPosition, ObjectBoxTest.Planes[PlaneIdx]) + ObjectBoxTest.Planes[PlaneIdx][3] > 0.2f)
				return;

		lcObjectSection& ObjectSection = ObjectBoxTest.ObjectSections.Add();
		ObjectSection.Object = const_cast<Light*>(this);
		ObjectSection.Section = LC_LIGHT_SECTION_POSITION;

		return;
	}

	lcVector3 Min(-0.2f, -0.2f, -0.2f);
	lcVector3 Max(0.2f, 0.2f, 0.2f);

	lcVector4 LocalPlanes[6];

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		lcVector3 Normal = lcMul30(ObjectBoxTest.Planes[PlaneIdx], mWorldLight);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, ObjectBoxTest.Planes[PlaneIdx][3] - lcDot3(mWorldLight[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		lcObjectSection& ObjectSection = ObjectBoxTest.ObjectSections.Add();
		ObjectSection.Object = const_cast<Light*>(this);
		ObjectSection.Section = LC_LIGHT_SECTION_POSITION;
	}

	Min = lcVector3(-0.2f, -0.2f, -0.2f);
	Max = lcVector3(0.2f, 0.2f, 0.2f);

	lcMatrix44 WorldTarget = mWorldLight;
	WorldTarget.SetTranslation(lcMul30(-mTargetPosition, WorldTarget));

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		lcVector3 Normal = lcMul30(ObjectBoxTest.Planes[PlaneIdx], WorldTarget);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, ObjectBoxTest.Planes[PlaneIdx][3] - lcDot3(WorldTarget[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		lcObjectSection& ObjectSection = ObjectBoxTest.ObjectSections.Add();
		ObjectSection.Object = const_cast<Light*>(this);
		ObjectSection.Section = LC_LIGHT_SECTION_TARGET;
	}
}

void Light::Move(unsigned short nTime, bool bAddKey, float dx, float dy, float dz)
{
	lcVector3 MoveVec(dx, dy, dz);

	if (IsSelected(LC_LIGHT_SECTION_POSITION))
	{
		mPosition += MoveVec;

		ChangeKey(nTime, bAddKey, mPosition, LC_LK_POSITION);
	}

	if (IsSelected(LC_LIGHT_SECTION_TARGET))
	{
		mTargetPosition += MoveVec;

		ChangeKey(nTime, bAddKey, mTargetPosition, LC_LK_TARGET);
	}
}

void Light::UpdatePosition(unsigned short nTime)
{
	CalculateKeys(nTime);

	if (IsPointLight())
	{
		mWorldLight = lcMatrix44Identity();
		mWorldLight.SetTranslation(-mPosition);
	}
	else
	{
		lcVector3 FrontVector(mTargetPosition - mPosition);
		lcVector3 UpVector(1, 1, 1);

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

		mWorldLight = lcMatrix44LookAt(mPosition, mTargetPosition, UpVector);
	}
}

void Light::Render(View* View)
{
	float LineWidth = lcGetPreferences().mLineWidth;
	const lcMatrix44& ViewMatrix = View->mCamera->mWorldView;
	lcContext* Context = View->mContext;

	if (IsPointLight())
	{
		Context->SetWorldViewMatrix(lcMul(lcMatrix44Translation(mPosition), ViewMatrix));

		if (IsSelected(LC_LIGHT_SECTION_POSITION))
		{
			if (IsFocused(LC_LIGHT_SECTION_POSITION))
				lcSetColorFocused();
			else
				lcSetColorSelected();
		}
		else
			lcSetColorLight();

		RenderSphere();
	}
	else
	{
		if (IsSelected(LC_LIGHT_SECTION_POSITION))
		{
			Context->SetLineWidth(2.0f * LineWidth);
			if (IsFocused(LC_LIGHT_SECTION_POSITION))
				lcSetColorFocused();
			else
				lcSetColorSelected();
		}
		else
		{
			Context->SetLineWidth(LineWidth);
			lcSetColorLight();
		}

		RenderCone(ViewMatrix);

		if (IsSelected(LC_LIGHT_SECTION_TARGET))
		{
			Context->SetLineWidth(2.0f * LineWidth);
			if (IsFocused(LC_LIGHT_SECTION_TARGET))
				lcSetColorFocused();
			else
				lcSetColorSelected();
		}
		else
		{
			Context->SetLineWidth(LineWidth);
			lcSetColorLight();
		}

		RenderTarget();

		Context->SetWorldViewMatrix(ViewMatrix);

		Context->SetLineWidth(LineWidth);
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
			Context->SetWorldViewMatrix(lcMul(ProjectionMatrix, lcMul(LightMatrix, ViewMatrix)));

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

bool Light::Setup(int LightIndex)
{
	GLenum LightName = (GLenum)(GL_LIGHT0 + LightIndex);

	if (mState & LC_LIGHT_DISABLED)
		return false;

	glEnable(LightName);

	glLightfv(LightName, GL_AMBIENT, mAmbientColor);
	glLightfv(LightName, GL_DIFFUSE, mDiffuseColor);
	glLightfv(LightName, GL_SPECULAR, mSpecularColor);

	if (!IsDirectionalLight())
	{
		glLightf(LightName, GL_CONSTANT_ATTENUATION, mConstantAttenuation);
		glLightf(LightName, GL_LINEAR_ATTENUATION, mLinearAttenuation);
		glLightf(LightName, GL_QUADRATIC_ATTENUATION, mQuadraticAttenuation);

		lcVector4 Position(mPosition, 1.0f);
		glLightfv(LightName, GL_POSITION, Position);
	}
	else
	{
		lcVector4 Position(mPosition, 0.0f);
		glLightfv(LightName, GL_POSITION, Position);
	}

	if (IsPointLight())
	{
		glLightf(LightName, GL_SPOT_CUTOFF, 180.0f);
	}
	else if (IsSpotLight())
	{
		lcVector3 Dir(mTargetPosition - mPosition);
		Dir.Normalize();

		glLightf(LightName, GL_SPOT_CUTOFF, mSpotCutoff);
		glLightf(LightName, GL_SPOT_EXPONENT, mSpotExponent);
		glLightfv(LightName, GL_SPOT_DIRECTION, Dir);
	}

	return true;
}
