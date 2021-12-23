#include "lc_global.h"
#include "lc_meshloader.h"
#include "lc_file.h"
#include "lc_colors.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_texture.h"

static void lcCheckTexCoordsWrap(const lcVector4& Plane2, const lcVector3 (&Positions)[3], lcVector2 (&TexCoords)[3])
{
	lcVector2& TexCoords1 = TexCoords[0];
	lcVector2& TexCoords2 = TexCoords[1];
	lcVector2& TexCoords3 = TexCoords[2];

	const float u12 = fabsf(TexCoords1.x - TexCoords2.x);
	const float u13 = fabsf(TexCoords1.x - TexCoords3.x);
	const float u23 = fabsf(TexCoords2.x - TexCoords3.x);

	if (u12 < 0.5f && u13 < 0.5f && u23 < 0.5f)
		return;

	const float Dot1 = fabsf(lcDot(Plane2, lcVector4(Positions[0], 1.0f)));
	const float Dot2 = fabsf(lcDot(Plane2, lcVector4(Positions[1], 1.0f)));
	const float Dot3 = fabsf(lcDot(Plane2, lcVector4(Positions[2], 1.0f)));

	if (Dot1 > Dot2)
	{
		if (Dot1 > Dot3)
		{
			if (u12 > 0.5f)
				TexCoords2.x += TexCoords2.x < 0.5f ? 1.0f : -1.0f;

			if (u13 > 0.5f)
				TexCoords3.x += TexCoords3.x < 0.5f ? 1.0f : -1.0f;
		}
		else
		{
			if (u13 > 0.5f)
				TexCoords1.x += TexCoords1.x < 0.5f ? 1.0f : -1.0f;

			if (u23 > 0.5f)
				TexCoords2.x += TexCoords2.x < 0.5f ? 1.0f : -1.0f;
		}
	}
	else
	{
		if (Dot2 > Dot3)
		{
			if (u12 > 0.5f)
				TexCoords1.x += TexCoords1.x < 0.5f ? 1.0f : -1.0f;

			if (u23 > 0.5f)
				TexCoords3.x += TexCoords3.x < 0.5f ? 1.0f : -1.0f;
		}
		else
		{
			if (u13 > 0.5f)
				TexCoords1.x += TexCoords1.x < 0.5f ? 1.0f : -1.0f;

			if (u23 > 0.5f)
				TexCoords2.x += TexCoords2.x < 0.5f ? 1.0f : -1.0f;
		}
	}
}

static void lcCheckTexCoordsPole(const lcVector4& FrontPlane, const lcVector4& Plane2, const lcVector3(&Positions)[3], lcVector2(&TexCoords)[3])
{
	int PoleIndex;
	int EdgeIndex1, EdgeIndex2;

	if (fabsf(lcDot(lcVector4(Positions[0], 1.0f), FrontPlane)) < 0.01f && fabsf(lcDot(lcVector4(Positions[0], 1.0f), Plane2)) < 0.01f)
	{
		PoleIndex = 0;
		EdgeIndex1 = 1;
		EdgeIndex2 = 2;
	}
	else if (fabsf(lcDot(lcVector4(Positions[1], 1.0f), FrontPlane)) < 0.01f && fabsf(lcDot(lcVector4(Positions[1], 1.0f), Plane2)) < 0.01f)
	{
		PoleIndex = 1;
		EdgeIndex1 = 0;
		EdgeIndex2 = 2;
	}
	else if (fabsf(lcDot(lcVector4(Positions[2], 1.0f), FrontPlane)) < 0.01f && fabsf(lcDot(lcVector4(Positions[2], 1.0f), Plane2)) < 0.01f)
	{
		PoleIndex = 2;
		EdgeIndex1 = 0;
		EdgeIndex2 = 1;
	}
	else
		return;

	const lcVector3 OppositeEdge = Positions[EdgeIndex2] - Positions[EdgeIndex1];
	const lcVector3 SideEdge = Positions[PoleIndex] - Positions[EdgeIndex1];

	const float OppositeLength = lcLength(OppositeEdge);
	const float Projection = lcDot(OppositeEdge, SideEdge) / (OppositeLength * OppositeLength);

	TexCoords[PoleIndex].x = TexCoords[EdgeIndex1].x + (TexCoords[EdgeIndex2].x - TexCoords[EdgeIndex1].x) * Projection;
}

static void lcResequenceQuad(int* Indices, int a, int b, int c, int d)
{
	Indices[0] = a;
	Indices[1] = b;
	Indices[2] = c;
	Indices[3] = d;
}

static void lcTestQuad(int (&TriangleIndices)[2][3], const lcVector3 (&Vertices)[4])
{
	const lcVector3 v01 = Vertices[1] - Vertices[0];
	const lcVector3 v02 = Vertices[2] - Vertices[0];
	const lcVector3 v03 = Vertices[3] - Vertices[0];
	const lcVector3 cp1 = lcCross(v01, v02);
	const lcVector3 cp2 = lcCross(v02, v03);

	if (lcDot(cp1, cp2) > 0.0f)
		return;

	const lcVector3 v12 = Vertices[2] - Vertices[1];
	const lcVector3 v13 = Vertices[3] - Vertices[1];
	const lcVector3 v23 = Vertices[3] - Vertices[2];
	int QuadIndices[4] = { 0, 1, 2, 3 };

	if (lcDot(lcCross(v12, v01), lcCross(v01, v13)) > 0.0f)
	{
		if (-lcDot(lcCross(v02, v12), lcCross(v12, v23)) > 0.0f)
			lcResequenceQuad(QuadIndices, 1, 2, 3, 0);
		else
			lcResequenceQuad(QuadIndices, 0, 3, 1, 2);
	}
	else
	{
		if (-lcDot(lcCross(v02, v12), lcCross(v12, v23)) > 0.0f)
			lcResequenceQuad(QuadIndices, 0, 1, 3, 2);
		else
			lcResequenceQuad(QuadIndices, 1, 2, 3, 0);
	}

	TriangleIndices[0][0] = QuadIndices[0];
	TriangleIndices[0][1] = QuadIndices[1];
	TriangleIndices[0][2] = QuadIndices[2];

	TriangleIndices[1][0] = QuadIndices[2];
	TriangleIndices[1][1] = QuadIndices[3];
	TriangleIndices[1][2] = QuadIndices[0];
}

constexpr float lcDistanceEpsilon = 0.01f; // Maximum value for 50591.dat

static bool lcCompareVertices(const lcVector3& Position1, const lcVector3& Position2)
{
	return fabsf(Position1.x - Position2.x) < lcDistanceEpsilon && fabsf(Position1.y - Position2.y) < lcDistanceEpsilon && fabsf(Position1.z - Position2.z) < lcDistanceEpsilon;
}

lcMeshLoaderSection* lcMeshLoaderTypeData::AddSection(lcMeshPrimitiveType PrimitiveType, lcMeshLoaderMaterial* Material)
{
	for (const std::unique_ptr<lcMeshLoaderSection>& Section : mSections)
		if (Section->mMaterial == Material && Section->mPrimitiveType == PrimitiveType)
			return Section.get();

	mSections.emplace_back(new lcMeshLoaderSection(PrimitiveType, Material));

	return mSections.back().get();
}

quint32 lcMeshLoaderTypeData::AddVertex(const lcVector3& Position, bool Optimize)
{
	if (Optimize)
	{
		for (int VertexIdx = mVertices.GetSize() - 1; VertexIdx >= 0; VertexIdx--)
		{
			const lcMeshLoaderVertex& Vertex = mVertices[VertexIdx];

			if (lcCompareVertices(Position, Vertex.Position))
				return VertexIdx;
		}
	}

	lcMeshLoaderVertex& Vertex = mVertices.Add();

	Vertex.Position = Position;
	Vertex.Normal = lcVector3(0.0f, 0.0f, 0.0f);
	Vertex.NormalWeight = 0.0f;

	return mVertices.GetSize() - 1;
}

