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
	for (int LodIdx = 0; LodIdx < LC_NUM_MESH_LODS; LodIdx++)
	{
		mLods[LodIdx].Sections = nullptr;
		mLods[LodIdx].NumSections = 0;
	}

	mNumVertices = 0;
	mNumTexturedVertices = 0;
	mIndexType = 0;
	mVertexData = nullptr;
	mVertexDataSize = 0;
	mIndexData = nullptr;
	mIndexDataSize = 0;
	mVertexCacheOffset = -1;
	mIndexCacheOffset = -1;
}

lcMesh::~lcMesh()
{
	free(mVertexData);
	free(mIndexData);
	for (int LodIdx = 0; LodIdx < LC_NUM_MESH_LODS; LodIdx++)
		delete[] mLods[LodIdx].Sections;
}

void lcMesh::Create(lcuint16 NumSections[LC_NUM_MESH_LODS], int NumVertices, int NumTexturedVertices, int NumIndices)
{
	for (int LodIdx = 0; LodIdx < LC_NUM_MESH_LODS; LodIdx++)
	{
		if (NumSections[LodIdx])
			mLods[LodIdx].Sections = new lcMeshSection[NumSections[LodIdx]];
		mLods[LodIdx].NumSections = NumSections[LodIdx];
	}

	mNumVertices = NumVertices;
	mNumTexturedVertices = NumTexturedVertices;
	mVertexDataSize = NumVertices * sizeof(lcVertex) + NumTexturedVertices * sizeof(lcVertexTextured);
	mVertexData = malloc(mVertexDataSize);

	if (NumVertices < 0x10000 && NumTexturedVertices < 0x10000)
	{
		mIndexType = GL_UNSIGNED_SHORT;
		mIndexDataSize = NumIndices * sizeof(GLushort);
	}
	else
	{
		mIndexType = GL_UNSIGNED_INT;
		mIndexDataSize = NumIndices * sizeof(GLuint);
	}

	mIndexData = malloc(mIndexDataSize);
}

