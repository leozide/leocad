#include "lc_global.h"
#include "camera.h"

#include "opengl.h"
#include "defines.h"
#include "file.h"
#include "lc_application.h"
#include "lc_colors.h"

#define LC_CAMERA_SAVE_VERSION 7 // LeoCAD 0.76

static LC_OBJECT_KEY_INFO camera_key_info[LC_CK_COUNT] =
{
	{ "Camera Position", 3, LC_CK_EYE },
	{ "Camera Target", 3, LC_CK_TARGET },
	{ "Camera Roll", 1, LC_CK_ROLL }
};

// =============================================================================
// CameraTarget class

CameraTarget::CameraTarget(lcCamera* Parent)
	: lcObject(LC_OBJECT_CAMERA_TARGET)
{
	m_Parent = Parent;
	m_Name = Parent->m_Name + ".Target";
}

CameraTarget::~CameraTarget()
{
}

void CameraTarget::ClosestLineIntersect(lcClickLine& ClickLine) const
{
	BoundingBox Box;
	Box.m_Max = Vector3(0.2f, 0.2f, 0.2f);
	Box.m_Min = Vector3(-0.2f, -0.2f, -0.2f);

	Matrix44 WorldView = ((lcCamera*)m_Parent)->m_WorldView;
	WorldView.SetTranslation(Mul30(-((lcCamera*)m_Parent)->m_TargetPosition, WorldView));

	Vector3 Start = Mul31(ClickLine.Start, WorldView);
	Vector3 End = Mul31(ClickLine.End, WorldView);

	float Dist;
	if (BoundingBoxRayMinIntersectDistance(Box, Start, End, &Dist) && (Dist < ClickLine.Dist))
	{
		ClickLine.Object = this;
		ClickLine.Dist = Dist;
	}
}

bool CameraTarget::IntersectsVolume(const Vector4* Planes, int NumPlanes) const
{
	BoundingBox Box;
	Box.m_Max = Vector3(0.2f, 0.2f, 0.2f);
	Box.m_Min = Vector3(-0.2f, -0.2f, -0.2f);

	// Transform the planes to local space.
	Vector4* LocalPlanes = new Vector4[NumPlanes];
	int i;

	Matrix44 WorldView = ((lcCamera*)m_Parent)->m_WorldView;
	WorldView.SetTranslation(Mul30(-((lcCamera*)m_Parent)->m_TargetPosition, WorldView));

	for (i = 0; i < NumPlanes; i++)
	{
		LocalPlanes[i] = Vector4(Mul30(Vector3(Planes[i]), WorldView));
		LocalPlanes[i][3] = Planes[i][3] - Dot3(Vector3(WorldView[3]), Vector3(LocalPlanes[i]));
	}

	bool Intersect = BoundingBoxIntersectsVolume(Box, LocalPlanes, NumPlanes);

	delete[] LocalPlanes;

	return Intersect;
}

void CameraTarget::Select(bool bSelecting, bool bFocus, bool bMultiple)
{
	m_Parent->SelectTarget(bSelecting, bFocus, bMultiple);
}

/////////////////////////////////////////////////////////////////////////////
// Camera construction/destruction

lcCamera::lcCamera()
	: lcObject(LC_OBJECT_CAMERA)
{
	Initialize();
}

lcCamera::lcCamera(lcCamera* Camera)
	: lcObject(LC_OBJECT_CAMERA)
{
	Initialize();

	ChangeKey(1, true, Camera->m_Position, LC_CK_EYE);
	ChangeKey(1, true, Camera->m_TargetPosition, LC_CK_TARGET);
	ChangeKey(1, true, &Camera->m_Roll, LC_CK_ROLL);

	UpdatePosition(1);
}

