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

#define LC_LIGHT_POSITION_EDGE 7.5f
#define LC_LIGHT_TARGET_EDGE 5.0f
#define LC_LIGHT_SPHERE_RADIUS 5.0f

// New omni light.
lcLight::lcLight(float px, float py, float pz)
	: lcObject(LC_OBJECT_LIGHT)
{
	Initialize(lcVector3(px, py, pz), lcVector3(0.0f, 0.0f, 0.0f));
	UpdatePosition(1);
}

// New directional or spot light.
lcLight::lcLight(float px, float py, float pz, float tx, float ty, float tz)
	: lcObject(LC_OBJECT_LIGHT)
{
	Initialize(lcVector3(px, py, pz), lcVector3(tx, ty, tz));
	mState |= LC_LIGHT_SPOT;
	UpdatePosition(1);
}

void lcLight::Initialize(const lcVector3& Position, const lcVector3& TargetPosition)
{
	mState = 0;
	memset(m_strName, 0, sizeof(m_strName));

	ChangeKey(mPositionKeys, Position, 1, true);
	ChangeKey(mTargetPositionKeys, TargetPosition, 1, true);
	ChangeKey(mAmbientColorKeys, lcVector4(0.0f, 0.0f, 0.0f, 1.0f), 1, true);
	ChangeKey(mDiffuseColorKeys, lcVector4(0.8f, 0.8f, 0.8f, 1.0f), 1, true);
	ChangeKey(mSpecularColorKeys, lcVector4(1.0f, 1.0f, 1.0f, 1.0f), 1, true);
	ChangeKey(mAttenuationKeys, lcVector3(1.0f, 0.0f, 0.0f), 1, true);
	ChangeKey(mSpotCutoffKeys, 30.0f, 1, true);
	ChangeKey(mSpotExponentKeys, 0.0f, 1, true);
}

lcLight::~lcLight()
{
}

void lcLight::SaveLDraw(QTextStream& Stream) const
{
}

