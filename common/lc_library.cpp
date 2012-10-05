#include "lc_global.h"
#include "lc_library.h"
#include "lc_zipfile.h"
#include "lc_file.h"
#include "pieceinf.h"
#include "lc_colors.h"
#include "system.h"

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

PieceInfo* lcPiecesLibrary::FindPiece(const char* PieceName, bool CreatePlaceholderIfMissing)
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		if (!strcmp(PieceName, mPieces[PieceIdx]->m_strName))
			return mPieces[PieceIdx];

	if (CreatePlaceholderIfMissing)
		return CreatePlaceholder(PieceName);

	return NULL;
}

PieceInfo* lcPiecesLibrary::CreatePlaceholder(const char* PieceName)
{
	PieceInfo* Info = new PieceInfo(-1);

	Info->CreatePlaceholder(PieceName);
	mPieces.Add(Info);

	return Info;
}

bool lcPiecesLibrary::Load(const char* SearchPath)
{
	char LibraryPath[LC_MAXPATH];

	strcpy(LibraryPath, SearchPath);

	int i = strlen(LibraryPath) - 1;
	if ((LibraryPath[i] != '\\') && (LibraryPath[i] != '/'))
		strcat(LibraryPath, "/");

	strcpy(mLibraryPath, LibraryPath);
	strcat(LibraryPath, "complete.zip");

	if (OpenArchive(LibraryPath))
	{
		lcMemFile ColorFile;

		if (!mZipFile->ExtractFile("ldraw/ldconfig.ldr", ColorFile) || !lcLoadColorFile(ColorFile))
			lcLoadDefaultColors();
	}
	else if (OpenDirectory(mLibraryPath))
	{
		char FileName[LC_MAXPATH];
		lcDiskFile ColorFile;

		sprintf(FileName, "%sldconfig.ldr", mLibraryPath);

		if (!ColorFile.Open(FileName, "rt") || !lcLoadColorFile(ColorFile))
			lcLoadDefaultColors();
	}
	else
		return false;

	const char* FileName = Sys_ProfileLoadString("Settings", "Categories", "");
	if (!FileName[0] || !LoadCategories(FileName))
		ResetCategories();

	SystemUpdateCategories(false);

	Sys_ProfileSaveString("Settings", "PiecesLibrary", mLibraryPath);

	return true;
}

