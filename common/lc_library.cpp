#include "lc_global.h"
#include "lc_library.h"
#include "lc_zipfile.h"
#include "lc_file.h"
#include "pieceinf.h"
#include "lc_colors.h"

lcPiecesLibrary::lcPiecesLibrary()
{
	mZipFile = NULL;
}

lcPiecesLibrary::~lcPiecesLibrary()
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		delete mPieces[PieceIdx];

	delete mZipFile;
}

bool lcPiecesLibrary::OpenArchive(const char* FileName)
{
	mZipFile = new lcZipFile();

	if (!mZipFile->Open(FileName))
	{
		delete mZipFile;
		mZipFile = NULL;
		return false;
	}

	for (int FileIdx = 0; FileIdx < mZipFile->mFiles.GetSize(); FileIdx++)
	{
		lcZipFileInfo& FileInfo = mZipFile->mFiles[FileIdx];
		char Name[LC_PIECE_NAME_LEN];

		const char* Src = FileInfo.file_name;
		char* Dst = Name;

		while (*Src && Dst - Name < LC_PIECE_NAME_LEN)
		{
			if (*Src >= 'a' && *Src <= 'z')
				*Dst = *Src + 'A' - 'a';
			else if (*Src == '\\')
				*Dst = '/';
			else
				*Dst = *Src;

			Src++;
			Dst++;
		}

		if (Dst - Name <= 4)
			continue;

		Dst -= 4;
		if (memcmp(Dst, ".DAT", 4))
			continue;
		*Dst = 0;

		if (memcmp(Name, "LDRAW/", 6))
			continue;

		if (!memcmp(Name + 6, "PARTS/", 6))
		{
			if (memcmp(Name + 12, "S/", 2))
			{
				PieceInfo* Info = new PieceInfo();
				mPieces.Add(Info);

				strncpy(Info->m_strName, Name + 12, sizeof(Info->m_strName));
				Info->m_strName[sizeof(Info->m_strName) - 1] = 0;

				Info->m_nFlags = 0;
				Info->m_nOffset = FileIdx;
			}
			else
			{
				lcLibraryPrimitive* Prim = new lcLibraryPrimitive();
				mPrimitives.Add(Prim);
				strncpy(Prim->mName, Name + 12, sizeof(Prim->mName));
				Prim->mName[sizeof(Prim->mName) - 1] = 0;
				Prim->mZipFileIndex = FileIdx;
				Prim->mLoaded = false;
				Prim->mStud = false;
				Prim->mSubFile = true;
			}
		}
		else if (!memcmp(Name + 6, "P/", 2))
		{
			lcLibraryPrimitive* Prim = new lcLibraryPrimitive();
			mPrimitives.Add(Prim);
			strncpy(Prim->mName, Name + 8, sizeof(Prim->mName));
			Prim->mName[sizeof(Prim->mName) - 1] = 0;
			Prim->mZipFileIndex = FileIdx;
			Prim->mLoaded = false;
			Prim->mStud = (memcmp(Prim->mName, "STU", 3) == 0);
			Prim->mSubFile = false;
		}
	}

	lcMemFile PieceFile;

	for (int PieceInfoIndex = 0; PieceInfoIndex < mPieces.GetSize(); PieceInfoIndex++)
	{
		PieceInfo* Info = mPieces[PieceInfoIndex];

		mZipFile->ExtractFile(Info->m_nOffset, PieceFile, 256);
		PieceFile.Seek(0, SEEK_END);
		PieceFile.WriteU8(0);

		char* Src = (char*)PieceFile.mBuffer + 2;
		char* Dst = Info->m_strDescription;

		for (;;)
		{
			if (*Src != '\r' && *Src != '\n' && *Src && Dst - Info->m_strDescription < sizeof(Info->m_strDescription) - 1)
			{
				*Dst++ = *Src++;
				continue;
			}

			*Dst = 0;
			break;
		}
	}

	return true;
}

