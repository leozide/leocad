// Camera object.

#ifdef LC_WINDOWS
#include "stdafx.h"
#endif
#include "GL/glu.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "globals.h"
#include "defines.h"
#include "vector.h"
#include "matrix.h"
#include "file.h"
#include "boundbox.h"
#include "camera.h"
#include "tr.h"

/////////////////////////////////////////////////////////////////////////////
// Static functions

static GLuint _nTargetList = 0;

static CAMERA_KEY* AddNode (CAMERA_KEY *node, unsigned short nTime, unsigned char nType)
{
	CAMERA_KEY* newnode = (CAMERA_KEY*)malloc(sizeof(CAMERA_KEY));

	if (node)
	{
		newnode->next = node->next;
		node->next = newnode;
	}
	else
		newnode->next = NULL;

	newnode->type = nType;
	newnode->time = nTime;
	newnode->param[0] = newnode->param[1] = newnode->param[2] = 0;

	return newnode;
}

static bool invert(double src[16], double inverse[16])
{
	double t;
	int i, j, k, swap;
	double tmp[4][4];

	for (i = 0; i < 16; i++)
		inverse[i] = 0.0;
	inverse[0] = inverse[5] = inverse[10] = inverse[15] = 1.0;

	for (i = 0; i < 4; i++)
	for (j = 0; j < 4; j++)
		tmp[i][j] = src[i*4+j];

	for (i = 0; i < 4; i++) 
	{
		// look for largest element in column.
		swap = i;
		for (j = i + 1; j < 4; j++) 
			if (fabs(tmp[j][i]) > fabs(tmp[i][i])) 
				swap = j;

		if (swap != i) 
		{
			// swap rows.
			for (k = 0; k < 4; k++)
			{
				t = tmp[i][k];
				tmp[i][k] = tmp[swap][k];
				tmp[swap][k] = t;

				t = inverse[i*4+k];
				inverse[i*4+k] = inverse[swap*4+k];
				inverse[swap*4+k] = t;
			}
		}

		if (tmp[i][i] == 0) 
		{
			// The matrix is singular, which shouldn't happen.
			return false;
		}

		t = tmp[i][i];
		for (k = 0; k < 4; k++) 
		{
			tmp[i][k] /= t;
			inverse[i*4+k] /= t;
		}
		for (j = 0; j < 4; j++) 
		{
			if (j != i) 
			{
				t = tmp[j][i];
				for (k = 0; k < 4; k++) 
				{
					tmp[j][k] -= tmp[i][k]*t;
					inverse[j*4+k] -= inverse[i*4+k]*t;
				}
			}
		}
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////
// Camera construction/destruction

Camera::Camera()
{
	Initialize();
}

// Start with a standard camera.
Camera::Camera(unsigned char nType, Camera* pPrev)
{
	if (nType > 7)
		nType = 8;

	char names[8][7] = { "Front", "Back",  "Top",  "Under", "Left", "Right", "Main", "User" };
	float eyes[8][3] = { { 50,0,0 }, { -50,0,0 }, { 0,0,50 }, { 0,0,-50 },
			     { 0,50,0 }, { 0,-50,0 }, { 10,10,5}, { 0,5,0 }};
	float ups [8][3] = {  { 0,0,1 }, { 0,0,1 }, { 1,0,0 }, { -1,0,0 }, { 0,0,1 },
			      { 0,0,1 }, {-0.2357f, -0.2357f, 0.94281f }, { 0,0,1 }};
	CAMERA_KEY* node;

	Initialize();
	m_pAnimationKeys = AddNode (NULL, 1, CK_EYE);
	m_pAnimationKeys->param[0] = eyes[nType][0];
	m_pAnimationKeys->param[1] = eyes[nType][1];
	m_pAnimationKeys->param[2] = eyes[nType][2];
	node = AddNode (m_pAnimationKeys, 1, CK_TARGET);
	node = AddNode (node, 1, CK_UP);
	node->param[0] = ups[nType][0];
	node->param[1] = ups[nType][1];
	node->param[2] = ups[nType][2];

	m_pInstructionKeys = AddNode (NULL, 1, CK_EYE);
	m_pInstructionKeys->param[0] = eyes[nType][0];
	m_pInstructionKeys->param[1] = eyes[nType][1];
	m_pInstructionKeys->param[2] = eyes[nType][2];
	node = AddNode (m_pInstructionKeys, 1, CK_TARGET);
	node = AddNode (node, 1, CK_UP);
	node->param[0] = ups[nType][0];
	node->param[1] = ups[nType][1];
	node->param[2] = ups[nType][2];

	strcpy (m_strName, names[nType]);
	if (nType != 8)
		m_nState = LC_CAMERA_HIDDEN;
	m_nType = nType;

	if (pPrev)
		pPrev->m_pNext = this;

	UpdatePosition(1, false);
}

// From OnMouseMove(), case LC_ACTION_ROTATE_VIEW
Camera::Camera(float eye[3], float target[3], float up[3], Camera* pCamera)
{
	CAMERA_KEY* node;

	// Fix the up vector
	Vector upvec(up), frontvec(eye[0]-target[0], eye[1]-target[1], eye[2]-target[2]), sidevec;
	frontvec.Normalize();
	sidevec.Cross(frontvec, upvec);
	upvec.Cross(sidevec, frontvec);
	upvec.Normalize();

	Initialize();
	m_pAnimationKeys = AddNode (NULL, 1, CK_EYE);
	m_pAnimationKeys->param[0] = eye[0];
	m_pAnimationKeys->param[1] = eye[1];
	m_pAnimationKeys->param[2] = eye[2];
	node = AddNode (m_pAnimationKeys, 1, CK_TARGET);
	node->param[0] = target[0];
	node->param[1] = target[1];
	node->param[2] = target[2];
	node = AddNode (node, 1, CK_UP);
	node->param[0] = upvec.X();
	node->param[1] = upvec.Y();
	node->param[2] = upvec.Z();

	m_pInstructionKeys = AddNode (NULL, 1, CK_EYE);
	m_pInstructionKeys->param[0] = eye[0];
	m_pInstructionKeys->param[1] = eye[1];
	m_pInstructionKeys->param[2] = eye[2];
	node = AddNode (m_pInstructionKeys, 1, CK_TARGET);
	node->param[0] = target[0];
	node->param[1] = target[1];
	node->param[2] = target[2];
	node = AddNode (node, 1, CK_UP);
	node->param[0] = upvec.X();
	node->param[1] = upvec.Y();
	node->param[2] = upvec.Z();

	int i, max = 0;

	for (;;)
	{
		if (strncmp (pCamera->m_strName, "Camera ", 7) == 0)
			if (sscanf(pCamera->m_strName, "Camera %d", &i) == 1)
				if (i > max) 
					max = i;

		if (pCamera->m_pNext == NULL)
		{
			sprintf(m_strName, "Camera %d", max+1);
			pCamera->m_pNext = this;
			break;
		}
		else
			pCamera = pCamera->m_pNext;
	}

	UpdatePosition(1, false);
}

// From LC_ACTION_CAMERA
Camera::Camera(float ex, float ey, float ez, float tx, float ty, float tz, Camera* pCamera)
{
	CAMERA_KEY* node;

	// Fix the up vector
	Vector upvec(0,0,1), frontvec(ex-tx, ey-ty, ez-tz), sidevec;
	frontvec.Normalize();
	if (frontvec == upvec)
		sidevec.FromFloat(1,0,0);
	else
		sidevec.Cross(frontvec, upvec);
	upvec.Cross(sidevec, frontvec);
	upvec.Normalize();

	Initialize();
	m_pAnimationKeys = AddNode (NULL, 1, CK_EYE);
	m_pAnimationKeys->param[0] = ex;
	m_pAnimationKeys->param[1] = ey;
	m_pAnimationKeys->param[2] = ez;
	node = AddNode (m_pAnimationKeys, 1, CK_TARGET);
	node->param[0] = tx;
	node->param[1] = ty;
	node->param[2] = tz;
	node = AddNode (node, 1, CK_UP);
	node->param[0] = upvec.X();
	node->param[1] = upvec.Y();
	node->param[2] = upvec.Z();

	m_pInstructionKeys = AddNode (NULL, 1, CK_EYE);
	m_pInstructionKeys->param[0] = ex;
	m_pInstructionKeys->param[1] = ey;
	m_pInstructionKeys->param[2] = ez;
	node = AddNode (m_pInstructionKeys, 1, CK_TARGET);
	node->param[0] = tx;
	node->param[1] = ty;
	node->param[2] = tz;
	node = AddNode (node, 1, CK_UP);
	node->param[0] = upvec.X();
	node->param[1] = upvec.Y();
	node->param[2] = upvec.Z();

	int i, max = 0;

	if (pCamera)
	for (;;)
	{
		if (strncmp (pCamera->m_strName, "Camera ", 7) == 0)
			if (sscanf(pCamera->m_strName, "Camera %d", &i) == 1)
				if (i > max) 
					max = i;

		if (pCamera->m_pNext == NULL)
		{
			sprintf(m_strName, "Camera %d", max+1);
			pCamera->m_pNext = this;
			break;
		}
		else
			pCamera = pCamera->m_pNext;
	}

	UpdatePosition(1, false);
}

Camera::~Camera()
{
	RemoveKeys();
}

void Camera::Initialize()
{
	m_fovy = 30;
	m_zNear = 1;
	m_zFar = 100;

	m_BoundingBox.Initialize(this, LC_CAMERA);
	m_TargetBoundingBox.Initialize(this, LC_CAMERA_TARGET);
	m_pNext = NULL;
	m_nState = 0;
	m_pAnimationKeys = NULL;
	m_pInstructionKeys = NULL;
	m_nList = 0;
	m_nType = LC_CAMERA_USER;

	m_pTR = NULL;
	for (unsigned char i = 0 ; i < sizeof(m_strName) ; i++ )
		m_strName[i] = 0;
}

/////////////////////////////////////////////////////////////////////////////
// Camera save/load

void Camera::FileLoad(File* file)
{
	RemoveKeys();
	unsigned char version, ch;
	CAMERA_KEY *node;

	file->Read(&version, 1);

	if (version == 4)
	{
		file->Read(m_strName, 80);
		m_strName[80] = 0;
	}
	else
	{
		file->Read(&ch, 1);
		if (ch == 0xFF)
			return; // don't read CString
		file->Read(m_strName, ch);
		m_strName[ch] = 0;
	}

	if (version < 3)
	{
		double d[3];
		file->Read(d, sizeof(d));
		m_pInstructionKeys = AddNode(NULL, 1, CK_EYE);
		m_pInstructionKeys->param[0] = (float)d[0];
		m_pInstructionKeys->param[1] = (float)d[1];
		m_pInstructionKeys->param[2] = (float)d[2];
		file->Read(d, sizeof(d));
		node = AddNode(m_pInstructionKeys, 1, CK_TARGET);
		node->param[0] = (float)d[0];
		node->param[1] = (float)d[1];
		node->param[2] = (float)d[2];
		file->Read (d, sizeof(d));
		node = AddNode(node, 1, CK_UP);
		node->param[0] = (float)d[0];
		node->param[1] = (float)d[1];
		node->param[2] = (float)d[2];
//		m_Start.snapshot = FALSE;
	}

	if (version == 3)
	{
		file->Read(&ch, 1);

		node = NULL;
		while (ch--)
		{
			if (node == NULL)
			{
				m_pInstructionKeys = AddNode(NULL, 1, CK_EYE);
				node = m_pInstructionKeys;
			}
			else
				node = AddNode(node, 1, CK_EYE);

			unsigned char step;
			double eye[3], target[3], up[3];
			file->Read(eye, sizeof(double[3]));
			file->Read(target, sizeof(double[3]));
			file->Read(up, sizeof(double[3]));
			file->Read(&step, 1);

			if (up[0] == 0 && up[1] == 0 && up[2] == 0)
				up[2] = 1;

			node->time = step;
			node->param[0] = (float)eye[0];
			node->param[1] = (float)eye[1];
			node->param[2] = (float)eye[2];
			node = AddNode(node, step, CK_TARGET);
			node->param[0] = (float)target[0];
			node->param[1] = (float)target[1];
			node->param[2] = (float)target[2];
			node = AddNode(node, step, CK_UP);
			node->param[0] = (float)up[0];
			node->param[1] = (float)up[1];
			node->param[2] = (float)up[2];

			int snapshot; // BOOL under Windows
			int cam;
			file->Read(&snapshot, 4);
			file->Read(&cam, 4);
//			if (cam == -1)
//				node->pCam = NULL;
//			else
//				node->pCam = pDoc->GetCamera(i);
			}
		}

		if (version < 4)
		{
			m_pAnimationKeys = AddNode (NULL, 1, CK_EYE);
			CAMERA_KEY* node = AddNode (m_pAnimationKeys, 1, CK_TARGET);
			node = AddNode (node, 1, CK_UP);
			memcpy(m_pAnimationKeys->param, m_pInstructionKeys->param, sizeof(float[3]));
			memcpy(m_pAnimationKeys->next->param, m_pInstructionKeys->next->param, sizeof(float[3]));
			memcpy(node->param, m_pInstructionKeys->next->next->param, sizeof(float[3]));

			double d;
			file->Read(&d, sizeof(d)); m_fovy = (float)d;
			file->Read(&d, sizeof(d)); m_zFar = (float)d;
			file->Read(&d, sizeof(d)); m_zNear= (float)d;
		}
		else
		{
			int n;

			file->Read(&n, 4);
			for (node = NULL; n--;)
			{
				if (node == NULL)
				{
					m_pInstructionKeys = AddNode(NULL, 1, CK_EYE);
					node = m_pInstructionKeys;
				}
				else
					node = AddNode(node, 1, CK_EYE);
				file->Read(&node->time, 2);
				file->Read(node->param, 12);
				file->Read(&node->type, 1);
			}

			file->Read(&n, 4);
			for (node = NULL; n--;)
			{
				if (node == NULL)
				{
					m_pAnimationKeys = AddNode(NULL, 1, CK_EYE);
					node = m_pAnimationKeys;
				}
				else
					node = AddNode(node, 1, CK_EYE);
				file->Read(&node->time, 2);
				file->Read(node->param, 12);
				file->Read(&node->type, 1);
			}

			file->Read(&m_fovy, 4);
			file->Read(&m_zFar, 4);
			file->Read(&m_zNear, 4);

			if (version < 5)
			{
				file->Read(&n, 4);
				if (n != 0)
					m_nState |= LC_CAMERA_HIDDEN;
			}
			else
			{
				file->Read(&m_nState, 1);
				file->Read(&m_nType, 1);
			}
		}

		if ((version > 1) && (version < 4))
		{
			unsigned long show;
			int user;

			file->Read(&show, 4);
//			if (version > 2)
				file->Read(&user, 4);
			if (show == 0)
				m_nState |= LC_CAMERA_HIDDEN;
		}
}

void Camera::FileSave(File* file)
{
	int n;
	CAMERA_KEY *node;
	unsigned char ch = 5; // LeoCAD 0.70

	file->Write(&ch, 1);
	ch = (unsigned char)strlen(m_strName);
	file->Write(&ch, 1);
	file->Write(m_strName, ch);

	for (n = 0, node = m_pInstructionKeys; node; node = node->next)
		n++;
	file->Write(&n, 4);

	for (node = m_pInstructionKeys; node; node = node->next)
	{
		file->Write(&node->time, 2);
		file->Write(node->param, 12);
		file->Write(&node->type, 1);
	}

	for (n = 0, node = m_pAnimationKeys; node; node = node->next)
		n++;
	file->Write(&n, 4);

	for (node = m_pAnimationKeys; node; node = node->next)
	{
		file->Write(&node->time, 2);
		file->Write(node->param, 12);
		file->Write(&node->type, 1);
	}

	file->Write(&m_fovy, 4);
	file->Write(&m_zFar, 4);
	file->Write(&m_zNear, 4);
	// version 5
	file->Write(&m_nState, 1);
	file->Write(&m_nType, 1);
}

/////////////////////////////////////////////////////////////////////////////
// Camera operations

void Camera::ChangeKey(unsigned short nTime, bool bAnimation, bool bAddKey, float param[3], unsigned char nKeyType)
{
	CAMERA_KEY *node, *poskey = NULL, *newpos = NULL;
	if (bAnimation)
		node = m_pAnimationKeys;
	else
		node = m_pInstructionKeys;

	while (node)
	{
		if ((node->time <= nTime) &&
			(node->type == nKeyType))
				poskey = node;

		node = node->next;
	}

	if (bAddKey)
	{
		if (poskey)
		{
			if (poskey->time != nTime)
				newpos = AddNode(poskey, nTime, nKeyType);
		}
		else
			newpos = AddNode(poskey, nTime, nKeyType);
	}

	if (newpos == NULL)
		newpos = poskey;

	newpos->param[0] = param[0];
	newpos->param[1] = param[1];
	newpos->param[2] = param[2];
}

void Camera::Move(unsigned short nTime, bool bAnimation, bool bAddKey, float x, float y, float z)
{
	if (IsSide())
	{
		m_fEye[0] += x;
		m_fEye[1] += y;
		m_fEye[2] += z;
		m_fTarget[0] += x;
		m_fTarget[1] += y;
		m_fTarget[2] += z;

		ChangeKey(nTime, bAnimation, bAddKey, m_fEye, CK_EYE);
		ChangeKey(nTime, bAnimation, bAddKey, m_fTarget, CK_TARGET);
	}
	else
	{
		if (IsEyeSelected())
		{
			m_fEye[0] += x;
			m_fEye[1] += y;
			m_fEye[2] += z;

			ChangeKey(nTime, bAnimation, bAddKey, m_fEye, CK_EYE);
		}

		if (IsTargetSelected())
		{
			m_fTarget[0] += x;
			m_fTarget[1] += y;
			m_fTarget[2] += z;

			ChangeKey(nTime, bAnimation, bAddKey, m_fTarget, CK_TARGET);
		}

		// Fix the up vector
		Vector upvec(m_fUp), frontvec(m_fTarget[0]-m_fEye[0], m_fTarget[1]-m_fEye[1], m_fTarget[2]-m_fEye[2]), sidevec;
		sidevec.Cross(frontvec, upvec);
		upvec.Cross(sidevec, frontvec);
		upvec.Normalize();
		upvec.ToFloat(m_fUp);

		ChangeKey(nTime, bAnimation, bAddKey, m_fUp, CK_UP);
	}
}

void Camera::RemoveKeys()
{
	CAMERA_KEY *node, *prev;

	for (node = m_pInstructionKeys; node;)
	{
		prev = node;
		node = node->next;
		free (prev);
	}

	for (node = m_pAnimationKeys; node;)
	{
		prev = node;
		node = node->next;
		free (prev);
	}
}

void Camera::CalculatePosition(unsigned short nTime, bool bAnimation, float eye[3], float target[3], float up[3])
{
	CAMERA_KEY *node, *pe = NULL, *ne = NULL, *pt = NULL, *nt = NULL, *pu = NULL, *nu = NULL;
	if (bAnimation)
		node = m_pAnimationKeys;
	else
		node = m_pInstructionKeys;

	while (node && (!ne || !nt || !nu))
	{
		if (node->time <= nTime)
		{
			switch (node->type)
			{
			case CK_EYE:	pe = node; break;
			case CK_TARGET: pt = node; break;
			case CK_UP:		pu = node; break;
			}
		}
		else
		{
			switch (node->type)
			{
			case CK_EYE:	if (ne == NULL) ne = node; break;
			case CK_TARGET: if (nt == NULL) nt = node; break;
			case CK_UP:		if (nu == NULL) nu = node; break;
			}
		}

		node = node->next;
	}

	// TODO: USE KEY IN/OUT WEIGHTS
	if (bAnimation && (ne != NULL) && (pe->time != nTime))
	{
		float t = (float)(nTime - pe->time)/(ne->time - pe->time);
		eye[0] = pe->param[0] + (ne->param[0] - pe->param[0])*t;
		eye[1] = pe->param[1] + (ne->param[1] - pe->param[1])*t;
		eye[2] = pe->param[2] + (ne->param[2] - pe->param[2])*t;
	}
	else
		memcpy (eye, pe->param, sizeof(float[3]));

	if (bAnimation && (nt != NULL) && (pt->time != nTime))
	{
		float t = (float)(nTime - pt->time)/(nt->time - pt->time);
		target[0] = pt->param[0] + (nt->param[0] - pt->param[0])*t;
		target[1] = pt->param[1] + (nt->param[1] - pt->param[1])*t;
		target[2] = pt->param[2] + (nt->param[2] - pt->param[2])*t;
	}
	else
		memcpy (target, pt->param, sizeof(float[3]));

	if (bAnimation && (nu != NULL) && (pu->time != nTime))
	{
		float t = (float)(nTime - pu->time)/(nu->time - pu->time);
		up[0] = pu->param[0] + (nu->param[0] - pu->param[0])*t;
		up[1] = pu->param[1] + (nu->param[1] - pu->param[1])*t;
		up[2] = pu->param[2] + (nu->param[2] - pu->param[2])*t;
	}
	else
		memcpy (up, pu->param, sizeof(float[3]));

	// Fix the up vector
	Vector upvec(up), frontvec(eye[0]-target[0], eye[1]-target[1], eye[2]-target[2]), sidevec;
	sidevec.Cross(frontvec, upvec);
	upvec.Cross(sidevec, frontvec);
	upvec.Normalize();
	upvec.ToFloat(up);
}

void Camera::UpdatePosition(unsigned short nTime, bool bAnimation)
{
	CalculatePosition(nTime, bAnimation, m_fEye, m_fTarget, m_fUp);

	float len;
	Vector upvec(m_fUp), frontvec(m_fTarget[0]-m_fEye[0], m_fTarget[1]-m_fEye[1], m_fTarget[2]-m_fEye[2]), sidevec;
	len = frontvec.Length();
	sidevec.Cross(frontvec, upvec);
	sidevec.Normalize();
	upvec.Normalize();
	frontvec.Normalize();

	double m[16], inverse[16];
#define M(row,col)  m[col*4+row]
	M(0,0) = sidevec.X();  M(0,1) = sidevec.Y();  M(0,2) = sidevec.Z();  M(0,3) = 0.0;
	M(1,0) = upvec.X();    M(1,1) = upvec.Y();    M(1,2) = upvec.Z();    M(1,3) = 0.0;
	M(2,0) = frontvec.X(); M(2,1) = frontvec.Y(); M(2,2) = frontvec.Z(); M(2,3) = 0.0;
	M(3,0) = 0.0;          M(3,1) = 0.0;          M(3,2) = 0.0;          M(3,3) = 1.0;
#undef M
	invert(m, inverse);

	Matrix mat(inverse);
	mat.SetTranslation(m_fEye[0], m_fEye[1], m_fEye[2]);
	m_BoundingBox.CalculateBoundingBox(&mat);
	mat.SetTranslation(m_fTarget[0], m_fTarget[1], m_fTarget[2]);
	m_TargetBoundingBox.CalculateBoundingBox(&mat);

	if (m_nList == 0)
		m_nList = glGenLists(1);

	glNewList(m_nList, GL_COMPILE);
		
	glPushMatrix();
	glTranslatef(m_fEye[0], m_fEye[1], m_fEye[2]);
	glMultMatrixd(inverse);

	glEnableClientState(GL_VERTEX_ARRAY);
	float verts[34][3] = {
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
		{ -0.3f, -0.3f,  0.6f }, { -0.3f,  0.3f,  0.6f },
		{  0.0f,  0.0f,  0.3f }, { -0.3f, -0.3f,  0.6f },
		{  0.3f, -0.3f,  0.6f }, {  0.0f,  0.0f,  0.3f },
		{  0.3f,  0.3f,  0.6f }, {  0.3f, -0.3f,  0.6f },
		{  0.3f,  0.3f,  0.6f }, { -0.3f,  0.3f,  0.6f } };
	glVertexPointer (3, GL_FLOAT, 0, verts);
	glDrawArrays(GL_LINES, 0, 24);
	glDrawArrays(GL_LINE_STRIP, 24, 10);

//	glBegin(GL_LINES);
//	glVertex3f(0,0,0);
//	glVertex3f(0,0,len);
//	glEnd();

	glTranslatef(0, 0, len);

	glEndList();

	if (_nTargetList == 0)
	{
		_nTargetList = glGenLists(1);
		glNewList(_nTargetList, GL_COMPILE);

		glEnableClientState(GL_VERTEX_ARRAY);
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
		glDrawArrays(GL_LINES, 0, 24);
		glPopMatrix();
		glEndList();
	}
}

void Camera::Render(float fLineWidth)
{
	if (IsEyeSelected())
	{
		glLineWidth(fLineWidth*2);
		glColor3ubv(FlatColorArray[(m_nState & LC_CAMERA_FOCUSED) != 0 ? LC_COL_FOCUSED : LC_COL_SELECTED]);
		glCallList(m_nList);
		glLineWidth(fLineWidth);
	}
	else
	{
		glColor3f(0.5f, 0.8f, 0.5f);
		glCallList(m_nList);
	}

	if (IsTargetSelected())
	{
		glLineWidth(fLineWidth*2);
		glColor3ubv(FlatColorArray[(m_nState & LC_CAMERA_TARGET_FOCUSED) != 0 ? LC_COL_FOCUSED : LC_COL_SELECTED]);
		glCallList(_nTargetList);
		glLineWidth(fLineWidth);
	}
	else
	{
		glColor3f(0.5f, 0.8f, 0.5f);
		glCallList(_nTargetList);
	}

	glColor3f(0.5f, 0.8f, 0.5f);
	glBegin(GL_LINES);
	glVertex3fv(m_fEye);
	glVertex3fv(m_fTarget);
	glEnd();

	if (IsSelected())
	{
		double projection[16], modelview[16], inverse[16];

		float len;
		Vector frontvec(m_fTarget[0]-m_fEye[0], m_fTarget[1]-m_fEye[1], m_fTarget[2]-m_fEye[2]);
		len = frontvec.Length();

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		gluPerspective(m_fovy, 1.33f, 0.01, len);
		glGetDoublev(GL_PROJECTION_MATRIX, projection);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		gluLookAt(m_fEye[0], m_fEye[1], m_fEye[2], m_fTarget[0], m_fTarget[1], m_fTarget[2], m_fUp[0], m_fUp[1], m_fUp[2]);
		glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
		glPopMatrix();

		glPushMatrix();
		invert(modelview, inverse);
		glMultMatrixd(inverse);
		invert(projection, inverse);
		glMultMatrixd(inverse);

		// draw the viewing frustum
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

void Camera::MinIntersectDist(CLICKLINE* pLine)
{
	double dist;

	if (m_nState & LC_CAMERA_HIDDEN)
		return;

	dist = m_BoundingBox.FindIntersectDist(pLine);

	if (dist < pLine->mindist)
	{
		pLine->mindist = dist;
		pLine->pClosest = &m_BoundingBox;
	}
	
	dist = m_TargetBoundingBox.FindIntersectDist(pLine);

	if (dist < pLine->mindist)
	{
		pLine->mindist = dist;
		pLine->pClosest = &m_TargetBoundingBox;
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
		gluPerspective(m_fovy, fAspect, m_zNear, m_zFar);
/*
		ymax = 10;//(m_zFar-m_zNear)*tan(DTOR*m_fovy)/3;
		ymin = -ymax;
		xmin = ymin * fAspect;
		xmax = ymax * fAspect;
		znear = -60;
		zfar = 60;
		glOrtho(xmin, xmax, ymin, ymax, znear, zfar);
*/
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(m_fEye[0], m_fEye[1], m_fEye[2], m_fTarget[0], m_fTarget[1], m_fTarget[2], m_fUp[0], m_fUp[1], m_fUp[2]);
}

void Camera::DoZoom(int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey)
{
	Vector frontvec(m_fEye[0]-m_fTarget[0], m_fEye[1]-m_fTarget[1], m_fEye[2]-m_fTarget[2]);
	frontvec.Normalize();
	frontvec *= 2.0f*dy/(21-mouse);

	// TODO: option to move eye, target or both
	m_fEye[0] += frontvec.X();
	m_fEye[1] += frontvec.Y();
	m_fEye[2] += frontvec.Z();
	m_fTarget[0] += frontvec.X();
	m_fTarget[1] += frontvec.Y();
	m_fTarget[2] += frontvec.Z();

	ChangeKey(nTime, bAnimation, bAddKey, m_fEye, CK_EYE);
	ChangeKey(nTime, bAnimation, bAddKey, m_fTarget, CK_TARGET);
	UpdatePosition(nTime, bAnimation);
}

void Camera::DoPan(int dx, int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey)
{
	Vector upvec(m_fUp), frontvec(m_fEye[0]-m_fTarget[0], m_fEye[1]-m_fTarget[1], m_fEye[2]-m_fTarget[2]), sidevec;
	sidevec.Cross(frontvec, upvec);
	sidevec.Normalize();
	sidevec *= 2.0f*dx/(21-mouse);
	upvec.Normalize();
	upvec *= -2.0f*dy/(21-mouse);

	m_fEye[0] += upvec.X() + sidevec.X();
	m_fEye[1] += upvec.Y() + sidevec.Y();
	m_fEye[2] += upvec.Z() + sidevec.Z();
	m_fTarget[0] += upvec.X() + sidevec.X();
	m_fTarget[1] += upvec.Y() + sidevec.Y();
	m_fTarget[2] += upvec.Z() + sidevec.Z();

	ChangeKey(nTime, bAnimation, bAddKey, m_fEye, CK_EYE);
	ChangeKey(nTime, bAnimation, bAddKey, m_fTarget, CK_TARGET);
	UpdatePosition(nTime, bAnimation);
}

void Camera::DoRotate(int dx, int dy, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey, float* /*center*/)
{
	Vector upvec(m_fUp), frontvec(m_fEye[0]-m_fTarget[0], m_fEye[1]-m_fTarget[1], m_fEye[2]-m_fTarget[2]), sidevec;
	sidevec.Cross(frontvec, upvec);
	sidevec.Normalize();
	sidevec *= 2.0f*dx/(21-mouse);
	upvec.Normalize();
	upvec *= -2.0f*dy/(21-mouse);

	// TODO: option to move eye or target
	float len = frontvec.Length();
	frontvec.Add(upvec.X() + sidevec.X(), upvec.Y() + sidevec.Y(), upvec.Z() + sidevec.Z());
	frontvec.Normalize();
	frontvec *= len;
	frontvec.Add(m_fTarget);
	frontvec.ToFloat(m_fEye);

	// Calculate new up
	upvec.FromFloat(m_fUp);
	frontvec.FromFloat(m_fEye[0]-m_fTarget[0], m_fEye[1]-m_fTarget[1], m_fEye[2]-m_fTarget[2]);
	sidevec.Cross(frontvec, upvec);
	upvec.Cross(sidevec, frontvec);
	upvec.Normalize();
	upvec.ToFloat(m_fUp);

	ChangeKey(nTime, bAnimation, bAddKey, m_fEye, CK_EYE);
	ChangeKey(nTime, bAnimation, bAddKey, m_fUp, CK_UP);
	UpdatePosition(nTime, bAnimation);
}

void Camera::DoRoll(int dx, int mouse, unsigned short nTime, bool bAnimation, bool bAddKey)
{
	Matrix mat;
	float front[3] = { m_fEye[0]-m_fTarget[0], m_fEye[1]-m_fTarget[1], m_fEye[2]-m_fTarget[2] };

	mat.FromAxisAngle(front, 2.0f*dx/(21-mouse));
	mat.TransformPoints(m_fUp, 1);

	ChangeKey(nTime, bAnimation, bAddKey, m_fUp, CK_UP);
	UpdatePosition(nTime, bAnimation);
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
