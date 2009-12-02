#include "lc_global.h"
#include "lc_mesh.h"

#include <malloc.h>
#include <stdlib.h>
//#include "lc_scene.h"
//#include "lc_pieceobj.h"
#include "lc_colors.h"
#include "opengl.h"
#include "system.h"
#include "debug.h"

lcMesh* lcSphereMesh;
lcMesh* lcBoxMesh;
lcMesh* lcWireframeBoxMesh;
lcMesh* lcSelectionMesh;

void lcCreateDefaultMeshes()
{
	lcSphereMesh = lcCreateSphereMesh(1.0f, 16);
	lcBoxMesh = lcCreateBoxMesh(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));
	lcWireframeBoxMesh = lcCreateWireframeBoxMesh(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));

	lcSelectionMesh = new lcMesh(1, 48, 32, NULL);

	lcMeshEditor<u16> MeshEdit(lcSelectionMesh);
	lcMeshSection* Section = MeshEdit.StartSection(GL_LINES, LC_COLOR_DEFAULT);

	Section->Box.m_Max = Vector3(0.5f,0.5f,0.5f);
	Section->Box.m_Min = Vector3(-0.5f,-0.5f,-0.5f);

	Vector3 SizeX(0.2f, 0.0f, 0.0f);
	Vector3 SizeY(0.0f, 0.2f, 0.0f);
	Vector3 SizeZ(0.0f, 0.0f, 0.2f);
	Vector3 Point;

	Point = Vector3(0.5f, 0.5f, -0.5f);
	MeshEdit.AddVertex(Point);
	MeshEdit.AddVertex(Point - SizeX);
	MeshEdit.AddVertex(Point - SizeY);
	MeshEdit.AddVertex(Point + SizeZ);

	Point = Vector3(-0.5f, 0.5f, -0.5f);
	MeshEdit.AddVertex(Point);
	MeshEdit.AddVertex(Point + SizeX);
	MeshEdit.AddVertex(Point - SizeY);
	MeshEdit.AddVertex(Point + SizeZ);

	Point = Vector3(0.5f, 0.5f, 0.5f);
	MeshEdit.AddVertex(Point);
	MeshEdit.AddVertex(Point - SizeX);
	MeshEdit.AddVertex(Point - SizeY);
	MeshEdit.AddVertex(Point - SizeZ);

	Point = Vector3(-0.5f, -0.5f, -0.5f);
	MeshEdit.AddVertex(Point);
	MeshEdit.AddVertex(Point + SizeX);
	MeshEdit.AddVertex(Point + SizeY);
	MeshEdit.AddVertex(Point + SizeZ);

	Point = Vector3(-0.5f, -0.5f, 0.5f);
	MeshEdit.AddVertex(Point);
	MeshEdit.AddVertex(Point + SizeX);
	MeshEdit.AddVertex(Point + SizeY);
	MeshEdit.AddVertex(Point - SizeZ);

	Point = Vector3(0.5f, -0.5f, 0.5f);
	MeshEdit.AddVertex(Point);
	MeshEdit.AddVertex(Point - SizeX);
	MeshEdit.AddVertex(Point + SizeY);
	MeshEdit.AddVertex(Point - SizeZ);

	Point = Vector3(0.5f, -0.5f, -0.5f);
	MeshEdit.AddVertex(Point);
	MeshEdit.AddVertex(Point - SizeX);
	MeshEdit.AddVertex(Point + SizeY);
	MeshEdit.AddVertex(Point + SizeZ);

	Point = Vector3(-0.5f, 0.5f, 0.5f);
	MeshEdit.AddVertex(Point);
	MeshEdit.AddVertex(Point + SizeX);
	MeshEdit.AddVertex(Point - SizeY);
	MeshEdit.AddVertex(Point - SizeZ);

	for (int i = 0; i < 8; i++)
	{
		MeshEdit.AddIndex(i*4);
		MeshEdit.AddIndex(i*4+1);
		MeshEdit.AddIndex(i*4);
		MeshEdit.AddIndex(i*4+2);
		MeshEdit.AddIndex(i*4);
		MeshEdit.AddIndex(i*4+3);
	}

	MeshEdit.EndSection();
}

void lcDestroyDefaultMeshes()
{
	delete lcSphereMesh;
	lcSphereMesh = NULL;
	delete lcBoxMesh;
	lcBoxMesh = NULL;
	delete lcWireframeBoxMesh;
	lcWireframeBoxMesh = NULL;
	delete lcSelectionMesh;
	lcSelectionMesh = NULL;
}

