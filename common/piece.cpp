// A piece object in the LeoCAD world.
//

#include "lc_global.h"
#include "lc_mesh.h"
#include "lc_colors.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "opengl.h"
#include "matrix.h"
#include "pieceinf.h"
#include "texture.h"
#include "piece.h"
#include "group.h"
#include "project.h"
#include "algebra.h"
#include "lc_application.h"

#define LC_PIECE_SAVE_VERSION 11 // LeoCAD 0.77

static LC_OBJECT_KEY_INFO piece_key_info[LC_PK_COUNT] =
{
  { "Position", 3, LC_PK_POSITION },
  { "Rotation", 4, LC_PK_ROTATION }
};

/////////////////////////////////////////////////////////////////////////////
// Piece construction/destruction

Piece::Piece(PieceInfo* pPieceInfo)
	: Object (LC_OBJECT_PIECE)
{
	m_pNext = NULL;
	m_pPieceInfo = pPieceInfo;
	m_nState = 0;
	mColorIndex = 0;
	mColorCode = 0;
	m_nStepShow = 1;
	m_nStepHide = 255;
	m_nFrameHide = 65535;
	memset(m_strName, 0, sizeof(m_strName));
	m_pGroup = NULL;

	if (m_pPieceInfo != NULL)
		m_pPieceInfo->AddRef();

	float *values[] = { m_fPosition, m_fRotation };
	RegisterKeys (values, piece_key_info, LC_PK_COUNT);

	float pos[3] = { 0, 0, 0 }, rot[4] = { 0, 0, 1, 0 };
	ChangeKey (1, false, true, pos, LC_PK_POSITION);
	ChangeKey (1, false, true, rot, LC_PK_ROTATION);
	ChangeKey (1, true, true, pos, LC_PK_POSITION);
	ChangeKey (1, true, true, rot, LC_PK_ROTATION);
}

Piece::~Piece()
{
	if (m_pPieceInfo != NULL)
		m_pPieceInfo->DeRef();
}

/////////////////////////////////////////////////////////////////////////////
// Piece save/load

// Use only when loading from a file
void Piece::SetPieceInfo(PieceInfo* pPieceInfo)
{
	m_pPieceInfo = pPieceInfo;
	m_pPieceInfo->AddRef();
}

