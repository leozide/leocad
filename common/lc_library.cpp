#include "lc_global.h"
#include "lc_library.h"
#include "lc_zipfile.h"
#include "lc_file.h"
#include "pieceinf.h"
#include "lc_colors.h"
#include "lc_texture.h"
#include "lc_category.h"
#include "lc_application.h"
#include "lc_context.h"
#include "lc_glextensions.h"
#include "lc_synth.h"
#include "project.h"
#include <ctype.h>
#include <locale.h>
#include <zlib.h>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtConcurrent>
#endif

#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif

#define LC_LIBRARY_CACHE_VERSION   0x0105
#define LC_LIBRARY_CACHE_ARCHIVE   0x0001
#define LC_LIBRARY_CACHE_DIRECTORY 0x0002

lcPiecesLibrary::lcPiecesLibrary()
	: mLoadMutex(QMutex::Recursive)
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
	mZipFiles[LC_ZIPFILE_OFFICIAL] = nullptr;
	mZipFiles[LC_ZIPFILE_UNOFFICIAL] = nullptr;
	mBuffersDirty = false;
	mHasUnofficial = false;
	mCancelLoading = false;
}

lcPiecesLibrary::~lcPiecesLibrary()
{
	mLoadMutex.lock();
	mLoadQueue.clear();
	mLoadMutex.unlock();
	mCancelLoading = true;
	WaitForLoadQueue();
	Unload();
}

void lcPiecesLibrary::Unload()
{
	for (const auto PieceIt : mPieces)
		delete PieceIt.second;
	mPieces.clear();

	for (const auto PrimitiveIt : mPrimitives)
		delete PrimitiveIt.second;
	mPrimitives.clear();

	for (int TextureIdx = 0; TextureIdx < mTextures.GetSize(); TextureIdx++)
		delete mTextures[TextureIdx];
	mTextures.RemoveAll();

	mNumOfficialPieces = 0;
	delete mZipFiles[LC_ZIPFILE_OFFICIAL];
	mZipFiles[LC_ZIPFILE_OFFICIAL] = nullptr;
	delete mZipFiles[LC_ZIPFILE_UNOFFICIAL];
	mZipFiles[LC_ZIPFILE_UNOFFICIAL] = nullptr;
}

void lcPiecesLibrary::RemoveTemporaryPieces()
{
	QMutexLocker LoadLock(&mLoadMutex);

	for (auto PieceIt = mPieces.begin(); PieceIt != mPieces.end();)
	{
		PieceInfo* Info = PieceIt->second;

		if (Info->IsTemporary() && Info->GetRefCount() == 0)
		{
			PieceIt = mPieces.erase(PieceIt);
			delete Info;
		}
		else
			PieceIt++;
	}
}

void lcPiecesLibrary::RemovePiece(PieceInfo* Info)
{
	for (auto PieceIt = mPieces.begin(); PieceIt != mPieces.end(); PieceIt++)
	{
		if (PieceIt->second == Info)
		{
			mPieces.erase(PieceIt);
			break;
		}
	}
	delete Info;
}

void lcPiecesLibrary::RenamePiece(PieceInfo* Info, const char* NewName)
{
	for (auto PieceIt = mPieces.begin(); PieceIt != mPieces.end(); PieceIt++)
	{
		if (PieceIt->second == Info)
		{
			mPieces.erase(PieceIt);
			break;
		}
	}

	strncpy(Info->mFileName, NewName, sizeof(Info->mFileName));
	Info->mFileName[sizeof(Info->mFileName) - 1] = 0;
	strncpy(Info->m_strDescription, NewName, sizeof(Info->m_strDescription));
	Info->m_strDescription[sizeof(Info->m_strDescription) - 1] = 0;

	char PieceName[LC_PIECE_NAME_LEN];
	strcpy(PieceName, Info->mFileName);
	strupr(PieceName);

	mPieces[PieceName] = Info;
}

PieceInfo* lcPiecesLibrary::FindPiece(const char* PieceName, Project* CurrentProject, bool CreatePlaceholder, bool SearchProjectFolder)
{
	QString ProjectPath;
	if (SearchProjectFolder)
	{
		QString FileName = CurrentProject->GetFileName();

		if (!FileName.isEmpty())
			ProjectPath = QFileInfo(FileName).absolutePath();
	}

	char CleanName[LC_PIECE_NAME_LEN];
	const char* Src = PieceName;
	char* Dst = CleanName;

	while (*Src && Dst - CleanName != sizeof(CleanName))
	{
		if (*Src == '\\')
			*Dst = '/';
		else if (*Src >= 'a' && *Src <= 'z')
			*Dst = *Src + 'A' - 'a';
		else
			*Dst = *Src;

		Src++;
		Dst++;
	}
	*Dst = 0;

	const auto PieceIt = mPieces.find(CleanName);

	if (PieceIt != mPieces.end())
	{
		PieceInfo* Info = PieceIt->second;

		if ((!CurrentProject || !Info->IsModel() || CurrentProject->GetModels().FindIndex(Info->GetModel()) != -1) && (!ProjectPath.isEmpty() || !Info->IsProject()))
			return Info;
	}

	if (!ProjectPath.isEmpty())
	{
		QFileInfo ProjectFile = QFileInfo(ProjectPath + QDir::separator() + PieceName);

		if (ProjectFile.isFile())
		{
			Project* NewProject = new Project();

			if (NewProject->Load(ProjectFile.absoluteFilePath()))
			{
				PieceInfo* Info = new PieceInfo();

				Info->CreateProject(NewProject, PieceName);
				mPieces[CleanName] = Info;

				return Info;
			}
			else
				delete NewProject;
		}
	}

	if (CreatePlaceholder)
	{
		PieceInfo* Info = new PieceInfo();

		Info->CreatePlaceholder(PieceName);
		mPieces[CleanName] = Info;

		return Info;
	}

	return nullptr;
}

lcTexture* lcPiecesLibrary::FindTexture(const char* TextureName, Project* CurrentProject, bool SearchProjectFolder)
{
	for (int TextureIdx = 0; TextureIdx < mTextures.GetSize(); TextureIdx++)
		if (!strcmp(TextureName, mTextures[TextureIdx]->mName))
			return mTextures[TextureIdx];

	QString ProjectPath;
	if (SearchProjectFolder)
	{
		QString FileName = CurrentProject->GetFileName();

		if (!FileName.isEmpty())
			ProjectPath = QFileInfo(FileName).absolutePath();
	}

	if (!ProjectPath.isEmpty())
	{
		QFileInfo TextureFile = QFileInfo(ProjectPath + QDir::separator() + TextureName + ".png");

		if (TextureFile.isFile())
		{
			lcTexture* Texture = lcLoadTexture(TextureFile.absoluteFilePath(), LC_TEXTURE_WRAPU | LC_TEXTURE_WRAPV);

			if (Texture)
			{
				mTextures.Add(Texture);
				return Texture;
			}
		}
	}

	return nullptr;
}

bool lcPiecesLibrary::Load(const QString& LibraryPath, bool ShowProgress)
{
	Unload();

	if (OpenArchive(LibraryPath, LC_ZIPFILE_OFFICIAL))
	{
		lcMemFile ColorFile;

		if (!mZipFiles[LC_ZIPFILE_OFFICIAL]->ExtractFile("ldraw/ldconfig.ldr", ColorFile) || !lcLoadColorFile(ColorFile))
			lcLoadDefaultColors();

		mLibraryDir = QFileInfo(LibraryPath).absoluteDir();
		QString UnofficialFileName = mLibraryDir.absoluteFilePath(QLatin1String("ldrawunf.zip"));

		if (!OpenArchive(UnofficialFileName, LC_ZIPFILE_UNOFFICIAL))
			UnofficialFileName.clear();

		ReadArchiveDescriptions(LibraryPath, UnofficialFileName);
	}
	else
	{
		mLibraryDir = LibraryPath;

		if (OpenDirectory(mLibraryDir, ShowProgress))
		{
			lcDiskFile ColorFile(mLibraryDir.absoluteFilePath(QLatin1String("ldconfig.ldr")));

			if (!ColorFile.Open(QIODevice::ReadOnly) || !lcLoadColorFile(ColorFile))
				lcLoadDefaultColors();
		}
		else
			return false;
	}

	lcLoadDefaultCategories();
	lcSynthInit();

	return true;
}

bool lcPiecesLibrary::OpenArchive(const QString& FileName, lcZipFileType ZipFileType)
{
	lcDiskFile* File = new lcDiskFile(FileName);

	if (!File->Open(QIODevice::ReadOnly) || !OpenArchive(File, FileName, ZipFileType))
	{
		delete File;
		return false;
	}

	return true;
}

bool lcPiecesLibrary::OpenArchive(lcFile* File, const QString& FileName, lcZipFileType ZipFileType)
{
	lcZipFile* ZipFile = new lcZipFile();

	if (!ZipFile->OpenRead(File))
	{
		delete ZipFile;
		return false;
	}

	mZipFiles[ZipFileType] = ZipFile;

	if (ZipFileType == LC_ZIPFILE_OFFICIAL)
		mLibraryFileName = FileName;
	else
		mUnofficialFileName = FileName;

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

		*Dst = 0;
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
				PieceInfo* Info = FindPiece(Name, nullptr, false, false);

				if (!Info)
				{
					Info = new PieceInfo();

					strncpy(Info->mFileName, FileInfo.file_name + (Name - NameBuffer), sizeof(Info->mFileName));
					Info->mFileName[sizeof(Info->mFileName) - 1] = 0;

					mPieces[Name] = Info;
				}

				Info->SetZipFile(ZipFileType, FileIdx);
			}
			else
			{
				lcLibraryPrimitive* Primitive = FindPrimitive(Name);

				if (!Primitive)
					mPrimitives[Name] = new lcLibraryPrimitive(FileInfo.file_name + (Name - NameBuffer), ZipFileType, FileIdx, false, true);
				else
					Primitive->SetZipFile(ZipFileType, FileIdx);
			}
		}
		else if (!memcmp(Name, "P/", 2))
		{
			Name += 2;

			lcLibraryPrimitive* Primitive = FindPrimitive(Name);

			if (!Primitive)
				mPrimitives[Name] = new lcLibraryPrimitive(FileInfo.file_name + (Name - NameBuffer), ZipFileType, FileIdx, (memcmp(Name, "STU", 3) == 0), false);
			else
				Primitive->SetZipFile(ZipFileType, FileIdx);
		}
	}

	return true;
}

