#include "lc_global.h"
#include "lc_piece.h"

#include "lc_mesh.h"
#include "lc_scene.h"
#include "file.h"
#include "pieceinf.h"
#include "texture.h"
#include "globals.h"

lcPiece::lcPiece(PieceInfo* Info)
	: lcPieceObject(LC_OBJECT_PIECE)
{
	m_PieceInfo = NULL;
	SetPieceInfo(Info);
}

lcPiece::lcPiece(lcPiece* Piece)
	: lcPieceObject(LC_OBJECT_PIECE)
{
	// lcObject members.
	m_Next = NULL;
	m_Parent = NULL;
	m_Children = NULL;

	m_Position = Piece->m_Position;
	m_Name = "";
	m_Flags = Piece->m_Flags;

	for (int i = 0; i < LC_PIECEOBJ_NUMKEYS; i++)
		m_Keys[i] = Piece->m_Keys[i];

	// lcPieceObj members.
	m_Color = Piece->m_Color;
	m_TimeShow = Piece->m_TimeShow;
	m_TimeHide = Piece->m_TimeHide;
	m_Mesh = NULL;

	m_ModelWorld = Piece->m_ModelWorld;
	m_AxisAngle = Piece->m_AxisAngle;
	m_BoundingBox = Piece->m_BoundingBox;

	// lcPiece members.
	m_PieceInfo = NULL;
	SetPieceInfo(Piece->m_PieceInfo);
}

lcPiece::~lcPiece()
{
	if (m_PieceInfo)
		m_PieceInfo->DeRef();
}

void lcPiece::SetPieceInfo(PieceInfo* Info)
{
	if (m_PieceInfo)
		m_PieceInfo->DeRef();

	m_PieceInfo = Info;

	if (m_PieceInfo)
	{
		m_BoundingBox = BoundingBox(Vector3(m_PieceInfo->m_fDimensions[3], m_PieceInfo->m_fDimensions[4], m_PieceInfo->m_fDimensions[5]),
		                            Vector3(m_PieceInfo->m_fDimensions[0], m_PieceInfo->m_fDimensions[1], m_PieceInfo->m_fDimensions[2]));
		m_PieceInfo->AddRef();

		BuildMesh();
	}
}

void lcPiece::Update(u32 Time)
{
	lcPieceObject::Update(Time);
}

void lcPiece::AddToScene(lcScene* Scene, const Matrix44& ParentWorld, int Color)
{
	int RenderColor = (m_Color == LC_COL_DEFAULT) ? Color : m_Color;

	for (int i = 0; i < m_Mesh->m_SectionCount; i++)
	{
		lcRenderSection RenderSection;

		RenderSection.Owner = this;
		RenderSection.ModelWorld = Mul(m_ModelWorld, ParentWorld);
		RenderSection.Mesh = m_Mesh;
		RenderSection.Section = &m_Mesh->m_Sections[i];
		RenderSection.Color = RenderSection.Section->ColorIndex;

		if (RenderSection.Color == LC_COL_DEFAULT)
			RenderSection.Color = RenderColor;

		if (RenderSection.Section->PrimitiveType == GL_LINES)
		{
			// FIXME: LC_DET_BRICKEDGES
//			if ((m_nDetail & LC_DET_BRICKEDGES) == 0)
//				continue;

			if (IsFocused())
				RenderSection.Color = LC_COL_FOCUSED;
			else if (IsSelected())
				RenderSection.Color = LC_COL_SELECTED;
		}

		if (RenderSection.Color > 13 && RenderSection.Color < 22)
		{
			Vector3 Pos = Mul31(m_BoundingBox.GetCenter(), m_ModelWorld);
			Pos = Mul31(Pos, Scene->m_WorldView);
			RenderSection.Distance = Pos[2];

			Scene->m_TranslucentSections.AddSorted(RenderSection, SortRenderSectionsCallback, NULL);
		}
		else
		{
			// Pieces are already sorted by vertex buffer, so no need to sort again here.
			// FIXME: not true anymore, need to sort by vertex buffer here.
			Scene->m_OpaqueSections.Add(RenderSection);
		}
	}
}

