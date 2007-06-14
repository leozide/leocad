// A piece object in the LeoCAD world.
//

#include "lc_global.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "opengl.h"
#include "pieceinf.h"
#include "texture.h"
#include "piece.h"
#include "group.h"
#include "project.h"
#include "algebra.h"
#include "lc_application.h"
#include "lc_mesh.h"
#include "matrix.h"

#if 0

#define LC_PIECE_SAVE_VERSION 9 // LeoCAD 0.73

/////////////////////////////////////////////////////////////////////////////
// Piece construction/destruction

static bool lockarrays = false;

Piece::Piece(PieceInfo* pPieceInfo)
  : Object (LC_OBJECT_PIECE)
{
  static bool first_time = true;

  if (first_time)
  {
    first_time = false;
    lockarrays = GL_HasCompiledVertexArrays();
  }
}

/////////////////////////////////////////////////////////////////////////////
// Piece save/load

bool Piece::FileLoad (File& file, char* name)
{
  unsigned char version, ch;

  file.ReadByte (&version, 1);

  if (version > LC_PIECE_SAVE_VERSION)
    return false;

  if (version > 8)
    if (!Object::FileLoad (file))
      return false;

  if (version < 9)
  {
    unsigned short time;
    float param[4];
    unsigned char type;

    if (version > 5)
    {
      unsigned long keys;

      file.ReadLong (&keys, 1);
      while (keys--)
      {
        file.ReadFloat (param, 4);
        file.ReadShort (&time, 1);
        file.ReadByte (&type, 1);

        ChangeKey (time, true, param, type);
      }

      file.ReadLong (&keys, 1);
      while (keys--)
      {
        file.ReadFloat (param, 4);
        file.ReadShort (&time, 1);
        file.ReadByte (&type, 1);

        ChangeKey (time, true, param, type);
      }
    }
    else
    {
      if (version > 2)
      {
        file.Read (&ch, 1);

        while (ch--)
        {
          Matrix mat;
          if (version > 3)
          {
            file.ReadFloat(mat.m, 16);
          }
          else
          {
            float move[3], rotate[3];
            file.ReadFloat(move, 3);
            file.ReadFloat(rotate, 3);
            mat.CreateOld (move[0], move[1], move[2], rotate[0], rotate[1], rotate[2]);
          }

          unsigned char b;
          file.ReadByte(&b, 1);
          time = b;

          mat.GetTranslation(&param[0], &param[1], &param[2]);
          param[3] = 0;
          ChangeKey (time, true, param, LC_PK_POSITION);
          ChangeKey (time, true, param, LC_PK_POSITION);

          mat.ToAxisAngle (param);
          ChangeKey (time, true, param, LC_PK_ROTATION);
          ChangeKey (time, true, param, LC_PK_ROTATION);

          int bl;
          file.ReadLong (&bl, 1);
        }
      }
      else
      {
        Matrix mat;
        float move[3], rotate[3];
        file.ReadFloat(move, 3);
        file.ReadFloat(rotate, 3);
        mat.CreateOld(move[0], move[1], move[2], rotate[0], rotate[1], rotate[2]);

        mat.GetTranslation(&param[0], &param[1], &param[2]);
        param[3] = 0;
        ChangeKey (1, true, param, LC_PK_POSITION);
        ChangeKey (1, true, param, LC_PK_POSITION);

        mat.ToAxisAngle(param);
        ChangeKey(1, true, param, LC_PK_ROTATION);
        ChangeKey(1, true, param, LC_PK_ROTATION);
      }
    }
  }

  // Common to all versions.
  file.Read (name, 9);
  file.ReadByte (&m_nColor, 1);

  if (version < 5)
  {
    const unsigned char conv[20] = { 0,2,4,9,7,6,22,8,10,11,14,16,18,9,21,20,22,8,10,11 };
    m_nColor = conv[m_nColor];
  }

	u8 Show;
  file.ReadByte(&Show, 1);
	m_TimeShow = Show;

  if (version > 1)
	{
		u8 Hide;
    file.ReadByte(&Hide, 1);
		m_TimeHide = Hide;
	}
	else
		m_TimeHide = LC_MAX_TIME;


  if (version > 5)
  {
		u16 dummy16;
    file.ReadShort(&dummy16, 1);
    file.ReadShort(&dummy16, 1);

    if (version > 7)
    {
      file.ReadByte(&m_nState, 1);
      Select (false, false);
      file.ReadByte(&ch, 1);
      file.Read(m_strName, ch);
    }
    else
    {
      int hide;
      file.ReadLong(&hide, 1);
      if (hide != 0)
        m_nState |= LC_PIECE_HIDDEN;
      file.Read(m_strName, 81);
    }

    // 7 (0.64)
    int i = -1;
    if (version > 6)
      file.ReadLong(&i, 1);
    m_pGroup = (Group*)i;
  }
  else
  {
    file.ReadByte(&ch, 1);
    if (ch == 0)
      m_pGroup = (Group*)-1;
    else
      m_pGroup = (Group*)(unsigned long)ch;

    file.ReadByte(&ch, 1);
    if (ch & 0x01)
      m_nState |= LC_PIECE_HIDDEN;
  }

  return true;
}

