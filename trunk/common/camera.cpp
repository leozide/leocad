// Camera object.

#include "lc_global.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "opengl.h"
#include "globals.h"
#include "defines.h"
#include "file.h"
#include "camera.h"
#include "tr.h"

#define LC_CAMERA_SAVE_VERSION 6 // LeoCAD 0.73

static LC_OBJECT_KEY_INFO camera_key_info[LC_CK_COUNT] =
{
  { "Camera Position", 3, LC_CK_EYE },
  { "Camera Target", 3, LC_CK_TARGET },
  { "Camera Up Vector", 3, LC_CK_UP }
};

/////////////////////////////////////////////////////////////////////////////
// Camera save/load

bool Camera::FileLoad(File& file)
{
	unsigned char version, ch;

	file.ReadByte(&version, 1);

	if (version > LC_CAMERA_SAVE_VERSION)
		return false;

	if (version > 5)
		if (!Object::FileLoad(file))
			return false;

	if (version == 4)
	{
		file.Read(m_strName, 80);
		m_strName[80] = 0;
	}
	else
	{
		file.Read(&ch, 1);
		if (ch == 0xFF)
			return false; // don't read CString
		file.Read(m_strName, ch);
		m_strName[ch] = 0;
	}

	if (version < 3)
	{
		double d[3];
		float f[3];

		file.ReadDouble(d, 3);
		f[0] = (float)d[0];
		f[1] = (float)d[1];
		f[2] = (float)d[2];
		ChangeKey(1, true, f, LC_CK_EYE);
		ChangeKey(1, true, f, LC_CK_EYE);

		file.ReadDouble(d, 3);
		f[0] = (float)d[0];
		f[1] = (float)d[1];
		f[2] = (float)d[2];
		ChangeKey(1, true, f, LC_CK_TARGET);
		ChangeKey(1, true, f, LC_CK_TARGET);

		file.ReadDouble(d, 3);
		f[0] = (float)d[0];
		f[1] = (float)d[1];
		f[2] = (float)d[2];
		ChangeKey(1, true, f, LC_CK_UP);
		ChangeKey(1, true, f, LC_CK_UP);
	}

	if (version == 3)
	{
		file.Read(&ch, 1);

		while (ch--)
		{
			unsigned char step;
			double eye[3], target[3], up[3];
			float f[3];

			file.ReadDouble(eye, 3);
			file.ReadDouble(target, 3);
			file.ReadDouble(up, 3);
			file.ReadByte(&step, 1);

			if (up[0] == 0 && up[1] == 0 && up[2] == 0)
				up[2] = 1;

			f[0] = (float)eye[0];
			f[1] = (float)eye[1];
			f[2] = (float)eye[2];
			ChangeKey(step, true, f, LC_CK_EYE);
			ChangeKey(step, true, f, LC_CK_EYE);

			f[0] = (float)target[0];
			f[1] = (float)target[1];
			f[2] = (float)target[2];
			ChangeKey(step, true, f, LC_CK_TARGET);
			ChangeKey(step, true, f, LC_CK_TARGET);

			f[0] = (float)up[0];
			f[1] = (float)up[1];
			f[2] = (float)up[2];
			ChangeKey(step, true, f, LC_CK_UP);
			ChangeKey(step, true, f, LC_CK_UP);

			int snapshot; // BOOL under Windows
			int cam;
			file.ReadLong(&snapshot, 1);
			file.ReadLong(&cam, 1);
//			if (cam == -1)
//				node->pCam = NULL;
//			else
//				node->pCam = pDoc->GetCamera(i);
		}
	}

	if (version < 4)
	{
		double d;
		file.ReadDouble(&d, 1); m_fovy = (float)d;
		file.ReadDouble(&d, 1); m_zFar = (float)d;
		file.ReadDouble(&d, 1); m_zNear= (float)d;
	}
	else
	{
		int n;

		if (version < 6)
		{
			unsigned short time;
			float param[4];
			unsigned char type;

			file.ReadLong(&n, 1);
			while (n--)
			{
				file.ReadShort(&time, 1);
				file.ReadFloat(param, 3);
				file.ReadByte(&type, 1);

				ChangeKey(time, true, param, type);
			}

			file.ReadLong(&n, 1);
			while (n--)
			{
				file.ReadShort(&time, 1);
				file.ReadFloat(param, 3);
				file.ReadByte(&type, 1);

				ChangeKey(time, true, param, type);
			}
		}

		file.ReadFloat(&m_fovy, 1);
		file.ReadFloat(&m_zFar, 1);
		file.ReadFloat(&m_zNear, 1);

		if (version < 5)
		{
			file.ReadLong(&n, 1);
			if (n != 0)
				m_nState |= LC_CAMERA_HIDDEN;
		}
		else
		{
			file.ReadByte(&m_nState, 1);
			file.ReadByte(&m_nType, 1);
		}
	}

	if ((version > 1) && (version < 4))
	{
		unsigned long show;
		int user;

		file.ReadLong(&show, 1);
//		if (version > 2)
		file.ReadLong(&user, 1);
		if (show == 0)
			m_nState |= LC_CAMERA_HIDDEN;
	}

	return true;
}

