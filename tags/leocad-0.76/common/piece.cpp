// A piece object in the LeoCAD world.
//

#include "lc_global.h"
#include "piece.h"

#include "opengl.h"
#include "matrix.h"
#include "pieceinf.h"
#include "texture.h"
#include "group.h"
#include "project.h"
#include "algebra.h"
#include "lc_application.h"
#include "lc_colors.h"

#define LC_PIECE_SAVE_VERSION 9 // LeoCAD 0.73

static LC_OBJECT_KEY_INFO piece_key_info[LC_PK_COUNT] =
{
	{ "Position", 3, LC_PK_POSITION },
	{ "Rotation", 4, LC_PK_ROTATION }
};

/////////////////////////////////////////////////////////////////////////////
// Static functions

inline static void SetCurrentColor(unsigned char nColor, bool bLighting)
{
	bool Transparent = (nColor > 13 && nColor < 22);

	lcSetColor(nColor);

	if (nColor > 27)
		return;

	if (Transparent)
	{
		glEnable(GL_BLEND);
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else
	{
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}
}

/////////////////////////////////////////////////////////////////////////////
// Piece construction/destruction

static bool lockarrays = false;

lcPiece::lcPiece(PieceInfo* pPieceInfo)
  : lcObject (LC_OBJECT_PIECE)
{
  static bool first_time = true;

  if (first_time)
  {
    first_time = false;
    lockarrays = GL_HasCompiledVertexArrays();
  }

	m_Next = NULL;
	m_pPieceInfo = pPieceInfo;
	m_nState = 0;
	m_nColor = 0;
	m_nStepShow = 1;
	m_nStepHide = 255;
	m_nFrameHide = 65535;
	m_Name = "";
	m_pGroup = NULL;
	m_pDrawInfo = NULL;
	m_pConnections = NULL;

	if (m_pPieceInfo != NULL)
	{
		m_pPieceInfo->AddRef();

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

  float *values[] = { m_fPosition, m_fRotation };
  RegisterKeys (values, piece_key_info, LC_PK_COUNT);

  float pos[3] = { 0, 0, 0 }, rot[4] = { 0, 0, 1, 0 };
  ChangeKey (1, false, true, pos, LC_PK_POSITION);
  ChangeKey (1, false, true, rot, LC_PK_ROTATION);
  ChangeKey (1, true, true, pos, LC_PK_POSITION);
  ChangeKey (1, true, true, rot, LC_PK_ROTATION);
}

lcPiece::~lcPiece()
{
  if (m_pPieceInfo != NULL)
    m_pPieceInfo->DeRef ();

  if (m_pDrawInfo != NULL)
    free (m_pDrawInfo);

  if (m_pConnections != NULL)
    free (m_pConnections);
}

/////////////////////////////////////////////////////////////////////////////
// Piece save/load

// Use only when loading from a file
void lcPiece::SetPieceInfo(PieceInfo* pPieceInfo)
{
	m_pPieceInfo = pPieceInfo;
	m_pPieceInfo->AddRef();

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

bool lcPiece::FileLoad (File& file, char* name)
{
  unsigned char version, ch;

  file.ReadByte (&version, 1);

  if (version > LC_PIECE_SAVE_VERSION)
    return false;

  if (version > 8)
    if (!lcObject::FileLoad (file))
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

        ChangeKey (time, false, true, param, type);
      }

      file.ReadLong (&keys, 1);
      while (keys--)
      {
        file.ReadFloat (param, 4);
        file.ReadShort (&time, 1);
        file.ReadByte (&type, 1);

        ChangeKey (time, true, true, param, type);
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
          ChangeKey (time, false, true, param, LC_PK_POSITION);
          ChangeKey (time, true, true, param, LC_PK_POSITION);

          mat.ToAxisAngle (param);
          ChangeKey (time, false, true, param, LC_PK_ROTATION);
          ChangeKey (time, true, true, param, LC_PK_ROTATION);

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
        ChangeKey (1, false, true, param, LC_PK_POSITION);
        ChangeKey (1, true, true, param, LC_PK_POSITION);

        mat.ToAxisAngle(param);
        ChangeKey(1, false, true, param, LC_PK_ROTATION);
        ChangeKey(1, true, true, param, LC_PK_ROTATION);
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

  file.ReadByte(&m_nStepShow, 1);
  if (version > 1)
    file.ReadByte(&m_nStepHide, 1);
  else
    m_nStepHide = 255;

  if (version > 5)
  {
    file.ReadShort(&m_nFrameShow, 1);
    file.ReadShort(&m_nFrameHide, 1);

    if (version > 7)
    {
      file.ReadByte(&m_nState, 1);
      Select (false, false, false);
      file.ReadByte(&ch, 1);
	  char buf[81];
      file.Read(buf, ch);
	  buf[80] = 0;
	  m_Name = buf;
    }
    else
    {
      int hide;
      file.ReadLong(&hide, 1);
      if (hide != 0)
        m_nState |= LC_PIECE_HIDDEN;
	  char buf[81];
      file.Read(buf, 81);
	  buf[80] = 0;
	  m_Name = buf;
    }

    // 7 (0.64)
    int i = -1;
    if (version > 6)
      file.ReadLong(&i, 1);
    m_pGroup = (Group*)i;
  }
  else
  {
    m_nFrameShow = 1;
    m_nFrameHide = 65535;

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

void lcPiece::FileSave (File& file, Group* pGroups)
{
  unsigned char ch = LC_PIECE_SAVE_VERSION;

  file.WriteByte (&ch, 1);

  lcObject::FileSave (file);

  file.Write(m_pPieceInfo->m_strName, 9);
  file.WriteByte(&m_nColor, 1);
  file.WriteByte(&m_nStepShow, 1);
  file.WriteByte(&m_nStepHide, 1);
  file.WriteShort(&m_nFrameShow, 1);
  file.WriteShort(&m_nFrameHide, 1);

  // version 8
  file.WriteByte(&m_nState, 1);
  ch = m_Name.GetLength();
  file.WriteByte(&ch, 1);
  file.Write((const char*)m_Name, ch);

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

void lcPiece::Initialize(float x, float y, float z, unsigned char nStep, unsigned short nFrame, unsigned char nColor)
{
  m_nFrameShow = nFrame;
  m_nStepShow = nStep;

  float pos[3] = { x, y, z }, rot[4] = { 0, 0, 1, 0 };
  ChangeKey (1, false, true, pos, LC_PK_POSITION);
  ChangeKey (1, false, true, rot, LC_PK_ROTATION);
  ChangeKey (1, true, true, pos, LC_PK_POSITION);
  ChangeKey (1, true, true, rot, LC_PK_ROTATION);

  UpdatePosition (1, false);

  m_nColor = nColor;
}

void lcPiece::CreateName(lcPiece* pPiece)
{
	int i, max = 0;

	for (; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
		if (strncmp(pPiece->m_Name, m_pPieceInfo->m_strDescription, strlen(m_pPieceInfo->m_strDescription)) == 0)
			if (sscanf((char*)pPiece->m_Name + strlen(m_pPieceInfo->m_strDescription), " #%d", &i) == 1)
				if (i > max) 
					max = i;

	char buf[256];
	sprintf(buf, "%s #%.2d", m_pPieceInfo->m_strDescription, max+1);
	m_Name = buf;
}

void lcPiece::Select(bool bSelecting, bool bFocus, bool bMultiple)
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

void lcPiece::InsertTime (unsigned short start, bool animation, unsigned short time)
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

  lcObject::InsertTime (start, animation, time);
}

void lcPiece::RemoveTime (unsigned short start, bool animation, unsigned short time)
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

  lcObject::RemoveTime (start, animation, time);
}

void lcPiece::ClosestLineIntersect(lcClickLine& ClickLine) const
{
	Matrix44 WorldModel;
	WorldModel = MatrixFromAxisAngle(Vector4(m_fRotation[0], m_fRotation[1], m_fRotation[2], -m_fRotation[3] * LC_DTOR));
	WorldModel.SetTranslation(Mul31(Vector3(-m_fPosition[0], -m_fPosition[1], -m_fPosition[2]), WorldModel));

	Vector3 Start = Mul31(ClickLine.Start, WorldModel);
	Vector3 End = Mul31(ClickLine.End, WorldModel);

	// Check the bounding box distance first.
	float Dist;
	BoundingBox Box = BoundingBox(Vector3(m_pPieceInfo->m_fDimensions[3], m_pPieceInfo->m_fDimensions[4], m_pPieceInfo->m_fDimensions[5]),
		                          Vector3(m_pPieceInfo->m_fDimensions[0], m_pPieceInfo->m_fDimensions[1], m_pPieceInfo->m_fDimensions[2]));

	if (!BoundingBoxRayMinIntersectDistance(Box, Start, End, &Dist) || (Dist >= ClickLine.Dist))
		return;

	float* verts = m_pPieceInfo->m_fVertexArray;
	Vector3 Intersection;

	if (m_pPieceInfo->m_nFlags & LC_PIECE_LONGDATA)
	{
		unsigned long* info = (unsigned long*)m_pDrawInfo, colors, i;
		colors = *info;
		info++;

		while (colors--)
		{
			info++;

			for (i = 0; i < *info; i += 3)
			{
				Vector3 v1(verts[info[i+1]*3], verts[info[i+1]*3+1], verts[info[i+1]*3+2]);
				Vector3 v2(verts[info[i+2]*3], verts[info[i+2]*3+1], verts[info[i+2]*3+2]);
				Vector3 v3(verts[info[i+3]*3], verts[info[i+3]*3+1], verts[info[i+3]*3+2]);

				if (LineTriangleMinIntersection(v1, v2, v3, Start, End, &ClickLine.Dist, &Intersection))
				{
					ClickLine.Object = this;
				}
			}

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

			for (i = 0; i < *info; i += 3)
			{
				Vector3 v1(verts[info[i+1]*3], verts[info[i+1]*3+1], verts[info[i+1]*3+2]);
				Vector3 v2(verts[info[i+2]*3], verts[info[i+2]*3+1], verts[info[i+2]*3+2]);
				Vector3 v3(verts[info[i+3]*3], verts[info[i+3]*3+1], verts[info[i+3]*3+2]);

				if (LineTriangleMinIntersection(v1, v2, v3, Start, End, &ClickLine.Dist, &Intersection))
				{
					ClickLine.Object = this;
				}
			}

			info += *info + 1;
			info += *info + 1;
		}
	}
}

bool lcPiece::IntersectsVolume(const Vector4* Planes, int NumPlanes) const
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
	WorldToLocal = MatrixFromAxisAngle(Vector4(m_fRotation[0], m_fRotation[1], m_fRotation[2], -m_fRotation[3] * LC_DTOR));
	WorldToLocal.SetTranslation(Mul31(Vector3(-m_fPosition[0], -m_fPosition[1], -m_fPosition[2]), WorldToLocal));

	Vector4* LocalPlanes = new Vector4[NumPlanes];
	int i;

	for (i = 0; i < NumPlanes; i++)
	{
		LocalPlanes[i] = Vector4(Mul30(Vector3(Planes[i]), WorldToLocal));
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
	float* verts = m_pPieceInfo->m_fVertexArray;
	bool ret = false;

	if (m_pPieceInfo->m_nFlags & LC_PIECE_LONGDATA)
	{
		unsigned long* info = (unsigned long*)m_pDrawInfo, colors, i;
		colors = *info;
		info++;

		while (colors--)
		{
			info++;

			for (i = 0; i < *info; i += 3)
			{
				if (PolygonIntersectsPlanes(&verts[info[i+1]*3], &verts[info[i+2]*3],
				                            &verts[info[i+3]*3], NULL, LocalPlanes, NumPlanes))
				{
					ret = true;
					break;
				}
			}

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

			for (i = 0; i < *info; i += 3)
			{
				if (PolygonIntersectsPlanes(&verts[info[i+1]*3], &verts[info[i+2]*3], 
				                            &verts[info[i+3]*3], NULL, LocalPlanes, NumPlanes))
				{
					ret = true;
					break;
				}
			}

			info += *info + 1;
			info += *info + 1;
		}
	}

	delete[] LocalPlanes;

	return ret;
}

void lcPiece::Move (unsigned short nTime, bool bAnimation, bool bAddKey, float dx, float dy, float dz)
{
  m_fPosition[0] += dx;
  m_fPosition[1] += dy;
  m_fPosition[2] += dz;

  ChangeKey (nTime, bAnimation, bAddKey, m_fPosition, LC_PK_POSITION);
}

bool lcPiece::IsVisible(unsigned short nTime, bool bAnimation)
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

void lcPiece::CompareBoundingBox(float box[6])
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

Group* lcPiece::GetTopGroup()
{
	return m_pGroup ? m_pGroup->GetTopGroup() : NULL;
}

void lcPiece::DoGroup(Group* pGroup)
{
	if (m_pGroup != NULL && m_pGroup != (Group*)-1 && m_pGroup > (Group*)0xFFFF)
		m_pGroup->SetGroup(pGroup);
	else
		m_pGroup = pGroup;
}

void lcPiece::UnGroup(Group* pGroup)
{
	if ((m_pGroup == pGroup) || (pGroup == NULL))
		m_pGroup = NULL;
	else
		if (m_pGroup != NULL)
			m_pGroup->UnGroup(pGroup);
}

// Recalculates current position and connections
void lcPiece::UpdatePosition(unsigned short nTime, bool bAnimation)
{
	if (!IsVisible(nTime, bAnimation))
		m_nState &= ~(LC_PIECE_SELECTED|LC_PIECE_FOCUSED);

	CalculateKeys(nTime, bAnimation);
//	if (CalculatePositionRotation(nTime, bAnimation, m_fPosition, m_fRotation))
	{
		Matrix44 mat;
		mat = MatrixFromAxisAngle(Vector3(m_fRotation[0], m_fRotation[1], m_fRotation[2]), m_fRotation[3] * LC_DTOR);
		mat.SetTranslation(Vector3(m_fPosition[0], m_fPosition[1], m_fPosition[2]));

		for (int i = 0; i < m_pPieceInfo->m_nConnectionCount; i++)
		{
			m_pConnections[i].center = Mul31(m_pPieceInfo->m_pConnections[i].center, mat);

			// TODO: rotate normal
		}
	}
}

void lcPiece::BuildDrawInfo()
{
	if (m_pDrawInfo != NULL)
	{
		free(m_pDrawInfo);
		m_pDrawInfo = NULL;
	}

	DRAWGROUP* dg;
	bool add;
	unsigned short group, colcount, i, j;
	u32* count = new u32[lcNumColors*2], vert;
	memset(count, 0, sizeof(u32)*lcNumColors*2);

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
					count[curcol*2+0] += *p;
					p += *p + 1;
					count[curcol*2+1] += *p;
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
					count[curcol*2+0] += *p;
					p += *p + 1;
					count[curcol*2+1] += *p;
					p += *p + 1;
				}
			}
		}
	}

	colcount = 0;
	vert = 0;
	for (i = 0; i < lcNumColors; i++)
		if (count[i*2+0] || count[i*2+1])
		{
			colcount++;
			vert += count[i*2+0] + count[i*2+1];
		}
	vert += (colcount*3)+1;

	// Build the info
	if (m_pPieceInfo->m_nFlags & LC_PIECE_LONGDATA)
	{
		m_pDrawInfo = malloc(vert*sizeof(unsigned long));
		unsigned long* drawinfo = (unsigned long*)m_pDrawInfo;
		*drawinfo = colcount;
		drawinfo++;

		for (i = 0; i < lcNumColors; i++)
		{
			if (count[i*2+0] || count[i*2+1])
			{
				*drawinfo = i;
				drawinfo++;

				for (j = 0; j < 2; j++)
				{
					*drawinfo = count[i*2+j];
					drawinfo++;

					if (count[i*2+j] == 0)
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
															}
							else
							{
								p++;
								p += *p + 1;
								p += *p + 1;
							}
						}
					}
				}
			}
		}
	}
	else
	{
		m_pDrawInfo = malloc(vert*sizeof(unsigned short));
		unsigned short* drawinfo = (unsigned short*)m_pDrawInfo;
		*drawinfo = colcount;
		drawinfo++;

		for (i = 0; i < lcNumColors; i++)
		{
			if (count[i*2+0] || count[i*2+1])
			{
				*drawinfo = i;
				drawinfo++;

				for (j = 0; j < 2; j++)
				{
					*drawinfo = (unsigned short)count[i*2+j];
					drawinfo++;

					if (count[i*2+j] == 0)
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
							}
							else
							{
								p++;
								p += *p + 1;
								p += *p + 1;
							}
						}
					}
				}
			}
		}
	}

	delete[] count;
}

