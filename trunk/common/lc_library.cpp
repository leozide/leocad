#include "lc_global.h"
#include "lc_library.h"
#include "lc_zipfile.h"
#include "lc_file.h"
#include "pieceinf.h"
#include "lc_colors.h"
#include "lc_texture.h"
#include "lc_category.h"
#include "lc_application.h"
#include "lc_mainwindow.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <locale.h>

#define LC_LIBRARY_CACHE_VERSION   0x0103
#define LC_LIBRARY_CACHE_ARCHIVE   0x0001
#define LC_LIBRARY_CACHE_DIRECTORY 0x0002

lcPiecesLibrary::lcPiecesLibrary()
{
	mNumOfficialPieces = 0;
	mLibraryPath[0] = 0;
	mCacheFileName[0] = 0;
	mCacheFileModifiedTime = 0;
	mLibraryFileName[0] = 0;
	mUnofficialFileName[0] = 0;
	mZipFiles[LC_ZIPFILE_OFFICIAL] = NULL;
	mZipFiles[LC_ZIPFILE_UNOFFICIAL] = NULL;
	mCacheFile = NULL;
	mCacheFileName[0] = 0;
	mSaveCache = false;
}

lcPiecesLibrary::~lcPiecesLibrary()
{
	Unload();
}

void lcPiecesLibrary::Unload()
{
	SaveCacheFile();

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
		delete mPieces[PieceIdx];
	mPieces.RemoveAll();

	for (int PrimitiveIdx = 0; PrimitiveIdx < mPrimitives.GetSize(); PrimitiveIdx++)
		delete mPrimitives[PrimitiveIdx];
	mPrimitives.RemoveAll();

	for (int TextureIdx = 0; TextureIdx < mTextures.GetSize(); TextureIdx++)
		delete mTextures[TextureIdx];
	mTextures.RemoveAll();

	mNumOfficialPieces = 0;
	delete mZipFiles[LC_ZIPFILE_OFFICIAL];
	mZipFiles[LC_ZIPFILE_OFFICIAL] = NULL;
	delete mZipFiles[LC_ZIPFILE_UNOFFICIAL];
	mZipFiles[LC_ZIPFILE_UNOFFICIAL] = NULL;
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
	PieceInfo* Info = new PieceInfo();

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

bool lcPiecesLibrary::Load(const char* LibraryPath, const char* CachePath)
{
	Unload();

	if (OpenArchive(LibraryPath, LC_ZIPFILE_OFFICIAL))
	{
		lcMemFile ColorFile;

		if (!mZipFiles[LC_ZIPFILE_OFFICIAL]->ExtractFile("ldraw/ldconfig.ldr", ColorFile) || !lcLoadColorFile(ColorFile))
			lcLoadDefaultColors();

		strcpy(mLibraryPath, LibraryPath);
		char* Slash = lcMax(strrchr(mLibraryPath, '/'), strrchr(mLibraryPath, '\\'));
		if (*Slash)
			*(Slash + 1) = 0;

		char UnofficialFileName[LC_MAXPATH];
		strcpy(UnofficialFileName, mLibraryPath);
		strcat(UnofficialFileName, "/ldrawunf.zip");

		OpenArchive(UnofficialFileName, LC_ZIPFILE_UNOFFICIAL);

		ReadArchiveDescriptions(LibraryPath, UnofficialFileName, CachePath);
	}
	else
	{
		strcpy(mLibraryPath, LibraryPath);

		int i = strlen(mLibraryPath) - 1;
		if ((mLibraryPath[i] != '\\') && (mLibraryPath[i] != '/'))
			strcat(mLibraryPath, "/");

		if (OpenDirectory(mLibraryPath))
		{
			char FileName[LC_MAXPATH];
			lcDiskFile ColorFile;

			sprintf(FileName, "%sldconfig.ldr", mLibraryPath);

			if (!ColorFile.Open(FileName, "rt") || !lcLoadColorFile(ColorFile))
				lcLoadDefaultColors();
		}
		else
			return false;
	}

	lcLoadDefaultCategories();

	return true;
}

bool lcPiecesLibrary::OpenArchive(const char* FileName, lcZipFileType ZipFileType)
{
	lcDiskFile* File = new lcDiskFile();

	if (!File->Open(FileName, "rb") || !OpenArchive(File, FileName, ZipFileType))
	{
		delete File;
		return false;
	}

	return true;
}

bool lcPiecesLibrary::OpenArchive(lcFile* File, const char* FileName, lcZipFileType ZipFileType)
{
	lcZipFile* ZipFile = new lcZipFile();

	if (!ZipFile->OpenRead(File))
	{
		delete ZipFile;
		return false;
	}

	mZipFiles[ZipFileType] = ZipFile;

	if (ZipFileType == LC_ZIPFILE_OFFICIAL)
		strcpy(mLibraryFileName, FileName);
	else
		strcpy(mUnofficialFileName, FileName);

	for (int FileIdx = 0; FileIdx < ZipFile->mFiles.GetSize(); FileIdx++)
	{
		lcZipFileInfo& FileInfo = ZipFile->mFiles[FileIdx];
		char NameBuffer[LC_PIECE_NAME_LEN];
		char* Name = NameBuffer;

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

				*Dst = 0;
				strncpy(Texture->mName, Name + 21, sizeof(Texture->mName));
				Texture->mName[sizeof(Texture->mName) - 1] = 0;
			}

			continue;
		}
		*Dst = 0;

		if (ZipFileType == LC_ZIPFILE_OFFICIAL)
		{
			if (memcmp(Name, "LDRAW/", 6))
				continue;

			Name += 6;
		}

		if (!memcmp(Name, "PARTS/", 6))
		{
			Name += 6;

			if (memcmp(Name, "S/", 2))
			{
				PieceInfo* Info = FindPiece(Name, false);

				if (!Info)
				{
					Info = new PieceInfo();
					mPieces.Add(Info);

					strncpy(Info->m_strName, Name, sizeof(Info->m_strName));
					Info->m_strName[sizeof(Info->m_strName) - 1] = 0;
				}

				Info->SetZipFile(ZipFileType, FileIdx);
			}
			else
			{
				int PrimitiveIndex = FindPrimitiveIndex(Name);

				if (PrimitiveIndex == -1)
					mPrimitives.Add(new lcLibraryPrimitive(Name, ZipFileType, FileIdx, false, true));
				else
					mPrimitives[PrimitiveIndex]->SetZipFile(ZipFileType, FileIdx);
			}
		}
		else if (!memcmp(Name, "P/", 2))
		{
			Name += 2;

			int PrimitiveIndex = FindPrimitiveIndex(Name);

			if (PrimitiveIndex == -1)
				mPrimitives.Add(new lcLibraryPrimitive(Name, ZipFileType, FileIdx, (memcmp(Name, "STU", 3) == 0), false));
			else
				mPrimitives[PrimitiveIndex]->SetZipFile(ZipFileType, FileIdx);
		}
	}

	return true;
}