void lcPiece::BuildMesh()
{
	delete m_Mesh;
	m_Mesh = NULL;

	int SectionIndices[LC_COL_DEFAULT+1][3];
	memset(SectionIndices, 0, sizeof(SectionIndices));

	int NumSections = 0, CurSection = 0;
	int NumIndices = 0;

	// Calculate the number of indices and sections.
	for (int i = 0; i < m_PieceInfo->m_nGroupCount; i++)
	{
		DRAWGROUP* dg = &m_PieceInfo->m_pGroups[i];
		unsigned short* sh = dg->connections;

		for (int s = 0; s < dg->NumSections; s++)
		{
			lcMeshSection* Section = &m_PieceInfo->GetMesh()->m_Sections[CurSection + s];

			switch (Section->PrimitiveType)
			{
			case GL_QUADS:
				if (!SectionIndices[Section->ColorIndex][0])
					NumSections++;
				SectionIndices[Section->ColorIndex][0] += Section->IndexCount;
				break;
			case GL_TRIANGLES:
				if (!SectionIndices[Section->ColorIndex][1])
					NumSections++;
				SectionIndices[Section->ColorIndex][1] += Section->IndexCount;
				break;
			case GL_LINES:
				if (!SectionIndices[Section->ColorIndex][2])
					NumSections++;
				SectionIndices[Section->ColorIndex][2] += Section->IndexCount;
				break;
			}

			NumIndices += Section->IndexCount;
		}

		CurSection += dg->NumSections;
	}

	lcMesh* Mesh = m_PieceInfo->GetMesh();
	m_Mesh = new lcMesh(NumSections, NumIndices, Mesh->m_VertexCount, Mesh->m_VertexBuffer);

	if (Mesh->m_VertexCount > 0xffff)
		BuildMesh(SectionIndices, TypeToType<u32>());
	else
		BuildMesh(SectionIndices, TypeToType<u16>());
}

template<typename T>
void lcPiece::BuildMesh(int SectionIndices[LC_COL_DEFAULT+1][3], TypeToType<T>)
{
	lcMeshEditor<T> MeshEdit(m_Mesh);

	lcMeshSection* DstSections[LC_COL_DEFAULT+1][3];
	memset(DstSections, 0, sizeof(DstSections));
	int CurSection = 0;

	void* SrcIndexBufer = m_PieceInfo->GetMesh()->m_IndexBuffer->MapBuffer(GL_READ_ONLY_ARB);

	// Copy indices.
	for (int i = 0; i < m_PieceInfo->m_nGroupCount; i++)
	{
		DRAWGROUP* dg = &m_PieceInfo->m_pGroups[i];

		for (int s = 0; s < dg->NumSections; s++)
		{
			lcMeshSection* SrcSection = &m_PieceInfo->GetMesh()->m_Sections[CurSection + s];
			int ReserveIndices = 0;

			switch (SrcSection->PrimitiveType)
			{
			case GL_QUADS:
				SectionIndices[SrcSection->ColorIndex][0] -= SrcSection->IndexCount;
				ReserveIndices = SectionIndices[SrcSection->ColorIndex][0];
				if (DstSections[SrcSection->ColorIndex][0])
					MeshEdit.SetCurrentSection(DstSections[SrcSection->ColorIndex][0]);
				else
					DstSections[SrcSection->ColorIndex][0] =  MeshEdit.StartSection(SrcSection->PrimitiveType, SrcSection->ColorIndex);
				break;
			case GL_TRIANGLES:
				SectionIndices[SrcSection->ColorIndex][1] -= SrcSection->IndexCount;
				ReserveIndices = SectionIndices[SrcSection->ColorIndex][1];
				if (DstSections[SrcSection->ColorIndex][1])
					MeshEdit.SetCurrentSection(DstSections[SrcSection->ColorIndex][1]);
				else
					DstSections[SrcSection->ColorIndex][1] =  MeshEdit.StartSection(SrcSection->PrimitiveType, SrcSection->ColorIndex);
				break;
			case GL_LINES:
				SectionIndices[SrcSection->ColorIndex][2] -= SrcSection->IndexCount;
				ReserveIndices = SectionIndices[SrcSection->ColorIndex][2];
				if (DstSections[SrcSection->ColorIndex][2])
					MeshEdit.SetCurrentSection(DstSections[SrcSection->ColorIndex][2]);
				else
					DstSections[SrcSection->ColorIndex][2] =  MeshEdit.StartSection(SrcSection->PrimitiveType, SrcSection->ColorIndex);
				break;
			}

			if (m_PieceInfo->m_nFlags & LC_PIECE_LONGDATA)
				MeshEdit.AddIndices32((char*)SrcIndexBufer + SrcSection->IndexOffset, SrcSection->IndexCount);
			else
				MeshEdit.AddIndices16((char*)SrcIndexBufer + SrcSection->IndexOffset, SrcSection->IndexCount);

			MeshEdit.EndSection(ReserveIndices);
		}

		CurSection += dg->NumSections;
	}

	m_PieceInfo->GetMesh()->m_IndexBuffer->UnmapBuffer();
}

