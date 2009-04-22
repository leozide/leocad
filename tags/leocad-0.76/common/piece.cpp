// A piece object in the LeoCAD world.
//

#include "lc_global.h"
#include "piece.h"

#include "opengl.h"
#include "pieceinf.h"
#include "texture.h"
#include "group.h"
#include "project.h"
#include "algebra.h"
#include "lc_application.h"
#include "lc_colors.h"
#include "lc_mesh.h"
#include "lc_model.h"
#include "matrix.h"

#define LC_PIECE_SAVE_VERSION 10 // LeoCAD 0.76

static LC_OBJECT_KEY_INFO piece_key_info[LC_PK_COUNT] =
{
	{ "Position", 3, LC_PK_POSITION },
	{ "Rotation", 4, LC_PK_ROTATION }
};

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
	m_PieceInfo = pPieceInfo;
	m_nState = 0;
	m_nColor = 0;
	m_TimeShow = 1;
	m_TimeHide = LC_OBJECT_TIME_MAX;
	m_Name = "";
	m_pGroup = NULL;

	if (m_PieceInfo != NULL)
		m_PieceInfo->AddRef();

	float *values[] = { m_Position, m_AxisAngle };
	RegisterKeys (values, piece_key_info, LC_PK_COUNT);

	float pos[3] = { 0, 0, 0 }, rot[4] = { 0, 0, 1, 0 };
	ChangeKey (1, true, pos, LC_PK_POSITION);
	ChangeKey (1, true, rot, LC_PK_ROTATION);
}

lcPiece::~lcPiece()
{
	if (m_PieceInfo)
		m_PieceInfo->DeRef();
}

/////////////////////////////////////////////////////////////////////////////
// Piece save/load

// Use only when loading from a file
void lcPiece::SetPieceInfo(PieceInfo* pPieceInfo)
{
	m_PieceInfo = pPieceInfo;
	m_PieceInfo->AddRef();
}