quint32 lcMeshLoaderTypeData::AddVertex(const lcVector3& Position, const lcVector3& Normal, float NormalWeight, bool Optimize)
{
	if (Optimize)
	{
		for (int VertexIdx = mVertices.GetSize() - 1; VertexIdx >= 0; VertexIdx--)
		{
			lcMeshLoaderVertex& Vertex = mVertices[VertexIdx];

			if (lcCompareVertices(Position, Vertex.Position))
			{
				if (Vertex.NormalWeight == 0.0f)
				{
					Vertex.Normal = Normal;
					Vertex.NormalWeight = NormalWeight;
					return VertexIdx;
				}
				else if (lcDot(Normal, Vertex.Normal) > 0.71f)
				{
					Vertex.Normal = lcNormalize(Vertex.Normal * Vertex.NormalWeight + Normal * NormalWeight);
					Vertex.NormalWeight += NormalWeight;
					return VertexIdx;
				}
			}
		}
	}

	lcMeshLoaderVertex& Vertex = mVertices.Add();

	Vertex.Position = Position;
	Vertex.Normal = Normal;
	Vertex.NormalWeight = 1.0f;

	return mVertices.GetSize() - 1;
}

quint32 lcMeshLoaderTypeData::AddConditionalVertex(const lcVector3(&Position)[4])
{
	lcMeshLoaderConditionalVertex& Vertex = mConditionalVertices.Add();

	Vertex.Position[0] = Position[0];
	Vertex.Position[1] = Position[1];
	Vertex.Position[2] = Position[2];
	Vertex.Position[3] = Position[3];

	return mConditionalVertices.GetSize() - 1;
}

void lcMeshLoaderTypeData::ProcessLine(int LineType, lcMeshLoaderMaterial* Material, bool WindingCCW, lcVector3 (&Vertices)[4], bool Optimize)
{
	constexpr lcMeshPrimitiveType PrimitiveTypes[4] = { LC_MESH_LINES, LC_MESH_TRIANGLES, LC_MESH_TRIANGLES, LC_MESH_CONDITIONAL_LINES };
	lcMeshPrimitiveType PrimitiveType = PrimitiveTypes[LineType - 2];

	if (Material->Type != lcMeshLoaderMaterialType::Solid && PrimitiveType == LC_MESH_TRIANGLES)
		PrimitiveType = LC_MESH_TEXTURED_TRIANGLES;

	lcMeshLoaderSection* Section = AddSection(PrimitiveType, Material);

	if (LineType == 3 || LineType == 4)
	{
		int TriangleIndices[2][3] = { { 0, 1, 2 }, { 2, 3, 0 } };

		if (LineType == 4)
			lcTestQuad(TriangleIndices, Vertices);

		lcVector3 Normal = lcNormalize(lcCross(Vertices[1] - Vertices[0], Vertices[2] - Vertices[0]));

		if (!WindingCCW)
			Normal = -Normal;

		for (int TriangleIndex = 0; TriangleIndex < LineType - 2; TriangleIndex++)
		{
			const lcVector3& Vertex1 = Vertices[TriangleIndices[TriangleIndex][0]];
			const lcVector3& Vertex2 = Vertices[TriangleIndices[TriangleIndex][1]];
			const lcVector3& Vertex3 = Vertices[TriangleIndices[TriangleIndex][2]];
			const lcVector3 Edge1 = lcNormalize(Vertex2 - Vertex1);
			const lcVector3 Edge2 = lcNormalize(Vertex3 - Vertex1);
			const lcVector3 Edge3 = lcNormalize(Vertex3 - Vertex2);
			const float Angle1 = acosf(lcDot(Edge1, Edge2));
			const float Angle2 = acosf(lcDot(-Edge1, Edge3));
			const float Angle3 = LC_PI - Angle1 - Angle2;

			int Indices[3];

			Indices[0] = AddVertex(Vertex1, Normal, Angle1, Optimize);
			Indices[1] = AddVertex(Vertex2, Normal, Angle2, Optimize);
			Indices[2] = AddVertex(Vertex3, Normal, Angle3, Optimize);

			if (Indices[0] != Indices[1] && Indices[0] != Indices[2] && Indices[1] != Indices[2])
			{
				if (WindingCCW)
				{
					Section->mIndices.Add(Indices[0]);
					Section->mIndices.Add(Indices[1]);
					Section->mIndices.Add(Indices[2]);
				}
				else
				{
					Section->mIndices.Add(Indices[2]);
					Section->mIndices.Add(Indices[1]);
					Section->mIndices.Add(Indices[0]);
				}
			}
		}
	}
	else if (LineType == 2)
	{
		int Indices[2];

		Indices[0] = AddVertex(Vertices[0], Optimize);
		Indices[1] = AddVertex(Vertices[1], Optimize);

		if (Indices[0] != Indices[1])
		{
			Section->mIndices.Add(Indices[0]);
			Section->mIndices.Add(Indices[1]);
		}
	}
	else if (LineType == 5)
	{
		int Indices[2];

		Indices[0] = AddConditionalVertex(Vertices);
		Section->mIndices.Add(Indices[0]);

		std::swap(Vertices[0], Vertices[1]);

		Indices[1] = AddConditionalVertex(Vertices);
		Section->mIndices.Add(Indices[1]);
	}
}

void lcMeshLoaderTypeData::AddMeshData(const lcMeshLoaderTypeData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcMeshLoaderTextureMap* TextureMap)
{
	const lcArray<lcMeshLoaderVertex>& DataVertices = Data.mVertices;
	lcArray<quint32> IndexRemap(DataVertices.GetSize());
	const lcMatrix33 NormalTransform = lcMatrix33Transpose(lcMatrix33(lcMatrix44Inverse(Transform)));

	mVertices.AllocGrow(DataVertices.GetSize());

	for (const lcMeshLoaderVertex& DataVertex : DataVertices)
	{
		const lcVector3 Position = lcMul31(DataVertex.Position, Transform);
		int Index;

		if (DataVertex.NormalWeight == 0.0f)
			Index = AddVertex(Position, true);
		else
		{
			lcVector3 Normal = lcNormalize(lcMul(DataVertex.Normal, NormalTransform));
			if (InvertNormals)
				Normal = -Normal;
			Index = AddVertex(Position, Normal, DataVertex.NormalWeight, true);
		}

		IndexRemap.Add(Index);
	}

	mConditionalVertices.AllocGrow(Data.mConditionalVertices.GetSize());
	lcArray<quint32> ConditionalRemap(Data.mConditionalVertices.GetSize());

	for (const lcMeshLoaderConditionalVertex& DataVertex : Data.mConditionalVertices)
	{
		lcVector3 Position[4];
		
		Position[0] = lcMul31(DataVertex.Position[0], Transform);
		Position[1] = lcMul31(DataVertex.Position[1], Transform);
		Position[2] = lcMul31(DataVertex.Position[2], Transform);
		Position[3] = lcMul31(DataVertex.Position[3], Transform);

		const int Index = AddConditionalVertex(Position);
		ConditionalRemap.Add(Index);
	}

	for (const std::unique_ptr<lcMeshLoaderSection>& SrcSection : Data.mSections)
	{
		const quint32 ColorCode = SrcSection->mMaterial->Color == 16 ? CurrentColorCode : SrcSection->mMaterial->Color;
		lcMeshPrimitiveType PrimitiveType = SrcSection->mPrimitiveType;
		lcMeshLoaderSection* DstSection;

		if (SrcSection->mMaterial->Type != lcMeshLoaderMaterialType::Solid)
		{
			lcMeshLoaderTextureMap DstTextureMap = *TextureMap;

			for (lcVector3& Point : DstTextureMap.Points)
				Point = lcMul31(Point, Transform);

			DstSection = AddSection(PrimitiveType, mMeshData->GetTexturedMaterial(ColorCode, DstTextureMap));
		}
		else if (TextureMap && SrcSection->mPrimitiveType == LC_MESH_TRIANGLES)
		{
			PrimitiveType = LC_MESH_TEXTURED_TRIANGLES;

			DstSection = AddSection(PrimitiveType, mMeshData->GetTexturedMaterial(ColorCode, *TextureMap));
		}
		else
		{
			DstSection = AddSection(PrimitiveType, mMeshData->GetMaterial(ColorCode));
		}

		DstSection->mIndices.AllocGrow(SrcSection->mIndices.GetSize());

		if (PrimitiveType == LC_MESH_CONDITIONAL_LINES)
		{
			for (const quint32 Index : SrcSection->mIndices)
				DstSection->mIndices.Add(ConditionalRemap[Index]);
		}
		else if (!InvertWinding || (PrimitiveType == LC_MESH_LINES))
		{
			for (const quint32 Index : SrcSection->mIndices)
				DstSection->mIndices.Add(IndexRemap[Index]);
		}
		else
		{
			for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx += 3)
			{
				DstSection->mIndices.Add(IndexRemap[SrcSection->mIndices[IndexIdx + 2]]);
				DstSection->mIndices.Add(IndexRemap[SrcSection->mIndices[IndexIdx + 1]]);
				DstSection->mIndices.Add(IndexRemap[SrcSection->mIndices[IndexIdx + 0]]);
			}
		}
	}
}

