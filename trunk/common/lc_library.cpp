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
#include "lc_context.h"
#include "lc_glextensions.h"
#include "project.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <locale.h>
#include <zlib.h>

#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif

#define LC_LIBRARY_CACHE_VERSION   0x0104
#define LC_LIBRARY_CACHE_ARCHIVE   0x0001
#define LC_LIBRARY_CACHE_DIRECTORY 0x0002

lcPiecesLibrary::lcPiecesLibrary()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	QStringList cachePathList = QStandardPaths::standardLocations(QStandardPaths::CacheLocation);
	mCachePath = cachePathList.first();
#else
	mCachePath  = QDesktopServices::storageLocation(QDesktopServices::CacheLocation);
#endif

	QDir Dir;
	Dir.mkpath(mCachePath);

	mNumOfficialPieces = 0;
	mLibraryPath[0] = 0;
	mLibraryFileName[0] = 0;
	mUnofficialFileName[0] = 0;
	mZipFiles[LC_ZIPFILE_OFFICIAL] = NULL;
	mZipFiles[LC_ZIPFILE_UNOFFICIAL] = NULL;
	mBuffersDirty = false;
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

	mNumOfficialPieces = 0;
	delete mZipFiles[LC_ZIPFILE_OFFICIAL];
	mZipFiles[LC_ZIPFILE_OFFICIAL] = NULL;
	delete mZipFiles[LC_ZIPFILE_UNOFFICIAL];
	mZipFiles[LC_ZIPFILE_UNOFFICIAL] = NULL;
}

void lcPiecesLibrary::RemoveTemporaryPieces()
{
	for (int PieceIdx = mPieces.GetSize() - 1; PieceIdx >= 0; PieceIdx--)
	{
		PieceInfo* Info = mPieces[PieceIdx];

		if (!Info->IsTemporary())
			break;

		if (!Info->IsLoaded())
		{
			mPieces.RemoveIndex(PieceIdx);
			delete Info;
		}
	}
}

void lcPiecesLibrary::RemovePiece(PieceInfo* Info)
{
	mPieces.Remove(Info);
	delete Info;
}

PieceInfo* lcPiecesLibrary::FindPiece(const char* PieceName, Project* Project, bool CreatePlaceholder)
{
	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		PieceInfo* Info = mPieces[PieceIdx];

		if (strcmp(PieceName, Info->m_strName))
			continue;

		if (Project && Info->IsModel() && Project->GetModels().FindIndex(Info->GetModel()) == -1)
			continue;

		return Info;
	}

	if (CreatePlaceholder)
	{
		PieceInfo* Info = new PieceInfo();

		Info->CreatePlaceholder(PieceName);
		mPieces.Add(Info);

		return Info;
	}

	return NULL;
}

lcTexture* lcPiecesLibrary::FindTexture(const char* TextureName)
{
	for (int TextureIdx = 0; TextureIdx < mTextures.GetSize(); TextureIdx++)
		if (!strcmp(TextureName, mTextures[TextureIdx]->mName))
			return mTextures[TextureIdx];

	return NULL;
}

bool lcPiecesLibrary::Load(const char* LibraryPath)
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

		ReadArchiveDescriptions(LibraryPath, UnofficialFileName);
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
				PieceInfo* Info = FindPiece(Name, NULL, false);

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