bool lcPiece::FileLoad (File& file, char* name)
{
  unsigned char version, ch;

  file.ReadByte (&version, 1);

  if (version > LC_PIECE_SAVE_VERSION)
    return false;

  if (version > 8)
  {
    if (!lcObject::FileLoad (file))
      return false;

	if (version < 10)
	{
		for (LC_OBJECT_KEY* node = m_Keys; node; node = node->Next)
			if (node->Type == LC_PK_ROTATION)
				node->Value[3] *= LC_DTOR;
	}
  }

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

		if (type == LC_PK_ROTATION)
			param[3] *= LC_DTOR;

        ChangeKey(time, true, param, type);
      }

      file.ReadLong (&keys, 1);
      while (keys--)
      {
        file.ReadFloat (param, 4);
        file.ReadShort (&time, 1);
        file.ReadByte (&type, 1);
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
          ChangeKey(time, true, param, LC_PK_POSITION);

          mat.ToAxisAngle(param);
          ChangeKey(time, true, param, LC_PK_ROTATION);

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
        ChangeKey(1, true, param, LC_PK_POSITION);

        mat.ToAxisAngle(param);
        ChangeKey(1, true, param, LC_PK_ROTATION);
      }
    }
  }

  // Common to all versions.
  file.Read (name, 9);
  file.ReadByte (&m_nColor, 1); // TODO: fix colors to new mapping in 0.76

  if (version < 5)
  {
    const unsigned char conv[20] = { 0,2,4,9,7,6,22,8,10,11,14,16,18,9,21,20,22,8,10,11 }; // TODO: fix colors to new mapping in 0.76
    m_nColor = conv[m_nColor];
  }

  u8 step;
  file.ReadByte(&step, 1);
  m_TimeShow = step;
  if (version > 1)
  {
    file.ReadByte(&step, 1);
    m_TimeHide = step;
  }
  else
    m_TimeHide = LC_OBJECT_TIME_MAX;

  if (version > 5)
  {
    u16 sh;
    file.ReadShort(&sh, 1); // m_nFrameShow
    file.ReadShort(&sh, 1); // m_nFrameHide

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
    m_TimeShow = 1;
    m_TimeHide = LC_OBJECT_TIME_MAX;

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

  file.Write(m_PieceInfo->m_strName, 9);
  file.WriteLong(&m_nColor, 1); // TODO: fix color index to new tables in 0.76
  file.WriteLong(&m_TimeShow, 1);
  file.WriteLong(&m_TimeHide, 1);

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

void lcPiece::Initialize(float x, float y, float z, u32 Time, int Color)
{
  m_TimeShow = Time;
  m_nColor = Color;

  float pos[3] = { x, y, z }, rot[4] = { 0, 0, 1, 0 };
  ChangeKey(1, true, pos, LC_PK_POSITION);
  ChangeKey(1, true, rot, LC_PK_ROTATION);

  UpdatePosition(1);
}

void lcPiece::CreateName(lcPiece* pPiece)
{
	int i, max = 0;

	for (; pPiece; pPiece = (lcPiece*)pPiece->m_Next)
		if (strncmp(pPiece->m_Name, m_PieceInfo->m_strDescription, strlen(m_PieceInfo->m_strDescription)) == 0)
			if (sscanf((char*)pPiece->m_Name + strlen(m_PieceInfo->m_strDescription), " #%d", &i) == 1)
				if (i > max) 
					max = i;

	char buf[256];
	sprintf(buf, "%s #%.2d", m_PieceInfo->m_strDescription, max+1);
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

void lcPiece::InsertTime(u32 Start, u32 Time)
{
	u32 Total = lcGetActiveProject()->IsAnimation() ? lcGetActiveProject()->m_ActiveModel->m_TotalFrames : LC_OBJECT_TIME_MAX;

	if (m_TimeShow >= Start)
		m_TimeShow = min(m_TimeShow + Time, Total);

	if (m_TimeHide >= Start)
		m_TimeHide = min(m_TimeHide + Time, Total);

	if (m_TimeShow > lcGetActiveProject()->m_ActiveModel->m_CurFrame)
		Select(false, false, false);

	lcObject::InsertTime(Start, Time);
}

void lcPiece::RemoveTime(u32 Start, u32 Time)
{
	if (m_TimeShow >= Start)
		m_TimeShow = max(m_TimeShow - Time, 1);

	if (m_TimeHide >= Start)
		m_TimeHide = max(m_TimeHide - Time, 1);

	if (m_TimeHide < lcGetActiveProject()->m_ActiveModel->m_CurFrame)
		Select(false, false, false);

	lcObject::RemoveTime(Start, Time);
}

void lcPiece::ClosestLineIntersect(lcClickLine& ClickLine) const
{
	Matrix44 WorldModel = RotTranInverse(m_ModelWorld);

	Vector3 Start = Mul31(ClickLine.Start, WorldModel);
	Vector3 End = Mul31(ClickLine.End, WorldModel);

	// Check the bounding box distance first.
	float Dist;
	BoundingBox Box = BoundingBox(Vector3(m_PieceInfo->m_fDimensions[3], m_PieceInfo->m_fDimensions[4], m_PieceInfo->m_fDimensions[5]),
		                          Vector3(m_PieceInfo->m_fDimensions[0], m_PieceInfo->m_fDimensions[1], m_PieceInfo->m_fDimensions[2]));

	if (!BoundingBoxRayMinIntersectDistance(Box, Start, End, &Dist) || (Dist >= ClickLine.Dist))
		return;

	// Check mesh.
	if (!m_PieceInfo->m_Mesh->ClosestRayIntersect(Start, End, &Dist) || (Dist >= ClickLine.Dist))
		return;

	ClickLine.Object = this;
	ClickLine.Dist = Dist;
}

bool lcPiece::IntersectsVolume(const Vector4* Planes, int NumPlanes) const
{
	// First check the bounding box for quick rejection.
	Vector3 Box[8] =
	{
		Vector3(m_PieceInfo->m_fDimensions[0], m_PieceInfo->m_fDimensions[1], m_PieceInfo->m_fDimensions[5]),
		Vector3(m_PieceInfo->m_fDimensions[3], m_PieceInfo->m_fDimensions[1], m_PieceInfo->m_fDimensions[5]),
		Vector3(m_PieceInfo->m_fDimensions[0], m_PieceInfo->m_fDimensions[1], m_PieceInfo->m_fDimensions[2]),
		Vector3(m_PieceInfo->m_fDimensions[3], m_PieceInfo->m_fDimensions[4], m_PieceInfo->m_fDimensions[5]),
		Vector3(m_PieceInfo->m_fDimensions[3], m_PieceInfo->m_fDimensions[4], m_PieceInfo->m_fDimensions[2]),
		Vector3(m_PieceInfo->m_fDimensions[0], m_PieceInfo->m_fDimensions[4], m_PieceInfo->m_fDimensions[2]),
		Vector3(m_PieceInfo->m_fDimensions[0], m_PieceInfo->m_fDimensions[4], m_PieceInfo->m_fDimensions[5]),
		Vector3(m_PieceInfo->m_fDimensions[3], m_PieceInfo->m_fDimensions[1], m_PieceInfo->m_fDimensions[2])
	};

	// Transform the planes to model space.
	Matrix44 WorldModel = RotTranInverse(m_ModelWorld);

	Vector4* LocalPlanes = new Vector4[NumPlanes];
	int i;

	for (i = 0; i < NumPlanes; i++)
	{
		LocalPlanes[i] = Vector4(Mul30(Vector3(Planes[i]), WorldModel));
		LocalPlanes[i][3] = Planes[i][3] - Dot3(Vector3(WorldModel[3]), Vector3(LocalPlanes[i]));
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
	lcMesh* Mesh = m_PieceInfo->m_Mesh;
	float* verts = (float*)Mesh->m_VertexBuffer->MapBuffer(GL_READ_ONLY_ARB);
	void* indices = Mesh->m_IndexBuffer->MapBuffer(GL_READ_ONLY_ARB);
	bool ret = false;

	for (int s = 0; s < Mesh->m_SectionCount; s++)
	{
		lcMeshSection* Section = &Mesh->m_Sections[s];

		if (Section->PrimitiveType != GL_TRIANGLES)
			continue;

		if (Mesh->m_IndexType == GL_UNSIGNED_INT)
		{
			u32* IndexPtr = (u32*)((char*)indices + Section->IndexOffset);
			for (int i = 0; i < Section->IndexCount; i += 4)
			{
				if (PolygonIntersectsPlanes(&verts[IndexPtr[i+0]*3], &verts[IndexPtr[i+1]*3],
				    &verts[IndexPtr[i+2]*3], NULL, LocalPlanes, NumPlanes))
				{
					ret = true;
					break;
				}
			}
		}
		else
		{
			u16* IndexPtr = (u16*)((char*)indices + Section->IndexOffset);
			for (int i = 0; i < Section->IndexCount; i += 4)
			{
				if (PolygonIntersectsPlanes(&verts[IndexPtr[i+0]*3], &verts[IndexPtr[i+1]*3],
				    &verts[IndexPtr[i+2]*3], NULL, LocalPlanes, NumPlanes))
				{
					ret = true;
					break;
				}
			}
		}
	}

	Mesh->m_VertexBuffer->UnmapBuffer();
	Mesh->m_IndexBuffer->UnmapBuffer();

	delete[] LocalPlanes;

	return ret;
}

void lcPiece::Move(u32 Time, bool AddKey, const Vector3& Delta)
{
	m_Position += Delta;

	ChangeKey(Time, AddKey, m_Position, LC_PK_POSITION);
}

bool lcPiece::IsVisible(u32 Time)
{
	if (m_nState & LC_PIECE_HIDDEN)
		return false;

	if (m_TimeShow > Time) return false;
	if ((m_TimeHide == LC_OBJECT_TIME_MAX) || (m_TimeHide > Time))
		return true;

	return false;
}

void lcPiece::MergeBoundingBox(BoundingBox* Box)
{
	Vector3 Points[8];

	m_PieceInfo->m_BoundingBox.GetPoints(Points);

	for (int i = 0; i < 8; i++)
		Box->AddPoint(Mul31(Points[i], m_ModelWorld));
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

// Recalculates current position
void lcPiece::UpdatePosition(u32 Time)
{
	if (!IsVisible(Time))
		m_nState &= ~(LC_PIECE_SELECTED|LC_PIECE_FOCUSED);

	CalculateKeys(Time);

	m_ModelWorld = MatrixFromAxisAngle(m_AxisAngle);
	m_ModelWorld.SetTranslation(m_Position);
}

void lcPiece::RenderBox(bool bHilite, float fLineWidth)
{
	glPushMatrix();
	glMultMatrixf(m_ModelWorld);

#ifndef LC_OPENGLES
	if (bHilite && ((m_nState & LC_PIECE_SELECTED) != 0))
	{
		lcSetColor(m_nState & LC_PIECE_FOCUSED ? LC_COLOR_FOCUS : LC_COLOR_SELECTION);
		glLineWidth(2*fLineWidth);
		glPushAttrib(GL_POLYGON_BIT);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		m_PieceInfo->RenderBox();
		glPopAttrib();
		glLineWidth(fLineWidth);
	}
	else
#endif
	{
		lcSetColor(m_nColor);
		m_PieceInfo->RenderBox();
	}
	glPopMatrix();
}

void lcPiece::Render(bool bLighting, bool bEdges)
{
	glPushMatrix();
	glMultMatrixf(m_ModelWorld);

	m_PieceInfo->m_Mesh->Render(m_nColor, IsSelected(), IsFocused());

	glPopMatrix();
}
