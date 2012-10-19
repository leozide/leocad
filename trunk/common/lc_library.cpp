#include "lc_global.h"
#include "lc_library.h"
#include "lc_zipfile.h"
#include "lc_file.h"
#include "pieceinf.h"
#include "lc_colors.h"
#include "lc_texture.h"
#include "system.h"
#include <sys/types.h>
#include <sys/stat.h>

#define LC_LIBRARY_CACHE_VERSION   0x0100
#define LC_LIBRARY_CACHE_ARCHIVE   0x0001
#define LC_LIBRARY_CACHE_DIRECTORY 0x0002

lcPiecesLibrary::lcPiecesLibrary()
{
	mZipFile = NULL;
}

lcPiecesLibrary::~lcPiecesLibrary()
{
	Unload();
}

void lcPiecesLibrary::Unload()
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		delete mPieces[PieceIdx];
	mPieces.RemoveAll();

	for (int PrimitiveIdx = 0; PrimitiveIdx < mPrimitives.GetSize(); PrimitiveIdx++)
		delete mPrimitives[PrimitiveIdx];
	mPrimitives.RemoveAll();

	for (int TextureIdx = 0; TextureIdx < mTextures.GetSize(); TextureIdx++)
		delete mTextures[TextureIdx];
	mTextures.RemoveAll();

	delete mZipFile;
	mZipFile = NULL;
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

lcTexture* lcPiecesLibrary::FindTexture(const char* TextureName)
{
	for (int TextureIdx = 0; TextureIdx < mTextures.GetSize(); TextureIdx++)
		if (!strcmp(TextureName, mTextures[TextureIdx]->mName))
			return mTextures[TextureIdx];

	return NULL;
}

bool lcPiecesLibrary::Load(const char* SearchPath, const char* CacheFilePath)
{
	char LibraryPath[LC_MAXPATH];

	strcpy(LibraryPath, SearchPath);

	int i = strlen(LibraryPath) - 1;
	if ((LibraryPath[i] != '\\') && (LibraryPath[i] != '/'))
		strcat(LibraryPath, "/");

	strcpy(mLibraryPath, LibraryPath);
	strcat(LibraryPath, "complete.zip");

	if (OpenArchive(LibraryPath, CacheFilePath))
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

bool lcPiecesLibrary::OpenArchive(const char* FileName, const char* CacheFilePath)
{
	Unload();

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
		{
			if (!memcmp(Dst, ".PNG", 4) && !memcmp(Name, "LDRAW/PARTS/TEXTURES/", 21))
			{
				lcTexture* Texture = new lcTexture();
				mTextures.Add(Texture);

				strncpy(Texture->mName, Name + 21, sizeof(Texture->mName));
				Texture->mName[sizeof(Texture->mName) - 1] = 0;
			}

			continue;
		}
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

	bool CacheValid = false;
	struct stat LibraryStat;

	strcpy(mCacheFileName, CacheFilePath);
	if (CacheFilePath[0])
		strcat(mCacheFileName, "library.cache");

	if (stat(FileName, &LibraryStat) == 0)
	{
		lcZipFile CacheFile;

		if (CacheFile.OpenRead(mCacheFileName))
		{
			lcMemFile VersionFile;

			if (CacheFile.ExtractFile("version", VersionFile))
			{
				lcuint32 CacheVersion;
				lcuint32 CacheFlags;
				lcuint64 LibrarySize;
				lcuint64 LibraryModified;

				if (VersionFile.ReadU32(&CacheVersion, 1) && VersionFile.ReadU32(&CacheFlags, 1) &&
				    VersionFile.ReadU64(&LibrarySize, 1) && VersionFile.ReadU64(&LibraryModified, 1))
				{
					if (CacheVersion == LC_LIBRARY_CACHE_VERSION && CacheFlags == LC_LIBRARY_CACHE_ARCHIVE &&
					    LibrarySize == (lcuint64)LibraryStat.st_size && LibraryModified == (lcuint64)LibraryStat.st_mtime)
						CacheValid = true;
				}
			}
		}

		if (CacheValid)
		{
			lcMemFile IndexFile;

			if (CacheFile.ExtractFile("index", IndexFile))
			{
				lcuint32 NumFiles;

				if (IndexFile.ReadU32(&NumFiles, 1) && NumFiles == (lcuint32)mPieces.GetSize())
				{
					for (int PieceInfoIndex = 0; PieceInfoIndex < mPieces.GetSize(); PieceInfoIndex++)
					{
						PieceInfo* Info = mPieces[PieceInfoIndex];

						LC_CASSERT(sizeof(Info->m_strDescription) == 128);

						if (!IndexFile.ReadBuffer(Info->m_strDescription, sizeof(Info->m_strDescription)))
						{
							CacheValid = false;
							break;
						}
					}
				}
				else
					CacheValid = false;
			}
			else
				CacheValid = false;
		}
	}

	if (!CacheValid)
	{
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
				if (*Src != '\r' && *Src != '\n' && *Src && Dst - Info->m_strDescription < (int)sizeof(Info->m_strDescription) - 1)
				{
					*Dst++ = *Src++;
					continue;
				}

				*Dst = 0;
				break;
			}
		}

		lcZipFile CacheFile;

		if (CacheFile.OpenWrite(mCacheFileName, false))
		{
			lcMemFile VersionFile;

			VersionFile.WriteU32(LC_LIBRARY_CACHE_VERSION);
			VersionFile.WriteU32(LC_LIBRARY_CACHE_ARCHIVE);
			VersionFile.WriteU64(LibraryStat.st_size);
			VersionFile.WriteU64(LibraryStat.st_mtime);

			CacheFile.AddFile("version", VersionFile);

			lcMemFile IndexFile;

			IndexFile.WriteU32(mPieces.GetSize());

			for (int PieceInfoIndex = 0; PieceInfoIndex < mPieces.GetSize(); PieceInfoIndex++)
			{
				PieceInfo* Info = mPieces[PieceInfoIndex];

				IndexFile.WriteBuffer(Info->m_strDescription, sizeof(Info->m_strDescription));
			}

			CacheFile.AddFile("index", IndexFile);
		}
	}

	return true;
}

