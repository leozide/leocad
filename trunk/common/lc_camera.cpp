#include "lc_global.h"
#include "lc_camera.h"
#include "lc_camera_target.h"

#include "globals.h"
#include "opengl.h"

lcCamera::lcCamera()
	: lcObject(LC_OBJECT_CAMERA, LC_CAMERA_NUMKEYS)
{
	m_CameraType = -1;
}

lcCamera::~lcCamera()
{
	delete m_Children;
}

void lcCamera::CreateCamera(int CameraType, bool Target)
{
	LC_ASSERT(m_CameraType == -1, "Trying to initialize a camera more than once.");
	LC_ASSERT(CameraType >= 0 && CameraType <= LC_CAMERA_USER, "Invalid camera type.");

	m_CameraType = CameraType;

	const char* CameraNames[] = { "Front", "Back",  "Top",  "Under", "Left", "Right", "Main", "User" };
	m_Name = CameraNames[CameraType];

	m_Children = new lcCameraTarget(this);

	Vector3 CameraPos[] = { Vector3(50, 0, 0), Vector3(-50, 0, 0), Vector3(0, 0, 50), Vector3(0, 0, -50),
	                        Vector3(0, 50, 0), Vector3(0, -50, 0), Vector3(-10, -10, 5), Vector3(-10, -10, 5) };

	ChangeKey(1, true, LC_CAMERA_POSITION, CameraPos[CameraType]);
	ChangeKey(1, true, LC_CAMERA_ROLL, 0.0f);

	m_NearDist = 1.0f;
	m_FarDist = 500.0f;
	m_FOV = 30.0f; // TODO: animate FOV

	SetVisible(CameraType == LC_CAMERA_USER);

	Update(1);
}

void lcCamera::ClosestRayIntersect(LC_CLICK_RAY* Ray) const
{
	BoundingBox Box;
	Box.m_Max = Vector3(0.3f, 0.3f, 0.3f);
	Box.m_Min = Vector3(-0.3f, -0.3f, -0.3f);

	Vector3 Start = Mul31(Ray->Start, m_WorldView);
	Vector3 End = Mul31(Ray->End, m_WorldView);

	float Dist;
	if (BoundingBoxRayMinIntersectDistance(Box, Start, End, &Dist) && (Dist < Ray->Dist))
	{
		Ray->Object = this;
		Ray->Dist = Dist;
	}

	m_Children->ClosestRayIntersect(Ray);
}

bool lcCamera::IntersectsVolume(const Vector4* Planes, int NumPlanes) const
{
	BoundingBox Box;
	Box.m_Max = Vector3(0.3f, 0.3f, 0.3f);
	Box.m_Min = Vector3(-0.3f, -0.3f, -0.3f);

	// Transform the planes to local space.
	Vector4* LocalPlanes = new Vector4[NumPlanes];
	int i;

	for (i = 0; i < NumPlanes; i++)
	{
		LocalPlanes[i] = Vector4(Mul30(Vector3(Planes[i]), m_WorldView));
		LocalPlanes[i][3] = Planes[i][3] - Dot3(Vector3(m_WorldView[3]), Vector3(LocalPlanes[i]));
	}

	bool Intersect = BoundingBoxIntersectsVolume(Box, LocalPlanes, NumPlanes);

	delete[] LocalPlanes;

	if (!Intersect)
		Intersect = m_Children->IntersectsVolume(Planes, NumPlanes);

	return Intersect;
}

void lcCamera::Update(u32 Time)
{
	// Update key values.
	CalculateKey(Time, LC_CAMERA_POSITION, &m_Position);
	CalculateKey(Time, LC_CAMERA_ROLL, &m_Roll);

	m_Children->Update(Time);

	Vector3 Z = Normalize(m_Position - m_Children->m_Position);

	// Build the Y vector of the matrix.
	Vector3 UpVector;

	if (fabsf(Z[0]) < 0.001f && fabsf(Z[1]) < 0.001f)
		UpVector = Vector3(-Z[2], 0, 0);
	else
		UpVector = Vector3(0, 0, 1);

	// Calculate X vector.
	Vector3 X = Cross3(UpVector, Z);

	// Calculate real Y vector.
	Vector3 Y = Cross3(Z, X);

	// Apply the roll rotation and recalculate X and Y.
	Matrix33 RollMat = MatrixFromAxisAngle(Z, m_Roll);
	Y = Normalize(Mul(Y, RollMat));
	X = Normalize(Cross3(Y, Z));

	// Build matrices.
	Vector4 Row0 = Vector4(X[0], Y[0], Z[0], 0.0f);
	Vector4 Row1 = Vector4(X[1], Y[1], Z[1], 0.0f);
	Vector4 Row2 = Vector4(X[2], Y[2], Z[2], 0.0f);
	Vector4 Row3 = Vector4(Vector3(Row0 * -m_Position[0] + Row1 * -m_Position[1] + Row2 * -m_Position[2]), 1.0f);

	m_WorldView = Matrix44(Row0, Row1, Row2, Row3);
	m_ViewWorld = RotTranInverse(m_WorldView);
}