void lcMeshLoaderTypeData::AddMeshDataNoDuplicateCheck(const lcMeshLoaderTypeData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcMeshLoaderTextureMap* TextureMap)
{
	const lcArray<lcMeshLoaderVertex>& DataVertices = Data.mVertices;
	quint32 BaseIndex;
	const lcMatrix33 NormalTransform = lcMatrix33Transpose(lcMatrix33(lcMatrix44Inverse(Transform)));

	BaseIndex = mVertices.GetSize();

	mVertices.SetGrow(lcMin(mVertices.GetSize(), 8 * 1024 * 1024));
	mVertices.AllocGrow(DataVertices.GetSize());

	for (int SrcVertexIdx = 0; SrcVertexIdx < DataVertices.GetSize(); SrcVertexIdx++)
	{
		const lcMeshLoaderVertex& SrcVertex = DataVertices[SrcVertexIdx];
		lcMeshLoaderVertex& DstVertex = mVertices.Add();
		DstVertex.Position = lcMul31(SrcVertex.Position, Transform);
		DstVertex.Normal = lcNormalize(lcMul(SrcVertex.Normal, NormalTransform));
		if (InvertNormals)
			DstVertex.Normal = -DstVertex.Normal;
		DstVertex.NormalWeight = SrcVertex.NormalWeight;
	}

	mConditionalVertices.AllocGrow(Data.mConditionalVertices.GetSize());
	const quint32 BaseConditional = mConditionalVertices.GetSize();

	for (const lcMeshLoaderConditionalVertex& DataVertex : Data.mConditionalVertices)
	{
		lcMeshLoaderConditionalVertex& Vertex = mConditionalVertices.Add();

		Vertex.Position[0] = lcMul31(DataVertex.Position[0], Transform);
		Vertex.Position[1] = lcMul31(DataVertex.Position[1], Transform);
		Vertex.Position[2] = lcMul31(DataVertex.Position[2], Transform);
		Vertex.Position[3] = lcMul31(DataVertex.Position[3], Transform);
	}

	for (const std::unique_ptr<lcMeshLoaderSection>& SrcSection : Data.mSections)
	{
		const quint32 ColorCode = SrcSection->mMaterial->Color == 16 ? CurrentColorCode : SrcSection->mMaterial->Color;
		lcMeshPrimitiveType PrimitiveType = SrcSection->mPrimitiveType;
		lcMeshLoaderSection* DstSection;

		if (SrcSection->mMaterial->Type != lcMeshLoaderMaterialType::Solid)
		{
			lcMeshLoaderTextureMap DstTextureMap = *TextureMap;

			for (lcVector3& Point : DstTextureMap.Points)
				Point = lcMul31(Point, Transform);

			DstSection = AddSection(PrimitiveType, mMeshData->GetTexturedMaterial(ColorCode, DstTextureMap));
		}
		else if (TextureMap && SrcSection->mPrimitiveType == LC_MESH_TRIANGLES)
		{
			PrimitiveType = LC_MESH_TEXTURED_TRIANGLES;

			DstSection = AddSection(PrimitiveType, mMeshData->GetTexturedMaterial(ColorCode, *TextureMap));
		}
		else
		{
			DstSection = AddSection(PrimitiveType, mMeshData->GetMaterial(ColorCode));
		}

		DstSection->mIndices.SetGrow(lcMin(DstSection->mIndices.GetSize(), 8 * 1024 * 1024));
		DstSection->mIndices.AllocGrow(SrcSection->mIndices.GetSize());

		if (PrimitiveType == LC_MESH_CONDITIONAL_LINES)
		{
			for (const quint32 Index : SrcSection->mIndices)
				DstSection->mIndices.Add(BaseConditional + Index);
		}
		else if (!InvertWinding || (PrimitiveType == LC_MESH_LINES))
		{
			for (const quint32 Index : SrcSection->mIndices)
				DstSection->mIndices.Add(BaseIndex + Index);
		}
		else
		{
			for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx += 3)
			{
				DstSection->mIndices.Add(BaseIndex + SrcSection->mIndices[IndexIdx + 2]);
				DstSection->mIndices.Add(BaseIndex + SrcSection->mIndices[IndexIdx + 1]);
				DstSection->mIndices.Add(BaseIndex + SrcSection->mIndices[IndexIdx + 0]);
			}
		}
	}
}

void lcLibraryMeshData::AddVertices(lcMeshDataType MeshDataType, int VertexCount, int* BaseVertex, lcMeshLoaderVertex** VertexBuffer)
{
	lcArray<lcMeshLoaderVertex>& Vertices = mData[MeshDataType].mVertices;
	int CurrentSize = Vertices.GetSize();

	Vertices.SetSize(CurrentSize + VertexCount);

	*BaseVertex = CurrentSize;
	*VertexBuffer = &Vertices[CurrentSize];
}

void lcLibraryMeshData::AddIndices(lcMeshDataType MeshDataType, lcMeshPrimitiveType PrimitiveType, quint32 ColorCode, int IndexCount, quint32** IndexBuffer)
{
	lcMeshLoaderSection* Section = mData[MeshDataType].AddSection(PrimitiveType, GetMaterial(ColorCode));
	lcArray<quint32>& Indices = Section->mIndices;
	const int CurrentSize = Indices.GetSize();

	Indices.SetSize(CurrentSize + IndexCount);

	*IndexBuffer = &Indices[CurrentSize];
}

void lcLibraryMeshData::AddMeshData(const lcLibraryMeshData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcMeshLoaderTextureMap* TextureMap, lcMeshDataType OverrideDestIndex)
{
	for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
	{
		const int DestIndex = OverrideDestIndex == LC_MESHDATA_SHARED ? MeshDataIdx : OverrideDestIndex;
		mData[DestIndex].AddMeshData(Data.mData[MeshDataIdx], Transform, CurrentColorCode, InvertWinding, InvertNormals, TextureMap);
	}

	mHasTextures |= (Data.mHasTextures || TextureMap);
}

void lcLibraryMeshData::AddMeshDataNoDuplicateCheck(const lcLibraryMeshData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcMeshLoaderTextureMap* TextureMap, lcMeshDataType OverrideDestIndex)
{
	for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
	{
		const int DestIndex = OverrideDestIndex == LC_MESHDATA_SHARED ? MeshDataIdx : OverrideDestIndex;
		mData[DestIndex].AddMeshDataNoDuplicateCheck(Data.mData[MeshDataIdx], Transform, CurrentColorCode, InvertWinding, InvertNormals, TextureMap);
	}

	mHasTextures |= (Data.mHasTextures || TextureMap);
}