void Camera::FileSave(File& file) const
{
	unsigned char ch = LC_CAMERA_SAVE_VERSION;

	file.WriteByte(&ch, 1);

	Object::FileSave(file);

	ch = (unsigned char)strlen(m_strName);
	file.Write(&ch, 1);
	file.Write(m_strName, ch);

	file.WriteFloat(&m_fovy, 1);
	file.WriteFloat(&m_zFar, 1);
	file.WriteFloat(&m_zNear, 1);
	// version 5
	file.WriteByte(&m_nState, 1);
	file.WriteByte(&m_nType, 1);
}

/////////////////////////////////////////////////////////////////////////////
// Camera operations

void Camera::Move(unsigned short nTime, bool bAddKey, float dx, float dy, float dz)
{
	if (IsSide())
	{
		m_Eye[0] += dx;
		m_Eye[1] += dy;
		m_Eye[2] += dz;
		m_Target[0] += dx;
		m_Target[1] += dy;
		m_Target[2] += dz;

		ChangeKey(nTime, bAddKey, m_Eye, LC_CK_EYE);
		ChangeKey(nTime, bAddKey, m_Target, LC_CK_TARGET);
	}
	else
	{
		if (IsEyeSelected())
		{
			m_Eye[0] += dx;
			m_Eye[1] += dy;
			m_Eye[2] += dz;

			ChangeKey(nTime, bAddKey, m_Eye, LC_CK_EYE);
		}

		if (IsTargetSelected())
		{
			m_Target[0] += dx;
			m_Target[1] += dy;
			m_Target[2] += dz;

			ChangeKey(nTime, bAddKey, m_Target, LC_CK_TARGET);
		}

		// Fix the up vector
		Vector3 upvec(m_Up), sidevec;
		Vector3 frontvec = m_Target - m_Eye;
		sidevec = Cross3(frontvec, upvec);
		upvec = Cross3(sidevec, frontvec);
		m_Up = upvec.Normalize();

		ChangeKey(nTime, bAddKey, m_Up, LC_CK_UP);
	}
}

void Camera::LoadProjection(float fAspect)
{
	if (m_pTR != NULL)
		m_pTR->BeginTile();
	else
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		if (m_nState & LC_CAMERA_ORTHOGRAPHIC)
		{
			float ymax, ymin, xmin, xmax, znear, zfar;
			Vector3 frontvec = m_Target - m_Eye;
			ymax = (frontvec.Length())*sinf(DTOR*m_fovy/2);
			ymin = -ymax;
			xmin = ymin * fAspect;
			xmax = ymax * fAspect;
			znear = m_zNear;
			zfar = m_zFar;
			glOrtho(xmin, xmax, ymin, ymax, znear, zfar);
		}
		else
		{
			gluPerspective(m_fovy, fAspect, m_zNear, m_zFar);
		}
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(m_WorldView);
}

void Camera::DoZoom(int dy, int mouse, unsigned short nTime, bool bAddKey)
{
	if (m_nState & LC_CAMERA_ORTHOGRAPHIC)
	{
		// TODO: have a different option to change the fov.
		m_fovy += (float)dy/(21-mouse);

		if (m_fovy < 0.001f)
			m_fovy = 0.001f;
		else if (m_fovy > 179.999f)
			m_fovy = 179.999f;
	}
	else
	{
		Vector3 frontvec = m_Eye - m_Target;
		frontvec.Normalize();
		frontvec *= 2.0f*dy/(21-mouse);

		// TODO: option to move eye, target or both
		m_Eye += frontvec;
		m_Target += frontvec;

		ChangeKey(nTime, bAddKey, m_Eye, LC_CK_EYE);
		ChangeKey(nTime, bAddKey, m_Target, LC_CK_TARGET);
		UpdatePosition(nTime);
	}
}

void Camera::DoPan(int dx, int dy, int mouse, unsigned short nTime, bool bAddKey)
{
	Vector3 upvec(m_Up), frontvec = m_Eye - m_Target;
	Vector3 sidevec = Cross3(frontvec, upvec);
	sidevec.Normalize();
	sidevec *= 2.0f*dx/(21-mouse);
	upvec.Normalize();
	upvec *= -2.0f*dy/(21-mouse);

	m_Eye += upvec + sidevec;
	m_Target += upvec + sidevec;

	ChangeKey(nTime, bAddKey, m_Eye, LC_CK_EYE);
	ChangeKey(nTime, bAddKey, m_Target, LC_CK_TARGET);
	UpdatePosition(nTime);
}

