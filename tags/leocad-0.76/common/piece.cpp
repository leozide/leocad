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

lcPiece::lcPiece(PieceInfo* pPieceInfo)
  : lcObject(LC_OBJECT_PIECE)
{
	m_PieceInfo = pPieceInfo;
	m_nState = 0;
	m_Color = 0;
	m_TimeShow = 1;
	m_TimeHide = LC_OBJECT_TIME_MAX;
	m_Group = NULL;

	if (m_PieceInfo != NULL)
		m_PieceInfo->AddRef();

	float *values[] = { m_Position, m_AxisAngle };
	RegisterKeys(values, piece_key_info, LC_PK_COUNT);

	float pos[3] = { 0, 0, 0 }, rot[4] = { 0, 0, 1, 0 };
	ChangeKey(1, true, pos, LC_PK_POSITION);
	ChangeKey(1, true, rot, LC_PK_ROTATION);
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

bool lcPiece::FileLoad(lcFile& file, char* name)
{
	u8 version, ch;

	file.ReadBytes(&version);

	if (version > LC_PIECE_SAVE_VERSION)
		return false;

	if (version > 8)
	{
		if (!lcObject::FileLoad(file))
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
		u16 time;
		float param[4];
		u8 type;

		if (version > 5)
		{
			u32 keys;

			file.ReadInts(&keys, 1);
			while (keys--)
			{
				file.ReadFloats(param, 4);
				file.ReadShorts(&time, 1);
				file.ReadBytes(&type, 1);

				if (type == LC_PK_ROTATION)
					param[3] *= LC_DTOR;

				ChangeKey(time, true, param, type);
			}

			file.ReadInts(&keys, 1);
			while (keys--)
			{
				file.ReadFloats(param, 4);
				file.ReadShorts(&time, 1);
				file.ReadBytes(&type, 1);
			}
		}
		else
		{
			if (version > 2)
			{
				file.Read(&ch, 1);

				while (ch--)
				{
					Matrix mat;
					if (version > 3)
					{
						file.ReadFloats(mat.m, 16);
					}
					else
					{
						float move[3], rotate[3];
						file.ReadFloats(move, 3);
						file.ReadFloats(rotate, 3);
						mat.CreateOld(move[0], move[1], move[2], rotate[0], rotate[1], rotate[2]);
					}

					unsigned char b;
					file.ReadBytes(&b, 1);
					time = b;

					mat.GetTranslation(&param[0], &param[1], &param[2]);
					param[3] = 0;
					ChangeKey(time, true, param, LC_PK_POSITION);

					mat.ToAxisAngle(param);
					ChangeKey(time, true, param, LC_PK_ROTATION);

					int bl;
					file.ReadInts(&bl, 1);
				}
			}
			else
			{
				Matrix mat;
				float move[3], rotate[3];
				file.ReadFloats(move, 3);
				file.ReadFloats(rotate, 3);
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
	file.Read(name, 9);
	if (version < 10)
	{
		u8 Color;
		file.ReadBytes(&Color, 1);

		if (version < 5)
		{
			// Convert from the old values to the 0.75 colors.
			const int ColorTable[20] = { 0,2,4,9,7,6,22,8,10,11,14,16,18,9,21,20,22,8,10,11 };
			Color = ColorTable[Color];
		}

		// Convert from 0.75 to LDraw.
		const int ColorTable[28] = { 4,25,2,10,1,9,14,15,8,0,6,13,13,334,36,44,34,42,33,41,46,47,7,382,6,13,11,383 };
		m_Color = lcConvertLDrawColor(ColorTable[Color]);
	}
	else
	{
		u32 Color;
		file.ReadInts(&Color, 1);
		m_Color = lcConvertLDrawColor(Color);
	}

	if (version < 10)
	{
		u8 step;
		file.ReadBytes(&step, 1);
		m_TimeShow = step;
		if (version > 1)
		{
			file.ReadBytes(&step, 1);
			m_TimeHide = step;
		}
		else
			m_TimeHide = LC_OBJECT_TIME_MAX;
	}
	else
	{
		file.ReadInts(&m_TimeShow, 1);
		file.ReadInts(&m_TimeHide, 1);
	}

	if (version > 5)
	{
		if (version < 10)
		{
			u16 sh;
			file.ReadShorts(&sh, 1); // m_nFrameShow
			file.ReadShorts(&sh, 1); // m_nFrameHide
		}

		if (version > 7)
		{
			file.ReadBytes(&m_nState, 1);
			Select (false, false, false);
			file.ReadBytes(&ch, 1);
			char buf[81];
			file.Read(buf, ch);
			buf[80] = 0;
			m_Name = buf;
		}
		else
		{
			int hide;
			file.ReadInts(&hide, 1);
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
			file.ReadInts(&i, 1);
		m_Group = (lcGroup*)i;
	}
	else
	{
		m_TimeShow = 1;
		m_TimeHide = LC_OBJECT_TIME_MAX;

		file.ReadBytes(&ch, 1);
		if (ch == 0)
			m_Group = (lcGroup*)-1;
		else
			m_Group = (lcGroup*)(u32)ch;

		file.ReadBytes(&ch, 1);
		if (ch & 0x01)
			m_nState |= LC_PIECE_HIDDEN;
	}

	return true;
}

void lcPiece::FileSave(lcFile& file, lcGroup* Groups)
{
  u8 ch = LC_PIECE_SAVE_VERSION;

  file.WriteBytes(&ch, 1);

  lcObject::FileSave(file);

  file.Write(m_PieceInfo->m_strName, 9);
  file.WriteInts(&g_ColorList[m_Color].Code, 1);
  file.WriteInts(&m_TimeShow, 1);
  file.WriteInts(&m_TimeHide, 1);

  // version 8
  file.WriteBytes(&m_nState, 1);
  ch = m_Name.GetLength();
  file.WriteBytes(&ch, 1);
  file.Write((const char*)m_Name, ch);

  // version 7
  int i;
  if (m_Group != NULL)
  {
    for (i = 0; Groups; Groups = Groups->m_Next)
    {
      if (m_Group == Groups)
        break;
      i++;
    }
  }
  else
    i = -1;
  file.WriteInts(&i, 1);
}

void lcPiece::Initialize(float x, float y, float z, u32 Time, int Color)
{
  m_TimeShow = Time;
  m_Color = Color;

  float pos[3] = { x, y, z }, rot[4] = { 0, 0, 1, 0 };
  ChangeKey(1, true, pos, LC_PK_POSITION);
  ChangeKey(1, true, rot, LC_PK_ROTATION);

  UpdatePosition(1);
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
			for (u32 i = 0; i < Section->IndexCount; i += 3)
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
			for (u32 i = 0; i < Section->IndexCount; i += 3)
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

lcGroup* lcPiece::GetTopGroup()
{
	return m_Group ? m_Group->GetTopGroup() : NULL;
}

void lcPiece::DoGroup(lcGroup* Group)
{
	if (m_Group != NULL && m_Group != (lcGroup*)-1 && m_Group > (lcGroup*)0xFFFF)
		m_Group->SetGroup(Group);
	else
		m_Group = Group;
}

void lcPiece::UnGroup(lcGroup* pGroup)
{
	if ((m_Group == pGroup) || (pGroup == NULL))
		m_Group = NULL;
	else
		if (m_Group != NULL)
			m_Group->UnGroup(pGroup);
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
		lcSetColor(m_Color);
		m_PieceInfo->RenderBox();
	}
	glPopMatrix();
}

void lcPiece::Render(bool bLighting, bool bEdges)
{
	glPushMatrix();
	glMultMatrixf(m_ModelWorld);

	m_PieceInfo->m_Mesh->Render(m_Color, IsSelected(), IsFocused());

	glPopMatrix();
}