void lcPiece::ClosestRayIntersect(LC_CLICK_RAY* Ray) const
{
	Matrix44 WorldModel = RotTranInverse(m_ModelWorld);
	Vector3 Start = Mul31(Ray->Start, WorldModel);
	Vector3 End = Mul31(Ray->End, WorldModel);

	// Check the bounding box distance first.
	float Dist;
	if (!BoundingBoxRayMinIntersectDistance(m_BoundingBox, Start, End, &Dist) || (Dist >= Ray->Dist))
		return;

	// Check each triangle.
	Vector3 Intersection;

	float* verts = (float*)m_PieceInfo->GetMesh()->m_VertexBuffer->MapBuffer(GL_READ_ONLY_ARB);
	void* indices = m_PieceInfo->GetMesh()->m_IndexBuffer->MapBuffer(GL_READ_ONLY_ARB);

	for (int s = 0; s < m_Mesh->m_SectionCount; s++)
	{
		lcMeshSection* Section = &m_Mesh->m_Sections[s];

		if (Section->PrimitiveType == GL_LINES)
			continue;

		if (Section->PrimitiveType == GL_QUADS)
		{
			if (m_PieceInfo->m_nFlags & LC_PIECE_LONGDATA)
			{
				u32* IndexPtr = (u32*)((char*)indices + Section->IndexOffset);
				for (int i = 0; i < Section->IndexCount; i += 4)
				{
					Vector3 v1(verts[IndexPtr[i+0]*3], verts[IndexPtr[i+0]*3+1], verts[IndexPtr[i+0]*3+2]);
					Vector3 v2(verts[IndexPtr[i+1]*3], verts[IndexPtr[i+1]*3+1], verts[IndexPtr[i+1]*3+2]);
					Vector3 v3(verts[IndexPtr[i+2]*3], verts[IndexPtr[i+2]*3+1], verts[IndexPtr[i+2]*3+2]);
					Vector3 v4(verts[IndexPtr[i+3]*3], verts[IndexPtr[i+3]*3+1], verts[IndexPtr[i+3]*3+2]);

					if (LineQuadMinIntersection(v1, v2, v3, v4, Start, End, &Ray->Dist, &Intersection))
					{
						Ray->Object = this;
					}
				}
			}
			else
			{
				u16* IndexPtr = (u16*)((char*)indices + Section->IndexOffset);
				for (int i = 0; i < Section->IndexCount; i += 4)
				{
					Vector3 v1(verts[IndexPtr[i+0]*3], verts[IndexPtr[i+0]*3+1], verts[IndexPtr[i+0]*3+2]);
					Vector3 v2(verts[IndexPtr[i+1]*3], verts[IndexPtr[i+1]*3+1], verts[IndexPtr[i+1]*3+2]);
					Vector3 v3(verts[IndexPtr[i+2]*3], verts[IndexPtr[i+2]*3+1], verts[IndexPtr[i+2]*3+2]);
					Vector3 v4(verts[IndexPtr[i+3]*3], verts[IndexPtr[i+3]*3+1], verts[IndexPtr[i+3]*3+2]);

					if (LineQuadMinIntersection(v1, v2, v3, v4, Start, End, &Ray->Dist, &Intersection))
					{
						Ray->Object = this;
					}
				}
			}
		}
		else if (Section->PrimitiveType == GL_TRIANGLES)
		{
			if (m_PieceInfo->m_nFlags & LC_PIECE_LONGDATA)
			{
				u32* IndexPtr = (u32*)((char*)indices + Section->IndexOffset);
				for (int i = 0; i < Section->IndexCount; i += 3)
				{
					Vector3 v1(verts[IndexPtr[i+0]*3], verts[IndexPtr[i+0]*3+1], verts[IndexPtr[i+0]*3+2]);
					Vector3 v2(verts[IndexPtr[i+1]*3], verts[IndexPtr[i+1]*3+1], verts[IndexPtr[i+1]*3+2]);
					Vector3 v3(verts[IndexPtr[i+2]*3], verts[IndexPtr[i+2]*3+1], verts[IndexPtr[i+2]*3+2]);

					if (LineTriangleMinIntersection(v1, v2, v3, Start, End, &Ray->Dist, &Intersection))
					{
						Ray->Object = this;
					}
				}
			}
			else
			{
				u16* IndexPtr = (u16*)((char*)indices + Section->IndexOffset);
				for (int i = 0; i < Section->IndexCount; i += 3)
				{
					Vector3 v1(verts[IndexPtr[i+0]*3], verts[IndexPtr[i+0]*3+1], verts[IndexPtr[i+0]*3+2]);
					Vector3 v2(verts[IndexPtr[i+1]*3], verts[IndexPtr[i+1]*3+1], verts[IndexPtr[i+1]*3+2]);
					Vector3 v3(verts[IndexPtr[i+2]*3], verts[IndexPtr[i+2]*3+1], verts[IndexPtr[i+2]*3+2]);

					if (LineTriangleMinIntersection(v1, v2, v3, Start, End, &Ray->Dist, &Intersection))
					{
						Ray->Object = this;
					}
				}
			}
		}
	}

	m_PieceInfo->GetMesh()->m_VertexBuffer->UnmapBuffer();
	m_PieceInfo->GetMesh()->m_IndexBuffer->UnmapBuffer();

}