lcMesh* lcCreateSphereMesh(float Radius, int Slices)
{
	int NumIndices = 3 * Slices + 6 * Slices * (Slices - 2) + 3 * Slices;
	int NumVertices = (Slices - 1) * Slices + 2;
	int i;

	lcMesh* SphereMesh = new lcMesh(1, NumIndices, NumVertices, NULL);

	lcMeshEditor<u16> MeshEdit(SphereMesh);
	lcMeshSection* Section = MeshEdit.StartSection(GL_TRIANGLES, LC_COLOR_DEFAULT);

	Section->Box.m_Min = Vector3(-Radius, -Radius, -Radius);
	Section->Box.m_Max = Vector3(Radius, Radius, Radius);

	MeshEdit.AddVertex(Vector3(0, 0, Radius));

	for (i = 1; i < Slices; i++ )
	{
		float r0 = Radius * sinf (i * (LC_PI / Slices));
		float z0 = Radius * cosf (i * (LC_PI / Slices));

		for (int j = 0; j < Slices; j++)
		{
			float x0 = r0 * sinf(j * (LC_2PI / Slices));
			float y0 = r0 * cosf(j * (LC_2PI / Slices));

			MeshEdit.AddVertex(Vector3(x0, y0, z0));
		}
	}

	MeshEdit.AddVertex(Vector3(0, 0, -Radius));

	for (i = 0; i < Slices - 1; i++ )
	{
		MeshEdit.AddIndex(0);
		MeshEdit.AddIndex(1 + i);
		MeshEdit.AddIndex(1 + i + 1);
	}

	MeshEdit.AddIndex(0);
	MeshEdit.AddIndex(1);
	MeshEdit.AddIndex(1 + Slices - 1);

	for (int i = 0; i < Slices - 2; i++ )
	{
		int Row1 = 1 + i * Slices;
		int Row2 = 1 + (i + 1) * Slices;

		for (int j = 0; j < Slices - 1; j++ )
		{
			MeshEdit.AddIndex(Row1 + j);
			MeshEdit.AddIndex(Row2 + j + 1);
			MeshEdit.AddIndex(Row2 + j);

			MeshEdit.AddIndex(Row1 + j);
			MeshEdit.AddIndex(Row1 + j + 1);
			MeshEdit.AddIndex(Row2 + j + 1);
		}

		MeshEdit.AddIndex(Row1 + Slices - 1);
		MeshEdit.AddIndex(Row2 + 0);
		MeshEdit.AddIndex(Row2 + Slices - 1);

		MeshEdit.AddIndex(Row1 + Slices - 1);
		MeshEdit.AddIndex(Row2 + 0);
		MeshEdit.AddIndex(Row1 + 0);
	}

	for (i = 0; i < Slices - 1; i++ )
	{
		MeshEdit.AddIndex((Slices - 1) * Slices + 1);
		MeshEdit.AddIndex((Slices - 1) * (Slices - 1) + i);
		MeshEdit.AddIndex((Slices - 1) * (Slices - 1) + i + 1);
	}

	MeshEdit.AddIndex((Slices - 1) * Slices + 1);
	MeshEdit.AddIndex((Slices - 1) * (Slices - 1) + (Slices - 2) + 1);
	MeshEdit.AddIndex((Slices - 1) * (Slices - 1));

	MeshEdit.EndSection();

	return SphereMesh;
}

lcMesh* lcCreateBoxMesh(const Vector3& Min, const Vector3& Max)
{
	int NumIndices = 24;
	int NumVertices = 8;

	lcMesh* BoxMesh = new lcMesh(1, NumIndices, NumVertices, NULL);

	lcMeshEditor<u16> MeshEdit(BoxMesh);
	lcMeshSection* Section = MeshEdit.StartSection(GL_QUADS, LC_COLOR_DEFAULT);

	Section->Box.m_Min = Min;
	Section->Box.m_Max = Max;

	MeshEdit.AddVertex(Vector3(Min[0], Min[1], Min[2]));
	MeshEdit.AddVertex(Vector3(Min[0], Max[1], Min[2]));
	MeshEdit.AddVertex(Vector3(Max[0], Max[1], Min[2]));
	MeshEdit.AddVertex(Vector3(Max[0], Min[1], Min[2]));
	MeshEdit.AddVertex(Vector3(Min[0], Min[1], Max[2]));
	MeshEdit.AddVertex(Vector3(Min[0], Max[1], Max[2]));
	MeshEdit.AddVertex(Vector3(Max[0], Max[1], Max[2]));
	MeshEdit.AddVertex(Vector3(Max[0], Min[1], Max[2]));

	MeshEdit.AddIndex(0);
	MeshEdit.AddIndex(1);
	MeshEdit.AddIndex(2);
	MeshEdit.AddIndex(3);
	MeshEdit.AddIndex(7);
	MeshEdit.AddIndex(6);
	MeshEdit.AddIndex(5);
	MeshEdit.AddIndex(4);

	MeshEdit.AddIndex(0);
	MeshEdit.AddIndex(1);
	MeshEdit.AddIndex(5);
	MeshEdit.AddIndex(4);
	MeshEdit.AddIndex(2);
	MeshEdit.AddIndex(3);
	MeshEdit.AddIndex(7);
	MeshEdit.AddIndex(6);

	MeshEdit.AddIndex(0);
	MeshEdit.AddIndex(3);
	MeshEdit.AddIndex(7);
	MeshEdit.AddIndex(4);
	MeshEdit.AddIndex(1);
	MeshEdit.AddIndex(2);
	MeshEdit.AddIndex(6);
	MeshEdit.AddIndex(5);

	MeshEdit.EndSection();

	return BoxMesh;
}

