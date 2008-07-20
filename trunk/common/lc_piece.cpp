#include "lc_global.h"
#include "lc_piece.h"

#include "lc_mesh.h"
#include "lc_scene.h"
#include "file.h"
#include "pieceinf.h"
#include "texture.h"
#include "lc_colors.h"
#include "typedefs.h"

lcPiece::lcPiece(PieceInfo* Info)
	: lcPieceObject(LC_OBJECT_PIECE)
{
	m_PieceInfo = NULL;
	SetPieceInfo(Info);
}

lcPiece::lcPiece(const lcPiece* Piece)
	: lcPieceObject(Piece)
{
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

void lcPiece::GetPieceList(lcObjArray<struct LC_PIECELIST_ENTRY>& Pieces, int Color) const
{
	LC_PIECELIST_ENTRY Entry;

	Entry.Info = m_PieceInfo;
	Entry.Color = (m_Color == LC_COLOR_DEFAULT) ? Color : m_Color;

	Pieces.Add(Entry);
}

void lcPiece::Update(u32 Time)
{
	lcPieceObject::Update(Time);
}

void lcPiece::AddToScene(lcScene* Scene, int Color)
{
	m_Mesh->AddToScene(Scene, m_ModelWorld, (m_Color == LC_COLOR_DEFAULT) ? Color : m_Color, this);

	if (IsSelected())
	{
		Vector3 Size = m_BoundingBox.m_Max - m_BoundingBox.m_Min;

		Matrix44 ScaleMatrix(Vector4(Size[0], 0.0f, 0.0f, 0.0f), Vector4(0.0f, Size[1], 0.0f, 0.0f), Vector4(0.0f, 0.0f, Size[2], 0.0f), Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		Matrix44 ModelWorld = Mul(ScaleMatrix, m_ModelWorld);
		ModelWorld.SetTranslation(m_ModelWorld.GetTranslation() + m_BoundingBox.GetCenter());

		lcSelectionMesh->AddToScene(Scene, ModelWorld, LC_COLOR_SELECTION, this);
	}
}

void lcPiece::BuildMesh()
{
	delete m_Mesh;
	m_Mesh = NULL;

	int* SectionIndices = new int[lcNumColors*3];
	memset(SectionIndices, 0, sizeof(int)*lcNumColors*3);

	int NumSections = 0, CurSection = 0;
	int NumIndices = 0;

	// Calculate the number of indices and sections.
	for (int i = 0; i < m_PieceInfo->m_nGroupCount; i++)
	{
		DRAWGROUP* dg = &m_PieceInfo->m_pGroups[i];

		for (int s = 0; s < dg->NumSections; s++)
		{
			lcMeshSection* Section = &m_PieceInfo->GetMesh()->m_Sections[CurSection + s];
			int SrcColor = 0;

			switch (Section->PrimitiveType)
			{
			case GL_QUADS:
				SrcColor = Section->ColorIndex*3+0;
				break;
			case GL_TRIANGLES:
				SrcColor = Section->ColorIndex*3+1;
				break;
			case GL_LINES:
				SrcColor = Section->ColorIndex*3+2;
				break;
			}

			if (!SectionIndices[SrcColor])
				NumSections++;
			SectionIndices[SrcColor] += Section->IndexCount;

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

	delete[] SectionIndices;
}

template<typename T>
void lcPiece::BuildMesh(int* SectionIndices, TypeToType<T>)
{
	lcMeshEditor<T> MeshEdit(m_Mesh);

	lcMeshSection** DstSections = new lcMeshSection*[lcNumColors*3];
	memset(DstSections, 0, sizeof(DstSections[0])*lcNumColors*3);
	int CurSection = 0;

	void* SrcIndexBufer = m_PieceInfo->GetMesh()->m_IndexBuffer->MapBuffer(GL_READ_ONLY_ARB);

	// Copy indices.
	for (int i = 0; i < m_PieceInfo->m_nGroupCount; i++)
	{
		DRAWGROUP* dg = &m_PieceInfo->m_pGroups[i];

		for (int s = 0; s < dg->NumSections; s++)
		{
			lcMeshSection* SrcSection = &m_PieceInfo->GetMesh()->m_Sections[CurSection + s];
			int SrcColor = 0;

			switch (SrcSection->PrimitiveType)
			{
			case GL_QUADS:
				SrcColor = SrcSection->ColorIndex*3+0;
				break;
			case GL_TRIANGLES:
				SrcColor = SrcSection->ColorIndex*3+1;
				break;
			case GL_LINES:
				SrcColor = SrcSection->ColorIndex*3+2;
				break;
			}

			SectionIndices[SrcColor] -= SrcSection->IndexCount;
			int ReserveIndices = SectionIndices[SrcColor];
			if (DstSections[SrcColor])
			{
				MeshEdit.SetCurrentSection(DstSections[SrcColor]);
				DstSections[SrcColor]->Box += SrcSection->Box;
			}
			else
			{
				DstSections[SrcColor] = MeshEdit.StartSection(SrcSection->PrimitiveType, SrcSection->ColorIndex);
				DstSections[SrcColor]->Box = SrcSection->Box;
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
	delete[] DstSections;
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

	// Check mesh.
	if (!m_Mesh->ClosestRayIntersect(Start, End, &Dist) || (Dist >= Ray->Dist))
		return;

	Ray->Object = this;
	Ray->Dist = Dist;
}

bool lcPiece::IntersectsVolume(const Vector4* Planes, int NumPlanes) const
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
