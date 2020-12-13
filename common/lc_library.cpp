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
#include "lc_profile.h"
#include "lc_meshloader.h"
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

#define LC_LIBRARY_CACHE_VERSION   0x0108
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
	mBuffersDirty = false;
	mHasUnofficial = false;
	mCancelLoading = false;
	mStudLogo = lcGetProfileInt(LC_PROFILE_STUD_LOGO);
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
	for (const auto& PieceIt : mPieces)
		delete PieceIt.second;
	mPieces.clear();

	for (const auto& PrimitiveIt : mPrimitives)
		delete PrimitiveIt.second;
	mPrimitives.clear();

	for (lcTexture* Texture : mTextures)
		delete Texture;
	mTextures.clear();

	mNumOfficialPieces = 0;
	mZipFiles[LC_ZIPFILE_OFFICIAL] = nullptr;
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
	for (lcTexture* Texture : mTextures)
		if (!strcmp(TextureName, Texture->mName))
			return Texture;

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
				mTextures.push_back(Texture);
				return Texture;
			}
		}
	}

	return nullptr;
}

bool lcPiecesLibrary::Load(const QString& LibraryPath, bool ShowProgress)
{
	Unload();

	auto LoadCustomColors = []()
	{
		QString CustomColorsPath = lcGetProfileString(LC_PROFILE_COLOR_CONFIG);

		if (CustomColorsPath.isEmpty())
			return false;

		lcDiskFile ColorFile(CustomColorsPath);
		return ColorFile.Open(QIODevice::ReadOnly) && lcLoadColorFile(ColorFile);
	};

	if (OpenArchive(LibraryPath, LC_ZIPFILE_OFFICIAL))
	{
		lcMemFile ColorFile;

		if (!LoadCustomColors())
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
		mLibraryDir.setPath(LibraryPath);

		if (OpenDirectory(mLibraryDir, ShowProgress))
		{
			if (!LoadCustomColors())
			{
				lcDiskFile ColorFile(mLibraryDir.absoluteFilePath(QLatin1String("ldconfig.ldr")));

				if (!ColorFile.Open(QIODevice::ReadOnly) || !lcLoadColorFile(ColorFile))
				{
					ColorFile.SetFileName(mLibraryDir.absoluteFilePath(QLatin1String("LDConfig.ldr")));

					if (!ColorFile.Open(QIODevice::ReadOnly) || !lcLoadColorFile(ColorFile))
						lcLoadDefaultColors();
				}
			}
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
	std::unique_ptr<lcDiskFile> File(new lcDiskFile(FileName));

	if (!File->Open(QIODevice::ReadOnly))
		return false;

	return OpenArchive(std::move(File), FileName, ZipFileType);
}

bool lcPiecesLibrary::OpenArchive(std::unique_ptr<lcFile> File, const QString& FileName, lcZipFileType ZipFileType)
{
	std::unique_ptr<lcZipFile> ZipFile(new lcZipFile());

	if (!ZipFile->OpenRead(std::move(File)))
		return false;

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
			if (!memcmp(Dst, ".PNG", 4))
			{
				if ((ZipFileType == LC_ZIPFILE_OFFICIAL && !memcmp(Name, "LDRAW/PARTS/TEXTURES/", 21)) ||
					(ZipFileType == LC_ZIPFILE_UNOFFICIAL && !memcmp(Name, "PARTS/TEXTURES/", 15)))
				{
					lcTexture* Texture = new lcTexture();
					mTextures.push_back(Texture);

					*Dst = 0;
					strncpy(Texture->mName, Name + (ZipFileType == LC_ZIPFILE_OFFICIAL ? 21 : 15), sizeof(Texture->mName));
					Texture->mName[sizeof(Texture->mName) - 1] = 0;
				}
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
					mPrimitives[Name] = new lcLibraryPrimitive(QString(), FileInfo.file_name + (Name - NameBuffer), ZipFileType, FileIdx, false, true);
				else
					Primitive->SetZipFile(ZipFileType, FileIdx);
			}
		}
		else if (!memcmp(Name, "P/", 2))
		{
			Name += 2;

			lcLibraryPrimitive* Primitive = FindPrimitive(Name);

			if (!Primitive)
				mPrimitives[Name] = new lcLibraryPrimitive(QString(), FileInfo.file_name + (Name - NameBuffer), ZipFileType, FileIdx, (memcmp(Name, "STU", 3) == 0), false);
			else
				Primitive->SetZipFile(ZipFileType, FileIdx);
		}
	}

	mZipFiles[ZipFileType] = std::move(ZipFile);

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

		for (const auto& PieceIt : mPieces)
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
	constexpr int NumBaseFolders = LC_ARRAY_COUNT(BaseFolders);

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

	for (unsigned int BaseFolderIdx = 0; BaseFolderIdx < LC_ARRAY_COUNT(BaseFolders); BaseFolderIdx++)
	{
		const char* PrimitiveDirectories[] = { "p/", "parts/s/" };
		bool SubFileDirectories[] = { false, false, true };
		QDir BaseDir(LibraryDir.absoluteFilePath(QLatin1String(BaseFolders[BaseFolderIdx])));

		for (int DirectoryIdx = 0; DirectoryIdx < (int)(LC_ARRAY_COUNT(PrimitiveDirectories)); DirectoryIdx++)
		{
			QString ChildPath = BaseDir.absoluteFilePath(QLatin1String(PrimitiveDirectories[DirectoryIdx]));
			QDirIterator DirIterator(ChildPath, QStringList() << QLatin1String("*.dat"), QDir::Files | QDir::Hidden | QDir::Readable, QDirIterator::Subdirectories);

			while (DirIterator.hasNext())
			{
				char Name[LC_PIECE_NAME_LEN];
				QString FileName = DirIterator.next();
				QByteArray FileString = BaseDir.relativeFilePath(FileName).toLatin1();
				const char* Src = strchr(FileString, '/') + 1;
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

				if (Dst - Name <= 4)
					continue;

				Dst -= 4;
				if (memcmp(Dst, ".DAT", 4))
					continue;

				if (mHasUnofficial && IsPrimitive(Name))
					continue;

				if (BaseFolderIdx == 0)
					mHasUnofficial = true;

				const bool SubFile = SubFileDirectories[DirectoryIdx];
				mPrimitives[Name] = new lcLibraryPrimitive(std::move(FileName), strchr(FileString, '/') + 1, LC_NUM_ZIPFILES, 0, !SubFile && (memcmp(Name, "STU", 3) == 0), SubFile);
			}
		}
	}

	for (unsigned int BaseFolderIdx = 0; BaseFolderIdx < LC_ARRAY_COUNT(BaseFolders); BaseFolderIdx++)
	{
		QDir BaseDir(LibraryDir.absoluteFilePath(QLatin1String(BaseFolders[BaseFolderIdx])));
		QDir Dir(BaseDir.absoluteFilePath(QLatin1String("parts/textures/")), QLatin1String("*.png"), QDir::SortFlags(QDir::Name | QDir::IgnoreCase), QDir::Files | QDir::Hidden | QDir::Readable);
		QStringList FileList = Dir.entryList();

		mTextures.reserve(mTextures.size() + FileList.size());

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
			mTextures.push_back(Texture);

			strncpy(Texture->mName, Name, sizeof(Texture->mName));
			Texture->mName[sizeof(Texture->mName) - 1] = 0;
			Texture->mFileName = Dir.absoluteFilePath(FileList[FileIdx]);
		}
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
				const uint64_t CachedFileTime = *(uint64_t*)(Description + strlen(Description) + 1 + 4 + 1);

#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
				quint64 FileTime = FileLists[Info->mFolderType][Info->mFolderIndex].lastModified().toMSecsSinceEpoch();
#else
				quint64 FileTime = FileLists[Info->mFolderType][Info->mFolderIndex].lastModified().toTime_t();
#endif

				if (FileTime == CachedFileTime)
				{
					strcpy(Info->m_strDescription, Description);
					return;
				}
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
	ProgressDialog->setMaximum((int)mPieces.size());
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
		lcMemFile NewIndexFile;

		NewIndexFile.WriteQString(mLibraryDir.absolutePath());

		NewIndexFile.WriteU32((quint32)mPieces.size());

		std::vector<PieceInfo*> SortedPieces;
		SortedPieces.reserve(mPieces.size());
		for (const auto& PieceIt : mPieces)
			SortedPieces.push_back(PieceIt.second);

		auto PieceInfoCompare = [](PieceInfo* Info1, PieceInfo* Info2)
		{
			return strcmp(Info1->mFileName, Info2->mFileName) < 0;
		};

		std::sort(SortedPieces.begin(), SortedPieces.end(), PieceInfoCompare);

		for (const PieceInfo* Info : SortedPieces)
		{
			if (NewIndexFile.WriteBuffer(Info->mFileName, strlen(Info->mFileName) + 1) == 0)
				return;

			if (NewIndexFile.WriteBuffer(Info->m_strDescription, strlen(Info->m_strDescription) + 1) == 0)
				return;

			NewIndexFile.WriteU8(Info->mFolderType);

#if (QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
			quint64 FileTime = FileLists[Info->mFolderType][Info->mFolderIndex].lastModified().toMSecsSinceEpoch();
#else
			quint64 FileTime = FileLists[Info->mFolderType][Info->mFolderIndex].lastModified().toTime_t();
#endif

			NewIndexFile.WriteU64(FileTime);
		}

		WriteDirectoryCacheFile(IndexFileName, NewIndexFile);
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

	constexpr int CHUNK = 16384;
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
				Q_FALLTHROUGH();
			case Z_DATA_ERROR:
				Q_FALLTHROUGH();
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

	constexpr quint32 CacheVersion = LC_LIBRARY_CACHE_VERSION;
	constexpr quint32 CacheFlags = LC_LIBRARY_CACHE_ARCHIVE;

	if (File.write((char*)&CacheVersion, sizeof(CacheVersion)) == -1)
		return false;

	if (File.write((char*)&CacheFlags, sizeof(CacheFlags)) == -1)
		return false;

	if (File.write((char*)&mArchiveCheckSum, sizeof(mArchiveCheckSum)) == -1)
		return false;

	const quint32 UncompressedSize = (quint32)CacheFile.GetLength();

	if (File.write((char*)&UncompressedSize, sizeof(UncompressedSize)) == -1)
		return false;

	constexpr size_t BufferSize = 16384;
	char WriteBuffer[BufferSize];
	z_stream Stream;
	quint32 Crc32 = 0;

	CacheFile.Seek(0, SEEK_SET);

	Stream.zalloc = nullptr;
	Stream.zfree = nullptr;
	Stream.opaque = nullptr;

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

	constexpr quint32 CacheVersion = LC_LIBRARY_CACHE_VERSION;
	if (File.write((char*)&CacheVersion, sizeof(CacheVersion)) == -1)
		return false;

	constexpr quint32 CacheFlags = LC_LIBRARY_CACHE_DIRECTORY;
	if (File.write((char*)&CacheFlags, sizeof(CacheFlags)) == -1)
		return false;

	const quint32 UncompressedSize = (quint32)CacheFile.GetLength();
	if (File.write((char*)&UncompressedSize, sizeof(UncompressedSize)) == -1)
		return false;

	File.write(qCompress(CacheFile.mBuffer, (int)CacheFile.GetLength()));

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

	for (const auto& PieceIt : mPieces)
	{
		PieceInfo* Info = PieceIt.second;
		quint8 Length;

		if (IndexFile.ReadBuffer((char*)&Length, sizeof(Length)) == 0 || Length >= sizeof(Info->m_strDescription))
			return false;

		if (IndexFile.ReadBuffer((char*)Info->m_strDescription, Length) == 0)
			return false;

		Info->m_strDescription[Length] = 0;
	}

	return true;
}