lcCamera::lcCamera(const Vector3& Position, const Vector3& Target)
	: lcObject(LC_OBJECT_CAMERA)
{
	Initialize();

	float roll = 0.0f;

	ChangeKey(1, true, Position, LC_CK_EYE);
	ChangeKey(1, true, Target, LC_CK_TARGET);
	ChangeKey(1, true, &roll, LC_CK_ROLL);

	UpdatePosition(1);
}

lcCamera::~lcCamera()
{
	delete m_Target;
}

void lcCamera::Initialize()
{
	m_FOV = 30;
	m_NearDist = 1;
	m_FarDist = 500;

	m_nState = 0;

	float *values[] = { m_Position, m_TargetPosition, &m_Roll };
	RegisterKeys(values, camera_key_info, LC_CK_COUNT);

	m_Target = new CameraTarget(this);
}

/////////////////////////////////////////////////////////////////////////////
// Camera save/load

bool lcCamera::FileLoad(lcFile& file)
{
	u8 version, ch;

	file.ReadBytes(&version);

	if (version > LC_CAMERA_SAVE_VERSION)
		return false;

	if (version > 5)
		if (!lcObject::FileLoad(file))
			return false;

	if (version == 6)
	{
		for (LC_OBJECT_KEY* node = m_Keys; node; node = node->Next)
		{
			if (node->Type == LC_CK_ROLL)
			{
				node->Value[0] = 0.0f;
				node->Value[1] = 0.0f;
				node->Value[2] = 0.0f;
				node->Value[3] = 0.0f;
			}
		}
	}

	if (version == 4)
	{
		char buf[81];
		file.Read(buf, 80);
		buf[80] = 0;
		m_Name = buf;
	}
	else
	{
		file.Read(&ch, 1);
		if (ch == 0xFF)
			return false; // don't read CString
		char buf[81];
		file.Read(buf, ch);
		buf[ch] = 0;
		m_Name = buf;
	}

	if (version < 3)
	{
		double d[3];
		float f[3];

		file.ReadDoubles(d, 3);
		f[0] = (float)d[0];
		f[1] = (float)d[1];
		f[2] = (float)d[2];
		ChangeKey(1, true, f, LC_CK_EYE);

		file.ReadDoubles(d, 3);
		f[0] = (float)d[0];
		f[1] = (float)d[1];
		f[2] = (float)d[2];
		ChangeKey(1, true, f, LC_CK_TARGET);

		file.ReadDoubles(d, 3);
		float roll = 0.0f;
		ChangeKey(1, true, &roll, LC_CK_ROLL);
	}

	if (version == 3)
	{
		file.ReadBytes(&ch);

		while (ch--)
		{
			u8 step;
			double eye[3], target[3], up[3];
			float f[3];

			file.ReadDoubles(eye, 3);
			file.ReadDoubles(target, 3);
			file.ReadDoubles(up, 3);
			file.ReadBytes(&step);

			f[0] = (float)eye[0];
			f[1] = (float)eye[1];
			f[2] = (float)eye[2];
			ChangeKey(step, true, f, LC_CK_EYE);

			f[0] = (float)target[0];
			f[1] = (float)target[1];
			f[2] = (float)target[2];
			ChangeKey(step, true, f, LC_CK_TARGET);

			float roll = 0.0f;
			ChangeKey(step, true, &roll, LC_CK_ROLL);

			u32 snapshot;
			u32 cam;
			file.ReadInts(&snapshot);
			file.ReadInts(&cam);
//			if (cam == -1)
//				node->pCam = NULL;
//			else
//				node->pCam = pDoc->GetCamera(i);
		}
	}

	if (version < 4)
	{
		double d;
		file.ReadDoubles(&d); m_FOV = (float)d;
		file.ReadDoubles(&d); m_FarDist = (float)d;
		file.ReadDoubles(&d); m_NearDist = (float)d;
	}
	else
	{
		int n;

		if (version < 6)
		{
			unsigned short time;
			float param[4];
			unsigned char type;

			file.ReadInts(&n);
			while (n--)
			{
				file.ReadShorts(&time);
				file.ReadFloats(param, 3);
				file.ReadBytes(&type);

				ChangeKey(time, true, param, type);
			}

			file.ReadInts(&n);
			while (n--)
			{
				file.ReadShorts(&time);
				file.ReadFloats(param, 3);
				file.ReadBytes(&type);
			}
		}

		file.ReadFloats(&m_FOV);
		file.ReadFloats(&m_FarDist);
		file.ReadFloats(&m_NearDist);

		if (version < 5)
		{
			file.ReadInts(&n);
			if (n != 0)
				m_nState |= LC_CAMERA_HIDDEN;
		}
		else
		{
			unsigned char Type;
			file.ReadBytes(&m_nState);
			file.ReadBytes(&Type);
		}
	}

	if ((version > 1) && (version < 4))
	{
		unsigned long show;
		int user;

		file.ReadInts(&show);
//		if (version > 2)
		file.ReadInts(&user);
		if (show == 0)
			m_nState |= LC_CAMERA_HIDDEN;
	}

	return true;
}