void lcPiecesLibrary::ReadArchiveDescriptions(const char* OfficialFileName, const char* UnofficialFileName, const char* CachePath)
{
	bool CacheValid = false;
	struct stat OfficialStat, UnofficialStat;

	strcpy(mCacheFileName, CachePath);
	mCacheFileModifiedTime = 0;

	if (mCacheFileName[0])
	{
		int Length = strlen(mCacheFileName);
		if (mCacheFileName[Length] != '/' && mCacheFileName[Length] != '\\')
			strcat(mCacheFileName, "/");

		strcat(mCacheFileName, "library.cache");
	}

	if (stat(OfficialFileName, &OfficialStat) == 0)
	{
		lcuint64 CheckSum[4] =
		{
			(lcuint64)OfficialStat.st_size, (lcuint64)OfficialStat.st_mtime, 0, 0
		};

		if (stat(UnofficialFileName, &UnofficialStat) == 0)
		{
			CheckSum[2] = (lcuint64)UnofficialStat.st_size;
			CheckSum[3] = (lcuint64)UnofficialStat.st_mtime;
		}

		lcZipFile CacheFile;

		if (CacheFile.OpenRead(mCacheFileName))
		{
			lcMemFile VersionFile;

			if (CacheFile.ExtractFile("version", VersionFile))
			{
				lcuint32 CacheVersion;
				lcuint32 CacheFlags;

				if (VersionFile.ReadU32(&CacheVersion, 1) && VersionFile.ReadU32(&CacheFlags, 1) &&
					CacheVersion == LC_LIBRARY_CACHE_VERSION && CacheFlags == LC_LIBRARY_CACHE_ARCHIVE)
				{
					lcuint64 CacheCheckSum[4];

					if (VersionFile.ReadU64(CacheCheckSum, 4))
						CacheValid = (memcmp(CacheCheckSum, CheckSum, sizeof(CheckSum)) == 0);
				}
			}
		}

		if (CacheValid)
			CacheValid = LoadCacheIndex(CacheFile);
	}

	if (CacheValid)
	{
		struct stat CacheStat;

		if (stat(mCacheFileName, &CacheStat) == 0)
			mCacheFileModifiedTime = CacheStat.st_mtime;
	}
	else
	{
		lcMemFile PieceFile;

		mSaveCache = true;

		for (int PieceInfoIndex = 0; PieceInfoIndex < mPieces.GetSize(); PieceInfoIndex++)
		{
			PieceInfo* Info = mPieces[PieceInfoIndex];

			mZipFiles[Info->mZipFileType]->ExtractFile(Info->mZipFileIndex, PieceFile, 256);
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
	}
}

bool lcPiecesLibrary::OpenDirectory(const char* Path)
{
	char FileName[LC_MAXPATH];
	lcArray<String> FileList;

	strcpy(FileName, Path);
	strcat(FileName, "parts.lst");

	lcDiskFile PartsList;

	if (PartsList.Open(FileName, "rt"))
	{
		char Line[1024];

		while (PartsList.ReadLine(Line, sizeof(Line)))
		{
			char* Chr = Line;
			char* Ext = NULL;

			while (*Chr)
			{
				if (*Chr >= 'a' && *Chr <= 'z')
					*Chr = *Chr + 'A' - 'a';
				else if (*Chr == '.')
					Ext = Chr;
				else if (isspace(*Chr))
				{
					*Chr++ = 0;
					break;
				}

				Chr++;
			}

			if (Ext && !strcmp(Ext, ".DAT"))
				*Ext = 0;

			while (*Chr && isspace(*Chr))
				Chr++;

			char* Description = Chr;

			while (*Chr)
			{
				if (*Chr == '\r' || *Chr == '\n')
				{
					*Chr = 0;
					break;
				}

				Chr++;
			}

			if (!*Line || !*Description)
				continue;

			PieceInfo* Info = new PieceInfo();
			mPieces.Add(Info);

			strncpy(Info->m_strName, Line, sizeof(Info->m_strName));
			Info->m_strName[sizeof(Info->m_strName) - 1] = 0;

			strncpy(Info->m_strDescription, Description, sizeof(Info->m_strDescription));
			Info->m_strDescription[sizeof(Info->m_strDescription) - 1] = 0;
		}
	}

	if (!mPieces.GetSize())
	{
		strcpy(FileName, Path);
		strcat(FileName, "parts/");
		int PathLength = strlen(FileName);

		g_App->GetFileList(FileName, FileList);

		mPieces.AllocGrow(FileList.GetSize());

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

			PieceInfo* Info = new PieceInfo();
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
	}

	if (!mPieces.GetSize())
		return false;

	const char* PrimitiveDirectories[] = { "p/", "p/48/", "parts/s/" };
	bool SubFileDirectories[] = { false, false, true };

	for (int DirectoryIdx = 0; DirectoryIdx < (int)(sizeof(PrimitiveDirectories) / sizeof(PrimitiveDirectories[0])); DirectoryIdx++)
	{
		strcpy(FileName, Path);
		int PathLength = strlen(FileName);

		strcat(FileName, PrimitiveDirectories[DirectoryIdx]);
		PathLength += strchr(PrimitiveDirectories[DirectoryIdx], '/') - PrimitiveDirectories[DirectoryIdx] + 1;

		g_App->GetFileList(FileName, FileList);

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
			lcLibraryPrimitive* Prim = new lcLibraryPrimitive(Name, LC_NUM_ZIPFILES, 0, !SubFile && (memcmp(Name, "STU", 3) == 0), SubFile);
			mPrimitives.Add(Prim);
		}
	}

	strcpy(FileName, Path);
	strcat(FileName, "parts/textures/");
	int PathLength = strlen(FileName);

	g_App->GetFileList(FileName, FileList);

	mTextures.AllocGrow(FileList.GetSize());

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

bool lcPiecesLibrary::OpenCache()
{
	struct stat CacheStat;

	if (!mCacheFileName[0])
		return false;

	if (stat(mCacheFileName, &CacheStat) != 0 || mCacheFileModifiedTime != (lcuint64)CacheStat.st_mtime)
		return false;

	mCacheFile = new lcZipFile;

	if (!mCacheFile->OpenRead(mCacheFileName))
	{
		delete mCacheFile;
		mCacheFile = NULL;

		return false;
	}

	return true;
}

void lcPiecesLibrary::CloseCache()
{
	delete mCacheFile;
	mCacheFile = NULL;

	SaveCacheFile();
}

bool lcPiecesLibrary::LoadCacheIndex(lcZipFile& CacheFile)
{
	lcMemFile IndexFile;

	if (!CacheFile.ExtractFile("index", IndexFile))
		return false;

	lcuint32 NumFiles;

	if (!IndexFile.ReadU32(&NumFiles, 1) || NumFiles != (lcuint32)mPieces.GetSize())
		return false;

	for (int PieceInfoIndex = 0; PieceInfoIndex < mPieces.GetSize(); PieceInfoIndex++)
	{
		PieceInfo* Info = mPieces[PieceInfoIndex];
		lcuint8 Length;

		if (!IndexFile.ReadU8(&Length, 1) || Length >= sizeof(Info->m_strDescription))
			return false;

		if (!IndexFile.ReadBuffer(Info->m_strDescription, Length) || !IndexFile.ReadU32(&Info->mFlags, 1))
			return false;

		Info->m_strDescription[Length] = 0;

		if (!IndexFile.ReadFloats(Info->m_fDimensions, 6))
			return false;
	}

	return true;
}

bool lcPiecesLibrary::LoadCachePiece(PieceInfo* Info)
{
	if ((Info->mFlags & LC_PIECE_CACHED) == 0)
		return false;

	if (mCacheFile)
	{
		lcMemFile PieceFile;

		if (!mCacheFile->ExtractFile(Info->m_strName, PieceFile))
			return false;

		Info->mMesh = new lcMesh;

		return Info->mMesh->FileLoad(PieceFile);
	}
	else
	{
		struct stat CacheStat;

		if (stat(mCacheFileName, &CacheStat) != 0 || mCacheFileModifiedTime != (lcuint64)CacheStat.st_mtime)
			return false;

		lcZipFile CacheFile;

		if (!CacheFile.OpenRead(mCacheFileName))
			return false;

		lcMemFile PieceFile;

		if (!CacheFile.ExtractFile(Info->m_strName, PieceFile))
			return false;

		Info->mMesh = new lcMesh;

		return Info->mMesh->FileLoad(PieceFile);
	}
}

void lcPiecesLibrary::SaveCacheFile()
{
	struct stat CacheStat;
	lcZipFile CacheFile;

	if (!mSaveCache)
		return;

	if (stat(mCacheFileName, &CacheStat) != 0 || mCacheFileModifiedTime != (lcuint64)CacheStat.st_mtime)
	{
		if (!CacheFile.OpenWrite(mCacheFileName, false))
			return;

		struct stat OfficialStat, UnofficialStat;

		if (stat(mLibraryFileName, &OfficialStat) != 0)
			return;

		lcuint64 CheckSum[4] =
		{
			(lcuint64)OfficialStat.st_size, (lcuint64)OfficialStat.st_mtime, 0, 0
		};

		if (stat(mUnofficialFileName, &UnofficialStat) == 0)
		{
			CheckSum[2] = (lcuint64)UnofficialStat.st_size;
			CheckSum[3] = (lcuint64)UnofficialStat.st_mtime;
		}

		lcMemFile VersionFile;

		VersionFile.WriteU32(LC_LIBRARY_CACHE_VERSION);
		VersionFile.WriteU32(LC_LIBRARY_CACHE_ARCHIVE);
		VersionFile.WriteU64(CheckSum, 4);

		CacheFile.AddFile("version", VersionFile);
	}
	else
	{
		if (!CacheFile.OpenWrite(mCacheFileName, true))
			return;

		CacheFile.DeleteFile("index");
	}

	lcMemFile IndexFile;
	int NumPieces = 0;

	IndexFile.WriteU32(0);

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		PieceInfo* Info = mPieces[PieceIdx];

		if (Info->mFlags & LC_PIECE_PLACEHOLDER)
			continue;

		bool Cached = (Info->mFlags & LC_PIECE_CACHED) != 0;

		if (Info->mMesh)
			Info->mFlags |= LC_PIECE_CACHED;

		int Length = strlen(Info->m_strDescription);

		IndexFile.WriteU8(Length);
		IndexFile.WriteBuffer(Info->m_strDescription, Length);
		IndexFile.WriteU32(Info->mFlags);
		IndexFile.WriteFloats(Info->m_fDimensions, 6);

		NumPieces++;

		if (Cached || !Info->mMesh)
			continue;

		lcMemFile PieceFile;

		Info->mMesh->FileSave(PieceFile);
		CacheFile.AddFile(Info->m_strName, PieceFile);

		Info->mFlags |= LC_PIECE_CACHED;
		Info->Release();
	}

	IndexFile.Seek(0, SEEK_SET);
	IndexFile.WriteU32(mPieces.GetSize());

	CacheFile.AddFile("index", IndexFile);

	mSaveCache = false;
}