void lcPiecesLibrary::ReadArchiveDescriptions(const QString& OfficialFileName, const QString& UnofficialFileName)
{
	QFileInfo OfficialInfo(OfficialFileName);
	QFileInfo UnofficialInfo(OfficialFileName);
	
	mArchiveCheckSum[0] = OfficialInfo.size();
	mArchiveCheckSum[1] = OfficialInfo.lastModified().toMSecsSinceEpoch();
	mArchiveCheckSum[2] = UnofficialInfo.size();
	mArchiveCheckSum[3] = UnofficialInfo.lastModified().toMSecsSinceEpoch();

	QString IndexFileName = QFileInfo(QDir(mCachePath), QLatin1String("index")).absoluteFilePath();

	if (!LoadCacheIndex(IndexFileName))
	{
		lcMemFile PieceFile;

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

		SaveCacheIndex(IndexFileName);
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

bool lcPiecesLibrary::ReadCacheFile(const QString& FileName, lcMemFile& CacheFile)
{
	QFile File(FileName);

	if (!File.open(QIODevice::ReadOnly))
		return false;

	quint32 CacheVersion, CacheFlags;
	
	if (File.read((char*)&CacheVersion, sizeof(CacheVersion)) == -1 || CacheVersion != LC_LIBRARY_CACHE_VERSION)
		return false;

	if (File.read((char*)&CacheFlags, sizeof(CacheFlags)) == -1 || CacheFlags != LC_LIBRARY_CACHE_ARCHIVE)
		return false;

	qint64 CacheCheckSum[4];

	if (File.read((char*)&CacheCheckSum, sizeof(CacheCheckSum)) == -1 || memcmp(CacheCheckSum, mArchiveCheckSum, sizeof(CacheCheckSum)))
		return false;

	quint32 UncompressedSize;

	if (File.read((char*)&UncompressedSize, sizeof(UncompressedSize)) == -1)
		return false;

	QByteArray CompressedData = File.readAll();

	CacheFile.SetLength(UncompressedSize);
	CacheFile.Seek(0, SEEK_SET);

	const int CHUNK = 16384;
	int ret;
	unsigned have;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];
	int pos;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	pos = 0;

	ret = inflateInit2(&strm, -MAX_WBITS);
	if (ret != Z_OK)
		return ret;

	do
	{
		strm.avail_in = lcMin(CompressedData.size() - pos, CHUNK);
		strm.next_in = in;

		if (strm.avail_in == 0)
			break;

		memcpy(in, CompressedData.constData() + pos, strm.avail_in);
		pos += strm.avail_in;

		do
		{
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = inflate(&strm, Z_NO_FLUSH);

			switch (ret)
			{
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				(void)inflateEnd(&strm);
				return ret;
			}

			have = CHUNK - strm.avail_out;
			CacheFile.WriteBuffer(out, have);
		} while (strm.avail_out == 0);
	} while (ret != Z_STREAM_END);

	(void)inflateEnd(&strm);
	
	CacheFile.Seek(0, SEEK_SET);

	return ret == Z_STREAM_END;
}

bool lcPiecesLibrary::WriteCacheFile(const QString& FileName, lcMemFile& CacheFile)
{
	QFile File(FileName);

	if (!File.open(QIODevice::WriteOnly))
		return false;

	quint32 CacheVersion = LC_LIBRARY_CACHE_VERSION;
	quint32 CacheFlags = LC_LIBRARY_CACHE_ARCHIVE;
	
	if (File.write((char*)&CacheVersion, sizeof(CacheVersion)) == -1)
		return false;

	if (File.write((char*)&CacheFlags, sizeof(CacheFlags)) == -1)
		return false;

	if (File.write((char*)&mArchiveCheckSum, sizeof(mArchiveCheckSum)) == -1)
		return false;

	quint32 UncompressedSize = CacheFile.GetLength();

	if (File.write((char*)&UncompressedSize, sizeof(UncompressedSize)) == -1)
		return false;

	const size_t BufferSize = 16384;
	char WriteBuffer[BufferSize];
	z_stream Stream;
	lcuint32 Crc32 = 0;

	CacheFile.Seek(0, SEEK_SET);

	Stream.zalloc = (alloc_func)0;
	Stream.zfree = (free_func)0;
	Stream.opaque = (voidpf)0;

	if (deflateInit2(&Stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK)
		return false;

	Bytef* BufferIn = CacheFile.mBuffer;
	int FlushMode;

	do
	{
		uInt Read = lcMin(CacheFile.GetLength() - (BufferIn - CacheFile.mBuffer), BufferSize);
		Stream.avail_in = Read;
		Stream.next_in = BufferIn;
		Crc32 = crc32(Crc32, BufferIn, Read);
		BufferIn += Read;

		FlushMode = (BufferIn >= CacheFile.mBuffer + CacheFile.GetLength()) ? Z_FINISH : Z_NO_FLUSH;

		do
		{
			Stream.avail_out = BufferSize;
			Stream.next_out = (Bytef*)WriteBuffer;
			deflate(&Stream, FlushMode);
			File.write(WriteBuffer, BufferSize - Stream.avail_out);
		} while (Stream.avail_out == 0);
	} while (FlushMode != Z_FINISH);

    deflateEnd(&Stream);

	return true;
}

bool lcPiecesLibrary::LoadCacheIndex(const QString& FileName)
{
	lcMemFile IndexFile;

	if (!ReadCacheFile(FileName, IndexFile))
		return false;

	qint32 NumFiles;

	if (IndexFile.ReadBuffer((char*)&NumFiles, sizeof(NumFiles)) == 0 || NumFiles != mPieces.GetSize())
		return false;

	for (int PieceInfoIndex = 0; PieceInfoIndex < mPieces.GetSize(); PieceInfoIndex++)
	{
		PieceInfo* Info = mPieces[PieceInfoIndex];
		quint8 Length;

		if (IndexFile.ReadBuffer((char*)&Length, sizeof(Length)) == 0 || Length >= sizeof(Info->m_strDescription))
			return false;

		if (IndexFile.ReadBuffer((char*)Info->m_strDescription, Length) == 0 || IndexFile.ReadBuffer((char*)&Info->mFlags, sizeof(Info->mFlags)) == 0)
			return false;

		Info->m_strDescription[Length] = 0;
	}

	return true;
}

bool lcPiecesLibrary::SaveCacheIndex(const QString& FileName)
{
	lcMemFile IndexFile;

	qint32 NumFiles = mPieces.GetSize();

	if (IndexFile.WriteBuffer((char*)&NumFiles, sizeof(NumFiles)) == 0)
		return false;

	for (int PieceInfoIndex = 0; PieceInfoIndex < mPieces.GetSize(); PieceInfoIndex++)
	{
		PieceInfo* Info = mPieces[PieceInfoIndex];
		quint8 Length = strlen(Info->m_strDescription);

		if (IndexFile.WriteBuffer((char*)&Length, sizeof(Length)) == 0)
			return false;

		if (IndexFile.WriteBuffer((char*)Info->m_strDescription, Length) == 0 || IndexFile.WriteBuffer((char*)&Info->mFlags, sizeof(Info->mFlags)) == 0)
			return false;
	}

	return WriteCacheFile(FileName, IndexFile);
}

bool lcPiecesLibrary::LoadCachePiece(PieceInfo* Info)
{
	QString FileName = QFileInfo(QDir(mCachePath), QString::fromLatin1(Info->m_strName)).absoluteFilePath();
	lcMemFile MeshData;

	if (!ReadCacheFile(FileName, MeshData))
		return false;

	lcMesh* Mesh = new lcMesh;
	Info->SetMesh(Mesh);

	quint32 Flags;
	if (MeshData.ReadBuffer((char*)&Flags, sizeof(Flags)) == 0)
		return false;

	Info->mFlags = Flags;

	if (MeshData.ReadBuffer((char*)Info->m_fDimensions, sizeof(Info->m_fDimensions)) == 0) // todo: move dimensions to the mesh
		return false;

	if (MeshData.ReadBuffer((char*)&Info->GetMesh()->mRadius, sizeof(float)) == 0)
		return false;

	return Mesh->FileLoad(MeshData);
}

bool lcPiecesLibrary::SaveCachePiece(PieceInfo* Info)
{
	lcMemFile MeshData;

	quint32 Flags = Info->mFlags;
	if (MeshData.WriteBuffer((char*)&Flags, sizeof(Flags)) == 0)
		return false;

	if (MeshData.WriteBuffer((char*)Info->m_fDimensions, sizeof(Info->m_fDimensions)) == 0)
		return false;

	if (MeshData.WriteBuffer((char*)&Info->GetMesh()->mRadius, sizeof(float)) == 0)
		return false;

	if (!Info->GetMesh()->FileSave(MeshData))
		return false;

	QString FileName = QFileInfo(QDir(mCachePath), QString::fromLatin1(Info->m_strName)).absoluteFilePath();

	return WriteCacheFile(FileName, MeshData);
}

struct lcMergeSection
{
	lcLibraryMeshSection* Shared;
	lcLibraryMeshSection* Lod;
};

static int LibraryMeshSectionCompare(lcMergeSection const& First, lcMergeSection const& Second)
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
		bool Ret = ReadMeshData(PieceFile, lcMatrix44Identity(), 16, TextureStack, MeshData, LC_MESHDATA_SHARED);
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
		bool Ret = ReadMeshData(PieceFile, lcMatrix44Identity(), 16, TextureStack, MeshData, LC_MESHDATA_SHARED);
		setlocale(LC_NUMERIC, OldLocale);

        if (!Ret)
			return false;
	}

	CreateMesh(Info, MeshData);

	if (mZipFiles[LC_ZIPFILE_OFFICIAL])
		SaveCachePiece(Info);

	return true;
}

