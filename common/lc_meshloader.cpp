#include "lc_global.h"
#include "lc_meshloader.h"
#include "lc_file.h"
#include "lc_colors.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_texture.h"

static lcVector2 lcCalculateTexCoord(const lcVector3& Position, const lcMeshLoaderTextureMap* TextureMap)
{
	switch (TextureMap->Type)
	{
	case lcMeshLoaderTextureMapType::Planar:
		return lcVector2(lcDot3(Position, TextureMap->Params.Planar.Planes[0]) + TextureMap->Params.Planar.Planes[0].w, lcDot3(Position, TextureMap->Params.Planar.Planes[1]) + TextureMap->Params.Planar.Planes[1].w);

	case lcMeshLoaderTextureMapType::Cylindrical:
	{
		const lcVector4& FrontPlane = TextureMap->Params.Cylindrical.FrontPlane;
		const lcVector4& Plane1 = TextureMap->Params.Cylindrical.Plane1;
		const lcVector4& Plane2 = TextureMap->Params.Cylindrical.Plane2;
		lcVector2 TexCoord;

		float DotPlane1 = lcDot(lcVector4(Position, 1.0f), Plane1);
		lcVector3 PointInPlane1 = Position - lcVector3(Plane1) * DotPlane1;
		float DotFrontPlane = lcDot(lcVector4(PointInPlane1, 1.0f), FrontPlane);
		float DotPlane2 = lcDot(lcVector4(PointInPlane1, 1.0f), Plane2);

		float Angle1 = atan2f(DotPlane2, DotFrontPlane) / LC_PI * TextureMap->Angle1;
		TexCoord.x = lcClamp(0.5f + 0.5f * Angle1, 0.0f, 1.0f);

		TexCoord.y = DotPlane1 / TextureMap->Params.Cylindrical.UpLength;

		return TexCoord;
	}

	case lcMeshLoaderTextureMapType::Spherical:
	{
		const lcVector4& FrontPlane = TextureMap->Params.Spherical.FrontPlane;
		const lcVector3& Center = TextureMap->Params.Spherical.Center;
		const lcVector4& Plane1 = TextureMap->Params.Spherical.Plane1;
		const lcVector4& Plane2 = TextureMap->Params.Spherical.Plane2;
		lcVector2 TexCoord;

		lcVector3 VertexDir = Position - Center;

		float DotPlane1 = lcDot(lcVector4(Position, 1.0f), Plane1);
		lcVector3 PointInPlane1 = Position - lcVector3(Plane1) * DotPlane1;
		float DotFrontPlane = lcDot(lcVector4(PointInPlane1, 1.0f), FrontPlane);
		float DotPlane2 = lcDot(lcVector4(PointInPlane1, 1.0f), Plane2);

		float Angle1 = atan2f(DotPlane2, DotFrontPlane) / LC_PI * TextureMap->Angle1;
		TexCoord.x = 0.5f + 0.5f * Angle1;

		float Angle2 = asinf(DotPlane1 / lcLength(VertexDir)) / LC_PI * TextureMap->Angle2;
		TexCoord.y = 0.5f - Angle2;

		return TexCoord;
	}
	}

	return lcVector2(0.0f, 0.0f);
}

static void lcResequenceQuad(int* Indices, int a, int b, int c, int d)
{
	Indices[0] = a;
	Indices[1] = b;
	Indices[2] = c;
	Indices[3] = d;
}

static void lcTestQuad(int* QuadIndices, const lcVector3* Vertices)
{
	lcVector3 v01 = Vertices[1] - Vertices[0];
	lcVector3 v02 = Vertices[2] - Vertices[0];
	lcVector3 v03 = Vertices[3] - Vertices[0];
	lcVector3 cp1 = lcCross(v01, v02);
	lcVector3 cp2 = lcCross(v02, v03);

	if (lcDot(cp1, cp2) > 0.0f)
		return;

	lcVector3 v12 = Vertices[2] - Vertices[1];
	lcVector3 v13 = Vertices[3] - Vertices[1];
	lcVector3 v23 = Vertices[3] - Vertices[2];

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
}

const float lcDistanceEpsilon = 0.01f; // Maximum value for 50591.dat
const float lcTexCoordEpsilon = 0.01f;

static bool lcCompareVertices(const lcVector3& Position1, const lcVector3& Position2)
{
	return fabsf(Position1.x - Position2.x) < lcDistanceEpsilon && fabsf(Position1.y - Position2.y) < lcDistanceEpsilon && fabsf(Position1.z - Position2.z) < lcDistanceEpsilon;
}

static bool lcCompareVertices(const lcVector3& Position1, const lcVector2& TexCoord1, const lcVector3& Position2, const lcVector2& TexCoord2)
{
	return lcCompareVertices(Position1, Position2) && fabsf(TexCoord1.x - TexCoord2.x) < lcTexCoordEpsilon && fabsf(TexCoord1.y - TexCoord2.y) < lcTexCoordEpsilon;
}

lcLibraryMeshSection* lcMeshLoaderTypeData::AddSection(lcMeshPrimitiveType PrimitiveType, quint32 ColorCode, lcTexture* Texture)
{
	for (std::unique_ptr<lcLibraryMeshSection>& Section : mSections)
		if (Section->mColor == ColorCode && Section->mPrimitiveType == PrimitiveType && Section->mTexture == Texture)
			return Section.get();

	mSections.emplace_back(new lcLibraryMeshSection(PrimitiveType, ColorCode, Texture));

	return mSections.back().get();
}

quint32 lcMeshLoaderTypeData::AddVertex(const lcVector3& Position, bool Optimize)
{
	if (Optimize)
	{
		for (int VertexIdx = mVertices.GetSize() - 1; VertexIdx >= 0; VertexIdx--)
		{
			lcMeshLoaderVertex& Vertex = mVertices[VertexIdx];

			if (lcCompareVertices(Position, Vertex.Position))
			{
				Vertex.Usage |= LC_LIBRARY_VERTEX_UNTEXTURED;
				return VertexIdx;
			}
		}
	}

	lcMeshLoaderVertex& Vertex = mVertices.Add();

	Vertex.Position = Position;
	Vertex.Normal = lcVector3(0.0f, 0.0f, 0.0f);
	Vertex.NormalWeight = 0.0f;
	Vertex.TexCoord = lcVector2(0.0f, 0.0f);
	Vertex.Usage = LC_LIBRARY_VERTEX_UNTEXTURED;

	return mVertices.GetSize() - 1;
}

quint32 lcMeshLoaderTypeData::AddVertex(const lcVector3& Position, const lcVector3& Normal, bool Optimize)
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
					Vertex.NormalWeight = 1.0f;
					Vertex.Usage |= LC_LIBRARY_VERTEX_UNTEXTURED;
					return VertexIdx;
				}
				else if (lcDot(Normal, Vertex.Normal) > 0.707f)
				{
					Vertex.Normal = lcNormalize(Vertex.Normal * Vertex.NormalWeight + Normal);
					Vertex.NormalWeight += 1.0f;
					Vertex.Usage |= LC_LIBRARY_VERTEX_UNTEXTURED;
					return VertexIdx;
				}
			}
		}
	}

	lcMeshLoaderVertex& Vertex = mVertices.Add();

	Vertex.Position = Position;
	Vertex.Normal = Normal;
	Vertex.NormalWeight = 1.0f;
	Vertex.TexCoord = lcVector2(0.0f, 0.0f);
	Vertex.Usage = LC_LIBRARY_VERTEX_UNTEXTURED;

	return mVertices.GetSize() - 1;
}