lcMesh* lcCreateWireframeBoxMesh(const Vector3& Min, const Vector3& Max)
{
	int NumIndices = 24;
	int NumVertices = 8;

	lcMesh* BoxMesh = new lcMesh(1, NumIndices, NumVertices, NULL);

	lcMeshEditor<u16> MeshEdit(BoxMesh);
	lcMeshSection* Section = MeshEdit.StartSection(GL_LINES, LC_COLOR_DEFAULT);

	Section->Box.m_Min = Min;
	Section->Box.m_Max = Max;

	MeshEdit.AddVertex(Vector3(Min[0], Min[1], Min[2]));
	MeshEdit.AddVertex(Vector3(Min[0], Max[1], Min[2]));
	MeshEdit.AddVertex(Vector3(Max[0], Max[1], Min[2]));
	MeshEdit.AddVertex(Vector3(Max[0], Min[1], Min[2]));
	MeshEdit.AddVertex(Vector3(Min[0], Min[1], Max[2]));
	MeshEdit.AddVertex(Vector3(Min[0], Max[1], Max[2]));
	MeshEdit.AddVertex(Vector3(Max[0], Max[1], Max[2]));
	MeshEdit.AddVertex(Vector3(Max[0], Min[1], Max[2]));

	MeshEdit.AddIndex(0);
	MeshEdit.AddIndex(1);
	MeshEdit.AddIndex(1);
	MeshEdit.AddIndex(2);
	MeshEdit.AddIndex(2);
	MeshEdit.AddIndex(3);
	MeshEdit.AddIndex(3);
	MeshEdit.AddIndex(0);

	MeshEdit.AddIndex(4);
	MeshEdit.AddIndex(5);
	MeshEdit.AddIndex(5);
	MeshEdit.AddIndex(6);
	MeshEdit.AddIndex(6);
	MeshEdit.AddIndex(7);
	MeshEdit.AddIndex(7);
	MeshEdit.AddIndex(4);

	MeshEdit.AddIndex(0);
	MeshEdit.AddIndex(4);
	MeshEdit.AddIndex(1);
	MeshEdit.AddIndex(5);
	MeshEdit.AddIndex(2);
	MeshEdit.AddIndex(6);
	MeshEdit.AddIndex(3);
	MeshEdit.AddIndex(7);

	MeshEdit.EndSection();

	return BoxMesh;
}

lcMesh::lcMesh(int NumSections, int NumIndices, int NumVertices, lcVertexBuffer* VertexBuffer)
{
	m_Sections = new lcMeshSection[NumSections];
	m_SectionCount = NumSections;

	if (VertexBuffer)
	{
		m_VertexBuffer = VertexBuffer;
		m_DeleteVertexBuffer = false;
	}
	else
	{
		m_VertexBuffer = new lcVertexBuffer(NumVertices * 3 * sizeof(float));
		m_DeleteVertexBuffer = true;
	}

	m_VertexCount = NumVertices;

	if (NumVertices > 0xffff)
	{
		m_IndexBuffer = new lcIndexBuffer(NumIndices * 4);
		m_IndexType = GL_UNSIGNED_INT;
	}
	else
	{
		m_IndexBuffer = new lcIndexBuffer(NumIndices * 2);
		m_IndexType = GL_UNSIGNED_SHORT;
	}
}

lcMesh::~lcMesh()
{
	Clear();
}