bool lcPiecesLibrary::LoadPiece(const char* PieceName)
{
	for (int PieceInfoIndex = 0; PieceInfoIndex < mPieces.GetSize(); PieceInfoIndex++)
	{
		PieceInfo* Info = mPieces[PieceInfoIndex];

		if (!strcmp(Info->m_strName, PieceName))
			return LoadPiece(PieceInfoIndex);
	}

	return false;
}

bool lcPiecesLibrary::LoadPiece(int PieceIndex)
{
	PieceInfo* Info = mPieces[PieceIndex];
	lcMemFile PieceFile;
	lcLibraryMeshData MeshData;

	if (!mZipFile->ExtractFile(Info->m_nOffset, PieceFile))
		return false;

	if (!ReadMeshData(PieceFile, lcMatrix44Identity(), 16, MeshData))
		return false;

	lcMesh* Mesh = new lcMesh();

	int NumIndices = 0;

	for (int SectionIdx = 0; SectionIdx < MeshData.mSections.GetSize(); SectionIdx++)
		NumIndices += MeshData.mSections[SectionIdx]->mIndices.GetSize();

	Mesh->Create(MeshData.mSections.GetSize(), MeshData.mVertices.GetSize(), NumIndices);

	lcVector3* DstVerts = (lcVector3*)Mesh->mVertexBuffer.mData;
	lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (int VertexIdx = 0; VertexIdx < MeshData.mVertices.GetSize(); VertexIdx++)
	{
		const lcVector3& SrcVertex = MeshData.mVertices[VertexIdx];
		lcVector3& Vertex = *DstVerts++;
		Vertex = lcVector3(SrcVertex.x / 25.0f, SrcVertex.z / 25.0f, -SrcVertex.y / 25.0f);

		Min.x = lcMin(Min.x, Vertex.x);
		Min.y = lcMin(Min.y, Vertex.y);
		Min.z = lcMin(Min.z, Vertex.z);
		Max.x = lcMax(Max.x, Vertex.x);
		Max.y = lcMax(Max.y, Vertex.y);
		Max.z = lcMax(Max.z, Vertex.z);
	}

	Info->m_fDimensions[0] = Max.x;
	Info->m_fDimensions[1] = Max.y;
	Info->m_fDimensions[2] = Max.z;
	Info->m_fDimensions[3] = Min.x;
	Info->m_fDimensions[4] = Min.y;
	Info->m_fDimensions[5] = Min.z;

	NumIndices = 0;

	for (int SectionIdx = 0; SectionIdx < MeshData.mSections.GetSize(); SectionIdx++)
	{
		lcMeshSection& DstSection = Mesh->mSections[SectionIdx];
		lcLibraryMeshSection* SrcSection = MeshData.mSections[SectionIdx];

		DstSection.ColorIndex = lcGetColorIndex(SrcSection->mColorCode);
		DstSection.PrimitiveType = SrcSection->mTriangles ? GL_TRIANGLES : GL_LINES;
		DstSection.NumIndices = SrcSection->mIndices.GetSize();

		if (Mesh->mNumVertices < 0x10000)
		{
			DstSection.IndexOffset = NumIndices * 2;

			lcuint16* Index = (lcuint16*)Mesh->mIndexBuffer.mData + NumIndices;

			for (int IndexIdx = 0; IndexIdx < DstSection.NumIndices; IndexIdx++)
				*Index++ = SrcSection->mIndices[IndexIdx];
		}
		else
		{
			DstSection.IndexOffset = NumIndices * 4;

			lcuint32* Index = (lcuint32*)Mesh->mIndexBuffer.mData + NumIndices;

			for (int IndexIdx = 0; IndexIdx < DstSection.NumIndices; IndexIdx++)
				*Index++ = SrcSection->mIndices[IndexIdx];
		}

		if (SrcSection->mTriangles)
		{
			if (SrcSection->mColorCode == 16)
				Info->m_nFlags |= LC_PIECE_HAS_DEFAULT;
			else
			{
				if (lcIsColorTranslucent(DstSection.ColorIndex))
					Info->m_nFlags |= LC_PIECE_HAS_TRANSLUCENT;
				else
					Info->m_nFlags |= LC_PIECE_HAS_SOLID;
			}
		}
		else
			Info->m_nFlags |= LC_PIECE_HAS_LINES;

		NumIndices += DstSection.NumIndices;
	}

	Mesh->UpdateBuffers();
	Info->mMesh = Mesh;

	return true;
}

