#include "lc_global.h"
#include "lc_camera.h"
#include "lc_camera_target.h"

#include "lc_colors.h"
#include "opengl.h"

lcCameraTarget::lcCameraTarget(lcCamera* Parent)
	: lcObject(LC_OBJECT_CAMERA_TARGET, LC_CAMERA_TARGET_NUMKEYS)
{
	m_Parent = Parent;

	m_Name = m_Parent->m_Name + ".Target";
}

lcCameraTarget::~lcCameraTarget()
{
}

void lcCameraTarget::Update(u32 Time)
{
	CalculateKey(Time, LC_CAMERA_TARGET_POSITION, &m_ParentPosition);
	m_WorldPosition = m_ParentPosition;
}

void lcCameraTarget::Render()
{
	// Draw target box.
	glPushMatrix();
	Matrix44 TargetMat = ((lcCamera*)m_Parent)->m_ViewWorld;
	TargetMat.SetTranslation(m_WorldPosition);
	glMultMatrixf(TargetMat);

	glEnableClientState(GL_VERTEX_ARRAY);
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

	if (IsSelected())
	{
		glLineWidth(2.0f);
		glColor3fv(lcColorList[IsFocused() ? LC_COLOR_FOCUS : LC_COLOR_SELECTION].Value);
	}
	else
	{
		glColor3f(0.5f, 0.8f, 0.5f);
	}

	glVertexPointer(3, GL_FLOAT, 0, Box);
	glDrawArrays(GL_LINES, 0, 24);

	glPopMatrix();
}

void lcCameraTarget::AddToScene(lcScene* Scene, int Color)
{
}

void lcCameraTarget::ClosestRayIntersect(LC_CLICK_RAY* Ray) const
{
	BoundingBox Box;
	Box.m_Max = Vector3(0.2f, 0.2f, 0.2f);
	Box.m_Min = Vector3(-0.2f, -0.2f, -0.2f);

	Matrix44 WorldView = ((lcCamera*)m_Parent)->m_WorldView;
	WorldView.SetTranslation(Mul30(-m_WorldPosition, WorldView));

	Vector3 Start = Mul31(Ray->Start, WorldView);
	Vector3 End = Mul31(Ray->End, WorldView);

	float Dist;
	if (BoundingBoxRayMinIntersectDistance(Box, Start, End, &Dist) && (Dist < Ray->Dist))
	{
		Ray->Object = this;
		Ray->Dist = Dist;
	}
}

bool lcCameraTarget::IntersectsVolume(const Vector4* Planes, int NumPlanes) const
{
	BoundingBox Box;
	Box.m_Max = Vector3(0.2f, 0.2f, 0.2f);
	Box.m_Min = Vector3(-0.2f, -0.2f, -0.2f);

	// Transform the planes to local space.
	Vector4* LocalPlanes = new Vector4[NumPlanes];
	int i;

	Matrix44 WorldView = ((lcCamera*)m_Parent)->m_WorldView;
	WorldView.SetTranslation(Mul30(-m_WorldPosition, WorldView));

	for (i = 0; i < NumPlanes; i++)
	{
		LocalPlanes[i] = Vector4(Mul30(Vector3(Planes[i]), WorldView));
		LocalPlanes[i][3] = Planes[i][3] - Dot3(Vector3(WorldView[3]), Vector3(LocalPlanes[i]));
	}

	bool Intersect = BoundingBoxIntersectsVolume(Box, LocalPlanes, NumPlanes);

	delete[] LocalPlanes;

	return Intersect;
}
