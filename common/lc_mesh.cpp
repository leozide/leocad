#include <malloc.h>
#include <stdlib.h>
#include "opengl.h"
#include "globals.h"
#include "lc_mesh.h"
#include "system.h"
#include "config.h"

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

	if (NumIndices > 0xffff)
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
		int CurColor = Section->ColorIndex;

		if (Section->PrimitiveType == GL_LINES)
		{
			if (Focused)
				CurColor = LC_COL_FOCUSED;
			else if (Selected)
				CurColor = LC_COL_SELECTED;
			else
				CurColor = Section->ColorIndex;
		}

		if (CurColor == LC_COL_DEFAULT)
			CurColor = Color;

		if (CurColor > 13 && CurColor < 22)
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
			glDepthMask(GL_FALSE);
			glColor4ubv(ColorArray[CurColor]);
		}
		else
		{
			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);
			glColor3ubv(FlatColorArray[CurColor]);
		}

		glDrawElements(Section->PrimitiveType, Section->IndexCount, m_IndexType, (char*)m_IndexBuffer->GetDrawElementsOffset() + Section->IndexOffset);
	}

	m_IndexBuffer->UnbindBuffer();
	m_VertexBuffer->UnbindBuffer();
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