int lcPiecesLibrary::FindPrimitiveIndex(const char* Name)
{
	for (int PrimitiveIndex = 0; PrimitiveIndex < mPrimitives.GetSize(); PrimitiveIndex++)
		if (!strcmp(mPrimitives[PrimitiveIndex]->mName, Name))
			return PrimitiveIndex;

	return -1;
}

bool lcPiecesLibrary::LoadPrimitive(int PrimitiveIndex)
{
	lcLibraryPrimitive* Primitive = mPrimitives[PrimitiveIndex];
	lcMemFile File;

	if (!mZipFile->ExtractFile(Primitive->mZipFileIndex, File))
		return false;

	if (!ReadMeshData(File, lcMatrix44Identity(), 16, Primitive->mMeshData))
		return false;

	Primitive->mLoaded = true;

	return true;
}

bool lcPiecesLibrary::ReadMeshData(lcFile& File, const lcMatrix44& CurrentTransform, lcuint32 CurrentColorCode, lcLibraryMeshData& MeshData)
{
	char Line[1024];

	while (File.ReadLine(Line, sizeof(Line)))
	{
		lcuint32 ColorCode, ColorCodeHex;
		int LineType;

		if (sscanf(Line, "%d", &LineType) != 1)
			continue;

		if (LineType < 1 || LineType > 4)
			continue;

		if (sscanf(Line, "%d %d", &LineType, &ColorCode) != 2)
			continue;

		if (ColorCode == 0)
		{
			sscanf(Line, "%d %i", &LineType, &ColorCodeHex);

			if (ColorCode != ColorCodeHex)
				ColorCode = ColorCodeHex | LC_COLOR_DIRECT;
		}

		if (ColorCode == 16)
			ColorCode = CurrentColorCode;

		int Dummy;
		lcVector3 Points[4];

		switch (LineType)
		{
		case 1:
			{
				char FileName[LC_MAXPATH];
				float fm[12];

				sscanf(Line, "%d %i %f %f %f %f %f %f %f %f %f %f %f %f %s", &LineType, &Dummy, &fm[0], &fm[1], &fm[2], &fm[3], &fm[4], &fm[5], &fm[6], &fm[7], &fm[8], &fm[9], &fm[10], &fm[11], FileName);

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
					if (!memcmp(Ch, ".DAT", 4))
						*Ch = 0;
				}

				int PrimitiveIndex = FindPrimitiveIndex(FileName);
				lcMatrix44 IncludeTransform(lcVector4(fm[3], fm[6], fm[9], 0.0f), lcVector4(fm[4], fm[7], fm[10], 0.0f), lcVector4(fm[5], fm[8], fm[11], 0.0f), lcVector4(fm[0], fm[1], fm[2], 1.0f));
				IncludeTransform = lcMul(IncludeTransform, CurrentTransform);

				if (PrimitiveIndex != -1)
				{
					lcLibraryPrimitive* Primitive = mPrimitives[PrimitiveIndex];

					if (!Primitive->mLoaded && !LoadPrimitive(PrimitiveIndex))
						continue;

					if (Primitive->mStud)
						MeshData.AddMeshDataNoDuplicateCheck(Primitive->mMeshData, IncludeTransform, ColorCode);
					else if (!Primitive->mSubFile)
						MeshData.AddMeshData(Primitive->mMeshData, IncludeTransform, ColorCode);
					else
					{
						lcMemFile IncludeFile;

						if (!mZipFile->ExtractFile(Primitive->mZipFileIndex, IncludeFile))
							continue;

						if (!ReadMeshData(IncludeFile, IncludeTransform, ColorCode, MeshData))
							continue;
					}
				}
				else
				{
					for (int PieceInfoIndex = 0; PieceInfoIndex < mPieces.GetSize(); PieceInfoIndex++)
					{
						PieceInfo* Info = mPieces[PieceInfoIndex];

						if (strcmp(Info->m_strName, FileName))
							continue;

						lcMemFile IncludeFile;

						if (!mZipFile->ExtractFile(Info->m_nOffset, IncludeFile))
							break;

						if (!ReadMeshData(IncludeFile, IncludeTransform, ColorCode, MeshData))
							break;

						break;
					}
				}
			} break;

		case 2:
			{
				sscanf(Line, "%d %i %f %f %f %f %f %f", &LineType, &Dummy, &Points[0].x, &Points[0].y, &Points[0].z, &Points[1].x, &Points[1].y, &Points[1].z);

				Points[0] = lcMul31(Points[0], CurrentTransform);
				Points[1] = lcMul31(Points[1], CurrentTransform);

				MeshData.AddLine(LineType, ColorCode, Points);
			} break;

		case 3:
			{
				sscanf(Line, "%d %i %f %f %f %f %f %f %f %f %f", &LineType, &Dummy, &Points[0].x, &Points[0].y, &Points[0].z,
				       &Points[1].x, &Points[1].y, &Points[1].z, &Points[2].x, &Points[2].y, &Points[2].z);

				Points[0] = lcMul31(Points[0], CurrentTransform);
				Points[1] = lcMul31(Points[1], CurrentTransform);
				Points[2] = lcMul31(Points[2], CurrentTransform);

				MeshData.AddLine(LineType, ColorCode, Points);
			} break;

		case 4:
			{
				sscanf(Line, "%d %i %f %f %f %f %f %f %f %f %f %f %f %f", &LineType, &Dummy, &Points[0].x, &Points[0].y, &Points[0].z,
				       &Points[1].x, &Points[1].y, &Points[1].z, &Points[2].x, &Points[2].y, &Points[2].z, &Points[3].x, &Points[3].y, &Points[3].z);

				Points[0] = lcMul31(Points[0], CurrentTransform);
				Points[1] = lcMul31(Points[1], CurrentTransform);
				Points[2] = lcMul31(Points[2], CurrentTransform);
				Points[3] = lcMul31(Points[3], CurrentTransform);

				MeshData.AddLine(LineType, ColorCode, Points);
			} break;
		}
	}

	return true;
}