bool lcPiecesLibrary::OpenArchive(const char* FileName)
{
	mZipFile = new lcZipFile();

	if (!mZipFile->OpenRead(FileName))
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
				PieceInfo* Info = new PieceInfo(FileIdx);
				mPieces.Add(Info);

				strncpy(Info->m_strName, Name + 12, sizeof(Info->m_strName));
				Info->m_strName[sizeof(Info->m_strName) - 1] = 0;
			}
			else
			{
				lcLibraryPrimitive* Prim = new lcLibraryPrimitive(Name + 12, FileIdx, false, true);
				mPrimitives.Add(Prim);
			}
		}
		else if (!memcmp(Name + 6, "P/", 2))
		{
			lcLibraryPrimitive* Prim = new lcLibraryPrimitive(Name + 8, FileIdx, (memcmp(Name + 8, "STU", 3) == 0), false);
			mPrimitives.Add(Prim);
		}
	}

	lcMemFile PieceFile;

	for (int PieceInfoIndex = 0; PieceInfoIndex < mPieces.GetSize(); PieceInfoIndex++)
	{
		PieceInfo* Info = mPieces[PieceInfoIndex];

		mZipFile->ExtractFile(Info->mZipFileIndex, PieceFile, 256);
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

bool lcPiecesLibrary::OpenDirectory(const char* Path)
{
	char FileName[LC_MAXPATH];
	ObjArray<String> FileList;

	strcpy(FileName, Path);
	strcat(FileName, "parts/");
	int PathLength = strlen(FileName);

	Sys_GetFileList(FileName, FileList);

	mPieces.RemoveAll();
	mPieces.Expand(FileList.GetSize());

	for (int FileIdx = 0; FileIdx < FileList.GetSize(); FileIdx++)
	{
		char Name[LC_PIECE_NAME_LEN];
		const char* Src = (const char*)FileList[FileIdx] + PathLength;
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

		lcDiskFile PieceFile;
		if (!PieceFile.Open(FileList[FileIdx], "rt"))
			continue;

		char Line[1024];
		if (!PieceFile.ReadLine(Line, sizeof(Line)))
			continue;

		PieceInfo* Info = new PieceInfo(-1);
		mPieces.Add(Info);

		Src = (char*)Line + 2;
		Dst = Info->m_strDescription;

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

		strncpy(Info->m_strName, Name, sizeof(Info->m_strName));
		Info->m_strName[sizeof(Info->m_strName) - 1] = 0;
	}

	if (!mPieces.GetSize())
		return false;

	const char* PrimitiveDirectories[] = { "p/", "p/48/", "parts/s/" };
	bool SubFileDirectories[] = { false, false, true };

	for (int DirectoryIdx = 0; DirectoryIdx < sizeof(PrimitiveDirectories) / sizeof(PrimitiveDirectories[0]); DirectoryIdx++)
	{
		strcpy(FileName, Path);
		PathLength = strlen(FileName);

		strcat(FileName, PrimitiveDirectories[DirectoryIdx]);
		PathLength += strchr(PrimitiveDirectories[DirectoryIdx], '/') - PrimitiveDirectories[DirectoryIdx] + 1;

		Sys_GetFileList(FileName, FileList);

		for (int FileIdx = 0; FileIdx < FileList.GetSize(); FileIdx++)
		{
			char Name[LC_PIECE_NAME_LEN];
			const char* Src = (const char*)FileList[FileIdx] + PathLength;
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

			bool SubFile = SubFileDirectories[DirectoryIdx];
			lcLibraryPrimitive* Prim = new lcLibraryPrimitive(Name, 0, !SubFile && (memcmp(Name, "STU", 3) == 0), SubFile);
			mPrimitives.Add(Prim);
		}
	}

	return true;
}

bool lcPiecesLibrary::LoadPiece(PieceInfo* Info)
{
	lcLibraryMeshData MeshData;

	if (mZipFile)
	{
		lcMemFile PieceFile;

		if (!mZipFile->ExtractFile(Info->mZipFileIndex, PieceFile))
			return false;

		if (!ReadMeshData(PieceFile, lcMatrix44Identity(), 16, MeshData))
			return false;
	}
	else
	{
		char Name[LC_PIECE_NAME_LEN];
		strcpy(Name, Info->m_strName);
		strlwr(Name);

		char FileName[LC_MAXPATH];
		lcDiskFile PieceFile;

		sprintf(FileName, "%sparts/%s.dat", mLibraryPath, Name);

		if (!PieceFile.Open(FileName, "rt"))
			return false;

		if (!ReadMeshData(PieceFile, lcMatrix44Identity(), 16, MeshData))
			return false;
	}

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
				Info->mFlags |= LC_PIECE_HAS_DEFAULT;
			else
			{
				if (lcIsColorTranslucent(DstSection.ColorIndex))
					Info->mFlags |= LC_PIECE_HAS_TRANSLUCENT;
				else
					Info->mFlags |= LC_PIECE_HAS_SOLID;
			}
		}
		else
			Info->mFlags |= LC_PIECE_HAS_LINES;

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

	if (mZipFile)
	{
		lcMemFile PrimFile;

		if (!mZipFile->ExtractFile(Primitive->mZipFileIndex, PrimFile))
			return false;

		if (!ReadMeshData(PrimFile, lcMatrix44Identity(), 16, Primitive->mMeshData))
			return false;
	}
	else
	{
		char Name[LC_PIECE_NAME_LEN];
		strcpy(Name, Primitive->mName);
		strlwr(Name);

		char FileName[LC_MAXPATH];
		lcDiskFile PrimFile;

		if (Primitive->mSubFile)
			sprintf(FileName, "%sparts/%s.dat", mLibraryPath, Name);
		else
			sprintf(FileName, "%sp/%s.dat", mLibraryPath, Name);

		if (!PrimFile.Open(FileName, "rt"))
			return false;

		if (!ReadMeshData(PrimFile, lcMatrix44Identity(), 16, Primitive->mMeshData))
			return false;
	}

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
						if (mZipFile)
						{
							lcMemFile IncludeFile;

							if (!mZipFile->ExtractFile(Primitive->mZipFileIndex, IncludeFile))
								continue;

							if (!ReadMeshData(IncludeFile, IncludeTransform, ColorCode, MeshData))
								continue;
						}
						else
						{
							char Name[LC_PIECE_NAME_LEN];
							strcpy(Name, Primitive->mName);
							strlwr(Name);

							char FileName[LC_MAXPATH];
							lcDiskFile IncludeFile;

							if (Primitive->mSubFile)
								sprintf(FileName, "%sparts/%s.dat", mLibraryPath, Name);
							else
								sprintf(FileName, "%sp/%s.dat", mLibraryPath, Name);

							if (!IncludeFile.Open(FileName, "rt"))
								continue;

							if (!ReadMeshData(IncludeFile, IncludeTransform, ColorCode, MeshData))
								continue;
						}
					}
				}
				else
				{
					for (int PieceInfoIndex = 0; PieceInfoIndex < mPieces.GetSize(); PieceInfoIndex++)
					{
						PieceInfo* Info = mPieces[PieceInfoIndex];

						if (strcmp(Info->m_strName, FileName))
							continue;

						if (mZipFile)
						{
							lcMemFile IncludeFile;

							if (!mZipFile->ExtractFile(Info->mZipFileIndex, IncludeFile))
								break;

							if (!ReadMeshData(IncludeFile, IncludeTransform, ColorCode, MeshData))
								break;
						}
						else
						{
							char Name[LC_PIECE_NAME_LEN];
							strcpy(Name, Info->m_strName);
							strlwr(Name);

							char FileName[LC_MAXPATH];
							lcDiskFile IncludeFile;

							sprintf(FileName, "%sparts/%s.dat", mLibraryPath, Name);

							if (!IncludeFile.Open(FileName, "rt"))
								break;

							if (!ReadMeshData(IncludeFile, IncludeTransform, ColorCode, MeshData))
								break;
						}

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
		if (Indices[0] != Indices[2] && Indices[0] != Indices[3] && Indices[2] != Indices[3])
		{
			Section->mIndices.Add(Indices[2]);
			Section->mIndices.Add(Indices[3]);
			Section->mIndices.Add(Indices[0]);
		}
	case 3:
		if (Indices[0] != Indices[1] && Indices[0] != Indices[2] && Indices[1] != Indices[2])
		{
			Section->mIndices.Add(Indices[0]);
			Section->mIndices.Add(Indices[1]);
			Section->mIndices.Add(Indices[2]);
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

bool lcPiecesLibrary::PieceInCategory(PieceInfo* Info, const String& CategoryKeywords) const
{
	String PieceName = Info->m_strDescription;
	PieceName.MakeLower();

	String Keywords = CategoryKeywords;
	Keywords.MakeLower();

	return PieceName.Match(Keywords);
}

int lcPiecesLibrary::GetFirstPieceCategory(PieceInfo* Info) const
{
	for (int i = 0; i < mCategories.GetSize(); i++)
		if (PieceInCategory(Info, mCategories[i].Keywords))
			return i;

	return -1;
}

void lcPiecesLibrary::GetCategoryEntries(int CategoryIndex, bool GroupPieces, PtrArray<PieceInfo>& SinglePieces, PtrArray<PieceInfo>& GroupedPieces)
{
	SinglePieces.RemoveAll();
	GroupedPieces.RemoveAll();

	// Don't group entries in the search results category.
	if (mCategories[CategoryIndex].Name == "Search Results")
		GroupPieces = false;

	for (int i = 0; i < mPieces.GetSize(); i++)
	{
		PieceInfo* Info = mPieces[i];

		if (!PieceInCategory(Info, mCategories[CategoryIndex].Keywords))
			continue;

		if (!GroupPieces)
		{
			SinglePieces.Add(Info);
			continue;
		}

		// Check if it's a patterned piece.
		if (Info->IsPatterned())
		{
			PieceInfo* Parent;

			// Find the parent of this patterned piece.
			char ParentName[LC_PIECE_NAME_LEN];
			strcpy(ParentName, Info->m_strName);
			*strchr(ParentName, 'P') = '\0';

			Parent = FindPiece(ParentName, false);

			if (Parent)
			{
				// Check if the parent was added as a single piece.
				int Index = SinglePieces.FindIndex(Parent);

				if (Index != -1)
					SinglePieces.RemoveIndex(Index);

				Index = GroupedPieces.FindIndex(Parent);

				if (Index == -1)
					GroupedPieces.Add(Parent);
			}
			else
			{
				// Patterned pieces should have a parent but in case they don't just add them anyway.
				SinglePieces.Add(Info);
			}
		}
		else
		{
			// Check if this piece has already been added to this category by one of its children.
			int Index = GroupedPieces.FindIndex(Info);

			if (Index == -1)
				SinglePieces.Add(Info);
		}
	}
}

void lcPiecesLibrary::GetPatternedPieces(PieceInfo* Parent, PtrArray<PieceInfo>& Pieces) const
{
	char Name[LC_PIECE_NAME_LEN];
	strcpy(Name, Parent->m_strName);
	strcat(Name, "P");

	Pieces.RemoveAll();

	for (int i = 0; i < mPieces.GetSize(); i++)
	{
		PieceInfo* Info = mPieces[i];

		if (strncmp(Name, Info->m_strName, strlen(Name)) == 0)
			Pieces.Add(Info);
	}

	// Sometimes pieces with A and B versions don't follow the same convention (for example, 3040Pxx instead of 3040BPxx).
	if (Pieces.GetSize() == 0)
	{
		strcpy(Name, Parent->m_strName);
		int Len = strlen(Name);
		if (Name[Len-1] < '0' || Name[Len-1] > '9')
			Name[Len-1] = 'P';

		for (int i = 0; i < mPieces.GetSize(); i++)
		{
			PieceInfo* Info = mPieces[i];

			if (strncmp(Name, Info->m_strName, strlen(Name)) == 0)
				Pieces.Add(Info);
		}
	}
}

void lcPiecesLibrary::ResetCategories()
{
	struct CategoryEntry
	{
		const char* Name;
		const char* Keywords;
	};

	// Animal, Antenna, Arch, Arm, Bar, Baseplate, Belville, Boat, Bracket, Brick,
	// Car, Cone, Container, Conveyor, Crane, Cylinder, Door, Electric, Exhaust,
	// Fence, Flag, Forklift, Freestyle, Garage, Gate, Glass, Grab, Hinge, Homemaker,
	// Hose, Jack, Ladder, Lever, Magnet, Minifig, Minifig Accessory, Panel, Plane,
	// Plant, Plate, Platform, Propellor, Rack, Roadsign, Rock, Scala, Slope, Staircase,
	// Support, Tail, Tap, Technic, Tile, Tipper, Tractor, Trailer, Train, Turntable,
	// Tyre, Wedge, Wheel, Winch, Window, Windscreen, Wing
	CategoryEntry DefaultCategories[] =
	{
		{ "Animal", "^%Animal" },
		{ "Antenna", "^%Antenna" },
		{ "Arch", "^%Arch" },
		{ "Bar", "^%Bar" },
		{ "Baseplate", "^%Baseplate | ^%Platform" },
		{ "Boat", "^%Boat" },
		{ "Brick", "^%Brick" },
		{ "Container", "^%Container | ^%Box | ^Chest | ^%Storage | ^Mailbox" },
		{ "Door and Window", "^%Door | ^%Window | ^%Glass | ^%Freestyle | ^%Gate | ^%Garage | ^%Roller" },
		{ "Duplo", "^%Duplo | ^%Scala | ^%Belville" },
		{ "Electric", "^%Electric | ^%Light | ^%Excavator | ^%Exhaust" },
		{ "Hinge and Bracket", "^%Hinge | ^%Bracket | ^%Turntable" },
		{ "Hose", "^%Hose" },
		{ "Minifig", "^%Minifig" },
		{ "Miscellaneous", "^%Arm | ^%Barrel | ^%Brush | ^%Cockpit | ^%Conveyor | ^%Crane | ^%Cupboard | ^%Fabuland | ^%Fence | ^%Homemaker | ^%Jack | ^%Ladder | ^%Rock | ^%Staircase | ^%Stretcher | ^%Tap | ^%Tipper | ^%Trailer | ^%Winch" },
		{ "Panel", "^%Panel | ^%Castle Wall | ^%Castle Turret" },
		{ "Plant", "^%Plant" },
		{ "Plate", "^%Plate" },
		{ "Round", "^%Cylinder | ^%Cone | ^%Dish | ^%Round" },
		{ "Sign and Flag", "^%Flag | ^%Roadsign | ^%Streetlight | ^%Flagpost | ^%Lamppost | ^%Signpost" },
		{ "Slope", "^%Slope" },
		{ "Space", "^%Space" },
		{ "Sticker", "^%Sticker" },
		{ "Support", "^%Support" },
		{ "Technic", "^%Technic | ^%Rack" },
		{ "Tile", "^%Tile" },
		{ "Train", "^%Train | ^%Monorail | ^%Magnet" },
		{ "Tyre and Wheel", "^%Tyre | %^Wheel | ^%Castle Wagon" },
		{ "Vehicle", "^%Car | ^%Tractor | ^%Bike | ^%Plane | ^%Propellor | ^%Tail | ^%Landing | ^%Forklift | ^%Grab Jaw" },
		{ "Windscreen", "^%Windscreen" },
		{ "Wedge", "^%Wedge" },
		{ "Wing", "^%Wing" },
	};
	const int NumCategories = sizeof(DefaultCategories)/sizeof(DefaultCategories[0]);

	mCategories.RemoveAll();
	for (int i = 0; i < NumCategories; i++)
	{
		lcLibraryCategory& Category = mCategories.Add();

		Category.Name = DefaultCategories[i].Name;
		Category.Keywords = DefaultCategories[i].Keywords;
	}

	strcpy(mCategoriesFile, "");
	Sys_ProfileSaveString("Settings", "Categories", mCategoriesFile);
	mCategoriesModified = false;
}

bool lcPiecesLibrary::LoadCategories(const char* FileName)
{
	char Path[LC_MAXPATH];

	if (FileName)
	{
		strcpy(Path, FileName);
	}
	else
	{
		LC_FILEOPENDLG_OPTS opts;

		opts.type = LC_FILEOPENDLG_LCF;
		strcpy(opts.path, mCategoriesFile);

		if (!SystemDoDialog(LC_DLG_FILE_OPEN, &opts)) 
			return false;

		strcpy(Path, (char*)opts.filenames);

		free(opts.filenames);
	}

	// Load the file.
	lcDiskFile File;

	if (!File.Open(Path, "rb"))
		return false;

	lcuint32 i;

	File.ReadU32(&i, 1);
	if (i != LC_FILE_ID)
		return false;

	File.ReadU32(&i, 1);
	if (i != LC_CATEGORY_FILE_ID)
		return false;

	File.ReadU32(&i, 1);
	if (i != LC_CATEGORY_FILE_VERSION)
		return false;

	mCategories.RemoveAll();

	File.ReadU32(&i, 1);
	while (i--)
	{
		lcLibraryCategory& Category = mCategories.Add();

		File.ReadString(Category.Name);
		File.ReadString(Category.Keywords);
	}

	strcpy(mCategoriesFile, Path);
	Sys_ProfileSaveString("Settings", "Categories", mCategoriesFile);
	mCategoriesModified = false;

	return true;
}

bool lcPiecesLibrary::SaveCategories()
{
	if (mCategoriesModified)
	{
		switch (SystemDoMessageBox("Save changes to categories?", LC_MB_YESNOCANCEL | LC_MB_ICONQUESTION))
		{
			case LC_CANCEL:
				return false;

			case LC_YES:
				if (!DoSaveCategories(false))
					return false;
				break;

			case LC_NO:
				return true;
				break;
		}
	}

	return true;
}

bool lcPiecesLibrary::DoSaveCategories(bool AskName)
{
	// Get the file name.
	if (AskName || !mCategoriesFile[0])
	{
		LC_FILESAVEDLG_OPTS opts;

		opts.type = LC_FILESAVEDLG_LCF;
		strcpy(opts.path, mCategoriesFile);

		if (!SystemDoDialog(LC_DLG_FILE_SAVE, &opts)) 
			return false; 

		strcpy(mCategoriesFile, opts.path);
	}

	// Save the file.
	lcDiskFile File;

	if (!File.Open(mCategoriesFile, "wb"))
		return false;

	File.WriteU32(LC_FILE_ID);
	File.WriteU32(LC_CATEGORY_FILE_ID);
	File.WriteU32(LC_CATEGORY_FILE_VERSION);

	int NumCategories = mCategories.GetSize();
	int i;

	for (i = 0; i < mCategories.GetSize(); i++)
	{
		if (mCategories[i].Name == "Search Results")
		{
			NumCategories--;
			break;
		}
	}

	File.WriteU32(NumCategories);
	for (i = 0; i < mCategories.GetSize(); i++)
	{
		if (mCategories[i].Name == "Search Results")
			continue;

		File.WriteString(mCategories[i].Name);
		File.WriteString(mCategories[i].Keywords);
	}

	Sys_ProfileSaveString("Settings", "Categories", mCategoriesFile);
	mCategoriesModified = false;

	return true;
}

int lcPiecesLibrary::FindCategoryIndex(const String& CategoryName) const
{
	for (int i = 0; i < mCategories.GetSize(); i++)
		if (mCategories[i].Name == CategoryName)
			return i;

	return -1;
}

void lcPiecesLibrary::SetCategory(int Index, const String& Name, const String& Keywords)
{
	mCategories[Index].Name = Name;
	mCategories[Index].Keywords = Keywords;

	SystemUpdateCategories(true);

	mCategoriesModified = true;
}

void lcPiecesLibrary::AddCategory(const String& Name, const String& Keywords)
{
	lcLibraryCategory& Category = mCategories.Add();

	Category.Name = Name;
	Category.Keywords = Keywords;

	SystemUpdateCategories(true);

	mCategoriesModified = true;
}

void lcPiecesLibrary::RemoveCategory(int Index)
{
	mCategories.RemoveIndex(Index);

	mCategoriesModified = true;
}
