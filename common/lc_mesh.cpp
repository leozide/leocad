#include <malloc.h>
#include <stdlib.h>
#include "lc_buffer.h"

lcVertexBuffer::lcVertexBuffer()
{
	m_VertexCount = 0;
	m_VertexSize = 0;
	m_Data = NULL;
}

lcVertexBuffer::~lcVertexBuffer()
{
	free(m_Data);
}

void lcVertexBuffer::Alloc(int NumVertices, int VertexSize)
{
	m_VertexCount = NumVertices;
	m_VertexSize = VertexSize;
	m_Data = realloc(m_Data, NumVertices * VertexSize);
}

lcIndexBuffer::lcIndexBuffer()
{
	m_IndexCount = 0;
	m_IndexSize = 0;
	m_Data = NULL;
}

lcIndexBuffer::~lcIndexBuffer()
{
	free(m_Data);
}

void lcIndexBuffer::Alloc(int NumIndices, int IndexSize)
{
	m_IndexCount = NumIndices;
	m_IndexSize = IndexSize;
	m_Data = realloc(m_Data, NumIndices * IndexSize);
}
