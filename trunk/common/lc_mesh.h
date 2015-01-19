#ifndef _LC_MESH_H_
#define _LC_MESH_H_

#include <stdlib.h>
#include "opengl.h"
#include "lc_math.h"

#define LC_MESH_FILE_ID      LC_FOURCC('M', 'E', 'S', 'H')
#define LC_MESH_FILE_VERSION 0x0100

struct lcVertex
{
	lcVector3 Position;
};

struct lcVertexTextured
{
	lcVector3 Position;
	lcVector2 TexCoord;
};

class lcVertexBuffer
{
public:
	lcVertexBuffer()
	{
		mData = NULL;
		mSize = 0;
		mBuffer = 0;
	}

	~lcVertexBuffer()
	{
		if (mBuffer)
		{
			glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
			glDeleteBuffers(1, &mBuffer);
		}

		free(mData);
	}

	void SetSize(int Size)
	{
		free(mData);
		mData = malloc(Size);
		mSize = Size;
	}

	void UpdateBuffer()
	{
		if (!GL_HasVertexBufferObject())
			return;

		if (!mBuffer)
			glGenBuffers(1, &mBuffer);

		glBindBuffer(GL_ARRAY_BUFFER_ARB, mBuffer);
		glBufferData(GL_ARRAY_BUFFER_ARB, mSize, mData, GL_STATIC_DRAW_ARB);
		glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
	}

	void* mData;
	int mSize;
	GLuint mBuffer;
};

class lcIndexBuffer
{
public:
	lcIndexBuffer()
	{
		mData = NULL;
		mSize = 0;
		mBuffer = 0;
	}

	~lcIndexBuffer()
	{
		if (mBuffer)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
			glDeleteBuffers(1, &mBuffer);
		}

		free(mData);
	}

	void SetSize(int Size)
	{
		free(mData);
		mData = malloc(Size);
		mSize = Size;
	}

	void UpdateBuffer()
	{
		if (!GL_HasVertexBufferObject())
			return;

		if (!mBuffer)
			glGenBuffers(1, &mBuffer);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, mBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER_ARB, mSize, mData, GL_STATIC_DRAW_ARB);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}

	void* mData;
	int mSize;
	GLuint mBuffer;
};

struct lcMeshSection
{
	int ColorIndex;
	int IndexOffset;
	int NumIndices;
	int PrimitiveType;
	lcTexture* Texture;
//	BoundingBox Box;
};

class lcMesh
{
public:
	lcMesh();
	~lcMesh();

	void Create(int NumSections, int NumVertices, int NumTexturedVertices, int NumIndices);
	void CreateBox();

	bool FileLoad(lcFile& File);
	void FileSave(lcFile& File);

	template<typename IndexType>
	void ExportPOVRay(lcFile& File, const char* MeshName, const char* ColorTable);
	void ExportPOVRay(lcFile& File, const char* MeshName, const char* ColorTable);

	template<typename IndexType>
	void ExportWavefrontIndices(lcFile& File, int DefaultColorIndex, int VertexOffset);
	void ExportWavefrontIndices(lcFile& File, int DefaultColorIndex, int VertexOffset);

	template<typename IndexType>
	bool MinIntersectDist(const lcVector3& Start, const lcVector3& End, float& MinDist, lcVector3& Intersection);
	bool MinIntersectDist(const lcVector3& Start, const lcVector3& End, float& MinDist, lcVector3& Intersection);

	template<typename IndexType>
	bool IntersectsPlanes(const lcVector4 Planes[6]);
	bool IntersectsPlanes(const lcVector4 Planes[6]);

	void UpdateBuffers()
	{
		mVertexBuffer.UpdateBuffer();
		mIndexBuffer.UpdateBuffer();
	}

	lcMeshSection* mSections;
	int mNumSections;

	lcVertexBuffer mVertexBuffer;
	lcIndexBuffer mIndexBuffer;
	int mNumVertices;
	int mNumTexturedVertices;
	int mIndexType;
};

struct lcRenderMesh
{
	lcMatrix44 WorldMatrix;
	lcMesh* Mesh;
	int ColorIndex;
	float Distance;
	bool Focused;
	bool Selected;
};

int lcTranslucentRenderMeshCompare(const lcRenderMesh& a, const lcRenderMesh& b);
int lcOpaqueRenderMeshCompare(const lcRenderMesh& a, const lcRenderMesh& b);

extern lcMesh* gPlaceholderMesh;

#endif // _LC_MESH_H_