quint32 lcMeshLoaderTypeData::AddTexturedVertex(const lcVector3& Position, const lcVector2& TexCoord, bool Optimize)
{
	if (Optimize)
	{
		for (int VertexIdx = mVertices.GetSize() - 1; VertexIdx >= 0; VertexIdx--)
		{
			lcMeshLoaderVertex& Vertex = mVertices[VertexIdx];

			if (Vertex.Usage & LC_LIBRARY_VERTEX_TEXTURED)
			{
				if (lcCompareVertices(Position, TexCoord, Vertex.Position, Vertex.TexCoord))
					return VertexIdx;
			}
			else
			{
				if (lcCompareVertices(Position, Vertex.Position))
				{
					Vertex.TexCoord = TexCoord;
					Vertex.Usage |= LC_LIBRARY_VERTEX_TEXTURED;
					return VertexIdx;
				}
			}
		}
	}

	lcMeshLoaderVertex& Vertex = mVertices.Add();

	Vertex.Position = Position;
	Vertex.Normal = lcVector3(0.0f, 0.0f, 0.0f);
	Vertex.NormalWeight = 0.0f;
	Vertex.TexCoord = TexCoord;
	Vertex.Usage = LC_LIBRARY_VERTEX_TEXTURED;

	return mVertices.GetSize() - 1;
}

