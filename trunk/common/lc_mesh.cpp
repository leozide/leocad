#include "lc_global.h"
#include "lc_mesh.h"
#include "lc_colors.h"

struct lcVertex
{
	float x, y, z;
};

lcMesh::lcMesh()
{
	mSections = NULL;
	mNumSections = 0;
}

lcMesh::~lcMesh()
{
	delete[] mSections;
}

void lcMesh::Create(int NumSections, int NumVertices, int NumIndices)
{
	mSections = new lcMeshSection[NumSections];
	mNumSections = NumSections;

	mVertexBuffer.SetSize(NumVertices * sizeof(lcVertex));

	if (NumVertices < 0x10000)
	{
		mIndexType = GL_UNSIGNED_SHORT;
		mIndexBuffer.SetSize(NumIndices * sizeof(GLushort));
	}
	else
	{
		mIndexType = GL_UNSIGNED_INT;
		mIndexBuffer.SetSize(NumIndices * sizeof(GLuint));
	}
}

void lcMesh::Render(int DefaultColorIdx, bool Selected, bool Focused)
{
	char* ElementsOffset;

	glEnableClientState(GL_VERTEX_ARRAY);

	if (GL_HasVertexBufferObject())
	{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, mVertexBuffer.mBuffer);
		glVertexPointer(3, GL_FLOAT, 0, NULL);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mIndexBuffer.mBuffer);
		ElementsOffset = NULL;
	}
	else
	{
		glVertexPointer(3, GL_FLOAT, 0, mVertexBuffer.mData);
		ElementsOffset = (char*)mIndexBuffer.mData;
	}

	for (int SectionIdx = 0; SectionIdx < mNumSections; SectionIdx++)
	{
		lcMeshSection* Section = &mSections[SectionIdx];
		int ColorIdx = Section->ColorIndex;

		if (Section->PrimitiveType == GL_TRIANGLES)
		{
			if (ColorIdx == gDefaultColor)
				ColorIdx = DefaultColorIdx;

			if (lcIsColorTranslucent(ColorIdx))
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // TODO: cache states
				glEnable(GL_BLEND);
				glDepthMask(GL_FALSE);
			}
			else
			{
				glDepthMask(GL_TRUE);
				glDisable(GL_BLEND);
			}

			lcSetColor(ColorIdx);
		}
		else
		{
			glDepthMask(GL_TRUE); // TODO: cache states
			glDisable(GL_BLEND);

			if (Focused)
				lcSetColorFocused();
			else if (Selected)
				lcSetColorSelected();
			else if (ColorIdx == gEdgeColor)
				lcSetEdgeColor(DefaultColorIdx);
			else
				lcSetColor(ColorIdx);
		}

		glDrawElements(Section->PrimitiveType, Section->NumIndices, mIndexType, ElementsOffset + Section->IndexOffset);
	}

	glDepthMask(GL_TRUE); // TODO: cache states
	glDisable(GL_BLEND);

	if (GL_HasVertexBufferObject())
	{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}
	else
		glVertexPointer(3, GL_FLOAT, 0, NULL);

	glDisableClientState(GL_VERTEX_ARRAY);
}

template<typename IndexType>
bool lcMesh::MinIntersectDist(const Vector3& Start, const Vector3& End, float& MinDist, Vector3& Intersection)
{
	float* Verts = (float*)mVertexBuffer.mData;
	bool Hit = false;

	for (int SectionIdx = 0; SectionIdx < mNumSections; SectionIdx++)
	{
		lcMeshSection* Section = &mSections[SectionIdx];

		if (Section->PrimitiveType != GL_TRIANGLES)
			continue;

		IndexType* Indices = (IndexType*)mIndexBuffer.mData + Section->IndexOffset / sizeof(IndexType);

		for (int Idx = 0; Idx < Section->NumIndices; Idx += 3)
		{
			float* p1 = Verts + Indices[Idx + 0] * 3;
			float* p2 = Verts + Indices[Idx + 1] * 3;
			float* p3 = Verts + Indices[Idx + 2] * 3;
			Vector3 v1(p1[0], p1[1], p1[2]);
			Vector3 v2(p2[0], p2[1], p2[2]);
			Vector3 v3(p3[0], p3[1], p3[2]);

			if (LineTriangleMinIntersection(v1, v2, v3, Start, End, MinDist, Intersection))
				Hit = true;
		}
	}

	return Hit;
}