bool Piece::FileLoad(lcFile& file, char* name)
{
  lcuint8 version, ch;

  version = file.ReadU8();

  if (version > LC_PIECE_SAVE_VERSION)
    return false;

  if (version > 8)
    if (!Object::FileLoad (file))
      return false;

  if (version < 9)
  {
    lcuint16 time;
    float param[4];
    lcuint8 type;

    if (version > 5)
    {
      lcuint32 keys;

      file.ReadU32(&keys, 1);
      while (keys--)
      {
        file.ReadFloats(param, 4);
        file.ReadU16(&time, 1);
        file.ReadU8(&type, 1);

        ChangeKey (time, false, true, param, type);
      }

      file.ReadU32(&keys, 1);
      while (keys--)
      {
        file.ReadFloats(param, 4);
        file.ReadU16(&time, 1);
        file.ReadU8(&type, 1);

        ChangeKey (time, true, true, param, type);
      }
    }
    else
    {
      if (version > 2)
      {
        file.ReadU8(&ch, 1);

        while (ch--)
        {
          Matrix mat;
          if (version > 3)
          {
            float m[16];
            file.ReadFloats(m, 16);
            mat.FromFloat (m);
          }
          else
          {
            float move[3], rotate[3];
            file.ReadFloats(move, 3);
            file.ReadFloats(rotate, 3);
            mat.CreateOld (move[0], move[1], move[2], rotate[0], rotate[1], rotate[2]);
          }

          lcuint8 b;
          file.ReadU8(&b, 1);
          time = b;

          mat.GetTranslation(&param[0], &param[1], &param[2]);
          param[3] = 0;
          ChangeKey (time, false, true, param, LC_PK_POSITION);
          ChangeKey (time, true, true, param, LC_PK_POSITION);

          mat.ToAxisAngle (param);
          ChangeKey (time, false, true, param, LC_PK_ROTATION);
          ChangeKey (time, true, true, param, LC_PK_ROTATION);

          lcint32 bl;
          file.ReadS32(&bl, 1);
        }
      }
      else
      {
        Matrix mat;
        float move[3], rotate[3];
        file.ReadFloats(move, 3);
        file.ReadFloats(rotate, 3);
        mat.CreateOld (move[0], move[1], move[2], rotate[0], rotate[1], rotate[2]);

        mat.GetTranslation(&param[0], &param[1], &param[2]);
        param[3] = 0;
        ChangeKey (1, false, true, param, LC_PK_POSITION);
        ChangeKey (1, true, true, param, LC_PK_POSITION);

        mat.ToAxisAngle (param);
        ChangeKey (1, false, true, param, LC_PK_ROTATION);
        ChangeKey (1, true, true, param, LC_PK_ROTATION);
      }
    }
  }

  // Common to all versions.
  if (version < 10)
  {
	  memset(name, 0, LC_PIECE_NAME_LEN);
	  file.ReadBuffer(name, 9);
  }
  else
	  file.ReadBuffer(name, LC_PIECE_NAME_LEN);

	// 11 (0.77)
	if (version < 11)
	{
		lcuint8 Color;

		file.ReadU8(&Color, 1);

		if (version < 5)
			mColorCode = lcGetColorCodeFromOriginalColor(Color);
		else
			mColorCode = lcGetColorCodeFromExtendedColor(Color);
	}
	else
		file.ReadU32(&mColorCode, 1);
	mColorIndex = lcGetColorIndex(mColorCode);

  file.ReadU8(&m_nStepShow, 1);
  if (version > 1)
    file.ReadU8(&m_nStepHide, 1);
  else
    m_nStepHide = 255;

  if (version > 5)
  {
    file.ReadU16(&m_nFrameShow, 1);
    file.ReadU16(&m_nFrameHide, 1);

    if (version > 7)
    {
      file.ReadU8(&m_nState, 1);
      Select (false, false, false);
      file.ReadU8(&ch, 1);
      file.ReadBuffer(m_strName, ch);
    }
    else
    {
      lcint32 hide;
      file.ReadS32(&hide, 1);
      if (hide != 0)
        m_nState |= LC_PIECE_HIDDEN;
      file.ReadBuffer(m_strName, 81);
    }

    // 7 (0.64)
    lcint32 i = -1;
    if (version > 6)
      file.ReadS32(&i, 1);
    m_pGroup = (Group*)i;
  }
  else
  {
    m_nFrameShow = 1;
    m_nFrameHide = 65535;

    file.ReadU8(&ch, 1);
    if (ch == 0)
      m_pGroup = (Group*)-1;
    else
      m_pGroup = (Group*)(lcuint32)ch;

    file.ReadU8(&ch, 1);
    if (ch & 0x01)
      m_nState |= LC_PIECE_HIDDEN;
  }

  return true;
}

void Piece::FileSave(lcFile& file, Group* pGroups)
{
	file.WriteU8(LC_PIECE_SAVE_VERSION);

	Object::FileSave (file);

	file.WriteBuffer(m_pPieceInfo->m_strName, LC_PIECE_NAME_LEN);
	file.WriteU32(mColorCode);
	file.WriteU8(m_nStepShow);
	file.WriteU8(m_nStepHide);
	file.WriteU16(m_nFrameShow);
	file.WriteU16(m_nFrameHide);

	// version 8
	file.WriteU8(m_nState);

	lcuint8 Length = strlen(m_strName);
	file.WriteU8(Length);
	file.WriteBuffer(m_strName, Length);

	// version 7
	lcint32 i;

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

	file.WriteS32(i);
}

