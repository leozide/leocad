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
#include <QtConcurrent>

#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif

#define LC_LIBRARY_CACHE_VERSION   0x0109
#define LC_LIBRARY_CACHE_ARCHIVE   0x0001
#define LC_LIBRARY_CACHE_DIRECTORY 0x0002

lcPiecesLibrary::lcPiecesLibrary()
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	: mLoadMutex(QMutex::Recursive)
#endif
{
	QStringList cachePathList = QStandardPaths::standardLocations(QStandardPaths::CacheLocation);
	mCachePath = cachePathList.first();

	QDir Dir;
	Dir.mkpath(mCachePath);

	mNumOfficialPieces = 0;
	mBuffersDirty = false;
	mHasUnofficial = false;
	mCancelLoading = false;
	mStudStyle = static_cast<lcStudStyle>(lcGetProfileInt(LC_PROFILE_STUD_STYLE));
}

lcPiecesLibrary::~lcPiecesLibrary()
{
	mLoadMutex.lock();
	mLoadQueue.clear();
	mLoadMutex.unlock();
	mCancelLoading = true;
	WaitForLoadQueue();
	Unload();
	ReleaseBuffers();
}

void lcPiecesLibrary::Unload()
{
	for (const auto& PieceIt : mPieces)
		delete PieceIt.second;
	mPieces.clear();

	mSources.clear();

	for (lcTexture* Texture : mTextures)
		delete Texture;
	mTextures.clear();

	mNumOfficialPieces = 0;

	for (std::unique_ptr<lcZipFile>& ZipFile : mZipFiles)
		ZipFile.reset();
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

		if ((!CurrentProject || !Info->IsModel() || CurrentProject->GetModels().FindIndex(Info->GetModel()) != -1) && (!ProjectPath.isEmpty() || !Info->IsProject() || Info->IsProjectPiece()))
			return Info;
	}

	if (!ProjectPath.isEmpty())
	{
		QFileInfo ProjectFile = QFileInfo(ProjectPath + QDir::separator() + PieceName);

		if (ProjectFile.isFile())
		{
			Project* NewProject = new Project();

			if (NewProject->Load(ProjectFile.absoluteFilePath(), false))
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

	if (OpenArchive(LibraryPath, lcZipFileType::Official))
	{
		LoadColors();

		mLibraryDir = QFileInfo(LibraryPath).absoluteDir();
		QString UnofficialFileName = mLibraryDir.absoluteFilePath(QLatin1String("ldrawunf.zip"));

		if (!OpenArchive(UnofficialFileName, lcZipFileType::Unofficial))
			UnofficialFileName.clear();

		ReadArchiveDescriptions(LibraryPath, UnofficialFileName);
	}
	else
	{
		mLibraryDir.setPath(LibraryPath);

		if (OpenDirectory(mLibraryDir, ShowProgress))
			LoadColors();
		else
			return false;
	}

	UpdateStudStyleSource();
	lcLoadDefaultCategories();
	lcSynthInit();

	return true;
}

void lcPiecesLibrary::LoadColors()
{
	QString CustomColorsPath = lcGetProfileString(LC_PROFILE_COLOR_CONFIG);

	if (!CustomColorsPath.isEmpty())
	{
		lcDiskFile ColorFile(CustomColorsPath);

		if (ColorFile.Open(QIODevice::ReadOnly) && lcLoadColorFile(ColorFile, mStudStyle))
		{
			emit ColorsLoaded();
			return;
		}
	}

	if (mZipFiles[static_cast<int>(lcZipFileType::Official)])
	{
		lcMemFile ColorFile;

		if (!mZipFiles[static_cast<int>(lcZipFileType::Official)]->ExtractFile("ldraw/ldconfig.ldr", ColorFile) || !lcLoadColorFile(ColorFile, mStudStyle))
			lcLoadDefaultColors(mStudStyle);
	}
	else
	{
		lcDiskFile ColorFile(mLibraryDir.absoluteFilePath(QLatin1String("ldconfig.ldr")));

		if (!ColorFile.Open(QIODevice::ReadOnly) || !lcLoadColorFile(ColorFile, mStudStyle))
		{
			ColorFile.SetFileName(mLibraryDir.absoluteFilePath(QLatin1String("LDConfig.ldr")));

			if (!ColorFile.Open(QIODevice::ReadOnly) || !lcLoadColorFile(ColorFile, mStudStyle))
				lcLoadDefaultColors(mStudStyle);
		}
	}

	emit ColorsLoaded();
}

bool lcPiecesLibrary::IsStudPrimitive(const char* FileName)
{
	return memcmp(FileName, "STU", 3) == 0;
}

bool lcPiecesLibrary::IsStudStylePrimitive(const char* FileName)
{
	constexpr std::array<const char*, 15> StudStylePrimitives =
	{
		"2-4STUD4.DAT", "STUD.DAT", "STUD2.DAT", "STUD2A.DAT", "STUD3.DAT", "STUD4.DAT", "STUD4A.DAT", "STUD4H.DAT",
		"8/STUD.DAT", "8/STUD2.DAT", "8/STUD2A.DAT", "8/STUD3.DAT", "8/STUD4.DAT", "8/STUD4A.DAT", "8/STUD4H.DAT"
	};

	for (const char* StudStylePrimitive : StudStylePrimitives)
		if (!strcmp(StudStylePrimitive, FileName))
			return true;

	return false;
}

void lcPiecesLibrary::UpdateStudStyleSource()
{
	if (!mSources.empty() && mSources.front()->Type == lcLibrarySourceType::StudStyle)
		mSources.erase(mSources.begin());

	mZipFiles[static_cast<int>(lcZipFileType::StudStyle)].reset();

	if (mStudStyle == lcStudStyle::Plain)
		return;

	const QLatin1String FileNames[] =
	{
		QLatin1String(""),                                // Plain
		QLatin1String(":/resources/studlogo1.zip"),       // ThinLinesLogo
		QLatin1String(":/resources/studlogo2.zip"),       // OutlineLogo
		QLatin1String(":/resources/studlogo3.zip"),       // SharpTopLogo
		QLatin1String(":/resources/studlogo4.zip"),       // RoundedTopLogo
		QLatin1String(":/resources/studlogo5.zip"),       // FlattenedLogo
		QLatin1String(":/resources/studslegostyle1.zip"), // HighContrast
		QLatin1String(":/resources/studslegostyle2.zip")  // HighContrastLogo
	};

	LC_ARRAY_SIZE_CHECK(FileNames, lcStudStyle::Count);

	std::unique_ptr<lcDiskFile> StudStyleFile(new lcDiskFile(FileNames[static_cast<int>(mStudStyle)]));

	if (StudStyleFile->Open(QIODevice::ReadOnly))
		OpenArchive(std::move(StudStyleFile), lcZipFileType::StudStyle);
}

bool lcPiecesLibrary::OpenArchive(const QString& FileName, lcZipFileType ZipFileType)
{
	std::unique_ptr<lcDiskFile> File(new lcDiskFile(FileName));

	if (!File->Open(QIODevice::ReadOnly))
		return false;

	return OpenArchive(std::move(File), ZipFileType);
}

bool lcPiecesLibrary::OpenArchive(std::unique_ptr<lcFile> File, lcZipFileType ZipFileType)
{
	std::unique_ptr<lcZipFile> ZipFile(new lcZipFile());

	if (!ZipFile->OpenRead(std::move(File)))
		return false;

	std::unique_ptr<lcLibrarySource> Source(new lcLibrarySource);
	Source->Type = ZipFileType != lcZipFileType::StudStyle ? lcLibrarySourceType::Library : lcLibrarySourceType::StudStyle;

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
				if ((ZipFileType == lcZipFileType::Official && !memcmp(Name, "LDRAW/PARTS/TEXTURES/", 21)) ||
					(ZipFileType == lcZipFileType::Unofficial && !memcmp(Name, "PARTS/TEXTURES/", 15)))
				{
					lcTexture* Texture = new lcTexture();
					mTextures.push_back(Texture);

					*Dst = 0;
					strncpy(Texture->mName, Name + (ZipFileType == lcZipFileType::Official ? 21 : 15), sizeof(Texture->mName)-1);
					Texture->mName[sizeof(Texture->mName) - 1] = 0;
				}
			}

			continue;
		}

		if (ZipFileType == lcZipFileType::Official)
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

					strncpy(Info->mFileName, FileInfo.file_name + (Name - NameBuffer), sizeof(Info->mFileName)-1);
					Info->mFileName[sizeof(Info->mFileName) - 1] = 0;

					mPieces[Name] = Info;
				}

				Info->SetZipFile(ZipFileType, FileIdx);
			}
			else
				Source->Primitives[Name] = new lcLibraryPrimitive(QString(), FileInfo.file_name + (Name - NameBuffer), ZipFileType, FileIdx, false, false, true);
		}
		else if (!memcmp(Name, "P/", 2))
		{
			Name += 2;

			Source->Primitives[Name] = new lcLibraryPrimitive(QString(), FileInfo.file_name + (Name - NameBuffer), ZipFileType, FileIdx, IsStudPrimitive(Name), IsStudStylePrimitive(Name), false);
		}
	}

	mZipFiles[static_cast<int>(ZipFileType)] = std::move(ZipFile);

	if (ZipFileType != lcZipFileType::StudStyle)
		mSources.emplace_back(std::move(Source));
	else
		mSources.insert(mSources.begin(), std::move(Source));

	return true;
}