lcMeshLoaderMaterial* lcLibraryMeshData::GetMaterial(quint32 ColorCode)
{
	for (const std::unique_ptr<lcMeshLoaderMaterial>& Material : mMaterials)
		if (Material->Type == lcMeshLoaderMaterialType::Solid && Material->Color == ColorCode)
			return Material.get();

	lcMeshLoaderMaterial* Material = new lcMeshLoaderMaterial();
	mMaterials.emplace_back(Material);

	Material->Type = lcMeshLoaderMaterialType::Solid;
	Material->Color = ColorCode;

	return Material;
}

lcMeshLoaderMaterial* lcLibraryMeshData::GetTexturedMaterial(quint32 ColorCode, const lcMeshLoaderTextureMap& TextureMap)
{
	for (const std::unique_ptr<lcMeshLoaderMaterial>& Material : mMaterials)
	{
		if (Material->Type != TextureMap.Type || Material->Color != ColorCode)
			continue;

		if (strcmp(Material->Name, TextureMap.Name))
			continue;

		if (Material->Points[0] != TextureMap.Points[0] || Material->Points[1] != TextureMap.Points[1] || Material->Points[2] != TextureMap.Points[2])
			continue;

		if (Material->Type == lcMeshLoaderMaterialType::Cylindrical)
		{
			if (Material->Angles[0] != TextureMap.Angles[0])
				continue;
		}
		else if (Material->Type == lcMeshLoaderMaterialType::Spherical)
		{
			if (Material->Angles[0] != TextureMap.Angles[0] || Material->Angles[1] != TextureMap.Angles[1])
				continue;
		}

		return Material.get();
	}

	lcMeshLoaderMaterial* Material = new lcMeshLoaderMaterial();
	mMaterials.emplace_back(Material);

	Material->Type = TextureMap.Type;
	Material->Color = ColorCode;
	Material->Points[0] = TextureMap.Points[0];
	Material->Points[1] = TextureMap.Points[1];
	Material->Points[2] = TextureMap.Points[2];
	Material->Angles[0] = TextureMap.Angles[0];
	Material->Angles[1] = TextureMap.Angles[1];
	strcpy(Material->Name, TextureMap.Name);

	return Material;
}

static bool lcMeshLoaderFinalSectionCompare(const lcMeshLoaderFinalSection& a, const lcMeshLoaderFinalSection& b)
{
	if (a.PrimitiveType != b.PrimitiveType)
	{
		const lcMeshPrimitiveType PrimitiveOrder[LC_MESH_NUM_PRIMITIVE_TYPES] =
		{
			LC_MESH_TRIANGLES,
			LC_MESH_TEXTURED_TRIANGLES,
			LC_MESH_LINES,
			LC_MESH_CONDITIONAL_LINES
		};

		for (int PrimitiveType = 0; PrimitiveType < LC_MESH_NUM_PRIMITIVE_TYPES; PrimitiveType++)
		{
			const lcMeshPrimitiveType Primitive = PrimitiveOrder[PrimitiveType];

			if (a.PrimitiveType == Primitive)
				return true;

			if (b.PrimitiveType == Primitive)
				return false;
		}
	}

	const bool TranslucentA = lcIsColorTranslucent(a.Color);
	const bool TranslucentB = lcIsColorTranslucent(b.Color);

	if (TranslucentA != TranslucentB)
		return !TranslucentA;

	return a.Color > b.Color;
}

quint32 lcLibraryMeshData::AddTexturedVertex(const lcVector3& Position, const lcVector3& Normal, const lcVector2& TexCoords)
{
	for (int VertexIndex = mTexturedVertices.GetSize() - 1; VertexIndex >= 0; VertexIndex--)
	{
		const lcMeshLoaderTexturedVertex& Vertex = mTexturedVertices[VertexIndex];

		if (Vertex.Position == Position && Vertex.Normal == Normal && Vertex.TexCoords == TexCoords)
			return VertexIndex;
	}

	lcMeshLoaderTexturedVertex& Vertex = mTexturedVertices.Add();

	Vertex.Position = Position;
	Vertex.Normal = Normal;
	Vertex.TexCoords = TexCoords;

	return mTexturedVertices.GetSize() - 1;
}

void lcLibraryMeshData::GeneratePlanarTexcoords(lcMeshLoaderSection* Section, const lcMeshLoaderTypeData& Data)
{
	const lcMeshLoaderMaterial* Material = Section->mMaterial;
	lcVector4 Planes[2];

	for (int EdgeIdx = 0; EdgeIdx < 2; EdgeIdx++)
	{
		lcVector3 Normal = Material->Points[EdgeIdx + 1] - Material->Points[0];
		const float Length = lcLength(Normal);
		Normal /= Length;

		Planes[EdgeIdx].x = Normal.x / Length;
		Planes[EdgeIdx].y = Normal.y / Length;
		Planes[EdgeIdx].z = Normal.z / Length;
		Planes[EdgeIdx].w = -lcDot(Normal, Material->Points[0]) / Length;
	}

	for (quint32& Index : Section->mIndices)
	{
		const lcMeshLoaderVertex& SrcVertex = Data.mVertices[Index];

		const lcVector2 TexCoords(lcDot3(SrcVertex.Position, Planes[0]) + Planes[0].w, lcDot3(SrcVertex.Position, Planes[1]) + Planes[1].w);

		Index = AddTexturedVertex(SrcVertex.Position, SrcVertex.Normal, TexCoords);
	}
}

void lcLibraryMeshData::GenerateCylindricalTexcoords(lcMeshLoaderSection* Section, const lcMeshLoaderTypeData& Data)
{
	const lcMeshLoaderMaterial* Material = Section->mMaterial;
	const lcVector3 Up = Material->Points[0] - Material->Points[1];
	const float UpLength = lcLength(Up);
	const lcVector3 Front = lcNormalize(Material->Points[2] - Material->Points[1]);
	const lcVector3 Plane1Normal = Up / UpLength;
	const lcVector3 Plane2Normal = lcNormalize(lcCross(Front, Up));
	const lcVector4 FrontPlane = lcVector4(Front, -lcDot(Front, Material->Points[1]));
	const lcVector4 Plane1 = lcVector4(Plane1Normal, -lcDot(Plane1Normal, Material->Points[1]));
	const lcVector4 Plane2 = lcVector4(Plane2Normal, -lcDot(Plane2Normal, Material->Points[1]));
	const float Angle = 360.0f / Material->Angles[0];

	for (int TriangleIndex = 0; TriangleIndex < Section->mIndices.GetSize(); TriangleIndex += 3)
	{
		const lcVector3 Positions[3] =
		{
			Data.mVertices[Section->mIndices[TriangleIndex + 0]].Position,
			Data.mVertices[Section->mIndices[TriangleIndex + 1]].Position,
			Data.mVertices[Section->mIndices[TriangleIndex + 2]].Position
		};

		lcVector2 TexCoords[3];

		for (int CornerIndex = 0; CornerIndex < 3; CornerIndex++)
		{
			const float DotPlane1 = lcDot(lcVector4(Positions[CornerIndex], 1.0f), Plane1);
			const lcVector3 PointInPlane1 = Positions[CornerIndex] - lcVector3(Plane1) * DotPlane1;
			const float DotFrontPlane = lcDot(lcVector4(PointInPlane1, 1.0f), FrontPlane);
			const float DotPlane2 = lcDot(lcVector4(PointInPlane1, 1.0f), Plane2);
			const float Angle1 = atan2f(DotPlane2, DotFrontPlane) / LC_PI * Angle;

			TexCoords[CornerIndex].x = lcClamp(0.5f + 0.5f * Angle1, 0.0f, 1.0f);
			TexCoords[CornerIndex].y = DotPlane1 / UpLength;
		}

		lcCheckTexCoordsWrap(Plane2, Positions, TexCoords);

		for (int CornerIndex = 0; CornerIndex < 3; CornerIndex++)
			Section->mIndices[TriangleIndex + CornerIndex] = AddTexturedVertex(Positions[CornerIndex], Data.mVertices[Section->mIndices[TriangleIndex + CornerIndex]].Normal, TexCoords[CornerIndex]);
	}
}

