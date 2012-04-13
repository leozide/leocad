#ifndef _LC_MESH_H_
#define _LC_MESH_H_

#include "opengl.h"
#include "algebra.h"

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
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
			glDeleteBuffersARB(1, &mBuffer);
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
			glGenBuffersARB(1, &mBuffer);

		glBindBufferARB(GL_ARRAY_BUFFER_ARB, mBuffer);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, mSize, mData, GL_STATIC_DRAW_ARB);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
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
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
			glDeleteBuffersARB(1, &mBuffer);
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
			glGenBuffersARB(1, &mBuffer);

		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mBuffer);
		glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mSize, mData, GL_STATIC_DRAW_ARB);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
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
//	BoundingBox Box;
};

class lcMesh
{
public:
	lcMesh();
	~lcMesh();

	void Create(int NumSections, int NumVertices, int NumIndices);
	void Render(int ColorIdx, bool Selected, bool Focused);

	bool MinIntersectDist(const Vector3& Start, const Vector3& End, float& MinDist, Vector3& Intersection)
	{
		if (mIndexType == GL_UNSIGNED_SHORT)
			return MinIntersectDist<GLushort>(Start, End, MinDist, Intersection);
		else
			return MinIntersectDist<GLuint>(Start, End, MinDist, Intersection);
	}

	bool IntersectsPlanes(const Vector4* Planes, int NumPlanes)
	{
		if (mIndexType == GL_UNSIGNED_SHORT)
			return IntersectsPlanes<GLushort>(Planes, NumPlanes);
		else
			return IntersectsPlanes<GLuint>(Planes, NumPlanes);
	}

	template<typename IndexType>
	bool MinIntersectDist(const Vector3& Start, const Vector3& End, float& MinDist, Vector3& Intersection);
	template<typename IndexType>
	bool IntersectsPlanes(const Vector4* Planes, int NumPlanes);

	void UpdateBuffers()
	{
		mVertexBuffer.UpdateBuffer();
		mIndexBuffer.UpdateBuffer();
	}

	lcMeshSection* mSections;
	int mNumSections;

	lcVertexBuffer mVertexBuffer;
	lcIndexBuffer mIndexBuffer;
	int mIndexType;
//	int mNumVertices;
};

#endif // _LC_MESH_H_