bool lcPiecesLibrary::OpenDirectory(const char* Path)
{
	Unload();

	char FileName[LC_MAXPATH];
	ObjArray<String> FileList;

	strcpy(FileName, Path);
	strcat(FileName, "parts/");
	int PathLength = strlen(FileName);

	Sys_GetFileList(FileName, FileList);

	mPieces.Expand(FileList.GetSize());

	for (int FileIdx = 0; FileIdx < FileList.GetSize(); FileIdx++)
	{
		char Name[LC_PIECE_NAME_LEN];
		const char* Src = (const char*)FileList[FileIdx] + PathLength;
		char* Dst = Name;

		while (*Src && Dst - Name < (int)sizeof(Name))
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
			if (*Src != '\r' && *Src != '\n' && *Src && Dst - Info->m_strDescription < (int)sizeof(Info->m_strDescription) - 1)
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

	for (int DirectoryIdx = 0; DirectoryIdx < (int)(sizeof(PrimitiveDirectories) / sizeof(PrimitiveDirectories[0])); DirectoryIdx++)
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

			while (*Src && Dst - Name < (int)sizeof(Name))
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

	strcpy(FileName, Path);
	strcat(FileName, "parts/textures/");
	PathLength = strlen(FileName);

	Sys_GetFileList(FileName, FileList);

	mTextures.Expand(FileList.GetSize());

	for (int FileIdx = 0; FileIdx < FileList.GetSize(); FileIdx++)
	{
		char Name[LC_MAXPATH];
		const char* Src = (const char*)FileList[FileIdx] + PathLength;
		char* Dst = Name;

		while (*Src && Dst - Name < (int)sizeof(Name))
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
		if (memcmp(Dst, ".PNG", 4))
			continue;
		*Dst = 0;

		lcTexture* Texture = new lcTexture();
		mTextures.Add(Texture);

		strncpy(Texture->mName, Name, sizeof(Texture->mName));
		Texture->mName[sizeof(Texture->mName) - 1] = 0;
	}

	return true;
}

int LibraryMeshSectionCompare(const lcLibraryMeshSection* a, const lcLibraryMeshSection* b, void* Data)
{
	if (a->mPrimitiveType != b->mPrimitiveType)
	{
		int PrimitiveOrder[LC_MESH_NUM_PRIMITIVE_TYPES] =
		{
			LC_MESH_TRIANGLES,
			LC_MESH_TEXTURED_TRIANGLES,
			LC_MESH_LINES,
			LC_MESH_TEXTURED_LINES,
		};

		for (int PrimitiveType = 0; PrimitiveType < LC_MESH_NUM_PRIMITIVE_TYPES; PrimitiveType++)
		{
			int Primitive = PrimitiveOrder[PrimitiveType];

			if (a->mPrimitiveType == Primitive)
				return -1;

			if (b->mPrimitiveType == Primitive)
				return 1;
		}
	}

	bool TranslucentA = lcIsColorTranslucent(a->mColor);
	bool TranslucentB = lcIsColorTranslucent(b->mColor);

	if (TranslucentA != TranslucentB)
		return TranslucentA ? 1 : -1;

	return a->mColor > b->mColor ? -1 : 1;
}

bool lcPiecesLibrary::LoadPiece(PieceInfo* Info)
{
	lcLibraryMeshData MeshData;
	ObjArray<lcLibraryTextureMap> TextureStack;

	if (mZipFile)
	{
		lcMemFile PieceFile;

		if (!mZipFile->ExtractFile(Info->mZipFileIndex, PieceFile))
			return false;

		if (!ReadMeshData(PieceFile, lcMatrix44Identity(), 16, TextureStack, MeshData))
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

		if (!ReadMeshData(PieceFile, lcMatrix44Identity(), 16, TextureStack, MeshData))
			return false;
	}

	lcMesh* Mesh = new lcMesh();

	int NumIndices = 0;

	for (int SectionIdx = 0; SectionIdx < MeshData.mSections.GetSize(); SectionIdx++)
	{
		lcLibraryMeshSection* Section = MeshData.mSections[SectionIdx];

		Section->mColor = lcGetColorIndex(Section->mColor);
		NumIndices += Section->mIndices.GetSize();
	}

	MeshData.mSections.Sort(LibraryMeshSectionCompare, NULL);

	Mesh->Create(MeshData.mSections.GetSize(), MeshData.mVertices.GetSize(), MeshData.mTexturedVertices.GetSize(), NumIndices);

	lcVertex* DstVerts = (lcVertex*)Mesh->mVertexBuffer.mData;
	lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (int VertexIdx = 0; VertexIdx < MeshData.mVertices.GetSize(); VertexIdx++)
	{
		lcVertex& DstVertex = *DstVerts++;

		const lcVector3& SrcPosition = MeshData.mVertices[VertexIdx].Position;
		lcVector3& DstPosition = DstVertex.Position;

		DstPosition = lcVector3(SrcPosition.x / 25.0f, SrcPosition.z / 25.0f, -SrcPosition.y / 25.0f);

		Min.x = lcMin(Min.x, DstPosition.x);
		Min.y = lcMin(Min.y, DstPosition.y);
		Min.z = lcMin(Min.z, DstPosition.z);
		Max.x = lcMax(Max.x, DstPosition.x);
		Max.y = lcMax(Max.y, DstPosition.y);
		Max.z = lcMax(Max.z, DstPosition.z);
	}

	lcVertexTextured* DstTexturedVerts = (lcVertexTextured*)DstVerts;

	for (int VertexIdx = 0; VertexIdx < MeshData.mTexturedVertices.GetSize(); VertexIdx++)
	{
		lcVertexTextured& DstVertex = *DstTexturedVerts++;
		lcVertexTextured& SrcVertex = MeshData.mTexturedVertices[VertexIdx];

		const lcVector3& SrcPosition = SrcVertex.Position;
		lcVector3& DstPosition = DstVertex.Position;

		DstPosition = lcVector3(SrcPosition.x / 25.0f, SrcPosition.z / 25.0f, -SrcPosition.y / 25.0f);
		DstVertex.TexCoord = SrcVertex.TexCoord;

		Min.x = lcMin(Min.x, DstPosition.x);
		Min.y = lcMin(Min.y, DstPosition.y);
		Min.z = lcMin(Min.z, DstPosition.z);
		Max.x = lcMax(Max.x, DstPosition.x);
		Max.y = lcMax(Max.y, DstPosition.y);
		Max.z = lcMax(Max.z, DstPosition.z);
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

		DstSection.ColorIndex = SrcSection->mColor;
		DstSection.PrimitiveType = (SrcSection->mPrimitiveType == LC_MESH_TRIANGLES || SrcSection->mPrimitiveType == LC_MESH_TEXTURED_TRIANGLES) ? GL_TRIANGLES : GL_LINES;
		DstSection.NumIndices = SrcSection->mIndices.GetSize();
		DstSection.Texture = SrcSection->mTexture;

		if (DstSection.Texture)
			DstSection.Texture->AddRef();

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

		if (DstSection.PrimitiveType == GL_TRIANGLES)
		{
			if (SrcSection->mColor == 16)
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

bool lcPiecesLibrary::LoadTexture(lcTexture* Texture)
{
	char Name[LC_MAXPATH], FileName[LC_MAXPATH];

	strcpy(Name, Texture->mName);
	strlwr(Name);

	if (mZipFile)
	{
		lcMemFile TextureFile;

		sprintf(FileName, "parts/textures/%s.png", Name);

		if (!mZipFile->ExtractFile(FileName, TextureFile))
			return false;

		if (!Texture->Load(TextureFile))
			return false;
	}
	else
	{
		lcDiskFile TextureFile;

		sprintf(FileName, "%sparts/textures/%s.png", mLibraryPath, Name);

		if (!TextureFile.Open(FileName, "rb"))
			return false;

		if (!Texture->Load(TextureFile))
			return false;
	}

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
	ObjArray<lcLibraryTextureMap> TextureStack;

	if (mZipFile)
	{
		lcMemFile PrimFile;

		if (!mZipFile->ExtractFile(Primitive->mZipFileIndex, PrimFile))
			return false;

		if (!ReadMeshData(PrimFile, lcMatrix44Identity(), 16, TextureStack, Primitive->mMeshData))
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

		if (!ReadMeshData(PrimFile, lcMatrix44Identity(), 16, TextureStack, Primitive->mMeshData))
			return false;
	}

	Primitive->mLoaded = true;

	return true;
}

bool lcPiecesLibrary::ReadMeshData(lcFile& File, const lcMatrix44& CurrentTransform, lcuint32 CurrentColorCode, ObjArray<lcLibraryTextureMap>& TextureStack, lcLibraryMeshData& MeshData)
{
	char Buffer[1024];
	char* Line;

	while (File.ReadLine(Buffer, sizeof(Buffer)))
	{
		lcuint32 ColorCode, ColorCodeHex;
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

					if (!strcmp(Token, "PLANAR"))
					{
						Token += 7;

						char FileName[LC_MAXPATH];
						lcVector3 Points[3];

						sscanf(Token, "%f %f %f %f %f %f %f %f %f %s", &Points[0].x, &Points[0].y, &Points[0].z, &Points[1].x, &Points[1].y, &Points[1].z, &Points[2].x, &Points[2].y, &Points[2].z, FileName);

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

						lcLibraryTextureMap& Map = TextureStack.Add();
						Map.Next = false;
						Map.Fallback = false;
						Map.Texture = FindTexture(FileName);

						for (int EdgeIdx = 0; EdgeIdx < 2; EdgeIdx++)
						{
							lcVector3 Normal = Points[EdgeIdx + 1] - Points[0];
							float Length = lcLength(Normal);
							Normal /= Length;

							Map.Params[EdgeIdx].x = Normal.x / Length;
							Map.Params[EdgeIdx].y = Normal.y / Length;
							Map.Params[EdgeIdx].z = Normal.z / Length;
							Map.Params[EdgeIdx].w = -lcDot(Normal, Points[0]) / Length;
						}
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

		if (LineType < 1 || LineType > 4)
			continue;

		if (ColorCode == 0)
		{
			sscanf(Line, "%d %i", &LineType, &ColorCodeHex);

			if (ColorCode != ColorCodeHex)
				ColorCode = ColorCodeHex | LC_COLOR_DIRECT;
		}

		if (ColorCode == 16)
			ColorCode = CurrentColorCode;

		lcLibraryTextureMap* TextureMap = NULL;

		if (TextureStack.GetSize())
		{
			TextureMap = &TextureStack[TextureStack.GetSize() - 1];

			if (TextureMap->Fallback)
				continue;
		}

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
						MeshData.AddMeshDataNoDuplicateCheck(Primitive->mMeshData, IncludeTransform, ColorCode, TextureMap);
					else if (!Primitive->mSubFile)
						MeshData.AddMeshData(Primitive->mMeshData, IncludeTransform, ColorCode, TextureMap);
					else
					{
						if (mZipFile)
						{
							lcMemFile IncludeFile;

							if (!mZipFile->ExtractFile(Primitive->mZipFileIndex, IncludeFile))
								continue;

							if (!ReadMeshData(IncludeFile, IncludeTransform, ColorCode, TextureStack, MeshData))
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

							if (!ReadMeshData(IncludeFile, IncludeTransform, ColorCode, TextureStack, MeshData))
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

							if (!ReadMeshData(IncludeFile, IncludeTransform, ColorCode, TextureStack, MeshData))
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

							if (!ReadMeshData(IncludeFile, IncludeTransform, ColorCode, TextureStack, MeshData))
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

				if (TextureMap)
				{
					MeshData.AddTexturedLine(LineType, ColorCode, *TextureMap, Points);

					if (TextureMap->Next)
						TextureStack.RemoveIndex(TextureStack.GetSize() - 1);
				}
				else
					MeshData.AddLine(LineType, ColorCode, Points);
			} break;

		case 3:
			{
				sscanf(Line, "%d %i %f %f %f %f %f %f %f %f %f", &LineType, &Dummy, &Points[0].x, &Points[0].y, &Points[0].z,
				       &Points[1].x, &Points[1].y, &Points[1].z, &Points[2].x, &Points[2].y, &Points[2].z);

				Points[0] = lcMul31(Points[0], CurrentTransform);
				Points[1] = lcMul31(Points[1], CurrentTransform);
				Points[2] = lcMul31(Points[2], CurrentTransform);

				if (TextureMap)
				{
					MeshData.AddTexturedLine(LineType, ColorCode, *TextureMap, Points);

					if (TextureMap->Next)
						TextureStack.RemoveIndex(TextureStack.GetSize() - 1);
				}
				else
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

				if (TextureMap)
				{
					MeshData.AddTexturedLine(LineType, ColorCode, *TextureMap, Points);

					if (TextureMap->Next)
						TextureStack.RemoveIndex(TextureStack.GetSize() - 1);
				}
				else
					MeshData.AddLine(LineType, ColorCode, Points);
			} break;
		}
	}

	return true;
}

void lcLibraryMeshData::AddLine(int LineType, lcuint32 ColorCode, const lcVector3* Vertices)
{
	lcLibraryMeshSection* Section = NULL;
	int SectionIdx;
	LC_MESH_PRIMITIVE_TYPE PrimitiveType = (LineType == 2) ? LC_MESH_LINES : LC_MESH_TRIANGLES;

	for (SectionIdx = 0; SectionIdx < mSections.GetSize(); SectionIdx++)
	{
		Section = mSections[SectionIdx];

		if (Section->mColor == ColorCode && Section->mPrimitiveType == PrimitiveType)
			break;
	}

	if (SectionIdx == mSections.GetSize())
	{
		Section = new lcLibraryMeshSection(PrimitiveType, ColorCode, NULL);

		mSections.Add(Section);
	}

	int Indices[4] = { -1, -1, -1, -1 };

	for (int IndexIdx = 0; IndexIdx < LineType; IndexIdx++)
	{
		const lcVector3& Position = Vertices[IndexIdx];

		for (int VertexIdx = mVertices.GetSize() - 1; VertexIdx >= 0; VertexIdx--)
		{
			lcVertex& DstVertex = mVertices[VertexIdx];

			if (Position == DstVertex.Position)
			{
				Indices[IndexIdx] = VertexIdx;
				break;
			}
		}

		if (Indices[IndexIdx] == -1)
		{
			Indices[IndexIdx] = mVertices.GetSize();
			lcVertex& DstVertex = mVertices.Add();
			DstVertex.Position = Position;
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

void lcLibraryMeshData::AddTexturedLine(int LineType, lcuint32 ColorCode, const lcLibraryTextureMap& Map, const lcVector3* Vertices)
{
	lcLibraryMeshSection* Section = NULL;
	int SectionIdx;
	LC_MESH_PRIMITIVE_TYPE PrimitiveType = (LineType == 2) ? LC_MESH_TEXTURED_LINES : LC_MESH_TEXTURED_TRIANGLES;

	for (SectionIdx = 0; SectionIdx < mSections.GetSize(); SectionIdx++)
	{
		Section = mSections[SectionIdx];

		if (Section->mColor == ColorCode && Section->mPrimitiveType == PrimitiveType)
			break;
	}

	if (SectionIdx == mSections.GetSize())
	{
		Section = new lcLibraryMeshSection(PrimitiveType, ColorCode, Map.Texture);

		mSections.Add(Section);
	}

	int Indices[4] = { -1, -1, -1, -1 };

	for (int IndexIdx = 0; IndexIdx < LineType; IndexIdx++)
	{
		const lcVector3& Position = Vertices[IndexIdx];
		lcVector2 TexCoord(lcDot3(lcVector3(Position.x, Position.y, Position.z), Map.Params[0]) + Map.Params[0].w,
						   lcDot3(lcVector3(Position.x, Position.y, Position.z), Map.Params[1]) + Map.Params[1].w);

		for (int VertexIdx = mTexturedVertices.GetSize() - 1; VertexIdx >= 0; VertexIdx--)
		{
			lcVertexTextured& DstVertex = mTexturedVertices[VertexIdx];

			if (Position == DstVertex.Position && TexCoord == DstVertex.TexCoord)
			{
				Indices[IndexIdx] = VertexIdx;
				break;
			}
		}

		if (Indices[IndexIdx] == -1)
		{
			Indices[IndexIdx] = mTexturedVertices.GetSize();
			lcVertexTextured& DstVertex = mTexturedVertices.Add();
			DstVertex.Position = Position;
			DstVertex.TexCoord = TexCoord;
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

void lcLibraryMeshData::AddMeshData(const lcLibraryMeshData& Data, const lcMatrix44& Transform, lcuint32 CurrentColorCode, lcLibraryTextureMap* TextureMap)
{
	int VertexCount = Data.mVertices.GetSize();
	ObjArray<lcuint32> IndexRemap(VertexCount);

	if (!TextureMap)
	{
		mVertices.Expand(VertexCount);

		for (int SrcVertexIdx = 0; SrcVertexIdx < VertexCount; SrcVertexIdx++)
		{
			lcVector3 Position = lcMul31(Data.mVertices[SrcVertexIdx].Position, Transform);
			int Index = -1;

			for (int DstVertexIdx = mVertices.GetSize() - 1; DstVertexIdx >= 0; DstVertexIdx--)
			{
				lcVertex& DstVertex = mVertices[DstVertexIdx];

//				if (Vertex == mVertices[DstVertexIdx])
				if (fabsf(Position.x - DstVertex.Position.x) < 0.1f && fabsf(Position.y - DstVertex.Position.y) < 0.1f && fabsf(Position.z - DstVertex.Position.z) < 0.1f)
				{
					Index = DstVertexIdx;
					break;
				}
			}

			if (Index == -1)
			{
				Index = mVertices.GetSize();
				lcVertex& DstVertex = mVertices.Add();
				DstVertex.Position = Position;
			}

			IndexRemap.Add(Index);
		}
	}
	else
	{
		mTexturedVertices.Expand(VertexCount);

		for (int SrcVertexIdx = 0; SrcVertexIdx < VertexCount; SrcVertexIdx++)
		{
			lcVertex& SrcVertex = Data.mVertices[SrcVertexIdx];
			lcVector3 Position = lcMul31(SrcVertex.Position, Transform);
			lcVector2 TexCoord(lcDot3(lcVector3(Position.x, Position.y, Position.z), TextureMap->Params[0]) + TextureMap->Params[0].w,
			                   lcDot3(lcVector3(Position.x, Position.y, Position.z), TextureMap->Params[1]) + TextureMap->Params[1].w);
			int Index = -1;

			for (int DstVertexIdx = mTexturedVertices.GetSize() - 1; DstVertexIdx >= 0; DstVertexIdx--)
			{
				lcVertexTextured& DstVertex = mTexturedVertices[DstVertexIdx];

//				if (Vertex == mTexturedVertices[DstVertexIdx])
				if (fabsf(Position.x - DstVertex.Position.x) < 0.1f && fabsf(Position.y - DstVertex.Position.y) < 0.1f && fabsf(Position.z - DstVertex.Position.z) < 0.1f &&
					fabsf(TexCoord.x - DstVertex.TexCoord.x) < 0.01f && fabsf(TexCoord.y - DstVertex.TexCoord.y) < 0.01f)
				{
					Index = DstVertexIdx;
					break;
				}
			}

			if (Index == -1)
			{
				Index = mTexturedVertices.GetSize();
				lcVertexTextured& DstVertex = mTexturedVertices.Add();
				DstVertex.Position = Position;
				DstVertex.TexCoord = TexCoord;
			}

			IndexRemap.Add(Index);
		}
	}

	int TexturedVertexCount = Data.mTexturedVertices.GetSize();
	ObjArray<lcuint32> TexturedIndexRemap(TexturedVertexCount);

	if (TexturedVertexCount)
	{
		mTexturedVertices.Expand(TexturedVertexCount);

		for (int SrcVertexIdx = 0; SrcVertexIdx < TexturedVertexCount; SrcVertexIdx++)
		{
			lcVertexTextured& SrcVertex = Data.mTexturedVertices[SrcVertexIdx];
			lcVector3 Position = lcMul31(SrcVertex.Position, Transform);
			int Index = -1;

			for (int DstVertexIdx = mTexturedVertices.GetSize() - 1; DstVertexIdx >= 0; DstVertexIdx--)
			{
				lcVertexTextured& DstVertex = mTexturedVertices[DstVertexIdx];

//				if (Vertex == mTexturedVertices[DstVertexIdx])
				if (fabsf(Position.x - DstVertex.Position.x) < 0.1f && fabsf(Position.y - DstVertex.Position.y) < 0.1f && fabsf(Position.z - DstVertex.Position.z) < 0.1f &&
					fabsf(SrcVertex.TexCoord.x - DstVertex.TexCoord.x) < 0.01f && fabsf(SrcVertex.TexCoord.y - DstVertex.TexCoord.y) < 0.01f)
				{
					Index = DstVertexIdx;
					break;
				}
			}

			if (Index == -1)
			{
				Index = mTexturedVertices.GetSize();
				lcVertexTextured& DstVertex = mTexturedVertices.Add();
				DstVertex.Position = Position;
				DstVertex.TexCoord = SrcVertex.TexCoord;
			}

			TexturedIndexRemap.Add(Index);
		}
	}

	for (int SrcSectionIdx = 0; SrcSectionIdx < Data.mSections.GetSize(); SrcSectionIdx++)
	{
		lcLibraryMeshSection* SrcSection = Data.mSections[SrcSectionIdx];
		lcLibraryMeshSection* DstSection = NULL;
		lcuint32 ColorCode = SrcSection->mColor == 16 ? CurrentColorCode : SrcSection->mColor;
		lcTexture* Texture;

		if (SrcSection->mTexture)
			Texture = SrcSection->mTexture;
		else if (TextureMap)
			Texture = TextureMap->Texture;
		else
			Texture = NULL;

		for (int DstSectionIdx = 0; DstSectionIdx < mSections.GetSize(); DstSectionIdx++)
		{
			lcLibraryMeshSection* Section = mSections[DstSectionIdx];

			if (Section->mColor == ColorCode && Section->mPrimitiveType == SrcSection->mPrimitiveType && Section->mTexture == Texture)
			{
				DstSection = Section;
				break;
			}
		}

		if (!DstSection)
		{
			DstSection = new lcLibraryMeshSection(SrcSection->mPrimitiveType, ColorCode, Texture);

			mSections.Add(DstSection);
		}

		DstSection->mIndices.Expand(SrcSection->mIndices.GetSize());

		if (!SrcSection->mTexture)
		{
			for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
				DstSection->mIndices.Add(IndexRemap[SrcSection->mIndices[IndexIdx]]);
		}
		else
		{
			for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
				DstSection->mIndices.Add(TexturedIndexRemap[SrcSection->mIndices[IndexIdx]]);
		}
	}
}

void lcLibraryMeshData::AddMeshDataNoDuplicateCheck(const lcLibraryMeshData& Data, const lcMatrix44& Transform, lcuint32 CurrentColorCode, lcLibraryTextureMap* TextureMap)
{
	lcuint32 BaseIndex;

	if (!TextureMap)
	{
		BaseIndex = mVertices.GetSize();

		mVertices.Expand(Data.mVertices.GetSize());

		for (int SrcVertexIdx = 0; SrcVertexIdx < Data.mVertices.GetSize(); SrcVertexIdx++)
		{
			lcVertex& Vertex = mVertices.Add();
			Vertex.Position = lcMul31(Data.mVertices[SrcVertexIdx].Position, Transform);
		}
	}
	else
	{
		BaseIndex = mTexturedVertices.GetSize();

		mTexturedVertices.Expand(Data.mVertices.GetSize());

		for (int SrcVertexIdx = 0; SrcVertexIdx < Data.mVertices.GetSize(); SrcVertexIdx++)
		{
			lcVertex& SrcVertex = Data.mVertices[SrcVertexIdx];
			lcVertexTextured& DstVertex = mTexturedVertices.Add();

			lcVector3 Position = lcMul31(SrcVertex.Position, Transform);
			lcVector2 TexCoord(lcDot3(lcVector3(Position.x, Position.y, Position.z), TextureMap->Params[0]) + TextureMap->Params[0].w,
			                   lcDot3(lcVector3(Position.x, Position.y, Position.z), TextureMap->Params[1]) + TextureMap->Params[1].w);

			DstVertex.Position = Position;
			DstVertex.TexCoord = TexCoord;
		}
	}

	int TexturedVertexCount = Data.mTexturedVertices.GetSize();
	lcuint32 BaseTexturedIndex = mTexturedVertices.GetSize();

	if (TexturedVertexCount)
	{
		mTexturedVertices.Expand(TexturedVertexCount);

		for (int SrcVertexIdx = 0; SrcVertexIdx < TexturedVertexCount; SrcVertexIdx++)
		{
			lcVertexTextured& SrcVertex = Data.mTexturedVertices[SrcVertexIdx];
			lcVertexTextured& DstVertex = mTexturedVertices.Add();
			DstVertex.Position = lcMul31(SrcVertex.Position, Transform);
			DstVertex.TexCoord = SrcVertex.TexCoord;
		}
	}

	for (int SrcSectionIdx = 0; SrcSectionIdx < Data.mSections.GetSize(); SrcSectionIdx++)
	{
		lcLibraryMeshSection* SrcSection = Data.mSections[SrcSectionIdx];
		lcLibraryMeshSection* DstSection = NULL;
		lcuint32 ColorCode = SrcSection->mColor == 16 ? CurrentColorCode : SrcSection->mColor;
		lcTexture* Texture;

		if (SrcSection->mTexture)
			Texture = SrcSection->mTexture;
		else if (TextureMap)
			Texture = TextureMap->Texture;
		else
			Texture = NULL;

		for (int DstSectionIdx = 0; DstSectionIdx < mSections.GetSize(); DstSectionIdx++)
		{
			lcLibraryMeshSection* Section = mSections[DstSectionIdx];

			if (Section->mColor == ColorCode && Section->mPrimitiveType == SrcSection->mPrimitiveType && Section->mTexture == Texture)
			{
				DstSection = Section;
				break;
			}
		}

		if (!DstSection)
		{
			DstSection = new lcLibraryMeshSection(SrcSection->mPrimitiveType, ColorCode, Texture);

			mSections.Add(DstSection);
		}

		DstSection->mIndices.Expand(SrcSection->mIndices.GetSize());

		if (!SrcSection->mTexture)
		{
			for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
				DstSection->mIndices.Add(BaseIndex + SrcSection->mIndices[IndexIdx]);
		}
		else
		{
			for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
				DstSection->mIndices.Add(BaseTexturedIndex + SrcSection->mIndices[IndexIdx]);
		}
	}
}

bool lcPiecesLibrary::PieceInCategory(PieceInfo* Info, const String& CategoryKeywords) const
{
	String PieceName;
	if (Info->m_strDescription[0] == '~' || Info->m_strDescription[0] == '_')
		PieceName = Info->m_strDescription + 1;
	else
		PieceName = Info->m_strDescription;
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
		{ "Animal", "^%Animal | ^%Bone" },
		{ "Antenna", "^%Antenna" },
		{ "Arch", "^%Arch" },
		{ "Bar", "^%Bar" },
		{ "Baseplate", "^%Baseplate | ^%Platform" },
		{ "Boat", "^%Boat" },
		{ "Brick", "^%Brick" },
		{ "Container", "^%Container | ^%Box | ^Chest | ^%Storage | ^Mailbox" },
		{ "Door and Window", "^%Door | ^%Window | ^%Glass | ^%Freestyle | ^%Gate | ^%Garage | ^%Roller" },
		{ "Electric", "^%Electric" },
		{ "Hinge and Bracket", "^%Hinge | ^%Bracket | ^%Turntable" },
		{ "Hose", "^%Hose | ^%String" },
		{ "Minifig", "^%Minifig" },
		{ "Miscellaneous", "^%Arm | ^%Barrel | ^%Brush | ^%Claw | ^%Cockpit | ^%Conveyor | ^%Crane | ^%Cupboard | ^%Fence | ^%Jack | ^%Ladder | ^%Motor | ^%Rock | ^%Rope | ^%Sheet | ^%Sports | ^%Staircase | ^%Stretcher | ^%Tap | ^%Tipper | ^%Trailer | ^%Umbrella | ^%Winch" },
		{ "Other", "^%Ball | ^%Belville | ^%Die | ^%Duplo | ^%Fabuland | ^%Figure | ^%Homemaker | ^%Maxifig | ^%Microfig | ^%Mursten | ^%Scala | ^%Znap" },
		{ "Panel", "^%Panel | ^%Castle Wall | ^%Castle Turret" },
		{ "Plant", "^%Plant" },
		{ "Plate", "^%Plate" },
		{ "Round", "^%Cylinder | ^%Cone | ^%Dish | ^%Dome | ^%Hemisphere | ^%Round" },
		{ "Sign and Flag", "^%Flag | ^%Roadsign | ^%Streetlight | ^%Flagpost | ^%Lamppost | ^%Signpost" },
		{ "Slope", "^%Slope | ^%Roof" },
		{ "Space", "^%Space" },
		{ "Sticker", "^%Sticker" },
		{ "Support", "^%Support" },
		{ "Technic", "^%Technic | ^%Rack" },
		{ "Tile", "^%Tile" },
		{ "Train", "^%Train | ^%Monorail | ^%Magnet" },
		{ "Tyre and Wheel", "^%Tyre | %^Wheel | %^Wheels | ^%Castle Wagon" },
		{ "Vehicle", "^%Bike | ^%Canvas | ^%Car | ^%Excavator | ^%Exhaust | ^%Forklift | ^%Grab Jaw | ^%Landing | ^%Motorcycle | ^%Plane | ^%Propellor | ^%Tail | ^%Tractor | ^%Vehicle | ^%Wheelbarrow" },
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