void lcLibraryMeshData::AddLine(int LineType, lcuint32 ColorCode, const lcVector3* Vertices)
{
	lcLibraryMeshSection* Section;
	int SectionIdx;
	bool Triangles = (LineType != 2);

	for (SectionIdx = 0; SectionIdx < mSections.GetSize(); SectionIdx++)
	{
		Section = mSections[SectionIdx];

		if (Section->mColorCode == ColorCode && Section->mTriangles == Triangles)
			break;
	}

	if (SectionIdx == mSections.GetSize())
	{
		Section = new lcLibraryMeshSection;

		Section->mColorCode = ColorCode;
		Section->mTriangles = Triangles;

		mSections.Add(Section);
	}

	int Indices[4] = { -1, -1, -1, -1 };

	for (int IndexIdx = 0; IndexIdx < LineType; IndexIdx++)
	{
		const lcVector3& Vertex = Vertices[IndexIdx];

		for (int VertexIdx = mVertices.GetSize() - 1; VertexIdx >= 0; VertexIdx--)
		{
			if (Vertex == mVertices[VertexIdx])
			{
				Indices[IndexIdx] = VertexIdx;
				break;
			}
		}

		if (Indices[IndexIdx] == -1)
		{
			Indices[IndexIdx] = mVertices.GetSize();
			mVertices.Add(Vertex);
		}
	}

	switch (LineType)
	{
	case 4:
		Section->mIndices.Add(Indices[2]);
		Section->mIndices.Add(Indices[3]);
		Section->mIndices.Add(Indices[0]);
	case 3:
		Section->mIndices.Add(Indices[0]);
		Section->mIndices.Add(Indices[1]);
		Section->mIndices.Add(Indices[2]);
		break;
	case 2:
		Section->mIndices.Add(Indices[0]);
		Section->mIndices.Add(Indices[1]);
		break;
	}
}