void Piece::FileSave (File& file, Group* pGroups)
{
  unsigned char ch = LC_PIECE_SAVE_VERSION;

  file.WriteByte (&ch, 1);

  Object::FileSave (file);

  file.Write(m_pPieceInfo->m_strName, 9);
  file.WriteByte(&m_nColor, 1);

	u8 Show = m_TimeShow;
	file.WriteByte(&Show, 1);
	u8 Hide = m_TimeHide;
  file.WriteByte(&Hide, 1);

	u16 dummy16 = 0;
	file.WriteShort(&dummy16, 1);
  file.WriteShort(&dummy16, 1);

  // version 8
  file.WriteByte(&m_nState, 1);
  ch = strlen(m_strName);
  file.WriteByte(&ch, 1);
  file.Write(m_strName, ch);

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
  file.WriteLong(&i, 1);
}

void Piece::Initialize(float x, float y, float z, u32 Time, unsigned char nColor)
{
  m_TimeShow = Time;

  float pos[3] = { x, y, z }, rot[4] = { 0, 0, 1, 0 };
  ChangeKey(1, true, pos, LC_PK_POSITION);
  ChangeKey(1, true, rot, LC_PK_ROTATION);

  UpdatePosition (1);

  m_nColor = nColor;
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
void Piece::UpdatePosition(unsigned short nTime)
{
	if (!IsVisible(nTime))
		m_nState &= ~(LC_PIECE_SELECTED|LC_PIECE_FOCUSED);

	CalculateKeys(nTime);

	m_ModelWorld = MatrixFromAxisAngle(Vector3(m_fRotation[0], m_fRotation[1], m_fRotation[2]), m_fRotation[3] * LC_DTOR);
	m_ModelWorld.SetTranslation(Vector3(m_fPosition[0], m_fPosition[1], m_fPosition[2]));

//	if (CalculatePositionRotation(nTime, bAnimation, m_fPosition, m_fRotation))
	{
		BoundingBoxCalculate(m_ModelWorld, m_pPieceInfo->m_fDimensions);
	}
}

void Piece::RenderBox(bool bHilite, float fLineWidth)
{
	glPushMatrix();
	glTranslatef(m_fPosition[0], m_fPosition[1], m_fPosition[2]);
	glRotatef(m_fRotation[3], m_fRotation[0], m_fRotation[1], m_fRotation[2]);

	if (bHilite && ((m_nState & LC_PIECE_SELECTED) != 0))
	{
		glColor3ubv(FlatColorArray[m_nState & LC_PIECE_FOCUSED ? LC_COL_FOCUSED : LC_COL_SELECTED]);
		glLineWidth(2*fLineWidth);
		glPushAttrib(GL_POLYGON_BIT);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glCallList(m_pPieceInfo->GetBoxDisplayList());
		glPopAttrib();
		glLineWidth(fLineWidth);
	}
	else
	{
		glColor3ubv(FlatColorArray[m_nColor]);
		glCallList(m_pPieceInfo->GetBoxDisplayList());
	}
	glPopMatrix();
}

void Piece::Render(bool bLighting, bool bEdges)
{
	glPushMatrix();
	glTranslatef(m_fPosition[0], m_fPosition[1], m_fPosition[2]);
	glRotatef(m_fRotation[3], m_fRotation[0], m_fRotation[1], m_fRotation[2]);

	for (int sh = 0; sh < m_pPieceInfo->m_nTextureCount; sh++)
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		m_pPieceInfo->m_pTextures[sh].texture->MakeCurrent();

		if (m_pPieceInfo->m_pTextures[sh].color == LC_COL_DEFAULT)
		{
			SetCurrentColor(m_nColor, bLighting);
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

	m_Mesh->Render(m_nColor, (m_nState & LC_PIECE_SELECTED) != 0, (m_nState & LC_PIECE_FOCUSED) != 0);

	glPopMatrix();
}

#endif