int LibraryMeshSectionCompare(lcLibraryMeshSection* const& a, lcLibraryMeshSection* const& b)
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
	lcArray<lcLibraryTextureMap> TextureStack;

	if (Info->mZipFileType != LC_NUM_ZIPFILES && mZipFiles[Info->mZipFileType])
	{
		if (LoadCachePiece(Info))
			return true;

		lcMemFile PieceFile;

		if (!mZipFiles[Info->mZipFileType]->ExtractFile(Info->mZipFileIndex, PieceFile))
			return false;

		const char* OldLocale = setlocale(LC_NUMERIC, "C");
		bool Ret = ReadMeshData(PieceFile, lcMatrix44Identity(), 16, TextureStack, MeshData);
		setlocale(LC_NUMERIC, OldLocale);

        if (!Ret)
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

		const char* OldLocale = setlocale(LC_NUMERIC, "C");
		bool Ret = ReadMeshData(PieceFile, lcMatrix44Identity(), 16, TextureStack, MeshData);
		setlocale(LC_NUMERIC, OldLocale);

        if (!Ret)
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

	MeshData.mSections.Sort(LibraryMeshSectionCompare);

	Mesh->Create(MeshData.mSections.GetSize(), MeshData.mVertices.GetSize(), MeshData.mTexturedVertices.GetSize(), NumIndices);

	lcVertex* DstVerts = (lcVertex*)Mesh->mVertexBuffer.mData;
	lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (int VertexIdx = 0; VertexIdx < MeshData.mVertices.GetSize(); VertexIdx++)
	{
		lcVertex& DstVertex = *DstVerts++;

		const lcVector3& SrcPosition = MeshData.mVertices[VertexIdx].Position;
		lcVector3& DstPosition = DstVertex.Position;

		DstPosition = lcVector3(SrcPosition.x, SrcPosition.z, -SrcPosition.y);

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

		DstPosition = lcVector3(SrcPosition.x, SrcPosition.z, -SrcPosition.y);
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
			if (DstSection.ColorIndex == gDefaultColor)
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
	Info->AddRef();

	if (mZipFiles[LC_ZIPFILE_OFFICIAL])
		mSaveCache = true;

	return true;
}

bool lcPiecesLibrary::LoadTexture(lcTexture* Texture)
{
	char Name[LC_MAXPATH], FileName[LC_MAXPATH];

	strcpy(Name, Texture->mName);
	strlwr(Name);

	if (mZipFiles[LC_ZIPFILE_OFFICIAL])
	{
		lcMemFile TextureFile;

		sprintf(FileName, "ldraw/parts/textures/%s.png", Name);

		if (!mZipFiles[LC_ZIPFILE_UNOFFICIAL] || !mZipFiles[LC_ZIPFILE_UNOFFICIAL]->ExtractFile(FileName, TextureFile))
			if (!mZipFiles[LC_ZIPFILE_OFFICIAL]->ExtractFile(FileName, TextureFile))
				return false;

		if (!Texture->Load(TextureFile))
			return false;
	}
	else
	{
		sprintf(FileName, "%sparts/textures/%s.png", mLibraryPath, Name);

		if (!Texture->Load(FileName))
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
	lcArray<lcLibraryTextureMap> TextureStack;

	if (mZipFiles[LC_ZIPFILE_OFFICIAL])
	{
		lcMemFile PrimFile;

		if (!mZipFiles[Primitive->mZipFileType]->ExtractFile(Primitive->mZipFileIndex, PrimFile))
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

bool lcPiecesLibrary::ReadMeshData(lcFile& File, const lcMatrix44& CurrentTransform, lcuint32 CurrentColorCode, lcArray<lcLibraryTextureMap>& TextureStack, lcLibraryMeshData& MeshData)
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
						if (mZipFiles[LC_ZIPFILE_OFFICIAL])
						{
							lcMemFile IncludeFile;

							if (!mZipFiles[Primitive->mZipFileType]->ExtractFile(Primitive->mZipFileIndex, IncludeFile))
								continue;

							if (!ReadMeshData(IncludeFile, IncludeTransform, ColorCode, TextureStack, MeshData))
								continue;
						}
						else
						{
							char Name[LC_PIECE_NAME_LEN];
							strcpy(Name, Primitive->mName);
							strlwr(Name);

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

						if (mZipFiles[LC_ZIPFILE_OFFICIAL])
						{
							lcMemFile IncludeFile;

							if (!mZipFiles[Info->mZipFileType]->ExtractFile(Info->mZipFileIndex, IncludeFile))
								break;

							if (!ReadMeshData(IncludeFile, IncludeTransform, ColorCode, TextureStack, MeshData))
								break;
						}
						else
						{
							char Name[LC_PIECE_NAME_LEN];
							strcpy(Name, Info->m_strName);
							strlwr(Name);

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

void lcLibraryMeshData::ResequenceQuad(lcVector3* Vertices, int a, int b, int c, int d)
{
	lcVector3 TempVertices[4];

	memcpy(TempVertices, Vertices, sizeof(TempVertices));

	Vertices[0] = TempVertices[a];
	Vertices[1] = TempVertices[b];
	Vertices[2] = TempVertices[c];
	Vertices[3] = TempVertices[d];
}

void lcLibraryMeshData::TestQuad(lcVector3* Vertices)
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
			ResequenceQuad(Vertices, 1, 2, 3, 0);
		else
			ResequenceQuad(Vertices, 0, 3, 1, 2);
	}
	else
	{
		if (-lcDot(lcCross(v02, v12), lcCross(v12, v23)) > 0.0f)
			ResequenceQuad(Vertices, 0, 1, 3, 2);
		else
			ResequenceQuad(Vertices, 1, 2, 3, 0);
	}
}

void lcLibraryMeshData::AddLine(int LineType, lcuint32 ColorCode, lcVector3* Vertices)
{
	lcLibraryMeshSection* Section = NULL;
	int SectionIdx;
	LC_MESH_PRIMITIVE_TYPE PrimitiveType = (LineType == 2) ? LC_MESH_LINES : LC_MESH_TRIANGLES;

	for (SectionIdx = 0; SectionIdx < mSections.GetSize(); SectionIdx++)
	{
		Section = mSections[SectionIdx];

		if (Section->mColor == ColorCode && Section->mPrimitiveType == PrimitiveType && Section->mTexture == NULL)
			break;
	}

	if (SectionIdx == mSections.GetSize())
	{
		Section = new lcLibraryMeshSection(PrimitiveType, ColorCode, NULL);

		mSections.Add(Section);
	}

	if (LineType == 4)
		TestQuad(Vertices);

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

void lcLibraryMeshData::AddTexturedLine(int LineType, lcuint32 ColorCode, const lcLibraryTextureMap& Map, lcVector3* Vertices)
{
	lcLibraryMeshSection* Section = NULL;
	int SectionIdx;
	LC_MESH_PRIMITIVE_TYPE PrimitiveType = (LineType == 2) ? LC_MESH_TEXTURED_LINES : LC_MESH_TEXTURED_TRIANGLES;

	for (SectionIdx = 0; SectionIdx < mSections.GetSize(); SectionIdx++)
	{
		Section = mSections[SectionIdx];

		if (Section->mColor == ColorCode && Section->mPrimitiveType == PrimitiveType && Section->mTexture == Map.Texture)
			break;
	}

	if (SectionIdx == mSections.GetSize())
	{
		Section = new lcLibraryMeshSection(PrimitiveType, ColorCode, Map.Texture);

		mSections.Add(Section);
	}

	if (LineType == 4)
		TestQuad(Vertices);

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
	lcArray<lcuint32> IndexRemap(VertexCount);
	const float DistanceEpsilon = 0.05f;

	if (!TextureMap)
	{
		mVertices.AllocGrow(VertexCount);

		for (int SrcVertexIdx = 0; SrcVertexIdx < VertexCount; SrcVertexIdx++)
		{
			lcVector3 Position = lcMul31(Data.mVertices[SrcVertexIdx].Position, Transform);
			int Index = -1;

			for (int DstVertexIdx = mVertices.GetSize() - 1; DstVertexIdx >= 0; DstVertexIdx--)
			{
				lcVertex& DstVertex = mVertices[DstVertexIdx];

//				if (Vertex == mVertices[DstVertexIdx])
				if (fabsf(Position.x - DstVertex.Position.x) < DistanceEpsilon && fabsf(Position.y - DstVertex.Position.y) < DistanceEpsilon && fabsf(Position.z - DstVertex.Position.z) < DistanceEpsilon)
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
		mTexturedVertices.AllocGrow(VertexCount);

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
				if (fabsf(Position.x - DstVertex.Position.x) < DistanceEpsilon && fabsf(Position.y - DstVertex.Position.y) < DistanceEpsilon && fabsf(Position.z - DstVertex.Position.z) < DistanceEpsilon &&
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
	lcArray<lcuint32> TexturedIndexRemap(TexturedVertexCount);

	if (TexturedVertexCount)
	{
		mTexturedVertices.AllocGrow(TexturedVertexCount);

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

		DstSection->mIndices.AllocGrow(SrcSection->mIndices.GetSize());

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

		mVertices.AllocGrow(Data.mVertices.GetSize());

		for (int SrcVertexIdx = 0; SrcVertexIdx < Data.mVertices.GetSize(); SrcVertexIdx++)
		{
			lcVertex& Vertex = mVertices.Add();
			Vertex.Position = lcMul31(Data.mVertices[SrcVertexIdx].Position, Transform);
		}
	}
	else
	{
		BaseIndex = mTexturedVertices.GetSize();

		mTexturedVertices.AllocGrow(Data.mVertices.GetSize());

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
		mTexturedVertices.AllocGrow(TexturedVertexCount);

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

		DstSection->mIndices.AllocGrow(SrcSection->mIndices.GetSize());

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

void lcPiecesLibrary::GetCategoryEntries(int CategoryIndex, bool GroupPieces, lcArray<PieceInfo*>& SinglePieces, lcArray<PieceInfo*>& GroupedPieces)
{
	if (gCategories[CategoryIndex].Name == "Search Results")
		GroupPieces = false;

	SearchPieces(gCategories[CategoryIndex].Keywords, GroupPieces, SinglePieces, GroupedPieces);
}

void lcPiecesLibrary::SearchPieces(const String& CategoryKeywords, bool GroupPieces, lcArray<PieceInfo*>& SinglePieces, lcArray<PieceInfo*>& GroupedPieces)
{
	SinglePieces.RemoveAll();
	GroupedPieces.RemoveAll();

	for (int i = 0; i < mPieces.GetSize(); i++)
	{
		PieceInfo* Info = mPieces[i];

		if (!PieceInCategory(Info, CategoryKeywords))
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

void lcPiecesLibrary::GetPatternedPieces(PieceInfo* Parent, lcArray<PieceInfo*>& Pieces) const
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

bool lcPiecesLibrary::LoadBuiltinPieces()
{
	QResource Resource(":/resources/library.zip");

	if (!Resource.isValid())
		return false;

	lcMemFile* File = new lcMemFile();
	File->WriteBuffer(Resource.data(), Resource.size());

	if (!OpenArchive(File, "builtin", LC_ZIPFILE_OFFICIAL))
	{
		delete File;
		return false;
	}

	lcMemFile PieceFile;

	mSaveCache = false;

	for (int PieceInfoIndex = 0; PieceInfoIndex < mPieces.GetSize(); PieceInfoIndex++)
	{
		PieceInfo* Info = mPieces[PieceInfoIndex];

		mZipFiles[Info->mZipFileType]->ExtractFile(Info->mZipFileIndex, PieceFile, 256);
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

	lcLoadDefaultColors();

	lcLoadDefaultCategories(true);
	gMainWindow->UpdateCategories();

	return true;
}