// Return true if a polygon intersects a set of planes.
static bool PolygonIntersectsPlanes(float* p1, float* p2, float* p3, float* p4, const Vector4* Planes, int NumPlanes) // TODO: move to lc_math
{
	float* Points[4] = { p1, p2, p3, p4 };
	int Outcodes[4] = { 0, 0, 0, 0 }, i;
	int NumPoints = (p4 != NULL) ? 4 : 3;

	// First do the Cohen-Sutherland out code test for trivial rejects/accepts.
	for (i = 0; i < NumPoints; i++)
	{
		Vector3 Pt(Points[i][0], Points[i][1], Points[i][2]);

		for (int j = 0; j < NumPlanes; j++)
		{
			if (Dot3(Pt, Planes[j]) + Planes[j][3] > 0)
				Outcodes[i] |= 1 << j;
		}
	}

	if (p4 != NULL)
	{
		// Polygon completely outside a plane.
		if ((Outcodes[0] & Outcodes[1] & Outcodes[2] & Outcodes[3]) != 0)
			return false;

		// If any vertex has an out code of all zeros then we intersect the volume.
		if (!Outcodes[0] || !Outcodes[1] || !Outcodes[2] || !Outcodes[3])
			return true;
	}
	else
	{
		// Polygon completely outside a plane.
		if ((Outcodes[0] & Outcodes[1] & Outcodes[2]) != 0)
			return false;

		// If any vertex has an out code of all zeros then we intersect the volume.
		if (!Outcodes[0] || !Outcodes[1] || !Outcodes[2])
			return true;
	}

	// Buffers for clipping the polygon.
	Vector3 ClipPoints[2][8];
	int NumClipPoints[2];
	int ClipBuffer = 0;

	NumClipPoints[0] = NumPoints;
	ClipPoints[0][0] = Vector3(p1[0], p1[1], p1[2]);
	ClipPoints[0][1] = Vector3(p2[0], p2[1], p2[2]);
	ClipPoints[0][2] = Vector3(p3[0], p3[1], p3[2]);

	if (NumPoints == 4)
		ClipPoints[0][3] = Vector3(p4[0], p4[1], p4[2]);

	// Now clip the polygon against the planes.
	for (i = 0; i < NumPlanes; i++)
	{
		PolygonPlaneClip(ClipPoints[ClipBuffer], NumClipPoints[ClipBuffer], ClipPoints[ClipBuffer^1], &NumClipPoints[ClipBuffer^1], Planes[i]);
		ClipBuffer ^= 1;

		if (!NumClipPoints[ClipBuffer])
			return false;
	}

	return true;
}

template<typename IndexType>
bool lcMesh::IntersectsPlanes(const Vector4* Planes, int NumPlanes)
{
	float* Verts = (float*)mVertexBuffer.mData;

	for (int SectionIdx = 0; SectionIdx < mNumSections; SectionIdx++)
	{
		lcMeshSection* Section = &mSections[SectionIdx];

		if (Section->PrimitiveType != GL_TRIANGLES)
			continue;

		IndexType* Indices = (IndexType*)mIndexBuffer.mData + Section->IndexOffset / sizeof(IndexType);

		for (int Idx = 0; Idx < Section->NumIndices; Idx += 3)
			if (PolygonIntersectsPlanes(&Verts[Indices[Idx]*3], &Verts[Indices[Idx+1]*3], &Verts[Indices[Idx+2]*3], NULL, Planes, NumPlanes))
				return true;
	}

	return false;
}