void lcPiecesLibrary::CreateMesh(PieceInfo* Info, lcLibraryMeshData& MeshData)
{
	lcMesh* Mesh = new lcMesh();

	int BaseVertices[LC_NUM_MESHDATA_TYPES];
	int BaseTexturedVertices[LC_NUM_MESHDATA_TYPES];
	int NumVertices = 0;
	int NumTexturedVertices = 0;

	for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
	{
		lcArray<lcLibraryMeshSection*>& Sections = MeshData.mSections[MeshDataIdx];

		for (int SectionIdx = 0; SectionIdx < Sections.GetSize(); SectionIdx++)
		{
			lcLibraryMeshSection* Section = Sections[SectionIdx];
			Section->mColor = lcGetColorIndex(Section->mColor);
		}

		BaseVertices[MeshDataIdx] = NumVertices;
		NumVertices += MeshData.mVertices[MeshDataIdx].GetSize();
		BaseTexturedVertices[MeshDataIdx] = NumTexturedVertices;
		NumTexturedVertices += MeshData.mTexturedVertices[MeshDataIdx].GetSize();
	}

	lcuint16 NumSections[LC_NUM_MESH_LODS];
	int NumIndices = 0;

	lcArray<lcMergeSection> MergeSections[LC_NUM_MESH_LODS];

	for (int LodIdx = 0; LodIdx < LC_NUM_MESH_LODS; LodIdx++)
	{
		const lcArray<lcLibraryMeshSection*>& SharedSections = MeshData.mSections[LC_MESHDATA_SHARED];
		const lcArray<lcLibraryMeshSection*>& Sections = MeshData.mSections[LodIdx];

		for (int SharedSectionIdx = 0; SharedSectionIdx < SharedSections.GetSize(); SharedSectionIdx++)
		{
			lcLibraryMeshSection* SharedSection = SharedSections[SharedSectionIdx];
			NumIndices += SharedSection->mIndices.GetSize();

			lcMergeSection& MergeSection = MergeSections[LodIdx].Add();
			MergeSection.Shared = SharedSection;
			MergeSection.Lod = NULL;
		}

		for (int SectionIdx = 0; SectionIdx < Sections.GetSize(); SectionIdx++)
		{
			lcLibraryMeshSection* Section = Sections[SectionIdx];
			bool Found = false;

			NumIndices += Section->mIndices.GetSize();

			for (int SharedSectionIdx = 0; SharedSectionIdx < SharedSections.GetSize(); SharedSectionIdx++)
			{
				lcLibraryMeshSection* SharedSection = SharedSections[SharedSectionIdx];

				if (SharedSection->mColor == Section->mColor && SharedSection->mPrimitiveType == Section->mPrimitiveType && SharedSection->mTexture == Section->mTexture)
				{
					lcMergeSection& MergeSection = MergeSections[LodIdx][SharedSectionIdx];
					MergeSection.Lod = Section;
					Found = true;
					break;
				}
			}

			if (!Found)
			{
				lcMergeSection& MergeSection = MergeSections[LodIdx].Add();
				MergeSection.Shared = NULL;
				MergeSection.Lod = Section;
			}
		}

		NumSections[LodIdx] = MergeSections[LodIdx].GetSize();
		MergeSections[LodIdx].Sort(LibraryMeshSectionCompare);
	}

	Mesh->Create(NumSections, NumVertices, NumTexturedVertices, NumIndices);

	lcVertex* DstVerts = (lcVertex*)Mesh->mVertexData;
	lcVector3 Min(FLT_MAX, FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
	{
		const lcArray<lcVertex>& Vertices = MeshData.mVertices[MeshDataIdx];

		for (int VertexIdx = 0; VertexIdx < Vertices.GetSize(); VertexIdx++)
		{
			lcVertex& DstVertex = *DstVerts++;

			const lcVector3& SrcPosition = Vertices[VertexIdx].Position;
			lcVector3& DstPosition = DstVertex.Position;

			DstPosition = lcVector3(SrcPosition.x, SrcPosition.z, -SrcPosition.y);

			Min.x = lcMin(Min.x, DstPosition.x);
			Min.y = lcMin(Min.y, DstPosition.y);
			Min.z = lcMin(Min.z, DstPosition.z);
			Max.x = lcMax(Max.x, DstPosition.x);
			Max.y = lcMax(Max.y, DstPosition.y);
			Max.z = lcMax(Max.z, DstPosition.z);
		}
	}

	lcVertexTextured* DstTexturedVerts = (lcVertexTextured*)DstVerts;

	for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
	{
		const lcArray<lcVertexTextured>& TexturedVertices = MeshData.mTexturedVertices[MeshDataIdx];

		for (int VertexIdx = 0; VertexIdx < TexturedVertices.GetSize(); VertexIdx++)
		{
			lcVertexTextured& DstVertex = *DstTexturedVerts++;
			lcVertexTextured& SrcVertex = TexturedVertices[VertexIdx];

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
	}

	Info->m_fDimensions[0] = Max.x;
	Info->m_fDimensions[1] = Max.y;
	Info->m_fDimensions[2] = Max.z;
	Info->m_fDimensions[3] = Min.x;
	Info->m_fDimensions[4] = Min.y;
	Info->m_fDimensions[5] = Min.z;
	Mesh->mRadius = lcLength((Max - Min) / 2.0f);

	NumIndices = 0;

	for (int LodIdx = 0; LodIdx < LC_NUM_MESH_LODS; LodIdx++)
	{
		for (int SectionIdx = 0; SectionIdx < MergeSections[LodIdx].GetSize(); SectionIdx++)
		{
			lcMergeSection& MergeSection = MergeSections[LodIdx][SectionIdx];
			lcMeshSection& DstSection = Mesh->mLods[LodIdx].Sections[SectionIdx];

			lcLibraryMeshSection* SetupSection = MergeSection.Shared ? MergeSection.Shared : MergeSection.Lod;

			DstSection.ColorIndex = SetupSection->mColor;
			DstSection.PrimitiveType = (SetupSection->mPrimitiveType == LC_MESH_TRIANGLES || SetupSection->mPrimitiveType == LC_MESH_TEXTURED_TRIANGLES) ? GL_TRIANGLES : GL_LINES;
			DstSection.NumIndices = 0;
			DstSection.Texture = SetupSection->mTexture;

			if (DstSection.Texture)
				DstSection.Texture->AddRef();

			if (Mesh->mNumVertices < 0x10000)
			{
				DstSection.IndexOffset = NumIndices * 2;

				lcuint16* Index = (lcuint16*)Mesh->mIndexData + NumIndices;

				if (MergeSection.Shared)
				{
					lcuint16 BaseVertex = DstSection.Texture ? BaseTexturedVertices[LC_MESHDATA_SHARED] : BaseVertices[LC_MESHDATA_SHARED];
					lcLibraryMeshSection* SrcSection = MergeSection.Shared;

					for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
						*Index++ = BaseVertex + SrcSection->mIndices[IndexIdx];

					DstSection.NumIndices += SrcSection->mIndices.GetSize();
				}

				if (MergeSection.Lod)
				{
					lcuint16 BaseVertex = DstSection.Texture ? BaseTexturedVertices[LodIdx] : BaseVertices[LodIdx];
					lcLibraryMeshSection* SrcSection = MergeSection.Lod;

					for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
						*Index++ = BaseVertex + SrcSection->mIndices[IndexIdx];

					DstSection.NumIndices += SrcSection->mIndices.GetSize();
				}
			}
			else
			{
				DstSection.IndexOffset = NumIndices * 4;

				lcuint32* Index = (lcuint32*)Mesh->mIndexData + NumIndices;

				if (MergeSection.Shared)
				{
					lcuint32 BaseVertex = DstSection.Texture ? BaseTexturedVertices[LC_MESHDATA_SHARED] : BaseVertices[LC_MESHDATA_SHARED];
					lcLibraryMeshSection* SrcSection = MergeSection.Shared;

					for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
						*Index++ = BaseVertex + SrcSection->mIndices[IndexIdx];

					DstSection.NumIndices += SrcSection->mIndices.GetSize();
				}

				if (MergeSection.Lod)
				{
					lcuint32 BaseVertex = DstSection.Texture ? BaseTexturedVertices[LodIdx] : BaseVertices[LodIdx];
					lcLibraryMeshSection* SrcSection = MergeSection.Shared;

					for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
						*Index++ = BaseVertex + SrcSection->mIndices[IndexIdx];

					DstSection.NumIndices += SrcSection->mIndices.GetSize();
				}
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
	}
	/*
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

			lcuint16* Index = (lcuint16*)Mesh->mIndexData + NumIndices;

			for (int IndexIdx = 0; IndexIdx < DstSection.NumIndices; IndexIdx++)
				*Index++ = SrcSection->mIndices[IndexIdx];
		}
		else
		{
			DstSection.IndexOffset = NumIndices * 4;

			lcuint32* Index = (lcuint32*)Mesh->mIndexData + NumIndices;

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
	*/
	Info->SetMesh(Mesh);
}

void lcPiecesLibrary::UpdateBuffers(lcContext* Context)
{
	if (!gSupportsVertexBufferObject || !mBuffersDirty)
		return;

	int VertexDataSize = 0;
	int IndexDataSize = 0;

	for (int PieceInfoIndex = 0; PieceInfoIndex < mPieces.GetSize(); PieceInfoIndex++)
	{
		PieceInfo* Info = mPieces[PieceInfoIndex];
		lcMesh* Mesh = Info->IsPlaceholder() ? gPlaceholderMesh : Info->GetMesh();

		if (!Mesh)
			continue;

		VertexDataSize += Mesh->mVertexDataSize;
		IndexDataSize += Mesh->mIndexDataSize;
	}

	Context->DestroyVertexBuffer(mVertexBuffer);
	Context->DestroyIndexBuffer(mIndexBuffer);

	if (!VertexDataSize || !IndexDataSize)
		return;

	void* VertexData = malloc(VertexDataSize);
	void* IndexData = malloc(IndexDataSize);

	VertexDataSize = 0;
	IndexDataSize = 0;

	for (int PieceInfoIndex = 0; PieceInfoIndex < mPieces.GetSize(); PieceInfoIndex++)
	{
		PieceInfo* Info = mPieces[PieceInfoIndex];
		lcMesh* Mesh = Info->IsPlaceholder() ? gPlaceholderMesh : Info->GetMesh();

		if (!Mesh)
			continue;

		Mesh->mVertexCacheOffset = VertexDataSize;
		Mesh->mIndexCacheOffset = IndexDataSize;

		memcpy((char*)VertexData + VertexDataSize, Mesh->mVertexData, Mesh->mVertexDataSize);
		memcpy((char*)IndexData + IndexDataSize, Mesh->mIndexData, Mesh->mIndexDataSize);

		VertexDataSize += Mesh->mVertexDataSize;
		IndexDataSize += Mesh->mIndexDataSize;
	}

	mVertexBuffer = Context->CreateVertexBuffer(VertexDataSize, VertexData);
	mIndexBuffer = Context->CreateIndexBuffer(IndexDataSize, IndexData);
	mBuffersDirty = false;

	free(VertexData);
	free(IndexData);
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

int lcPiecesLibrary::FindPrimitiveIndex(const char* Name) const
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
		int LowPrimitiveIndex = -1;

		if (Primitive->mStud && strncmp(Primitive->mName, "8/", 2))
		{
			char Name[LC_PIECE_NAME_LEN];
			strcpy(Name, "8/");
			strcat(Name, Primitive->mName);

			LowPrimitiveIndex = FindPrimitiveIndex(Name);
		}

		lcMemFile PrimFile;

		if (!mZipFiles[Primitive->mZipFileType]->ExtractFile(Primitive->mZipFileIndex, PrimFile))
			return false;

		if (LowPrimitiveIndex == -1)
		{
			if (!ReadMeshData(PrimFile, lcMatrix44Identity(), 16, TextureStack, Primitive->mMeshData, LC_MESHDATA_SHARED))
				return false;
		}
		else
		{
			if (!ReadMeshData(PrimFile, lcMatrix44Identity(), 16, TextureStack, Primitive->mMeshData, LC_MESHDATA_HIGH))
				return false;

			lcLibraryPrimitive* LowPrimitive = mPrimitives[LowPrimitiveIndex];

			if (!mZipFiles[LowPrimitive->mZipFileType]->ExtractFile(LowPrimitive->mZipFileIndex, PrimFile))
				return false;

			TextureStack.RemoveAll();

			if (!ReadMeshData(PrimFile, lcMatrix44Identity(), 16, TextureStack, Primitive->mMeshData, LC_MESHDATA_LOW))
				return false;
		}
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

		if (!ReadMeshData(PrimFile, lcMatrix44Identity(), 16, TextureStack, Primitive->mMeshData, LC_MESHDATA_SHARED))
			return false;
	}

	Primitive->mLoaded = true;

	return true;
}

bool lcPiecesLibrary::ReadMeshData(lcFile& File, const lcMatrix44& CurrentTransform, lcuint32 CurrentColorCode, lcArray<lcLibraryTextureMap>& TextureStack, lcLibraryMeshData& MeshData, lcMeshDataType MeshDataType)
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
						MeshData.AddMeshDataNoDuplicateCheck(Primitive->mMeshData, IncludeTransform, ColorCode, TextureMap, MeshDataType);
					else if (!Primitive->mSubFile)
						MeshData.AddMeshData(Primitive->mMeshData, IncludeTransform, ColorCode, TextureMap, MeshDataType);
					else
					{
						if (mZipFiles[LC_ZIPFILE_OFFICIAL])
						{
							lcMemFile IncludeFile;

							if (!mZipFiles[Primitive->mZipFileType]->ExtractFile(Primitive->mZipFileIndex, IncludeFile))
								continue;

							if (!ReadMeshData(IncludeFile, IncludeTransform, ColorCode, TextureStack, MeshData, MeshDataType))
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

							if (!ReadMeshData(IncludeFile, IncludeTransform, ColorCode, TextureStack, MeshData, MeshDataType))
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

							if (!ReadMeshData(IncludeFile, IncludeTransform, ColorCode, TextureStack, MeshData, MeshDataType))
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

							if (!ReadMeshData(IncludeFile, IncludeTransform, ColorCode, TextureStack, MeshData, MeshDataType))
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
					MeshData.AddTexturedLine(MeshDataType, LineType, ColorCode, *TextureMap, Points);

					if (TextureMap->Next)
						TextureStack.RemoveIndex(TextureStack.GetSize() - 1);
				}
				else
					MeshData.AddLine(MeshDataType, LineType, ColorCode, Points);
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
					MeshData.AddTexturedLine(MeshDataType, LineType, ColorCode, *TextureMap, Points);

					if (TextureMap->Next)
						TextureStack.RemoveIndex(TextureStack.GetSize() - 1);
				}
				else
					MeshData.AddLine(MeshDataType, LineType, ColorCode, Points);
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
					MeshData.AddTexturedLine(MeshDataType, LineType, ColorCode, *TextureMap, Points);

					if (TextureMap->Next)
						TextureStack.RemoveIndex(TextureStack.GetSize() - 1);
				}
				else
					MeshData.AddLine(MeshDataType, LineType, ColorCode, Points);
			} break;
		}
	}

	return true;
}