void lcLibraryMeshData::GenerateSphericalTexcoords(lcMeshLoaderSection* Section, const lcMeshLoaderTypeData& Data)
{
	const lcMeshLoaderMaterial* Material = Section->mMaterial;
	const lcVector3 Front = lcNormalize(Material->Points[1] - Material->Points[0]);
	const lcVector3 Plane1Normal = lcNormalize(lcCross(Front, Material->Points[2] - Material->Points[0]));
	const lcVector3 Plane2Normal = lcNormalize(lcCross(Plane1Normal, Front));
	const lcVector4 FrontPlane = lcVector4(Front, -lcDot(Front, Material->Points[0]));
	const lcVector3 Center = Material->Points[0];
	const lcVector4 Plane1 = lcVector4(Plane1Normal, -lcDot(Plane1Normal, Material->Points[0]));
	const lcVector4 Plane2 = lcVector4(Plane2Normal, -lcDot(Plane2Normal, Material->Points[0]));
	const float Angle1 = 360.0f / Material->Angles[0];
	const float Angle2 = 180.0f / Material->Angles[1];

	for (int TriangleIndex = 0; TriangleIndex < Section->mIndices.GetSize(); TriangleIndex += 3)
	{
		const lcVector3 Positions[3] =
		{
			Data.mVertices[Section->mIndices[TriangleIndex + 0]].Position,
			Data.mVertices[Section->mIndices[TriangleIndex + 1]].Position,
			Data.mVertices[Section->mIndices[TriangleIndex + 2]].Position
		};

		lcVector2 TexCoords[3];

		for (int CornerIndex = 0; CornerIndex < 3; CornerIndex++)
		{
			const lcVector3 VertexDir = Positions[CornerIndex] - Center;

			const float DotPlane1 = lcDot(lcVector4(Positions[CornerIndex], 1.0f), Plane1);
			const lcVector3 PointInPlane1 = Positions[CornerIndex] - lcVector3(Plane1) * DotPlane1;
			const float DotFrontPlane = lcDot(lcVector4(PointInPlane1, 1.0f), FrontPlane);
			const float DotPlane2 = lcDot(lcVector4(PointInPlane1, 1.0f), Plane2);

			const float AngleX = atan2f(DotPlane2, DotFrontPlane) / LC_PI * Angle1;
			TexCoords[CornerIndex].x = 0.5f + 0.5f * AngleX;

			const float AngleY = asinf(DotPlane1 / lcLength(VertexDir)) / LC_PI * Angle2;
			TexCoords[CornerIndex].y = 0.5f - AngleY;
		}

		lcCheckTexCoordsWrap(Plane2, Positions, TexCoords);
		lcCheckTexCoordsPole(FrontPlane, Plane2, Positions, TexCoords);

		for (int CornerIndex = 0; CornerIndex < 3; CornerIndex++)
			Section->mIndices[TriangleIndex + CornerIndex] = AddTexturedVertex(Positions[CornerIndex], Data.mVertices[Section->mIndices[TriangleIndex + CornerIndex]].Normal, TexCoords[CornerIndex]);
	}
}

void lcLibraryMeshData::GenerateTexturedVertices()
{
	for (lcMeshLoaderTypeData& Data : mData)
	{
		for (const std::unique_ptr<lcMeshLoaderSection>& Section : Data.mSections)
		{
			switch (Section->mMaterial->Type)
			{
				case lcMeshLoaderMaterialType::Solid:
					break;

				case lcMeshLoaderMaterialType::Planar:
					GeneratePlanarTexcoords(Section.get(), Data);
					break;

				case lcMeshLoaderMaterialType::Cylindrical:
					GenerateCylindricalTexcoords(Section.get(), Data);
					break;

				case lcMeshLoaderMaterialType::Spherical:
					GenerateSphericalTexcoords(Section.get(), Data);
					break;
			}
		}
	}
}

lcMesh* lcLibraryMeshData::CreateMesh()
{
	lcMesh* Mesh = new lcMesh();

	int BaseVertices[LC_NUM_MESHDATA_TYPES];
	int BaseConditionalVertices[LC_NUM_MESHDATA_TYPES];
	int NumVertices = 0;
	int ConditionalVertexCount = 0;

	for (const std::unique_ptr<lcMeshLoaderMaterial>& Material : mMaterials)
		Material->Color = lcGetColorIndex(Material->Color);

	for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
	{
		BaseVertices[MeshDataIdx] = NumVertices;
		NumVertices += mData[MeshDataIdx].mVertices.GetSize();
		BaseConditionalVertices[MeshDataIdx] = ConditionalVertexCount;
		ConditionalVertexCount += mData[MeshDataIdx].mConditionalVertices.GetSize();
	}

	if (mHasTextures)
		GenerateTexturedVertices();

	quint16 NumSections[LC_NUM_MESH_LODS];
	int NumIndices = 0;

	lcArray<lcMeshLoaderFinalSection> FinalSections[LC_NUM_MESH_LODS];

	for (int LodIdx = 0; LodIdx < LC_NUM_MESH_LODS; LodIdx++)
	{
		auto AddFinalSection = [](lcMeshLoaderSection* Section, lcArray<lcMeshLoaderFinalSection>& FinalSections)
		{
			for (const lcMeshLoaderFinalSection& FinalSection : FinalSections)
				if (FinalSection.PrimitiveType == Section->mPrimitiveType && FinalSection.Color == Section->mMaterial->Color && !strcmp(FinalSection.Name, Section->mMaterial->Name))
					return;

			lcMeshLoaderFinalSection& FinalSection = FinalSections.Add();

			FinalSection.PrimitiveType = Section->mPrimitiveType;
			FinalSection.Color = Section->mMaterial->Color;
			strcpy(FinalSection.Name, Section->mMaterial->Name);
		};

		for (const std::unique_ptr<lcMeshLoaderSection>& Section : mData[LC_MESHDATA_SHARED].mSections)
		{
			NumIndices += Section->mIndices.GetSize();

			AddFinalSection(Section.get(), FinalSections[LodIdx]);
		}

		for (const std::unique_ptr<lcMeshLoaderSection>& Section : mData[LodIdx].mSections)
		{
			NumIndices += Section->mIndices.GetSize();

			AddFinalSection(Section.get(), FinalSections[LodIdx]);
		}

		NumSections[LodIdx] = FinalSections[LodIdx].GetSize();
		std::sort(FinalSections[LodIdx].begin(), FinalSections[LodIdx].end(), lcMeshLoaderFinalSectionCompare);
	}

	Mesh->Create(NumSections, NumVertices, mTexturedVertices.GetSize(), ConditionalVertexCount, NumIndices);

	lcVertex* DstVerts = (lcVertex*)Mesh->mVertexData;

	for (const lcMeshLoaderTypeData& Data : mData)
	{
		for (const lcMeshLoaderVertex& SrcVertex : Data.mVertices)
		{
			lcVertex& DstVertex = *DstVerts++;

			DstVertex.Position = lcVector3LDrawToLeoCAD(SrcVertex.Position);
			DstVertex.Normal = lcPackNormal(lcVector3LDrawToLeoCAD(SrcVertex.Normal));
		}
	}

	lcVertexTextured* DstTexturedVerts = (lcVertexTextured*)DstVerts;

	if (mHasTextures)
	{
		for (const lcMeshLoaderTexturedVertex& SrcVertex : mTexturedVertices)
		{
			lcVertexTextured& DstVertex = *DstTexturedVerts++;

			DstVertex.Position = lcVector3LDrawToLeoCAD(SrcVertex.Position);
			DstVertex.Normal = lcPackNormal(lcVector3LDrawToLeoCAD(SrcVertex.Normal));
			DstVertex.TexCoord = SrcVertex.TexCoords;
		}
	}

	lcVertexConditional* DstConditionalVerts = (lcVertexConditional*)DstTexturedVerts;

	for (const lcMeshLoaderTypeData& Data : mData)
	{
		for (const lcMeshLoaderConditionalVertex& SrcVertex : Data.mConditionalVertices)
		{
			lcVertexConditional& DstVertex = *DstConditionalVerts++;

			DstVertex.Position1 = lcVector3LDrawToLeoCAD(SrcVertex.Position[0]);
			DstVertex.Position2 = lcVector3LDrawToLeoCAD(SrcVertex.Position[1]);
			DstVertex.Position3 = lcVector3LDrawToLeoCAD(SrcVertex.Position[2]);
			DstVertex.Position4 = lcVector3LDrawToLeoCAD(SrcVertex.Position[3]);
		}
	}

	if (Mesh->mIndexType == GL_UNSIGNED_SHORT)
		WriteSections<quint16>(Mesh, FinalSections, BaseVertices, BaseConditionalVertices);
	else
		WriteSections<quint32>(Mesh, FinalSections, BaseVertices, BaseConditionalVertices);

	if (mHasStyleStud)
		Mesh->mFlags |= lcMeshFlag::HasStyleStud;

	UpdateMeshBoundingBox(Mesh);

	return Mesh;
}