void lcPiecesLibrary::ReadArchiveDescriptions(const QString& OfficialFileName, const QString& UnofficialFileName)
{
	QFileInfo OfficialInfo(OfficialFileName);
	QFileInfo UnofficialInfo(UnofficialFileName);

	mArchiveCheckSum[0] = OfficialInfo.size();
	mArchiveCheckSum[1] = OfficialInfo.lastModified().toMSecsSinceEpoch();

	if (!UnofficialFileName.isEmpty())
	{
		mArchiveCheckSum[2] = UnofficialInfo.size();
		mArchiveCheckSum[3] = UnofficialInfo.lastModified().toMSecsSinceEpoch();
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

			mZipFiles[static_cast<int>(Info->mZipFileType)]->ExtractFile(Info->mZipFileIndex, PieceFile, 256);
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
	const QLatin1String BaseFolders[] = { QLatin1String(""), QLatin1String("unofficial/") };
	constexpr int NumBaseFolders = LC_ARRAY_COUNT(BaseFolders);

	QFileInfoList FileLists[NumBaseFolders];

	for (unsigned int BaseFolderIdx = 0; BaseFolderIdx < NumBaseFolders; BaseFolderIdx++)
	{
		QString ParstPath = QDir(LibraryDir.absoluteFilePath(BaseFolders[BaseFolderIdx])).absoluteFilePath(QLatin1String("parts/"));
		QDir Dir = QDir(ParstPath, QLatin1String("*.dat"), QDir::SortFlags(QDir::Name | QDir::IgnoreCase), QDir::Files | QDir::Hidden | QDir::Readable);
		FileLists[BaseFolderIdx] = Dir.entryInfoList();
	}

	if (FileLists[static_cast<int>(lcLibraryFolderType::Official)].isEmpty())
		return false;

	mHasUnofficial = !FileLists[static_cast<int>(lcLibraryFolderType::Unofficial)].isEmpty();
	ReadDirectoryDescriptions(FileLists, ShowProgress);

	for (unsigned int BaseFolderIdx = 0; BaseFolderIdx < LC_ARRAY_COUNT(BaseFolders); BaseFolderIdx++)
	{
		std::unique_ptr<lcLibrarySource> Source(new lcLibrarySource);
		Source->Type = lcLibrarySourceType::Library;

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

				if (BaseFolderIdx > 0 && IsPrimitive(Name))
					continue;

				if (BaseFolderIdx == static_cast<int>(lcLibraryFolderType::Unofficial))
					mHasUnofficial = true;

				const bool SubFile = SubFileDirectories[DirectoryIdx];
				Source->Primitives[Name] = new lcLibraryPrimitive(std::move(FileName), strchr(FileString, '/') + 1, lcZipFileType::Count, 0, !SubFile && IsStudPrimitive(Name), IsStudStylePrimitive(Name), SubFile);
			}
		}
		
		mSources.emplace_back(std::move(Source));
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

void lcPiecesLibrary::ReadDirectoryDescriptions(const QFileInfoList (&FileLists)[static_cast<int>(lcLibraryFolderType::Count)], bool ShowProgress)
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

	for (int FolderIdx = 0; FolderIdx < static_cast<int>(lcLibraryFolderType::Count); FolderIdx++)
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

			if (FolderIdx > 0 && mPieces.find(Name) != mPieces.end())
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

				quint64 FileTime = FileLists[Info->mFolderType][Info->mFolderIndex].lastModified().toMSecsSinceEpoch();

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
		ProgressDialog->setValue(FilesLoaded);
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}

	ProgressDialog->setValue(FilesLoaded);
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

			NewIndexFile.WriteU8(static_cast<quint8>(Info->mFolderType));

			quint64 FileTime = FileLists[Info->mFolderType][Info->mFolderIndex].lastModified().toMSecsSinceEpoch();

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

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	constexpr qsizetype CHUNK = 16384;
#else
	constexpr int CHUNK = 16384;