bool lcPiece::IntersectsVolume(const class Vector4* Planes, int NumPlanes) const
{
	// First check the bounding box for quick rejection.
	Vector3 Box[8];
	m_BoundingBox.GetPoints(Box);

	// Transform the planes to local space.
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
	float* verts = (float*)m_PieceInfo->GetMesh()->m_VertexBuffer->MapBuffer(GL_READ_ONLY_ARB);
	void* indices = m_PieceInfo->GetMesh()->m_IndexBuffer->MapBuffer(GL_READ_ONLY_ARB);
	bool ret = false;

	for (int s = 0; s < m_Mesh->m_SectionCount; s++)
	{
		lcMeshSection* Section = &m_Mesh->m_Sections[s];

		if (Section->PrimitiveType == GL_LINES)
			continue;

		if (Section->PrimitiveType == GL_QUADS)
		{
			if (m_PieceInfo->m_nFlags & LC_PIECE_LONGDATA)
			{
				u32* IndexPtr = (u32*)((char*)indices + Section->IndexOffset);
				for (int i = 0; i < Section->IndexCount; i += 4)
				{
					if (PolygonIntersectsPlanes(&verts[IndexPtr[i+0]*3], &verts[IndexPtr[i+1]*3],
																			&verts[IndexPtr[i+2]*3], &verts[IndexPtr[i+3]*3], LocalPlanes, NumPlanes))
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
																			&verts[IndexPtr[i+2]*3], &verts[IndexPtr[i+3]*3], LocalPlanes, NumPlanes))
					{
						ret = true;
						break;
					}
				}
			}
		}
		else if (Section->PrimitiveType == GL_QUADS)
		{
			if (m_PieceInfo->m_nFlags & LC_PIECE_LONGDATA)
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
	}

	m_PieceInfo->GetMesh()->m_VertexBuffer->UnmapBuffer();
	m_PieceInfo->GetMesh()->m_IndexBuffer->UnmapBuffer();

	delete[] LocalPlanes;

	return ret;
}

void lcPiece::ExportLDraw(File& file) const
{
	/* FIXME: ldraw export
	float f[12], position[3], rotation[4];
	GetPosition(position);
	GetRotation(rotation);
	Matrix mat(rotation, position);
	mat.ToLDraw(f);
	sprintf(buf, " 1 %d %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %s.DAT\r\n",
	        col[m_Color], f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9], f[10], f[11], m_PieceInfo->m_strName);
	file.Write(buf, strlen(buf));
	*/
}
