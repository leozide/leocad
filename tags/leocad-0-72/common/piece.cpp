// A piece object in the LeoCAD world.
//

#ifdef LC_WINDOWS
#include "stdafx.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "matrix.h"
#include "pieceinf.h"
#include "boundbox.h"
#include "texture.h"
#include "piece.h"
#include "group.h"
#include "project.h"

/////////////////////////////////////////////////////////////////////////////
// Static functions

static PIECE_KEY* AddNode (PIECE_KEY *node, unsigned short nTime, unsigned char nType)
{
	PIECE_KEY* newnode = (PIECE_KEY*)malloc(sizeof(PIECE_KEY));

	if (node)
	{
		newnode->next = node->next;
		node->next = newnode;
	}
	else
		newnode->next = NULL;

	newnode->type = nType;
	newnode->time = nTime;
	newnode->param[0] = newnode->param[1] = newnode->param[2] = newnode->param[3] = 0;

	if (nType == PK_ROTATION)
		newnode->param[2] = 1;

	return newnode;
}

inline static void SetCurrentColor(unsigned char nColor, bool* bTrans, bool bLighting, bool bNoAlpha)
{
	if (bLighting || !bNoAlpha)
		glColor4ubv(ColorArray[nColor]);
	else
		glColor3ubv(FlatColorArray[nColor]);

	if (nColor > 27)
		return;

	if (bNoAlpha)
	{
		if (nColor > 13 && nColor < 22)
		{
			if (!*bTrans)
			{
				*bTrans = true;
				glEnable(GL_POLYGON_STIPPLE);
			}
		}
		else
		{
			if (*bTrans)
			{
				*bTrans = false;
				glDisable(GL_POLYGON_STIPPLE);
			}
		}
	}
	else
	{
		if (nColor > 13 && nColor < 22)
		{
			if (!*bTrans)
			{
				*bTrans = true;
				glEnable(GL_BLEND);
				glDepthMask(GL_FALSE);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
		}
		else
		{
			if (*bTrans)
			{
				*bTrans = false;
				glDepthMask(GL_TRUE);
				glDisable(GL_BLEND);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Piece construction/destruction

static bool lockarrays = false;

#ifdef LC_WINDOWS
typedef void (APIENTRY * GLLOCKARRAYSEXTPROC) (GLint first, GLsizei count);
typedef void (APIENTRY * GLUNLOCKARRAYSEXTPROC) ();
static GLLOCKARRAYSEXTPROC glLockArraysEXT = NULL;
static GLUNLOCKARRAYSEXTPROC glUnlockArraysEXT = NULL;
#else
// TODO: get the entry point under different OS's
#define glLockArraysEXT(a,b) {}
#define glUnlockArraysEXT() {}
#endif

Piece::Piece(PieceInfo* pPieceInfo)
{
#ifdef LC_WINDOWS
	static bool first_time = true;

	if (first_time)
	{
		first_time = false;
		char* extensions = (char*)glGetString(GL_EXTENSIONS);

		if (strstr(extensions, "GL_EXT_compiled_vertex_array "))
		{
			glLockArraysEXT = (GLLOCKARRAYSEXTPROC)wglGetProcAddress("glLockArraysEXT");
			glUnlockArraysEXT = (GLUNLOCKARRAYSEXTPROC)wglGetProcAddress("glUnlockArraysEXT");
			lockarrays = true;
		}
	}
#endif

	m_BoundingBox.Initialize(this, LC_PIECE);
	m_pNext = NULL;
	m_pPieceInfo = pPieceInfo;
	m_nState = 0;
	m_nColor = 0;
	m_pInstructionKeys = NULL;
	m_pAnimationKeys = NULL;
	m_nStepShow = 1;
	m_nStepHide = 255;
	m_nFrameHide = 65535;
	memset(m_strName, 0, sizeof(m_strName));
	m_pGroup = NULL;
	m_pDrawInfo = NULL;
	m_pConnections = NULL;

	if (m_pPieceInfo != NULL)
	{
		m_nBoxList = m_pPieceInfo->AddRef();
		if (m_pPieceInfo->m_nConnectionCount > 0)
		{
			m_pConnections = (CONNECTION*)malloc(sizeof(CONNECTION)*(m_pPieceInfo->m_nConnectionCount));
			
			for (int i = 0; i < m_pPieceInfo->m_nConnectionCount; i++)
			{
				m_pConnections[i].type = m_pPieceInfo->m_pConnections[i].type;
				m_pConnections[i].link = NULL;
				m_pConnections[i].owner = this;
			}
		}
	}
}

Piece::~Piece()
{
	RemoveKeys();
	if (m_pPieceInfo != NULL)
		m_pPieceInfo->DeRef();
	if (m_pDrawInfo != NULL)
		free(m_pDrawInfo);
	if (m_pConnections != NULL)
		free(m_pConnections);
}

/////////////////////////////////////////////////////////////////////////////
// Piece save/load

// Use only when loading from a file
void Piece::SetPieceInfo(PieceInfo* pPieceInfo)
{
	m_pPieceInfo = pPieceInfo;
	m_nBoxList = m_pPieceInfo->AddRef();

	if (m_pPieceInfo->m_nConnectionCount > 0)
	{
		m_pConnections = (CONNECTION*)malloc(sizeof(CONNECTION)*(m_pPieceInfo->m_nConnectionCount));

		for (int i = 0; i < m_pPieceInfo->m_nConnectionCount; i++)
		{
			m_pConnections[i].type = m_pPieceInfo->m_pConnections[i].type;
			m_pConnections[i].link = NULL;
			m_pConnections[i].owner = this;
		}
	}
}

void Piece::FileLoad(File* file, char* name)
{
	PIECE_KEY *node;
	unsigned char version, ch;

	file->ReadByte(&version, 1);

	if (version > 5)
	{
		unsigned long keys;

		file->ReadLong(&keys, 1);
		for (node = NULL; keys--;)
		{
			if (node == NULL)
			{
				m_pInstructionKeys = AddNode(NULL, 1, PK_POSITION);
				node = m_pInstructionKeys;
			}
			else
				node = AddNode(node, 1, PK_POSITION);

			file->Read(node->param, 16);
			file->ReadShort(&node->time, 1);
			file->ReadByte(&node->type, 1);
		}

		file->ReadLong(&keys, 1);
		for (node = NULL; keys--;)
		{
			if (node == NULL)
			{
				m_pAnimationKeys = AddNode(NULL, 1, PK_POSITION);
				node = m_pAnimationKeys;
			}
			else
				node = AddNode(node, 1, PK_POSITION);

			file->Read(node->param, 16);
			file->ReadShort(&node->time, 1);
			file->ReadByte(&node->type, 1);
		}
	}
	else
	{
		if (version > 2)
		{
			file->Read(&ch, 1);

			for (node = NULL; ch--;)
			{
				if (node == NULL)
				{
					m_pInstructionKeys = AddNode(NULL, 1, PK_POSITION);
					node = m_pInstructionKeys;
				}
				else
					node = AddNode(node, 1, PK_POSITION);

				Matrix mat;
				if (version > 3)
				{
					float m[16];
					file->Read(m, sizeof(m));
					mat.FromFloat(m);
				}
				else
				{
					float move[3], rotate[3];
					file->Read(move, sizeof(float[3]));
					file->Read(rotate, sizeof(float[3]));
					mat.CreateOld(move[0], move[1], move[2], rotate[0], rotate[1], rotate[2]);
				}

				unsigned char b;
				file->ReadByte(&b, 1);
				node->time = b;
				mat.GetTranslation(&node->param[0], &node->param[1], &node->param[2]);
				node = AddNode(node, b, PK_ROTATION);
				mat.ToAxisAngle(node->param);
				
				int bl;
				file->ReadLong(&bl, 1);
			}
		}
		else
		{
			Matrix mat;
			float move[3], rotate[3];
			file->Read(move, sizeof(float[3]));
			file->Read(rotate, sizeof(float[3]));
			mat.CreateOld(move[0], move[1], move[2], rotate[0], rotate[1], rotate[2]);

			m_pInstructionKeys = AddNode (NULL, 1, PK_POSITION);
			mat.GetTranslation(&m_pInstructionKeys->param[0], &m_pInstructionKeys->param[1], &m_pInstructionKeys->param[2]);
			AddNode(m_pInstructionKeys, 1, PK_ROTATION);
			mat.ToAxisAngle(m_pInstructionKeys->next->param);
		}

		m_pAnimationKeys = AddNode (NULL, 1, PK_POSITION);
		AddNode(m_pAnimationKeys, 1, PK_ROTATION);
		memcpy(m_pAnimationKeys->param, m_pInstructionKeys->param, sizeof(float[4]));
		memcpy(m_pAnimationKeys->next->param, m_pInstructionKeys->next->param, sizeof(float[4]));
	}

	// Common to all versions.
	file->Read(name, 9);
	file->ReadByte(&m_nColor, 1);

	if (version < 5)
	{
		const unsigned char conv[20] = { 0,2,4,9,7,6,22,8,10,11,14,16,18,9,21,20,22,8,10,11 };
		m_nColor = conv[m_nColor];
	}

	file->ReadByte(&m_nStepShow, 1);
	if (version > 1)
		file->ReadByte(&m_nStepHide, 1);
	else
		m_nStepHide = 255;

	if (version > 5)
	{
		file->ReadShort(&m_nFrameShow, 1);
		file->ReadShort(&m_nFrameHide, 1);

		if (version > 7)
		{
			file->ReadByte(&m_nState, 1);
			UnSelect();
			file->ReadByte(&ch, 1);
			file->Read(m_strName, ch);
		}
		else
		{
			int hide;
			file->ReadLong(&hide, 1);
			if (hide != 0)
				m_nState |= LC_PIECE_HIDDEN;
			file->Read(m_strName, 81);
		}

		// 7 (0.64)
		int i = -1;
		if (version > 6)
			file->ReadLong(&i, 1);
		m_pGroup = (Group*)i;
	}
	else
	{
		m_nFrameShow = 1;
		m_nFrameHide = 65535;

		file->ReadByte(&ch, 1);
		if (ch == 0)
			m_pGroup = (Group*)-1;
		else
			m_pGroup = (Group*)ch;

		file->ReadByte(&ch, 1);
		if (ch & 0x01)
			m_nState |= LC_PIECE_HIDDEN;
	}
}

void Piece::FileSave(File* file, Group* pGroups)
{
	PIECE_KEY *node;
	unsigned char ch = 8; // LeoCAD 0.70
	unsigned long n;

	file->WriteByte(&ch, 1);

	for (n = 0, node = m_pInstructionKeys; node; node = node->next)
		n++;
	file->WriteLong(&n, 1);

	for (node = m_pInstructionKeys; node; node = node->next)
	{
		file->Write(node->param, 16);
		file->WriteShort(&node->time, 1);
		file->WriteByte(&node->type, 1);
	}

	for (n = 0, node = m_pAnimationKeys; node; node = node->next)
		n++;
	file->WriteLong(&n, 1);

	for (node = m_pAnimationKeys; node; node = node->next)
	{
		file->Write(node->param, 16);
		file->WriteShort(&node->time, 1);
		file->WriteByte(&node->type, 1);
	}

	file->Write(m_pPieceInfo->m_strName, 9);

	file->WriteByte(&m_nColor, 1);
	file->WriteByte(&m_nStepShow, 1);
	file->WriteByte(&m_nStepHide, 1);
	file->WriteShort(&m_nFrameShow, 1);
	file->WriteShort(&m_nFrameHide, 1);

	// version 8
	file->WriteByte(&m_nState, 1);
	ch = strlen(m_strName);
	file->WriteByte(&ch, 1);
	file->Write(m_strName, ch);

	// version 7
	int i;
	if (m_pGroup != NULL)
	{
		for (i = 0; pGroups; pGroups = pGroups->m_pNext)
		{
			if (m_pGroup == pGroups)
				break;
			i++;
		}
	}
	else
		i = -1;
	file->WriteLong(&i, 1);
}

void Piece::Initialize(float x, float y, float z, unsigned char nStep, unsigned short nFrame, unsigned char nColor)
{
	m_nFrameShow = nFrame;
	m_nStepShow = nStep;

	m_pAnimationKeys = AddNode (NULL, 1, PK_POSITION);
	AddNode(m_pAnimationKeys, 1, PK_ROTATION);
	m_pAnimationKeys->param[0] = x;
	m_pAnimationKeys->param[1] = y;
	m_pAnimationKeys->param[2] = z;

	m_pInstructionKeys = AddNode (NULL, 1, PK_POSITION);
	AddNode(m_pInstructionKeys, 1, PK_ROTATION);
	m_pInstructionKeys->param[0] = x;
	m_pInstructionKeys->param[1] = y;
	m_pInstructionKeys->param[2] = z;

	UpdatePosition(1, false);

	m_nColor = nColor;
}

void Piece::CreateName(Piece* pPiece)
{
	int i, max = 0;

	for (; pPiece; pPiece = pPiece->m_pNext)
		if (strncmp (pPiece->m_strName, m_pPieceInfo->m_strDescription, strlen(m_pPieceInfo->m_strDescription)) == 0)
			if (sscanf(pPiece->m_strName + strlen(m_pPieceInfo->m_strDescription), " #%d", &i) == 1)
				if (i > max) 
					max = i;

	sprintf (m_strName, "%s #%.2d", m_pPieceInfo->m_strDescription, max+1);
}

void Piece::LineFacet(float* p1, float* p2, float* p3, float* p4, CLICKLINE* pLine)
{
	double t, t1, t2, x, y, z, plane[4];
	plane[0] = ((p1[1]-p2[1])*(p3[2]-p2[2])) - ((p1[2]-p2[2])*(p3[1]-p2[1]));
	plane[1] = ((p1[2]-p2[2])*(p3[0]-p2[0])) - ((p1[0]-p2[0])*(p3[2]-p2[2])); 
	plane[2] = ((p1[0]-p2[0])*(p3[1]-p2[1])) - ((p1[1]-p2[1])*(p3[0]-p2[0]));
	plane[3] = -(plane[0]*p1[0]) -(plane[1]*p1[1]) -(plane[2]*p1[2]);
	t1 = (plane[0]*pLine->a1 + plane[1]*pLine->b1 + plane[2]*pLine->c1 + plane[3]);
	t2 = (plane[0]*pLine->a2 + plane[1]*pLine->b2 + plane[2]*pLine->c2);

	if (t1 != 0 && t2 != 0)
	{
		t = -(t1 / t2);
		if (t >= 0)
		{
			x = pLine->a1+pLine->a2*t;
			y = pLine->b1+pLine->b2*t;
			z = pLine->c1+pLine->c2*t;

			if (fabs(plane[0]*x + plane[1]*y + plane[2]*z + plane[3]) <= 0.001)
			{
				double dist = sqrt((pLine->a1-x)*(pLine->a1-x)+(pLine->b1-y)*(pLine->b1-y)+(pLine->c1-z)*(pLine->c1-z));

				if (dist < pLine->mindist)
				{
					double pa1[3], pa2[3], pa3[3], a1, a2, a3, inv;
					pa1[0] = p1[0] - x;
					pa1[1] = p1[1] - y;
					pa1[2] = p1[2] - z;
					inv = 1.0/sqrt(pa1[0]*pa1[0] + pa1[1]*pa1[1] + pa1[2]*pa1[2]);
					pa1[0] *= inv;
					pa1[1] *= inv;
					pa1[2] *= inv;
					pa2[0] = p2[0] - x;
					pa2[1] = p2[1] - y;
					pa2[2] = p2[2] - z;
					inv = 1.0/sqrt(pa2[0]*pa2[0] + pa2[1]*pa2[1] + pa2[2]*pa2[2]);
					pa2[0] *= inv;
					pa2[1] *= inv;
					pa2[2] *= inv;
					pa3[0] = p3[0] - x;
					pa3[1] = p3[1] - y;
					pa3[2] = p3[2] - z;
					inv = 1.0/sqrt(pa3[0]*pa3[0] + pa3[1]*pa3[1] + pa3[2]*pa3[2]);
					pa3[0] *= inv;
					pa3[1] *= inv;
					pa3[2] *= inv;
					a1 = pa1[0]*pa2[0] + pa1[1]*pa2[1] + pa1[2]*pa2[2];
					a2 = pa2[0]*pa3[0] + pa2[1]*pa3[1] + pa2[2]*pa3[2];
					a3 = pa3[0]*pa1[0] + pa3[1]*pa1[1] + pa3[2]*pa1[2];
					double total = (acos(a1) + acos(a2) + acos(a3)) * RTOD;

					if (fabs(total - 360) > 0.001) // Outside triangle
					{
						if (p4 != NULL)
						{
							pa2[0] = p4[0] - x;
							pa2[1] = p4[1] - y;
							pa2[2] = p4[2] - z;
							inv = 1.0/sqrt(pa2[0]*pa2[0] + pa2[1]*pa2[1] + pa2[2]*pa2[2]);
							pa2[0] *= inv;
							pa2[1] *= inv;
							pa2[2] *= inv;
							a1 = pa1[0]*pa2[0] + pa1[1]*pa2[1] + pa1[2]*pa2[2];
							a2 = pa2[0]*pa3[0] + pa2[1]*pa3[1] + pa2[2]*pa3[2];
							a3 = pa3[0]*pa1[0] + pa3[1]*pa1[1] + pa3[2]*pa1[2];
							total = (acos(a1) + acos(a2) + acos(a3)) * RTOD;
							
							if (fabs(total - 360) > 0.001)
								return; // Outside other triangle
						}
						else
							return;
					}

					pLine->mindist = dist;
					pLine->pClosest = &m_BoundingBox;
				}
			}
		}
	}
}

void Piece::MinIntersectDist(CLICKLINE* pLine)
{
	double dist;

	dist = m_BoundingBox.FindIntersectDist(pLine);
	if (dist >= pLine->mindist)
		return;

	Matrix mat(m_fRotation, m_fPosition);
	float* verts = (float*)malloc(sizeof(float)*3*m_pPieceInfo->m_nVertexCount);
	memcpy(verts, m_pPieceInfo->m_fVertexArray, sizeof(float)*3*m_pPieceInfo->m_nVertexCount);
	mat.TransformPoints(verts, m_pPieceInfo->m_nVertexCount);

	if (m_pPieceInfo->m_nFlags & LC_PIECE_LONGDATA)
	{
		unsigned long* info = (unsigned long*)m_pDrawInfo, colors, i;
		colors = *info;
		info++;

		while (colors--)
		{
			info++;
			for (i = 0; i < *info; i += 4)
				LineFacet(&verts[info[i+1]*3], &verts[info[i+2]*3], 
					&verts[info[i+3]*3], &verts[info[i+4]*3], pLine);
			info += *info + 1;
			for (i = 0; i < *info; i += 3)
				LineFacet(&verts[info[i+1]*3], &verts[info[i+2]*3], 
					&verts[info[i+3]*3], NULL, pLine);
			info += *info + 1;
			info += *info + 1;
		}
	}
	else
	{
		unsigned short* info = (unsigned short*)m_pDrawInfo, colors, i;
		colors = *info;
		info++;

		while (colors--)
		{
			info++;
			for (i = 0; i < *info; i += 4)
				LineFacet(&verts[info[i+1]*3], &verts[info[i+2]*3], 
					&verts[info[i+3]*3], &verts[info[i+4]*3], pLine);
			info += *info + 1;
			for (i = 0; i < *info; i += 3)
				LineFacet(&verts[info[i+1]*3], &verts[info[i+2]*3], 
					&verts[info[i+3]*3], NULL, pLine);
			info += *info + 1;
			info += *info + 1;
		}
	}

	free(verts);
}

void Piece::Move(unsigned short nTime, bool bAnimation, bool bAddKey, float x, float y, float z)
{
	m_fPosition[0] += x;
	m_fPosition[1] += y;
	m_fPosition[2] += z;

	ChangeKey(nTime, bAnimation, bAddKey, m_fPosition, PK_POSITION);
}

void Piece::ChangeKey(unsigned short nTime, bool bAnimation, bool bAddKey, float* param, unsigned char nKeyType)
{
	PIECE_KEY *node, *poskey = NULL, *newpos = NULL;
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

	if (nKeyType == PK_ROTATION)
		newpos->param[3] = param[3];
}

void Piece::RemoveKeys()
{
	PIECE_KEY *node, *prev;

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

bool Piece::IsVisible(unsigned short nTime, bool bAnimation)
{
	if (m_nState & LC_PIECE_HIDDEN)
		return false;

	if (bAnimation)
	{
		if (m_nFrameShow > nTime) return false;
		if (m_nFrameHide < nTime) return false;
		return true;
	}
	else
	{
		if (m_nStepShow > nTime) return false;
		if ((m_nStepHide == 255) || (m_nStepHide > nTime))
			return true;
		return false;
	}
}

void Piece::CompareBoundingBox(float box[6])
{
	float v[24] = {
		m_pPieceInfo->m_fDimensions[0], m_pPieceInfo->m_fDimensions[1], m_pPieceInfo->m_fDimensions[5],
		m_pPieceInfo->m_fDimensions[3], m_pPieceInfo->m_fDimensions[1], m_pPieceInfo->m_fDimensions[5],
		m_pPieceInfo->m_fDimensions[0], m_pPieceInfo->m_fDimensions[1], m_pPieceInfo->m_fDimensions[2],
		m_pPieceInfo->m_fDimensions[3], m_pPieceInfo->m_fDimensions[4], m_pPieceInfo->m_fDimensions[5],
		m_pPieceInfo->m_fDimensions[3], m_pPieceInfo->m_fDimensions[4], m_pPieceInfo->m_fDimensions[2],
		m_pPieceInfo->m_fDimensions[0], m_pPieceInfo->m_fDimensions[4], m_pPieceInfo->m_fDimensions[2],
		m_pPieceInfo->m_fDimensions[0], m_pPieceInfo->m_fDimensions[4], m_pPieceInfo->m_fDimensions[5],
		m_pPieceInfo->m_fDimensions[3], m_pPieceInfo->m_fDimensions[1], m_pPieceInfo->m_fDimensions[2] };

	Matrix m(m_fRotation, m_fPosition);
	m.TransformPoints(v, 8);

	for (int i = 0; i < 24; i += 3)
	{
		if (v[i]   < box[0]) box[0] = v[i];
		if (v[i+1] < box[1]) box[1] = v[i+1];
		if (v[i+2] < box[2]) box[2] = v[i+2];
		if (v[i]   > box[3]) box[3] = v[i];
		if (v[i+1] > box[4]) box[4] = v[i+1];
		if (v[i+2] > box[5]) box[5] = v[i+2];
	}
}

bool Piece::CalculatePositionRotation(unsigned short nTime, bool bAnimation, float pos[3], float rot[4])
{
	PIECE_KEY *node, *prevpos = NULL, *nextpos = NULL, *prevrot = NULL, *nextrot = NULL;
	if (bAnimation)
		node = m_pAnimationKeys;
	else
		node = m_pInstructionKeys;

	while (node && (!nextpos || !nextrot))
	{
		if (node->time <= nTime)
		{
			if (node->type == PK_POSITION)
				prevpos = node;
			else
				prevrot = node;
		}
		else
		{
			if (node->type == PK_POSITION)
			{
				if (nextpos == NULL)
					nextpos = node;
			}
			else
			{
				if (nextrot == NULL)
					nextrot = node;
			}
		}

		node = node->next;
	}

	if (bAnimation)
	{
		if ((nextpos != NULL) && (prevpos->time != nTime))
		{
			// TODO: USE KEY IN/OUT WEIGHTS
			float t = (float)(nTime - prevpos->time)/(nextpos->time - prevpos->time);
			pos[0] = prevpos->param[0] + (nextpos->param[0] - prevpos->param[0])*t;
			pos[1] = prevpos->param[1] + (nextpos->param[1] - prevpos->param[1])*t;
			pos[2] = prevpos->param[2] + (nextpos->param[2] - prevpos->param[2])*t;
		}
		else
			memcpy (pos, prevpos->param, sizeof(float[3]));

		if ((nextrot != NULL) && (prevrot->time != nTime))
		{
			// TODO: USE KEY IN/OUT WEIGHTS
			float t = (float)(nTime - prevrot->time)/(nextrot->time - prevrot->time);
			rot[0] = prevrot->param[0] + (nextrot->param[0] - prevrot->param[0])*t;
			rot[1] = prevrot->param[1] + (nextrot->param[1] - prevrot->param[1])*t;
			rot[2] = prevrot->param[2] + (nextrot->param[2] - prevrot->param[2])*t;
			rot[3] = prevrot->param[3] + (nextrot->param[3] - prevrot->param[3])*t;
		}
		else
			memcpy (rot, prevrot->param, sizeof(float[4]));
	}
	else
	{
		if (memcmp(pos, prevpos->param, sizeof(float[3])) ||
			memcmp(rot, prevrot->param, sizeof(float[4])))
		{
			memcpy (pos, prevpos->param, sizeof(float[3]));
			memcpy (rot, prevrot->param, sizeof(float[4]));
		}
		else
			return false;
	}

	return true;
}

Group* Piece::GetTopGroup()
{
	return m_pGroup ? m_pGroup->GetTopGroup() : NULL;
}

void Piece::DoGroup(Group* pGroup)
{
	if (m_pGroup != NULL && m_pGroup != (Group*)-1 && m_pGroup > (Group*)0xFFFF)
		m_pGroup->SetGroup(pGroup);
	else
		m_pGroup = pGroup;
}

void Piece::UnGroup(Group* pGroup)
{
	if ((m_pGroup == pGroup) || (pGroup == NULL))
		m_pGroup = NULL;
	else
		if (m_pGroup != NULL)
			m_pGroup->UnGroup(pGroup);
}

// Recalculates current position and connections
void Piece::UpdatePosition(unsigned short nTime, bool bAnimation)
{
	if (!IsVisible(nTime, bAnimation))
		m_nState &= ~(LC_PIECE_SELECTED|LC_PIECE_FOCUSED);

	CalculatePositionRotation(nTime, bAnimation, m_fPosition, m_fRotation);
//	if (CalculatePositionRotation(nTime, bAnimation, m_fPosition, m_fRotation))
	{
		Matrix mat(m_fRotation, m_fPosition);
		m_BoundingBox.CalculateBoundingBox(&mat, m_pPieceInfo->m_fDimensions);
		for (int i = 0; i < m_pPieceInfo->m_nConnectionCount; i++)
		{
			mat.TransformPoint(m_pConnections[i].center, m_pPieceInfo->m_pConnections[i].center);

			// TODO: rotate normal
		}
	}
}

void Piece::BuildDrawInfo()
{
	if (m_pDrawInfo != NULL)
	{
		free(m_pDrawInfo);
		m_pDrawInfo = NULL;
	}

	DRAWGROUP* dg;
	bool add;
	unsigned short group, colcount, i, j;
	unsigned long count[LC_COL_DEFAULT+1][3], vert;
	memset (count, 0, sizeof(count));

	// Get the vertex count
	for (group = m_pPieceInfo->m_nGroupCount, dg = m_pPieceInfo->m_pGroups; group--; dg++)
	{
		unsigned short* sh = dg->connections;
		add = IsTransparent() || *sh == 0xFFFF;

		if (!add)
			for (; *sh != 0xFFFF; sh++)
				if ((m_pConnections[*sh].link == NULL) ||
					(m_pConnections[*sh].link->owner->IsTransparent()))
					{
						add = true;
						break;
					}

		if (add)
		{
			if (m_pPieceInfo->m_nFlags & LC_PIECE_LONGDATA)
			{
				unsigned long* p, curcol, colors;
				p = (unsigned long*)dg->drawinfo;
				colors = *p;
				p++;

				while (colors--)
				{
					curcol = *p;
					p++;
					count[curcol][0] += *p;
					p += *p + 1;
					count[curcol][1] += *p;
					p += *p + 1;
					count[curcol][2] += *p;
					p += *p + 1;
				}
			}
			else
			{
				unsigned short* p, curcol, colors;
				p = (unsigned short*)dg->drawinfo;
				colors = *p;
				p++;

				while (colors--)
				{
					curcol = *p;
					p++;
					count[curcol][0] += *p;
					p += *p + 1;
					count[curcol][1] += *p;
					p += *p + 1;
					count[curcol][2] += *p;
					p += *p + 1;
				}
			}
		}
	}

	colcount = 0;
	vert = 0;
	for (i = 0; i < LC_COL_DEFAULT+1; i++)
		if (count[i][0] || count[i][1] || count[i][2])
		{
			colcount++;
			vert += count[i][0] + count[i][1] + count[i][2];
		}
	vert += (colcount*4)+1;

	// Build the info
	if (m_pPieceInfo->m_nFlags & LC_PIECE_LONGDATA)
	{
		m_pDrawInfo = malloc(vert*sizeof(unsigned long));
		unsigned long* drawinfo = (unsigned long*)m_pDrawInfo;
		*drawinfo = colcount;
		drawinfo++;
		i = LC_COL_DEFAULT;

		for (i = LC_COL_DEFAULT; i != LC_COL_EDGES+1;)
		{
			if (count[i][0] || count[i][1] || count[i][2])
			{
				*drawinfo = i;
				drawinfo++;

				for (j = 0; j < 3; j++)
				{
					*drawinfo = count[i][j];
					drawinfo++;

					if (count[i][j] == 0)
						continue;

					for (group = m_pPieceInfo->m_nGroupCount, dg = m_pPieceInfo->m_pGroups; group--; dg++)
					{
						unsigned short* sh = dg->connections;
						add = IsTransparent() || *sh == 0xFFFF;

						if (!add)
							for (; *sh != 0xFFFF; sh++)
								if ((m_pConnections[*sh].link == NULL) ||
									(m_pConnections[*sh].link->owner->IsTransparent()))
									{
										add = true;
										break;
									}

						if (!add)
							continue;

						unsigned long* p, colors;
						p = (unsigned long*)dg->drawinfo;
						colors = *p;
						p++;

						while (colors--)
						{
							if (*p == i)
							{
								p++;

								if (j == 0)
								{
									memcpy(drawinfo, p+1, (*p)*sizeof(unsigned long));
									drawinfo += *p;
								}
								p += *p + 1;

								if (j == 1)
								{
									memcpy(drawinfo, p+1, (*p)*sizeof(unsigned long));
									drawinfo += *p;
								}
								p += *p + 1;
								
								if (j == 2)
								{
									memcpy(drawinfo, p+1, (*p)*sizeof(unsigned long));
									drawinfo += *p;
								}
								p += *p + 1;
							}
							else
							{
								p++;
								p += *p + 1;
								p += *p + 1;
								p += *p + 1;
							}
						}
					}
				}
			}
						
			if (i == LC_COL_DEFAULT)
				i = 0;
			else
				i++;
		}
	}
	else
	{
		m_pDrawInfo = malloc(vert*sizeof(unsigned short));
		unsigned short* drawinfo = (unsigned short*)m_pDrawInfo;
		*drawinfo = colcount;
		drawinfo++;

		for (i = LC_COL_DEFAULT; i != LC_COL_EDGES+1;)
		{
			if (count[i][0] || count[i][1] || count[i][2])
			{
				*drawinfo = i;
				drawinfo++;

				for (j = 0; j < 3; j++)
				{
					*drawinfo = (unsigned short)count[i][j];
					drawinfo++;

					if (count[i][j] == 0)
						continue;

					for (group = m_pPieceInfo->m_nGroupCount, dg = m_pPieceInfo->m_pGroups; group--; dg++)
					{
						unsigned short* sh = dg->connections;
						add = IsTransparent() || *sh == 0xFFFF;

						if (!add)
							for (; *sh != 0xFFFF; sh++)
								if ((m_pConnections[*sh].link == NULL) ||
									(m_pConnections[*sh].link->owner->IsTransparent()))
									{
										add = true;
										break;
									}

						if (!add)
							continue;

						unsigned short* p, colors;
						p = (unsigned short*)dg->drawinfo;
						colors = *p;
						p++;

						while (colors--)
						{
							if (*p == i)
							{
								p++;

								if (j == 0)
								{
									memcpy(drawinfo, p+1, (*p)*sizeof(unsigned short));
									drawinfo += *p;
								}
								p += *p + 1;

								if (j == 1)
								{
									memcpy(drawinfo, p+1, (*p)*sizeof(unsigned short));
									drawinfo += *p;
								}
								p += *p + 1;
								
								if (j == 2)
								{
									memcpy(drawinfo, p+1, (*p)*sizeof(unsigned short));
									drawinfo += *p;
								}
								p += *p + 1;
							}
							else
							{
								p++;
								p += *p + 1;
								p += *p + 1;
								p += *p + 1;
							}
						}
					}
				}
			}

			if (i == LC_COL_DEFAULT)
				i = 0;
			else
				i++;
		}
	}
}

void Piece::Render(bool bLighting, bool bNoAlpha, bool bEdges, unsigned char* nLastColor, bool* bTrans)
{
	glPushMatrix();
	glTranslatef(m_fPosition[0], m_fPosition[1], m_fPosition[2]);
	glRotatef(m_fRotation[3], m_fRotation[0], m_fRotation[1], m_fRotation[2]);
	glVertexPointer (3, GL_FLOAT, 0, m_pPieceInfo->m_fVertexArray);

//	glEnable(GL_POLYGON_STIPPLE);

	for (int sh = 0; sh < m_pPieceInfo->m_nTextureCount; sh++)
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		m_pPieceInfo->m_pTextures[sh].texture->MakeCurrent();

		if (m_pPieceInfo->m_pTextures[sh].color == LC_COL_DEFAULT)
		{
			SetCurrentColor(m_nColor, bTrans, bLighting, bNoAlpha);
			*nLastColor = m_nColor;
		}

		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glTexCoord2fv(m_pPieceInfo->m_pTextures[sh].coords[0]);
		glVertex3fv(m_pPieceInfo->m_pTextures[sh].vertex[0]);
		glTexCoord2fv(m_pPieceInfo->m_pTextures[sh].coords[1]);
		glVertex3fv(m_pPieceInfo->m_pTextures[sh].vertex[1]);
		glTexCoord2fv(m_pPieceInfo->m_pTextures[sh].coords[2]);
		glVertex3fv(m_pPieceInfo->m_pTextures[sh].vertex[2]);
		glTexCoord2fv(m_pPieceInfo->m_pTextures[sh].coords[3]);
		glVertex3fv(m_pPieceInfo->m_pTextures[sh].vertex[3]);
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}

	if (m_pPieceInfo->m_nFlags & LC_PIECE_LONGDATA)
	{
		unsigned long colors, *info = (unsigned long*)m_pDrawInfo;
		colors = *info;
		info++;

		while (colors--)
		{
			bool lock = lockarrays && (*info == LC_COL_DEFAULT || *info == LC_COL_EDGES);

			if (*info != *nLastColor)
			{
				if (*info == LC_COL_DEFAULT)
				{
					SetCurrentColor(m_nColor, bTrans, bLighting, bNoAlpha);
					*nLastColor = m_nColor;
				}
				else
				{
					SetCurrentColor((unsigned char)*info, bTrans, bLighting, bNoAlpha);
					*nLastColor = (unsigned char)*info;
				}
			}
			info++;

			if (lock)
				glLockArraysEXT(0, m_pPieceInfo->m_nVertexCount);

			if (*info)
			{
				glDrawElements(GL_QUADS, *info, GL_UNSIGNED_INT, info+1);
				info += *info + 1;
			}
			else
				info++;

			if (*info)
			{
				glDrawElements(GL_TRIANGLES, *info, GL_UNSIGNED_INT, info+1);
				info += *info + 1;
			}
			else
				info++;

			if (*info)
			{
				if (m_nState & LC_PIECE_SELECTED)
				{
					SetCurrentColor(m_nState & LC_PIECE_FOCUSED ? LC_COL_FOCUSED : LC_COL_SELECTED, bTrans, bLighting, bNoAlpha);
					*nLastColor = m_nState & LC_PIECE_FOCUSED ? LC_COL_FOCUSED : LC_COL_SELECTED;
					glDrawElements(GL_LINES, *info, GL_UNSIGNED_INT, info+1);
				}
				else
					if (bEdges)
						glDrawElements(GL_LINES, *info, GL_UNSIGNED_INT, info+1);

				info += *info + 1;
			}
			else
				info++;

			if (lock)
				glUnlockArraysEXT();
		}
	}
	else
	{
		unsigned short colors, *info = (unsigned short*)m_pDrawInfo;
		colors = *info;
		info++;

		while (colors--)
		{
			bool lock = lockarrays && (*info == LC_COL_DEFAULT || *info == LC_COL_EDGES);

			if (*info != *nLastColor)
			{
				if (*info == LC_COL_DEFAULT)
				{
					SetCurrentColor(m_nColor, bTrans, bLighting, bNoAlpha);
					*nLastColor = m_nColor;
				}
				else
				{
					SetCurrentColor((unsigned char)*info, bTrans, bLighting, bNoAlpha);
					*nLastColor = (unsigned char)*info;
				}
			}
			info++;

			if (lock)
				glLockArraysEXT(0, m_pPieceInfo->m_nVertexCount);

			if (*info)
			{
				glDrawElements(GL_QUADS, *info, GL_UNSIGNED_SHORT, info+1);
				info += *info + 1;
			}
			else
				info++;

			if (*info)
			{
				glDrawElements(GL_TRIANGLES, *info, GL_UNSIGNED_SHORT, info+1);
				info += *info + 1;
			}
			else
				info++;

			if (*info)
			{
				if (m_nState & LC_PIECE_SELECTED)
				{
					SetCurrentColor(m_nState & LC_PIECE_FOCUSED ? LC_COL_FOCUSED : LC_COL_SELECTED, bTrans, bLighting, bNoAlpha);
					*nLastColor = m_nState & LC_PIECE_FOCUSED ? LC_COL_FOCUSED : LC_COL_SELECTED;
					glDrawElements(GL_LINES, *info, GL_UNSIGNED_SHORT, info+1);
				}
				else
					if (bEdges)
						glDrawElements(GL_LINES, *info, GL_UNSIGNED_SHORT, info+1);

				info += *info + 1;
			}
			else
				info++;

			if (lock)
				glUnlockArraysEXT();
		}
	}

	glPopMatrix();
}

void Piece::CalculateConnections(CONNECTION_TYPE* pConnections, unsigned short nTime, bool bAnimation, bool bForceRebuild, bool bFixOthers)
{
	if (m_pConnections == NULL)
	{
		if (m_pDrawInfo == NULL)
			BuildDrawInfo();
		return;
	}

	bool rebuild = bForceRebuild || (m_pDrawInfo == NULL);
	Piece* pPiece;
	CONNECTION_ENTRY* entry;
	int i, j, c;

	if (bFixOthers)
		m_pLink = NULL;

	for (j = 0; j < m_pPieceInfo->m_nConnectionCount; j++)
	{
		CONNECTION* new_link = NULL;

		// studs
		if (m_pConnections[j].type == 0)
		{
			i = pConnections[1].numentries;
			entry = pConnections[1].entries;

			for (; i--; entry++)
			{
				if ((entry->owner == this) ||
					(!entry->owner->IsVisible(nTime, bAnimation)))
					continue;

				for (c = 0; c < entry->numcons; c++)
				{
					CONNECTION* con = entry->cons[c];

					if (((m_pConnections[j].center[0] - con->center[0]) <  0.1f) && 
						((m_pConnections[j].center[1] - con->center[1]) <  0.1f) && 
						((m_pConnections[j].center[2] - con->center[2]) <  0.1f) &&
						((m_pConnections[j].center[0] - con->center[0]) > -0.1f) && 
						((m_pConnections[j].center[1] - con->center[1]) > -0.1f) && 
						((m_pConnections[j].center[2] - con->center[2]) > -0.1f))
					{
						new_link = con;
						i = 0;
						break;
					}
				}
			}

			if (new_link != m_pConnections[j].link)
			{
				if ((m_pConnections[j].link != NULL) != (new_link != NULL))
					rebuild = true;

				if (bFixOthers)
				{
					// Update old connection
					if (m_pConnections[j].link != NULL)
					{
						Piece* pOwner = m_pConnections[j].link->owner;

						if (pOwner != this)
						{
							if (m_pLink == NULL)
							{
								m_pLink = pOwner;
								pOwner->m_pLink = NULL;
							}
							else
								for (pPiece = m_pLink; pPiece; pPiece = pPiece->m_pLink)
								{
									if (pPiece == pOwner)
										break;

									if (pPiece->m_pLink == NULL)
									{
										pPiece->m_pLink = pOwner;
										pOwner->m_pLink = NULL;
									}
							}
						}

						if (new_link)
						{
							pOwner = new_link->owner;

							if (m_pLink == NULL)
							{
								m_pLink = pOwner;
								pOwner->m_pLink = NULL;
							}
							else
								for (pPiece = m_pLink; pPiece; pPiece = pPiece->m_pLink)
								{
									if (pPiece == pOwner)
										break;

									if (pPiece->m_pLink == NULL)
									{
										pPiece->m_pLink = pOwner;
										pOwner->m_pLink = NULL;
									}
							}
						}
					}
				}

				m_pConnections[j].link = new_link;
			}

			continue;
		}

		// invert studs
		if (m_pConnections[j].type == 1)
		{
			i = pConnections[0].numentries;
			entry = pConnections[0].entries;

			for (; i--; entry++)
			{
				if ((entry->owner == this) ||
					(!entry->owner->IsVisible(nTime, bAnimation)))
					continue;

				for (c = 0; c < entry->numcons; c++)
				{
					CONNECTION* con = entry->cons[c];

					if (((m_pConnections[j].center[0] - con->center[0]) <  0.1f) && 
						((m_pConnections[j].center[1] - con->center[1]) <  0.1f) && 
						((m_pConnections[j].center[2] - con->center[2]) <  0.1f) &&
						((m_pConnections[j].center[0] - con->center[0]) > -0.1f) && 
						((m_pConnections[j].center[1] - con->center[1]) > -0.1f) && 
						((m_pConnections[j].center[2] - con->center[2]) > -0.1f))
					{
						new_link = con;
						i = 0;
						break;
					}
				}
			}

			if (new_link != m_pConnections[j].link)
			{
				if ((m_pConnections[j].link != NULL) != (new_link != NULL))
					rebuild = true;

				if (bFixOthers)
				{
					Piece* pOwner;

					// Update old connection
					if (m_pConnections[j].link != NULL)
					{
						pOwner = m_pConnections[j].link->owner;

						if (pOwner != this)
						{
							if (m_pLink == NULL)
							{
								m_pLink = pOwner;
								pOwner->m_pLink = NULL;
							}
							else
								for (pPiece = m_pLink; pPiece; pPiece = pPiece->m_pLink)
								{
									if (pPiece == pOwner)
										break;

									if (pPiece->m_pLink == NULL)
									{
										pPiece->m_pLink = pOwner;
										pOwner->m_pLink = NULL;
									}
							}
						}
					}

					if (new_link)
					{
						pOwner = new_link->owner;

						if (m_pLink == NULL)
						{
							m_pLink = pOwner;
							pOwner->m_pLink = NULL;
						}
						else
							for (pPiece = m_pLink; pPiece; pPiece = pPiece->m_pLink)
							{
								if (pPiece == pOwner)
									break;

								if (pPiece->m_pLink == NULL)
								{
									pPiece->m_pLink = pOwner;
									pOwner->m_pLink = NULL;
								}
							}
					}
				}

				m_pConnections[j].link = new_link;
			}
			else
			{
				if (bFixOthers && bForceRebuild)
				{
					if (!m_pConnections[j].link)
						continue;

					Piece* pOwner = m_pConnections[j].link->owner;

					if (m_pLink == NULL)
					{
						m_pLink = pOwner;
						pOwner->m_pLink = NULL;
					}
					else
						for (pPiece = m_pLink; pPiece; pPiece = pPiece->m_pLink)
						{
							if (pPiece == pOwner)
								break;

							if (pPiece->m_pLink == NULL)
							{
								pPiece->m_pLink = pOwner;
								pOwner->m_pLink = NULL;
							}
						}
				}
			}

			continue;
		}
	}

	if (bFixOthers)
		for (pPiece = m_pLink; pPiece; pPiece = pPiece->m_pLink)
			pPiece->CalculateConnections(pConnections, nTime, bAnimation, true, false);

/*
	BOOL bRebuild = FALSE;
	CONNECTION *c1, *c2;
	int sz = sizeof(CPiece*)*(m_pInfo->m_nConnectionCount-1);
	CPiece** pConnections = (CPiece**)malloc(sz);
	memset(pConnections, 0, sz);

	for (POSITION pos = pDoc->m_Pieces.GetHeadPosition(); pos != NULL;)
	{
		CPiece* pPiece = pDoc->m_Pieces.GetNext(pos);
		if ((pPiece == this) || (pPiece->m_pInfo->m_nConnectionCount == 1) ||
			(!pPiece->IsVisible(nTime, bAnimator)))
			continue;
		pPiece->m_bUpdated = FALSE;

		for (i = 0; i < m_pInfo->m_nConnectionCount-1; i++)
		{
			c1 = &m_pInfo->m_pConnections[i+1];

			for (j = 0; j < pPiece->m_pInfo->m_nConnectionCount-1; j++)
			{
				c2 = &pPiece->m_pInfo->m_pConnections[j+1];
			
				if (ConnectionsMatch(c1->type, c2->type))
				{
// normal
					if ((fabs(m_pConnections[i].pos[0]-pPiece->m_pConnections[j].pos[0]) < 0.1) && 
						(fabs(m_pConnections[i].pos[1]-pPiece->m_pConnections[j].pos[1]) < 0.1) && 
						(fabs(m_pConnections[i].pos[2]-pPiece->m_pConnections[j].pos[2]) < 0.1))
					{
						pConnections[i] = pPiece;
						break;
					}
				}
			}
		}
	}

	for (i = 0; i < m_pInfo->m_nConnectionCount-1; i++)
	if (m_pConnections[i].pPiece != pConnections[i])
	{
		if (bOthers)
		{
			if ((m_pConnections[i].pPiece != NULL) &&
				(m_pConnections[i].pPiece->IsVisible(nTime, bAnimator)))
				m_pConnections[i].pPiece->UpdateConnections(this);
			if ((pConnections[i] != NULL) &&
				(pConnections[i]->IsVisible(nTime, bAnimator)))
				pConnections[i]->UpdateConnections(this);
		}

		if (m_pConnections[i].pPiece == NULL)
		{
			if (m_pInfo->m_pConnections[i].info != NULL)
				bRebuild = TRUE;
		}
		else
		{
			if (pConnections[i] == NULL)
				if (m_pInfo->m_pConnections[i].info != NULL)
					bRebuild = TRUE;
		}

		m_pConnections[i].pPiece = pConnections[i];
	}

	free(pConnections);
*/
	if (rebuild)
		BuildDrawInfo();
}

/*
inline static BOOL ConnectionsMatch(BYTE c1, BYTE c2)
{
	if (c1 == 1)
	{
		if (c2 == 2)
			return TRUE;
		else
			return FALSE;
	}

	if (c2 == 1)
	{
		if (c1 == 2)
			return TRUE;
		else
			return FALSE;
	}

	// 1: STUD
	// 2: STUD CONNECTION
//	int i = __min (c1, c2);
//	int j = __max (c1, c2);
//	switch (i)
//	{
//	case 1: if (j == 2) return TRUE;
//	}

	return FALSE;
}

void CPiece::UpdateConnections(CPiece* pPiece)
{
	if (m_bUpdated || m_pInfo->m_nConnectionCount == 1)
		return;
	BOOL bRebuild = FALSE;
	int sz = sizeof(CPiece*)*(m_pInfo->m_nConnectionCount-1), i, j;
	CONNECTION *c1, *c2;
	CPiece** pConnections = (CPiece**)malloc(sz);
	memset(pConnections, 0, sz);

	for (i = 0; i < m_pInfo->m_nConnectionCount-1; i++)
	{
		c1 = &m_pInfo->m_pConnections[i+1];

		for (j = 0; j < pPiece->m_pInfo->m_nConnectionCount-1; j++)
		{
			c2 = &pPiece->m_pInfo->m_pConnections[j+1];
			
			if (ConnectionsMatch(c1->type, c2->type))
			{
// normal
				if ((fabs(m_pConnections[i].pos[0]-pPiece->m_pConnections[j].pos[0]) < 0.1) && 
					(fabs(m_pConnections[i].pos[1]-pPiece->m_pConnections[j].pos[1]) < 0.1) && 
					(fabs(m_pConnections[i].pos[2]-pPiece->m_pConnections[j].pos[2]) < 0.1))
				{
					pConnections[i] = pPiece;
					break;
				}
			}
		}
	}

	for (i = 0; i < m_pInfo->m_nConnectionCount-1; i++)
	{
		if (m_pConnections[i].pPiece == pPiece && pConnections[i] == NULL)
		{
			m_pConnections[i].pPiece = NULL;
			bRebuild = TRUE;
		}

		if (pConnections[i] == pPiece && m_pConnections[i].pPiece == NULL)
		{
			m_pConnections[i].pPiece = pPiece;
			bRebuild = TRUE;
		}
	}

	if (bRebuild)
		BuildDrawInfo();
	free(pConnections);
	m_bUpdated = TRUE;
}
*/

void Piece::AddConnections(CONNECTION_TYPE* pConnections)
{
	int i, j, c;

	for (i = 0; i < LC_CONNECTIONS; i++)
	{
		c = 0;

		for (j = 0; j < m_pPieceInfo->m_nConnectionCount; j++)
			if (m_pConnections[j].type == i)
				c++;

		if (c > 0)
		{
			// check if we need to realloc
			if (pConnections[i].numentries % 5 == 0)
			{
				if (pConnections[i].numentries > 0)
					pConnections[i].entries = (CONNECTION_ENTRY*)realloc(pConnections[i].entries, sizeof(CONNECTION_ENTRY)*(pConnections[i].numentries+5));
				else
					pConnections[i].entries = (CONNECTION_ENTRY*)realloc(pConnections[i].entries, sizeof(CONNECTION_ENTRY)*5);
			}

			CONNECTION_ENTRY* entry = &pConnections[i].entries[pConnections[i].numentries];
			pConnections[i].numentries++;

			entry->owner = this;
			entry->numcons = c;
			entry->cons = (CONNECTION**)malloc(c*sizeof(CONNECTION*));

			c = 0;
			for (j = 0; j < m_pPieceInfo->m_nConnectionCount; j++)
				if (m_pConnections[j].type == i)
				{
					entry->cons[c] = &m_pConnections[j];
					c++;
				}
		}
	}
}

void Piece::RemoveConnections(CONNECTION_TYPE* pConnections)
{
	int i, j;

	for (i = 0; i < LC_CONNECTIONS; i++)
		for (j = 0; j < pConnections[i].numentries; j++)
			if (pConnections[i].entries[j].owner == this)
			{
				free(pConnections[i].entries[j].cons);
				pConnections[i].numentries--;

				for (; j < pConnections[i].numentries; j++)
					pConnections[i].entries[j] = pConnections[i].entries[j+1];

				if (pConnections[i].numentries % 5 == 0)
				{
					if (pConnections[i].numentries > 0)
						pConnections[i].entries = (CONNECTION_ENTRY*)realloc(pConnections[i].entries, sizeof(CONNECTION_ENTRY)*pConnections[i].numentries);
					else
					{
						free (pConnections[i].entries);
						pConnections[i].entries = NULL;
					}
				}
			}
}

/*
// Performs exact collision detection using the RAPID library.
// Check if there are 2 objects at the same place at the same time
BOOL CPiece::Collide(CPiece* pPiece)
{
	return CollideAt(pPiece, m_fRotation, m_fPosition);
}

// Use this to check if the piece can be modified
BOOL CPiece::CollideAt(CPiece* pPiece, float rot[4], float pos[3])
{
	double rotation1[3][3], rotation2[3][3];
	double translation1[3], translation2[3];

	CMatrix m1(rot, pos);
	rotation1[0][0] = m1.m[0];
	rotation1[0][1] = m1.m[4];
	rotation1[0][2] = m1.m[8];
	rotation1[1][0] = m1.m[1];
	rotation1[1][1] = m1.m[5];
	rotation1[1][2] = m1.m[9];
	rotation1[2][0] = m1.m[2];
	rotation1[2][1] = m1.m[6];
	rotation1[2][2] = m1.m[10];
	translation1[0] = m1.m[12];
	translation1[1] = m1.m[13];
	translation1[2] = m1.m[14];

	CMatrix m2(pPiece->m_fRotation, pPiece->m_fPosition);
	rotation2[0][0] = m2.m[0];
	rotation2[0][1] = m2.m[4];
	rotation2[0][2] = m2.m[8];
	rotation2[1][0] = m2.m[1];
	rotation2[1][1] = m2.m[5];
	rotation2[1][2] = m2.m[9];
	rotation2[2][0] = m2.m[2];
	rotation2[2][1] = m2.m[6];
	rotation2[2][2] = m2.m[10];
	translation2[0] = m2.m[12];
	translation2[1] = m2.m[13];
	translation2[2] = m2.m[14];

	return CollisionCheck(rotation1, translation1, m_pInfo->m_pRModel,
		rotation2, translation2, pPiece->m_pInfo->m_pRModel);
}
*/