template<typename IndexType>
void lcLibraryMeshData::WriteSections(lcMesh* Mesh, const lcArray<lcMeshLoaderFinalSection> (&FinalSections)[LC_NUM_MESH_LODS], int(&BaseVertices)[LC_NUM_MESHDATA_TYPES], int(&BaseConditionalVertices)[LC_NUM_MESHDATA_TYPES])
{
	int NumIndices = 0;

	for (int LodIdx = 0; LodIdx < LC_NUM_MESH_LODS; LodIdx++)
	{
		for (int SectionIdx = 0; SectionIdx < FinalSections[LodIdx].GetSize(); SectionIdx++)
		{
			const lcMeshLoaderFinalSection& FinalSection = FinalSections[LodIdx][SectionIdx];
			lcMeshSection& DstSection = Mesh->mLods[LodIdx].Sections[SectionIdx];

			DstSection.ColorIndex = FinalSection.Color;
			DstSection.PrimitiveType = FinalSection.PrimitiveType;
			DstSection.NumIndices = 0;

			if (!FinalSection.Name[0])
				DstSection.Texture = nullptr;
			else
			{
				if (mMeshLoader)
					DstSection.Texture = lcGetPiecesLibrary()->FindTexture(FinalSection.Name, mMeshLoader->mCurrentProject, mMeshLoader->mSearchProjectFolder);
				else
					DstSection.Texture = lcGetPiecesLibrary()->FindTexture(FinalSection.Name, nullptr, false);

				if (DstSection.Texture)
					DstSection.Texture->AddRef();
			}

			DstSection.IndexOffset = NumIndices * sizeof(IndexType);

			IndexType* Index = (IndexType*)Mesh->mIndexData + NumIndices;

			const auto AddSection = [&DstSection, &Index, &BaseVertices, &BaseConditionalVertices](lcMeshLoaderSection* SrcSection, lcMeshDataType SrcDataType)
			{
				switch (DstSection.PrimitiveType)
				{
					case LC_MESH_LINES:
					case LC_MESH_TRIANGLES:
					{
						const IndexType BaseVertex = BaseVertices[SrcDataType];

						for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
							*Index++ = BaseVertex + SrcSection->mIndices[IndexIdx];
					}
					break;

					case LC_MESH_CONDITIONAL_LINES:
					{
						const IndexType BaseVertex = BaseConditionalVertices[SrcDataType];

						for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
							*Index++ = BaseVertex + SrcSection->mIndices[IndexIdx];
					}
					break;

					case LC_MESH_TEXTURED_TRIANGLES:
					{
						for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
							*Index++ = SrcSection->mIndices[IndexIdx];
					}
					break;

					case LC_MESH_NUM_PRIMITIVE_TYPES:
						break;
				}

				DstSection.NumIndices += SrcSection->mIndices.GetSize();
			};

			for (const std::unique_ptr<lcMeshLoaderSection>& Section : mData[LC_MESHDATA_SHARED].mSections)
				if (FinalSection.PrimitiveType == Section->mPrimitiveType && FinalSection.Color == Section->mMaterial->Color && !strcmp(FinalSection.Name, Section->mMaterial->Name))
					AddSection(Section.get(), LC_MESHDATA_SHARED);

			const lcMeshDataType MeshDataType = (LodIdx == LC_MESH_LOD_LOW) ? LC_MESHDATA_LOW : LC_MESHDATA_HIGH;

			for (const std::unique_ptr<lcMeshLoaderSection>& Section : mData[MeshDataType].mSections)
				if (FinalSection.PrimitiveType == Section->mPrimitiveType && FinalSection.Color == Section->mMaterial->Color && !strcmp(FinalSection.Name, Section->mMaterial->Name))
					AddSection(Section.get(), MeshDataType);

			if (DstSection.PrimitiveType == LC_MESH_TRIANGLES || DstSection.PrimitiveType == LC_MESH_TEXTURED_TRIANGLES)
			{
				if (DstSection.ColorIndex == gDefaultColor)
					Mesh->mFlags |= lcMeshFlag::HasDefault;
				else
				{
					if (lcIsColorTranslucent(DstSection.ColorIndex))
						Mesh->mFlags |= lcMeshFlag::HasTranslucent;
					else
						Mesh->mFlags |= lcMeshFlag::HasSolid;
				}
			}
			else
				Mesh->mFlags |= lcMeshFlag::HasLines;

			if (DstSection.PrimitiveType == LC_MESH_TEXTURED_TRIANGLES)
				Mesh->mFlags |= lcMeshFlag::HasTexture;

			NumIndices += DstSection.NumIndices;
		}
	}
}

void lcLibraryMeshData::UpdateMeshBoundingBox(lcMesh* Mesh)
{
	lcVector3 MeshMin(FLT_MAX, FLT_MAX, FLT_MAX), MeshMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	bool UpdatedBoundingBox = false;

	for (int LodIdx = 0; LodIdx < LC_NUM_MESH_LODS; LodIdx++)
	{
		lcMeshLod& Lod = Mesh->mLods[LodIdx];

		for (int SectionIdx = 0; SectionIdx < Lod.NumSections; SectionIdx++)
		{
			lcMeshSection& Section = Lod.Sections[SectionIdx];
			lcVector3 SectionMin(FLT_MAX, FLT_MAX, FLT_MAX), SectionMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

			if (Mesh->mIndexType == GL_UNSIGNED_SHORT)
				UpdateMeshSectionBoundingBox<quint16>(Mesh, Section, SectionMin, SectionMax);
			else
				UpdateMeshSectionBoundingBox<quint32>(Mesh, Section, SectionMin, SectionMax);

			Section.BoundingBox.Max = SectionMax;
			Section.BoundingBox.Min = SectionMin;
			Section.Radius = lcLength((SectionMax - SectionMin) / 2.0f);

			if (Section.PrimitiveType == LC_MESH_TRIANGLES || Section.PrimitiveType == LC_MESH_TEXTURED_TRIANGLES)
			{
				UpdatedBoundingBox = true;
				MeshMin = lcMin(SectionMin, MeshMin);
				MeshMax = lcMax(SectionMax, MeshMax);
			}
		}
	}

	if (!UpdatedBoundingBox)
		MeshMin = MeshMax = lcVector3(0.0f, 0.0f, 0.0f);

	Mesh->mBoundingBox.Max = MeshMax;
	Mesh->mBoundingBox.Min = MeshMin;
	Mesh->mRadius = lcLength((MeshMax - MeshMin) / 2.0f);
}