void lcCamera::Render()
{
	// Draw camera.
	glPushMatrix();
	glMultMatrixf(m_ViewWorld);

	glEnableClientState(GL_VERTEX_ARRAY);
	float verts[34][3] =
	{
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
		{  0.3f,  0.3f, -0.6f }, { -0.3f,  0.3f, -0.6f }
	};

	if (IsSelected())
	{
		glLineWidth(2.0f);
		glColor3ubv(FlatColorArray[IsFocused() ? LC_COL_FOCUSED : LC_COL_SELECTED]);
	}
	else
	{
		glLineWidth(1.0f);
		glColor3f(0.5f, 0.8f, 0.5f);
	}

	glVertexPointer(3, GL_FLOAT, 0, verts);
	glDrawArrays(GL_LINES, 0, 24);
	glDrawArrays(GL_LINE_STRIP, 24, 10);

	glPopMatrix();

	// Draw target box.
	((lcCameraTarget*)m_Children)->Render();

	// Draw a line from the camera to the target.
	glLineWidth(1.0f);
	glColor3f(0.5f, 0.8f, 0.5f);
	glBegin(GL_LINES);
	glVertex3fv(m_Position);
	glVertex3fv(m_Children->m_Position);
	glEnd();

	// Draw the view frustum.
	if (IsSelected() || m_Children->IsSelected() || IsFlagged(LC_CAMERA_SHOW_CONE))
	{
		glPushMatrix();
		glMultMatrixf(m_ViewWorld);

		float Dist = Length(m_Children->m_Position - m_Position);
		Matrix44 Projection;
		Projection = CreatePerspectiveMatrix(m_FOV, 1.33f, 0.01f, Dist);
		Projection = Inverse(Projection);
		glMultMatrixf(Projection);

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

void lcCamera::AddToScene(lcScene* Scene, const Matrix44& ParentWorld, int Color)
{
}

void lcCamera::Move(u32 Time, bool AddKey, const Vector3& Delta)
{
	if (IsSelected())
		lcObject::Move(Time, AddKey, Delta);

	if (m_Children->IsSelected())
		m_Children->Move(Time, AddKey, Delta);

	Update(Time);
}

void lcCamera::SetRoll(u32 Time, bool AddKey, const float NewRoll)
{
	ChangeKey(Time, AddKey, LC_CAMERA_ROLL, NewRoll);
}

void lcCamera::Roll(u32 Time, bool AddKey, float MouseX, float MouseY)
{
	float NewRoll = m_Roll + MouseX / 100;
	SetRoll(Time, AddKey, NewRoll);

	Update(Time);
}







// ============================================================================
// Everything from now on needs to be rewritten

void lcCamera::Zoom(float MouseY, u32 Time, bool AddKey)
{
	if (m_Flags & LC_CAMERA_ORTHOGRAPHIC)
	{
		// TODO: have a different option to change the FOV.
		m_FOV += MouseY;
		m_FOV = lcClamp(m_FOV, 0.001f, 179.999f);
	}
	else
	{
		Vector3 Delta = Vector3(m_ViewWorld[2]) * MouseY;

		// TODO: option to move eye, target or both
		if (m_Children)
			m_Children->Move(Time, AddKey, Delta);

		Move(Time, AddKey, Delta);
	}
}

void lcCamera::Pan(float MouseX, float MouseY, u32 Time, bool AddKey)
{
	Vector3 Delta = Vector3(m_ViewWorld[0]) * -MouseX + Vector3(m_ViewWorld[1]) * -MouseY;

	if (m_Children)
		m_Children->SetPosition(Time, AddKey, m_Children->m_Position + Delta);

	SetPosition(Time, AddKey, m_Position + Delta);
}

void lcCamera::Rotate(float MouseX, float MouseY, u32 Time, bool AddKey)
{
	Vector3 Target;

	if (m_Children)
		Target = m_Children->m_Position;
	else
	{
		// FIXME: Camera rotation center.
		Target = Vector3(0, 0, 0);
//		Target = lcGetActiveProject()->GetActiveModel()->GetCenter();

		Vector4 Plane;
		Plane = Vector4(Target - m_Position);
		Plane[3] = -Dot3(Plane, Target);

		Vector3 Intersection;
		LinePlaneIntersection(&Intersection, m_Position, Target, Plane);
	}

	Vector3 Dir = m_Position - Target;

	// The X axis of the mouse always corresponds to Z in the world.
	if (fabsf(MouseX) > 0.01f)
	{
		float AngleX = -MouseX * LC_DTOR;
		Matrix33 RotX = MatrixFromAxisAngle(Vector4(0, 0, 1, AngleX));

		Dir = Mul(Dir, RotX);
	}

	// The Y axis will the side vector of the camera.
	if (fabsf(MouseY) > 0.01f)
	{
		float AngleY = MouseY * LC_DTOR;
		Matrix33 RotY = MatrixFromAxisAngle(Vector4(m_WorldView[0][0], m_WorldView[1][0], m_WorldView[2][0], AngleY));

		Dir = Mul(Dir, RotY);
	}

	Move(Time, AddKey, Dir);
}

// FIXME: Move this to the View class, or remove.
void lcCamera::GetFrustumPlanes(float Aspect, Vector4 Planes[6]) const
{
	Matrix44 Projection = CreatePerspectiveMatrix(m_FOV, Aspect, m_NearDist, m_FarDist);
	::GetFrustumPlanes(m_WorldView, Projection, Planes);
}

void lcCamera::LoadProjection(float Aspect)
{
	// FIXME: tiled rendering
//	if (m_pTR != NULL)
//		m_pTR->BeginTile();
//	else
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		if (IsOrtho())
		{
			float ymax, ymin, xmin, xmax, znear, zfar;
			Vector3 frontvec = Vector3(m_ViewWorld[2]);//m_Target - m_Eye; // FIXME: free ortho cameras = crash
			ymax = (frontvec.Length())*sinf(DTOR*m_FOV/2);
			ymin = -ymax;
			xmin = ymin * Aspect;
			xmax = ymax * Aspect;
			znear = m_NearDist;
			zfar = m_FarDist;
			glOrtho(xmin, xmax, ymin, ymax, znear, zfar);
		}
		else
		{
			gluPerspective(m_FOV, Aspect, m_NearDist, m_FarDist);
		}
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(m_WorldView);
}