void lcPiecesLibrary::ReadArchiveDescriptions(const QString& OfficialFileName, const QString& UnofficialFileName)
{
	QFileInfo OfficialInfo(OfficialFileName);
	QFileInfo UnofficialInfo(UnofficialFileName);

	mArchiveCheckSum[0] = OfficialInfo.size();
#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
	mArchiveCheckSum[1] = OfficialInfo.lastModified().toMSecsSinceEpoch();
#else
	mArchiveCheckSum[1] = OfficialInfo.lastModified().toTime_t();
#endif
	if (!UnofficialFileName.isEmpty())
	{
		mArchiveCheckSum[2] = UnofficialInfo.size();
#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
		mArchiveCheckSum[3] = UnofficialInfo.lastModified().toMSecsSinceEpoch();
#else
		mArchiveCheckSum[3] = UnofficialInfo.lastModified().toTime_t();
#endif
	}
	else
	{
		mArchiveCheckSum[2] = 0;
		mArchiveCheckSum[3] = 0;
	}

	QString IndexFileName = QFileInfo(QDir(mCachePath), QLatin1String("index")).absoluteFilePath();

	if (!LoadCacheIndex(IndexFileName))
	{
		lcMemFile PieceFile;

		for (const auto PieceIt : mPieces)
		{
			PieceInfo* Info = PieceIt.second;

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

		SaveArchiveCacheIndex(IndexFileName);
	}
}

bool lcPiecesLibrary::OpenDirectory(const QDir& LibraryDir, bool ShowProgress)
{
	const QLatin1String BaseFolders[LC_NUM_FOLDERTYPES] = { QLatin1String("unofficial/"), QLatin1String("") };
	const int NumBaseFolders = sizeof(BaseFolders) / sizeof(BaseFolders[0]);

	QFileInfoList FileLists[NumBaseFolders];

	for (unsigned int BaseFolderIdx = 0; BaseFolderIdx < NumBaseFolders; BaseFolderIdx++)
	{
		QString ParstPath = QDir(LibraryDir.absoluteFilePath(BaseFolders[BaseFolderIdx])).absoluteFilePath(QLatin1String("parts/"));
		QDir Dir = QDir(ParstPath, QLatin1String("*.dat"), QDir::SortFlags(QDir::Name | QDir::IgnoreCase), QDir::Files | QDir::Hidden | QDir::Readable);
		FileLists[BaseFolderIdx] = Dir.entryInfoList();
	}

	if (FileLists[LC_FOLDER_OFFICIAL].isEmpty())
		return false;

	mHasUnofficial = !FileLists[LC_FOLDER_UNOFFICIAL].isEmpty();
	ReadDirectoryDescriptions(FileLists, ShowProgress);

	for (unsigned int BaseFolderIdx = 0; BaseFolderIdx < sizeof(BaseFolders) / sizeof(BaseFolders[0]); BaseFolderIdx++)
	{
		const char* PrimitiveDirectories[] = { "p/", "p/48/", "parts/s/" };
		bool SubFileDirectories[] = { false, false, true };
		QDir BaseDir(LibraryDir.absoluteFilePath(QLatin1String(BaseFolders[BaseFolderIdx])));

		for (int DirectoryIdx = 0; DirectoryIdx < (int)(sizeof(PrimitiveDirectories) / sizeof(PrimitiveDirectories[0])); DirectoryIdx++)
		{
			QDir Dir(BaseDir.absoluteFilePath(QLatin1String(PrimitiveDirectories[DirectoryIdx])), QLatin1String("*.dat"), QDir::SortFlags(QDir::Name | QDir::IgnoreCase), QDir::Files | QDir::Hidden | QDir::Readable);
			QStringList FileList = Dir.entryList();

			for (int FileIdx = 0; FileIdx < FileList.size(); FileIdx++)
			{
				char Name[LC_PIECE_NAME_LEN];
				QByteArray FileString = FileList[FileIdx].toLatin1();
				const char* Src = FileString;

				strcpy(Name, strchr(PrimitiveDirectories[DirectoryIdx], '/') + 1);
				strupr(Name);
				char* Dst = Name + strlen(Name);

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
				*Dst = 0;

				if (Dst - Name <= 4)
					continue;

				Dst -= 4;
				if (memcmp(Dst, ".DAT", 4))
					continue;

				if (mHasUnofficial && IsPrimitive(Name))
					continue;

				if (BaseFolderIdx == 0)
					mHasUnofficial = true;

				bool SubFile = SubFileDirectories[DirectoryIdx];
				mPrimitives[Name] = new lcLibraryPrimitive(QByteArray(strchr(PrimitiveDirectories[DirectoryIdx], '/') + 1) + FileString, LC_NUM_ZIPFILES, 0, !SubFile && (memcmp(Name, "STU", 3) == 0), SubFile);
			}
		}
	}

	QDir Dir(LibraryDir.absoluteFilePath(QLatin1String("parts/textures/")), QLatin1String("*.png"), QDir::SortFlags(QDir::Name | QDir::IgnoreCase), QDir::Files | QDir::Hidden | QDir::Readable);
	QStringList FileList = Dir.entryList();

	mTextures.AllocGrow(FileList.size());

	for (int FileIdx = 0; FileIdx < FileList.size(); FileIdx++)
	{
		char Name[LC_MAXPATH];
		QByteArray FileString = FileList[FileIdx].toLatin1();
		const char* Src = FileString;
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

void lcPiecesLibrary::ReadDirectoryDescriptions(const QFileInfoList (&FileLists)[LC_NUM_FOLDERTYPES], bool ShowProgress)
{
	QString IndexFileName = QFileInfo(QDir(mCachePath), QLatin1String("index")).absoluteFilePath();
	lcMemFile IndexFile;
	std::vector<const char*> CachedDescriptions;

	if (ReadDirectoryCacheFile(IndexFileName, IndexFile))
	{
		QString LibraryPath = IndexFile.ReadQString();

		if (LibraryPath == mLibraryDir.absolutePath())
		{
			int NumDescriptions = IndexFile.ReadU32();
			CachedDescriptions.reserve(NumDescriptions);

			while (NumDescriptions--)
			{
				const char* FileName = (const char*)IndexFile.mBuffer + IndexFile.GetPosition();
				CachedDescriptions.push_back(FileName);
				IndexFile.Seek(strlen(FileName) + 1, SEEK_CUR);
				const char* Description = (const char*)IndexFile.mBuffer + IndexFile.GetPosition(); 
				IndexFile.Seek(strlen(Description) + 1, SEEK_CUR);
				IndexFile.Seek(4 + 1 + 8, SEEK_CUR);
			}
		}
	}

	for (int FolderIdx = 0; FolderIdx < LC_NUM_FOLDERTYPES; FolderIdx++)
	{
		const QFileInfoList& FileList = FileLists[FolderIdx];

		for (int FileIdx = 0; FileIdx < FileList.size(); FileIdx++)
		{
			char Name[LC_PIECE_NAME_LEN];
			QByteArray FileString = FileList[FileIdx].fileName().toLatin1();
			const char* Src = FileString;
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
			*Dst = 0;

			if (FolderIdx == LC_FOLDER_OFFICIAL && mHasUnofficial && mPieces.find(Name) != mPieces.end())
				continue;

			PieceInfo* Info = new PieceInfo();

			strncpy(Info->mFileName, FileString, sizeof(Info->mFileName));
			Info->mFileName[sizeof(Info->mFileName) - 1] = 0;
			Info->mFolderType = FolderIdx;
			Info->mFolderIndex = FileIdx;

			mPieces[Name] = Info;
		}
	}

	QAtomicInt FilesLoaded;
	bool Modified = false;

	auto ReadDescriptions = [&FileLists, &CachedDescriptions, &FilesLoaded, &Modified](const std::pair<std::string, PieceInfo*>& Entry)
	{
		PieceInfo* Info = Entry.second;
		FilesLoaded.ref();

		lcDiskFile PieceFile(FileLists[Info->mFolderType][Info->mFolderIndex].absoluteFilePath());
		char Line[1024];

		if (!CachedDescriptions.empty())
		{
			auto DescriptionCompare = [](const void* Key, const void* Element)
			{
				return strcmp((const char*)Key, *(const char**)Element);
			};

			void* CachedDescription = bsearch(Info->mFileName, &CachedDescriptions.front(), CachedDescriptions.size(), sizeof(char*), DescriptionCompare);

			if (CachedDescription)
			{
				const char* FileName = *(const char**)CachedDescription;
				const char* Description = FileName + strlen(FileName) + 1;
				strcpy(Info->m_strDescription, Description);
				return;
			}
		}

		if (!PieceFile.Open(QIODevice::ReadOnly) || !PieceFile.ReadLine(Line, sizeof(Line)))
		{
			strcpy(Info->m_strDescription, "Unknown");
			return;
		}

		const char* Src = Line + 2;
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

		Modified = true;
	};

	QProgressDialog* ProgressDialog = new QProgressDialog(nullptr);
	ProgressDialog->setWindowFlags(ProgressDialog->windowFlags() & ~Qt::WindowCloseButtonHint);
	ProgressDialog->setWindowTitle(tr("Initializing"));
	ProgressDialog->setLabelText(tr("Loading Parts Library"));
	ProgressDialog->setMaximum(mPieces.size());
	ProgressDialog->setMinimum(0);
	ProgressDialog->setValue(0);
	ProgressDialog->setCancelButton(nullptr);
	ProgressDialog->setAutoReset(false);
	if (ShowProgress)
		ProgressDialog->show();

	QFuture<void> LoadFuture = QtConcurrent::map(mPieces, ReadDescriptions);

	while (!LoadFuture.isFinished())
	{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 3, 0) || QT_VERSION < QT_VERSION_CHECK(5, 0, 0) )
		ProgressDialog->setValue(FilesLoaded);
#else
		ProgressDialog->setValue(FilesLoaded.load());
#endif
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 3, 0) || QT_VERSION < QT_VERSION_CHECK(5, 0, 0) )
	ProgressDialog->setValue(FilesLoaded);
#else
	ProgressDialog->setValue(FilesLoaded.load());
#endif
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

	ProgressDialog->deleteLater();

	if (Modified)
	{
		lcMemFile IndexFile;

		IndexFile.WriteQString(mLibraryDir.absolutePath());

		IndexFile.WriteU32(mPieces.size());

		std::vector<PieceInfo*> SortedPieces;
		SortedPieces.reserve(mPieces.size());
		for (const auto PieceIt : mPieces)
			SortedPieces.push_back(PieceIt.second);

		auto PieceInfoCompare = [](PieceInfo* Info1, PieceInfo* Info2)
		{
			return strcmp(Info1->mFileName, Info2->mFileName) < 0;
		};

		std::sort(SortedPieces.begin(), SortedPieces.end(), PieceInfoCompare);

		for (const PieceInfo* Info : SortedPieces)
		{
			if (IndexFile.WriteBuffer(Info->mFileName, strlen(Info->mFileName) + 1) == 0)
				return;

			if (IndexFile.WriteBuffer(Info->m_strDescription, strlen(Info->m_strDescription) + 1) == 0)
				return;

			IndexFile.WriteU32(Info->mFlags);
			IndexFile.WriteU8(Info->mFolderType);

#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
			quint64 FileTime = FileLists[Info->mFolderType][Info->mFolderIndex].lastModified().toMSecsSinceEpoch();
#else
			quint64 FileTime = FileLists[Info->mFolderType][Info->mFolderIndex].lastModified().toTime_t();
#endif
		
			IndexFile.WriteU64(FileTime);
		}

		WriteDirectoryCacheFile(IndexFileName, IndexFile);
	}
}