void lcPiece::RenderBox(bool bHilite, float fLineWidth)
{
	glPushMatrix();
	glTranslatef(m_fPosition[0], m_fPosition[1], m_fPosition[2]);
	glRotatef(m_fRotation[3], m_fRotation[0], m_fRotation[1], m_fRotation[2]);

#ifndef LC_OPENGLES
	if (bHilite && ((m_nState & LC_PIECE_SELECTED) != 0))
	{
		lcSetColor(m_nState & LC_PIECE_FOCUSED ? LC_COLOR_FOCUS : LC_COLOR_SELECTION);
		glLineWidth(2*fLineWidth);
		glPushAttrib(GL_POLYGON_BIT);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		m_pPieceInfo->RenderBox();
		glPopAttrib();
		glLineWidth(fLineWidth);
	}
	else
#endif
	{
		lcSetColor(m_nColor);
		m_pPieceInfo->RenderBox();
	}
	glPopMatrix();
}

void lcPiece::Render(bool bLighting, bool bEdges)
{
	glPushMatrix();
	glTranslatef(m_fPosition[0], m_fPosition[1], m_fPosition[2]);
	glRotatef(m_fRotation[3], m_fRotation[0], m_fRotation[1], m_fRotation[2]);
	glVertexPointer (3, GL_FLOAT, 0, m_pPieceInfo->m_fVertexArray);

	for (int sh = 0; sh < m_pPieceInfo->m_nTextureCount; sh++)
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		m_pPieceInfo->m_pTextures[sh].texture->MakeCurrent();

		if (m_pPieceInfo->m_pTextures[sh].color == LC_COLOR_DEFAULT)
			SetCurrentColor(m_nColor, bLighting);

		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, m_pPieceInfo->m_pTextures[sh].vertex);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, m_pPieceInfo->m_pTextures[sh].coords);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
	}

	glEnableClientState(GL_VERTEX_ARRAY);

	if (m_pPieceInfo->m_nFlags & LC_PIECE_LONGDATA)
	{
#ifndef LC_OPENGLES
		unsigned long colors, *info = (unsigned long*)m_pDrawInfo;
		colors = *info;
		info++;

		while (colors--)
		{
			bool lock = lockarrays && (*info == LC_COLOR_DEFAULT || *info == LC_COLOR_EDGE);

			if (*info == LC_COLOR_DEFAULT)
			{
				SetCurrentColor(m_nColor, bLighting);
			}
			else
			{
				SetCurrentColor((unsigned char)*info, bLighting);
			}
			info++;

			if (lock)
				glLockArraysEXT(0, m_pPieceInfo->m_nVertexCount);

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
			    if (lock)
			      glUnlockArraysEXT();

			    SetCurrentColor(m_nState & LC_PIECE_FOCUSED ? LC_COLOR_FOCUS : LC_COLOR_SELECTION, bLighting);

			    if (lock)
			      glLockArraysEXT(0, m_pPieceInfo->m_nVertexCount);

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
#endif
	}
	else
	{
		unsigned short colors, *info = (unsigned short*)m_pDrawInfo;
		colors = *info;
		info++;

		while (colors--)
		{
			bool lock = lockarrays && (*info == LC_COLOR_DEFAULT || *info == LC_COLOR_EDGE);

			if (*info == LC_COLOR_DEFAULT)
			{
				SetCurrentColor(m_nColor, bLighting);
			}
			else
			{
				SetCurrentColor((unsigned char)*info, bLighting);
			}
			info++;

			if (lock)
				glLockArraysEXT(0, m_pPieceInfo->m_nVertexCount);

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
			    if (lock)
			      glUnlockArraysEXT();
			    SetCurrentColor((m_nState & LC_PIECE_FOCUSED) ? LC_COLOR_FOCUS : LC_COLOR_SELECTION, bLighting);
			    
			    if (lock)
			      glLockArraysEXT(0, m_pPieceInfo->m_nVertexCount);

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

void lcPiece::CalculateConnections(CONNECTION_TYPE* pConnections, unsigned short nTime, bool bAnimation, bool bForceRebuild, bool bFixOthers)
{
	if (m_pConnections == NULL)
	{
		if (m_pDrawInfo == NULL)
			BuildDrawInfo();
		return;
	}

	bool rebuild = bForceRebuild || (m_pDrawInfo == NULL);
	lcPiece* pPiece;
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
						lcPiece* pOwner = m_pConnections[j].link->owner;

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
					lcPiece* pOwner;

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

					lcPiece* pOwner = m_pConnections[j].link->owner;

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

	if (rebuild)
		BuildDrawInfo();
}

void lcPiece::AddConnections(CONNECTION_TYPE* pConnections)
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

void lcPiece::RemoveConnections(CONNECTION_TYPE* pConnections)
{
	lcPtrArray<lcPiece> RebuildList;
	int i, j;

	for (i = 0; i < LC_CONNECTIONS; i++)
	{
		CONNECTION_TYPE* Type = &pConnections[i];

		for (j = 0; j < Type->numentries; j++)
		{
			CONNECTION_ENTRY* Entry = &Type->entries[j];

			if (Entry->owner == this)
			{
				// Save a list of pieces that their lost connection to this one.
				for (int k = 0; k < Entry->numcons; k++)
				{
					if (Entry->cons[k]->link != NULL)
					{
						if (RebuildList.FindIndex(Entry->cons[k]->link->owner) == -1)
							RebuildList.Add(Entry->cons[k]->link->owner);
						Entry->cons[k]->link->link = NULL;
					}
				}

				free(Entry->cons);
				Type->numentries--;

				// Shrink array.
				for (; j < Type->numentries; j++)
					Type->entries[j] = Type->entries[j+1];

				// Realloc to save memory.
				if (Type->numentries % 5 == 0)
				{
					if (Type->numentries > 0)
						Type->entries = (CONNECTION_ENTRY*)realloc(Type->entries, sizeof(CONNECTION_ENTRY)*Type->numentries);
					else
					{
						free(Type->entries);
						Type->entries = NULL;
					}
				}
			}
		}
	}

	// Fix pieces that lost their connection to this one.
	for (i = 0; i < RebuildList.GetSize(); i++)
		RebuildList[i]->BuildDrawInfo();
}