void lcLibraryMeshData::ResequenceQuad(int* Indices, int a, int b, int c, int d)
{
	Indices[0] = a;
	Indices[1] = b;
	Indices[2] = c;
	Indices[3] = d;
}

void lcLibraryMeshData::TestQuad(int* QuadIndices, const lcVector3* Vertices)
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
			ResequenceQuad(QuadIndices, 1, 2, 3, 0);
		else
			ResequenceQuad(QuadIndices, 0, 3, 1, 2);
	}
	else
	{
		if (-lcDot(lcCross(v02, v12), lcCross(v12, v23)) > 0.0f)
			ResequenceQuad(QuadIndices, 0, 1, 3, 2);
		else
			ResequenceQuad(QuadIndices, 1, 2, 3, 0);
	}
}

void lcLibraryMeshData::AddLine(lcMeshDataType MeshDataType, int LineType, lcuint32 ColorCode, const lcVector3* Vertices)
{
	lcLibraryMeshSection* Section = NULL;
	int SectionIdx;
	LC_MESH_PRIMITIVE_TYPE PrimitiveType = (LineType == 2) ? LC_MESH_LINES : LC_MESH_TRIANGLES;
	lcArray<lcLibraryMeshSection*>& Sections = mSections[MeshDataType];

	for (SectionIdx = 0; SectionIdx < Sections.GetSize(); SectionIdx++)
	{
		Section = Sections[SectionIdx];

		if (Section->mColor == ColorCode && Section->mPrimitiveType == PrimitiveType && Section->mTexture == NULL)
			break;
	}

	if (SectionIdx == Sections.GetSize())
	{
		Section = new lcLibraryMeshSection(PrimitiveType, ColorCode, NULL);

		Sections.Add(Section);
	}

	int QuadIndices[4] = { 0, 1, 2, 3 };

	if (LineType == 4)
		TestQuad(QuadIndices, Vertices);

	int Indices[4] = { -1, -1, -1, -1 };

	for (int IndexIdx = 0; IndexIdx < LineType; IndexIdx++)
	{
		const lcVector3& Position = Vertices[QuadIndices[IndexIdx]];
		lcArray<lcVertex>& VertexArray = mVertices[MeshDataType];

		for (int VertexIdx = VertexArray.GetSize() - 1; VertexIdx >= 0; VertexIdx--)
		{
			lcVertex& DstVertex = VertexArray[VertexIdx];

			if (Position == DstVertex.Position)
			{
				Indices[IndexIdx] = VertexIdx;
				break;
			}
		}

		if (Indices[IndexIdx] == -1)
		{
			Indices[IndexIdx] = VertexArray.GetSize();
			lcVertex& DstVertex = VertexArray.Add();
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

void lcLibraryMeshData::AddTexturedLine(lcMeshDataType MeshDataType, int LineType, lcuint32 ColorCode, const lcLibraryTextureMap& Map, const lcVector3* Vertices)
{
	lcLibraryMeshSection* Section = NULL;
	int SectionIdx;
	LC_MESH_PRIMITIVE_TYPE PrimitiveType = (LineType == 2) ? LC_MESH_TEXTURED_LINES : LC_MESH_TEXTURED_TRIANGLES;
	lcArray<lcLibraryMeshSection*>& Sections = mSections[MeshDataType];

	for (SectionIdx = 0; SectionIdx < Sections.GetSize(); SectionIdx++)
	{
		Section = Sections[SectionIdx];

		if (Section->mColor == ColorCode && Section->mPrimitiveType == PrimitiveType && Section->mTexture == Map.Texture)
			break;
	}

	if (SectionIdx == Sections.GetSize())
	{
		Section = new lcLibraryMeshSection(PrimitiveType, ColorCode, Map.Texture);

		Sections.Add(Section);
	}

	int QuadIndices[4] = { 0, 1, 2, 3 };

	if (LineType == 4)
		TestQuad(QuadIndices, Vertices);

	int Indices[4] = { -1, -1, -1, -1 };

	for (int IndexIdx = 0; IndexIdx < LineType; IndexIdx++)
	{
		const lcVector3& Position = Vertices[QuadIndices[IndexIdx]];
		lcVector2 TexCoord(lcDot3(lcVector3(Position.x, Position.y, Position.z), Map.Params[0]) + Map.Params[0].w,
						   lcDot3(lcVector3(Position.x, Position.y, Position.z), Map.Params[1]) + Map.Params[1].w);
		lcArray<lcVertexTextured>& VertexArray = mTexturedVertices[MeshDataType];

		for (int VertexIdx = VertexArray.GetSize() - 1; VertexIdx >= 0; VertexIdx--)
		{
			lcVertexTextured& DstVertex = VertexArray[VertexIdx];

			if (Position == DstVertex.Position && TexCoord == DstVertex.TexCoord)
			{
				Indices[IndexIdx] = VertexIdx;
				break;
			}
		}

		if (Indices[IndexIdx] == -1)
		{
			Indices[IndexIdx] = VertexArray.GetSize();
			lcVertexTextured& DstVertex = VertexArray.Add();
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

void lcLibraryMeshData::AddMeshData(const lcLibraryMeshData& Data, const lcMatrix44& Transform, lcuint32 CurrentColorCode, lcLibraryTextureMap* TextureMap, lcMeshDataType OverrideDestIndex)
{
	for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
	{
		int DestIndex = OverrideDestIndex == LC_MESHDATA_SHARED ? MeshDataIdx : OverrideDestIndex;
		const lcArray<lcVertex>& DataVertices = Data.mVertices[MeshDataIdx];
		lcArray<lcVertex>& Vertices = mVertices[DestIndex];
		lcArray<lcVertexTextured>& TexturedVertices = mTexturedVertices[DestIndex];

		int VertexCount = DataVertices.GetSize();
		lcArray<lcuint32> IndexRemap(VertexCount);
		const float DistanceEpsilon = 0.05f;

		if (!TextureMap)
		{
			Vertices.AllocGrow(VertexCount);

			for (int SrcVertexIdx = 0; SrcVertexIdx < VertexCount; SrcVertexIdx++)
			{
				lcVector3 Position = lcMul31(DataVertices[SrcVertexIdx].Position, Transform);
				int Index = -1;

				for (int DstVertexIdx = Vertices.GetSize() - 1; DstVertexIdx >= 0; DstVertexIdx--)
				{
					lcVertex& DstVertex = Vertices[DstVertexIdx];

	//				if (Vertex == Vertices[DstVertexIdx])
					if (fabsf(Position.x - DstVertex.Position.x) < DistanceEpsilon && fabsf(Position.y - DstVertex.Position.y) < DistanceEpsilon && fabsf(Position.z - DstVertex.Position.z) < DistanceEpsilon)
					{
						Index = DstVertexIdx;
						break;
					}
				}

				if (Index == -1)
				{
					Index = Vertices.GetSize();
					lcVertex& DstVertex = Vertices.Add();
					DstVertex.Position = Position;
				}

				IndexRemap.Add(Index);
			}
		}
		else
		{
			TexturedVertices.AllocGrow(VertexCount);

			for (int SrcVertexIdx = 0; SrcVertexIdx < VertexCount; SrcVertexIdx++)
			{
				lcVertex& SrcVertex = DataVertices[SrcVertexIdx];
				lcVector3 Position = lcMul31(SrcVertex.Position, Transform);
				lcVector2 TexCoord(lcDot3(lcVector3(Position.x, Position.y, Position.z), TextureMap->Params[0]) + TextureMap->Params[0].w,
								   lcDot3(lcVector3(Position.x, Position.y, Position.z), TextureMap->Params[1]) + TextureMap->Params[1].w);
				int Index = -1;

				for (int DstVertexIdx = TexturedVertices.GetSize() - 1; DstVertexIdx >= 0; DstVertexIdx--)
				{
					lcVertexTextured& DstVertex = TexturedVertices[DstVertexIdx];

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
					Index = TexturedVertices.GetSize();
					lcVertexTextured& DstVertex = TexturedVertices.Add();
					DstVertex.Position = Position;
					DstVertex.TexCoord = TexCoord;
				}

				IndexRemap.Add(Index);
			}
		}

		const lcArray<lcVertexTextured>& DataTexturedVertices = Data.mTexturedVertices[MeshDataIdx];
		int TexturedVertexCount = DataTexturedVertices.GetSize();
		lcArray<lcuint32> TexturedIndexRemap(TexturedVertexCount);

		if (TexturedVertexCount)
		{
			TexturedVertices.AllocGrow(TexturedVertexCount);

			for (int SrcVertexIdx = 0; SrcVertexIdx < TexturedVertexCount; SrcVertexIdx++)
			{
				lcVertexTextured& SrcVertex = DataTexturedVertices[SrcVertexIdx];
				lcVector3 Position = lcMul31(SrcVertex.Position, Transform);
				int Index = -1;

				for (int DstVertexIdx = TexturedVertices.GetSize() - 1; DstVertexIdx >= 0; DstVertexIdx--)
				{
					lcVertexTextured& DstVertex = TexturedVertices[DstVertexIdx];

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
					Index = TexturedVertices.GetSize();
					lcVertexTextured& DstVertex = TexturedVertices.Add();
					DstVertex.Position = Position;
					DstVertex.TexCoord = SrcVertex.TexCoord;
				}

				TexturedIndexRemap.Add(Index);
			}
		}

		const lcArray<lcLibraryMeshSection*>& DataSections = Data.mSections[MeshDataIdx];
		lcArray<lcLibraryMeshSection*>& Sections = mSections[DestIndex];

		for (int SrcSectionIdx = 0; SrcSectionIdx < DataSections.GetSize(); SrcSectionIdx++)
		{
			lcLibraryMeshSection* SrcSection = DataSections[SrcSectionIdx];
			lcLibraryMeshSection* DstSection = NULL;
			lcuint32 ColorCode = SrcSection->mColor == 16 ? CurrentColorCode : SrcSection->mColor;
			lcTexture* Texture;

			if (SrcSection->mTexture)
				Texture = SrcSection->mTexture;
			else if (TextureMap)
				Texture = TextureMap->Texture;
			else
				Texture = NULL;

			for (int DstSectionIdx = 0; DstSectionIdx < Sections.GetSize(); DstSectionIdx++)
			{
				lcLibraryMeshSection* Section = Sections[DstSectionIdx];

				if (Section->mColor == ColorCode && Section->mPrimitiveType == SrcSection->mPrimitiveType && Section->mTexture == Texture)
				{
					DstSection = Section;
					break;
				}
			}

			if (!DstSection)
			{
				DstSection = new lcLibraryMeshSection(SrcSection->mPrimitiveType, ColorCode, Texture);

				Sections.Add(DstSection);
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
}

void lcLibraryMeshData::AddMeshDataNoDuplicateCheck(const lcLibraryMeshData& Data, const lcMatrix44& Transform, lcuint32 CurrentColorCode, lcLibraryTextureMap* TextureMap, lcMeshDataType OverrideDestIndex)
{
	for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
	{
		int DestIndex = OverrideDestIndex == LC_MESHDATA_SHARED ? MeshDataIdx : OverrideDestIndex;
		const lcArray<lcVertex>& DataVertices = Data.mVertices[MeshDataIdx];
		lcArray<lcVertex>& Vertices = mVertices[DestIndex];
		lcArray<lcVertexTextured>& TexturedVertices = mTexturedVertices[DestIndex];
		lcuint32 BaseIndex;

		if (!TextureMap)
		{
			BaseIndex = Vertices.GetSize();

			Vertices.AllocGrow(DataVertices.GetSize());

			for (int SrcVertexIdx = 0; SrcVertexIdx < DataVertices.GetSize(); SrcVertexIdx++)
			{
				lcVertex& Vertex = Vertices.Add();
				Vertex.Position = lcMul31(DataVertices[SrcVertexIdx].Position, Transform);
			}
		}
		else
		{
			BaseIndex = TexturedVertices.GetSize();

			TexturedVertices.AllocGrow(DataVertices.GetSize());

			for (int SrcVertexIdx = 0; SrcVertexIdx < DataVertices.GetSize(); SrcVertexIdx++)
			{
				lcVertex& SrcVertex = DataVertices[SrcVertexIdx];
				lcVertexTextured& DstVertex = TexturedVertices.Add();

				lcVector3 Position = lcMul31(SrcVertex.Position, Transform);
				lcVector2 TexCoord(lcDot3(lcVector3(Position.x, Position.y, Position.z), TextureMap->Params[0]) + TextureMap->Params[0].w,
								   lcDot3(lcVector3(Position.x, Position.y, Position.z), TextureMap->Params[1]) + TextureMap->Params[1].w);

				DstVertex.Position = Position;
				DstVertex.TexCoord = TexCoord;
			}
		}

		const lcArray<lcVertexTextured>& DataTexturedVertices = Data.mTexturedVertices[MeshDataIdx];

		int TexturedVertexCount = DataTexturedVertices.GetSize();
		lcuint32 BaseTexturedIndex = TexturedVertices.GetSize();

		if (TexturedVertexCount)
		{
			TexturedVertices.AllocGrow(TexturedVertexCount);

			for (int SrcVertexIdx = 0; SrcVertexIdx < TexturedVertexCount; SrcVertexIdx++)
			{
				lcVertexTextured& SrcVertex = DataTexturedVertices[SrcVertexIdx];
				lcVertexTextured& DstVertex = TexturedVertices.Add();
				DstVertex.Position = lcMul31(SrcVertex.Position, Transform);
				DstVertex.TexCoord = SrcVertex.TexCoord;
			}
		}

		const lcArray<lcLibraryMeshSection*>& DataSections = Data.mSections[MeshDataIdx];
		lcArray<lcLibraryMeshSection*>& Sections = mSections[DestIndex];

		for (int SrcSectionIdx = 0; SrcSectionIdx < DataSections.GetSize(); SrcSectionIdx++)
		{
			lcLibraryMeshSection* SrcSection = DataSections[SrcSectionIdx];
			lcLibraryMeshSection* DstSection = NULL;
			lcuint32 ColorCode = SrcSection->mColor == 16 ? CurrentColorCode : SrcSection->mColor;
			lcTexture* Texture;

			if (SrcSection->mTexture)
				Texture = SrcSection->mTexture;
			else if (TextureMap)
				Texture = TextureMap->Texture;
			else
				Texture = NULL;

			for (int DstSectionIdx = 0; DstSectionIdx < Sections.GetSize(); DstSectionIdx++)
			{
				lcLibraryMeshSection* Section = Sections[DstSectionIdx];

				if (Section->mColor == ColorCode && Section->mPrimitiveType == SrcSection->mPrimitiveType && Section->mTexture == Texture)
				{
					DstSection = Section;
					break;
				}
			}

			if (!DstSection)
			{
				DstSection = new lcLibraryMeshSection(SrcSection->mPrimitiveType, ColorCode, Texture);

				Sections.Add(DstSection);
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
}

bool lcPiecesLibrary::PieceInCategory(PieceInfo* Info, const String& CategoryKeywords) const
{
	if (Info->IsTemporary())
		return false;

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
	GetCategoryEntries(gCategories[CategoryIndex].Keywords, GroupPieces, SinglePieces, GroupedPieces);
}

void lcPiecesLibrary::GetCategoryEntries(const String& CategoryKeywords, bool GroupPieces, lcArray<PieceInfo*>& SinglePieces, lcArray<PieceInfo*>& GroupedPieces)
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

			Parent = FindPiece(ParentName, NULL, false);

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

void lcPiecesLibrary::SearchPieces(const char* Keyword, lcArray<PieceInfo*>& Pieces) const
{
	Pieces.RemoveAll();

	String LowerKeyword = Keyword;
	LowerKeyword.MakeLower();

	for (int PieceIdx = 0; PieceIdx < mPieces.GetSize(); PieceIdx++)
	{
		PieceInfo* Info = mPieces[PieceIdx];

		char LowerName[sizeof(Info->m_strName)];
		strcpy(LowerName, Info->m_strName);
		strlwr(LowerName);

		if (strstr(LowerName, LowerKeyword))
		{
			Pieces.Add(Info);
			continue;
		}

		char LowerDescription[sizeof(Info->m_strDescription)];
		strcpy(LowerDescription, Info->m_strDescription);
		strlwr(LowerDescription);

		if (strstr(LowerDescription, LowerKeyword))
			Pieces.Add(Info);
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

	return true;
}