void Piece::Initialize(float x, float y, float z, unsigned char nStep, unsigned short nFrame)
{
	m_nFrameShow = nFrame;
	m_nStepShow = nStep;

	float pos[3] = { x, y, z }, rot[4] = { 0, 0, 1, 0 };
	ChangeKey (1, false, true, pos, LC_PK_POSITION);
	ChangeKey (1, false, true, rot, LC_PK_ROTATION);
	ChangeKey (1, true, true, pos, LC_PK_POSITION);
	ChangeKey (1, true, true, rot, LC_PK_ROTATION);

	UpdatePosition (1, false);
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

void Piece::Select (bool bSelecting, bool bFocus, bool bMultiple)
{
  if (bSelecting == true)
  {
    if (bFocus == true)
      m_nState |= (LC_PIECE_FOCUSED|LC_PIECE_SELECTED);
    else
      m_nState |= LC_PIECE_SELECTED;
  }
  else
  {
    if (bFocus == true)
      m_nState &= ~(LC_PIECE_FOCUSED);
    else
      m_nState &= ~(LC_PIECE_SELECTED|LC_PIECE_FOCUSED);
  } 
}

void Piece::InsertTime (unsigned short start, bool animation, unsigned short time)
{
  if (animation)
  {
    if (m_nFrameShow >= start)
      m_nFrameShow = min(m_nFrameShow + time, lcGetActiveProject()->GetTotalFrames());

    if (m_nFrameHide >= start)
      m_nFrameHide = min(m_nFrameHide + time, lcGetActiveProject()->GetTotalFrames());

    if (m_nFrameShow > lcGetActiveProject()->GetCurrentTime())
      Select (false, false, false);
  }
  else
  {
    if (m_nStepShow >= start)
      m_nStepShow = min(m_nStepShow + time, 255);

    if (m_nStepHide >= start)
      m_nStepHide = min(m_nStepHide + time, 255);

    if (m_nStepShow > lcGetActiveProject()->GetCurrentTime ())
      Select (false, false, false);
  }

  Object::InsertTime (start, animation, time);
}

void Piece::RemoveTime (unsigned short start, bool animation, unsigned short time)
{
  if (animation)
  {
    if (m_nFrameShow >= start)
      m_nFrameShow = max(m_nFrameShow - time, 1);

    if (m_nFrameHide == lcGetActiveProject()->GetTotalFrames())
      m_nFrameHide = lcGetActiveProject()->GetTotalFrames();
    else
      m_nFrameHide = max(m_nFrameHide - time, 1);

    if (m_nFrameHide < lcGetActiveProject()->GetCurrentTime())
      Select (false, false, false);
  }
  else
  {
    if (m_nStepShow >= start)
      m_nStepShow = max (m_nStepShow - time, 1);

    if (m_nStepHide != 255)
      m_nStepHide = max (m_nStepHide - time, 1);

    if (m_nStepHide < lcGetActiveProject()->GetCurrentTime())
      Select (false, false, false);
  }

  Object::RemoveTime (start, animation, time);
}

void Piece::MinIntersectDist(LC_CLICKLINE* pLine)
{
	double dist;

	dist = BoundingBoxIntersectDist(pLine);
	if (dist >= pLine->mindist)
		return;

	Matrix44 WorldToLocal;
	WorldToLocal.CreateFromAxisAngle(Vector3(m_fRotation[0], m_fRotation[1], m_fRotation[2]), -m_fRotation[3] * LC_DTOR);
	WorldToLocal.SetTranslation(Mul31(Vector3(-m_fPosition[0], -m_fPosition[1], -m_fPosition[2]), WorldToLocal));

	Vector3 Start = Mul31(Vector3(pLine->a1, pLine->b1, pLine->c1), WorldToLocal);
	Vector3 End = Mul31(Vector3(pLine->a1 + pLine->a2, pLine->b1 + pLine->b2, pLine->c1 + pLine->c2), WorldToLocal);
	Vector3 Intersection;

	if (m_pPieceInfo->mMesh->MinIntersectDist(Start, End, pLine->mindist, Intersection))
		pLine->pClosest = this;
}

bool Piece::IntersectsVolume(const Vector4* Planes, int NumPlanes)
{
	// First check the bounding box for quick rejection.
	Vector3 Box[8] =
	{
		Vector3(m_pPieceInfo->m_fDimensions[0], m_pPieceInfo->m_fDimensions[1], m_pPieceInfo->m_fDimensions[5]),
		Vector3(m_pPieceInfo->m_fDimensions[3], m_pPieceInfo->m_fDimensions[1], m_pPieceInfo->m_fDimensions[5]),
		Vector3(m_pPieceInfo->m_fDimensions[0], m_pPieceInfo->m_fDimensions[1], m_pPieceInfo->m_fDimensions[2]),
		Vector3(m_pPieceInfo->m_fDimensions[3], m_pPieceInfo->m_fDimensions[4], m_pPieceInfo->m_fDimensions[5]),
		Vector3(m_pPieceInfo->m_fDimensions[3], m_pPieceInfo->m_fDimensions[4], m_pPieceInfo->m_fDimensions[2]),
		Vector3(m_pPieceInfo->m_fDimensions[0], m_pPieceInfo->m_fDimensions[4], m_pPieceInfo->m_fDimensions[2]),
		Vector3(m_pPieceInfo->m_fDimensions[0], m_pPieceInfo->m_fDimensions[4], m_pPieceInfo->m_fDimensions[5]),
		Vector3(m_pPieceInfo->m_fDimensions[3], m_pPieceInfo->m_fDimensions[1], m_pPieceInfo->m_fDimensions[2])
	};

	// Transform the planes to local space.
	Matrix44 WorldToLocal;
	WorldToLocal.CreateFromAxisAngle(Vector3(m_fRotation[0], m_fRotation[1], m_fRotation[2]), -m_fRotation[3] * LC_DTOR);
	WorldToLocal.SetTranslation(Mul31(Vector3(-m_fPosition[0], -m_fPosition[1], -m_fPosition[2]), WorldToLocal));

	Vector4* LocalPlanes = new Vector4[NumPlanes];
	int i;

	for (i = 0; i < NumPlanes; i++)
	{
		LocalPlanes[i] = Mul30(Vector3(Planes[i]), WorldToLocal);
		LocalPlanes[i][3] = Planes[i][3] - Dot3(Vector3(WorldToLocal[3]), Vector3(LocalPlanes[i]));
	}

	// Start by testing trivial reject/accept cases.
	int Outcodes[8];

	for (i = 0; i < 8; i++)
	{
		Outcodes[i] = 0;

		for (int j = 0; j < NumPlanes; j++)
		{
			if (Dot3(Box[i], LocalPlanes[j]) + LocalPlanes[j][3] > 0)
				Outcodes[i] |= 1 << j;
		}
	}

	int OutcodesOR = 0, OutcodesAND = 0x3f;

	for (i = 0; i < 8; i++)
	{
		OutcodesAND &= Outcodes[i];
		OutcodesOR |= Outcodes[i];
	}

	// All corners outside the same plane.
	if (OutcodesAND != 0)
	{
		delete[] LocalPlanes;
		return false;
	}

	// All corners inside the volume.
	if (OutcodesOR == 0)
	{
		delete[] LocalPlanes;
		return true;
	}

	// Partial intersection, so check if any triangles are inside.
	bool Hit = m_pPieceInfo->mMesh->IntersectsPlanes(LocalPlanes, NumPlanes);

	delete[] LocalPlanes;

	return Hit;
}

void Piece::Move (unsigned short nTime, bool bAnimation, bool bAddKey, float dx, float dy, float dz)
{
  m_fPosition[0] += dx;
  m_fPosition[1] += dy;
  m_fPosition[2] += dz;

  ChangeKey (nTime, bAnimation, bAddKey, m_fPosition, LC_PK_POSITION);
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

	CalculateKeys (nTime, bAnimation);
//	if (CalculatePositionRotation(nTime, bAnimation, m_fPosition, m_fRotation))
	{
		Matrix mat(m_fRotation, m_fPosition);
		BoundingBoxCalculate(&mat, m_pPieceInfo->m_fDimensions);
	}
}

void Piece::RenderBox(bool bHilite, float fLineWidth)
{
	glPushMatrix();
	glTranslatef(m_fPosition[0], m_fPosition[1], m_fPosition[2]);
	glRotatef(m_fRotation[3], m_fRotation[0], m_fRotation[1], m_fRotation[2]);

	if (bHilite && ((m_nState & LC_PIECE_SELECTED) != 0))
	{
		if (m_nState & LC_PIECE_FOCUSED)
			lcSetColorFocused();
		else
			lcSetColorSelected();
		glLineWidth(2*fLineWidth);
		glPushAttrib(GL_POLYGON_BIT);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glCallList(m_pPieceInfo->GetBoxDisplayList());
		glPopAttrib();
		glLineWidth(fLineWidth);
	}
	else
	{
		lcSetColor(mColorIndex);
		glCallList(m_pPieceInfo->GetBoxDisplayList());
	}
	glPopMatrix();
}

void Piece::Render(bool bLighting, bool bEdges)
{
	glPushMatrix();
	glTranslatef(m_fPosition[0], m_fPosition[1], m_fPosition[2]);
	glRotatef(m_fRotation[3], m_fRotation[0], m_fRotation[1], m_fRotation[2]);
	m_pPieceInfo->mMesh->Render(mColorIndex, (m_nState & LC_PIECE_SELECTED) != 0, (m_nState & LC_PIECE_FOCUSED) != 0);
	glPopMatrix();
}