void lcMesh::CreateBox()
{
	lcuint16 NumSections[LC_NUM_MESH_LODS];
	memset(NumSections, 0, sizeof(NumSections));
	NumSections[LC_MESH_LOD_HIGH] = 2;

	Create(NumSections, 24, 0, 36 + 24);

	lcVector3 Min(-10.0f, -10.0f, -24.0f);
	lcVector3 Max(10.0f, 10.0f, 4.0f);
	mRadius = lcLength(Max - Min) / 2.0f;
	mBoundingBox.Min = Min;
	mBoundingBox.Max = Max;

	lcVertex* Verts = (lcVertex*)mVertexData;
	lcuint16* Indices = (lcuint16*)mIndexData;

	Verts[0].Position = lcVector3(Min[0], Min[1], Min[2]);
	Verts[0].Normal = lcPackNormal(lcVector3(0.0f, 0.0f, -1.0f));
	Verts[1].Position = lcVector3(Min[0], Max[1], Min[2]);
	Verts[1].Normal = lcPackNormal(lcVector3(0.0f, 0.0f, -1.0f));
	Verts[2].Position = lcVector3(Max[0], Max[1], Min[2]);
	Verts[2].Normal = lcPackNormal(lcVector3(0.0f, 0.0f, -1.0f));
	Verts[3].Position = lcVector3(Max[0], Min[1], Min[2]);
	Verts[3].Normal = lcPackNormal(lcVector3(0.0f, 0.0f, -1.0f));
	Verts[4].Position = lcVector3(Min[0], Min[1], Max[2]);
	Verts[4].Normal = lcPackNormal(lcVector3(0.0f, 0.0f, 1.0f));
	Verts[5].Position = lcVector3(Min[0], Max[1], Max[2]);
	Verts[5].Normal = lcPackNormal(lcVector3(0.0f, 0.0f, 1.0f));
	Verts[6].Position = lcVector3(Max[0], Max[1], Max[2]);
	Verts[6].Normal = lcPackNormal(lcVector3(0.0f, 0.0f, 1.0f));
	Verts[7].Position = lcVector3(Max[0], Min[1], Max[2]);
	Verts[7].Normal = lcPackNormal(lcVector3(0.0f, 0.0f, 1.0f));

	Verts[8].Position = lcVector3(Min[0], Min[1], Min[2]);
	Verts[8].Normal = lcPackNormal(lcVector3(-1.0f, 0.0f, 0.0f));
	Verts[9].Position = lcVector3(Min[0], Min[1], Max[2]);
	Verts[9].Normal = lcPackNormal(lcVector3(-1.0f, 0.0f, 0.0f));
	Verts[10].Position = lcVector3(Min[0], Max[1], Max[2]);
	Verts[10].Normal = lcPackNormal(lcVector3(-1.0f, 0.0f, 0.0f));
	Verts[11].Position = lcVector3(Min[0], Max[1], Min[2]);
	Verts[11].Normal = lcPackNormal(lcVector3(-1.0f, 0.0f, 0.0f));
	Verts[12].Position = lcVector3(Max[0], Min[1], Min[2]);
	Verts[12].Normal = lcPackNormal(lcVector3(1.0f, 0.0f, 0.0f));
	Verts[13].Position = lcVector3(Max[0], Min[1], Max[2]);
	Verts[13].Normal = lcPackNormal(lcVector3(1.0f, 0.0f, 0.0f));
	Verts[14].Position = lcVector3(Max[0], Max[1], Max[2]);
	Verts[14].Normal = lcPackNormal(lcVector3(1.0f, 0.0f, 0.0f));
	Verts[15].Position = lcVector3(Max[0], Max[1], Min[2]);
	Verts[15].Normal = lcPackNormal(lcVector3(1.0f, 0.0f, 0.0f));

	Verts[16].Position = lcVector3(Min[0], Min[1], Min[2]);
	Verts[16].Normal = lcPackNormal(lcVector3(0.0f, -1.0f, 0.0f));
	Verts[17].Position = lcVector3(Min[0], Min[1], Max[2]);
	Verts[17].Normal = lcPackNormal(lcVector3(0.0f, -1.0f, 0.0f));
	Verts[18].Position = lcVector3(Max[0], Min[1], Max[2]);
	Verts[18].Normal = lcPackNormal(lcVector3(0.0f, -1.0f, 0.0f));
	Verts[19].Position = lcVector3(Max[0], Min[1], Min[2]);
	Verts[19].Normal = lcPackNormal(lcVector3(0.0f, -1.0f, 0.0f));
	Verts[20].Position = lcVector3(Min[0], Max[1], Min[2]);
	Verts[20].Normal = lcPackNormal(lcVector3(0.0f, 1.0f, 0.0f));
	Verts[21].Position = lcVector3(Min[0], Max[1], Max[2]);
	Verts[21].Normal = lcPackNormal(lcVector3(0.0f, 1.0f, 0.0f));
	Verts[22].Position = lcVector3(Max[0], Max[1], Max[2]);
	Verts[22].Normal = lcPackNormal(lcVector3(0.0f, 1.0f, 0.0f));
	Verts[23].Position = lcVector3(Max[0], Max[1], Min[2]);
	Verts[23].Normal = lcPackNormal(lcVector3(0.0f, 1.0f, 0.0f));

	lcMeshSection* Section = &mLods[LC_MESH_LOD_HIGH].Sections[0];
	Section->ColorIndex = gDefaultColor;
	Section->IndexOffset = 0;
	Section->NumIndices = 36;
	Section->PrimitiveType = LC_MESH_TRIANGLES;
	Section->Texture = nullptr;

	*Indices++ = 0; *Indices++ = 1; *Indices++ = 2;
	*Indices++ = 0; *Indices++ = 2; *Indices++ = 3;

	*Indices++ = 7; *Indices++ = 6; *Indices++ = 5;
	*Indices++ = 7; *Indices++ = 5; *Indices++ = 4;

	*Indices++ = 8; *Indices++ = 9; *Indices++ = 10;
	*Indices++ = 8; *Indices++ = 10; *Indices++ = 11;

	*Indices++ = 15; *Indices++ = 14; *Indices++ = 13;
	*Indices++ = 15; *Indices++ = 13; *Indices++ = 12;

	*Indices++ = 16; *Indices++ = 17; *Indices++ = 18;
	*Indices++ = 16; *Indices++ = 18; *Indices++ = 19;

	*Indices++ = 23; *Indices++ = 22; *Indices++ = 21;
	*Indices++ = 23; *Indices++ = 21; *Indices++ = 20;


	Section = &mLods[LC_MESH_LOD_HIGH].Sections[1];
	Section->ColorIndex = gEdgeColor;
	Section->IndexOffset = 36 * 2;
	Section->NumIndices = 24;
	Section->PrimitiveType = LC_MESH_LINES;
	Section->Texture = nullptr;

	*Indices++ = 0; *Indices++ = 1; *Indices++ = 1; *Indices++ = 2;
	*Indices++ = 2; *Indices++ = 3; *Indices++ = 3; *Indices++ = 0;

	*Indices++ = 4; *Indices++ = 5; *Indices++ = 5; *Indices++ = 6;
	*Indices++ = 6; *Indices++ = 7; *Indices++ = 7; *Indices++ = 4;

	*Indices++ = 0; *Indices++ = 4; *Indices++ = 1; *Indices++ = 5;
	*Indices++ = 2; *Indices++ = 6; *Indices++ = 3; *Indices++ = 7;
}