bool lcPiecesLibrary::ReadArchiveCacheFile(const QString& FileName, lcMemFile& CacheFile)
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

bool lcPiecesLibrary::WriteArchiveCacheFile(const QString& FileName, lcMemFile& CacheFile)
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

	quint32 UncompressedSize = (quint32)CacheFile.GetLength();

	if (File.write((char*)&UncompressedSize, sizeof(UncompressedSize)) == -1)
		return false;

	const size_t BufferSize = 16384;
	char WriteBuffer[BufferSize];
	z_stream Stream;
	quint32 Crc32 = 0;

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
		uInt Read = (uInt)lcMin(CacheFile.GetLength() - (BufferIn - CacheFile.mBuffer), BufferSize);
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

bool lcPiecesLibrary::ReadDirectoryCacheFile(const QString& FileName, lcMemFile& CacheFile)
{
	QFile File(FileName);

	if (!File.open(QIODevice::ReadOnly))
		return false;

	quint32 CacheVersion, CacheFlags;

	if (File.read((char*)&CacheVersion, sizeof(CacheVersion)) == -1 || CacheVersion != LC_LIBRARY_CACHE_VERSION)
		return false;

	if (File.read((char*)&CacheFlags, sizeof(CacheFlags)) == -1 || CacheFlags != LC_LIBRARY_CACHE_DIRECTORY)
		return false;

	quint32 UncompressedSize;

	if (File.read((char*)&UncompressedSize, sizeof(UncompressedSize)) == -1)
		return false;

	QByteArray Data = qUncompress(File.readAll());
	if (Data.isEmpty())
		return false;

	CacheFile.SetLength(Data.size());
	CacheFile.Seek(0, SEEK_SET);
	CacheFile.WriteBuffer(Data.constData(), Data.size());
	CacheFile.Seek(0, SEEK_SET);

	return true;
}

bool lcPiecesLibrary::WriteDirectoryCacheFile(const QString& FileName, lcMemFile& CacheFile)
{
	QFile File(FileName);

	if (!File.open(QIODevice::WriteOnly))
		return false;

	quint32 CacheVersion = LC_LIBRARY_CACHE_VERSION;
	if (File.write((char*)&CacheVersion, sizeof(CacheVersion)) == -1)
		return false;

	quint32 CacheFlags = LC_LIBRARY_CACHE_DIRECTORY;
	if (File.write((char*)&CacheFlags, sizeof(CacheFlags)) == -1)
		return false;

	quint32 UncompressedSize = (quint32)CacheFile.GetLength();
	if (File.write((char*)&UncompressedSize, sizeof(UncompressedSize)) == -1)
		return false;

	File.write(qCompress(CacheFile.mBuffer, CacheFile.GetLength()));

	return true;
}

bool lcPiecesLibrary::LoadCacheIndex(const QString& FileName)
{
	lcMemFile IndexFile;

	if (!ReadArchiveCacheFile(FileName, IndexFile))
		return false;

	quint32 NumFiles;

	if (IndexFile.ReadBuffer((char*)&NumFiles, sizeof(NumFiles)) == 0 || NumFiles != mPieces.size())
		return false;

	for (const auto PieceIt : mPieces)
	{
		PieceInfo* Info = PieceIt.second;
		quint8 Length;

		if (IndexFile.ReadBuffer((char*)&Length, sizeof(Length)) == 0 || Length >= sizeof(Info->m_strDescription))
			return false;

		if (IndexFile.ReadBuffer((char*)Info->m_strDescription, Length) == 0 || IndexFile.ReadBuffer((char*)&Info->mFlags, sizeof(Info->mFlags)) == 0)
			return false;

		Info->m_strDescription[Length] = 0;
	}

	return true;
}

bool lcPiecesLibrary::SaveArchiveCacheIndex(const QString& FileName)
{
	lcMemFile IndexFile;

	quint32 NumFiles = mPieces.size();

	if (IndexFile.WriteBuffer((char*)&NumFiles, sizeof(NumFiles)) == 0)
		return false;

	for (const auto PieceIt : mPieces)
	{
		PieceInfo* Info = PieceIt.second;
		quint8 Length = (quint8)strlen(Info->m_strDescription);

		if (IndexFile.WriteBuffer((char*)&Length, sizeof(Length)) == 0)
			return false;

		if (IndexFile.WriteBuffer((char*)Info->m_strDescription, Length) == 0 || IndexFile.WriteBuffer((char*)&Info->mFlags, sizeof(Info->mFlags)) == 0)
			return false;
	}

	return WriteArchiveCacheFile(FileName, IndexFile);
}

bool lcPiecesLibrary::LoadCachePiece(PieceInfo* Info)
{
	QString FileName = QFileInfo(QDir(mCachePath), QString::fromLatin1(Info->mFileName)).absoluteFilePath();
	lcMemFile MeshData;

	if (!ReadArchiveCacheFile(FileName, MeshData))
		return false;

	quint32 Flags;
	if (MeshData.ReadBuffer((char*)&Flags, sizeof(Flags)) == 0)
		return false;

	Info->mFlags = Flags;

	lcMesh* Mesh = new lcMesh;
	if (Mesh->FileLoad(MeshData))
	{
		Info->SetMesh(Mesh);
		return true;
	}
	else
	{
		delete Mesh;
		return false;
	}
}

bool lcPiecesLibrary::SaveCachePiece(PieceInfo* Info)
{
	lcMemFile MeshData;

	quint32 Flags = Info->mFlags;
	if (MeshData.WriteBuffer((char*)&Flags, sizeof(Flags)) == 0)
		return false;

	if (!Info->GetMesh()->FileSave(MeshData))
		return false;

	QString FileName = QFileInfo(QDir(mCachePath), QString::fromLatin1(Info->mFileName)).absoluteFilePath();

	return WriteArchiveCacheFile(FileName, MeshData);
}

static void lcLoadPieceFuture(lcPiecesLibrary* Library)
{
	Library->LoadQueuedPiece();
}

class lcSleeper : public QThread
{
public:
	static void msleep(unsigned long Msecs)
	{
		QThread::msleep(Msecs);
	}
};

void lcPiecesLibrary::LoadPieceInfo(PieceInfo* Info, bool Wait, bool Priority)
{
	QMutexLocker LoadLock(&mLoadMutex);

	if (Wait)
	{
		if (Info->AddRef() == 1)
			Info->Load();
		else
		{
			if (Info->mState == LC_PIECEINFO_UNLOADED)
			{
				Info->Load();
				emit PartLoaded(Info);
			}
			else
			{
				LoadLock.unlock();

				while (Info->mState != LC_PIECEINFO_LOADED)
					lcSleeper::msleep(10);
			}
		}
	}
	else
	{
		if (Info->AddRef() == 1)
		{
			if (Priority)
				mLoadQueue.prepend(Info);
			else
				mLoadQueue.append(Info);

			mLoadFutures.append(QtConcurrent::run(lcLoadPieceFuture, this));
		}
	}
}

