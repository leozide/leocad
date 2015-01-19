#include "lc_global.h"
#include "lc_mesh.h"
#include "lc_colors.h"
#include "lc_texture.h"
#include "lc_file.h"
#include "lc_math.h"
#include "lc_application.h"
#include "lc_library.h"

lcMesh* gPlaceholderMesh;

lcMesh::lcMesh()
{
	mSections = NULL;
	mNumSections = 0;
	mNumVertices = 0;
	mNumTexturedVertices = 0;
	mIndexType = 0;
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

	if (NumVertices < 0x10000 && NumTexturedVertices < 0x10000)
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

	lcVector3 Min(-10.0f, -10.0f, -24.0f);
	lcVector3 Max(10.0f, 10.0f, 4.0f);

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
				-Verts[Indices[Idx+0]*3+1] / 25.0f, -Verts[Indices[Idx+0]*3] / 25.0f, Verts[Indices[Idx+0]*3+2] / 25.0f,
				-Verts[Indices[Idx+1]*3+1] / 25.0f, -Verts[Indices[Idx+1]*3] / 25.0f, Verts[Indices[Idx+1]*3+2] / 25.0f,
				-Verts[Indices[Idx+2]*3+1] / 25.0f, -Verts[Indices[Idx+2]*3] / 25.0f, Verts[Indices[Idx+2]*3+2] / 25.0f);
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

bool lcMesh::FileLoad(lcFile& File)
{
	if (File.ReadU32() != LC_FILE_ID || File.ReadU32() != LC_MESH_FILE_ID || File.ReadU32() != LC_MESH_FILE_VERSION)
		return false;

	lcuint32 NumVertices, NumTexturedVertices, NumIndices;
	lcuint16 NumSections;

	if (!File.ReadU16(&NumSections, 1) || !File.ReadU32(&NumVertices, 1) || !File.ReadU32(&NumTexturedVertices, 1) || !File.ReadU32(&NumIndices, 1))
		return false;

	Create(NumSections, NumVertices, NumTexturedVertices, NumIndices);

	for (int SectionIdx = 0; SectionIdx < mNumSections; SectionIdx++)
	{
		lcMeshSection& Section = mSections[SectionIdx];

		lcuint32 ColorCode, IndexOffset;
		lcuint16 Triangles, Length;

		if (!File.ReadU32(&ColorCode, 1) || !File.ReadU32(&IndexOffset, 1) || !File.ReadU32(&NumIndices, 1) || !File.ReadU16(&Triangles, 1))
			return false;

		Section.ColorIndex = lcGetColorIndex(ColorCode);
		Section.IndexOffset = IndexOffset;
		Section.NumIndices = NumIndices;
		Section.PrimitiveType = Triangles ? GL_TRIANGLES : GL_LINES;

		if (!File.ReadU16(&Length, 1))
			return false;

		if (Length)
		{
			if (Length >= LC_TEXTURE_NAME_LEN)
				return false;

			char FileName[LC_TEXTURE_NAME_LEN];

			File.ReadBuffer(FileName, Length);
			FileName[Length] = 0;

			Section.Texture = lcGetPiecesLibrary()->FindTexture(FileName);
		}
		else
			Section.Texture = NULL;
	}

	File.ReadFloats((float*)mVertexBuffer.mData, 3 * mNumVertices + 5 * mNumTexturedVertices);
	if (mIndexType == GL_UNSIGNED_SHORT)
		File.ReadU16((lcuint16*)mIndexBuffer.mData, mIndexBuffer.mSize / 2);
	else
		File.ReadU32((lcuint32*)mIndexBuffer.mData, mIndexBuffer.mSize / 4);

	UpdateBuffers();

	return true;
}

void lcMesh::FileSave(lcFile& File)
{
	File.WriteU32(LC_FILE_ID);
	File.WriteU32(LC_MESH_FILE_ID);
	File.WriteU32(LC_MESH_FILE_VERSION);

	File.WriteU16(mNumSections);
	File.WriteU32(mNumVertices);
	File.WriteU32(mNumTexturedVertices);
	File.WriteU32(mIndexBuffer.mSize / (mIndexType == GL_UNSIGNED_SHORT ? 2 : 4));

	for (int SectionIdx = 0; SectionIdx < mNumSections; SectionIdx++)
	{
		lcMeshSection& Section = mSections[SectionIdx];

		File.WriteU32(lcGetColorCode(Section.ColorIndex));
		File.WriteU32(Section.IndexOffset);
		File.WriteU32(Section.NumIndices);
		File.WriteU16(Section.PrimitiveType == GL_TRIANGLES ? 1 : 0);

		if (Section.Texture)
		{
			int Length = strlen(Section.Texture->mName);
			File.WriteU16(Length);
			File.WriteBuffer(Section.Texture->mName, Length);
		}
		else
			File.WriteU16(0);
	}

	File.WriteFloats((float*)mVertexBuffer.mData, 3 * mNumVertices + 5 * mNumTexturedVertices);
	if (mIndexType == GL_UNSIGNED_SHORT)
		File.WriteU16((lcuint16*)mIndexBuffer.mData, mIndexBuffer.mSize / 2);
	else
		File.WriteU32((lcuint32*)mIndexBuffer.mData, mIndexBuffer.mSize / 4);
}

int lcTranslucentRenderMeshCompare(const lcRenderMesh& a, const lcRenderMesh& b)
{
	if (a.Distance > b.Distance)
		return 1;
	else
		return -1;
}

int lcOpaqueRenderMeshCompare(const lcRenderMesh& a, const lcRenderMesh& b)
{
	if (a.Mesh > b.Mesh)
		return 1;
	else
		return -1;
}