void lcLight::CreateName(const lcArray<lcLight*>& Lights)
{
	if (m_strName[0])
	{
		bool Found = false;
		for (int LightIdx = 0; LightIdx < Lights.GetSize(); LightIdx++)
		{
			if (!strcmp(Lights[LightIdx]->m_strName, m_strName))
			{
				Found = true;
				break;
			}
		}

		if (!Found)
			return;
	}

	int i, max = 0;

	for (int LightIdx = 0; LightIdx < Lights.GetSize(); LightIdx++)
	{
		lcLight* pLight = Lights[LightIdx];

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

void lcLight::CompareBoundingBox(float box[6])
{
	const lcVector3 Points[2] =
	{
		mPosition, mTargetPosition
	};

	for (int i = 0; i < (IsPointLight() ? 1 : 2); i++)
	{
		const lcVector3& Point = Points[i];

		if (Point[0] < box[0]) box[0] = Point[0];
		if (Point[1] < box[1]) box[1] = Point[1];
		if (Point[2] < box[2]) box[2] = Point[2];
		if (Point[0] > box[3]) box[3] = Point[0];
		if (Point[1] > box[4]) box[4] = Point[1];
		if (Point[2] > box[5]) box[5] = Point[2];
	}
}

void lcLight::RayTest(lcObjectRayTest& ObjectRayTest) const
{
	if (IsPointLight())
	{
		float Distance;

		if (lcSphereRayMinIntersectDistance(mPosition, LC_LIGHT_SPHERE_RADIUS, ObjectRayTest.Start, ObjectRayTest.End, &Distance))
		{
			ObjectRayTest.ObjectSection.Object = const_cast<lcLight*>(this);
			ObjectRayTest.ObjectSection.Section = LC_LIGHT_SECTION_POSITION;
			ObjectRayTest.Distance = Distance;
		}

		return;
	}

	lcVector3 Min = lcVector3(-LC_LIGHT_POSITION_EDGE, -LC_LIGHT_POSITION_EDGE, -LC_LIGHT_POSITION_EDGE);
	lcVector3 Max = lcVector3(LC_LIGHT_POSITION_EDGE, LC_LIGHT_POSITION_EDGE, LC_LIGHT_POSITION_EDGE);

	lcVector3 Start = lcMul31(ObjectRayTest.Start, mWorldLight);
	lcVector3 End = lcMul31(ObjectRayTest.End, mWorldLight);

	float Distance;
	if (lcBoundingBoxRayIntersectDistance(Min, Max, Start, End, &Distance, NULL) && (Distance < ObjectRayTest.Distance))
	{
		ObjectRayTest.ObjectSection.Object = const_cast<lcLight*>(this);
		ObjectRayTest.ObjectSection.Section = LC_LIGHT_SECTION_POSITION;
		ObjectRayTest.Distance = Distance;
	}

	Min = lcVector3(-LC_LIGHT_TARGET_EDGE, -LC_LIGHT_TARGET_EDGE, -LC_LIGHT_TARGET_EDGE);
	Max = lcVector3(LC_LIGHT_TARGET_EDGE, LC_LIGHT_TARGET_EDGE, LC_LIGHT_TARGET_EDGE);

	lcMatrix44 WorldTarget = mWorldLight;
	WorldTarget.SetTranslation(lcMul30(-mTargetPosition, WorldTarget));

	Start = lcMul31(ObjectRayTest.Start, WorldTarget);
	End = lcMul31(ObjectRayTest.End, WorldTarget);

	if (lcBoundingBoxRayIntersectDistance(Min, Max, Start, End, &Distance, NULL) && (Distance < ObjectRayTest.Distance))
	{
		ObjectRayTest.ObjectSection.Object = const_cast<lcLight*>(this);
		ObjectRayTest.ObjectSection.Section = LC_LIGHT_SECTION_TARGET;
		ObjectRayTest.Distance = Distance;
	}
}

void lcLight::BoxTest(lcObjectBoxTest& ObjectBoxTest) const
{
	if (IsPointLight())
	{
		for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
			if (lcDot3(mPosition, ObjectBoxTest.Planes[PlaneIdx]) + ObjectBoxTest.Planes[PlaneIdx][3] > LC_LIGHT_SPHERE_RADIUS)
				return;

		ObjectBoxTest.Objects.Add(const_cast<lcLight*>(this));
		return;
	}

	lcVector3 Min(-LC_LIGHT_POSITION_EDGE, -LC_LIGHT_POSITION_EDGE, -LC_LIGHT_POSITION_EDGE);
	lcVector3 Max(LC_LIGHT_POSITION_EDGE, LC_LIGHT_POSITION_EDGE, LC_LIGHT_POSITION_EDGE);

	lcVector4 LocalPlanes[6];

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		lcVector3 Normal = lcMul30(ObjectBoxTest.Planes[PlaneIdx], mWorldLight);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, ObjectBoxTest.Planes[PlaneIdx][3] - lcDot3(mWorldLight[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		ObjectBoxTest.Objects.Add(const_cast<lcLight*>(this));
		return;
	}

	Min = lcVector3(-LC_LIGHT_TARGET_EDGE, -LC_LIGHT_TARGET_EDGE, -LC_LIGHT_TARGET_EDGE);
	Max = lcVector3(LC_LIGHT_TARGET_EDGE, LC_LIGHT_TARGET_EDGE, LC_LIGHT_TARGET_EDGE);

	lcMatrix44 WorldTarget = mWorldLight;
	WorldTarget.SetTranslation(lcMul30(-mTargetPosition, WorldTarget));

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		lcVector3 Normal = lcMul30(ObjectBoxTest.Planes[PlaneIdx], WorldTarget);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, ObjectBoxTest.Planes[PlaneIdx][3] - lcDot3(WorldTarget[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		ObjectBoxTest.Objects.Add(const_cast<lcLight*>(this));
		return;
	}
}

void lcLight::Move(lcStep Step, bool AddKey, const lcVector3& Distance)
{
	if (IsSelected(LC_LIGHT_SECTION_POSITION))
	{
		mPosition += Distance;
		ChangeKey(mPositionKeys, mPosition, Step, AddKey);
	}

	if (IsSelected(LC_LIGHT_SECTION_TARGET))
	{
		mTargetPosition += Distance;
		ChangeKey(mTargetPositionKeys, mTargetPosition, Step, AddKey);
	}
}

void lcLight::InsertTime(lcStep Start, lcStep Time)
{
	lcObject::InsertTime(mPositionKeys, Start, Time);
	lcObject::InsertTime(mTargetPositionKeys, Start, Time);
	lcObject::InsertTime(mAmbientColorKeys, Start, Time);
	lcObject::InsertTime(mDiffuseColorKeys, Start, Time);
	lcObject::InsertTime(mSpecularColorKeys, Start, Time);
	lcObject::InsertTime(mAttenuationKeys, Start, Time);
	lcObject::InsertTime(mSpotCutoffKeys, Start, Time);
	lcObject::InsertTime(mSpotExponentKeys, Start, Time);
}

void lcLight::RemoveTime(lcStep Start, lcStep Time)
{
	lcObject::RemoveTime(mPositionKeys, Start, Time);
	lcObject::RemoveTime(mTargetPositionKeys, Start, Time);
	lcObject::RemoveTime(mAmbientColorKeys, Start, Time);
	lcObject::RemoveTime(mDiffuseColorKeys, Start, Time);
	lcObject::RemoveTime(mSpecularColorKeys, Start, Time);
	lcObject::RemoveTime(mAttenuationKeys, Start, Time);
	lcObject::RemoveTime(mSpotCutoffKeys, Start, Time);
	lcObject::RemoveTime(mSpotExponentKeys, Start, Time);
}

void lcLight::UpdatePosition(lcStep Step)
{
	mPosition = CalculateKey(mPositionKeys, Step);
	mTargetPosition = CalculateKey(mTargetPositionKeys, Step);
	mAmbientColor = CalculateKey(mAmbientColorKeys, Step);
	mDiffuseColor = CalculateKey(mDiffuseColorKeys, Step);
	mSpecularColor = CalculateKey(mSpecularColorKeys, Step);
	mAttenuation = CalculateKey(mAttenuationKeys, Step);
	mSpotCutoff = CalculateKey(mSpotCutoffKeys, Step);
	mSpotExponent = CalculateKey(mSpotExponentKeys, Step);

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

void lcLight::DrawInterface(lcContext* Context, const lcMatrix44& ViewMatrix) const
{
	if (IsPointLight())
		DrawPointLight(Context, ViewMatrix);
	else
		DrawSpotLight(Context, ViewMatrix);
}

void lcLight::DrawSpotLight(lcContext* Context, const lcMatrix44& ViewMatrix) const
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

	lcMatrix44 LightMatrix = lcMatrix44LookAt(mPosition, mTargetPosition, UpVector);
	LightMatrix = lcMatrix44AffineInverse(LightMatrix);
	LightMatrix.SetTranslation(lcVector3(0, 0, 0));

	lcMatrix44 LightViewMatrix = lcMul(LightMatrix, lcMul(lcMatrix44Translation(mPosition), ViewMatrix));
	Context->SetWorldViewMatrix(LightViewMatrix);

	float Verts[(20 + 8 + 2 + 16) * 3];
	float* CurVert = Verts;

	for (int EdgeIdx = 0; EdgeIdx < 8; EdgeIdx++)
	{
		float c = cosf((float)EdgeIdx / 4 * LC_PI) * LC_LIGHT_POSITION_EDGE;
		float s = sinf((float)EdgeIdx / 4 * LC_PI) * LC_LIGHT_POSITION_EDGE;

		*CurVert++ = c;
		*CurVert++ = s;
		*CurVert++ = LC_LIGHT_POSITION_EDGE;
		*CurVert++ = c;
		*CurVert++ = s;
		*CurVert++ = -LC_LIGHT_POSITION_EDGE;
	}

	*CurVert++ = -12.5f; *CurVert++ = -12.5f; *CurVert++ = -LC_LIGHT_POSITION_EDGE;
	*CurVert++ =  12.5f; *CurVert++ = -12.5f; *CurVert++ = -LC_LIGHT_POSITION_EDGE;
	*CurVert++ =  12.5f; *CurVert++ =  12.5f; *CurVert++ = -LC_LIGHT_POSITION_EDGE;
	*CurVert++ = -12.5f; *CurVert++ =  12.5f; *CurVert++ = -LC_LIGHT_POSITION_EDGE;

	float Length = FrontVector.Length();

	*CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE - Length;
	*CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE - Length;
	*CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE - Length;
	*CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE - Length;
	*CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE - Length;
	*CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE - Length;
	*CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE - Length;
	*CurVert++ =  LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE; *CurVert++ = -LC_LIGHT_TARGET_EDGE - Length;

	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = 0.0f;
	*CurVert++ = 0.0f; *CurVert++ = 0.0f; *CurVert++ = -Length;

	const GLushort Indices[56 + 24 + 2 + 40] = 
	{
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14, 0,
		1, 3, 3, 5, 5, 7, 7, 9, 9, 11, 11, 13, 13, 15, 15, 1,
		16, 17, 17, 18, 18, 19, 19, 16,
		20, 21, 21, 22, 22, 23, 23, 20,
		24, 25, 25, 26, 26, 27, 27, 24,
		20, 24, 21, 25, 22, 26, 23, 27,
		28, 29,
		30, 31, 31, 32, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37, 38,
		38, 39, 39, 40, 40, 41, 41, 42, 42, 43, 43, 44, 44, 45, 45, 30,
		28, 30, 28, 34, 28, 38, 28, 42
	};

	Context->SetVertexBufferPointer(Verts);
	Context->SetVertexFormat(0, 3, 0, 0);

	float LineWidth = lcGetPreferences().mLineWidth;

	if (!IsSelected())
	{
		Context->SetLineWidth(LineWidth);
		Context->SetInterfaceColor(LC_COLOR_LIGHT);

		glDrawElements(GL_LINES, 56 + 24 + 2, GL_UNSIGNED_SHORT, Indices);
	}
	else
	{
		if (IsSelected(LC_LIGHT_SECTION_POSITION))
		{
			Context->SetLineWidth(2.0f * LineWidth);
			if (IsFocused(LC_LIGHT_SECTION_POSITION))
				Context->SetInterfaceColor(LC_COLOR_FOCUSED);
			else
				Context->SetInterfaceColor(LC_COLOR_SELECTED);
		}
		else
		{
			Context->SetLineWidth(LineWidth);
			Context->SetInterfaceColor(LC_COLOR_LIGHT);
		}

		glDrawElements(GL_LINES, 56, GL_UNSIGNED_SHORT, Indices);

		if (IsSelected(LC_LIGHT_SECTION_TARGET))
		{
			Context->SetLineWidth(2.0f * LineWidth);
			if (IsFocused(LC_LIGHT_SECTION_TARGET))
				Context->SetInterfaceColor(LC_COLOR_FOCUSED);
			else
				Context->SetInterfaceColor(LC_COLOR_SELECTED);
		}
		else
		{
			Context->SetLineWidth(LineWidth);
			Context->SetInterfaceColor(LC_COLOR_LIGHT);
		}

		glDrawElements(GL_LINES, 24, GL_UNSIGNED_SHORT, Indices + 56);

		Context->SetLineWidth(LineWidth);
		Context->SetInterfaceColor(LC_COLOR_LIGHT);

		float Radius = tanf(LC_DTOR * mSpotCutoff) * Length;

		for (int EdgeIdx = 0; EdgeIdx < 16; EdgeIdx++)
		{
			*CurVert++ = cosf((float)EdgeIdx / 16 * LC_2PI) * Radius;
			*CurVert++ = sinf((float)EdgeIdx / 16 * LC_2PI) * Radius;
			*CurVert++ = -Length;
		}

		glDrawElements(GL_LINES, 2 + 40, GL_UNSIGNED_SHORT, Indices + 56 + 24);
	}

	Context->ClearVertexBuffer(); // context remove
}

void lcLight::DrawPointLight(lcContext* Context, const lcMatrix44& ViewMatrix) const
{
	const int Slices = 6;
	const int NumIndices = 3 * Slices + 6 * Slices * (Slices - 2) + 3 * Slices;
	const int NumVertices = (Slices - 1) * Slices + 2;
	const float Radius = LC_LIGHT_SPHERE_RADIUS;
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

	Context->SetWorldViewMatrix(lcMul(lcMatrix44Translation(mPosition), ViewMatrix));

	if (IsFocused(LC_LIGHT_SECTION_POSITION))
		Context->SetInterfaceColor(LC_COLOR_FOCUSED);
	else if (IsSelected(LC_LIGHT_SECTION_POSITION))
		Context->SetInterfaceColor(LC_COLOR_SELECTED);
	else
		Context->SetInterfaceColor(LC_COLOR_LIGHT);

	Context->SetVertexBufferPointer(Vertices);
	Context->SetVertexFormat(0, 3, 0, 0);
	glDrawElements(GL_TRIANGLES, NumIndices, GL_UNSIGNED_SHORT, Indices);
}

bool lcLight::Setup(int LightIndex)
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
		glLightf(LightName, GL_CONSTANT_ATTENUATION, mAttenuation[0]);
		glLightf(LightName, GL_LINEAR_ATTENUATION, mAttenuation[1]);
		glLightf(LightName, GL_QUADRATIC_ATTENUATION, mAttenuation[2]);

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
