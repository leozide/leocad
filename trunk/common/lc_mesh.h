#ifndef _LC_BUFFER_H_
#define _LC_BUFFER_H_

#include "opengl.h"

class lcVertexBuffer
{
public:
	lcVertexBuffer(int DataSize)
	{
		if (GL_HasVertexBufferObject())
		{
			glGenBuffersARB(1, &m_Buffer.ID);
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_Buffer.ID);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB, DataSize, NULL, GL_STATIC_DRAW_ARB);
		}
		else
		{
			m_Buffer.Data = malloc(DataSize);
		}
	}

	~lcVertexBuffer()
	{
		if (GL_HasVertexBufferObject())
		{
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
			glDeleteBuffersARB(1, &m_Buffer.ID);
		}
		else
			free(m_Buffer.Data);
	}

	void* MapBuffer(GLenum Access)
	{
		if (GL_HasVertexBufferObject())
		{
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_Buffer.ID);
			return glMapBufferARB(GL_ARRAY_BUFFER_ARB, Access);
		}
		else
			return m_Buffer.Data;
	}

	void UnmapBuffer()
	{
		if (GL_HasVertexBufferObject())
		{
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_Buffer.ID);
			glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		}
	}

	void BindBuffer()
	{
		if (GL_HasVertexBufferObject())
		{
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_Buffer.ID);
			glVertexPointer(3, GL_FLOAT, 0, NULL);
		}
		else
			glVertexPointer(3, GL_FLOAT, 0, m_Buffer.Data);
	}

	void UnbindBuffer()
	{
		if (GL_HasVertexBufferObject())
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		glVertexPointer(3, GL_FLOAT, 0, NULL);
	}

protected:
	union
	{
		void* Data;
		GLuint ID;
	} m_Buffer;
};

class lcIndexBuffer
{
public:
	lcIndexBuffer(int DataSize)
	{
		if (GL_HasVertexBufferObject())
		{
			glGenBuffersARB(1, &m_Buffer.ID);
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_Buffer.ID);
			glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, DataSize, NULL, GL_STATIC_DRAW_ARB);
		}
		else
		{
			m_Buffer.Data = malloc(DataSize);
		}
	}

	~lcIndexBuffer()
	{
		if (GL_HasVertexBufferObject())
		{
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
			glDeleteBuffersARB(1, &m_Buffer.ID);
		}
		else
			free(m_Buffer.Data);
	}

	void* MapBuffer(GLenum Access)
	{
		if (GL_HasVertexBufferObject())
		{
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_Buffer.ID);
			return glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, Access);
		}
		else
			return m_Buffer.Data;
	}

	void UnmapBuffer()
	{
		if (GL_HasVertexBufferObject())
		{
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_Buffer.ID);
			glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		}
	}

	void BindBuffer()
	{
		if (GL_HasVertexBufferObject())
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_Buffer.ID);
	}

	void UnbindBuffer()
	{
		if (GL_HasVertexBufferObject())
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}

	void* GetDrawElementsOffset()
	{
		if (GL_HasVertexBufferObject())
			return NULL;
		else
			return m_Buffer.Data;
	}

protected:
	union
	{
		void* Data;
		GLuint ID;
	} m_Buffer;
};

struct lcMeshSection
{
	int ColorIndex;
	int IndexOffset;
	int IndexCount;
	int PrimitiveType;
};

class lcMesh
{
public:
	lcMesh(int NumSections, int NumIndices, int NumVertices, lcVertexBuffer* VertexBuffer);
	~lcMesh();

	void Clear();
	void Render(int Color, bool Selected = false, bool Focused = false);

public:
	lcMeshSection* m_Sections;
	int m_SectionCount;

	lcVertexBuffer* m_VertexBuffer;
	int m_VertexCount;
	bool m_DeleteVertexBuffer;

	lcIndexBuffer* m_IndexBuffer;
	int m_IndexType;
};

template<typename T>
class lcMeshEditor
{
public:
	lcMeshEditor(lcMesh* Mesh)
	{
		m_Mesh = Mesh;
		m_CurVertex = 0;
		m_CurIndex = 0;
		m_LastIndex = 0;
		m_UsedSections = 0;
		m_CurSection = -1;
		m_VertexBuffer = (float*)Mesh->m_VertexBuffer->MapBuffer(GL_WRITE_ONLY_ARB);
		m_IndexBuffer = (T*)Mesh->m_IndexBuffer->MapBuffer(GL_WRITE_ONLY_ARB);
	}

	~lcMeshEditor()
	{
		m_Mesh->m_VertexBuffer->UnmapBuffer();
		m_Mesh->m_VertexBuffer->UnbindBuffer();
		m_Mesh->m_IndexBuffer->UnmapBuffer();
		m_Mesh->m_IndexBuffer->UnbindBuffer();
	};

	void AddIndex(int Index)
	{ m_IndexBuffer[m_CurIndex++] = Index; }

	void AddVertex(float* Vert)
	{
		AddVertices(Vert, 1);
	}

	void AddVertices(float* Vert, int Count)
	{
		memcpy(m_VertexBuffer + 3 * m_CurVertex, Vert, 3 * sizeof(float) * Count);
		m_CurVertex += Count;
	}

	void AddIndices16(void* Indices, int NumIndices);
	void AddIndices32(void* Indices, int NumIndices);

	lcMeshSection* StartSection(int Primitive, int Color)
	{
		m_CurIndex = m_LastIndex;
		m_CurSection = m_UsedSections++;
		lcMeshSection* Section = &m_Mesh->m_Sections[m_CurSection];

		Section->ColorIndex = Color;
		Section->PrimitiveType = Primitive;
		Section->IndexOffset = m_CurIndex * sizeof(T);

		return Section;
	}

	void SetCurrentSection(lcMeshSection* Section)
	{
		m_CurSection = Section - m_Mesh->m_Sections;
		m_CurIndex = Section->IndexOffset / sizeof(T) + Section->IndexCount;
	}

	void EndSection(int ReserveIndices = 0)
	{
		lcMeshSection* Section = &m_Mesh->m_Sections[m_CurSection];
		Section->IndexCount = m_CurIndex - Section->IndexOffset / sizeof(T);
		m_CurIndex += ReserveIndices;
		if (m_LastIndex < m_CurIndex)
			m_LastIndex = m_CurIndex;
		m_CurSection = -1;
	}

	void OffsetIndices(int FirstIndex, int NumIndices, int Offset)
	{
		for (int i = 0; i < NumIndices; i++)
			m_IndexBuffer[FirstIndex+i] += Offset;
	}

public:
	lcMesh* m_Mesh;
	int m_CurVertex;
	int m_CurIndex;
	int m_LastIndex;
	int m_CurSection;
	int m_UsedSections;
	float* m_VertexBuffer;
	T* m_IndexBuffer;
};

#endif