void lcMesh::Clear()
{
	delete[] m_Sections;
	m_Sections = NULL;
	m_SectionCount = 0;

	if (m_DeleteVertexBuffer)
		delete m_VertexBuffer;
	m_VertexBuffer = NULL;
	m_VertexCount = 0;

	delete m_IndexBuffer;
	m_IndexBuffer = NULL;
	m_IndexType = 0;
}

void lcMesh::Render(int Color, bool Selected, bool Focused)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	m_VertexBuffer->BindBuffer();
	m_IndexBuffer->BindBuffer();

	for (int i = 0; i < m_SectionCount; i++)
	{
		lcMeshSection* Section = &m_Sections[i];
		u32 CurColor = Section->ColorIndex;

		if (Section->PrimitiveType == GL_LINES)
		{
			if (Focused)
				CurColor = LC_COLOR_FOCUS;
			else if (Selected)
				CurColor = LC_COLOR_SELECTION;
			else
				CurColor = Section->ColorIndex;
		}

		if (CurColor == LC_COLOR_DEFAULT)
			CurColor = Color;

		if (LC_COLOR_TRANSLUCENT(CurColor))
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
			glDepthMask(GL_FALSE);
		}
		else
		{
			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);
		}

		if (CurColor != LC_COLOR_EDGE || Section->PrimitiveType != GL_LINES)
			lcSetColor(CurColor);
		else
			lcSetEdgeColor(Color);

#if LC_PROFILE
		if (Section->PrimitiveType == GL_QUADS)
			g_RenderStats.QuadCount += Section->IndexCount / 4;
		else if (Section->PrimitiveType == GL_TRIANGLES)
			g_RenderStats.TriCount += Section->IndexCount / 3;
		else if (Section->PrimitiveType == GL_LINES)
			g_RenderStats.LineCount += Section->IndexCount / 2;
#endif

		glDrawElements(Section->PrimitiveType, Section->IndexCount, m_IndexType, (char*)m_IndexBuffer->GetDrawElementsOffset() + Section->IndexOffset);
	}

	m_IndexBuffer->UnbindBuffer();
	m_VertexBuffer->UnbindBuffer();
}

/*
void lcMesh::AddToScene(lcScene* Scene, const Matrix44& ModelWorld, int Color, lcPieceObject* Owner)
{
	for (int i = 0; i < m_SectionCount; i++)
	{
		lcRenderSection RenderSection;
		lcMeshSection* Section = &m_Sections[i];

		RenderSection.Owner = Owner;
		RenderSection.ModelWorld = ModelWorld;
		RenderSection.Mesh = this;
		RenderSection.Section = Section;
		RenderSection.Color = Section->ColorIndex;

		if (RenderSection.Color == LC_COLOR_DEFAULT)
			RenderSection.Color = Color;

		if (RenderSection.Section->PrimitiveType == GL_LINES)
		{
			// FIXME: LC_DET_BRICKEDGES
//			if ((m_nDetail & LC_DET_BRICKEDGES) == 0)
//				continue;

			if (Owner->IsFocused())
				RenderSection.Color = LC_COLOR_FOCUS;
			else if (Owner->IsSelected())
				RenderSection.Color = LC_COLOR_SELECTION;
		}

		if (LC_COLOR_TRANSLUCENT(RenderSection.Color))
		{
			Vector3 Pos = Mul31(Section->Box.GetCenter(), ModelWorld);
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
*/