void lcPiecesLibrary::ReleasePieceInfo(PieceInfo* Info)
{
	QMutexLocker LoadLock(&mLoadMutex);

	if (Info->GetRefCount() == 0 || Info->Release() == 0)
		Info->Unload();
}

void lcPiecesLibrary::LoadQueuedPiece()
{
	mLoadMutex.lock();

	PieceInfo* Info = nullptr;

	while (!mLoadQueue.isEmpty())
	{
		Info = mLoadQueue.takeFirst();

		if (Info->mState == LC_PIECEINFO_UNLOADED && Info->GetRefCount() > 0)
		{
			Info->mState = LC_PIECEINFO_LOADING;
			break;
		}

		Info = nullptr;
	}

	mLoadMutex.unlock();

	if (Info)
		Info->Load();

	emit PartLoaded(Info);
}

void lcPiecesLibrary::WaitForLoadQueue()
{
	for (QFuture<void>& Future : mLoadFutures)
		Future.waitForFinished();
	mLoadFutures.clear();
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
			LC_MESH_CONDITIONAL_LINES
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

bool lcPiecesLibrary::LoadPieceData(PieceInfo* Info)
{
	lcLibraryMeshData MeshData;
	lcArray<lcLibraryTextureMap> TextureStack;

	bool Loaded = false;
	bool SaveCache = false;

	if (Info->mZipFileType != LC_NUM_ZIPFILES && mZipFiles[Info->mZipFileType])
	{
		if (LoadCachePiece(Info))
			return true;

		lcMemFile PieceFile;

		if (mZipFiles[Info->mZipFileType]->ExtractFile(Info->mZipFileIndex, PieceFile))
			Loaded = ReadMeshData(PieceFile, lcMatrix44Identity(), 16, false, TextureStack, MeshData, LC_MESHDATA_SHARED, true, nullptr, false);

		SaveCache = Loaded && (Info->mZipFileType == LC_ZIPFILE_OFFICIAL);
	}
	else
	{
		char FileName[LC_MAXPATH];
		lcDiskFile PieceFile;

		if (mHasUnofficial)
		{
			sprintf(FileName, "unofficial/parts/%s", Info->mFileName);
			PieceFile.SetFileName(mLibraryDir.absoluteFilePath(QLatin1String(FileName)));
			if (PieceFile.Open(QIODevice::ReadOnly))
				Loaded = ReadMeshData(PieceFile, lcMatrix44Identity(), 16, false, TextureStack, MeshData, LC_MESHDATA_SHARED, true, nullptr, false);
		}

		if (!Loaded)
		{
			sprintf(FileName, "parts/%s", Info->mFileName);
			PieceFile.SetFileName(mLibraryDir.absoluteFilePath(QLatin1String(FileName)));
			if (PieceFile.Open(QIODevice::ReadOnly))
				Loaded = ReadMeshData(PieceFile, lcMatrix44Identity(), 16, false, TextureStack, MeshData, LC_MESHDATA_SHARED, true, nullptr, false);
		}
	}
	
	if (!Loaded || mCancelLoading)
		return false;

	CreateMesh(Info, MeshData);

	if (SaveCache)
		SaveCachePiece(Info);

	return true;
}

lcMesh* lcPiecesLibrary::CreateMesh(PieceInfo* Info, lcLibraryMeshData& MeshData)
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

	quint16 NumSections[LC_NUM_MESH_LODS];
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
			MergeSection.Lod = nullptr;
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
				MergeSection.Shared = nullptr;
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
		const lcArray<lcLibraryMeshVertex>& Vertices = MeshData.mVertices[MeshDataIdx];

		for (int VertexIdx = 0; VertexIdx < Vertices.GetSize(); VertexIdx++)
		{
			lcVertex& DstVertex = *DstVerts++;
			const lcLibraryMeshVertex& SrcVertex = Vertices[VertexIdx];

			DstVertex.Position = lcVector3LDrawToLeoCAD(SrcVertex.Position);
			DstVertex.Normal = lcPackNormal(lcVector3LDrawToLeoCAD(SrcVertex.Normal));

			lcVector3& Position = DstVertex.Position;
			Min = lcMin(Min, Position);
			Max = lcMax(Max, Position);
		}
	}

	lcVertexTextured* DstTexturedVerts = (lcVertexTextured*)DstVerts;

	for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
	{
		const lcArray<lcLibraryMeshVertexTextured>& TexturedVertices = MeshData.mTexturedVertices[MeshDataIdx];

		for (int VertexIdx = 0; VertexIdx < TexturedVertices.GetSize(); VertexIdx++)
		{
			lcVertexTextured& DstVertex = *DstTexturedVerts++;
			const lcLibraryMeshVertexTextured& SrcVertex = TexturedVertices[VertexIdx];

			DstVertex.Position = lcVector3LDrawToLeoCAD(SrcVertex.Position);
			DstVertex.Normal = lcPackNormal(lcVector3LDrawToLeoCAD(SrcVertex.Normal));
			DstVertex.TexCoord = SrcVertex.TexCoord;

			lcVector3& Position = DstVertex.Position;
			Min = lcMin(Min, Position);
			Max = lcMax(Max, Position);
		}
	}

	Mesh->mBoundingBox.Max = Max;
	Mesh->mBoundingBox.Min = Min;
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
					quint16 BaseVertex = DstSection.Texture ? BaseTexturedVertices[LC_MESHDATA_SHARED] : BaseVertices[LC_MESHDATA_SHARED];
					lcLibraryMeshSection* SrcSection = MergeSection.Shared;

					for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
						*Index++ = BaseVertex + SrcSection->mIndices[IndexIdx];

					DstSection.NumIndices += SrcSection->mIndices.GetSize();
				}

				if (MergeSection.Lod)
				{
					quint16 BaseVertex = DstSection.Texture ? BaseTexturedVertices[LodIdx] : BaseVertices[LodIdx];
					lcLibraryMeshSection* SrcSection = MergeSection.Lod;

					for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
						*Index++ = BaseVertex + SrcSection->mIndices[IndexIdx];

					DstSection.NumIndices += SrcSection->mIndices.GetSize();
				}
			}
			else
			{
				DstSection.IndexOffset = NumIndices * 4;

				quint32* Index = (quint32*)Mesh->mIndexData + NumIndices;

				if (MergeSection.Shared)
				{
					quint32 BaseVertex = DstSection.Texture ? BaseTexturedVertices[LC_MESHDATA_SHARED] : BaseVertices[LC_MESHDATA_SHARED];
					lcLibraryMeshSection* SrcSection = MergeSection.Shared;

					for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
						*Index++ = BaseVertex + SrcSection->mIndices[IndexIdx];

					DstSection.NumIndices += SrcSection->mIndices.GetSize();
				}

				if (MergeSection.Lod)
				{
					quint32 BaseVertex = DstSection.Texture ? BaseTexturedVertices[LodIdx] : BaseVertices[LodIdx];
					lcLibraryMeshSection* SrcSection = MergeSection.Lod;

					for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
						*Index++ = BaseVertex + SrcSection->mIndices[IndexIdx];

					DstSection.NumIndices += SrcSection->mIndices.GetSize();
				}
			}

			if (Info)
			{
				if (DstSection.PrimitiveType == LC_MESH_TRIANGLES || DstSection.PrimitiveType == LC_MESH_TEXTURED_TRIANGLES)
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

				if (DstSection.PrimitiveType == LC_MESH_TEXTURED_TRIANGLES || DstSection.PrimitiveType == LC_MESH_TEXTURED_LINES)
					Info->mFlags |= LC_PIECE_HAS_TEXTURE;
			}

			NumIndices += DstSection.NumIndices;
		}
	}
	/*
	for (int SectionIdx = 0; SectionIdx < MeshData.mSections.GetSize(); SectionIdx++)
	{
		lcMeshSection& DstSection = Mesh->mSections[SectionIdx];
		lcLibraryMeshSection* SrcSection = MeshData.mSections[SectionIdx];

		DstSection.ColorIndex = SrcSection->mColor;
		DstSection.PrimitiveType = SrcSection->mPrimitiveType;
		DstSection.NumIndices = SrcSection->mIndices.GetSize();
		DstSection.Texture = SrcSection->mTexture;

		if (DstSection.Texture)
			DstSection.Texture->AddRef();

		if (Mesh->mNumVertices < 0x10000)
		{
			DstSection.IndexOffset = NumIndices * 2;

			quint16* Index = (quint16*)Mesh->mIndexData + NumIndices;

			for (int IndexIdx = 0; IndexIdx < DstSection.NumIndices; IndexIdx++)
				*Index++ = SrcSection->mIndices[IndexIdx];
		}
		else
		{
			DstSection.IndexOffset = NumIndices * 4;

			quint32* Index = (quint32*)Mesh->mIndexData + NumIndices;

			for (int IndexIdx = 0; IndexIdx < DstSection.NumIndices; IndexIdx++)
				*Index++ = SrcSection->mIndices[IndexIdx];
		}

		if (DstSection.PrimitiveType == LC_MESH_TRIANGLES || DstSection.PrimitiveType == LC_MESH_TEXTURED_TRIANGLES)
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

	if (Info)
		Info->SetMesh(Mesh);

	return Mesh;
}

void lcPiecesLibrary::ReleaseBuffers(lcContext* Context)
{
	Context->DestroyVertexBuffer(mVertexBuffer);
	Context->DestroyIndexBuffer(mIndexBuffer);
	mBuffersDirty = true;
}

void lcPiecesLibrary::UpdateBuffers(lcContext* Context)
{
	if (!gSupportsVertexBufferObject || !mBuffersDirty)
		return;

	int VertexDataSize = 0;
	int IndexDataSize = 0;

	for (const auto PieceIt : mPieces)
	{
		PieceInfo* Info = PieceIt.second;
		lcMesh* Mesh = Info->IsPlaceholder() ? gPlaceholderMesh : Info->GetMesh();

		if (!Mesh)
			continue;

		if (Mesh->mVertexDataSize > 16 * 1024 * 1024 || Mesh->mIndexDataSize > 16 * 1024 * 1024)
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

	for (const auto PieceIt : mPieces)
	{
		PieceInfo* Info = PieceIt.second;
		lcMesh* Mesh = Info->IsPlaceholder() ? gPlaceholderMesh : Info->GetMesh();

		if (!Mesh)
			continue;

		if (Mesh->mVertexDataSize > 16 * 1024 * 1024 || Mesh->mIndexDataSize > 16 * 1024 * 1024)
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

void lcPiecesLibrary::UnloadUnusedParts()
{
	QMutexLocker LoadLock(&mLoadMutex);

	for (const auto PieceIt : mPieces)
	{
		PieceInfo* Info = PieceIt.second;
		if (Info->GetRefCount() == 0 && Info->mState != LC_PIECEINFO_UNLOADED)
			ReleasePieceInfo(Info);
	}
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
		sprintf(FileName, "parts/textures/%s.png", Name);

		if (!Texture->Load(mLibraryDir.absoluteFilePath(QLatin1String(FileName))))
			return false;
	}

	return true;
}

void lcPiecesLibrary::ReleaseTexture(lcTexture* Texture)
{
	QMutexLocker LoadLock(&mLoadMutex);

	if (Texture->Release() == 0 && Texture->IsTemporary())
	{
		mTextures.Remove(Texture);
		delete Texture;
	}
}

bool lcPiecesLibrary::LoadPrimitive(lcLibraryPrimitive* Primitive)
{
	mLoadMutex.lock();

	if (Primitive->mState == lcPrimitiveState::NOT_LOADED)
		Primitive->mState = lcPrimitiveState::LOADING;
	else
	{
		mLoadMutex.unlock();

		while (Primitive->mState == lcPrimitiveState::LOADING)
			lcSleeper::msleep(5);

		return Primitive->mState == lcPrimitiveState::LOADED;
	}

	mLoadMutex.unlock();

	lcArray<lcLibraryTextureMap> TextureStack;

	if (mZipFiles[LC_ZIPFILE_OFFICIAL])
	{
		lcLibraryPrimitive* LowPrimitive = nullptr;

		if (Primitive->mStud && strncmp(Primitive->mName, "8/", 2))
		{
			char Name[LC_PIECE_NAME_LEN];
			strcpy(Name, "8/");
			strcat(Name, Primitive->mName);

			LowPrimitive = FindPrimitive(Name);
		}

		lcMemFile PrimFile;

		if (!mZipFiles[Primitive->mZipFileType]->ExtractFile(Primitive->mZipFileIndex, PrimFile))
			return false;

		if (!LowPrimitive)
		{
			if (!ReadMeshData(PrimFile, lcMatrix44Identity(), 16, false, TextureStack, Primitive->mMeshData, LC_MESHDATA_SHARED, true, nullptr, false))
				return false;
		}
		else
		{
			if (!ReadMeshData(PrimFile, lcMatrix44Identity(), 16, false, TextureStack, Primitive->mMeshData, LC_MESHDATA_HIGH, true, nullptr, false))
				return false;

			if (!mZipFiles[LowPrimitive->mZipFileType]->ExtractFile(LowPrimitive->mZipFileIndex, PrimFile))
				return false;

			TextureStack.RemoveAll();

			if (!ReadMeshData(PrimFile, lcMatrix44Identity(), 16, false, TextureStack, Primitive->mMeshData, LC_MESHDATA_LOW, true, nullptr, false))
				return false;
		}
	}
	else
	{
		char FileName[LC_MAXPATH];
		lcDiskFile PrimFile;
		bool Found = false;

		if (mHasUnofficial)
		{
			if (Primitive->mSubFile)
				sprintf(FileName, "unofficial/parts/%s", Primitive->mName);
			else
				sprintf(FileName, "unofficial/p/%s", Primitive->mName);
			PrimFile.SetFileName(mLibraryDir.absoluteFilePath(QLatin1String(FileName)));
			Found = PrimFile.Open(QIODevice::ReadOnly);
		}

		if (!Found)
		{
			if (Primitive->mSubFile)
				sprintf(FileName, "parts/%s", Primitive->mName);
			else
				sprintf(FileName, "p/%s", Primitive->mName);
			PrimFile.SetFileName(mLibraryDir.absoluteFilePath(QLatin1String(FileName)));
			Found = PrimFile.Open(QIODevice::ReadOnly);
		}

		if (!Found || !ReadMeshData(PrimFile, lcMatrix44Identity(), 16, false, TextureStack, Primitive->mMeshData, LC_MESHDATA_SHARED, true, nullptr, false))
			return false;
	}

	mLoadMutex.lock();
	Primitive->mState = lcPrimitiveState::LOADED;
	mLoadMutex.unlock();

	return true;
}

bool lcPiecesLibrary::ReadMeshData(lcFile& File, const lcMatrix44& CurrentTransform, quint32 CurrentColorCode, bool InvertWinding, lcArray<lcLibraryTextureMap>& TextureStack, lcLibraryMeshData& MeshData, lcMeshDataType MeshDataType, bool Optimize, Project* CurrentProject, bool SearchProjectFolder)
{
	char Buffer[1024];
	char* Line;
	bool InvertNext = false;
	bool WindingCCW = !InvertWinding;

	while (File.ReadLine(Buffer, sizeof(Buffer)))
	{
		if (mCancelLoading)
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
						Map.Texture = FindTexture(FileName, CurrentProject, SearchProjectFolder);

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

		lcLibraryTextureMap* TextureMap = nullptr;

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

				lcLibraryPrimitive* Primitive = FindPrimitive(FileName);
				lcMatrix44 IncludeTransform(lcVector4(fm[3], fm[6], fm[9], 0.0f), lcVector4(fm[4], fm[7], fm[10], 0.0f), lcVector4(fm[5], fm[8], fm[11], 0.0f), lcVector4(fm[0], fm[1], fm[2], 1.0f));
				IncludeTransform = lcMul(IncludeTransform, CurrentTransform);
				bool Mirror = IncludeTransform.Determinant() < 0.0f;

				if (Primitive)
				{
					if (Primitive->mState != lcPrimitiveState::LOADED && !LoadPrimitive(Primitive))
						break;

					if (Primitive->mStud)
						MeshData.AddMeshDataNoDuplicateCheck(Primitive->mMeshData, IncludeTransform, ColorCode, Mirror ^ InvertNext, InvertNext, TextureMap, MeshDataType);
					else if (!Primitive->mSubFile)
					{
						if (Optimize)
							MeshData.AddMeshData(Primitive->mMeshData, IncludeTransform, ColorCode, Mirror ^ InvertNext, InvertNext, TextureMap, MeshDataType);
						else
							MeshData.AddMeshDataNoDuplicateCheck(Primitive->mMeshData, IncludeTransform, ColorCode, Mirror ^ InvertNext, InvertNext, TextureMap, MeshDataType);
					}
					else
					{
						if (mZipFiles[LC_ZIPFILE_OFFICIAL])
						{
							lcMemFile IncludeFile;

							if (mZipFiles[Primitive->mZipFileType]->ExtractFile(Primitive->mZipFileIndex, IncludeFile))
								ReadMeshData(IncludeFile, IncludeTransform, ColorCode, Mirror ^ InvertNext, TextureStack, MeshData, MeshDataType, Optimize, CurrentProject, SearchProjectFolder);
						}
						else
						{
							char Name[LC_PIECE_NAME_LEN];
							strcpy(Name, Primitive->mName);
							strlwr(Name);

							lcDiskFile IncludeFile;
							bool Found = false;

							if (mHasUnofficial)
							{
								if (Primitive->mSubFile)
									sprintf(FileName, "unofficial/parts/%s", Name);
								else
									sprintf(FileName, "unofficial/p/%s", Name);
								IncludeFile.SetFileName(mLibraryDir.absoluteFilePath(QLatin1String(FileName)));
								Found = IncludeFile.Open(QIODevice::ReadOnly);
							}

							if (!Found)
							{
								if (Primitive->mSubFile)
									sprintf(FileName, "parts/%s", Name);
								else
									sprintf(FileName, "p/%s", Name);
								IncludeFile.SetFileName(mLibraryDir.absoluteFilePath(QLatin1String(FileName)));
								Found = IncludeFile.Open(QIODevice::ReadOnly);
							}
							if (Found)
								ReadMeshData(IncludeFile, IncludeTransform, ColorCode, Mirror ^ InvertNext, TextureStack, MeshData, MeshDataType, Optimize, CurrentProject, SearchProjectFolder);
						}
					}
				}
				else
				{
					const auto PieceIt = mPieces.find(FileName);

					if (PieceIt != mPieces.end())
					{
						PieceInfo* Info = PieceIt->second;

						if (mZipFiles[LC_ZIPFILE_OFFICIAL] && Info->mZipFileType != LC_NUM_ZIPFILES)
						{
							lcMemFile IncludeFile;

							if (mZipFiles[Info->mZipFileType]->ExtractFile(Info->mZipFileIndex, IncludeFile))
								ReadMeshData(IncludeFile, IncludeTransform, ColorCode, Mirror ^ InvertNext, TextureStack, MeshData, MeshDataType, Optimize, CurrentProject, SearchProjectFolder);
						}
						else
						{
							lcDiskFile IncludeFile;
							bool Found = false;

							if (mHasUnofficial)
							{
								sprintf(FileName, "unofficial/parts/%s", Info->mFileName);
								IncludeFile.SetFileName(mLibraryDir.absoluteFilePath(QLatin1String(FileName)));
								Found = IncludeFile.Open(QIODevice::ReadOnly);
							}

							if (!Found)
							{
								sprintf(FileName, "parts/%s", Info->mFileName);
								IncludeFile.SetFileName(mLibraryDir.absoluteFilePath(QLatin1String(FileName)));
								Found = IncludeFile.Open(QIODevice::ReadOnly);
							}

							if (Found)
								ReadMeshData(IncludeFile, IncludeTransform, ColorCode, Mirror ^ InvertNext, TextureStack, MeshData, MeshDataType, Optimize, CurrentProject, SearchProjectFolder);
						}

						break;
					}
				}
			} break;

		case 2:
			sscanf(Line, "%d %i %f %f %f %f %f %f", &LineType, &Dummy, &Points[0].x, &Points[0].y, &Points[0].z, &Points[1].x, &Points[1].y, &Points[1].z);

			Points[0] = lcMul31(Points[0], CurrentTransform);
			Points[1] = lcMul31(Points[1], CurrentTransform);

			if (TextureMap)
			{
				MeshData.AddTexturedLine(MeshDataType, LineType, ColorCode, WindingCCW, *TextureMap, Points, Optimize);

				if (TextureMap->Next)
					TextureStack.RemoveIndex(TextureStack.GetSize() - 1);
			}
			else
				MeshData.AddLine(MeshDataType, LineType, ColorCode, WindingCCW, Points, Optimize);
			break;

		case 3:
			sscanf(Line, "%d %i %f %f %f %f %f %f %f %f %f", &LineType, &Dummy, &Points[0].x, &Points[0].y, &Points[0].z,
			       &Points[1].x, &Points[1].y, &Points[1].z, &Points[2].x, &Points[2].y, &Points[2].z);

			Points[0] = lcMul31(Points[0], CurrentTransform);
			Points[1] = lcMul31(Points[1], CurrentTransform);
			Points[2] = lcMul31(Points[2], CurrentTransform);

			if (TextureMap)
			{
				MeshData.AddTexturedLine(MeshDataType, LineType, ColorCode, WindingCCW, *TextureMap, Points, Optimize);

				if (TextureMap->Next)
					TextureStack.RemoveIndex(TextureStack.GetSize() - 1);
			}
			else
				MeshData.AddLine(MeshDataType, LineType, ColorCode, WindingCCW, Points, Optimize);
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
				MeshData.AddTexturedLine(MeshDataType, LineType, ColorCode, WindingCCW, *TextureMap, Points, Optimize);

				if (TextureMap->Next)
					TextureStack.RemoveIndex(TextureStack.GetSize() - 1);
			}
			else
				MeshData.AddLine(MeshDataType, LineType, ColorCode, WindingCCW, Points, Optimize);
			break;

		case 5:
			sscanf(Line, "%d %i %f %f %f %f %f %f %f %f %f %f %f %f", &LineType, &Dummy, &Points[0].x, &Points[0].y, &Points[0].z,
			       &Points[1].x, &Points[1].y, &Points[1].z, &Points[2].x, &Points[2].y, &Points[2].z, &Points[3].x, &Points[3].y, &Points[3].z);

			Points[0] = lcMul31(Points[0], CurrentTransform);
			Points[1] = lcMul31(Points[1], CurrentTransform);
			Points[2] = lcMul31(Points[2], CurrentTransform);
			Points[3] = lcMul31(Points[3], CurrentTransform);

			MeshData.AddLine(MeshDataType, LineType, ColorCode, WindingCCW, Points, Optimize);
			break;
		}

		InvertNext = false;
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

lcLibraryMeshSection* lcLibraryMeshData::AddSection(lcMeshDataType MeshDataType, lcMeshPrimitiveType PrimitiveType, quint32 ColorCode, lcTexture* Texture)
{
	lcArray<lcLibraryMeshSection*>& Sections = mSections[MeshDataType];
	lcLibraryMeshSection* Section;

	for (int SectionIdx = 0; SectionIdx < Sections.GetSize(); SectionIdx++)
	{
		Section = Sections[SectionIdx];

		if (Section->mColor == ColorCode && Section->mPrimitiveType == PrimitiveType && Section->mTexture == Texture)
			return Section;
	}

	Section = new lcLibraryMeshSection(PrimitiveType, ColorCode, Texture);
	Sections.Add(Section);

	return Section;
}

void lcLibraryMeshData::AddVertices(lcMeshDataType MeshDataType, int VertexCount, int* BaseVertex, lcLibraryMeshVertex** VertexBuffer)
{
	lcArray<lcLibraryMeshVertex>& Vertices = mVertices[MeshDataType];
	int CurrentSize = Vertices.GetSize();

	Vertices.SetSize(CurrentSize + VertexCount);

	*BaseVertex = CurrentSize;
	*VertexBuffer = &Vertices[CurrentSize];
}

quint32 lcLibraryMeshData::AddVertex(lcMeshDataType MeshDataType, const lcVector3& Position, bool Optimize)
{
	lcArray<lcLibraryMeshVertex>& VertexArray = mVertices[MeshDataType];

	if (Optimize)
	{
		for (int VertexIdx = VertexArray.GetSize() - 1; VertexIdx >= 0; VertexIdx--)
		{
			lcLibraryMeshVertex& Vertex = VertexArray[VertexIdx];

			if (Position == Vertex.Position)
				return VertexIdx;
		}
	}

	lcLibraryMeshVertex& Vertex = VertexArray.Add();
	Vertex.Position = Position;
	Vertex.Normal = lcVector3(0.0f, 0.0f, 0.0f);
	Vertex.NormalWeight = 0.0f;

	return VertexArray.GetSize() - 1;
}

const float DistanceEpsilon = 0.05f;

quint32 lcLibraryMeshData::AddVertex(lcMeshDataType MeshDataType, const lcVector3& Position, const lcVector3& Normal, bool Optimize)
{
	lcArray<lcLibraryMeshVertex>& VertexArray = mVertices[MeshDataType];

	if (Optimize)
	{
		for (int VertexIdx = VertexArray.GetSize() - 1; VertexIdx >= 0; VertexIdx--)
		{
			lcLibraryMeshVertex& Vertex = VertexArray[VertexIdx];

			if (fabsf(Position.x - Vertex.Position.x) < DistanceEpsilon && fabsf(Position.y - Vertex.Position.y) < DistanceEpsilon && fabsf(Position.z - Vertex.Position.z) < DistanceEpsilon)
//			if (Position == Vertex.Position)
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
	}

	lcLibraryMeshVertex& Vertex = VertexArray.Add();
	Vertex.Position = Position;
	Vertex.Normal = Normal;
	Vertex.NormalWeight = 1.0f;

	return VertexArray.GetSize() - 1;
}

quint32 lcLibraryMeshData::AddTexturedVertex(lcMeshDataType MeshDataType, const lcVector3& Position, const lcVector2& TexCoord, bool Optimize)
{
	lcArray<lcLibraryMeshVertexTextured>& VertexArray = mTexturedVertices[MeshDataType];

	if (Optimize)
	{
		for (int VertexIdx = VertexArray.GetSize() - 1; VertexIdx >= 0; VertexIdx--)
		{
			lcLibraryMeshVertexTextured& Vertex = VertexArray[VertexIdx];

			if (Position == Vertex.Position && TexCoord == Vertex.TexCoord)
				return VertexIdx;
		}
	}

	lcLibraryMeshVertexTextured& Vertex = VertexArray.Add();
	Vertex.Position = Position;
	Vertex.Normal = lcVector3(0.0f, 0.0f, 0.0f);
	Vertex.NormalWeight = 0.0f;
	Vertex.TexCoord = TexCoord;

	return VertexArray.GetSize() - 1;
}

quint32 lcLibraryMeshData::AddTexturedVertex(lcMeshDataType MeshDataType, const lcVector3& Position, const lcVector3& Normal, const lcVector2& TexCoord, bool Optimize)
{
	lcArray<lcLibraryMeshVertexTextured>& VertexArray = mTexturedVertices[MeshDataType];

	if (Optimize)
	{
		for (int VertexIdx = VertexArray.GetSize() - 1; VertexIdx >= 0; VertexIdx--)
		{
			lcLibraryMeshVertexTextured& Vertex = VertexArray[VertexIdx];

			if (Position == Vertex.Position && TexCoord == Vertex.TexCoord)
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
	}

	lcLibraryMeshVertexTextured& Vertex = VertexArray.Add();
	Vertex.Position = Position;
	Vertex.Normal = Normal;
	Vertex.NormalWeight = 1.0f;
	Vertex.TexCoord = TexCoord;

	return VertexArray.GetSize() - 1;
}

void lcLibraryMeshData::AddIndices(lcMeshDataType MeshDataType, lcMeshPrimitiveType PrimitiveType, quint32 ColorCode, int IndexCount, quint32** IndexBuffer)
{
	lcLibraryMeshSection* Section = AddSection(MeshDataType, PrimitiveType, ColorCode, nullptr);
	lcArray<quint32>& Indices = Section->mIndices;
	int CurrentSize = Indices.GetSize();

	Indices.SetSize(CurrentSize + IndexCount);

	*IndexBuffer = &Indices[CurrentSize];
}

void lcLibraryMeshData::AddLine(lcMeshDataType MeshDataType, int LineType, quint32 ColorCode, bool WindingCCW, const lcVector3* Vertices, bool Optimize)
{
	lcMeshPrimitiveType PrimitiveTypes[4] = { LC_MESH_LINES, LC_MESH_TRIANGLES, LC_MESH_TRIANGLES, LC_MESH_CONDITIONAL_LINES };
	lcMeshPrimitiveType PrimitiveType = PrimitiveTypes[LineType - 2];
	lcLibraryMeshSection* Section = AddSection(MeshDataType, PrimitiveType, ColorCode, nullptr);

	int QuadIndices[4] = { 0, 1, 2, 3 };
	int Indices[4] = { -1, -1, -1, -1 };

	if (LineType == 3 || LineType == 4)
	{
		if (LineType == 4)
			TestQuad(QuadIndices, Vertices);

		lcVector3 Normal = lcNormalize(lcCross(Vertices[1] - Vertices[0], Vertices[2] - Vertices[0]));

		if (!WindingCCW)
			Normal = -Normal;

		for (int IndexIdx = 0; IndexIdx < lcMin(LineType, 4); IndexIdx++)
		{
			const lcVector3& Position = Vertices[QuadIndices[IndexIdx]];
			Indices[IndexIdx] = AddVertex(MeshDataType, Position, Normal, Optimize);
		}
	}
	else
	{
		for (int IndexIdx = 0; IndexIdx < lcMin(LineType, 4); IndexIdx++)
		{
			const lcVector3& Position = Vertices[QuadIndices[IndexIdx]];
			Indices[IndexIdx] = AddVertex(MeshDataType, Position, Optimize);
		}
	}

	switch (LineType)
	{
	case 5:
		if (Indices[0] != Indices[1] && Indices[0] != Indices[2] && Indices[0] != Indices[3] && Indices[1] != Indices[2] && Indices[1] != Indices[3] && Indices[2] != Indices[3])
		{
			Section->mIndices.Add(Indices[0]);
			Section->mIndices.Add(Indices[1]);
			Section->mIndices.Add(Indices[2]);
			Section->mIndices.Add(Indices[3]);
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

void lcLibraryMeshData::AddTexturedLine(lcMeshDataType MeshDataType, int LineType, quint32 ColorCode, bool WindingCCW, const lcLibraryTextureMap& Map, const lcVector3* Vertices, bool Optimize)
{
	lcMeshPrimitiveType PrimitiveType = (LineType == 2) ? LC_MESH_TEXTURED_LINES : LC_MESH_TEXTURED_TRIANGLES;
	lcLibraryMeshSection* Section = AddSection(MeshDataType, PrimitiveType, ColorCode, Map.Texture);

	int QuadIndices[4] = { 0, 1, 2, 3 };
	int Indices[4] = { -1, -1, -1, -1 };

	if (LineType == 3 || LineType == 4)
	{
		if (LineType == 4)
			TestQuad(QuadIndices, Vertices);

		lcVector3 Normal = lcNormalize(lcCross(Vertices[1] - Vertices[0], Vertices[2] - Vertices[0]));

		if (!WindingCCW)
			Normal = -Normal;

		for (int IndexIdx = 0; IndexIdx < lcMin(LineType, 4); IndexIdx++)
		{
			const lcVector3& Position = Vertices[QuadIndices[IndexIdx]];
			lcVector2 TexCoord(lcDot3(lcVector3(Position.x, Position.y, Position.z), Map.Params[0]) + Map.Params[0].w,
							   lcDot3(lcVector3(Position.x, Position.y, Position.z), Map.Params[1]) + Map.Params[1].w);
			Indices[IndexIdx] = AddTexturedVertex(MeshDataType, Position, Normal, TexCoord, Optimize);
		}
	}
	else
	{
		for (int IndexIdx = 0; IndexIdx < lcMin(LineType, 4); IndexIdx++)
		{
			const lcVector3& Position = Vertices[QuadIndices[IndexIdx]];
			lcVector2 TexCoord(lcDot3(lcVector3(Position.x, Position.y, Position.z), Map.Params[0]) + Map.Params[0].w,
							   lcDot3(lcVector3(Position.x, Position.y, Position.z), Map.Params[1]) + Map.Params[1].w);
			Indices[IndexIdx] = AddTexturedVertex(MeshDataType, Position, TexCoord, Optimize);
		}
	}

	switch (LineType)
	{
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

void lcLibraryMeshData::AddMeshData(const lcLibraryMeshData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcLibraryTextureMap* TextureMap, lcMeshDataType OverrideDestIndex)
{
	for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
	{
		int DestIndex = OverrideDestIndex == LC_MESHDATA_SHARED ? MeshDataIdx : OverrideDestIndex;
		const lcArray<lcLibraryMeshVertex>& DataVertices = Data.mVertices[MeshDataIdx];
		lcArray<lcLibraryMeshVertex>& Vertices = mVertices[DestIndex];
		lcArray<lcLibraryMeshVertexTextured>& TexturedVertices = mTexturedVertices[DestIndex];

		int VertexCount = DataVertices.GetSize();
		lcArray<quint32> IndexRemap(VertexCount);

		if (!TextureMap)
		{
			Vertices.AllocGrow(VertexCount);

			for (int SrcVertexIdx = 0; SrcVertexIdx < VertexCount; SrcVertexIdx++)
			{
				lcVector3 Position = lcMul31(DataVertices[SrcVertexIdx].Position, Transform);
				int Index;

				if (DataVertices[SrcVertexIdx].NormalWeight == 0.0f)
					Index = AddVertex((lcMeshDataType)DestIndex, Position, true);
				else
				{
					lcVector3 Normal = lcNormalize(lcMul30(DataVertices[SrcVertexIdx].Normal, Transform));
					if (InvertNormals)
						Normal = -Normal;
					Index = AddVertex((lcMeshDataType)DestIndex, Position, Normal, true);
				}

				IndexRemap.Add(Index);
			}
		}
		else
		{
			TexturedVertices.AllocGrow(VertexCount);

			for (int SrcVertexIdx = 0; SrcVertexIdx < VertexCount; SrcVertexIdx++)
			{
				const lcLibraryMeshVertex& SrcVertex = DataVertices[SrcVertexIdx];
				lcVector3 Position = lcMul31(SrcVertex.Position, Transform);
				lcVector2 TexCoord(lcDot3(lcVector3(Position.x, Position.y, Position.z), TextureMap->Params[0]) + TextureMap->Params[0].w,
								   lcDot3(lcVector3(Position.x, Position.y, Position.z), TextureMap->Params[1]) + TextureMap->Params[1].w);
				int Index;

				if (DataVertices[SrcVertexIdx].NormalWeight == 0.0f)
					Index = AddTexturedVertex((lcMeshDataType)DestIndex, Position, TexCoord, true);
				else
				{
					lcVector3 Normal = lcNormalize(lcMul30(DataVertices[SrcVertexIdx].Normal, Transform));
					if (InvertNormals)
						Normal = -Normal;
					Index = AddTexturedVertex((lcMeshDataType)DestIndex, Position, Normal, TexCoord, true);
				}

				IndexRemap.Add(Index);
			}
		}

		const lcArray<lcLibraryMeshVertexTextured>& DataTexturedVertices = Data.mTexturedVertices[MeshDataIdx];
		int TexturedVertexCount = DataTexturedVertices.GetSize();
		lcArray<quint32> TexturedIndexRemap(TexturedVertexCount);

		if (TexturedVertexCount)
		{
			TexturedVertices.AllocGrow(TexturedVertexCount);

			for (int SrcVertexIdx = 0; SrcVertexIdx < TexturedVertexCount; SrcVertexIdx++)
			{
				const lcLibraryMeshVertexTextured& SrcVertex = DataTexturedVertices[SrcVertexIdx];
				lcVector3 Position = lcMul31(SrcVertex.Position, Transform);
				int Index;

				if (DataVertices[SrcVertexIdx].NormalWeight == 0.0f)
					Index = AddTexturedVertex((lcMeshDataType)DestIndex, Position, SrcVertex.TexCoord, true);
				else
				{
					lcVector3 Normal = lcNormalize(lcMul30(DataVertices[SrcVertexIdx].Normal, Transform));
					if (InvertNormals)
						Normal = -Normal;
					Index = AddTexturedVertex((lcMeshDataType)DestIndex, Position, Normal, SrcVertex.TexCoord, true);
				}

				TexturedIndexRemap.Add(Index);
			}
		}

		const lcArray<lcLibraryMeshSection*>& DataSections = Data.mSections[MeshDataIdx];
		lcArray<lcLibraryMeshSection*>& Sections = mSections[DestIndex];

		for (int SrcSectionIdx = 0; SrcSectionIdx < DataSections.GetSize(); SrcSectionIdx++)
		{
			lcLibraryMeshSection* SrcSection = DataSections[SrcSectionIdx];
			lcLibraryMeshSection* DstSection = nullptr;
			quint32 ColorCode = SrcSection->mColor == 16 ? CurrentColorCode : SrcSection->mColor;
			lcTexture* Texture;

			if (SrcSection->mTexture)
				Texture = SrcSection->mTexture;
			else if (TextureMap)
				Texture = TextureMap->Texture;
			else
				Texture = nullptr;

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
				if (!InvertWinding || (SrcSection->mPrimitiveType != LC_MESH_TRIANGLES && SrcSection->mPrimitiveType != LC_MESH_TEXTURED_TRIANGLES))
				{
					for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
						DstSection->mIndices.Add(IndexRemap[SrcSection->mIndices[IndexIdx]]);
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
			else
			{
				if (!InvertWinding || (SrcSection->mPrimitiveType != LC_MESH_TRIANGLES && SrcSection->mPrimitiveType != LC_MESH_TEXTURED_TRIANGLES))
				{
					for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
						DstSection->mIndices.Add(TexturedIndexRemap[SrcSection->mIndices[IndexIdx]]);
				}
				else
				{
					for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx += 3)
					{
						DstSection->mIndices.Add(TexturedIndexRemap[SrcSection->mIndices[IndexIdx + 2]]);
						DstSection->mIndices.Add(TexturedIndexRemap[SrcSection->mIndices[IndexIdx + 1]]);
						DstSection->mIndices.Add(TexturedIndexRemap[SrcSection->mIndices[IndexIdx + 0]]);
					}
				}
			}
		}
	}
}

void lcLibraryMeshData::AddMeshDataNoDuplicateCheck(const lcLibraryMeshData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcLibraryTextureMap* TextureMap, lcMeshDataType OverrideDestIndex)
{
	for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
	{
		int DestIndex = OverrideDestIndex == LC_MESHDATA_SHARED ? MeshDataIdx : OverrideDestIndex;
		const lcArray<lcLibraryMeshVertex>& DataVertices = Data.mVertices[MeshDataIdx];
		lcArray<lcLibraryMeshVertex>& Vertices = mVertices[DestIndex];
		lcArray<lcLibraryMeshVertexTextured>& TexturedVertices = mTexturedVertices[DestIndex];
		quint32 BaseIndex;

		if (!TextureMap)
		{
			BaseIndex = Vertices.GetSize();

			Vertices.SetGrow(lcMin(Vertices.GetSize(), 8 * 1024 * 1024));
			Vertices.AllocGrow(DataVertices.GetSize());

			for (int SrcVertexIdx = 0; SrcVertexIdx < DataVertices.GetSize(); SrcVertexIdx++)
			{
				const lcLibraryMeshVertex& SrcVertex = DataVertices[SrcVertexIdx];
				lcLibraryMeshVertex& DstVertex = Vertices.Add();
				DstVertex.Position = lcMul31(SrcVertex.Position, Transform);
				DstVertex.Normal = lcNormalize(lcMul30(SrcVertex.Normal, Transform));
				if (InvertNormals)
					DstVertex.Normal = -DstVertex.Normal;
				DstVertex.NormalWeight = SrcVertex.NormalWeight;
			}
		}
		else
		{
			BaseIndex = TexturedVertices.GetSize();

			TexturedVertices.AllocGrow(DataVertices.GetSize());

			for (int SrcVertexIdx = 0; SrcVertexIdx < DataVertices.GetSize(); SrcVertexIdx++)
			{
				const lcLibraryMeshVertex& SrcVertex = DataVertices[SrcVertexIdx];
				lcLibraryMeshVertexTextured& DstVertex = TexturedVertices.Add();

				lcVector3 Position = lcMul31(SrcVertex.Position, Transform);
				lcVector2 TexCoord(lcDot3(lcVector3(Position.x, Position.y, Position.z), TextureMap->Params[0]) + TextureMap->Params[0].w,
								   lcDot3(lcVector3(Position.x, Position.y, Position.z), TextureMap->Params[1]) + TextureMap->Params[1].w);

				DstVertex.Position = Position;
				DstVertex.Normal = lcNormalize(lcMul30(SrcVertex.Normal, Transform));
				if (InvertNormals)
					DstVertex.Normal = -DstVertex.Normal;
				DstVertex.NormalWeight = SrcVertex.NormalWeight;
				DstVertex.TexCoord = TexCoord;
			}
		}

		const lcArray<lcLibraryMeshVertexTextured>& DataTexturedVertices = Data.mTexturedVertices[MeshDataIdx];

		int TexturedVertexCount = DataTexturedVertices.GetSize();
		quint32 BaseTexturedIndex = TexturedVertices.GetSize();

		if (TexturedVertexCount)
		{
			TexturedVertices.AllocGrow(TexturedVertexCount);

			for (int SrcVertexIdx = 0; SrcVertexIdx < TexturedVertexCount; SrcVertexIdx++)
			{
				const lcLibraryMeshVertexTextured& SrcVertex = DataTexturedVertices[SrcVertexIdx];
				lcLibraryMeshVertexTextured& DstVertex = TexturedVertices.Add();
				DstVertex.Position = lcMul31(SrcVertex.Position, Transform);
				DstVertex.Normal = SrcVertex.Normal;
				if (InvertNormals)
					DstVertex.Normal = -DstVertex.Normal;
				DstVertex.NormalWeight = SrcVertex.NormalWeight;
				DstVertex.TexCoord = SrcVertex.TexCoord;
			}
		}

		const lcArray<lcLibraryMeshSection*>& DataSections = Data.mSections[MeshDataIdx];
		lcArray<lcLibraryMeshSection*>& Sections = mSections[DestIndex];

		for (int SrcSectionIdx = 0; SrcSectionIdx < DataSections.GetSize(); SrcSectionIdx++)
		{
			lcLibraryMeshSection* SrcSection = DataSections[SrcSectionIdx];
			lcLibraryMeshSection* DstSection = nullptr;
			quint32 ColorCode = SrcSection->mColor == 16 ? CurrentColorCode : SrcSection->mColor;
			lcTexture* Texture;

			if (SrcSection->mTexture)
				Texture = SrcSection->mTexture;
			else if (TextureMap)
				Texture = TextureMap->Texture;
			else
				Texture = nullptr;

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

			DstSection->mIndices.SetGrow(lcMin(DstSection->mIndices.GetSize(), 8 * 1024 * 1024));
			DstSection->mIndices.AllocGrow(SrcSection->mIndices.GetSize());

			if (!SrcSection->mTexture)
			{
				if (!InvertWinding || (SrcSection->mPrimitiveType != LC_MESH_TRIANGLES && SrcSection->mPrimitiveType != LC_MESH_TEXTURED_TRIANGLES))
				{
					for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
						DstSection->mIndices.Add(BaseIndex + SrcSection->mIndices[IndexIdx]);
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
			else
			{
				if (!InvertWinding || (SrcSection->mPrimitiveType != LC_MESH_TRIANGLES && SrcSection->mPrimitiveType != LC_MESH_TEXTURED_TRIANGLES))
				{
					for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx++)
						DstSection->mIndices.Add(BaseTexturedIndex + SrcSection->mIndices[IndexIdx]);
				}
				else
				{
					for (int IndexIdx = 0; IndexIdx < SrcSection->mIndices.GetSize(); IndexIdx += 3)
					{
						DstSection->mIndices.Add(BaseTexturedIndex + SrcSection->mIndices[IndexIdx + 2]);
						DstSection->mIndices.Add(BaseTexturedIndex + SrcSection->mIndices[IndexIdx + 1]);
						DstSection->mIndices.Add(BaseTexturedIndex + SrcSection->mIndices[IndexIdx + 0]);
					}
				}
			}
		}
	}
}

bool lcPiecesLibrary::PieceInCategory(PieceInfo* Info, const char* CategoryKeywords) const
{
	if (Info->IsTemporary())
		return false;

	const char* PieceName;
	if (Info->m_strDescription[0] == '~' || Info->m_strDescription[0] == '_')
		PieceName = Info->m_strDescription + 1;
	else
		PieceName = Info->m_strDescription;

	return lcMatchCategory(PieceName, CategoryKeywords);
}

void lcPiecesLibrary::GetCategoryEntries(int CategoryIndex, bool GroupPieces, lcArray<PieceInfo*>& SinglePieces, lcArray<PieceInfo*>& GroupedPieces)
{
	if (CategoryIndex >= 0 && CategoryIndex < gCategories.GetSize())
		GetCategoryEntries(gCategories[CategoryIndex].Keywords.constData(), GroupPieces, SinglePieces, GroupedPieces);
}

void lcPiecesLibrary::GetCategoryEntries(const char* CategoryKeywords, bool GroupPieces, lcArray<PieceInfo*>& SinglePieces, lcArray<PieceInfo*>& GroupedPieces)
{
	SinglePieces.RemoveAll();
	GroupedPieces.RemoveAll();

	for (const auto PieceIt : mPieces)
	{
		PieceInfo* Info = PieceIt.second;

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
			strcpy(ParentName, Info->mFileName);
			*strchr(ParentName, 'P') = '\0';
			strcat(ParentName, ".dat");

			Parent = FindPiece(ParentName, nullptr, false, false);

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
	strcpy(Name, Parent->mFileName);
	char* Ext = strchr(Name, '.');
	if (Ext)
		*Ext = 0;
	strcat(Name, "P");
	strupr(Name);

	Pieces.RemoveAll();

	for (const auto PieceIt : mPieces)
		if (strncmp(Name, PieceIt.first.c_str(), strlen(Name)) == 0)
			Pieces.Add(PieceIt.second);

	// Sometimes pieces with A and B versions don't follow the same convention (for example, 3040Pxx instead of 3040BPxx).
	if (Pieces.GetSize() == 0)
	{
		strcpy(Name, Parent->mFileName);
		Ext = strchr(Name, '.');
		if (Ext)
			*Ext = 0;
		size_t Len = strlen(Name);
		if (Name[Len-1] < '0' || Name[Len-1] > '9')
			Name[Len-1] = 'P';

		for (const auto PieceIt : mPieces)
			if (strncmp(Name, PieceIt.first.c_str(), strlen(Name)) == 0)
				Pieces.Add(PieceIt.second);
	}
}

void lcPiecesLibrary::GetParts(lcArray<PieceInfo*>& Parts)
{
	Parts.SetSize(0);
	Parts.AllocGrow(mPieces.size());

	for (const auto PartIt : mPieces)
		Parts.Add(PartIt.second);
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

	for (const auto PieceIt : mPieces)
	{
		PieceInfo* Info = PieceIt.second;

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
	lcSynthInit();

	return true;
}
