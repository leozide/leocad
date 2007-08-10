#include "lc_global.h"
#include "lc_modelref.h"

#include "lc_model.h"
#include "lc_mesh.h"
#include "lc_scene.h"
#include "lc_colors.h"

lcModelRef::lcModelRef(lcModel* Model)
	: lcPieceObject(LC_OBJECT_MODELREF)
{
	m_Model = Model;
	m_Mesh = NULL;
	m_BoundingBox = Model->m_BoundingBox;
}

lcModelRef::lcModelRef(const lcModelRef* ModelRef)
	: lcPieceObject(ModelRef)
{
	// lcModelRef members.
	m_Model = ModelRef->m_Model;
	m_Mesh = NULL;
}

lcModelRef::~lcModelRef()
{
}

void lcModelRef::GetPieceList(ObjArray<struct LC_PIECELIST_ENTRY>& Pieces, int Color) const
{
	m_Model->GetPieceList(Pieces, Color);
}

void lcModelRef::Update(u32 Time)
{
	lcPieceObject::Update(Time);

	// FIXME: don't call BuildMesh() here
	BuildMesh();
}

void lcModelRef::AddToScene(lcScene* Scene, int Color)
{
	m_Mesh->AddToScene(Scene, m_ModelWorld, (m_Color == LC_COLOR_DEFAULT) ? Color : m_Color, this);
}

void lcModelRef::ClosestRayIntersect(LC_CLICK_RAY* Ray) const
{
	// FIXME: modelref intersect
}

bool lcModelRef::IntersectsVolume(const Vector4* Planes, int NumPlanes) const
{
	// FIXME: modelref intersect
	return false;
}

void lcModelRef::BuildMesh()
{
	delete m_Mesh;
	m_Mesh = NULL;

	// Calculate the mesh size.
	int* SectionIndices = new int[lcNumColors*3];
	memset(SectionIndices, 0, sizeof(SectionIndices[0])*lcNumColors*3);

	int NumSections = 0;
	int NumVertices = 0;
	int NumIndices = 0;
// FIXME: make sure recursive models all have their meshes ready first.
	for (lcPieceObject* Piece = m_Model->m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
	{
		lcMesh* Mesh = Piece->m_Mesh;

		for (int i = 0; i < Mesh->m_SectionCount; i++)
		{
			lcMeshSection* Section = &Mesh->m_Sections[i];
			int SrcColor;

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

		NumVertices += Mesh->m_VertexCount;
	}

	// Create a new mesh.
	m_Mesh = new lcMesh(NumSections, NumIndices, NumVertices, NULL);

	if (NumVertices > 0xffff)
		BuildMesh(SectionIndices, TypeToType<u32>());
	else
		BuildMesh(SectionIndices, TypeToType<u16>());

	delete[] SectionIndices;
}

template<typename T>
void lcModelRef::BuildMesh(int* SectionIndices, TypeToType<T>)
{
	// Copy data from all meshes into the new mesh.
	lcMeshEditor<T> MeshEdit(m_Mesh);

	lcMeshSection** DstSections = new lcMeshSection*[lcNumColors*3];
	memset(DstSections, 0, sizeof(DstSections[0])*lcNumColors*3);

	for (lcPieceObject* Piece = m_Model->m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
	{
		lcMesh* SrcMesh = Piece->m_Mesh;

		void* SrcIndexBufer = SrcMesh->m_IndexBuffer->MapBuffer(GL_READ_ONLY_ARB);

		for (int i = 0; i < SrcMesh->m_SectionCount; i++)
		{
			lcMeshSection* SrcSection = &SrcMesh->m_Sections[i];
			int SrcColor;

			// Create a new section if needed.
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

			// Copy indices.
			if (SrcMesh->m_VertexCount > 0xffff)
				MeshEdit.AddIndices32((char*)SrcIndexBufer + SrcSection->IndexOffset, SrcSection->IndexCount);
			else
				MeshEdit.AddIndices16((char*)SrcIndexBufer + SrcSection->IndexOffset, SrcSection->IndexCount);

			// Fix the indices to point to the right place after the vertex buffers are merged.
			MeshEdit.OffsetIndices(MeshEdit.m_CurIndex - SrcSection->IndexCount, SrcSection->IndexCount, MeshEdit.m_CurVertex);

			MeshEdit.EndSection(ReserveIndices);
		}

		SrcMesh->m_IndexBuffer->UnmapBuffer();

		// Transform and copy vertices.
		float* SrcVertexBuffer = (float*)SrcMesh->m_VertexBuffer->MapBuffer(GL_READ_ONLY_ARB);

		for (int i = 0; i < SrcMesh->m_VertexCount; i++)
		{
			float* SrcPtr = SrcVertexBuffer + 3 * i;
			Vector3 Vert(SrcPtr[0], SrcPtr[1], SrcPtr[2]);
			Vert = Mul31(Vert, Piece->m_ModelWorld);
			MeshEdit.AddVertex(Vert);
		}

		SrcMesh->m_VertexBuffer->UnmapBuffer();
	}
}