template<typename IndexType>
bool lcMesh::MinIntersectDist(const lcVector3& Start, const lcVector3& End, float& MinDistance)
{
	float Distance;
	if (!lcBoundingBoxRayIntersectDistance(mBoundingBox.Min, mBoundingBox.Max, Start, End, &Distance, nullptr) || (Distance >= MinDistance))
		return false;

	lcVertex* Verts = (lcVertex*)mVertexData;
	bool Hit = false;
	lcVector3 Intersection;

	for (int SectionIdx = 0; SectionIdx < mLods[LC_MESH_LOD_HIGH].NumSections; SectionIdx++)
	{
		lcMeshSection* Section = &mLods[LC_MESH_LOD_HIGH].Sections[SectionIdx];

		if (Section->PrimitiveType != LC_MESH_TRIANGLES && Section->PrimitiveType != LC_MESH_TEXTURED_TRIANGLES)
			continue;

		IndexType* Indices = (IndexType*)mIndexData + Section->IndexOffset / sizeof(IndexType);

		for (int Idx = 0; Idx < Section->NumIndices; Idx += 3)
		{
			const lcVector3& v1 = Verts[Indices[Idx]].Position;
			const lcVector3& v2 = Verts[Indices[Idx + 1]].Position;
			const lcVector3& v3 = Verts[Indices[Idx + 2]].Position;

			if (lcLineTriangleMinIntersection(v1, v2, v3, Start, End, &MinDistance, &Intersection))
				Hit = true;
		}
	}

	return Hit;
}

bool lcMesh::MinIntersectDist(const lcVector3& Start, const lcVector3& End, float& MinDist)
{
	if (mIndexType == GL_UNSIGNED_SHORT)
		return MinIntersectDist<GLushort>(Start, End, MinDist);
	else
		return MinIntersectDist<GLuint>(Start, End, MinDist);
}