bool lcPiecesLibrary::SaveArchiveCacheIndex(const QString& FileName)
{
	lcMemFile IndexFile;

	const quint32 NumFiles = (quint32)mPieces.size();

	if (IndexFile.WriteBuffer((char*)&NumFiles, sizeof(NumFiles)) == 0)
		return false;

	for (const auto& PieceIt : mPieces)
	{
		const PieceInfo* Info = PieceIt.second;
		const quint8 Length = (quint8)strlen(Info->m_strDescription);

		if (IndexFile.WriteBuffer((char*)&Length, sizeof(Length)) == 0)
			return false;

		if (IndexFile.WriteBuffer((char*)Info->m_strDescription, Length) == 0)
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

	qint32 Flags;
	if (MeshData.ReadBuffer((char*)&Flags, sizeof(Flags)) == 0)
		return false;

	if (Flags != mStudLogo)
		return false;

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

	const qint32 Flags = mStudLogo;
	if (MeshData.WriteBuffer((char*)&Flags, sizeof(Flags)) == 0)
		return false;

	if (!Info->GetMesh()->FileSave(MeshData))
		return false;

	QString FileName = QFileInfo(QDir(mCachePath), QString::fromLatin1(Info->mFileName)).absoluteFilePath();

	return WriteArchiveCacheFile(FileName, MeshData);
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

			mLoadFutures.append(QtConcurrent::run([this]() { LoadQueuedPiece(); }));
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

bool lcPiecesLibrary::LoadPieceData(PieceInfo* Info)
{
	lcLibraryMeshData MeshData;
	lcMeshLoader MeshLoader(MeshData, true, nullptr, false);

	bool Loaded = false;
	bool SaveCache = false;

	if (Info->mZipFileType != LC_NUM_ZIPFILES && mZipFiles[Info->mZipFileType])
	{
		if (LoadCachePiece(Info))
			return true;

		lcMemFile PieceFile;

		if (mZipFiles[Info->mZipFileType]->ExtractFile(Info->mZipFileIndex, PieceFile))
			Loaded = MeshLoader.LoadMesh(PieceFile, LC_MESHDATA_SHARED);

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
				Loaded = MeshLoader.LoadMesh(PieceFile, LC_MESHDATA_SHARED);
		}

		if (!Loaded)
		{
			sprintf(FileName, "parts/%s", Info->mFileName);
			PieceFile.SetFileName(mLibraryDir.absoluteFilePath(QLatin1String(FileName)));
			if (PieceFile.Open(QIODevice::ReadOnly))
				Loaded = MeshLoader.LoadMesh(PieceFile, LC_MESHDATA_SHARED);
		}
	}

	if (!Loaded || mCancelLoading)
		return false;

	if (Info) 
		Info->SetMesh(MeshData.CreateMesh());

	if (SaveCache)
		SaveCachePiece(Info);

	return true;
}

void lcPiecesLibrary::GetPrimitiveFile(lcLibraryPrimitive* Primitive, std::function<void(lcFile& File)> Callback)
{
	if (mZipFiles[LC_ZIPFILE_OFFICIAL])
	{
		lcMemFile IncludeFile;

		if (mZipFiles[Primitive->mZipFileType]->ExtractFile(Primitive->mZipFileIndex, IncludeFile))
			Callback(IncludeFile);
	}
	else
	{
		lcDiskFile IncludeFile(Primitive->mFileName);

		if (IncludeFile.Open(QIODevice::ReadOnly))
			Callback(IncludeFile);
	}
}

void lcPiecesLibrary::GetPieceFile(const char* PieceName, std::function<void(lcFile& File)> Callback)
{
	const auto PieceIt = mPieces.find(PieceName);

	if (PieceIt != mPieces.end())
	{
		PieceInfo* Info = PieceIt->second;

		if (mZipFiles[LC_ZIPFILE_OFFICIAL] && Info->mZipFileType != LC_NUM_ZIPFILES)
		{
			lcMemFile IncludeFile;

			if (mZipFiles[Info->mZipFileType]->ExtractFile(Info->mZipFileIndex, IncludeFile))
				Callback(IncludeFile);
		}
		else
		{
			lcDiskFile IncludeFile;
			char FileName[LC_MAXPATH];
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
				Callback(IncludeFile);
		}
	}
	else
	{
		bool Found = false;

		if (mZipFiles[LC_ZIPFILE_OFFICIAL])
		{
			lcMemFile IncludeFile;

			auto LoadIncludeFile = [&IncludeFile, PieceName, this](const char* Folder, int ZipFileIndex)
			{
				char IncludeFileName[LC_MAXPATH];
				sprintf(IncludeFileName, Folder, PieceName);
				return mZipFiles[ZipFileIndex]->ExtractFile(IncludeFileName, IncludeFile);
			};

			if (mHasUnofficial)
			{
				Found = LoadIncludeFile("parts/%s", LC_ZIPFILE_UNOFFICIAL);

				if (!Found)
					Found = LoadIncludeFile("p/%s", LC_ZIPFILE_UNOFFICIAL);
			}

			if (!Found)
			{
				Found = LoadIncludeFile("ldraw/parts/%s", LC_ZIPFILE_OFFICIAL);

				if (!Found)
					Found = LoadIncludeFile("ldraw/p/%s", LC_ZIPFILE_OFFICIAL);
			}

			if (Found)
				Callback(IncludeFile);
		}
		else
		{
			lcDiskFile IncludeFile;

			auto LoadIncludeFile = [&IncludeFile, PieceName, this](const char* Folder)
			{
				char IncludeFileName[LC_MAXPATH];
				sprintf(IncludeFileName, Folder, PieceName);
				IncludeFile.SetFileName(mLibraryDir.absoluteFilePath(QLatin1String(IncludeFileName)));
				if (IncludeFile.Open(QIODevice::ReadOnly))
					return true;

#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
				// todo: instead of using strlwr, search the parts/primitive lists and get the file name from there
				strlwr(IncludeFileName);
				IncludeFile.SetFileName(mLibraryDir.absoluteFilePath(QLatin1String(IncludeFileName)));
				return IncludeFile.Open(QIODevice::ReadOnly);
#else
				return false;
#endif
			};

			if (mHasUnofficial)
			{
				Found = LoadIncludeFile("unofficial/parts/%s");

				if (!Found)
					Found = LoadIncludeFile("unofficial/p/%s");
			}

			if (!Found)
			{
				Found = LoadIncludeFile("parts/%s");

				if (!Found)
					Found = LoadIncludeFile("p/%s");
			}

			if (Found)
				Callback(IncludeFile);
		}
	}
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
	std::vector<lcMesh*> Meshes;

	for (const auto& PieceIt : mPieces)
	{
		const PieceInfo* const Info = PieceIt.second;
		lcMesh* Mesh = Info->IsPlaceholder() ? gPlaceholderMesh : Info->GetMesh();

		if (!Mesh)
			continue;

		if (Mesh->mVertexDataSize > 16 * 1024 * 1024 || Mesh->mIndexDataSize > 16 * 1024 * 1024)
			continue;

		VertexDataSize += Mesh->mVertexDataSize;
		IndexDataSize += Mesh->mIndexDataSize;

		Meshes.push_back(Mesh);
	}

	Context->DestroyVertexBuffer(mVertexBuffer);
	Context->DestroyIndexBuffer(mIndexBuffer);

	if (!VertexDataSize || !IndexDataSize)
		return;

	void* VertexData = malloc(VertexDataSize);
	void* IndexData = malloc(IndexDataSize);

	VertexDataSize = 0;
	IndexDataSize = 0;

	for (lcMesh* Mesh : Meshes)
	{
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

	for (const auto& PieceIt : mPieces)
	{
		PieceInfo* Info = PieceIt.second;
		if (Info->GetRefCount() == 0 && Info->mState != LC_PIECEINFO_UNLOADED)
			ReleasePieceInfo(Info);
	}
}

bool lcPiecesLibrary::LoadTexture(lcTexture* Texture)
{
	char FileName[2*LC_MAXPATH];

	if (mZipFiles[LC_ZIPFILE_OFFICIAL])
	{
		lcMemFile TextureFile;

		sprintf(FileName, "parts/textures/%s.png", Texture->mName);

		if (!mZipFiles[LC_ZIPFILE_UNOFFICIAL] || !mZipFiles[LC_ZIPFILE_UNOFFICIAL]->ExtractFile(FileName, TextureFile))
		{
			sprintf(FileName, "ldraw/parts/textures/%s.png", Texture->mName);

			if (!mZipFiles[LC_ZIPFILE_OFFICIAL]->ExtractFile(FileName, TextureFile))
				return false;
		}

		return Texture->Load(TextureFile);
	}
	else
		return Texture->Load(Texture->mFileName);
}

void lcPiecesLibrary::ReleaseTexture(lcTexture* Texture)
{
	QMutexLocker LoadLock(&mLoadMutex);

	if (Texture->Release() == 0 && Texture->IsTemporary())
	{
		std::vector<lcTexture*>::iterator TextureIt = std::find(mTextures.begin(), mTextures.end(), Texture);
		if (TextureIt != mTextures.end())
			mTextures.erase(TextureIt);
		delete Texture;
	}
}

void lcPiecesLibrary::QueueTextureUpload(lcTexture* Texture)
{
	QMutexLocker Lock(&mTextureMutex);
	mTextureUploads.push_back(Texture);
}

void lcPiecesLibrary::UploadTextures(lcContext* Context)
{
	QMutexLocker Lock(&mTextureMutex);

	for (lcTexture* Texture : mTextureUploads)
		Texture->Upload(Context);

	mTextureUploads.clear();
}

bool lcPiecesLibrary::SupportsStudLogo() const
{
	return mZipFiles[LC_ZIPFILE_UNOFFICIAL] || mHasUnofficial;
}

void lcPiecesLibrary::SetStudLogo(int StudLogo, bool Reload)
{
	mStudLogo = StudLogo;

	mLoadMutex.lock();

	for (const auto& PrimitiveIt : mPrimitives)
	{
		lcLibraryPrimitive* Primitive = PrimitiveIt.second;
		if (Primitive->mMeshData.mHasLogoStud)
			Primitive->Unload();
	}

	mLoadMutex.unlock();

	if (Reload)
	{
		mLoadMutex.lock();

		for (const auto& PieceIt : mPieces)
		{
			PieceInfo* Info = PieceIt.second;

			if (Info->mState == LC_PIECEINFO_LOADED && Info->GetMesh() && Info->GetMesh()->mFlags & lcMeshFlag::HasLogoStud)
			{
				Info->Unload();
				mLoadQueue.append(Info);
				mLoadFutures.append(QtConcurrent::run([this]() { LoadQueuedPiece(); }));
			}
		}

		mLoadMutex.unlock();

		WaitForLoadQueue();
	}
}

bool lcPiecesLibrary::GetStudLogoFile(lcMemFile& PrimFile, int StudLogo, bool OpenStud)
{
	if (!StudLogo || (!mZipFiles[LC_ZIPFILE_UNOFFICIAL] && !mHasUnofficial))
		return false;

	QString Logo        = QString("%1").arg(StudLogo);
	QString LogoRefLine = QString("1 16 0 0 0 1 0 0 0 1 0 0 0 1 ");
	LogoRefLine        += (OpenStud ? QString("stud2-logo%1.dat").arg(StudLogo > 1 ? Logo : ""):
									  QString("stud-logo%1.dat").arg(StudLogo > 1 ? Logo : ""));

	const QLatin1String LineEnding("\r\n");
	QByteArray FileData;
	QTextStream TextStream(&FileData);

	TextStream << (OpenStud ? "0 Stud Open" : "0 Stud") << LineEnding;
	TextStream << (OpenStud ? "0 Name: stud2.dat" : "0 Name: stud.dat") << LineEnding;
	TextStream << "0 Author: James Jessiman" << LineEnding;
	TextStream << "0 !LDRAW_ORG Primitive" << LineEnding;
	TextStream << "0 BFC CERTIFY CCW" << LineEnding;
	TextStream << LogoRefLine << LineEnding;

	PrimFile.WriteBuffer(FileData.constData(), size_t(FileData.size()));
	PrimFile.Seek(0, SEEK_SET);

	return true;
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

	lcMeshLoader MeshLoader(Primitive->mMeshData, true, nullptr, false);

	bool SetStudLogo = false;

	if (mZipFiles[LC_ZIPFILE_OFFICIAL])
	{
		lcLibraryPrimitive* LowPrimitive = nullptr;

		lcMemFile PrimFile;

		if (Primitive->mStud)
		{
			const bool OpenStud = !strcmp(Primitive->mName,"stud2.dat");
			if (OpenStud || !strcmp(Primitive->mName,"stud.dat"))
			{
				Primitive->mMeshData.mHasLogoStud = true;

				if (mStudLogo)
					SetStudLogo = GetStudLogoFile(PrimFile, mStudLogo, OpenStud);
			}

			if (!SetStudLogo && strncmp(Primitive->mName, "8/", 2)) // todo: this is currently the only place that uses mName so use mFileName instead. this should also be done for the loose file libraries.
			{
				char Name[LC_PIECE_NAME_LEN];
				strcpy(Name, "8/");
				strcat(Name, Primitive->mName);
				strupr(Name);

				LowPrimitive = FindPrimitive(Name);
			}
		}

		if (!SetStudLogo && !mZipFiles[Primitive->mZipFileType]->ExtractFile(Primitive->mZipFileIndex, PrimFile))
			return false;

		if (!LowPrimitive)
		{
			if (!MeshLoader.LoadMesh(PrimFile, LC_MESHDATA_SHARED))
				return false;
		}
		else
		{
			if (!MeshLoader.LoadMesh(PrimFile, LC_MESHDATA_HIGH))
				return false;

			if (!mZipFiles[LowPrimitive->mZipFileType]->ExtractFile(LowPrimitive->mZipFileIndex, PrimFile))
				return false;

			if (!MeshLoader.LoadMesh(PrimFile, LC_MESHDATA_LOW))
				return false;
		}
	}
	else
	{
		if (Primitive->mStud)
		{
			const bool OpenStud = !strcmp(Primitive->mName,"stud2.dat");
			if (OpenStud || !strcmp(Primitive->mName,"stud.dat"))
			{
				Primitive->mMeshData.mHasLogoStud = true;

				if (mStudLogo)
				{
					lcMemFile PrimFile;

					if (GetStudLogoFile(PrimFile, mStudLogo, OpenStud))
						SetStudLogo = MeshLoader.LoadMesh(PrimFile, LC_MESHDATA_SHARED);
				}
			}
		}

		if (!SetStudLogo)
		{
			lcDiskFile PrimFile(Primitive->mFileName);

			if (!PrimFile.Open(QIODevice::ReadOnly) || !MeshLoader.LoadMesh(PrimFile, LC_MESHDATA_SHARED)) // todo: LOD like the zip files
				return false;
		}
	}

	mLoadMutex.lock();
	Primitive->mState = lcPrimitiveState::LOADED;
	mLoadMutex.unlock();

	return true;
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
	if (CategoryIndex >= 0 && CategoryIndex < static_cast<int>(gCategories.size()))
		GetCategoryEntries(gCategories[CategoryIndex].Keywords.constData(), GroupPieces, SinglePieces, GroupedPieces);
}

void lcPiecesLibrary::GetCategoryEntries(const char* CategoryKeywords, bool GroupPieces, lcArray<PieceInfo*>& SinglePieces, lcArray<PieceInfo*>& GroupedPieces)
{
	SinglePieces.RemoveAll();
	GroupedPieces.RemoveAll();

	for (const auto& PieceIt : mPieces)
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
			const int Index = GroupedPieces.FindIndex(Info);

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

	for (const auto& PieceIt : mPieces)
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

		for (const auto& PieceIt : mPieces)
			if (strncmp(Name, PieceIt.first.c_str(), strlen(Name)) == 0)
				Pieces.Add(PieceIt.second);
	}
}

void lcPiecesLibrary::GetParts(lcArray<PieceInfo*>& Parts) const
{
	Parts.SetSize(0);
	Parts.AllocGrow(mPieces.size());

	for (const auto& PartIt : mPieces)
		Parts.Add(PartIt.second);
}

std::vector<PieceInfo*> lcPiecesLibrary::GetPartsFromSet(const std::vector<std::string>& PartIds) const
{
	std::vector<PieceInfo*> Parts;
	Parts.reserve(PartIds.size());

	for (const std::string& PartId : PartIds)
	{
		std::map<std::string, PieceInfo*>::const_iterator PartIt = mPieces.find(PartId);

		if (PartIt != mPieces.end())
			Parts.push_back(PartIt->second);
	}

	return Parts;
}

std::string lcPiecesLibrary::GetPartId(const PieceInfo* Info) const
{
	std::map<std::string, PieceInfo*>::const_iterator PartIt = std::find_if(mPieces.begin(), mPieces.end(), [Info](const std::pair<std::string, PieceInfo*>& PartIt)
	{
		return PartIt.second == Info;
	});

	if (PartIt != mPieces.end())
		return PartIt->first;
	else
		return std::string();
}

bool lcPiecesLibrary::LoadBuiltinPieces()
{
	QResource Resource(":/resources/library.zip");

	if (!Resource.isValid())
		return false;

	std::unique_ptr<lcMemFile> File(new lcMemFile());
	File->WriteBuffer(Resource.data(), Resource.size());

	if (!OpenArchive(std::move(File), "builtin", LC_ZIPFILE_OFFICIAL))
		return false;

	lcMemFile PieceFile;

	for (const auto& PieceIt : mPieces)
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