void lcCamera::FileSave(lcFile& file) const
{
	unsigned char ch = LC_CAMERA_SAVE_VERSION;

	file.WriteBytes(&ch, 1);

	lcObject::FileSave(file);

	ch = (u8)m_Name.GetLength();
	file.WriteBytes(&ch);
	file.Write((char*)m_Name, ch);

	file.WriteFloats(&m_FOV);
	file.WriteFloats(&m_FarDist);
	file.WriteFloats(&m_NearDist);
	// version 5
	file.WriteBytes(&m_nState);
	unsigned char Type = 0;
	file.WriteBytes(&Type);
}

/////////////////////////////////////////////////////////////////////////////
// Camera operations

void lcCamera::Move(u32 Time, bool AddKey, const Vector3& Delta)
{
	/*
	if (IsSide())
	{
		m_Position += Delta;
		m_TargetPosition += Delta;

		ChangeKey(Time, AddKey, m_Position, LC_CK_EYE);
		ChangeKey(Time, AddKey, m_TargetPosition, LC_CK_TARGET);
	}
	else
	*/
	{
		if (IsEyeSelected())
		{
			m_Position += Delta;

			ChangeKey(Time, AddKey, m_Position, LC_CK_EYE);
		}

		if (IsTargetSelected())
		{
			m_TargetPosition += Delta;

			ChangeKey(Time, AddKey, m_TargetPosition, LC_CK_TARGET);
		}
	}
}

void lcCamera::Select(bool bSelecting, bool bFocus, bool bMultiple)
{
	if (bSelecting == true)
	{
		if (bFocus == true)
		{
			m_nState |= (LC_CAMERA_FOCUSED|LC_CAMERA_SELECTED);

			m_Target->Select(false, true, bMultiple);
		}
		else
			m_nState |= LC_CAMERA_SELECTED;

		if (bMultiple == false)
			m_Target->Select(false, false, bMultiple);
	}
	else
	{
		if (bFocus == true)
			m_nState &= ~(LC_CAMERA_FOCUSED);
		else
			m_nState &= ~(LC_CAMERA_SELECTED|LC_CAMERA_FOCUSED);
	} 
}

void lcCamera::SelectTarget(bool bSelecting, bool bFocus, bool bMultiple)
{
	// FIXME: the target should handle this

	if (bSelecting == true)
	{
		if (bFocus == true)
		{
			m_nState |= (LC_CAMERA_TARGET_FOCUSED|LC_CAMERA_TARGET_SELECTED);

			Select(false, true, bMultiple);
		}
		else
			m_nState |= LC_CAMERA_TARGET_SELECTED;

		if (bMultiple == false)
			Select(false, false, bMultiple);
	}
	else
	{
		if (bFocus == true)
			m_nState &= ~(LC_CAMERA_TARGET_FOCUSED);
		else
			m_nState &= ~(LC_CAMERA_TARGET_SELECTED|LC_CAMERA_TARGET_FOCUSED);
	} 
}

