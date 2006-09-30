#ifndef _LC_BUFFER_H_
#define _LC_BUFFER_H_

class lcVertexBuffer
{
public:
	lcVertexBuffer();
	~lcVertexBuffer();

	void Alloc(int NumVertices, int VertexSize);

protected:
	int m_VertexCount;
	int m_VertexSize;
	void* m_Data;
};

class lcIndexBuffer
{
public:
	lcIndexBuffer();
	~lcIndexBuffer();

	void Alloc(int NumIndices, int IndexSize);

protected:
	int m_IndexCount;
	int m_IndexSize;
	void* m_Data;
};

class lcMeshSection
{
public:
	lcMeshSection();
	~lcMeshSection();

protected:
	int m_ColorIndex;
	int m_PrimitiveType;
	lcIndexBuffer* m_IndexBuffer;
	lcVertexBuffer* m_VertexBuffer;
};

class lcTexturedMeshSection : public lcMeshSection
{
public:
	lcTexturedMeshSection();
	~lcTexturedMeshSection();

protected:
	Texture* m_Texture;
};

class lcMesh
{
public:
	lcMesh();
	~lcMesh();

protected:
	int m_SectionCount;
	lcMeshSection* m_Sections;
};

#endif
