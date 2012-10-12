#include "lc_global.h"
#include "lc_mesh.h"
#include "lc_colors.h"
#include "lc_texture.h"
#include "lc_file.h"
#include "lc_math.h"

lcMesh::lcMesh()
{
	mSections = NULL;
	mNumSections = 0;
}

lcMesh::~lcMesh()
{
	delete[] mSections;
}

void lcMesh::Create(int NumSections, int NumVertices, int NumTexturedVertices, int NumIndices)
{
	mSections = new lcMeshSection[NumSections];
	mNumSections = NumSections;

	mNumVertices = NumVertices;
	mNumTexturedVertices = NumTexturedVertices;
	mVertexBuffer.SetSize(NumVertices * sizeof(lcVertex) + NumTexturedVertices * sizeof(lcVertexTextured));

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
	Create(2, 8, 0, 36 + 24);

	lcVector3 Min(-0.4f, -0.4f, -0.96f);
	lcVector3 Max(0.4f, 0.4f, 0.16f);

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
	Section->Texture = NULL;

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
	Section->Texture = NULL;

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
	char* BufferOffset;

	glEnableClientState(GL_VERTEX_ARRAY);

	if (GL_HasVertexBufferObject())
	{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, mVertexBuffer.mBuffer);
		BufferOffset = NULL;
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mIndexBuffer.mBuffer);
		ElementsOffset = NULL;
	}
	else
	{
		BufferOffset = (char*)mVertexBuffer.mData;
		ElementsOffset = (char*)mIndexBuffer.mData;
	}

	glVertexPointer(3, GL_FLOAT, 0, BufferOffset);

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

		if (mNumTexturedVertices)
		{
			if (Section->Texture) // TODO: cache states
			{
				glVertexPointer(3, GL_FLOAT, sizeof(lcVertexTextured), BufferOffset + (mNumVertices * sizeof(lcVertex)));
				glTexCoordPointer(2, GL_FLOAT, sizeof(lcVertexTextured), BufferOffset + ((mNumVertices + 1) * sizeof(lcVertex)));
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);

				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
				glBindTexture(GL_TEXTURE_2D, Section->Texture->mTexture);
				glEnable(GL_TEXTURE_2D);
			}
			else
			{
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				glVertexPointer(3, GL_FLOAT, 0, BufferOffset);
				glTexCoordPointer(2, GL_FLOAT, 0, NULL);

				glDisable(GL_TEXTURE_2D);
			}
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

	if (mNumTexturedVertices)
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, NULL);
		glDisable(GL_TEXTURE_2D);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
}

template<typename IndexType>
bool lcMesh::MinIntersectDist(const lcVector3& Start, const lcVector3& End, float& MinDist, lcVector3& Intersection)
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
			lcVector3 v1(p1[0], p1[1], p1[2]);
			lcVector3 v2(p2[0], p2[1], p2[2]);
			lcVector3 v3(p3[0], p3[1], p3[2]);

			if (lcLineTriangleMinIntersection(v1, v2, v3, Start, End, &MinDist, &Intersection))
				Hit = true;
		}
	}

	return Hit;
}

bool lcMesh::MinIntersectDist(const lcVector3& Start, const lcVector3& End, float& MinDist, lcVector3& Intersection)
{
	if (mIndexType == GL_UNSIGNED_SHORT)
		return MinIntersectDist<GLushort>(Start, End, MinDist, Intersection);
	else
		return MinIntersectDist<GLuint>(Start, End, MinDist, Intersection);
}

template<typename IndexType>
bool lcMesh::IntersectsPlanes(const lcVector4 Planes[6])
{
	float* Verts = (float*)mVertexBuffer.mData;

	for (int SectionIdx = 0; SectionIdx < mNumSections; SectionIdx++)
	{
		lcMeshSection* Section = &mSections[SectionIdx];

		if (Section->PrimitiveType != GL_TRIANGLES)
			continue;

		IndexType* Indices = (IndexType*)mIndexBuffer.mData + Section->IndexOffset / sizeof(IndexType);

		for (int Idx = 0; Idx < Section->NumIndices; Idx += 3)
			if (lcTriangleIntersectsPlanes(&Verts[Indices[Idx]*3], &Verts[Indices[Idx+1]*3], &Verts[Indices[Idx+2]*3], Planes))
				return true;
	}

	return false;
}

bool lcMesh::IntersectsPlanes(const lcVector4 Planes[6])
{
	if (mIndexType == GL_UNSIGNED_SHORT)
		return IntersectsPlanes<GLushort>(Planes);
	else
		return IntersectsPlanes<GLuint>(Planes);
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
