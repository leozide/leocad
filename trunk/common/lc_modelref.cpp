#include "lc_global.h"
#include "lc_modelref.h"

#include "lc_model.h"
#include "lc_mesh.h"

lcModelRef::lcModelRef(lcModel* Model)
	: lcPieceObject(LC_OBJECT_MODELREF)
{
	m_Model = Model;
	m_Mesh = NULL;
	m_BoundingBox = Model->m_BoundingBox;
}

lcModelRef::lcModelRef(lcModelRef* ModelRef)
	: lcPieceObject(LC_OBJECT_MODELREF)
{
	// lcObject members.
	m_Next = NULL;
	m_Parent = NULL;
	m_Children = NULL;

	m_Position = ModelRef->m_Position;
	m_Name = "";
	m_Flags = ModelRef->m_Flags;

	for (int i = 0; i < LC_PIECEOBJ_NUMKEYS; i++)
		m_Keys[i] = ModelRef->m_Keys[i];

	// lcPieceObj members.
	m_Color = ModelRef->m_Color;
	m_TimeShow = ModelRef->m_TimeShow;
	m_TimeHide = ModelRef->m_TimeHide;
	m_Mesh = NULL;

	m_ModelWorld = ModelRef->m_ModelWorld;
	m_AxisAngle = ModelRef->m_AxisAngle;
	m_BoundingBox = ModelRef->m_BoundingBox;

	// lcModelRef members.
	m_Model = ModelRef->m_Model;
	m_Mesh = NULL;
}

lcModelRef::~lcModelRef()
{
}

void lcModelRef::Update(u32 Time)
{
	lcPieceObject::Update(Time);
}

void lcModelRef::AddToScene(lcScene* Scene, const Matrix44& ParentWorld, int Color)
{
	// FIXME: prebuild mesh and use it
	m_Model->AddToScene(Scene, Mul(m_ModelWorld, ParentWorld), Color);
}

void lcModelRef::ClosestRayIntersect(LC_CLICK_RAY* Ray) const
{
	// FIXME: modelref intersect
}

bool lcModelRef::IntersectsVolume(const class Vector4* Planes, int NumPlanes) const
{
	// FIXME: modelref intersect
	return false;
}

void lcModelRef::BuildMesh()
{
	delete m_Mesh;
	m_Mesh = NULL;

	// Calculate the mesh size.
	int SectionIndices[LC_COL_DEFAULT+1][3];
	memset(SectionIndices, 0, sizeof(SectionIndices));

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

		NumVertices += Mesh->m_VertexCount;
	}

	// Create a new mesh.
	m_Mesh = new lcMesh(NumSections, NumIndices, NumVertices, NULL);

	if (NumVertices > 0xffff)
		BuildMesh(SectionIndices, TypeToType<u32>());
	else
		BuildMesh(SectionIndices, TypeToType<u16>());
}

template<typename T>
void lcModelRef::BuildMesh(int SectionIndices[LC_COL_DEFAULT+1][3], TypeToType<T>)
{
	// Copy data from all meshes into the new mesh.
	lcMeshEditor<T> MeshEdit(m_Mesh);

	lcMeshSection* DstSections[LC_COL_DEFAULT+1][3];
	memset(DstSections, 0, sizeof(DstSections));

	for (lcPieceObject* Piece = m_Model->m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
	{
		lcMesh* SrcMesh = Piece->m_Mesh;

		void* SrcIndexBufer = SrcMesh->m_IndexBuffer->MapBuffer(GL_READ_ONLY_ARB);

		for (int i = 0; i < SrcMesh->m_SectionCount; i++)
		{
			lcMeshSection* SrcSection = &SrcMesh->m_Sections[i];
			int ReserveIndices = 0;

			// Create a new section if needed.
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

			// Copy indices.
			if (SrcMesh->m_VertexCount > 0xffff)
				MeshEdit.AddIndices32((char*)SrcIndexBufer + SrcSection->IndexOffset, SrcSection->IndexCount);
			else
				MeshEdit.AddIndices16((char*)SrcIndexBufer + SrcSection->IndexOffset, SrcSection->IndexCount);

			// Fix the indices to point to the right place after the vertex buffers are merged.
			MeshEdit.OffsetIndices(MeshEdit.m_CurIndex - SrcSection->IndexCount, SrcSection->IndexCount, SrcMesh->m_VertexCount);

			MeshEdit.EndSection(ReserveIndices);
		}

		SrcMesh->m_IndexBuffer->UnmapBuffer();

		// Copy vertices.
		void* SrcVertexBuffer = SrcMesh->m_VertexBuffer->MapBuffer(GL_READ_ONLY_ARB);
		MeshEdit.AddVertices((float*)SrcVertexBuffer, SrcMesh->m_VertexCount);
		SrcMesh->m_VertexBuffer->UnmapBuffer();
	}
}
