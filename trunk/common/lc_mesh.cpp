#include "lc_global.h"
#include "lc_mesh.h"
#include "lc_colors.h"
#include "lc_file.h"
#include "globals.h"

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

	mNumVertices = NumVertices;
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

void lcMesh::CreateBox()
{
	Create(2, 8, 36 + 24);

	Vector3 Min(-0.4f, -0.4f, -0.96f);
	Vector3 Max(0.4f, 0.4f, 0.16f);

	float* Verts = (float*)mVertexBuffer.mData;
	lcuint16* Indices = (lcuint16*)mIndexBuffer.mData;

	*Verts++ = Min[0]; *Verts++ = Min[1]; *Verts++ = Min[2];
	*Verts++ = Min[0]; *Verts++ = Max[1]; *Verts++ = Min[2];
	*Verts++ = Max[0]; *Verts++ = Max[1]; *Verts++ = Min[2];
	*Verts++ = Max[0]; *Verts++ = Min[1]; *Verts++ = Min[2];
	*Verts++ = Min[0]; *Verts++ = Min[1]; *Verts++ = Max[2];
	*Verts++ = Min[0]; *Verts++ = Max[1]; *Verts++ = Max[2];
	*Verts++ = Max[0]; *Verts++ = Max[1]; *Verts++ = Max[2];
	*Verts++ = Max[0]; *Verts++ = Min[1]; *Verts++ = Max[2];

	lcMeshSection* Section = &mSections[0];
	Section->ColorIndex = gDefaultColor;
	Section->IndexOffset = 0;
	Section->NumIndices = 36;
	Section->PrimitiveType = GL_TRIANGLES;

	*Indices++ = 0; *Indices++ = 1; *Indices++ = 2;
	*Indices++ = 0; *Indices++ = 2; *Indices++ = 3;

	*Indices++ = 7; *Indices++ = 6; *Indices++ = 5;
	*Indices++ = 7; *Indices++ = 5; *Indices++ = 4;

	*Indices++ = 0; *Indices++ = 1; *Indices++ = 5;
	*Indices++ = 0; *Indices++ = 5; *Indices++ = 4;

	*Indices++ = 2; *Indices++ = 3; *Indices++ = 7;
	*Indices++ = 2; *Indices++ = 7; *Indices++ = 6;

	*Indices++ = 0; *Indices++ = 3; *Indices++ = 7;
	*Indices++ = 0; *Indices++ = 7; *Indices++ = 4;

	*Indices++ = 1; *Indices++ = 2; *Indices++ = 6;
	*Indices++ = 1; *Indices++ = 6; *Indices++ = 5;

	Section = &mSections[1];
	Section->ColorIndex = gEdgeColor;
	Section->IndexOffset = 36 * 2;
	Section->NumIndices = 24;
	Section->PrimitiveType = GL_LINES;

	*Indices++ = 0; *Indices++ = 1; *Indices++ = 1; *Indices++ = 2;
	*Indices++ = 2; *Indices++ = 3; *Indices++ = 3; *Indices++ = 0;

	*Indices++ = 4; *Indices++ = 5; *Indices++ = 5; *Indices++ = 6;
	*Indices++ = 6; *Indices++ = 7; *Indices++ = 7; *Indices++ = 4;

	*Indices++ = 0; *Indices++ = 4; *Indices++ = 1; *Indices++ = 5;
	*Indices++ = 2; *Indices++ = 6; *Indices++ = 3; *Indices++ = 7;

	UpdateBuffers();
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

bool lcMesh::MinIntersectDist(const Vector3& Start, const Vector3& End, float& MinDist, Vector3& Intersection)
{
	if (mIndexType == GL_UNSIGNED_SHORT)
		return MinIntersectDist<GLushort>(Start, End, MinDist, Intersection);
	else
		return MinIntersectDist<GLuint>(Start, End, MinDist, Intersection);
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

bool lcMesh::IntersectsPlanes(const Vector4* Planes, int NumPlanes)
{
	if (mIndexType == GL_UNSIGNED_SHORT)
		return IntersectsPlanes<GLushort>(Planes, NumPlanes);
	else
		return IntersectsPlanes<GLuint>(Planes, NumPlanes);
}

template<typename IndexType>
void lcMesh::ExportPOVRay(lcFile& File, const char* MeshName, const char* ColorTable)
{
	char Line[1024];

	sprintf(Line, "#declare lc_%s = union {\n", MeshName);
	File.WriteLine(Line);

	float* Verts = (float*)mVertexBuffer.mData;

	for (int SectionIdx = 0; SectionIdx < mNumSections; SectionIdx++)
	{
		lcMeshSection* Section = &mSections[SectionIdx];

		if (Section->PrimitiveType != GL_TRIANGLES)
			continue;

		IndexType* Indices = (IndexType*)mIndexBuffer.mData + Section->IndexOffset / sizeof(IndexType);

		File.WriteLine(" mesh {\n");

		for (int Idx = 0; Idx < Section->NumIndices; Idx += 3)
		{
			sprintf(Line, "  triangle { <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f> }\n",
				-Verts[Indices[Idx+0]*3+1], -Verts[Indices[Idx+0]*3], Verts[Indices[Idx+0]*3+2],
				-Verts[Indices[Idx+1]*3+1], -Verts[Indices[Idx+1]*3], Verts[Indices[Idx+1]*3+2],
				-Verts[Indices[Idx+2]*3+1], -Verts[Indices[Idx+2]*3], Verts[Indices[Idx+2]*3+2]);
			File.WriteLine(Line);
		}

		if (Section->ColorIndex != gDefaultColor)
		{
			sprintf(Line, "material { texture { %s normal { bumps 0.1 scale 2 } } }", &ColorTable[Section->ColorIndex * LC_MAX_COLOR_NAME]);
			File.WriteLine(Line);
		}

		File.WriteLine(" }\n");
	}
}

void lcMesh::ExportPOVRay(lcFile& File, const char* MeshName, const char* ColorTable)
{
	if (mIndexType == GL_UNSIGNED_SHORT)
		ExportPOVRay<GLushort>(File, MeshName, ColorTable);
	else
		ExportPOVRay<GLuint>(File, MeshName, ColorTable);
}

template<typename IndexType>
void lcMesh::ExportWavefrontIndices(lcFile& File, int DefaultColorIndex, int VertexOffset)
{
	char Line[1024];

	for (int SectionIdx = 0; SectionIdx < mNumSections; SectionIdx++)
	{
		lcMeshSection* Section = &mSections[SectionIdx];

		if (Section->PrimitiveType != GL_TRIANGLES)
			continue;

		IndexType* Indices = (IndexType*)mIndexBuffer.mData + Section->IndexOffset / sizeof(IndexType);

		if (Section->ColorIndex == gDefaultColor)
			sprintf(Line, "usemtl %s\n", gColorList[DefaultColorIndex].SafeName);
		else
			sprintf(Line, "usemtl %s\n", gColorList[Section->ColorIndex].SafeName);
		File.WriteLine(Line);

		for (int Idx = 0; Idx < Section->NumIndices; Idx += 3)
		{
			long int idx1 = Indices[Idx + 0] + VertexOffset;
			long int idx2 = Indices[Idx + 1] + VertexOffset;
			long int idx3 = Indices[Idx + 2] + VertexOffset;

			if (idx1 != idx2 && idx1 != idx3 && idx2 != idx3)
				sprintf(Line, "f %ld %ld %ld\n", idx1, idx2, idx3);
			File.WriteLine(Line);
		}
	}

	File.WriteLine("\n");
}

void lcMesh::ExportWavefrontIndices(lcFile& File, int DefaultColorIndex, int VertexOffset)
{
	if (mIndexType == GL_UNSIGNED_SHORT)
		ExportWavefrontIndices<GLushort>(File, DefaultColorIndex, VertexOffset);
	else
		ExportWavefrontIndices<GLuint>(File, DefaultColorIndex, VertexOffset);
}