template<typename IndexType>
bool lcMesh::IntersectsPlanes(const lcVector4 Planes[6])
{
	lcVertex* Verts = (lcVertex*)mVertexData;

	for (int SectionIdx = 0; SectionIdx < mLods[LC_MESH_LOD_HIGH].NumSections; SectionIdx++)
	{
		lcMeshSection* Section = &mLods[LC_MESH_LOD_HIGH].Sections[SectionIdx];

		if (Section->PrimitiveType != LC_MESH_TRIANGLES && Section->PrimitiveType != LC_MESH_TEXTURED_TRIANGLES)
			continue;

		IndexType* Indices = (IndexType*)mIndexData + Section->IndexOffset / sizeof(IndexType);

		for (int Idx = 0; Idx < Section->NumIndices; Idx += 3)
			if (lcTriangleIntersectsPlanes(Verts[Indices[Idx]].Position, Verts[Indices[Idx+1]].Position, Verts[Indices[Idx+2]].Position, Planes))
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
void lcMesh::ExportPOVRay(lcFile& File, const char* MeshName, const char** ColorTable)
{
	char Line[1024];

	int NumSections = 0;

	for (int SectionIdx = 0; SectionIdx < mLods[LC_MESH_LOD_HIGH].NumSections; SectionIdx++)
	{
		lcMeshSection* Section = &mLods[LC_MESH_LOD_HIGH].Sections[SectionIdx];

		if (Section->PrimitiveType == LC_MESH_TRIANGLES || Section->PrimitiveType == LC_MESH_TEXTURED_TRIANGLES)
			NumSections++;
	}

	if (NumSections > 1)
		sprintf(Line, "#declare lc_%s = union {\n", MeshName);
	else
		sprintf(Line, "#declare lc_%s = mesh {\n", MeshName);
	File.WriteLine(Line);

	lcVertex* Verts = (lcVertex*)mVertexData;

	for (int SectionIdx = 0; SectionIdx < mLods[LC_MESH_LOD_HIGH].NumSections; SectionIdx++)
	{
		lcMeshSection* Section = &mLods[LC_MESH_LOD_HIGH].Sections[SectionIdx];

		if (Section->PrimitiveType != LC_MESH_TRIANGLES && Section->PrimitiveType != LC_MESH_TEXTURED_TRIANGLES)
			continue;

		IndexType* Indices = (IndexType*)mIndexData + Section->IndexOffset / sizeof(IndexType);

		if (NumSections > 1)
			File.WriteLine(" mesh {\n");

		for (int Idx = 0; Idx < Section->NumIndices; Idx += 3)
		{
			const lcVector3 v1 = Verts[Indices[Idx]].Position / 25.0f;
			const lcVector3 v2 = Verts[Indices[Idx + 1]].Position / 25.0f;
			const lcVector3 v3 = Verts[Indices[Idx + 2]].Position / 25.0f;
			const lcVector3 n1 = lcUnpackNormal(Verts[Indices[Idx]].Normal);
			const lcVector3 n2 = lcUnpackNormal(Verts[Indices[Idx + 1]].Normal);
			const lcVector3 n3 = lcUnpackNormal(Verts[Indices[Idx + 2]].Normal);

			sprintf(Line, "  smooth_triangle { <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f>, <%.2f, %.2f, %.2f> }\n",
			        -v1.y, -v1.x, v1.z, -n1.y, -n1.x, n1.z, -v2.y, -v2.x, v2.z, -n2.y, -n2.x, n2.z, -v3.y, -v3.x, v3.z, -n3.y, -n3.x, n3.z);
			File.WriteLine(Line);
		}

		if (Section->ColorIndex != gDefaultColor)
		{
			sprintf(Line, "material { texture { %s normal { bumps 0.1 scale 2 } } }", ColorTable[Section->ColorIndex]);
			File.WriteLine(Line);
		}

		if (NumSections > 1)
			File.WriteLine(" }\n");
	}

	File.WriteLine("}\n\n");
}

void lcMesh::ExportPOVRay(lcFile& File, const char* MeshName, const char** ColorTable)
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

	for (int SectionIdx = 0; SectionIdx < mLods[LC_MESH_LOD_HIGH].NumSections; SectionIdx++)
	{
		lcMeshSection* Section = &mLods[LC_MESH_LOD_HIGH].Sections[SectionIdx];

		if (Section->PrimitiveType != LC_MESH_TRIANGLES && Section->PrimitiveType != LC_MESH_TEXTURED_TRIANGLES)
			continue;

		IndexType* Indices = (IndexType*)mIndexData + Section->IndexOffset / sizeof(IndexType);

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
				sprintf(Line, "f %ld//%ld %ld//%ld %ld//%ld\n", idx1, idx1, idx2, idx2, idx3, idx3);
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

bool lcMesh::FileLoad(lcMemFile& File)
{
	if (File.ReadU32() != LC_MESH_FILE_ID || File.ReadU32() != LC_MESH_FILE_VERSION)
		return false;

	mBoundingBox.Min = File.ReadVector3();
	mBoundingBox.Max = File.ReadVector3();
	mRadius = File.ReadFloat();

	lcuint32 NumVertices, NumTexturedVertices, NumIndices;
	lcuint16 NumLods, NumSections[LC_NUM_MESH_LODS];

	if (!File.ReadU32(&NumVertices, 1) || !File.ReadU32(&NumTexturedVertices, 1) || !File.ReadU32(&NumIndices, 1))
		return false;

	if (!File.ReadU16(&NumLods, 1) || NumLods != LC_NUM_MESH_LODS || !File.ReadU16(NumSections, LC_NUM_MESH_LODS))
		return false;

	Create(NumSections, NumVertices, NumTexturedVertices, NumIndices);

	for (int LodIdx = 0; LodIdx < LC_NUM_MESH_LODS; LodIdx++)
	{
		for (int SectionIdx = 0; SectionIdx < mLods[LodIdx].NumSections; SectionIdx++)
		{
			lcMeshSection& Section = mLods[LodIdx].Sections[SectionIdx];

			lcuint32 ColorCode, IndexOffset;
			lcuint16 PrimtiveType, Length;

			if (!File.ReadU32(&ColorCode, 1) || !File.ReadU32(&IndexOffset, 1) || !File.ReadU32(&NumIndices, 1) || !File.ReadU16(&PrimtiveType, 1))
				return false;

			Section.ColorIndex = lcGetColorIndex(ColorCode);
			Section.IndexOffset = IndexOffset;
			Section.NumIndices = NumIndices;
			Section.PrimitiveType = (lcMeshPrimitiveType)PrimtiveType;

			if (!File.ReadU16(&Length, 1))
				return false;

			if (Length)
			{
				if (Length >= LC_TEXTURE_NAME_LEN)
					return false;

				char FileName[LC_TEXTURE_NAME_LEN];

				File.ReadBuffer(FileName, Length);
				FileName[Length] = 0;

				Section.Texture = lcGetPiecesLibrary()->FindTexture(FileName, nullptr, false);
			}
			else
				Section.Texture = nullptr;
		}
	}

	File.ReadBuffer(mVertexData, mNumVertices * sizeof(lcVertex) + mNumTexturedVertices * sizeof(lcVertexTextured));
	if (mIndexType == GL_UNSIGNED_SHORT)
		File.ReadU16((lcuint16*)mIndexData, mIndexDataSize / 2);
	else
		File.ReadU32((lcuint32*)mIndexData, mIndexDataSize / 4);

	return true;
}

bool lcMesh::FileSave(lcMemFile& File)
{
	File.WriteU32(LC_MESH_FILE_ID);
	File.WriteU32(LC_MESH_FILE_VERSION);

	File.WriteVector3(mBoundingBox.Min);
	File.WriteVector3(mBoundingBox.Max);
	File.WriteFloat(mRadius);

	File.WriteU32(mNumVertices);
	File.WriteU32(mNumTexturedVertices);
	File.WriteU32(mIndexDataSize / (mIndexType == GL_UNSIGNED_SHORT ? 2 : 4));

	File.WriteU16(LC_NUM_MESH_LODS);
	for (int LodIdx = 0; LodIdx < LC_NUM_MESH_LODS; LodIdx++)
		File.WriteU16(mLods[LodIdx].NumSections);

	for (int LodIdx = 0; LodIdx < LC_NUM_MESH_LODS; LodIdx++)
	{
		for (int SectionIdx = 0; SectionIdx < mLods[LodIdx].NumSections; SectionIdx++)
		{
			lcMeshSection& Section = mLods[LodIdx].Sections[SectionIdx];

			File.WriteU32(lcGetColorCode(Section.ColorIndex));
			File.WriteU32(Section.IndexOffset);
			File.WriteU32(Section.NumIndices);
			File.WriteU16(Section.PrimitiveType);

			if (Section.Texture)
			{
				lcuint16 Length = (lcuint16)strlen(Section.Texture->mName);
				File.WriteU16(Length);
				File.WriteBuffer(Section.Texture->mName, Length);
			}
			else
				File.WriteU16(0);
		}
	}

	File.WriteBuffer(mVertexData, mNumVertices * sizeof(lcVertex) + mNumTexturedVertices * sizeof(lcVertexTextured));
	if (mIndexType == GL_UNSIGNED_SHORT)
		File.WriteU16((lcuint16*)mIndexData, mIndexDataSize / 2);
	else
		File.WriteU32((lcuint32*)mIndexData, mIndexDataSize / 4);

	return true;
}

int lcMesh::GetLodIndex(float Distance) const
{
	if (mLods[LC_MESH_LOD_LOW].NumSections && (Distance - mRadius) > 250.0f)
		return LC_MESH_LOD_LOW;
	else
		return LC_MESH_LOD_HIGH;
}