void lcCamera::UpdatePosition(u32 Time)
{
	CalculateKeys(Time);

	CalculateMatrices();
}

void lcCamera::Render(float fLineWidth)
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

	if (IsEyeSelected())
	{
		glLineWidth(2.0f);
		lcSetColor((m_nState & LC_CAMERA_FOCUSED) != 0 ? LC_COLOR_FOCUS : LC_COLOR_SELECTION);
	}
	else
	{
		glLineWidth(1.0f);
		glColor4f(0.5f, 0.8f, 0.5f, 1.0f);
	}

	glVertexPointer(3, GL_FLOAT, 0, verts);
	glDrawArrays(GL_LINES, 0, 24);
	glDrawArrays(GL_LINE_STRIP, 24, 10);

	glPopMatrix();

	// Draw target box.
	glPushMatrix();
	Matrix44 TargetMat = m_ViewWorld;
	TargetMat.SetTranslation(m_TargetPosition);
	glMultMatrixf(TargetMat);

	if (IsTargetSelected())
	{
		glLineWidth(2.0f);
		lcSetColor((m_nState & LC_CAMERA_TARGET_FOCUSED) != 0 ? LC_COLOR_FOCUS : LC_COLOR_SELECTION);
	}
	else
	{
		glLineWidth(1.0f);
		glColor4f(0.5f, 0.8f, 0.5f, 1.0f);
	}

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

	glLineWidth(1.0f);

	float Line[2][3] =
	{
		{ m_Position[0], m_Position[1], m_Position[2] },
		{ m_TargetPosition[0], m_TargetPosition[1], m_TargetPosition[2] },
	};

	glVertexPointer(3, GL_FLOAT, 0, Line);
	glColor4f(0.5f, 0.8f, 0.5f, 1.0f);
	glDrawArrays(GL_LINES, 0, 2);

	if (IsSelected())
	{
		glPushMatrix();
		glMultMatrixf(m_ViewWorld);

		float Dist = Length(m_TargetPosition - m_Position);
		Matrix44 Projection;
		Projection = CreatePerspectiveMatrix(m_FOV, 1.33f, 0.01f, Dist);
		Projection = Inverse(Projection);
		glMultMatrixf(Projection);

		// Draw the view frustum.
		float verts[16][3] =
		{
			{  1,  1,  1 }, { -1,  1, 1 },
			{ -1,  1,  1 }, { -1, -1, 1 },
			{ -1, -1,  1 }, {  1, -1, 1 },
			{  1, -1,  1 }, {  1,  1, 1 },
			{  1,  1, -1 }, {  1,  1, 1 },
			{ -1,  1, -1 }, { -1,  1, 1 },
			{ -1, -1, -1 }, { -1, -1, 1 },
			{  1, -1, -1 }, {  1, -1, 1 },
		};

		glVertexPointer(3, GL_FLOAT, 0, verts);
		glDrawArrays(GL_LINES, 0, 16);

		glPopMatrix();
	}

	glDisableClientState(GL_VERTEX_ARRAY);
}

void lcCamera::ClosestLineIntersect(lcClickLine& ClickLine) const
{
	BoundingBox Box;
	Box.m_Max = Vector3(0.2f, 0.2f, 0.2f);
	Box.m_Min = Vector3(-0.2f, -0.2f, -0.2f);

	Vector3 Start = Mul31(ClickLine.Start, m_WorldView);
	Vector3 End = Mul31(ClickLine.End, m_WorldView);

	float Dist;
	if (BoundingBoxRayMinIntersectDistance(Box, Start, End, &Dist) && (Dist < ClickLine.Dist))
	{
		ClickLine.Object = this;
		ClickLine.Dist = Dist;
	}

	m_Target->ClosestLineIntersect(ClickLine);
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
		Intersect = m_Target->IntersectsVolume(Planes, NumPlanes);

	return Intersect;
}