bool lcMesh::ClosestRayIntersect(const Vector3& Start, const Vector3& End, float* Dist) const
{
	bool Hit = false;
	*Dist = FLT_MAX;
	Vector3 Intersection;

	float* verts = (float*)m_VertexBuffer->MapBuffer(GL_READ_ONLY_ARB);
	void* indices = m_IndexBuffer->MapBuffer(GL_READ_ONLY_ARB);

	for (int s = 0; s < m_SectionCount; s++)
	{
		lcMeshSection* Section = &m_Sections[s];

		if (Section->PrimitiveType == GL_LINES)
			continue;

		if (Section->PrimitiveType == GL_QUADS)
		{
			if (m_IndexType == GL_UNSIGNED_INT)
			{
				u32* IndexPtr = (u32*)((char*)indices + Section->IndexOffset);
				for (u32 i = 0; i < Section->IndexCount; i += 4)
				{
					Vector3 v1(verts[IndexPtr[i+0]*3], verts[IndexPtr[i+0]*3+1], verts[IndexPtr[i+0]*3+2]);
					Vector3 v2(verts[IndexPtr[i+1]*3], verts[IndexPtr[i+1]*3+1], verts[IndexPtr[i+1]*3+2]);
					Vector3 v3(verts[IndexPtr[i+2]*3], verts[IndexPtr[i+2]*3+1], verts[IndexPtr[i+2]*3+2]);
					Vector3 v4(verts[IndexPtr[i+3]*3], verts[IndexPtr[i+3]*3+1], verts[IndexPtr[i+3]*3+2]);

					if (LineQuadMinIntersection(v1, v2, v3, v4, Start, End, Dist, &Intersection))
						Hit = true;
				}
			}
			else
			{
				u16* IndexPtr = (u16*)((char*)indices + Section->IndexOffset);
				for (u32 i = 0; i < Section->IndexCount; i += 4)
				{
					Vector3 v1(verts[IndexPtr[i+0]*3], verts[IndexPtr[i+0]*3+1], verts[IndexPtr[i+0]*3+2]);
					Vector3 v2(verts[IndexPtr[i+1]*3], verts[IndexPtr[i+1]*3+1], verts[IndexPtr[i+1]*3+2]);
					Vector3 v3(verts[IndexPtr[i+2]*3], verts[IndexPtr[i+2]*3+1], verts[IndexPtr[i+2]*3+2]);
					Vector3 v4(verts[IndexPtr[i+3]*3], verts[IndexPtr[i+3]*3+1], verts[IndexPtr[i+3]*3+2]);

					if (LineQuadMinIntersection(v1, v2, v3, v4, Start, End, Dist, &Intersection))
						Hit = true;
				}
			}
		}
		else if (Section->PrimitiveType == GL_TRIANGLES)
		{
			if (m_IndexType == GL_UNSIGNED_INT)
			{
				u32* IndexPtr = (u32*)((char*)indices + Section->IndexOffset);
				for (u32 i = 0; i < Section->IndexCount; i += 3)
				{
					Vector3 v1(verts[IndexPtr[i+0]*3], verts[IndexPtr[i+0]*3+1], verts[IndexPtr[i+0]*3+2]);
					Vector3 v2(verts[IndexPtr[i+1]*3], verts[IndexPtr[i+1]*3+1], verts[IndexPtr[i+1]*3+2]);
					Vector3 v3(verts[IndexPtr[i+2]*3], verts[IndexPtr[i+2]*3+1], verts[IndexPtr[i+2]*3+2]);

					if (LineTriangleMinIntersection(v1, v2, v3, Start, End, Dist, &Intersection))
						Hit = true;
				}
			}
			else
			{
				u16* IndexPtr = (u16*)((char*)indices + Section->IndexOffset);
				for (u32 i = 0; i < Section->IndexCount; i += 3)
				{
					Vector3 v1(verts[IndexPtr[i+0]*3], verts[IndexPtr[i+0]*3+1], verts[IndexPtr[i+0]*3+2]);
					Vector3 v2(verts[IndexPtr[i+1]*3], verts[IndexPtr[i+1]*3+1], verts[IndexPtr[i+1]*3+2]);
					Vector3 v3(verts[IndexPtr[i+2]*3], verts[IndexPtr[i+2]*3+1], verts[IndexPtr[i+2]*3+2]);

					if (LineTriangleMinIntersection(v1, v2, v3, Start, End, Dist, &Intersection))
						Hit = true;
				}
			}
		}
	}

	m_VertexBuffer->UnmapBuffer();
	m_IndexBuffer->UnmapBuffer();

	return Hit;
}

template<>
void lcMeshEditor<u16>::AddIndices16(void* Indices, int NumIndices)
{
	memcpy((u16*)m_IndexBuffer + m_CurIndex, Indices, NumIndices * 2);
	m_CurIndex += NumIndices;
}

template<>
void lcMeshEditor<u16>::AddIndices32(void* Indices, int NumIndices)
{
	u16* Dst = (u16*)m_IndexBuffer + m_CurIndex;
	u32* Src = (u32*)Indices;

	for (int i = 0; i < NumIndices; i++)
		Dst[i] = Src[i];

	m_CurIndex += NumIndices;
}

template<>
void lcMeshEditor<u32>::AddIndices16(void* Indices, int NumIndices)
{
	u32* Dst = (u32*)m_IndexBuffer + m_CurIndex;
	u16* Src = (u16*)Indices;

	for (int i = 0; i < NumIndices; i++)
		Dst[i] = Src[i];

	m_CurIndex += NumIndices;
}

template<>
void lcMeshEditor<u32>::AddIndices32(void* Indices, int NumIndices)
{
	memcpy((u32*)m_IndexBuffer + m_CurIndex, Indices, NumIndices * 4);
	m_CurIndex += NumIndices;
}
