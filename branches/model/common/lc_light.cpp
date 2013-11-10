#include "lc_global.h"
#include "lc_light.h"
#include "lc_file.h"
#include "lc_colors.h"
#include "lc_application.h"

lcLight::lcLight(const lcVector3& Position) :
	lcObject(LC_OBJECT_TYPE_LIGHT)
{
	mState = 0;
	mName[0] = 0;

	mPosition = Position;
	mTargetPosition = Position;
	mAmbientColor = lcVector4(0.0f, 0.0f, 0.0f, 1.0f);
	mDiffuseColor = lcVector4(1.0f, 1.0f, 1.0f, 1.0f);
	mSpecularColor = lcVector4(1.0f, 1.0f, 1.0f, 1.0f);
	mAttenuation = lcVector3(1.0f, 0.0f, 0.0f);
	mSpotCutoff = 180.0f;
	mSpotExponent = 0.0f;
}

lcLight::~lcLight()
{
}

void lcLight::Save(lcFile& File)
{
	File.WriteFloats(mPosition, 3);
	File.WriteFloats(mTargetPosition, 3);
}

void lcLight::Load(lcFile& File)
{
	File.ReadFloats(mPosition, 3);
	File.ReadFloats(mTargetPosition, 3);
}

void lcLight::Update()
{
	if (IsPointLight())
	{
		mWorldLight = lcMatrix44Identity();
		mWorldLight.SetTranslation(-mPosition);

		return;
	}

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

void lcLight::ClosestHitTest(lcObjectHitTest& HitTest)
{
	if (HitTest.PiecesOnly)
		return;

	if (IsPointLight())
	{
		float Distance;

		if (lcSphereRayMinIntersectDistance(mPosition, 0.2f, HitTest.Start, HitTest.End, &Distance))
		{
			HitTest.ObjectSection.Object = this;
			HitTest.ObjectSection.Section = LC_LIGHT_POSITION;
			HitTest.Distance = Distance;
		}

		return;
	}

	lcVector3 Min = lcVector3(-0.2f, -0.2f, -0.2f);
	lcVector3 Max = lcVector3(0.2f, 0.2f, 0.2f);

	lcVector3 Start = lcMul31(HitTest.Start, mWorldLight);
	lcVector3 End = lcMul31(HitTest.End, mWorldLight);

	float Distance;
	if (lcBoundingBoxRayMinIntersectDistance(Min, Max, Start, End, &Distance, NULL) && (Distance < HitTest.Distance))
	{
		HitTest.ObjectSection.Object = this;
		HitTest.ObjectSection.Section = LC_LIGHT_POSITION;
		HitTest.Distance = Distance;
	}

	Min = lcVector3(-0.2f, -0.2f, -0.2f);
	Max = lcVector3(0.2f, 0.2f, 0.2f);

	lcMatrix44 WorldTarget = mWorldLight;
	WorldTarget.SetTranslation(lcMul30(-mTargetPosition, WorldTarget));

	Start = lcMul31(HitTest.Start, WorldTarget);
	End = lcMul31(HitTest.End, WorldTarget);

	if (lcBoundingBoxRayMinIntersectDistance(Min, Max, Start, End, &Distance, NULL) && (Distance < HitTest.Distance))
	{
		HitTest.ObjectSection.Object = this;
		HitTest.ObjectSection.Section = LC_LIGHT_TARGET;
		HitTest.Distance = Distance;
	}
}

void lcLight::BoxTest(lcObjectBoxTest& BoxTest)
{
	if (IsPointLight())
	{
		for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
			if (lcDot3(mPosition, BoxTest.Planes[PlaneIdx]) + BoxTest.Planes[PlaneIdx][3] > 0.2f)
				return;

		lcObjectSection& ObjectSection = BoxTest.ObjectSections.Add();
		ObjectSection.Object = this;
		ObjectSection.Section = LC_LIGHT_POSITION;

		return;
	}

	lcVector3 Min(-0.2f, -0.2f, -0.2f);
	lcVector3 Max(0.2f, 0.2f, 0.2f);

	lcVector4 LocalPlanes[6];

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		lcVector3 Normal = lcMul30(BoxTest.Planes[PlaneIdx], mWorldLight);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, BoxTest.Planes[PlaneIdx][3] - lcDot3(mWorldLight[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		lcObjectSection& ObjectSection = BoxTest.ObjectSections.Add();
		ObjectSection.Object = this;
		ObjectSection.Section = LC_LIGHT_POSITION;
	}

	Min = lcVector3(-0.2f, -0.2f, -0.2f);
	Max = lcVector3(0.2f, 0.2f, 0.2f);

	lcMatrix44 WorldTarget = mWorldLight;
	WorldTarget.SetTranslation(lcMul30(-mTargetPosition, WorldTarget));

	for (int PlaneIdx = 0; PlaneIdx < 6; PlaneIdx++)
	{
		lcVector3 Normal = lcMul30(BoxTest.Planes[PlaneIdx], WorldTarget);
		LocalPlanes[PlaneIdx] = lcVector4(Normal, BoxTest.Planes[PlaneIdx][3] - lcDot3(WorldTarget[3], Normal));
	}

	if (lcBoundingBoxIntersectsVolume(Min, Max, LocalPlanes))
	{
		lcObjectSection& ObjectSection = BoxTest.ObjectSections.Add();
		ObjectSection.Object = this;
		ObjectSection.Section = LC_LIGHT_TARGET;
	}
}

void lcLight::GetRenderMeshes(View* View, lcArray<lcRenderMesh>& OpaqueMeshes, lcArray<lcRenderMesh>& TranslucentMeshes, lcArray<lcObject*>& InterfaceObjects)
{
	if (!IsVisible())
		return;

	InterfaceObjects.Add(this);
}

void lcLight::RenderInterface(View* View) const
{
	float LineWidth = lcGetPreferences()->mLineWidth;

	if (IsPointLight())
	{
		glPushMatrix();
		glTranslatef(mPosition[0], mPosition[1], mPosition[2]);

		if (mState & LC_LIGHT_POSITION_SELECTED)
		{
			glLineWidth(2.0f * LineWidth);

			if (mState & LC_LIGHT_POSITION_FOCUSED)
				lcSetColorFocused();
			else
				lcSetColorSelected();
		}
		else
		{
			glLineWidth(LineWidth);
			lcSetColorLight();
		}

		DrawPointLight();

		glPopMatrix();

		glLineWidth(LineWidth);
		return;
	}

	lcMatrix44 ViewLight = lcMatrix44AffineInverse(mWorldLight);

	// Light.
	glPushMatrix();

	if (mState & LC_LIGHT_POSITION_SELECTED)
	{
		glLineWidth(2.0f * LineWidth);

		if (mState & LC_LIGHT_POSITION_FOCUSED)
			lcSetColorFocused();
		else
			lcSetColorSelected();
	}
	else
	{
		glLineWidth(LineWidth);
		lcSetColorLight();
	}

	glTranslatef(mPosition[0], mPosition[1], mPosition[2]);

	lcMatrix44 LightMatrix = ViewLight;
	LightMatrix.SetTranslation(lcVector3(0, 0, 0));

	glMultMatrixf(LightMatrix);

	DrawSpotLight();

	if (mState & LC_LIGHT_TARGET_SELECTED)
	{
		glLineWidth(2.0f * LineWidth);

		if (mState & LC_LIGHT_TARGET_FOCUSED)
			lcSetColorFocused();
		else
			lcSetColorSelected();
	}
	else
	{
		glLineWidth(LineWidth);
		lcSetColorLight();
	}

	// Target
	float Box[24][3] =
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

	lcVector3 FrontVector(mTargetPosition - mPosition);
	float Length = FrontVector.Length();

	glTranslatef(0, 0, -Length);

	glVertexPointer(3, GL_FLOAT, 0, Box);
	glDrawArrays(GL_LINES, 0, 24);

	glPopMatrix();

	glLineWidth(LineWidth);
	lcSetColorLight();

	lcVector3 Line[2] = { mPosition, mTargetPosition };
	glVertexPointer(3, GL_FLOAT, 0, Line);

	glDrawArrays(GL_LINES, 0, 2);

	if (IsSelected())
	{
		glPushMatrix();
		glMultMatrixf(ViewLight);

		lcMatrix44 Projection = lcMatrix44Perspective(2 * mSpotCutoff, 1.0f, 0.01f, Length);
		Projection = lcMatrix44Inverse(Projection);
		glMultMatrixf(Projection);

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

		glPopMatrix();
	}
}

void lcLight::Move(const lcVector3& Distance, lcTime Time, bool AddKeys)
{
	if (mState & LC_LIGHT_POSITION_SELECTED)
	{
		mPosition += Distance;

		ChangeKey(mPositionKeys, mPosition, Time, AddKeys);
	}

	if (mState & LC_LIGHT_TARGET_SELECTED)
	{
		mTargetPosition += Distance;

		ChangeKey(mTargetPositionKeys, mTargetPosition, Time, AddKeys);
	}
}

void lcLight::DrawPointLight() const
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

void lcLight::DrawSpotLight() const
{
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