void Camera::DoRotate(int dx, int dy, int mouse, unsigned short nTime, bool bAddKey, float* /*center*/)
{
	Vector3 Dir = m_Eye - m_Target;

	// The X axis of the mouse always corresponds to Z in the world.
	if (dx)
	{
		float AngleX = -2.0f * dx / (21 - mouse) * LC_DTOR;
		Matrix33 RotX = MatrixFromAxisAngle(Vector3(0, 0, 1), AngleX);

		Dir = Mul(Dir, RotX);
		m_Up = Mul(m_Up, RotX);
	}

	// The Y axis will the side vector of the camera.
	if (dy)
	{
		float AngleY = 2.0f * dy / (21 - mouse) * LC_DTOR;
		Matrix33 RotY = MatrixFromAxisAngle(Vector3(m_WorldView[0][0], m_WorldView[1][0], m_WorldView[2][0]), AngleY);

		Dir = Mul(Dir, RotY);
		m_Up = Mul(m_Up, RotY);
	}

	m_Eye = m_Target + Dir;

	ChangeKey(nTime, bAddKey, m_Eye, LC_CK_EYE);
	ChangeKey(nTime, bAddKey, m_Up, LC_CK_UP);
	UpdatePosition(nTime);
}

void Camera::DoRoll(int dx, int mouse, unsigned short nTime, bool bAddKey)
{
	Matrix44 mat;
	Vector3 front = m_Eye - m_Target;

	mat = MatrixFromAxisAngle(front, LC_DTOR * 2.0f*dx/(21-mouse));
	m_Up = Mul30(m_Up, mat);

	ChangeKey(nTime, bAddKey, m_Up, LC_CK_UP);
	UpdatePosition(nTime);
}

float Camera::GetRoll() const
{
	Vector3 Front = Normalize(m_Target - m_Eye);

	// Calculate pitch and yaw angles.
	float Pitch, Yaw;
	Pitch = asinf(Front[2]);

	Front[2] = 0;
	if (Front.LengthSquared() != 0.0f)
		Front = Normalize(Front);
	Yaw = atan2f(-Front[0], Front[1]);

	// Rotate the up and side vectors.
	Quaternion PitchRot = CreateRotationXQuaternion(Pitch);
	Quaternion YawRot = CreateRotationZQuaternion(Yaw);

	Vector3 Up(0, 0, 1);
	Up = Mul(Up, PitchRot);
	Up = Mul(Up, YawRot);

	Vector3 Side(1, 0, 0);
	Side = Mul(Side, PitchRot);
	Side = Mul(Side, YawRot);

	float Angle = acosf(max(min(Dot3(Up, m_Up), 1), -1));
	float Sign1 = Dot3(Side, m_Up);
	float Sign2 = Dot3(m_Up, Up);

	if (Sign1 > 0)
		Angle = -Angle;

	if (Sign2 < 0)
		Angle -= LC_PI;

	if (Angle <= -LC_PI)
		Angle += 2*LC_PI;

	return Angle;
}

void Camera::SetRoll(float Roll, unsigned short nTime, bool bAddKey)
{
	Vector3 Front = Normalize(m_Target - m_Eye);

	// Calculate pitch and yaw angles.
	float Pitch, Yaw;
	Pitch = asinf(Front[2]);

	Front[2] = 0;
	if (Front.LengthSquared() != 0.0f)
		Front = Normalize(Front);
	Yaw = atan2f(-Front[0], Front[1]);

	// Rotate the up and side vectors.
	Quaternion PitchRot = CreateRotationXQuaternion(Pitch);
	Quaternion YawRot = CreateRotationZQuaternion(Yaw);

	Vector3 Up(0, 0, 1);
	Up = Mul(Up, PitchRot);
	Up = Mul(Up, YawRot);

	Vector3 Side(1, 0, 0);
	Side = Mul(Side, PitchRot);
	Side = Mul(Side, YawRot);

	float Sign1 = Dot3(Side, m_Up);
	float Sign2 = Dot3(m_Up, Up);

	if (Sign1 > 0)
		Roll = -Roll;

	if (Sign2 < 0)
		Roll -= LC_PI;

	if (Roll <= -LC_PI)
		Roll += 2*LC_PI;

	Quaternion RollRot = QuaternionFromAxisAngle(Vector4(Normalize(m_Target - m_Eye), Roll));
	Up = Mul(Up, RollRot);

	ChangeKey(nTime, bAddKey, Up, LC_CK_UP);
	UpdatePosition(nTime);
}

void Camera::StartTiledRendering(int tw, int th, int iw, int ih, float fAspect)
{
	m_pTR = new TiledRender();
	m_pTR->TileSize(tw, th, 0);
	m_pTR->ImageSize(iw, ih);
	m_pTR->Perspective(m_fovy, fAspect, m_zNear, m_zFar);
}

void Camera::GetTileInfo(int* row, int* col, int* width, int* height)
{
	if (m_pTR != NULL)
	{
		*row = m_pTR->m_Rows - m_pTR->m_CurrentRow - 1;
		*col = m_pTR->m_CurrentColumn;
		*width = m_pTR->m_CurrentTileWidth;
		*height = m_pTR->m_CurrentTileHeight;
	}
}

bool Camera::EndTile()
{
	if (m_pTR != NULL)
	{
		if (m_pTR->EndTile())
			return true;

		delete m_pTR;
		m_pTR = NULL;
	}

	return false;
}