void lcLibraryMeshData::AddMeshData(const lcLibraryMeshData& Data, const lcMatrix44& Transform, lcuint32 CurrentColorCode)
{
	int VertexCount = Data.mVertices.GetSize();
	ObjArray<lcuint32> IndexRemap(VertexCount);

	mVertices.Expand(Data.mVertices.GetSize());

	for (int SrcVertexIdx = 0; SrcVertexIdx < VertexCount; SrcVertexIdx++)
	{
		lcVector3 Vertex = lcMul31(Data.mVertices[SrcVertexIdx], Transform);
		int Index = -1;

		for (int DstVertexIdx = mVertices.GetSize() - 1; DstVertexIdx >= 0; DstVertexIdx--)
		{
//			if (Vertex == mVertices[DstVertexIdx])
			if (fabsf(Vertex.x - mVertices[DstVertexIdx].x) < 0.25 && fabsf(Vertex.y - mVertices[DstVertexIdx].y) < 0.25 && fabsf(Vertex.z - mVertices[DstVertexIdx].z) < 0.25)
			{
				Index = DstVertexIdx;
				break;
			}
		}

		if (Index == -1)
		{
			Index = mVertices.GetSize();
			mVertices.Add(Vertex);
		}

		IndexRemap.Add(Index);
	}

	for (int SrcSectionIdx = 0; SrcSectionIdx < Data.mSections.GetSize(); SrcSectionIdx++)
	{
		lcLibraryMeshSection* SrcSection = Data.mSections[SrcSectionIdx];
		lcLibraryMeshSection* DstSection = NULL;
		lcuint32 ColorCode = SrcSection->mColorCode == 16 ? CurrentColorCode : SrcSection->mColorCode;

		for (int DstSectionIdx = 0; DstSectionIdx < mSections.GetSize(); DstSectionIdx++)
		{
			lcLibraryMeshSection* Section = mSections[DstSectionIdx];

			if (Section->mColorCode == ColorCode && Section->mTriangles == SrcSection->mTriangles)
			{
				DstSection = Section;
				break;
			}
		}

		if (!DstSection)
		{
			DstSection = new lcLibraryMeshSection;

			DstSection->mColorCode = ColorCode;
			DstSection->mTriangles = SrcSection->mTriangles;

			mSections.Add(DstSection);
		}

		DstSection->mIndices.Expand(SrcSection->mIndices.GetSize());
		for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
			DstSection->mIndices.Add(IndexRemap[SrcSection->mIndices[IndexIdx]]);
	}
}

void lcLibraryMeshData::AddMeshDataNoDuplicateCheck(const lcLibraryMeshData& Data, const lcMatrix44& Transform, lcuint32 CurrentColorCode)
{
	lcuint32 BaseIndex = mVertices.GetSize();

	mVertices.Expand(Data.mVertices.GetSize());

	for (int SrcVertexIdx = 0; SrcVertexIdx < Data.mVertices.GetSize(); SrcVertexIdx++)
		mVertices.Add(lcMul31(Data.mVertices[SrcVertexIdx], Transform));

	for (int SrcSectionIdx = 0; SrcSectionIdx < Data.mSections.GetSize(); SrcSectionIdx++)
	{
		lcLibraryMeshSection* SrcSection = Data.mSections[SrcSectionIdx];
		lcLibraryMeshSection* DstSection = NULL;
		lcuint32 ColorCode = SrcSection->mColorCode == 16 ? CurrentColorCode : SrcSection->mColorCode;

		for (int DstSectionIdx = 0; DstSectionIdx < mSections.GetSize(); DstSectionIdx++)
		{
			lcLibraryMeshSection* Section = mSections[DstSectionIdx];

			if (Section->mColorCode == ColorCode && Section->mTriangles == SrcSection->mTriangles)
			{
				DstSection = Section;
				break;
			}
		}

		if (!DstSection)
		{
			DstSection = new lcLibraryMeshSection;

			DstSection->mColorCode = ColorCode;
			DstSection->mTriangles = SrcSection->mTriangles;

			mSections.Add(DstSection);
		}

		DstSection->mIndices.Expand(SrcSection->mIndices.GetSize());
		for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
			DstSection->mIndices.Add(BaseIndex + SrcSection->mIndices[IndexIdx]);
	}
}