template<typename IndexType>
void lcLibraryMeshData::UpdateMeshSectionBoundingBox(const lcMesh* Mesh, const lcMeshSection& Section, lcVector3& SectionMin, lcVector3& SectionMax)
{
	const IndexType* IndexBuffer = reinterpret_cast<IndexType*>(static_cast<char*>(Mesh->mIndexData) + Section.IndexOffset);

	switch (Section.PrimitiveType)
	{
		case LC_MESH_LINES:
		case LC_MESH_TRIANGLES:
		{
			const lcVertex* VertexBuffer = Mesh->GetVertexData();

			for (int Index = 0; Index < Section.NumIndices; Index++)
			{
				const lcVector3& Position = VertexBuffer[IndexBuffer[Index]].Position;
				SectionMin = lcMin(SectionMin, Position);
				SectionMax = lcMax(SectionMax, Position);
			}
		}
		break;

		case LC_MESH_CONDITIONAL_LINES:
		{
			const lcVertexConditional* VertexBuffer = Mesh->GetConditionalVertexData();

			for (int Index = 0; Index < Section.NumIndices; Index++)
			{
				const lcVector3& Position = VertexBuffer[IndexBuffer[Index]].Position1;
				SectionMin = lcMin(SectionMin, Position);
				SectionMax = lcMax(SectionMax, Position);
			}
		}
		break;

		case LC_MESH_TEXTURED_TRIANGLES:
		{
			const lcVertexTextured* VertexBuffer = Mesh->GetTexturedVertexData();

			for (int Index = 0; Index < Section.NumIndices; Index++)
			{
				const lcVector3& Position = VertexBuffer[IndexBuffer[Index]].Position;
				SectionMin = lcMin(SectionMin, Position);
				SectionMax = lcMax(SectionMax, Position);
			}
		}
		break;

		case LC_MESH_NUM_PRIMITIVE_TYPES:
			break;
	}
}

lcMeshLoader::lcMeshLoader(lcLibraryMeshData& MeshData, bool Optimize, Project* CurrentProject, bool SearchProjectFolder)
	: mCurrentProject(CurrentProject), mSearchProjectFolder(SearchProjectFolder), mMeshData(MeshData), mOptimize(Optimize)
{
	MeshData.SetMeshLoader(this);
}

bool lcMeshLoader::LoadMesh(lcFile& File, lcMeshDataType MeshDataType)
{
	return ReadMeshData(File, lcMatrix44Identity(), 16, false, MeshDataType);
}