quint32 lcMeshLoaderTypeData::AddTexturedVertex(const lcVector3& Position, const lcVector3& Normal, const lcVector2& TexCoord, bool Optimize)
{
	if (Optimize)
	{
		for (int VertexIdx = mVertices.GetSize() - 1; VertexIdx >= 0; VertexIdx--)
		{
			lcMeshLoaderVertex& Vertex = mVertices[VertexIdx];

			if (Vertex.Usage & LC_LIBRARY_VERTEX_TEXTURED)
			{
				if (lcCompareVertices(Position, TexCoord, Vertex.Position, Vertex.TexCoord))
				{
					if (Vertex.NormalWeight == 0.0f)
					{
						Vertex.Normal = Normal;
						Vertex.NormalWeight = 1.0f;
						return VertexIdx;
					}
					else if (lcDot(Normal, Vertex.Normal) > 0.707f)
					{
						Vertex.Normal = lcNormalize(Vertex.Normal * Vertex.NormalWeight + Normal);
						Vertex.NormalWeight += 1.0f;
						return VertexIdx;
					}
				}
			}
			else
			{
				if (lcCompareVertices(Position, Vertex.Position))
				{
					if (Vertex.NormalWeight == 0.0f)
					{
						Vertex.Normal = Normal;
						Vertex.NormalWeight = 1.0f;
						Vertex.TexCoord = TexCoord;
						Vertex.Usage |= LC_LIBRARY_VERTEX_TEXTURED;
						return VertexIdx;
					}
					else if (lcDot(Normal, Vertex.Normal) > 0.707f)
					{
						Vertex.Normal = lcNormalize(Vertex.Normal * Vertex.NormalWeight + Normal);
						Vertex.NormalWeight += 1.0f;
						Vertex.TexCoord = TexCoord;
						Vertex.Usage |= LC_LIBRARY_VERTEX_TEXTURED;
						return VertexIdx;
					}
				}
			}
		}
	}

	lcMeshLoaderVertex& Vertex = mVertices.Add();

	Vertex.Position = Position;
	Vertex.Normal = Normal;
	Vertex.NormalWeight = 1.0f;
	Vertex.TexCoord = TexCoord;
	Vertex.Usage = LC_LIBRARY_VERTEX_TEXTURED;

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

void lcMeshLoaderTypeData::ProcessLine(int LineType, quint32 ColorCode, bool WindingCCW, lcVector3 (&Vertices)[4], bool Optimize)
{
	lcMeshPrimitiveType PrimitiveTypes[4] = { LC_MESH_LINES, LC_MESH_TRIANGLES, LC_MESH_TRIANGLES, LC_MESH_CONDITIONAL_LINES };
	lcMeshPrimitiveType PrimitiveType = PrimitiveTypes[LineType - 2];
	lcLibraryMeshSection* Section = AddSection(PrimitiveType, ColorCode, nullptr);

	int QuadIndices[4] = { 0, 1, 2, 3 };
	int Indices[4] = { -1, -1, -1, -1 };

	if (LineType == 3 || LineType == 4)
	{
		if (LineType == 4)
			lcTestQuad(QuadIndices, Vertices);

		lcVector3 Normal = lcNormalize(lcCross(Vertices[1] - Vertices[0], Vertices[2] - Vertices[0]));

		if (!WindingCCW)
			Normal = -Normal;

		for (int IndexIdx = 0; IndexIdx < lcMin(LineType, 4); IndexIdx++)
		{
			const lcVector3& Position = Vertices[QuadIndices[IndexIdx]];
			Indices[IndexIdx] = AddVertex(Position, Normal, Optimize);
		}
	}
	else if (LineType == 2)
	{
		for (int IndexIdx = 0; IndexIdx < 2; IndexIdx++)
		{
			const lcVector3& Position = Vertices[QuadIndices[IndexIdx]];
			Indices[IndexIdx] = AddVertex(Position, Optimize);
		}
	}
	else if (LineType == 5)
	{
		Indices[0] = AddConditionalVertex(Vertices);
		std::swap(Vertices[0], Vertices[1]);
		Indices[1] = AddConditionalVertex(Vertices);
	}

	switch (LineType)
	{
		case 5:
			if (Indices[0] != Indices[1])
			{
				Section->mIndices.Add(Indices[0]);
				Section->mIndices.Add(Indices[1]);
			}
			break;

		case 4:
			if (Indices[0] != Indices[2] && Indices[0] != Indices[3] && Indices[2] != Indices[3])
			{
				if (WindingCCW)
				{
					Section->mIndices.Add(Indices[2]);
					Section->mIndices.Add(Indices[3]);
					Section->mIndices.Add(Indices[0]);
				}
				else
				{
					Section->mIndices.Add(Indices[0]);
					Section->mIndices.Add(Indices[3]);
					Section->mIndices.Add(Indices[2]);
				}
			}
			Q_FALLTHROUGH();

		case 3:
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
			break;

		case 2:
			if (Indices[0] != Indices[1])
			{
				Section->mIndices.Add(Indices[0]);
				Section->mIndices.Add(Indices[1]);
			}
			break;
	}
}

void lcMeshLoaderTypeData::ProcessTexturedLine(int LineType, quint32 ColorCode, bool WindingCCW, const lcMeshLoaderTextureMap& TextureMap, const lcVector3* Vertices, bool Optimize)
{
	lcMeshPrimitiveType PrimitiveType = LC_MESH_TEXTURED_TRIANGLES;
	lcLibraryMeshSection* Section = AddSection(PrimitiveType, ColorCode, TextureMap.Texture);

	int QuadIndices[4] = { 0, 1, 2, 3 };
	int Indices[4] = { -1, -1, -1, -1 };

	if (LineType == 4)
		lcTestQuad(QuadIndices, Vertices);

	lcVector3 Normal = lcNormalize(lcCross(Vertices[1] - Vertices[0], Vertices[2] - Vertices[0]));

	if (!WindingCCW)
		Normal = -Normal;

	lcVector2 TexCoords[4];

	for (int IndexIdx = 0; IndexIdx < lcMin(LineType, 4); IndexIdx++)
	{
		const lcVector3& Position = Vertices[QuadIndices[IndexIdx]];
		TexCoords[QuadIndices[IndexIdx]] = lcCalculateTexCoord(Position, &TextureMap);
	}

	if (TextureMap.Type == lcMeshLoaderTextureMapType::Cylindrical || TextureMap.Type == lcMeshLoaderTextureMapType::Spherical)
	{
		auto CheckTexCoordsWrap = [&TexCoords, &Vertices, &TextureMap](int Index1, int Index2, int Index3)
		{
			float u12 = fabsf(TexCoords[Index1].x - TexCoords[Index2].x);
			float u13 = fabsf(TexCoords[Index1].x - TexCoords[Index3].x);
			float u23 = fabsf(TexCoords[Index2].x - TexCoords[Index3].x);

			if (u12 < 0.5f && u13 < 0.5f && u23 < 0.5f)
				return;

			const lcVector4& Plane2 = (TextureMap.Type == lcMeshLoaderTextureMapType::Cylindrical) ? TextureMap.Params.Cylindrical.Plane2 : TextureMap.Params.Spherical.Plane2;
			float Dot1 = fabsf(lcDot(Plane2, lcVector4(Vertices[Index1], 1.0f)));
			float Dot2 = fabsf(lcDot(Plane2, lcVector4(Vertices[Index2], 1.0f)));
			float Dot3 = fabsf(lcDot(Plane2, lcVector4(Vertices[Index3], 1.0f)));

			if (Dot1 > Dot2)
			{
				if (Dot1 > Dot3)
				{
					if (u12 > 0.5f)
						TexCoords[Index2].x += TexCoords[Index2].x < 0.5f ? 1.0f : -1.0f;

					if (u13 > 0.5f)
						TexCoords[Index3].x += TexCoords[Index3].x < 0.5f ? 1.0f : -1.0f;
				}
				else
				{
					if (u13 > 0.5f)
						TexCoords[Index1].x += TexCoords[Index1].x < 0.5f ? 1.0f : -1.0f;

					if (u23 > 0.5f)
						TexCoords[Index2].x += TexCoords[Index2].x < 0.5f ? 1.0f : -1.0f;
				}
			}
			else
			{
				if (Dot2 > Dot3)
				{
					if (u12 > 0.5f)
						TexCoords[Index1].x += TexCoords[Index1].x < 0.5f ? 1.0f : -1.0f;

					if (u23 > 0.5f)
						TexCoords[Index3].x += TexCoords[Index3].x < 0.5f ? 1.0f : -1.0f;
				}
				else
				{
					if (u13 > 0.5f)
						TexCoords[Index1].x += TexCoords[Index1].x < 0.5f ? 1.0f : -1.0f;

					if (u23 > 0.5f)
						TexCoords[Index2].x += TexCoords[Index2].x < 0.5f ? 1.0f : -1.0f;
				}
			}
		};

		CheckTexCoordsWrap(QuadIndices[0], QuadIndices[1], QuadIndices[2]);

		if (LineType == 4)
			CheckTexCoordsWrap(QuadIndices[2], QuadIndices[3], QuadIndices[0]);
	}

	if (TextureMap.Type == lcMeshLoaderTextureMapType::Spherical)
	{
		auto CheckTexCoordsPole = [&TexCoords, &Vertices, &TextureMap](int Index1, int Index2, int Index3)
		{
			const lcVector4& FrontPlane = TextureMap.Params.Spherical.FrontPlane;
			const lcVector4& Plane2 = TextureMap.Params.Spherical.Plane2;
			int PoleIndex;
			int EdgeIndex1, EdgeIndex2;

			if (fabsf(lcDot(lcVector4(Vertices[Index1], 1.0f), FrontPlane)) < 0.01f && fabsf(lcDot(lcVector4(Vertices[Index1], 1.0f), Plane2)) < 0.01f)
			{
				PoleIndex = Index1;
				EdgeIndex1 = Index2;
				EdgeIndex2 = Index3;
			}
			else if (fabsf(lcDot(lcVector4(Vertices[Index2], 1.0f), FrontPlane)) < 0.01f && fabsf(lcDot(lcVector4(Vertices[Index2], 1.0f), Plane2)) < 0.01f)
			{
				PoleIndex = Index2;
				EdgeIndex1 = Index1;
				EdgeIndex2 = Index3;
			}
			else if (fabsf(lcDot(lcVector4(Vertices[Index3], 1.0f), FrontPlane)) < 0.01f && fabsf(lcDot(lcVector4(Vertices[Index3], 1.0f), Plane2)) < 0.01f)
			{
				PoleIndex = Index3;
				EdgeIndex1 = Index1;
				EdgeIndex2 = Index2;
			}
			else
				return;

			lcVector3 OppositeEdge = Vertices[EdgeIndex2] - Vertices[EdgeIndex1];
			lcVector3 SideEdge = Vertices[PoleIndex] - Vertices[EdgeIndex1];

			float OppositeLength = lcLength(OppositeEdge);
			float Projection = lcDot(OppositeEdge, SideEdge) / (OppositeLength * OppositeLength);

			TexCoords[PoleIndex].x = TexCoords[EdgeIndex1].x + (TexCoords[EdgeIndex2].x - TexCoords[EdgeIndex1].x) * Projection;
		};

		CheckTexCoordsPole(QuadIndices[0], QuadIndices[1], QuadIndices[2]);

		if (LineType == 4)
			CheckTexCoordsPole(QuadIndices[2], QuadIndices[3], QuadIndices[0]);
	}

	for (int IndexIdx = 0; IndexIdx < lcMin(LineType, 4); IndexIdx++)
	{
		const lcVector3& Position = Vertices[QuadIndices[IndexIdx]];
		Indices[IndexIdx] = AddTexturedVertex(Position, Normal, TexCoords[QuadIndices[IndexIdx]], Optimize);
	}

	if (LineType == 4)
	{
		if (Indices[0] != Indices[2] && Indices[0] != Indices[3] && Indices[2] != Indices[3])
		{
			if (WindingCCW)
			{
				Section->mIndices.Add(Indices[2]);
				Section->mIndices.Add(Indices[3]);
				Section->mIndices.Add(Indices[0]);
			}
			else
			{
				Section->mIndices.Add(Indices[0]);
				Section->mIndices.Add(Indices[3]);
				Section->mIndices.Add(Indices[2]);
			}
		}
	}

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

void lcMeshLoaderTypeData::AddMeshData(const lcMeshLoaderTypeData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcMeshLoaderTextureMap* TextureMap)
{
	const lcArray<lcMeshLoaderVertex>& DataVertices = Data.mVertices;
	lcArray<quint32> IndexRemap(DataVertices.GetSize());

	if (!TextureMap)
	{
		mVertices.AllocGrow(DataVertices.GetSize());

		for (const lcMeshLoaderVertex& DataVertex : DataVertices)
		{
			lcVector3 Position = lcMul31(DataVertex.Position, Transform);
			int Index;

			if ((DataVertex.Usage & LC_LIBRARY_VERTEX_TEXTURED) == 0)
			{
				if (DataVertex.NormalWeight == 0.0f)
					Index = AddVertex(Position, true);
				else
				{
					lcVector3 Normal = lcNormalize(lcMul30(DataVertex.Normal, Transform));
					if (InvertNormals)
						Normal = -Normal;
					Index = AddVertex(Position, Normal, true);
				}
			}
			else
			{
				if (DataVertex.NormalWeight == 0.0f)
					Index = AddTexturedVertex(Position, DataVertex.TexCoord, true);
				else
				{
					lcVector3 Normal = lcNormalize(lcMul30(DataVertex.Normal, Transform));
					if (InvertNormals)
						Normal = -Normal;
					Index = AddTexturedVertex(Position, Normal, DataVertex.TexCoord, true);
				}

				mVertices[Index].Usage = DataVertex.Usage; // todo: I think this should be |=
			}

			IndexRemap.Add(Index);
		}
	}
	else
	{
		for (const lcMeshLoaderVertex& DataVertex : DataVertices)
		{
			lcVector3 Position = lcMul31(DataVertex.Position, Transform);
			lcVector2 TexCoord = lcCalculateTexCoord(Position, TextureMap);
			int Index;

			if (DataVertex.NormalWeight == 0.0f)
				Index = AddTexturedVertex(Position, TexCoord, true);
			else
			{
				lcVector3 Normal = lcNormalize(lcMul30(DataVertex.Normal, Transform));
				if (InvertNormals)
					Normal = -Normal;
				Index = AddTexturedVertex(Position, Normal, TexCoord, true);
			}

			IndexRemap.Add(Index);
		}
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

		int Index = AddConditionalVertex(Position);
		ConditionalRemap.Add(Index);
	}

	for (const std::unique_ptr<lcLibraryMeshSection>& SrcSection : Data.mSections)
	{
		quint32 ColorCode = SrcSection->mColor == 16 ? CurrentColorCode : SrcSection->mColor;
		lcTexture* Texture;
		lcMeshPrimitiveType PrimitiveType = SrcSection->mPrimitiveType;

		if (SrcSection->mTexture)
			Texture = SrcSection->mTexture;
		else if (TextureMap && SrcSection->mPrimitiveType == LC_MESH_TRIANGLES)
		{
			Texture = TextureMap->Texture;
			PrimitiveType = LC_MESH_TEXTURED_TRIANGLES;
		}
		else
			Texture = nullptr;

		lcLibraryMeshSection* DstSection = AddSection(PrimitiveType, ColorCode, Texture);
		DstSection->mIndices.AllocGrow(SrcSection->mIndices.GetSize());

		if (PrimitiveType == LC_MESH_CONDITIONAL_LINES)
		{
			for (quint32 Index : SrcSection->mIndices)
				DstSection->mIndices.Add(ConditionalRemap[Index]);
		}
		else if (!InvertWinding || (PrimitiveType == LC_MESH_LINES))
		{
			for (quint32 Index : SrcSection->mIndices)
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

	if (!TextureMap)
	{
		BaseIndex = mVertices.GetSize();

		mVertices.SetGrow(lcMin(mVertices.GetSize(), 8 * 1024 * 1024));
		mVertices.AllocGrow(DataVertices.GetSize());

		for (int SrcVertexIdx = 0; SrcVertexIdx < DataVertices.GetSize(); SrcVertexIdx++)
		{
			const lcMeshLoaderVertex& SrcVertex = DataVertices[SrcVertexIdx];
			lcMeshLoaderVertex& DstVertex = mVertices.Add();
			DstVertex.Position = lcMul31(SrcVertex.Position, Transform);
			DstVertex.Normal = lcNormalize(lcMul30(SrcVertex.Normal, Transform));
			if (InvertNormals)
				DstVertex.Normal = -DstVertex.Normal;
			DstVertex.NormalWeight = SrcVertex.NormalWeight;
			DstVertex.TexCoord = SrcVertex.TexCoord;
			DstVertex.Usage = SrcVertex.Usage;
		}
	}
	else
	{
		BaseIndex = mVertices.GetSize();

		mVertices.AllocGrow(DataVertices.GetSize());

		for (int SrcVertexIdx = 0; SrcVertexIdx < DataVertices.GetSize(); SrcVertexIdx++)
		{
			const lcMeshLoaderVertex& SrcVertex = DataVertices[SrcVertexIdx];
			lcMeshLoaderVertex& DstVertex = mVertices.Add();

			lcVector3 Position = lcMul31(SrcVertex.Position, Transform);
			lcVector2 TexCoord = lcCalculateTexCoord(Position, TextureMap);

			DstVertex.Position = Position;
			DstVertex.Normal = lcNormalize(lcMul30(SrcVertex.Normal, Transform));
			if (InvertNormals)
				DstVertex.Normal = -DstVertex.Normal;
			DstVertex.NormalWeight = SrcVertex.NormalWeight;
			DstVertex.TexCoord = TexCoord;
			DstVertex.Usage = LC_LIBRARY_VERTEX_TEXTURED;
		}
	}

	mConditionalVertices.AllocGrow(Data.mConditionalVertices.GetSize());
	quint32 BaseConditional = mConditionalVertices.GetSize();

	for (const lcMeshLoaderConditionalVertex& DataVertex : Data.mConditionalVertices)
	{
		lcMeshLoaderConditionalVertex& Vertex = mConditionalVertices.Add();

		Vertex.Position[0] = lcMul31(DataVertex.Position[0], Transform);
		Vertex.Position[1] = lcMul31(DataVertex.Position[1], Transform);
		Vertex.Position[2] = lcMul31(DataVertex.Position[2], Transform);
		Vertex.Position[3] = lcMul31(DataVertex.Position[3], Transform);
	}

	for (const std::unique_ptr<lcLibraryMeshSection>& SrcSection : Data.mSections)
	{
		quint32 ColorCode = SrcSection->mColor == 16 ? CurrentColorCode : SrcSection->mColor;
		lcTexture* Texture;
		lcMeshPrimitiveType PrimitiveType = SrcSection->mPrimitiveType;

		if (SrcSection->mTexture)
			Texture = SrcSection->mTexture;
		else if (TextureMap && SrcSection->mPrimitiveType == LC_MESH_TRIANGLES)
		{
			Texture = TextureMap->Texture;
			PrimitiveType = LC_MESH_TEXTURED_TRIANGLES;
		}
		else
			Texture = nullptr;

		lcLibraryMeshSection* DstSection = AddSection(SrcSection->mPrimitiveType, ColorCode, Texture);
		DstSection->mIndices.SetGrow(lcMin(DstSection->mIndices.GetSize(), 8 * 1024 * 1024));
		DstSection->mIndices.AllocGrow(SrcSection->mIndices.GetSize());

		if (PrimitiveType == LC_MESH_CONDITIONAL_LINES)
		{
			for (quint32 Index : SrcSection->mIndices)
				DstSection->mIndices.Add(BaseConditional + Index);
		}
		else if (!InvertWinding || (PrimitiveType == LC_MESH_LINES))
		{
			for (quint32 Index : SrcSection->mIndices)
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
	lcLibraryMeshSection* Section = mData[MeshDataType].AddSection(PrimitiveType, ColorCode, nullptr);
	lcArray<quint32>& Indices = Section->mIndices;
	int CurrentSize = Indices.GetSize();

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

struct lcMergeSection
{
	lcLibraryMeshSection* Shared;
	lcLibraryMeshSection* Lod;
};

static bool lcLibraryMeshSectionCompare(const lcMergeSection& First, const lcMergeSection& Second)
{
	lcLibraryMeshSection* a = First.Lod ? First.Lod : First.Shared;
	lcLibraryMeshSection* b = Second.Lod ? Second.Lod : Second.Shared;

	if (a->mPrimitiveType != b->mPrimitiveType)
	{
		int PrimitiveOrder[LC_MESH_NUM_PRIMITIVE_TYPES] =
		{
			LC_MESH_TRIANGLES,
			LC_MESH_TEXTURED_TRIANGLES,
			LC_MESH_LINES,
			LC_MESH_CONDITIONAL_LINES
		};

		for (int PrimitiveType = 0; PrimitiveType < LC_MESH_NUM_PRIMITIVE_TYPES; PrimitiveType++)
		{
			int Primitive = PrimitiveOrder[PrimitiveType];

			if (a->mPrimitiveType == Primitive)
				return true;

			if (b->mPrimitiveType == Primitive)
				return false;
		}
	}

	bool TranslucentA = lcIsColorTranslucent(a->mColor);
	bool TranslucentB = lcIsColorTranslucent(b->mColor);

	if (TranslucentA != TranslucentB)
		return !TranslucentA;

	return a->mColor > b->mColor;
}

lcMesh* lcLibraryMeshData::CreateMesh()
{
	lcMesh* Mesh = new lcMesh();

	int BaseVertices[LC_NUM_MESHDATA_TYPES];
	int BaseTexturedVertices[LC_NUM_MESHDATA_TYPES];
	int BaseConditionalVertices[LC_NUM_MESHDATA_TYPES];
	int NumVertices = 0;
	int NumTexturedVertices = 0;
	int ConditionalVertexCount = 0;
	std::vector<quint32> IndexRemap[LC_NUM_MESHDATA_TYPES];
	std::vector<quint32> TexturedIndexRemap[LC_NUM_MESHDATA_TYPES];

	if (!mHasTextures)
	{
		for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
		{
			std::vector<std::unique_ptr<lcLibraryMeshSection>>& Sections = mData[MeshDataIdx].mSections;

			for (const std::unique_ptr<lcLibraryMeshSection>& Section : Sections)
				Section->mColor = lcGetColorIndex(Section->mColor);

			BaseVertices[MeshDataIdx] = NumVertices;
			NumVertices += mData[MeshDataIdx].mVertices.GetSize();
			BaseConditionalVertices[MeshDataIdx] = ConditionalVertexCount;
			ConditionalVertexCount += mData[MeshDataIdx].mConditionalVertices.GetSize();
		}
	}
	else
	{
		for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
		{
			std::vector<std::unique_ptr<lcLibraryMeshSection>>& Sections = mData[MeshDataIdx].mSections;

			for (const std::unique_ptr<lcLibraryMeshSection>& Section : Sections)
				Section->mColor = lcGetColorIndex(Section->mColor);

			BaseVertices[MeshDataIdx] = NumVertices;
			BaseTexturedVertices[MeshDataIdx] = NumTexturedVertices;

			const lcArray<lcMeshLoaderVertex>& Vertices = mData[MeshDataIdx].mVertices;
			IndexRemap[MeshDataIdx].resize(Vertices.GetSize());
			TexturedIndexRemap[MeshDataIdx].resize(Vertices.GetSize());

			for (int VertexIdx = 0; VertexIdx < Vertices.GetSize(); VertexIdx++)
			{
				const lcMeshLoaderVertex& Vertex = Vertices[VertexIdx];

				if (Vertex.Usage & LC_LIBRARY_VERTEX_UNTEXTURED)
				{
					IndexRemap[MeshDataIdx][VertexIdx] = NumVertices;
					NumVertices++;
				}

				if (Vertex.Usage & LC_LIBRARY_VERTEX_TEXTURED)
				{
					TexturedIndexRemap[MeshDataIdx][VertexIdx] = NumTexturedVertices;
					NumTexturedVertices++;
				}
			}

			BaseConditionalVertices[MeshDataIdx] = ConditionalVertexCount;
			ConditionalVertexCount += mData[MeshDataIdx].mConditionalVertices.GetSize();
		}
	}

	quint16 NumSections[LC_NUM_MESH_LODS];
	int NumIndices = 0;

	lcArray<lcMergeSection> MergeSections[LC_NUM_MESH_LODS];

	for (int LodIdx = 0; LodIdx < LC_NUM_MESH_LODS; LodIdx++)
	{
		std::vector<std::unique_ptr<lcLibraryMeshSection>>& SharedSections = mData[LC_MESHDATA_SHARED].mSections;
		std::vector<std::unique_ptr<lcLibraryMeshSection>>& Sections = mData[LodIdx].mSections;

		for (std::unique_ptr<lcLibraryMeshSection>& SharedSection : SharedSections)
		{
			NumIndices += SharedSection->mIndices.GetSize();

			lcMergeSection& MergeSection = MergeSections[LodIdx].Add();
			MergeSection.Shared = SharedSection.get();
			MergeSection.Lod = nullptr;
		}

		for (std::unique_ptr<lcLibraryMeshSection>& Section : Sections)
		{
			bool Found = false;

			NumIndices += Section->mIndices.GetSize();

			for (int SharedSectionIdx = 0; SharedSectionIdx < (int)SharedSections.size(); SharedSectionIdx++)
			{
				lcLibraryMeshSection* SharedSection = SharedSections[SharedSectionIdx].get();

				if (SharedSection->mColor == Section->mColor && SharedSection->mPrimitiveType == Section->mPrimitiveType && SharedSection->mTexture == Section->mTexture)
				{
					lcMergeSection& MergeSection = MergeSections[LodIdx][SharedSectionIdx];
					MergeSection.Lod = Section.get();
					Found = true;
					break;
				}
			}

			if (!Found)
			{
				lcMergeSection& MergeSection = MergeSections[LodIdx].Add();
				MergeSection.Shared = nullptr;
				MergeSection.Lod = Section.get();
			}
		}

		NumSections[LodIdx] = MergeSections[LodIdx].GetSize();
		std::sort(MergeSections[LodIdx].begin(), MergeSections[LodIdx].end(), lcLibraryMeshSectionCompare);
	}

	Mesh->Create(NumSections, NumVertices, NumTexturedVertices, ConditionalVertexCount, NumIndices);

	lcVertex* DstVerts = (lcVertex*)Mesh->mVertexData;

	if (!mHasTextures)
	{
		for (lcMeshLoaderTypeData& Data : mData)
		{
			for (const lcMeshLoaderVertex& SrcVertex : Data.mVertices)
			{
				lcVertex& DstVertex = *DstVerts++;

				DstVertex.Position = lcVector3LDrawToLeoCAD(SrcVertex.Position);
				DstVertex.Normal = lcPackNormal(lcVector3LDrawToLeoCAD(SrcVertex.Normal));
			}
		}

		lcVertexConditional* DstConditionalVerts = (lcVertexConditional*)DstVerts;

		for (lcMeshLoaderTypeData& Data : mData)
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
	}
	else
	{
		for (lcMeshLoaderTypeData& Data : mData)
		{
			for (const lcMeshLoaderVertex& SrcVertex : Data.mVertices)
			{
				if ((SrcVertex.Usage & LC_LIBRARY_VERTEX_UNTEXTURED) == 0)
					continue;

				lcVertex& DstVertex = *DstVerts++;

				DstVertex.Position = lcVector3LDrawToLeoCAD(SrcVertex.Position);
				DstVertex.Normal = lcPackNormal(lcVector3LDrawToLeoCAD(SrcVertex.Normal));
			}
		}

		lcVertexTextured* DstTexturedVerts = (lcVertexTextured*)DstVerts;

		for (lcMeshLoaderTypeData& Data : mData)
		{
			for (const lcMeshLoaderVertex& SrcVertex : Data.mVertices)
			{
				if ((SrcVertex.Usage & LC_LIBRARY_VERTEX_TEXTURED) == 0)
					continue;

				lcVertexTextured& DstVertex = *DstTexturedVerts++;

				DstVertex.Position = lcVector3LDrawToLeoCAD(SrcVertex.Position);
				DstVertex.Normal = lcPackNormal(lcVector3LDrawToLeoCAD(SrcVertex.Normal));
				DstVertex.TexCoord = SrcVertex.TexCoord;
			}
		}

		for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
		{
			for (const std::unique_ptr<lcLibraryMeshSection>& Section : mData[MeshDataIdx].mSections)
			{
				if (Section->mPrimitiveType == LC_MESH_TRIANGLES)
				{
					for (quint32& Index : Section->mIndices)
						Index = IndexRemap[MeshDataIdx][Index];
				}
				else
				{
					if (!Section->mTexture)
					{
						for (quint32& Index : Section->mIndices)
							Index = IndexRemap[MeshDataIdx][Index];
					}
					else
					{
						for (quint32& Index : Section->mIndices)
							Index = TexturedIndexRemap[MeshDataIdx][Index];
					}
				}
			}
		}

		lcVertexConditional* DstConditionalVerts = (lcVertexConditional*)DstTexturedVerts;

		for (lcMeshLoaderTypeData& Data : mData)
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
	}

	NumIndices = 0;

	for (int LodIdx = 0; LodIdx < LC_NUM_MESH_LODS; LodIdx++)
	{
		for (int SectionIdx = 0; SectionIdx < MergeSections[LodIdx].GetSize(); SectionIdx++)
		{
			lcMergeSection& MergeSection = MergeSections[LodIdx][SectionIdx];
			lcMeshSection& DstSection = Mesh->mLods[LodIdx].Sections[SectionIdx];

			lcLibraryMeshSection* SetupSection = MergeSection.Shared ? MergeSection.Shared : MergeSection.Lod;

			DstSection.ColorIndex = SetupSection->mColor;
			DstSection.PrimitiveType = SetupSection->mPrimitiveType;
			DstSection.NumIndices = 0;
			DstSection.Texture = SetupSection->mTexture;

			if (DstSection.Texture)
				DstSection.Texture->AddRef();

			if (Mesh->mNumVertices < 0x10000)
			{
				DstSection.IndexOffset = NumIndices * 2;

				quint16* Index = (quint16*)Mesh->mIndexData + NumIndices;

				if (MergeSection.Shared)
				{
					lcLibraryMeshSection* SrcSection = MergeSection.Shared;

					if (DstSection.PrimitiveType != LC_MESH_CONDITIONAL_LINES)
					{
						if (!mHasTextures)
						{
							quint16 BaseVertex = DstSection.Texture ? BaseTexturedVertices[LC_MESHDATA_SHARED] : BaseVertices[LC_MESHDATA_SHARED];

							for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
								*Index++ = BaseVertex + SrcSection->mIndices[IndexIdx];
						}
						else
							for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
								*Index++ = SrcSection->mIndices[IndexIdx];
					}
					else
					{
						quint16 BaseVertex = BaseConditionalVertices[LC_MESHDATA_SHARED];

						for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
							*Index++ = BaseVertex + SrcSection->mIndices[IndexIdx];
					}

					DstSection.NumIndices += SrcSection->mIndices.GetSize();
				}

				if (MergeSection.Lod)
				{
					lcLibraryMeshSection* SrcSection = MergeSection.Lod;

					if (DstSection.PrimitiveType != LC_MESH_CONDITIONAL_LINES)
					{
						if (!mHasTextures)
						{
							quint16 BaseVertex = DstSection.Texture ? BaseTexturedVertices[LodIdx] : BaseVertices[LodIdx];

							for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
								*Index++ = BaseVertex + SrcSection->mIndices[IndexIdx];
						}
						else
							for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
								*Index++ = SrcSection->mIndices[IndexIdx];
					}
					else
					{
						quint16 BaseVertex = BaseConditionalVertices[LodIdx];

						for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
							*Index++ = BaseVertex + SrcSection->mIndices[IndexIdx];
					}

					DstSection.NumIndices += SrcSection->mIndices.GetSize();
				}
			}
			else
			{
				DstSection.IndexOffset = NumIndices * 4;

				quint32* Index = (quint32*)Mesh->mIndexData + NumIndices;

				if (MergeSection.Shared)
				{
					lcLibraryMeshSection* SrcSection = MergeSection.Shared;

					if (DstSection.PrimitiveType != LC_MESH_CONDITIONAL_LINES)
					{
						if (!mHasTextures)
						{
							quint32 BaseVertex = DstSection.Texture ? BaseTexturedVertices[LC_MESHDATA_SHARED] : BaseVertices[LC_MESHDATA_SHARED];

							for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
								*Index++ = BaseVertex + SrcSection->mIndices[IndexIdx];
						}
						else
							for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
								*Index++ = SrcSection->mIndices[IndexIdx];
					}
					else
					{
						quint32 BaseVertex = BaseConditionalVertices[LC_MESHDATA_SHARED];

						for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
							*Index++ = BaseVertex + SrcSection->mIndices[IndexIdx];
					}

					DstSection.NumIndices += SrcSection->mIndices.GetSize();
				}

				if (MergeSection.Lod)
				{
					lcLibraryMeshSection* SrcSection = MergeSection.Lod;

					if (DstSection.PrimitiveType != LC_MESH_CONDITIONAL_LINES)
					{
						if (!mHasTextures)
						{
							quint32 BaseVertex = DstSection.Texture ? BaseTexturedVertices[LodIdx] : BaseVertices[LodIdx];

							for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
								*Index++ = BaseVertex + SrcSection->mIndices[IndexIdx];

							DstSection.NumIndices += SrcSection->mIndices.GetSize();
						}
						else
							for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
								*Index++ = SrcSection->mIndices[IndexIdx];
					}
					else
					{
						quint16 BaseVertex = BaseConditionalVertices[LodIdx];

						for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
							*Index++ = BaseVertex + SrcSection->mIndices[IndexIdx];
					}

					DstSection.NumIndices += SrcSection->mIndices.GetSize();
				}
			}

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

	if (mHasStyleStud)
		Mesh->mFlags |= lcMeshFlag::HasStyleStud;

	lcVector3 MeshMin(FLT_MAX, FLT_MAX, FLT_MAX), MeshMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	bool UpdatedBoundingBox = false;

	for (int LodIdx = 0; LodIdx < LC_NUM_MESH_LODS; LodIdx++)
	{
		lcMeshLod& Lod = Mesh->mLods[LodIdx];

		for (int SectionIdx = 0; SectionIdx < Lod.NumSections; SectionIdx++)
		{
			lcMeshSection& Section = Lod.Sections[SectionIdx];
			lcVector3 SectionMin(FLT_MAX, FLT_MAX, FLT_MAX), SectionMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

			if (Mesh->mNumVertices < 0x10000)
			{
				const quint16* IndexBuffer = static_cast<quint16*>(Mesh->mIndexData) + Section.IndexOffset / 2;

				if (!Section.Texture)
				{
					if (Section.PrimitiveType != LC_MESH_CONDITIONAL_LINES)
					{
						const lcVertex* VertexBuffer = static_cast<lcVertex*>(Mesh->mVertexData);

						for (int Index = 0; Index < Section.NumIndices; Index++)
						{
							const lcVector3& Position = VertexBuffer[IndexBuffer[Index]].Position;
							SectionMin = lcMin(SectionMin, Position);
							SectionMax = lcMax(SectionMax, Position);
						}
					}
					else
					{
						const lcVertexConditional* VertexBuffer = reinterpret_cast<lcVertexConditional*>(static_cast<char*>(Mesh->mVertexData) + Mesh->mNumVertices * sizeof(lcVertex) + Mesh->mNumTexturedVertices * sizeof(lcVertexTextured));

						for (int Index = 0; Index < Section.NumIndices; Index++)
						{
							const lcVector3& Position = VertexBuffer[IndexBuffer[Index]].Position1;
							SectionMin = lcMin(SectionMin, Position);
							SectionMax = lcMax(SectionMax, Position);
						}
					}
				}
				else
				{
					const lcVertexTextured* VertexBuffer = reinterpret_cast<lcVertexTextured*>(static_cast<char*>(Mesh->mVertexData) + Mesh->mNumVertices * sizeof(lcVertex));

					for (int Index = 0; Index < Section.NumIndices; Index++)
					{
						const lcVector3& Position = VertexBuffer[IndexBuffer[Index]].Position;
						SectionMin = lcMin(SectionMin, Position);
						SectionMax = lcMax(SectionMax, Position);
					}
				}
			}
			else
			{
				const quint32* IndexBuffer = static_cast<quint32*>(Mesh->mIndexData) + Section.IndexOffset / 4;

				if (!Section.Texture)
				{
					if (Section.PrimitiveType != LC_MESH_CONDITIONAL_LINES)
					{
						const lcVertex* VertexBuffer = static_cast<lcVertex*>(Mesh->mVertexData);

						for (int Index = 0; Index < Section.NumIndices; Index++)
						{
							const lcVector3& Position = VertexBuffer[IndexBuffer[Index]].Position;
							SectionMin = lcMin(SectionMin, Position);
							SectionMax = lcMax(SectionMax, Position);
						}
					}
					else
					{
						const lcVertexConditional* VertexBuffer = reinterpret_cast<lcVertexConditional*>(static_cast<char*>(Mesh->mVertexData) + Mesh->mNumVertices * sizeof(lcVertex) + Mesh->mNumTexturedVertices * sizeof(lcVertexTextured));

						for (int Index = 0; Index < Section.NumIndices; Index++)
						{
							const lcVector3& Position = VertexBuffer[IndexBuffer[Index]].Position1;
							SectionMin = lcMin(SectionMin, Position);
							SectionMax = lcMax(SectionMax, Position);
						}
					}
				}
				else
				{
					const lcVertexTextured* VertexBuffer = static_cast<lcVertexTextured*>(Mesh->mVertexData);

					for (int Index = 0; Index < Section.NumIndices; Index++)
					{
						const lcVector3& Position = VertexBuffer[IndexBuffer[Index]].Position;
						SectionMin = lcMin(SectionMin, Position);
						SectionMax = lcMax(SectionMax, Position);
					}
				}
			}

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

	return Mesh;
}

lcMeshLoader::lcMeshLoader(lcLibraryMeshData& MeshData, bool Optimize, Project* CurrentProject, bool SearchProjectFolder)
	: mMeshData(MeshData), mOptimize(Optimize), mCurrentProject(CurrentProject), mSearchProjectFolder(SearchProjectFolder)
{
}

bool lcMeshLoader::LoadMesh(lcFile& File, lcMeshDataType MeshDataType)
{
	lcArray<lcMeshLoaderTextureMap> TextureStack;

	return ReadMeshData(File, lcMatrix44Identity(), 16, false, TextureStack, MeshDataType);
}

bool lcMeshLoader::ReadMeshData(lcFile& File, const lcMatrix44& CurrentTransform, quint32 CurrentColorCode, bool InvertWinding, lcArray<lcMeshLoaderTextureMap>& TextureStack, lcMeshDataType MeshDataType)
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

						char FileName[LC_MAXPATH];
						lcVector3 Points[3];

						sscanf(Token, "%f %f %f %f %f %f %f %f %f %s", &Points[0].x, &Points[0].y, &Points[0].z, &Points[1].x, &Points[1].y, &Points[1].z, &Points[2].x, &Points[2].y, &Points[2].z, FileName);

						Points[0] = lcMul31(Points[0], CurrentTransform);
						Points[1] = lcMul31(Points[1], CurrentTransform);
						Points[2] = lcMul31(Points[2], CurrentTransform);

						CleanTextureName(FileName);

						lcMeshLoaderTextureMap& Map = TextureStack.Add();
						Map.Next = false;
						Map.Fallback = false;
						Map.Texture = Library->FindTexture(FileName, mCurrentProject, mSearchProjectFolder);
						Map.Type = lcMeshLoaderTextureMapType::Planar;

						for (int EdgeIdx = 0; EdgeIdx < 2; EdgeIdx++)
						{
							lcVector3 Normal = Points[EdgeIdx + 1] - Points[0];
							float Length = lcLength(Normal);
							Normal /= Length;

							Map.Params.Planar.Planes[EdgeIdx].x = Normal.x / Length;
							Map.Params.Planar.Planes[EdgeIdx].y = Normal.y / Length;
							Map.Params.Planar.Planes[EdgeIdx].z = Normal.z / Length;
							Map.Params.Planar.Planes[EdgeIdx].w = -lcDot(Normal, Points[0]) / Length;
						}
					}
					else if (!strcmp(Token, "CYLINDRICAL"))
					{
						Token += 12;

						char FileName[LC_MAXPATH];
						lcVector3 Points[3];
						float Angle;

						sscanf(Token, "%f %f %f %f %f %f %f %f %f %f %s", &Points[0].x, &Points[0].y, &Points[0].z, &Points[1].x, &Points[1].y, &Points[1].z, &Points[2].x, &Points[2].y, &Points[2].z, &Angle, FileName);

						Points[0] = lcMul31(Points[0], CurrentTransform);
						Points[1] = lcMul31(Points[1], CurrentTransform);
						Points[2] = lcMul31(Points[2], CurrentTransform);

						CleanTextureName(FileName);

						lcMeshLoaderTextureMap& Map = TextureStack.Add();
						Map.Next = false;
						Map.Fallback = false;
						Map.Texture = Library->FindTexture(FileName, mCurrentProject, mSearchProjectFolder);

						Map.Type = lcMeshLoaderTextureMapType::Cylindrical;
						lcVector3 Up = Points[0] - Points[1];
						float UpLength = lcLength(Up);
						lcVector3 Front = lcNormalize(Points[2] - Points[1]);
						lcVector3 Plane1Normal = Up / UpLength;
						lcVector3 Plane2Normal = lcNormalize(lcCross(Front, Up));
						Map.Params.Cylindrical.FrontPlane = lcVector4(Front, -lcDot(Front, Points[1]));
						Map.Params.Cylindrical.UpLength = UpLength;
						Map.Params.Cylindrical.Plane1 = lcVector4(Plane1Normal, -lcDot(Plane1Normal, Points[1]));
						Map.Params.Cylindrical.Plane2 = lcVector4(Plane2Normal, -lcDot(Plane2Normal, Points[1]));
						Map.Angle1 = 360.0f / Angle;
					}
					else if (!strcmp(Token, "SPHERICAL"))
					{
						Token += 10;

						char FileName[LC_MAXPATH];
						lcVector3 Points[3];
						float Angle1, Angle2;

						sscanf(Token, "%f %f %f %f %f %f %f %f %f %f %f %s", &Points[0].x, &Points[0].y, &Points[0].z, &Points[1].x, &Points[1].y, &Points[1].z, &Points[2].x, &Points[2].y, &Points[2].z, &Angle1, &Angle2, FileName);

						Points[0] = lcMul31(Points[0], CurrentTransform);
						Points[1] = lcMul31(Points[1], CurrentTransform);
						Points[2] = lcMul31(Points[2], CurrentTransform);

						CleanTextureName(FileName);

						lcMeshLoaderTextureMap& Map = TextureStack.Add();
						Map.Next = false;
						Map.Fallback = false;
						Map.Texture = Library->FindTexture(FileName, mCurrentProject, mSearchProjectFolder);
						Map.Type = lcMeshLoaderTextureMapType::Spherical;

						lcVector3 Front = lcNormalize(Points[1] - Points[0]);
						lcVector3 Plane1Normal = lcNormalize(lcCross(Front, Points[2] - Points[0]));
						lcVector3 Plane2Normal = lcNormalize(lcCross(Plane1Normal, Front));
						Map.Params.Spherical.FrontPlane = lcVector4(Front, -lcDot(Front, Points[0]));
						Map.Params.Spherical.Center = Points[0];
						Map.Params.Spherical.Plane1 = lcVector4(Plane1Normal, -lcDot(Plane1Normal, Points[0]));
						Map.Params.Spherical.Plane2 = lcVector4(Plane2Normal, -lcDot(Plane2Normal, Points[0]));
						Map.Angle1 = 360.0f / Angle1;
						Map.Angle2 = 180.0f / Angle2;
					}
				}
				else if (!strcmp(Token, "FALLBACK"))
				{
					if (TextureStack.GetSize())
						TextureStack[TextureStack.GetSize() - 1].Fallback = true;
				}
				else if (!strcmp(Token, "END"))
				{
					if (TextureStack.GetSize())
						TextureStack.RemoveIndex(TextureStack.GetSize() - 1);
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

				if (!TextureStack.GetSize())
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

		if (TextureStack.GetSize())
		{
			TextureMap = &TextureStack[TextureStack.GetSize() - 1];

			if (TextureMap->Texture)
			{
				if (TextureMap->Fallback)
					continue;
			}
			else
			{
				if (!TextureMap->Fallback)
					continue;

				TextureMap = nullptr;
			}
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

			auto FileCallback = [this, &IncludeTransform, &ColorCode, &Mirror, &InvertNext, &TextureStack, &MeshDataType](lcFile& File)
			{
				ReadMeshData(File, IncludeTransform, ColorCode, Mirror ^ InvertNext, TextureStack, MeshDataType);
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

			mMeshData.mData[MeshDataType].ProcessLine(LineType, ColorCode, WindingCCW, Points, mOptimize);
			break;

		case 3:
			sscanf(Line, "%d %i %f %f %f %f %f %f %f %f %f", &LineType, &Dummy, &Points[0].x, &Points[0].y, &Points[0].z,
				   &Points[1].x, &Points[1].y, &Points[1].z, &Points[2].x, &Points[2].y, &Points[2].z);

			Points[0] = lcMul31(Points[0], CurrentTransform);
			Points[1] = lcMul31(Points[1], CurrentTransform);
			Points[2] = lcMul31(Points[2], CurrentTransform);

			if (TextureMap)
			{
				mMeshData.mHasTextures = true;
				mMeshData.mData[MeshDataType].ProcessTexturedLine(LineType, ColorCode, WindingCCW, *TextureMap, Points, mOptimize);

				if (TextureMap->Next)
					TextureStack.RemoveIndex(TextureStack.GetSize() - 1);
			}
			else
				mMeshData.mData[MeshDataType].ProcessLine(LineType, ColorCode, WindingCCW, Points, mOptimize);
			break;

		case 4:
			sscanf(Line, "%d %i %f %f %f %f %f %f %f %f %f %f %f %f", &LineType, &Dummy, &Points[0].x, &Points[0].y, &Points[0].z,
				   &Points[1].x, &Points[1].y, &Points[1].z, &Points[2].x, &Points[2].y, &Points[2].z, &Points[3].x, &Points[3].y, &Points[3].z);

			Points[0] = lcMul31(Points[0], CurrentTransform);
			Points[1] = lcMul31(Points[1], CurrentTransform);
			Points[2] = lcMul31(Points[2], CurrentTransform);
			Points[3] = lcMul31(Points[3], CurrentTransform);

			if (TextureMap)
			{
				mMeshData.mHasTextures = true;
				mMeshData.mData[MeshDataType].ProcessTexturedLine(LineType, ColorCode, WindingCCW, *TextureMap, Points, mOptimize);

				if (TextureMap->Next)
					TextureStack.RemoveIndex(TextureStack.GetSize() - 1);
			}
			else
				mMeshData.mData[MeshDataType].ProcessLine(LineType, ColorCode, WindingCCW, Points, mOptimize);
			break;

		case 5:
			sscanf(Line, "%d %i %f %f %f %f %f %f %f %f %f %f %f %f", &LineType, &Dummy, &Points[0].x, &Points[0].y, &Points[0].z,
				   &Points[1].x, &Points[1].y, &Points[1].z, &Points[2].x, &Points[2].y, &Points[2].z, &Points[3].x, &Points[3].y, &Points[3].z);

			Points[0] = lcMul31(Points[0], CurrentTransform);
			Points[1] = lcMul31(Points[1], CurrentTransform);
			Points[2] = lcMul31(Points[2], CurrentTransform);
			Points[3] = lcMul31(Points[3], CurrentTransform);

			mMeshData.mData[MeshDataType].ProcessLine(LineType, ColorCode, WindingCCW, Points, mOptimize);
			break;
		}

		InvertNext = false;
	}

	return true;
}