#endif
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

	if (Flags != static_cast<qint32>(mStudStyle))
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

	const qint32 Flags = static_cast<qint32>(mStudStyle);
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
			if (Info->mState == lcPieceInfoState::Unloaded)
			{
				Info->Load();
				emit PartLoaded(Info);
			}
			else
			{
				LoadLock.unlock();

				while (Info->mState != lcPieceInfoState::Loaded)
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

		if (Info->mState == lcPieceInfoState::Unloaded && Info->GetRefCount() > 0)
		{
			Info->mState = lcPieceInfoState::Loading;
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

	if (Info->mZipFileType != lcZipFileType::Count && mZipFiles[static_cast<int>(Info->mZipFileType)])
	{
		if (LoadCachePiece(Info))
			return true;

		lcMemFile PieceFile;

		if (mZipFiles[static_cast<int>(Info->mZipFileType)]->ExtractFile(Info->mZipFileIndex, PieceFile))
			Loaded = MeshLoader.LoadMesh(PieceFile, LC_MESHDATA_SHARED);

		SaveCache = Loaded && (Info->mZipFileType == lcZipFileType::Official);
	}
	else
	{
		char FileName[LC_MAXPATH];
		lcDiskFile PieceFile;

		sprintf(FileName, "parts/%s", Info->mFileName);
		PieceFile.SetFileName(mLibraryDir.absoluteFilePath(QLatin1String(FileName)));
		if (PieceFile.Open(QIODevice::ReadOnly))
			Loaded = MeshLoader.LoadMesh(PieceFile, LC_MESHDATA_SHARED);

		if (mHasUnofficial && !Loaded)
		{
			sprintf(FileName, "unofficial/parts/%s", Info->mFileName);
			PieceFile.SetFileName(mLibraryDir.absoluteFilePath(QLatin1String(FileName)));
			if (PieceFile.Open(QIODevice::ReadOnly))
				Loaded = MeshLoader.LoadMesh(PieceFile, LC_MESHDATA_SHARED);
		}
	}

	if (mCancelLoading)
		return false;

	if (Info)
	{
		if (Loaded)
			Info->SetMesh(MeshData.CreateMesh());
		else
		{
			lcMesh* Mesh = new lcMesh;
			Mesh->CreateBox();
			Info->SetMesh(Mesh);
		}
	}

	if (SaveCache)
		SaveCachePiece(Info);

	return Loaded;
}

void lcPiecesLibrary::GetPrimitiveFile(lcLibraryPrimitive* Primitive, std::function<void(lcFile& File)> Callback)
{
	if (mZipFiles[static_cast<int>(lcZipFileType::Official)])
	{
		lcMemFile IncludeFile;

		if (mZipFiles[static_cast<int>(Primitive->mZipFileType)]->ExtractFile(Primitive->mZipFileIndex, IncludeFile))
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

		if (mZipFiles[static_cast<int>(lcZipFileType::Official)] && Info->mZipFileType != lcZipFileType::Count)
		{
			lcMemFile IncludeFile;

			if (mZipFiles[static_cast<int>(Info->mZipFileType)]->ExtractFile(Info->mZipFileIndex, IncludeFile))
				Callback(IncludeFile);
		}
		else
		{
			lcDiskFile IncludeFile;
			char FileName[LC_MAXPATH];
			bool Found = false;

			sprintf(FileName, "parts/%s", Info->mFileName);
			IncludeFile.SetFileName(mLibraryDir.absoluteFilePath(QLatin1String(FileName)));
			Found = IncludeFile.Open(QIODevice::ReadOnly);

			if (mHasUnofficial && !Found)
			{
				sprintf(FileName, "unofficial/parts/%s", Info->mFileName);
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

		if (mZipFiles[static_cast<int>(lcZipFileType::Official)])
		{
			lcMemFile IncludeFile;

			auto LoadIncludeFile = [&IncludeFile, PieceName, this](const char* Folder, lcZipFileType ZipFileType)
			{
				char IncludeFileName[LC_MAXPATH];
				sprintf(IncludeFileName, Folder, PieceName);
				return mZipFiles[static_cast<int>(ZipFileType)]->ExtractFile(IncludeFileName, IncludeFile);
			};

			Found = LoadIncludeFile("ldraw/parts/%s", lcZipFileType::Official);

			if (!Found)
				Found = LoadIncludeFile("ldraw/p/%s", lcZipFileType::Official);

			if (mZipFiles[static_cast<int>(lcZipFileType::Unofficial)] && !Found)
			{
				Found = LoadIncludeFile("parts/%s", lcZipFileType::Unofficial);

				if (!Found)
					Found = LoadIncludeFile("p/%s", lcZipFileType::Unofficial);
			}

			if (Found)
				Callback(IncludeFile);
		}
		else
		{
			lcDiskFile IncludeFile;

			auto LoadIncludeFile = [&IncludeFile, PieceName, this](const QLatin1String& Folder)
			{
				const QString IncludeFileName = Folder + PieceName;
				IncludeFile.SetFileName(mLibraryDir.absoluteFilePath(IncludeFileName));
				if (IncludeFile.Open(QIODevice::ReadOnly))
					return true;

#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
				// todo: search the parts/primitive lists and get the file name from there instead of using toLower
				IncludeFile.SetFileName(mLibraryDir.absoluteFilePath(IncludeFileName.toLower()));
				return IncludeFile.Open(QIODevice::ReadOnly);
#else
				return false;
#endif
			};

			Found = LoadIncludeFile(QLatin1String("parts/"));

			if (!Found)
				Found = LoadIncludeFile(QLatin1String("p/"));

			if (mHasUnofficial && !Found)
			{
				Found = LoadIncludeFile(QLatin1String("unofficial/parts/"));

				if (!Found)
					Found = LoadIncludeFile(QLatin1String("unofficial/p/"));
			}

			if (Found)
				Callback(IncludeFile);
		}
	}
}

void lcPiecesLibrary::ReleaseBuffers()
{
	lcContext* Context = lcContext::GetGlobalOffscreenContext();

	Context->MakeCurrent();
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
		lcMesh* Mesh = Info->GetMesh();

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
		if (Info->GetRefCount() == 0 && Info->mState != lcPieceInfoState::Unloaded)
			ReleasePieceInfo(Info);
	}
}

bool lcPiecesLibrary::LoadTexture(lcTexture* Texture)
{
	QMutexLocker Lock(&mTextureMutex);
	char FileName[2*LC_MAXPATH];

	if (mZipFiles[static_cast<int>(lcZipFileType::Official)])
	{
		lcMemFile TextureFile;

		sprintf(FileName, "ldraw/parts/textures/%s.png", Texture->mName);

		if (!mZipFiles[static_cast<int>(lcZipFileType::Official)]->ExtractFile(FileName, TextureFile))
		{
			sprintf(FileName, "parts/textures/%s.png", Texture->mName);

			if (!mZipFiles[static_cast<int>(lcZipFileType::Unofficial)] || !mZipFiles[static_cast<int>(lcZipFileType::Unofficial)]->ExtractFile(FileName, TextureFile))
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

bool lcPiecesLibrary::SupportsStudStyle() const
{
	return true;
}

void lcPiecesLibrary::SetStudStyle(lcStudStyle StudStyle, bool Reload)
{
	if (mStudStyle == StudStyle)
		return;

	mStudStyle = StudStyle;

	LoadColors();
	UpdateStudStyleSource();

	mLoadMutex.lock();

	for (const std::unique_ptr<lcLibrarySource>& Source : mSources)
	{
		for (const auto& PrimitiveIt : Source->Primitives)
		{
			lcLibraryPrimitive* Primitive = PrimitiveIt.second;

			if (Primitive->mStudStyle || Primitive->mMeshData.mHasStyleStud)
				Primitive->Unload();
		}
	}

	mLoadMutex.unlock();

	if (Reload)
	{
		mLoadMutex.lock();

		for (const auto& PieceIt : mPieces)
		{
			PieceInfo* Info = PieceIt.second;

			if (Info->mState == lcPieceInfoState::Loaded && Info->GetMesh() && Info->GetMesh()->mFlags & lcMeshFlag::HasStyleStud)
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

bool lcPiecesLibrary::IsPrimitive(const char* Name) const
{
	for (const std::unique_ptr<lcLibrarySource>& Source : mSources)
		if (Source->Primitives.find(Name) != Source->Primitives.end())
			return true;

	return false;
}

lcLibraryPrimitive* lcPiecesLibrary::FindPrimitive(const char* Name) const
{
	for (const std::unique_ptr<lcLibrarySource>& Source : mSources)
	{
		const auto PrimitiveIt = Source->Primitives.find(Name);

		if (PrimitiveIt != Source->Primitives.end())
			return PrimitiveIt->second;
	}

	return  nullptr;
}

bool lcPiecesLibrary::LoadPrimitive(lcLibraryPrimitive* Primitive)
{
	mLoadMutex.lock();

	if (Primitive->mState == lcPrimitiveState::NotLoaded)
		Primitive->mState = lcPrimitiveState::Loading;
	else
	{
		mLoadMutex.unlock();

		while (Primitive->mState == lcPrimitiveState::Loading)
			lcSleeper::msleep(5);

		return Primitive->mState == lcPrimitiveState::Loaded;
	}

	mLoadMutex.unlock();

	lcMeshLoader MeshLoader(Primitive->mMeshData, true, nullptr, false);

	if (mZipFiles[static_cast<int>(lcZipFileType::Official)])
	{
		lcLibraryPrimitive* LowPrimitive = nullptr;

		lcMemFile PrimFile;

		if (Primitive->mStud && !Primitive->mStudStyle)
		{
			if (strncmp(Primitive->mName, "8/", 2)) // todo: this is currently the only place that uses mName so use mFileName instead. this should also be done for the loose file libraries.
			{
				char Name[LC_PIECE_NAME_LEN];
				strcpy(Name, "8/");
				strcat(Name, Primitive->mName);
				strupr(Name);

				LowPrimitive = FindPrimitive(Name); // todo: low primitives don't work with studlogo, because the low stud gets added as shared
			}
		}

		if (!mZipFiles[static_cast<int>(Primitive->mZipFileType)]->ExtractFile(Primitive->mZipFileIndex, PrimFile))
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

			if (!mZipFiles[static_cast<int>(LowPrimitive->mZipFileType)]->ExtractFile(LowPrimitive->mZipFileIndex, PrimFile))
				return false;

			if (!MeshLoader.LoadMesh(PrimFile, LC_MESHDATA_LOW))
				return false;
		}
	}
	else
	{
		if (Primitive->mZipFileType == lcZipFileType::Count)
		{
			lcDiskFile PrimFile(Primitive->mFileName);

			if (!PrimFile.Open(QIODevice::ReadOnly) || !MeshLoader.LoadMesh(PrimFile, LC_MESHDATA_SHARED)) // todo: LOD like the zip files
				return false;
		}
		else
		{
			lcMemFile PrimFile;

			if (!mZipFiles[static_cast<int>(Primitive->mZipFileType)]->ExtractFile(Primitive->mZipFileIndex, PrimFile))
				return false;

			if (!MeshLoader.LoadMesh(PrimFile, LC_MESHDATA_SHARED))
				return false;
		}
	}

	mLoadMutex.lock();
	Primitive->mState = lcPrimitiveState::Loaded;
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
	std::unique_ptr<lcDiskFile> File(new lcDiskFile(":/resources/library.zip"));

	if (!File->Open(QIODevice::ReadOnly) || !OpenArchive(std::move(File), lcZipFileType::Official))
		return false;

	lcMemFile PieceFile;

	for (const auto& PieceIt : mPieces)
	{
		PieceInfo* Info = PieceIt.second;

		mZipFiles[static_cast<int>(Info->mZipFileType)]->ExtractFile(Info->mZipFileIndex, PieceFile, 256);
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

	lcLoadDefaultColors(lcStudStyle::Plain);
	lcLoadDefaultCategories(true);
	lcSynthInit();

	return true;
}