bool lcMeshLoader::ReadMeshData(lcFile& File, const lcMatrix44& CurrentTransform, quint32 CurrentColorCode, bool InvertWinding, lcMeshDataType MeshDataType)
{
	char Buffer[1024];
	char* Line;
	bool InvertNext = false;
	bool WindingCCW = !InvertWinding;
	lcPiecesLibrary* Library = lcGetPiecesLibrary();

	while (File.ReadLine(Buffer, sizeof(Buffer)))
	{
		if (Library->ShouldCancelLoading())
			return false;

		quint32 ColorCode, ColorCodeHex;
		bool LastToken = false;
		int LineType;

		Line = Buffer;

		if (sscanf(Line, "%d", &LineType) != 1)
			continue;

		if (LineType == 0)
		{
			char* Token = Line;

			while (*Token && *Token <= 32)
				Token++;

			Token++;

			while (*Token && *Token <= 32)
				Token++;

			char* End = Token;
			while (*End && *End > 32)
				End++;

			LastToken = (*End == 0);
			*End = 0;

			if (!strcmp(Token, "!TEXMAP"))
			{
				Token += 8;

				while (*Token && *Token <= 32)
					Token++;

				End = Token;
				while (*End && *End > 32)
					End++;
				*End = 0;

				bool Start = false;
				bool Next = false;

				if (!strcmp(Token, "START"))
				{
					Token += 6;
					Start = true;
				}
				else if (!strcmp(Token, "NEXT"))
				{
					Token += 5;
					Next = true;
				}

				if (Start || Next)
				{
					while (*Token && *Token <= 32)
						Token++;

					End = Token;
					while (*End && *End > 32)
						End++;
					*End = 0;

					auto CleanTextureName = [](char* FileName)
					{
						char* Ch;
						for (Ch = FileName; *Ch; Ch++)
						{
							if (*Ch >= 'a' && *Ch <= 'z')
								*Ch = *Ch + 'A' - 'a';
							else if (*Ch == '\\')
								*Ch = '/';
						}

						if (Ch - FileName > 4)
						{
							Ch -= 4;
							if (!memcmp(Ch, ".PNG", 4))
								*Ch = 0;
						}
					};

					if (!strcmp(Token, "PLANAR"))
					{
						Token += 7;

						mTextureStack.emplace_back();
						lcMeshLoaderTextureMap& Map = mTextureStack.back();
						Map.Type = lcMeshLoaderMaterialType::Planar;

						lcVector3 (&Points)[3] = Map.Points;

						sscanf(Token, "%f %f %f %f %f %f %f %f %f %s", &Points[0].x, &Points[0].y, &Points[0].z, &Points[1].x, &Points[1].y, &Points[1].z, &Points[2].x, &Points[2].y, &Points[2].z, Map.Name);

						Points[0] = lcMul31(Points[0], CurrentTransform);
						Points[1] = lcMul31(Points[1], CurrentTransform);
						Points[2] = lcMul31(Points[2], CurrentTransform);

						CleanTextureName(Map.Name);
					}
					else if (!strcmp(Token, "CYLINDRICAL"))
					{
						Token += 12;

						mTextureStack.emplace_back();
						lcMeshLoaderTextureMap& Map = mTextureStack.back();
						Map.Type = lcMeshLoaderMaterialType::Cylindrical;

						lcVector3 (&Points)[3] = Map.Points;
						float& Angle = Map.Angles[0];

						sscanf(Token, "%f %f %f %f %f %f %f %f %f %f %s", &Points[0].x, &Points[0].y, &Points[0].z, &Points[1].x, &Points[1].y, &Points[1].z, &Points[2].x, &Points[2].y, &Points[2].z, &Angle, Map.Name);

						Points[0] = lcMul31(Points[0], CurrentTransform);
						Points[1] = lcMul31(Points[1], CurrentTransform);
						Points[2] = lcMul31(Points[2], CurrentTransform);

						CleanTextureName(Map.Name);
					}
					else if (!strcmp(Token, "SPHERICAL"))
					{
						Token += 10;

						mTextureStack.emplace_back();
						lcMeshLoaderTextureMap& Map = mTextureStack.back();
						Map.Type = lcMeshLoaderMaterialType::Spherical;

						lcVector3(&Points)[3] = Map.Points;
						float& Angle1 = Map.Angles[0];
						float& Angle2 = Map.Angles[1];

						sscanf(Token, "%f %f %f %f %f %f %f %f %f %f %f %s", &Points[0].x, &Points[0].y, &Points[0].z, &Points[1].x, &Points[1].y, &Points[1].z, &Points[2].x, &Points[2].y, &Points[2].z, &Angle1, &Angle2, Map.Name);

						Points[0] = lcMul31(Points[0], CurrentTransform);
						Points[1] = lcMul31(Points[1], CurrentTransform);
						Points[2] = lcMul31(Points[2], CurrentTransform);

						CleanTextureName(Map.Name);
					}
				}
				else if (!strcmp(Token, "FALLBACK"))
				{
					if (!mTextureStack.empty())
						mTextureStack.back().Fallback = true;
				}
				else if (!strcmp(Token, "END"))
				{
					if (!mTextureStack.empty())
						mTextureStack.pop_back();
				}

				continue;
			}
			else if (!strcmp(Token, "BFC"))
			{
				while (!LastToken)
				{
					Token = End + 1;

					while (*Token && *Token <= 32)
						Token++;

					End = Token;
					while (*End && *End > 32)
						End++;

					LastToken = (*End == 0);
					*End = 0;

					if (!strcmp(Token, "INVERTNEXT"))
						InvertNext = true;
					else if (!strcmp(Token, "CCW"))
						WindingCCW = !InvertWinding;
					else if (!strcmp(Token, "CW"))
						WindingCCW = InvertWinding;
				}
			}
			else if (!strcmp(Token, "!:"))
			{
				Token += 3;

				Line = Token;

				if (mTextureStack.empty())
					continue;
			}
			else
				continue;
		}

		if (sscanf(Line, "%d %d", &LineType, &ColorCode) != 2)
			continue;

		if (LineType < 1 || LineType > 5)
			continue;

		if (ColorCode == 0)
		{
			sscanf(Line, "%d %i", &LineType, &ColorCodeHex);

			if (ColorCode != ColorCodeHex)
				ColorCode = ColorCodeHex | LC_COLOR_DIRECT;
		}

		if (ColorCode == 16)
			ColorCode = CurrentColorCode;

		lcMeshLoaderTextureMap* TextureMap = nullptr;

		if (!mTextureStack.empty())
		{
			TextureMap = &mTextureStack.back();

			// TODO: think about a way to handle the texture fallback
//			if (TextureMap->Texture)
			{
				if (TextureMap->Fallback)
					continue;
			}
//			else
//			{
//				if (!TextureMap->Fallback)
//					continue;
//
//				TextureMap = nullptr;
//			}
		}

		int Dummy;
		lcVector3 Points[4];

		switch (LineType)
		{
		case 1:
		{
			char OriginalFileName[LC_MAXPATH];
			float fm[12];

			sscanf(Line, "%d %i %f %f %f %f %f %f %f %f %f %f %f %f %s", &LineType, &Dummy, &fm[0], &fm[1], &fm[2], &fm[3], &fm[4], &fm[5], &fm[6], &fm[7], &fm[8], &fm[9], &fm[10], &fm[11], OriginalFileName);

			char FileName[LC_MAXPATH];
			strcpy(FileName, OriginalFileName);

			char* Ch;
			for (Ch = FileName; *Ch; Ch++)
			{
				if (*Ch >= 'a' && *Ch <= 'z')
					*Ch = *Ch + 'A' - 'a';
				else if (*Ch == '\\')
					*Ch = '/';
			}

			lcLibraryPrimitive* Primitive = !TextureMap ? Library->FindPrimitive(FileName) : nullptr;
			lcMatrix44 IncludeTransform(lcVector4(fm[3], fm[6], fm[9], 0.0f), lcVector4(fm[4], fm[7], fm[10], 0.0f), lcVector4(fm[5], fm[8], fm[11], 0.0f), lcVector4(fm[0], fm[1], fm[2], 1.0f));
			IncludeTransform = lcMul(IncludeTransform, CurrentTransform);
			bool Mirror = IncludeTransform.Determinant() < 0.0f;

			const auto FileCallback = [this, &IncludeTransform, &ColorCode, &Mirror, &InvertNext, &MeshDataType](lcFile& File)
			{
				ReadMeshData(File, IncludeTransform, ColorCode, Mirror ^ InvertNext, MeshDataType);
			};

			if (Primitive)
			{
				if (Primitive->mState != lcPrimitiveState::Loaded && !Library->LoadPrimitive(Primitive))
					break;

				if (Primitive->mStud)
					mMeshData.AddMeshDataNoDuplicateCheck(Primitive->mMeshData, IncludeTransform, ColorCode, Mirror ^ InvertNext, InvertNext, TextureMap, MeshDataType);
				else if (!Primitive->mSubFile)
				{
					if (mOptimize)
						mMeshData.AddMeshData(Primitive->mMeshData, IncludeTransform, ColorCode, Mirror ^ InvertNext, InvertNext, TextureMap, MeshDataType);
					else
						mMeshData.AddMeshDataNoDuplicateCheck(Primitive->mMeshData, IncludeTransform, ColorCode, Mirror ^ InvertNext, InvertNext, TextureMap, MeshDataType);
				}
				else
					Library->GetPrimitiveFile(Primitive, FileCallback);

				mMeshData.mHasStyleStud |= Primitive->mStudStyle | Primitive->mMeshData.mHasStyleStud;
			}
			else
				Library->GetPieceFile(FileName, FileCallback);
		} break;

		case 2:
			sscanf(Line, "%d %i %f %f %f %f %f %f", &LineType, &Dummy, &Points[0].x, &Points[0].y, &Points[0].z, &Points[1].x, &Points[1].y, &Points[1].z);

			Points[0] = lcMul31(Points[0], CurrentTransform);
			Points[1] = lcMul31(Points[1], CurrentTransform);

			mMeshData.mData[MeshDataType].ProcessLine(LineType, mMeshData.GetMaterial(ColorCode), WindingCCW, Points, mOptimize);
			break;

		case 3:
			sscanf(Line, "%d %i %f %f %f %f %f %f %f %f %f", &LineType, &Dummy, &Points[0].x, &Points[0].y, &Points[0].z,
				   &Points[1].x, &Points[1].y, &Points[1].z, &Points[2].x, &Points[2].y, &Points[2].z);

			Points[0] = lcMul31(Points[0], CurrentTransform);
			Points[1] = lcMul31(Points[1], CurrentTransform);
			Points[2] = lcMul31(Points[2], CurrentTransform);

			if (!TextureMap)
				mMeshData.mData[MeshDataType].ProcessLine(LineType, mMeshData.GetMaterial(ColorCode), WindingCCW, Points, mOptimize);
			else
			{
				mMeshData.mHasTextures = true;
				mMeshData.mData[MeshDataType].ProcessLine(LineType, mMeshData.GetTexturedMaterial(ColorCode, *TextureMap), WindingCCW, Points, mOptimize);

				if (TextureMap->Next)
					mTextureStack.pop_back();
			}
			break;

		case 4:
			sscanf(Line, "%d %i %f %f %f %f %f %f %f %f %f %f %f %f", &LineType, &Dummy, &Points[0].x, &Points[0].y, &Points[0].z,
				   &Points[1].x, &Points[1].y, &Points[1].z, &Points[2].x, &Points[2].y, &Points[2].z, &Points[3].x, &Points[3].y, &Points[3].z);

			Points[0] = lcMul31(Points[0], CurrentTransform);
			Points[1] = lcMul31(Points[1], CurrentTransform);
			Points[2] = lcMul31(Points[2], CurrentTransform);
			Points[3] = lcMul31(Points[3], CurrentTransform);

			if (!TextureMap)
				mMeshData.mData[MeshDataType].ProcessLine(LineType, mMeshData.GetMaterial(ColorCode), WindingCCW, Points, mOptimize);
			else
			{
				mMeshData.mHasTextures = true;
				mMeshData.mData[MeshDataType].ProcessLine(LineType, mMeshData.GetTexturedMaterial(ColorCode, *TextureMap), WindingCCW, Points, mOptimize);

				if (TextureMap->Next)
					mTextureStack.pop_back();
			}
			break;

		case 5:
			sscanf(Line, "%d %i %f %f %f %f %f %f %f %f %f %f %f %f", &LineType, &Dummy, &Points[0].x, &Points[0].y, &Points[0].z,
				   &Points[1].x, &Points[1].y, &Points[1].z, &Points[2].x, &Points[2].y, &Points[2].z, &Points[3].x, &Points[3].y, &Points[3].z);

			Points[0] = lcMul31(Points[0], CurrentTransform);
			Points[1] = lcMul31(Points[1], CurrentTransform);
			Points[2] = lcMul31(Points[2], CurrentTransform);
			Points[3] = lcMul31(Points[3], CurrentTransform);

			mMeshData.mData[MeshDataType].ProcessLine(LineType, mMeshData.GetMaterial(ColorCode), WindingCCW, Points, mOptimize);
			break;
		}

		InvertNext = false;
	}

	return true;
}
