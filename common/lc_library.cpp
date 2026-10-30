#include "lc_global.h"
#include "lc_library.h"
#include "lc_assetloader.h"
#include "lc_thumbnailmanager.h"
#include "lc_zipfile.h"
#include "lc_file.h"
#include "lc_piecedata.h"
#include "lc_piecemanager.h"
#include "lc_colors.h"
#include "lc_texture.h"
#include "lc_texturemanager.h"
#include "lc_category.h"
#include "lc_application.h"
#include "lc_context.h"
#include "lc_glextensions.h"
#include "lc_synth.h"
#include "lc_traintrack.h"
#include "project.h"
#include "lc_profile.h"
#include "lc_meshloader.h"
#include "lc_model.h"
#include "lc_view.h"
#include "lc_string.h"

// Cache payloads use Qt compression with checksum validation.
constexpr quint32 LC_LIBRARY_CACHE_VERSION = 0x0115;
constexpr qint32 LC_LIBRARY_MESH_CACHE_VERSION = 2;

enum class lcLibrarySourceType
{
	Library,
	StudStyle
};

struct lcLibrarySource
{
	lcLibrarySource() = default;

	lcLibrarySource(const lcLibrarySource&) = delete;
	lcLibrarySource(lcLibrarySource&&) = delete;
	lcLibrarySource& operator=(const lcLibrarySource&) = delete;
	lcLibrarySource& operator=(lcLibrarySource&&) = delete;

	lcLibrarySourceType Type;
	std::map<std::string, std::unique_ptr<lcLibraryPrimitive>> Primitives;
};

lcLibraryPrimitive::lcLibraryPrimitive(lcFileSource Source, const char* Name, bool Stud, bool StudStyle, bool SubFile)
	: mSource(std::move(Source))
{
	Q_ASSERT(!mSource.EmbeddedData);
	Q_ASSERT(mSource.FilePath.isEmpty() || (mSource.ArchiveType == -1 && mSource.ArchiveIndex == -1));
	Q_ASSERT(!mSource.FilePath.isEmpty() || (mSource.ArchiveType >= 0 && mSource.ArchiveType < static_cast<int>(lcZipFileType::Count) && mSource.ArchiveIndex >= 0));

	lcstrcpy(mName, Name);

	mState = lcPrimitiveState::NotLoaded;
	mStud = Stud;
	mStudStyle = StudStyle;
	mSubFile = SubFile;
}

lcPiecesLibrary::lcPiecesLibrary(lcStudStyle StudStyle, bool StudCylinderColorEnabled)
	: mStudStyle(StudStyle), mStudCylinderColorEnabled(StudCylinderColorEnabled)
{
	mPieceManager = std::make_unique<lcPieceManager>();
	mTextureManager = std::make_unique<lcTextureManager>();
	mAssetLoader = std::unique_ptr<lcAssetLoader>(new lcAssetLoader(this));
	mThumbnailManager = std::unique_ptr<lcThumbnailManager>(new lcThumbnailManager(this));
	mLoadingMesh = std::unique_ptr<lcMesh>(new lcMesh);
	mLoadingMesh->CreateBox();
	QStringList cachePathList = QStandardPaths::standardLocations(QStandardPaths::CacheLocation);
	mCachePath = cachePathList.first();

	QDir Dir;
	Dir.mkpath(mCachePath);

	mBuffersDirty = false;
	mHasUnofficial = false;
}

lcPiecesLibrary::~lcPiecesLibrary()
{
	mThumbnailManager.reset();
	mCancelLoading.store(true);
	mAssetLoader.reset();
	Unload();
	ReleaseBuffers();
}

void lcPiecesLibrary::Unload()
{
	Q_ASSERT(QThread::currentThread() == thread());

	const bool HadLoader = static_cast<bool>(mAssetLoader);

	if (HadLoader && mThumbnailManager)
		mThumbnailManager->Clear();

	mAssetLoader.reset();
	mSynthPieces.clear();

	mPieceManager->Clear();

	mSources.clear();

	mTextureManager->Clear();
	mTextureDiskEntries.clear();

	for (std::map<QString, int>& Entries : mTextureArchiveEntries)
		Entries.clear();

	mOfficialPieceCount = 0;

	for (std::unique_ptr<lcZipFile>& ZipFile : mZipFiles)
		ZipFile.reset();

	if (HadLoader)
		mAssetLoader = std::unique_ptr<lcAssetLoader>(new lcAssetLoader(this));
}

void lcPiecesLibrary::RemoveTemporaryPieceData()
{
	mPieceManager->RemoveTemporaryPieceData();
}

void lcPiecesLibrary::ReleaseProjectPieceData(Project* OwnerProject)
{
	mPieceManager->ReleaseProjectPieceData(OwnerProject);
}

void lcPiecesLibrary::TransferProjectPieceData(Project* Source, Project* Destination)
{
	mPieceManager->TransferProjectPieceData(Source, Destination);
}

bool lcPiecesLibrary::RenameModelPieceData(lcPieceData* PieceData, const char* NewName)
{
	if (!mPieceManager->RenameModelPieceData(PieceData, NewName))
		return false;

	if (PieceData->mState != lcAssetState::Failed)
		ClearPieceDataLoadError(PieceData);

	return true;
}

std::string lcPiecesLibrary::NormalizePieceDataName(const char* PieceName)
{
	return lcPieceManager::NormalizePieceDataName(PieceName);
}

lcPieceDataRef lcPiecesLibrary::AcquireModelPieceData(const char* PieceName, Project* Project, bool& Reused)
{
	lcPieceData* PieceData = mPieceManager->AcquireModelPieceData(PieceName, Project, Reused);

	lcPieceDataRef PieceDataRef = lcPieceDataRef::Adopt(PieceData, this);

	if (Reused)
		InvalidatePieceDataLoad(PieceData);

	return PieceDataRef;
}

lcResult<Project*> lcPiecesLibrary::LoadExternalProject(const QFileInfo& FileInfo, bool Preview, bool DeferModelMeshRequests)
{
	static thread_local std::vector<QString> LoadingPaths;
	const QString Path = FileInfo.canonicalFilePath();

	if (std::find(LoadingPaths.begin(), LoadingPaths.end(), Path) != LoadingPaths.end())
		return lcUnexpected(tr("Circular reference between external model files at '%1'.").arg(Path));

	LoadingPaths.push_back(Path);

	Project* ExternalProject = new Project(Preview);
	ExternalProject->SetDeferModelMeshRequests(DeferModelMeshRequests);
	const bool Loaded = ExternalProject->Load(FileInfo.absoluteFilePath(), false);

	LoadingPaths.pop_back();

	if (!Loaded)
	{
		delete ExternalProject;
		return lcUnexpected(tr("Could not load external model file '%1'.").arg(FileInfo.absoluteFilePath()));
	}

	return ExternalProject;
}

lcPieceData* lcPiecesLibrary::FindPieceData(const char* PieceName, Project* CurrentProject)
{
	const std::string Name = NormalizePieceDataName(PieceName);

	return mPieceManager->FindPieceData(Name, CurrentProject);
}

lcPieceDataRef lcPiecesLibrary::AcquirePieceData(const char* PieceName, Project* CurrentProject, lcAcquirePieceDataFlags Flags)
{
	Q_ASSERT(QThread::currentThread() == thread());

	const bool ProjectFileOnly = Flags.testFlag(lcAcquirePieceDataFlag::ProjectFileOnly);

	Q_ASSERT(!ProjectFileOnly || !Flags.testFlag(lcAcquirePieceDataFlag::CreateMissing));

	QString ProjectPath;
	QString LoadError;
	lcPieceDataRef MissingPieceData;

	if (lcPieceData* PieceData = FindPieceData(PieceName, CurrentProject))
	{
		// A bare filename must resolve to the same asset when the document is reopened.
		// Do not offer disk files shadowed by embedded models or library parts.
		if (!ProjectFileOnly)
			return lcPieceDataRef(PieceData, this);

		if (PieceData->IsProject())
		{
			const std::vector<const Project*> Dependencies = PieceData->GetProject()->GetDependencies();
			const bool Changed = std::any_of(Dependencies.begin(), Dependencies.end(), [](const Project* Dependency)
			{
				if (Dependency->HasFileChangedOnDisk())
					return true;

				const QDir Directory(QFileInfo(Dependency->GetFileName()).absolutePath());

				for (const std::unique_ptr<lcModel>& Model : Dependency->GetModels())
					for (const std::unique_ptr<lcPiece>& Piece : Model->GetPieces())
					{
						const lcPieceData* Data = Piece->mPieceData.Get();

						if (Data && !Data->IsLibraryPiece() && !Data->IsModel() && !Data->IsProject() &&
							QFileInfo(Directory.filePath(Piece->GetID())).isFile())
							return true;
					}

				return false;
			});

			if (!Changed)
			{
				if (CurrentProject && PieceData->GetProject()->ReferencesFile(CurrentProject->GetFileName()))
					return lcPieceDataRef();

				return lcPieceDataRef(PieceData, this);
			}
		}
		else if (PieceData->IsLibraryPiece() || PieceData->IsModel() || PieceData->mState != lcAssetState::Failed)
			return lcPieceDataRef();

		// Keep placements bound to the same identity when recovering or reloading a file.
		MissingPieceData = lcPieceDataRef(PieceData, this);
	}

	if (CurrentProject && (Flags.testFlag(lcAcquirePieceDataFlag::SearchProjectFolder) || ProjectFileOnly) && !CurrentProject->GetFileName().isEmpty())
		ProjectPath = QFileInfo(CurrentProject->GetFileName()).absolutePath();

	if (!ProjectPath.isEmpty())
	{
		QFileInfo ProjectFile = QFileInfo(ProjectPath + QDir::separator() + PieceName);

		if (ProjectFile.isFile())
		{
			const lcResult<Project*> ExternalProject = LoadExternalProject(ProjectFile, CurrentProject && CurrentProject->IsPreview(), CurrentProject && CurrentProject->DefersModelMeshRequests());

			if (ExternalProject)
			{
				if (ProjectFileOnly && ExternalProject.value()->ReferencesFile(CurrentProject->GetFileName()))
				{
					delete ExternalProject.value();
					return lcPieceDataRef();
				}

				lcPieceData* PieceData = MissingPieceData ? MissingPieceData.Get() : mPieceManager->CreatePieceData(PieceName, CurrentProject);

				if (MissingPieceData)
					InvalidatePieceDataLoad(PieceData);

				PieceData->CreateProject(ExternalProject.value(), PieceName);

				if (MissingPieceData)
					NotifyAssetStatusChanged();

				return lcPieceDataRef(PieceData, this);
			}
			else
				LoadError = ExternalProject.error();
		}
	}

	if (Flags.testFlag(lcAcquirePieceDataFlag::CreateMissing))
	{
		lcPieceData* PieceData = mPieceManager->CreatePieceData(PieceName, CurrentProject);

		PieceData->SetFailed(LoadError.isEmpty() ? tr("Could not find piece '%1'.").arg(QString::fromLatin1(PieceName)) : LoadError);

		return lcPieceDataRef(PieceData, this);
	}

	return lcPieceDataRef();
}

bool lcPiecesLibrary::RemapProjectPieceData(lcPieceData* PieceData, const QString& ProjectDirectory, bool IsPreview)
{
	const QByteArray PieceName(PieceData->mFileName);
	const QFileInfo Candidate(QDir(ProjectDirectory).absoluteFilePath(QString::fromLatin1(PieceName.constData())));

	if (PieceData->IsProject() && Candidate.isFile() &&
		QFileInfo(PieceData->GetProject()->GetFileName()).canonicalFilePath() == Candidate.canonicalFilePath())
		return false;

	if (!PieceData->IsProject() && !Candidate.isFile())
		return false;

	InvalidatePieceDataLoad(PieceData);

	if (Candidate.isFile())
	{
		const lcResult<Project*> ExternalProject = LoadExternalProject(Candidate, IsPreview, true);

		if (ExternalProject)
			PieceData->CreateProject(ExternalProject.value(), PieceName.constData());
		else
		{
			PieceData->DetachContainer();
			PieceData->SetFailed(ExternalProject.error());
		}
	}
	else
	{
		PieceData->DetachContainer();
		PieceData->SetFailed(tr("Could not find external model file '%1'.").arg(Candidate.absoluteFilePath()));
	}

	NotifyAssetStatusChanged();

	return true;
}

lcTextureRef lcPiecesLibrary::AcquireTexture(const char* TextureName, Project* CurrentProject, bool SearchProjectFolder)
{
	QString ProjectPath;

	if (SearchProjectFolder && CurrentProject)
	{
		QString FileName = CurrentProject->GetFileName();

		if (!FileName.isEmpty())
			ProjectPath = QFileInfo(FileName).absolutePath();
	}

	std::map<QString, std::shared_ptr<const lcEmbeddedData>> EmbeddedData;

	if (CurrentProject)
		EmbeddedData = CurrentProject->GetEmbeddedDataLookup();

	lcFileSource Source;
	return AcquireTexture(TextureName, std::vector<QString>{ ProjectPath }, EmbeddedData, Source);
}

QString lcPiecesLibrary::FindProjectTextureFile(const QString& ProjectPath, const QString& TextureName)
{
	QDir Directory(ProjectPath);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	const auto SkipEmptyParts = Qt::SkipEmptyParts;
#else
	const auto SkipEmptyParts = QString::SplitBehavior::SkipEmptyParts;
#endif
	const QStringList PathComponents = (TextureName + QLatin1String(".png")).split(QLatin1Char('/'), SkipEmptyParts);

	for (int ComponentIdx = 0; ComponentIdx < PathComponents.size(); ComponentIdx++)
	{
		const QString& Component = PathComponents[ComponentIdx];
		const bool LastComponent = ComponentIdx == PathComponents.size() - 1;

		if (Component == QLatin1String(".") || Component == QLatin1String(".."))
		{
			if (LastComponent || !Directory.cd(Component))
				return QString();
			continue;
		}

		QFileInfo Entry(Directory, Component);

		if (LastComponent ? !Entry.isFile() : !Entry.isDir())
		{
			Entry = QFileInfo();

			for (const QFileInfo& Candidate : Directory.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System))
			{
				if (Candidate.fileName().compare(Component, Qt::CaseInsensitive) == 0 &&
					(LastComponent ? Candidate.isFile() : Candidate.isDir()))
				{
					Entry = Candidate;
					break;
				}
			}
		}

		if (LastComponent)
			return Entry.isFile() ? Entry.absoluteFilePath() : QString();

		if (!Entry.isDir())
			return QString();

		Directory.setPath(Entry.absoluteFilePath());
	}

	return QString();
}

lcFileSource lcPiecesLibrary::ResolveTextureSource(const char* TextureName, const std::vector<QString>& SearchDirectories, const std::map<QString, std::shared_ptr<const lcEmbeddedData>>& EmbeddedData, QString& ProjectPath)
{
	Q_ASSERT(QThread::currentThread() == thread());

	ProjectPath.clear();

	if (!TextureName[0] || strlen(TextureName) >= LC_TEXTURE_NAME_LEN)
		return lcFileSource();

	const bool ArchiveLibrary = static_cast<bool>(mZipFiles[static_cast<int>(lcZipFileType::Official)]);
	lcFileSource Source;
	const QString Name = QString::fromLatin1(TextureName);
	const QString Candidates[] = { QLatin1String("textures/") + Name, Name };
	const QString Folders[] = { QStringLiteral("p/"), QStringLiteral("parts/"), QStringLiteral("models/"), QStringLiteral("unofficial/p/"), QStringLiteral("unofficial/parts/") };

	// Complete the prefixed search before considering any unprefixed location.
	for (const QString& Candidate : Candidates)
	{
		QString EmbeddedName = Candidate + QLatin1String(".png");
		EmbeddedName.replace(QLatin1Char('\\'), QLatin1Char('/'));
		const auto Embedded = EmbeddedData.find(QDir::cleanPath(EmbeddedName).toUpper());

		if (Embedded != EmbeddedData.end())
		{
			Source.EmbeddedData = Embedded->second;
			break;
		}

		for (const QString& Directory : SearchDirectories)
		{
			if (Directory.isEmpty())
				continue;

			Source.FilePath = FindProjectTextureFile(Directory, Candidate);

			if (!Source.FilePath.isEmpty())
			{
				ProjectPath = Directory;
				break;
			}
		}

		if (!Source.FilePath.isEmpty())
			break;

		for (const QString& Folder : Folders)
		{
			if (ArchiveLibrary)
			{
				const bool Unofficial = Folder.startsWith(QLatin1String("unofficial/"));
				const int Type = static_cast<int>(Unofficial ? lcZipFileType::Unofficial : lcZipFileType::Official);
				QString EntryName;

				if (Unofficial)
					EntryName = Folder.mid(11) + Candidate + QLatin1String(".png");
				else
					EntryName = QLatin1String("ldraw/") + Folder + Candidate + QLatin1String(".png");

				const std::map<QString, int>& Entries = mTextureArchiveEntries[Type];
				const auto Entry = Entries.find(QDir::cleanPath(EntryName).toUpper());

				if (Entry != Entries.end())
				{
					Source.ArchiveType = Type;
					Source.ArchiveIndex = Entry->second;
					break;
				}
			}
			else
			{
				const QString EntryName = QDir::cleanPath(Folder + Candidate + QLatin1String(".png")).toUpper();
				const auto Entry = mTextureDiskEntries.find(EntryName);

				if (Entry != mTextureDiskEntries.end())
				{
					Source.FilePath = Entry->second;
					break;
				}
			}
		}

		if (!Source.FilePath.isEmpty() || Source.ArchiveIndex != -1)
			break;
	}

	if (!Source.FilePath.isEmpty())
	{
		const QString CanonicalPath = QFileInfo(Source.FilePath).canonicalFilePath();
		Source.FilePath = CanonicalPath.isEmpty() ? QFileInfo(Source.FilePath).absoluteFilePath() : CanonicalPath;
	}

	return Source;
}

lcTextureRef lcPiecesLibrary::AcquireTexture(const char* TextureName, const std::vector<QString>& SearchDirectories, const std::map<QString, std::shared_ptr<const lcEmbeddedData>>& EmbeddedData, lcFileSource& Source)
{
	QString ProjectPath;
	Source = ResolveTextureSource(TextureName, SearchDirectories, EmbeddedData, ProjectPath);

	return lcTextureRef::Adopt(mTextureManager->AcquireTexture(TextureName, Source, ProjectPath), this);
}

void lcPiecesLibrary::RequestTextureLoad(lcTexture* Texture, lcLoadPriority LoadPriority)
{
	Q_ASSERT(QThread::currentThread() == thread());

	mAssetLoader->RequestTextureLoad(Texture, Texture->mSource, LoadPriority);
}

bool lcPiecesLibrary::WaitForTexture(const lcTextureRef& TextureRef)
{
	Q_ASSERT(QThread::currentThread() == thread());

	return mAssetLoader->WaitForTexture(TextureRef.Get(), TextureRef->mSource);
}

lcTextureBuildResult lcPiecesLibrary::BuildTextureData(const lcFileSource& Source)
{
	const lcResult<std::unique_ptr<lcFile>> File = OpenFile(Source);

	if (!File)
	{
		lcTextureBuildResult Result;
		Result.Error = lcTextureLoadError::DecodeFailed;
		Result.ErrorDetails = File.error();
		return Result;
	}

	lcTextureBuildResult Result = mTextureManager->BuildTextureData(*File.value());

	if (Result.DecodedImage.isNull() && Source.EmbeddedData)
		Result.ErrorDetails = tr("Could not decode embedded texture '%1'.").arg(Source.EmbeddedData->FileName);

	return Result;
}

lcResult<std::unique_ptr<lcFile>> lcPiecesLibrary::OpenFile(const lcFileSource& Source)
{
	if (Source.EmbeddedData)
	{
		const lcEmbeddedData& Data = *Source.EmbeddedData;

		if (!Data.Error.isEmpty())
			return lcUnexpected(Data.Error);

		if (!Data.HasValidEncoding())
			return lcUnexpected(tr("Invalid Base64 data for embedded file '%1'.").arg(Data.FileName));

		const QByteArray Bytes = QByteArray::fromBase64(Data.EncodedData);
		std::unique_ptr<lcMemFile> File(new lcMemFile);

		if (File->WriteBuffer(Bytes.constData(), Bytes.size()) != static_cast<size_t>(Bytes.size()))
			return lcUnexpected(tr("Could not read embedded file '%1'.").arg(Data.FileName));

		File->Seek(0, SEEK_SET);
		return std::unique_ptr<lcFile>(std::move(File));
	}

	if (!Source.FilePath.isEmpty())
	{
		std::unique_ptr<lcDiskFile> File(new lcDiskFile(Source.FilePath));

		if (!File->Open(QIODevice::ReadOnly))
			return lcUnexpected(tr("Could not open file '%1'.").arg(Source.FilePath));

		return std::unique_ptr<lcFile>(std::move(File));
	}

	if (Source.ArchiveType < 0 || Source.ArchiveType >= static_cast<int>(lcZipFileType::Count) || Source.ArchiveIndex < 0)
		return lcUnexpected(tr("Invalid archive file source."));

	std::unique_ptr<lcMemFile> File(new lcMemFile);

	// Source archives are stable until workers are drained; extraction locks
	// the selected archive internally without serializing unrelated archives.
	const std::unique_ptr<lcZipFile>& Archive = mZipFiles[Source.ArchiveType];

	if (!Archive || static_cast<size_t>(Source.ArchiveIndex) >= Archive->mFiles.size())
		return lcUnexpected(tr("Could not read archive entry %1.").arg(Source.ArchiveIndex));

	if (!Archive->ExtractFile(Source.ArchiveIndex, *File))
		return lcUnexpected(tr("Could not read archive file '%1'.").arg(QString::fromLatin1(Archive->mFiles[Source.ArchiveIndex].file_name)));

	File->Seek(0, SEEK_SET);
	return std::unique_ptr<lcFile>(std::move(File));
}

bool lcPiecesLibrary::Load(const QString& LibraryPath, bool ShowProgress, bool OfficialLibrary)
{
	Unload();

	if (OpenArchive(LibraryPath, lcZipFileType::Official))
	{
		if (OfficialLibrary)
			mOfficialPieceCount = static_cast<int>(mPieceManager->GetLibraryPieceDataCount());

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
	lcTrainTrackInfo::Initialize(this);

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
			UpdateLoadingMeshColors();
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

	UpdateLoadingMeshColors();
	emit ColorsLoaded();
}

void lcPiecesLibrary::UpdateLoadingMeshColors()
{
	lcMeshSection* Sections = mLoadingMesh->mLods[LC_MESH_LOD_HIGH].Sections;

	Sections[0].ColorIndex = gDefaultColor;
	Sections[1].ColorIndex = gEdgeColor;
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

qint32 lcPiecesLibrary::MeshCacheSettingsKey(lcStudStyle StudStyle, bool StudCylinderColorEnabled)
{
	return (LC_LIBRARY_MESH_CACHE_VERSION << 16) | (static_cast<qint32>(StudStyle) << 1) | static_cast<qint32>(StudCylinderColorEnabled);
}

void lcPiecesLibrary::UpdateStudStyleSource()
{
	if (!mSources.empty() && mSources.front()->Type == lcLibrarySourceType::StudStyle)
		mSources.erase(mSources.begin());

	mZipFiles[static_cast<int>(lcZipFileType::StudStyle)].reset();

	if (mStudStyle == lcStudStyle::Plain || (mStudStyle >= lcStudStyle::HighContrast && !mStudCylinderColorEnabled))
		return;

	const QLatin1String FileNames[] =
	{
		QLatin1String(""),								  // Plain
		QLatin1String(":/resources/studlogo1.zip"),		  // ThinLinesLogo
		QLatin1String(":/resources/studlogo2.zip"),		  // OutlineLogo
		QLatin1String(":/resources/studlogo3.zip"),		  // SharpTopLogo
		QLatin1String(":/resources/studlogo4.zip"),		  // RoundedTopLogo
		QLatin1String(":/resources/studlogo5.zip"),		  // FlattenedLogo
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

	mTextureArchiveEntries[static_cast<int>(ZipFileType)].clear();

	std::unique_ptr<lcLibrarySource> Source(new lcLibrarySource);
	Source->Type = ZipFileType != lcZipFileType::StudStyle ? lcLibrarySourceType::Library : lcLibrarySourceType::StudStyle;

	for (quint32 FileIdx = 0; FileIdx < ZipFile->mFiles.size(); FileIdx++)
	{
		lcZipFileInfo& FileInfo = ZipFile->mFiles[FileIdx];
		const std::string NormalizedName = NormalizePieceDataName(FileInfo.file_name);
		const char* Name = NormalizedName.c_str();

		if (NormalizedName.size() <= 4)
			continue;

		const char* Extension = Name + NormalizedName.size() - 4;

		if (memcmp(Extension, ".DAT", 4))
		{
			if (!memcmp(Extension, ".PNG", 4))
			{
				if (ZipFileType == lcZipFileType::Official || ZipFileType == lcZipFileType::Unofficial)
					mTextureArchiveEntries[static_cast<int>(ZipFileType)].emplace(QDir::cleanPath(QString::fromLatin1(Name)).toUpper(), FileIdx);
			}

			continue;
		}

		if (ZipFileType == lcZipFileType::Official)
		{
			if (strncmp(Name, "LDRAW/", 6))
				continue;

			Name += 6;
		}

		if (!strncmp(Name, "PARTS/", 6))
		{
			Name += 6;

			if (strncmp(Name, "S/", 2))
			{
				lcPieceData* PieceData = FindPieceData(Name, nullptr);

				// Unofficial additions must not replace registered official parts.
				if (PieceData && ZipFileType == lcZipFileType::Unofficial && PieceData->mZipFileType == lcZipFileType::Official)
					continue;

				if (!PieceData)
				{
					PieceData = mPieceManager->CreateLibraryPieceData(Name, FileInfo.file_name + (Name - NormalizedName.c_str()));
				}

				PieceData->SetZipFile(ZipFileType, FileIdx);
			}
			else
			{
				lcFileSource PrimitiveSource;
				PrimitiveSource.ArchiveType = static_cast<int>(ZipFileType);
				PrimitiveSource.ArchiveIndex = FileIdx;

				Source->Primitives[Name] = std::make_unique<lcLibraryPrimitive>(std::move(PrimitiveSource), FileInfo.file_name + (Name - NormalizedName.c_str()), false, false, true);
			}
		}
		else if (!strncmp(Name, "P/", 2))
		{
			Name += 2;

			// Match directory discovery and loose-file lookup: parts/s/ wins over p/s/.
			const auto Existing = Source->Primitives.find(Name);

			if (Existing != Source->Primitives.end() && Existing->second->mSubFile)
				continue;

			lcFileSource PrimitiveSource;
			PrimitiveSource.ArchiveType = static_cast<int>(ZipFileType);
			PrimitiveSource.ArchiveIndex = FileIdx;

			Source->Primitives[Name] = std::make_unique<lcLibraryPrimitive>(std::move(PrimitiveSource), FileInfo.file_name + (Name - NormalizedName.c_str()), IsStudPrimitive(Name), IsStudStylePrimitive(Name), false);
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

		for (lcPieceData* PieceData : mPieceManager->GetLibraryPieceData())
		{
			mZipFiles[static_cast<int>(PieceData->mZipFileType)]->ExtractFile(PieceData->mZipFileIndex, PieceFile, 256);
			PieceFile.Seek(0, SEEK_END);
			PieceFile.WriteU8(0);

			char* Src = (char*)PieceFile.mBuffer + 2;
			char* Dst = PieceData->m_strDescription;

			for (;;)
			{
				if (*Src != '\r' && *Src != '\n' && *Src && Dst - PieceData->m_strDescription < (int)sizeof(PieceData->m_strDescription) - 1)
				{
					*Dst++ = *Src++;
					continue;
				}

				*Dst = 0;
				break;
			}
		}

		ClassifyPieces(DirectoryFilePaths());
		SaveArchiveCacheIndex(IndexFileName);
	}
}

bool lcPiecesLibrary::OpenDirectory(const QDir& LibraryDir, bool ShowProgress)
{
	mTextureDiskEntries.clear();

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

	constexpr std::pair<const char*, bool> PrimitiveDirectories[] =
	{
		{ "p/", false },
		{ "parts/s/", true }
	};

	for (unsigned int BaseFolderIdx = 0; BaseFolderIdx < LC_ARRAY_COUNT(BaseFolders); BaseFolderIdx++)
	{
		std::unique_ptr<lcLibrarySource> Source(new lcLibrarySource);
		Source->Type = lcLibrarySourceType::Library;

		QDir BaseDir(LibraryDir.absoluteFilePath(QLatin1String(BaseFolders[BaseFolderIdx])));

		for (const auto& [Path, SubFile] : PrimitiveDirectories)
		{
			QString ChildPath = BaseDir.absoluteFilePath(QLatin1String(Path));
			QDirIterator DirIterator(ChildPath, QStringList() << QLatin1String("*.dat"), QDir::Files | QDir::Hidden | QDir::Readable, QDirIterator::Subdirectories);

			while (DirIterator.hasNext())
			{
				QString FileName = DirIterator.next();
				QByteArray FileString = BaseDir.relativeFilePath(FileName).toLatin1();
				const char* RelativeName = strchr(FileString.constData(), '/') + 1;
				const std::string Name = NormalizePieceDataName(RelativeName);

				if (Name.size() <= 4 || Name.compare(Name.size() - 4, 4, ".DAT"))
					continue;

				if (BaseFolderIdx > 0 && FindPrimitive(Name.c_str()))
					continue;

				lcFileSource PrimitiveSource;
				PrimitiveSource.FilePath = QFileInfo(FileName).canonicalFilePath();

				if (PrimitiveSource.FilePath.isEmpty())
					continue;

				if (BaseFolderIdx == static_cast<int>(lcLibraryFolderType::Unofficial))
					mHasUnofficial = true;

				Source->Primitives[Name] = std::make_unique<lcLibraryPrimitive>(std::move(PrimitiveSource), RelativeName, !SubFile && IsStudPrimitive(Name.c_str()), IsStudStylePrimitive(Name.c_str()), SubFile);
			}
		}

		mSources.emplace_back(std::move(Source));

		// Index PNG paths once; UI-thread texture lookup must not enumerate library folders.
		const char* TextureDirectories[] = { "p/", "parts/", "models/" };

		for (const char* TextureDirectory : TextureDirectories)
		{
			const QString ChildPath = BaseDir.absoluteFilePath(QLatin1String(TextureDirectory));
			QDirIterator DirIterator(ChildPath, QStringList() << QLatin1String("*.png"), QDir::Files | QDir::Hidden | QDir::Readable, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);

			while (DirIterator.hasNext())
			{
				const QString FilePath = DirIterator.next();
				const QString Name = QDir::cleanPath(LibraryDir.relativeFilePath(FilePath)).toUpper();

				mTextureDiskEntries.emplace(Name, FilePath);
			}
		}
	}

	return true;
}

void lcPiecesLibrary::ReadDirectoryDescriptions(const QFileInfoList (&FileLists)[static_cast<int>(lcLibraryFolderType::Count)], bool ShowProgress)
{
	QString IndexFileName = QFileInfo(QDir(mCachePath), QLatin1String("index")).absoluteFilePath();
	lcMemFile IndexFile;

	struct CachedDescription
	{
		lcPieceData::CacheData CacheData;
		quint8 FolderType;
		quint64 FileTime;
	};

	std::map<std::string, CachedDescription> CachedDescriptions;

	if (ReadCacheFile(IndexFileName, IndexFile, CacheType::Directory) && IndexFile.ReadQString() == mLibraryDir.absolutePath())
	{
		quint32 Count;

		if (IndexFile.ReadU32(&Count, 1) == 1)
		{
			for (quint32 Index = 0; Index < Count; Index++)
			{
				std::string FileName;
				CachedDescription Entry;

				if (!ReadIndexString(IndexFile, FileName) || FileName.empty() ||
					!ReadIndexString(IndexFile, Entry.CacheData.Description) || !ReadIndexString(IndexFile, Entry.CacheData.PatternParentId) ||
					IndexFile.ReadU8(&Entry.CacheData.Flags, 1) != 1 ||
					IndexFile.ReadU8(&Entry.FolderType, 1) != 1 || IndexFile.ReadU64(&Entry.FileTime, 1) != 1 ||
					Entry.FolderType >= static_cast<quint8>(lcLibraryFolderType::Count) ||
					!CachedDescriptions.emplace(std::move(FileName), std::move(Entry)).second)
				{
					CachedDescriptions.clear();
					break;
				}
			}
		}
	}

	struct DescriptionJob
	{
		lcPieceData* PieceData;
		QFileInfo FileInfo;
		lcLibraryFolderType FolderType;
		bool Modified = false;
	};

	std::vector<DescriptionJob> Jobs;

	for (int FolderIdx = 0; FolderIdx < static_cast<int>(lcLibraryFolderType::Count); FolderIdx++)
	{
		const QFileInfoList& FileList = FileLists[FolderIdx];

		for (int FileIdx = 0; FileIdx < FileList.size(); FileIdx++)
		{
			QByteArray FileString = FileList[FileIdx].fileName().toLatin1();
			const std::string Name = NormalizePieceDataName(FileString.constData());

			if (FolderIdx > 0 && mPieceManager->FindPieceData(Name, nullptr))
				continue;

			lcPieceData* PieceData = mPieceManager->CreateLibraryPieceData(Name, FileString.constData());

			Jobs.push_back({ PieceData, FileList[FileIdx], static_cast<lcLibraryFolderType>(FolderIdx) });
		}
	}

	// Only process entries retained by the normalized registry after discovery.
	Jobs.erase(std::remove_if(Jobs.begin(), Jobs.end(), [this](const DescriptionJob& Job)
	{
		return FindPieceData(Job.FileInfo.fileName().toLatin1().constData(), nullptr) != Job.PieceData;
	}), Jobs.end());

	QAtomicInt FilesLoaded;

	auto ReadDescriptions = [&CachedDescriptions, &FilesLoaded](DescriptionJob& Job)
	{
		FilesLoaded.ref();

		lcPieceData* PieceData = Job.PieceData;
		lcDiskFile PieceFile(Job.FileInfo.absoluteFilePath());
		char Line[1024];

		const auto CachedIt = CachedDescriptions.find(PieceData->mFileName);
		const quint64 FileTime = Job.FileInfo.lastModified().toMSecsSinceEpoch();

		if (CachedIt != CachedDescriptions.end() && CachedIt->second.FileTime == FileTime && CachedIt->second.FolderType == static_cast<quint8>(Job.FolderType) &&
			PieceData->InitializeFromCache(CachedIt->second.CacheData))
			return;

		Job.Modified = true;

		if (!PieceFile.Open(QIODevice::ReadOnly) || !PieceFile.ReadLine(Line, sizeof(Line)))
		{
			lcstrcpy(PieceData->m_strDescription, "Unknown");
			return;
		}

		const char* Src = Line + 2;
		char* Dst = PieceData->m_strDescription;

		for (;;)
		{
			if (*Src != '\r' && *Src != '\n' && *Src && Dst - PieceData->m_strDescription < (int)sizeof(PieceData->m_strDescription) - 1)
			{
				*Dst++ = *Src++;
				continue;
			}

			*Dst = 0;
			break;
		}
	};

	QProgressDialog* ProgressDialog = new QProgressDialog(nullptr);
	ProgressDialog->setWindowFlags(ProgressDialog->windowFlags() & ~Qt::WindowCloseButtonHint);
	ProgressDialog->setWindowTitle(tr("Initializing"));
	ProgressDialog->setLabelText(tr("Loading Pieces Library"));
	ProgressDialog->setMaximum(static_cast<int>(Jobs.size()));
	ProgressDialog->setMinimum(0);
	ProgressDialog->setValue(0);
	ProgressDialog->setCancelButton(nullptr);
	ProgressDialog->setAutoReset(false);
	if (ShowProgress)
		ProgressDialog->show();

	QFuture<void> LoadFuture = QtConcurrent::map(Jobs, ReadDescriptions);

	while (!LoadFuture.isFinished())
	{
		ProgressDialog->setValue(FilesLoaded);
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}

	ProgressDialog->setValue(FilesLoaded);
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

	ProgressDialog->deleteLater();

	// A target's change, addition or removal can invalidate otherwise unchanged aliases.
	const bool Modified = std::any_of(Jobs.begin(), Jobs.end(), [](const DescriptionJob& Job)
	{
		return Job.Modified;
	});

	if (Modified || CachedDescriptions.size() != Jobs.size())
	{
		DirectoryFilePaths FilePaths;

		for (const DescriptionJob& Job : Jobs)
			FilePaths.emplace(Job.PieceData, Job.FileInfo.absoluteFilePath());

		ClassifyPieces(FilePaths);

		lcMemFile NewIndexFile;

		NewIndexFile.WriteQString(mLibraryDir.absolutePath());
		NewIndexFile.WriteU32(static_cast<quint32>(Jobs.size()));

		for (const DescriptionJob& Job : Jobs)
		{
			const lcPieceData* PieceData = Job.PieceData;
			const lcPieceData::CacheData CacheData = PieceData->GetCacheData();

			if (!WriteIndexString(NewIndexFile, PieceData->mFileName) ||
				!WriteIndexString(NewIndexFile, CacheData.Description) || !WriteIndexString(NewIndexFile, CacheData.PatternParentId))
				return;

			NewIndexFile.WriteU8(CacheData.Flags);
			NewIndexFile.WriteU8(static_cast<quint8>(Job.FolderType));
			const quint64 FileTime = Job.FileInfo.lastModified().toMSecsSinceEpoch();
			NewIndexFile.WriteU64(FileTime);
		}

		WriteCacheFile(IndexFileName, NewIndexFile, CacheType::Directory);
	}
}

bool lcPiecesLibrary::ReadCacheFile(const QString& FileName, lcMemFile& CacheFile, CacheType Type)
{
	QFile File(FileName);

	if (!File.open(QIODevice::ReadOnly))
		return false;

	quint32 CacheVersion, CacheFlags;

	if (File.read(reinterpret_cast<char*>(&CacheVersion), sizeof(CacheVersion)) != sizeof(CacheVersion) || CacheVersion != LC_LIBRARY_CACHE_VERSION)
		return false;

	if (File.read(reinterpret_cast<char*>(&CacheFlags), sizeof(CacheFlags)) != sizeof(CacheFlags) || CacheFlags != static_cast<quint32>(Type))
		return false;

	if (Type == CacheType::Archive)
	{
		qint64 CacheCheckSum[4];

		if (File.read(reinterpret_cast<char*>(CacheCheckSum), sizeof(CacheCheckSum)) != sizeof(CacheCheckSum) ||
			memcmp(CacheCheckSum, mArchiveCheckSum, sizeof(CacheCheckSum)))
			return false;
	}

	quint32 UncompressedSize;

	if (File.read(reinterpret_cast<char*>(&UncompressedSize), sizeof(UncompressedSize)) != sizeof(UncompressedSize))
		return false;

	const QByteArray Data = qUncompress(File.readAll());

	if (Data.isEmpty() || static_cast<quint64>(Data.size()) != UncompressedSize)
		return false;

	CacheFile.SetLength(Data.size());
	CacheFile.Seek(0, SEEK_SET);

	if (!CacheFile.mBuffer)
		return false;

	CacheFile.WriteBuffer(Data.constData(), Data.size());
	CacheFile.Seek(0, SEEK_SET);

	return true;
}

bool lcPiecesLibrary::WriteCacheFile(const QString& FileName, const lcMemFile& CacheFile, CacheType Type)
{
	if (CacheFile.GetLength() > static_cast<size_t>(std::numeric_limits<int>::max()))
		return false;

	QFile File(FileName);

	if (!File.open(QIODevice::WriteOnly))
		return false;

	constexpr quint32 CacheVersion = LC_LIBRARY_CACHE_VERSION;
	const quint32 CacheFlags = static_cast<quint32>(Type);

	if (File.write(reinterpret_cast<const char*>(&CacheVersion), sizeof(CacheVersion)) != sizeof(CacheVersion) ||
		File.write(reinterpret_cast<const char*>(&CacheFlags), sizeof(CacheFlags)) != sizeof(CacheFlags))
		return false;

	if (Type == CacheType::Archive && File.write(reinterpret_cast<const char*>(mArchiveCheckSum), sizeof(mArchiveCheckSum)) != sizeof(mArchiveCheckSum))
		return false;

	const quint32 UncompressedSize = static_cast<quint32>(CacheFile.GetLength());

	if (File.write(reinterpret_cast<const char*>(&UncompressedSize), sizeof(UncompressedSize)) != sizeof(UncompressedSize))
		return false;

	const QByteArray CompressedData = qCompress(CacheFile.mBuffer, static_cast<int>(CacheFile.GetLength()));

	return !CompressedData.isEmpty() && File.write(CompressedData) == CompressedData.size();
}

bool lcPiecesLibrary::ReadIndexString(lcMemFile& File, std::string& String)
{
	quint8 Length;

	if (File.ReadU8(&Length, 1) != 1)
		return false;

	String.resize(Length);

	return File.ReadBuffer(String.data(), Length) == Length;
}

bool lcPiecesLibrary::WriteIndexString(lcMemFile& File, const std::string& String)
{
	if (String.size() > 255)
		return false;

	const quint8 Length = static_cast<quint8>(String.size());

	return File.WriteU8(&Length, 1) == 1 && File.WriteBuffer(String.data(), Length) == Length;
}

std::string lcPiecesLibrary::ReadAliasTarget(lcFile& File)
{
	std::string Target;
	char Buffer[1024];

	while (File.ReadLine(Buffer, sizeof(Buffer)))
	{
		QString Line = QString::fromLatin1(Buffer).trimmed();

		if (Line.isEmpty())
			continue;

		QTextStream Stream(&Line, QIODevice::ReadOnly);
		int LineType;

		Stream >> LineType;

		if (Stream.status() != QTextStream::Ok)
			return std::string();

		if (LineType == 0)
			continue;

		// Aliases contain exactly one reference and no direct geometry.
		if (LineType != 1 || !Target.empty())
			return std::string();

		int Color;
		Stream >> Color;

		for (int ValueIndex = 0; ValueIndex < 12; ValueIndex++)
		{
			float Value;
			Stream >> Value;
		}

		if (Stream.status() != QTextStream::Ok)
			return std::string();

		const QByteArray Name = Stream.readAll().trimmed().toLatin1();

		if (Name.isEmpty() || Name.size() >= LC_PIECE_NAME_LEN)
			return std::string();

		Target = NormalizePieceDataName(Name.constData());
	}

	return Target;
}

void lcPiecesLibrary::ClassifyPieces(const DirectoryFilePaths& FilePaths)
{
	const std::vector<lcPieceData*> Parts = mPieceManager->GetLibraryPieceData();
	std::unordered_map<lcPieceData*, lcPieceData*> AliasTargets;

	for (lcPieceData* PieceData : Parts)
	{
		const std::string Name = NormalizePieceDataName(PieceData->mFileName);
		const std::vector<std::string> ParentIds = PieceData->Classify(Name);

		if (PieceData->IsAlias())
		{
			lcFileSource Source;

			if (PieceData->mZipFileType != lcZipFileType::Count)
			{
				Source.ArchiveType = static_cast<int>(PieceData->mZipFileType);
				Source.ArchiveIndex = PieceData->mZipFileIndex;
			}
			else
			{
				const auto FileIt = FilePaths.find(PieceData);

				if (FileIt == FilePaths.end())
				{
					AliasTargets.emplace(PieceData, nullptr);
					continue;
				}

				Source.FilePath = FileIt->second;
			}

			const lcResult<std::unique_ptr<lcFile>> File = OpenFile(Source);
			lcPieceData* Target = nullptr;

			if (File)
			{
				const std::string TargetId = ReadAliasTarget(*File.value());

				if (!TargetId.empty())
					Target = FindPieceData(TargetId.c_str(), nullptr);
			}

			AliasTargets.emplace(PieceData, Target);
			continue;
		}

		for (const std::string& ParentId : ParentIds)
		{
			const lcPieceData* Parent = FindPieceData(ParentId.c_str(), nullptr);

			if (Parent && Parent->IsLibraryPiece())
			{
				PieceData->SetPatternParentId(ParentId);
				break;
			}
		}
	}

	std::unordered_set<lcPieceData*> ResolvedAliases;

	for (const auto& [Alias, Target] : AliasTargets)
	{
		if (ResolvedAliases.find(Alias) != ResolvedAliases.end())
			continue;

		std::vector<lcPieceData*> Chain;
		lcPieceData* Current = Alias;
		bool Cycle = false;

		while (Current)
		{
			const auto TargetIt = AliasTargets.find(Current);

			if (TargetIt == AliasTargets.end() || ResolvedAliases.find(Current) != ResolvedAliases.end())
				break;

			if (std::find(Chain.begin(), Chain.end(), Current) != Chain.end())
			{
				Cycle = true;
				break;
			}

			Chain.push_back(Current);
			Current = TargetIt->second;
		}

		// Only the referencing alias inherits decoration from its target.
		for (auto PieceIt = Chain.rbegin(); PieceIt != Chain.rend(); PieceIt++)
		{
			lcPieceData* PieceData = *PieceIt;
			PieceData->SetAliasTarget(Cycle ? nullptr : Current);
			Current = PieceData;
			ResolvedAliases.insert(PieceData);
		}
	}
}

bool lcPiecesLibrary::LoadCacheIndex(const QString& FileName)
{
	lcMemFile IndexFile;

	if (!ReadCacheFile(FileName, IndexFile, CacheType::Archive))
		return false;

	quint32 NumFiles;

	if (IndexFile.ReadU32(&NumFiles, 1) != 1 || NumFiles != mPieceManager->GetLibraryPieceDataCount())
		return false;

	for (lcPieceData* PieceData : mPieceManager->GetLibraryPieceData())
	{
		lcPieceData::CacheData CacheData;

		if (!ReadIndexString(IndexFile, CacheData.Description) || !ReadIndexString(IndexFile, CacheData.PatternParentId) ||
			IndexFile.ReadU8(&CacheData.Flags, 1) != 1 || !PieceData->InitializeFromCache(CacheData))
			return false;
	}

	return true;
}

bool lcPiecesLibrary::SaveArchiveCacheIndex(const QString& FileName)
{
	lcMemFile IndexFile;

	const quint32 NumFiles = (quint32)mPieceManager->GetLibraryPieceDataCount();

	if (IndexFile.WriteBuffer((char*)&NumFiles, sizeof(NumFiles)) == 0)
		return false;

	for (const lcPieceData* PieceData : mPieceManager->GetLibraryPieceData())
	{
		const lcPieceData::CacheData CacheData = PieceData->GetCacheData();

		if (!WriteIndexString(IndexFile, CacheData.Description) || !WriteIndexString(IndexFile, CacheData.PatternParentId))
			return false;

		IndexFile.WriteU8(CacheData.Flags);
	}

	return WriteCacheFile(FileName, IndexFile, CacheType::Archive);
}

void lcPiecesLibrary::RequestPieceDataLoad(lcPieceData* PieceData, lcLoadPriority LoadPriority)
{
	Q_ASSERT(QThread::currentThread() == thread());

	if (PieceData->IsProject() || (PieceData->IsModel() && !PieceData->GetModel()->HasDirectGeometry()))
	{
		PieceData->mState = lcAssetState::Loaded;
		ClearPieceDataLoadError(PieceData);
		return;
	}

	if (PieceData->mState == lcAssetState::Unloaded)
	{
		PieceData->mState = lcAssetState::Loading;
		ClearPieceDataLoadError(PieceData);

		const lcBoundingBox& Box = GetLoadingMesh()->mBoundingBox;

		if (!PieceData->IsModel())
			PieceData->SetBoundingBox(Box.Min, Box.Max);
		else
		{
			const lcBoundingBox& ModelBox = PieceData->GetBoundingBox();
			PieceData->SetBoundingBox(lcMin(ModelBox.Min, Box.Min), lcMax(ModelBox.Max, Box.Max));
		}
	}

	if (PieceData->IsLoading())
		mAssetLoader->RequestPieceLoad(PieceData, LoadPriority);
}

void lcPiecesLibrary::PieceDataLoadCancelled(lcPieceData* PieceData)
{
	Q_ASSERT(QThread::currentThread() == thread());

	PieceData->mState = lcAssetState::Unloaded;
	ClearPieceDataLoadError(PieceData);
}

void lcPiecesLibrary::NotifyAssetStatusChanged()
{
	emit AssetStatusChanged();
}

bool lcPiecesLibrary::EnsurePieceDataAssetsReady(const lcPieceDataRef& PieceData)
{
	const bool PieceReady = WaitForPieceData(PieceData);

	if (PieceData->IsModel())
		return static_cast<bool>(PieceData->GetModel()->EnsureAssetsReady()) && PieceReady;

	if (PieceData->IsProject())
	{
		const bool ProjectReady = static_cast<bool>(PieceData->GetProject()->EnsureAssetsReady());

		PieceData->GetProject()->UpdatePieceData(PieceData.Get());
		return ProjectReady && PieceReady;
	}

	return PieceReady;
}

bool lcPiecesLibrary::WaitForPieceData(const lcPieceDataRef& PieceDataRef)
{
	Q_ASSERT(QThread::currentThread() == thread());

	if (PieceDataRef->mState == lcAssetState::Unloaded)
		RequestPieceDataLoad(PieceDataRef.Get(), lcLoadPriority::Blocking);

	const bool RequestsSucceeded = mAssetLoader->WaitForPieces({ PieceDataRef.Get() });

	return RequestsSucceeded && PieceDataRef->mState == lcAssetState::Loaded;
}

bool lcPiecesLibrary::WaitForPieceData(const std::vector<lcPieceDataRef>& PieceDataRefs)
{
	Q_ASSERT(QThread::currentThread() == thread());

	std::vector<lcPieceData*> Pieces;
	Pieces.reserve(PieceDataRefs.size());

	for (const lcPieceDataRef& PieceDataRef : PieceDataRefs)
	{
		if (PieceDataRef->mState == lcAssetState::Unloaded)
			RequestPieceDataLoad(PieceDataRef.Get(), lcLoadPriority::Blocking);

		Pieces.push_back(PieceDataRef.Get());
	}

	const bool RequestsSucceeded = mAssetLoader->WaitForPieces(Pieces);

	for (const lcPieceDataRef& PieceDataRef : PieceDataRefs)
		if (PieceDataRef->mState != lcAssetState::Loaded)
			return false;

	return RequestsSucceeded;
}

void lcPiecesLibrary::RequestSynthMesh(lcPiece* Piece)
{
	Q_ASSERT(QThread::currentThread() == thread());

	lcPieceData* PieceData = Piece->mPieceData.Get();
	std::shared_ptr<const lcSynthInfo> SynthInfo = PieceData ? PieceData->GetSynthInfoShared() : nullptr;

	if (!SynthInfo)
	{
		CancelSynthMesh(Piece);
		return;
	}

	mSynthPieces.insert_or_assign(Piece, QString());

	mAssetLoader->RequestSynthMesh(Piece, PieceData, std::move(SynthInfo), Piece->GetControlPoints());
}

void lcPiecesLibrary::CancelSynthMesh(lcPiece* Piece)
{
	Q_ASSERT(QThread::currentThread() == thread());

	mSynthPieces.erase(Piece);

	if (mAssetLoader)
		mAssetLoader->CancelSynthMesh(Piece);
}

bool lcPiecesLibrary::WaitForSynthMeshes(const std::vector<lcPiece*>& Pieces)
{
	Q_ASSERT(QThread::currentThread() == thread());

	const bool RequestsSucceeded = mAssetLoader->WaitForSynthMeshes(Pieces);

	for (const lcPiece* Piece : Pieces)
		if (Piece->mPieceData && Piece->mPieceData->GetSynthInfo() && !Piece->HasGeneratedMesh())
			return false;

	return RequestsSucceeded;
}

QString lcPiecesLibrary::GetSynthMeshError(const lcPiece* Piece) const
{
	Q_ASSERT(QThread::currentThread() == thread());

	const auto It = mSynthPieces.find(const_cast<lcPiece*>(Piece));

	return It != mSynthPieces.end() ? It->second : QString();
}

bool lcPiecesLibrary::PublishPieceDataMesh(lcPieceData* PieceData, std::unique_ptr<lcMesh> Mesh, QString Error)
{
	Q_ASSERT(QThread::currentThread() == thread());

	const bool Loaded = Error.isEmpty();

	if (Loaded)
	{
		if (Mesh)
			PieceData->SetMesh(Mesh.release());

		PieceData->mState = lcAssetState::Loaded;
		ClearPieceDataLoadError(PieceData);
	}
	else
		PieceData->SetFailed(std::move(Error));

	ScheduleBufferRepack();

	return Loaded;
}

bool lcPiecesLibrary::PublishSynthMesh(lcPiece* Piece, std::unique_ptr<lcMesh> Mesh, QString Error)
{
	Q_ASSERT(QThread::currentThread() == thread());

	const bool Loaded = Error.isEmpty() && Mesh;

	if (!Loaded && Error.isEmpty())
		Error = tr("Could not generate geometry for piece '%1'.").arg(QString::fromLatin1(Piece->mPieceData->mFileName));

	Piece->SetGeneratedMesh(Loaded ? Mesh.release() : nullptr);

	const auto It = mSynthPieces.find(Piece);

	if (It != mSynthPieces.end())
		It->second = Loaded ? QString() : std::move(Error);

	return Loaded;
}

void lcPiecesLibrary::PublishTexture(lcTexture* Texture, lcTextureBuildResult Result)
{
	Q_ASSERT(QThread::currentThread() == thread());

	if (!Result.DecodedImage.isNull())
		Texture->SetImage(std::move(Result.DecodedImage), LC_TEXTURE_MIPMAPS);
	else
		Texture->SetFailed(Result.Error, std::move(Result.ErrorDetails));
}

void lcPiecesLibrary::RebuildModelPieceData(lcPieceData* PieceData)
{
	Q_ASSERT(QThread::currentThread() == thread());
	Q_ASSERT(PieceData->IsModel());

	InvalidatePieceDataLoad(PieceData);
	PieceData->ReleaseMesh();
	RequestPieceDataLoad(PieceData, lcLoadPriority::Normal);
}

void lcPiecesLibrary::InvalidatePieceDataLoad(lcPieceData* PieceData)
{
	Q_ASSERT(QThread::currentThread() == thread());

	PieceData->mState = lcAssetState::Unloaded;
	ClearPieceDataLoadError(PieceData);

	if (mAssetLoader)
		mAssetLoader->InvalidatePieceLoad(PieceData);
}

void lcPiecesLibrary::ReleasePieceData(lcPieceData* PieceData)
{
	const int Remaining = mPieceManager->ReleasePieceData(PieceData);

	if (Remaining == 1 && mAssetLoader)
		mAssetLoader->CancelPieceLoad(PieceData);
}

void lcPiecesLibrary::AddPieceDataReference(lcPieceData* PieceData)
{
	mPieceManager->AddPieceDataReference(PieceData);
}

void lcPiecesLibrary::SetPieceDataLoadError(const lcPieceData* PieceData, QString Error)
{
	mPieceManager->SetPieceDataLoadError(PieceData, std::move(Error));
}

QString lcPiecesLibrary::GetPieceDataLoadError(const lcPieceData* PieceData) const
{
	return mPieceManager->GetPieceDataLoadError(PieceData);
}

void lcPiecesLibrary::ClearPieceDataLoadError(const lcPieceData* PieceData)
{
	mPieceManager->ClearPieceDataLoadError(PieceData);
}

lcPieceSourceSnapshot lcPiecesLibrary::SnapshotPieceDataSource(const lcPieceData* PieceData) const
{
	lcPieceSourceSnapshot Source;

	Source.FileName = QString::fromLatin1(PieceData->mFileName);
	Source.LibraryDirectory = mLibraryDir.absolutePath();
	Source.CachePath = QFileInfo(QDir(mCachePath), Source.FileName).absoluteFilePath();
	Source.ZipFileType = PieceData->mZipFileType;
	Source.ZipFileIndex = PieceData->mZipFileIndex;
	Source.StudStyle = mStudStyle;
	Source.StudCylinderColorEnabled = mStudCylinderColorEnabled;

	if (PieceData->IsModel())
	{
		Source.InlineModel = true;
		Source.InlineMeshLines = PieceData->GetModel()->GetFileLines();

		const lcModel* Model = PieceData->GetModel();
		Source.TextureSearchDirectories = Model->GetAssetSearchDirectories();

		if (Model->GetProject())
			Source.EmbeddedData = Model->GetProject()->GetEmbeddedDataLookup();
	}

	return Source;
}

lcPieceBuildResult lcPiecesLibrary::BuildPieceData(const lcPieceSourceSnapshot& Source)
{
	lcPieceBuildResult Result;

	if (mCancelLoading.load())
	{
		Result.Error = tr("Library loading was canceled.");
		return Result;
	}

	if (Source.UnavailableTextures.empty() && !Source.InlineModel && Source.ZipFileType != lcZipFileType::Count && mZipFiles[static_cast<int>(Source.ZipFileType)])
	{
		lcMemFile CacheData;

		if (ReadCacheFile(Source.CachePath, CacheData, CacheType::Archive))
		{
			qint32 Flags;

			if (CacheData.ReadBuffer(&Flags, sizeof(Flags)) == sizeof(Flags) && Flags == MeshCacheSettingsKey(Source.StudStyle, Source.StudCylinderColorEnabled))
			{
				Result.Mesh.reset(new lcMesh);

				if (Result.Mesh->FileLoad(CacheData))
				{
					Result.FromCache = true;
					return Result;
				}

				Result.Mesh.reset();
			}
		}
	}

	lcLibraryMeshData MeshData;
	lcMeshLoader MeshLoader(MeshData, nullptr, lcMeshLoaderFlag::Optimize | lcMeshLoaderFlag::RequireAllIncludes);

	MeshLoader.SetUnavailableTextures(Source.UnavailableTextures);

	if (Source.InlineModel)
	{
		lcMemFile PieceFile;

		for (const QString& Line : Source.InlineMeshLines)
		{
			const QByteArray Buffer = Line.toLatin1();

			PieceFile.WriteBuffer(Buffer.constData(), Buffer.size());
			PieceFile.WriteBuffer("\r\n", 2);
		}

		PieceFile.Seek(0, SEEK_SET);

		if (!MeshLoader.LoadMesh(PieceFile, LC_MESHDATA_SHARED))
		{
			Result.Error = tr("Could not load model geometry '%1'.").arg(Source.FileName);

			if (!MeshLoader.GetError().isEmpty())
				Result.Error += QLatin1Char('\n') + MeshLoader.GetError();

			return Result;
		}

		if (MeshData.IsEmpty())
		{
			Result.EmptyGeometry = true;
			return Result;
		}
	}
	else
	{
		bool Loaded = false;

		if (Source.ZipFileType != lcZipFileType::Count && mZipFiles[static_cast<int>(Source.ZipFileType)])
		{
			lcFileSource FileSource;
			FileSource.ArchiveType = static_cast<int>(Source.ZipFileType);
			FileSource.ArchiveIndex = Source.ZipFileIndex;
			const lcResult<std::unique_ptr<lcFile>> File = OpenFile(FileSource);

			if (File)
				Loaded = MeshLoader.LoadMesh(*File.value(), LC_MESHDATA_SHARED);
		}
		else
		{
			lcFileSource FileSource;
			FileSource.FilePath = QDir(Source.LibraryDirectory).absoluteFilePath(QStringLiteral("parts/") + Source.FileName);
			const lcResult<std::unique_ptr<lcFile>> File = OpenFile(FileSource);

			if (File)
				Loaded = MeshLoader.LoadMesh(*File.value(), LC_MESHDATA_SHARED);

			// Directory loading retries unofficial geometry after an open or parse failure.
			if (mHasUnofficial && !Loaded)
			{
				MeshData.Clear();

				FileSource.FilePath = QDir(Source.LibraryDirectory).absoluteFilePath(QStringLiteral("unofficial/parts/") + Source.FileName);
				const lcResult<std::unique_ptr<lcFile>> UnofficialFile = OpenFile(FileSource);

				if (UnofficialFile)
					Loaded = MeshLoader.LoadMesh(*UnofficialFile.value(), LC_MESHDATA_SHARED);
			}
		}

		if (mCancelLoading.load())
			Result.Error = tr("Library loading was canceled.");
		else if (!Loaded)
		{
			Result.Error = tr("Could not load piece '%1'.").arg(Source.FileName);

			if (!MeshLoader.GetError().isEmpty())
				Result.Error += QLatin1Char('\n') + MeshLoader.GetError();
		}

		if (!Result.Error.isEmpty())
			return Result;
	}

	Result.Mesh.reset(MeshData.CreateMesh(nullptr));

	if (!Result.Mesh)
		Result.Error = tr("Could not prepare piece '%1' for rendering.").arg(Source.FileName);

	return Result;
}

void lcPiecesLibrary::SaveBuiltPieceDataCache(const lcPieceSourceSnapshot& Source, lcMesh& Mesh)
{
	if (Source.ZipFileType == lcZipFileType::Count)
		return;

	lcMemFile MeshData;

	const qint32 Flags = MeshCacheSettingsKey(Source.StudStyle, Source.StudCylinderColorEnabled);

	if (MeshData.WriteBuffer((char*)&Flags, sizeof(Flags)) != sizeof(Flags) || !Mesh.FileSave(MeshData))
		return;

	WriteCacheFile(Source.CachePath, MeshData, CacheType::Archive);
}

lcResult<lcOpenedLibraryFile> lcPiecesLibrary::OpenPrimitiveFile(const lcLibraryPrimitive* Primitive)
{
	lcResult<std::unique_ptr<lcFile>> File = OpenFile(Primitive->mSource);

	if (!File)
		return lcUnexpected(File.error());

	return lcOpenedLibraryFile{ std::move(File.value()), Primitive->mSource };
}

lcResult<lcOpenedLibraryFile> lcPiecesLibrary::OpenPieceDataFile(const char* PieceName)
{
	const std::optional<lcPieceManager::FileInfo> FileInfo = mPieceManager->GetPieceDataFileInfo(PieceName);

	if (mZipFiles[static_cast<int>(lcZipFileType::Official)])
	{
		if (FileInfo && FileInfo->ZipFileType != lcZipFileType::Count)
		{
			lcFileSource Source;
			Source.ArchiveType = static_cast<int>(FileInfo->ZipFileType);
			Source.ArchiveIndex = FileInfo->ZipFileIndex;
			lcResult<std::unique_ptr<lcFile>> File = OpenFile(Source);

			if (!File)
				return lcUnexpected(File.error());

			return lcOpenedLibraryFile{ std::move(File.value()), Source };
		}

		if (!FileInfo)
		{
			for (const lcZipFileType Type : { lcZipFileType::Official, lcZipFileType::Unofficial })
			{
				const std::unique_ptr<lcZipFile>& Archive = mZipFiles[static_cast<int>(Type)];

				if (!Archive)
					continue;

				const QString Prefix = Type == lcZipFileType::Official ? QStringLiteral("ldraw/") : QString();

				for (const QString& Folder : { QStringLiteral("parts/"), QStringLiteral("p/") })
				{
					const QByteArray FileName = (Prefix + Folder + QString::fromLatin1(PieceName)).toLatin1();

					const std::optional<quint32> FileIndex = Archive->FindFileIndex(FileName.constData());

					if (!FileIndex)
						continue;

					lcFileSource Source;
					Source.ArchiveType = static_cast<int>(Type);
					Source.ArchiveIndex = *FileIndex;

					if (lcResult<std::unique_ptr<lcFile>> File = OpenFile(Source))
						return lcOpenedLibraryFile{ std::move(File.value()), Source };
				}
			}

			return lcUnexpected(tr("Could not open included file '%1'.").arg(QString::fromLatin1(PieceName)));
		}
	}

	const QString Name = QString::fromLatin1(FileInfo ? FileInfo->FileName.c_str() : PieceName);
	std::vector<QString> Folders(1, QStringLiteral("parts/"));

	if (!FileInfo)
		Folders.emplace_back(QStringLiteral("p/"));

	if (mHasUnofficial)
	{
		Folders.emplace_back(QStringLiteral("unofficial/parts/"));

		if (!FileInfo)
			Folders.emplace_back(QStringLiteral("unofficial/p/"));
	}

	for (const QString& Folder : Folders)
	{
		const QString FileName = Folder + Name;
		lcFileSource Source;
		Source.FilePath = mLibraryDir.absoluteFilePath(FileName);

		if (lcResult<std::unique_ptr<lcFile>> File = OpenFile(Source))
		{
			Source.FilePath = QFileInfo(Source.FilePath).canonicalFilePath();
			return lcOpenedLibraryFile{ std::move(File.value()), Source };
		}

#if defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
		if (!FileInfo)
		{
			Source.FilePath = mLibraryDir.absoluteFilePath(FileName.toLower());

			if (lcResult<std::unique_ptr<lcFile>> File = OpenFile(Source))
			{
				Source.FilePath = QFileInfo(Source.FilePath).canonicalFilePath();
				return lcOpenedLibraryFile{ std::move(File.value()), Source };
			}
		}
#endif
	}

	return lcUnexpected(tr("Could not open included file '%1'.").arg(QString::fromLatin1(PieceName)));
}

void lcPiecesLibrary::ReleaseBuffers()
{
	mBuffersDirty = true;

	// Library-only loading can finish without creating a renderer or shared buffers.
	if (!mVertexBuffer.IsValid() && !mIndexBuffer.IsValid())
		return;

	lcContext* Context = lcContext::GetGlobalOffscreenContext();

	Context->MakeCurrent();
	Context->DestroyVertexBuffer(mVertexBuffer);
	Context->DestroyIndexBuffer(mIndexBuffer);
}

void lcPiecesLibrary::UpdateBuffers(lcContext* Context)
{
	if (!gSupportsVertexBufferObject || !mBuffersDirty)
		return;

	int VertexDataSize = 0;
	int IndexDataSize = 0;
	std::vector<lcMesh*> Meshes;

	const auto AddMesh = [&VertexDataSize, &IndexDataSize, &Meshes](lcPieceData* PieceData)
	{
		lcMesh* Mesh = PieceData->GetMesh();

		if (!Mesh)
			return;

		if (Mesh->mVertexDataSize < 0 || Mesh->mIndexDataSize < 0 ||
			Mesh->mVertexDataSize > 16 * 1024 * 1024 || Mesh->mIndexDataSize > 16 * 1024 * 1024 ||
			(Mesh->mVertexDataSize && !Mesh->mVertexData) || (Mesh->mIndexDataSize && !Mesh->mIndexData) ||
			VertexDataSize > std::numeric_limits<int>::max() - Mesh->mVertexDataSize ||
			IndexDataSize > std::numeric_limits<int>::max() - Mesh->mIndexDataSize)
		{
			Mesh->mVertexCacheOffset = -1;
			Mesh->mIndexCacheOffset = -1;
			return;
		}

		VertexDataSize += Mesh->mVertexDataSize;
		IndexDataSize += Mesh->mIndexDataSize;

		Meshes.push_back(Mesh);
	};

	for (lcPieceData* PieceData : mPieceManager->GetAllPieceData())
		AddMesh(PieceData);

	if (!VertexDataSize)
	{
		Context->DestroyVertexBuffer(mVertexBuffer);
		Context->DestroyIndexBuffer(mIndexBuffer);
		mBuffersDirty = false;

		return;
	}

	void* VertexData = malloc(VertexDataSize);
	void* IndexData = IndexDataSize ? malloc(IndexDataSize) : nullptr;

	if (!VertexData || (IndexDataSize && !IndexData))
	{
		free(VertexData);
		free(IndexData);

		return;
	}

	VertexDataSize = 0;
	IndexDataSize = 0;

	for (lcMesh* Mesh : Meshes)
	{
		if (Mesh->mIndexDataSize && !IndexData)
		{
			for (lcMesh* CachedMesh : Meshes)
			{
				CachedMesh->mVertexCacheOffset = -1;
				CachedMesh->mIndexCacheOffset = -1;
			}

			free(VertexData);
			free(IndexData);
			return;
		}

		Mesh->mVertexCacheOffset = VertexDataSize;
		Mesh->mIndexCacheOffset = IndexDataSize;

		if (Mesh->mVertexDataSize)
			memcpy((char*)VertexData + VertexDataSize, Mesh->mVertexData, Mesh->mVertexDataSize);

		if (Mesh->mIndexDataSize)
			memcpy((char*)IndexData + IndexDataSize, Mesh->mIndexData, Mesh->mIndexDataSize);

		VertexDataSize += Mesh->mVertexDataSize;
		IndexDataSize += Mesh->mIndexDataSize;
	}

	Context->DestroyVertexBuffer(mVertexBuffer);
	Context->DestroyIndexBuffer(mIndexBuffer);

	mVertexBuffer = Context->CreateVertexBuffer(VertexDataSize, VertexData);
	if (IndexDataSize)
		mIndexBuffer = Context->CreateIndexBuffer(IndexDataSize, IndexData);
	mBuffersDirty = false;

	free(VertexData);
	free(IndexData);
}

void lcPiecesLibrary::ScheduleBufferRepack()
{
	mStreamingBuffersPending = true;

	if (mBufferRepackScheduled)
		return;

	mBufferRepackScheduled = true;

	QTimer::singleShot(120, this, [this]()
	{
		mBufferRepackScheduled = false;

		if (!mStreamingBuffersPending)
			return;

		if (mAssetLoader && mAssetLoader->HasPendingWork())
		{
			ScheduleBufferRepack();
			return;
		}

		mStreamingBuffersPending = false;
		mBuffersDirty = true;
		lcView::UpdateAllViews();
	});
}

lcMesh* lcPiecesLibrary::GetLoadingMesh() const
{
	return mLoadingMesh.get();
}

void lcPiecesLibrary::AddTextureReference(lcTexture* Texture)
{
	mTextureManager->AddTextureReference(Texture);
}

void lcPiecesLibrary::ReleaseTexture(lcTexture* Texture)
{
	mTextureManager->ReleaseTexture(Texture);
}

void lcPiecesLibrary::UpdateStudStyleAndColors(lcStudStyle StudStyle, bool StudCylinderColorEnabled)
{
	Q_ASSERT(QThread::currentThread() == thread());

	if (mStudStyle == StudStyle && mStudCylinderColorEnabled == StudCylinderColorEnabled)
		return;

	// Finish running workers before changing shared primitive data and colors.
	// Queued requests resume with the new settings afterward.
	mAssetLoader->PauseQueuedWork();

	mStudStyle = StudStyle;
	mStudCylinderColorEnabled = StudCylinderColorEnabled;

	LoadColors();
	UpdateStudStyleSource();

	for (const std::unique_ptr<lcLibrarySource>& Source : mSources)
	{
		for (const auto& PrimitiveIt : Source->Primitives)
		{
			lcLibraryPrimitive* Primitive = PrimitiveIt.second.get();

			if (Primitive->mStudStyle || Primitive->mMeshData.mHasStyleStud)
				Primitive->Unload();
		}
	}

	const auto ReloadPiece = [this](lcPieceData* PieceData)
	{
		if (PieceData->mState != lcAssetState::Loaded || !PieceData->GetMesh() || !(PieceData->GetMesh()->mFlags & lcMeshFlag::HasStyleStud))
			return;

		if (PieceData->IsModel())
		{
			PieceData->ReleaseMesh();
			PieceData->mState = lcAssetState::Unloaded;

			RequestPieceDataLoad(PieceData, lcLoadPriority::Low);
		}
		else if (!PieceData->IsProject())
		{
			PieceData->Unload();
			RequestPieceDataLoad(PieceData, lcLoadPriority::Low);
		}

		mBuffersDirty = true;
	};

	for (lcPieceData* PieceData : mPieceManager->GetAllPieceData())
		ReloadPiece(PieceData);

	std::vector<lcPiece*> SynthPieces;
	SynthPieces.reserve(mSynthPieces.size());

	for (const auto& Entry : mSynthPieces)
		SynthPieces.push_back(Entry.first);

	for (lcPiece* Piece : SynthPieces)
		Piece->RefreshGeneratedMesh();

	mAssetLoader->ResumeQueuedWork();
}

lcLibraryPrimitive* lcPiecesLibrary::FindPrimitive(const char* Name) const
{
	for (const std::unique_ptr<lcLibrarySource>& Source : mSources)
	{
		const auto PrimitiveIt = Source->Primitives.find(Name);

		if (PrimitiveIt != Source->Primitives.end())
			return PrimitiveIt->second.get();
	}

	return nullptr;
}

lcResult<void> lcPiecesLibrary::LoadPrimitive(lcLibraryPrimitive* Primitive)
{
	QThread* Thread = QThread::currentThread();
	lcLibraryPrimitive* Parent = nullptr;

	mPrimitiveMutex.lock();

	if (ShouldCancelLoading())
	{
		mPrimitiveMutex.unlock();
		return lcUnexpected(tr("Library loading was canceled."));
	}

	if (Primitive->mState == lcPrimitiveState::Loaded)
	{
		mPrimitiveMutex.unlock();
		return lcResult<void>();
	}

	std::vector<lcLibraryPrimitive*>& Stack = mPrimitiveLoadStacks[Thread];

	if (!Stack.empty())
	{
		Parent = Stack.back();

		for (lcLibraryPrimitive* Dependency = Primitive; Dependency;)
		{
			if (Dependency == Parent)
			{
				mPrimitiveMutex.unlock();
				return lcUnexpected(tr("Primitive include cycle at '%1'.").arg(QString::fromLatin1(Primitive->mName)));
			}

			const auto It = mPrimitiveDependencies.find(Dependency);
			Dependency = It != mPrimitiveDependencies.end() ? It->second : nullptr;
		}

		mPrimitiveDependencies[Parent] = Primitive;
	}

	Stack.push_back(Primitive);
	mPrimitiveMutex.unlock();

	lcResult<void> Result = LoadPrimitiveData(Primitive);

	mPrimitiveMutex.lock();

	if (Parent)
		mPrimitiveDependencies.erase(Parent);

	std::vector<lcLibraryPrimitive*>& CompletedStack = mPrimitiveLoadStacks[Thread];
	CompletedStack.pop_back();

	if (CompletedStack.empty())
		mPrimitiveLoadStacks.erase(Thread);

	mPrimitiveMutex.unlock();

	return Result;
}

lcResult<void> lcPiecesLibrary::LoadPrimitiveData(lcLibraryPrimitive* Primitive)
{
	mPrimitiveMutex.lock();

	if (ShouldCancelLoading())
	{
		mPrimitiveMutex.unlock();
		return lcUnexpected(tr("Library loading was canceled."));
	}

	if (Primitive->mState == lcPrimitiveState::NotLoaded)
		Primitive->mState = lcPrimitiveState::Loading;
	else
	{
		while (Primitive->mState == lcPrimitiveState::Loading)
		{
			if (ShouldCancelLoading())
			{
				mPrimitiveMutex.unlock();
				return lcUnexpected(tr("Library loading was canceled."));
			}

			mPrimitiveLoaded.wait(&mPrimitiveMutex, 10);
		}

		const bool Loaded = Primitive->mState == lcPrimitiveState::Loaded;
		const QString Error = Primitive->mLoadError;
		mPrimitiveMutex.unlock();

		if (Loaded)
			return lcResult<void>();

		return lcUnexpected(Error);
	}

	mPrimitiveMutex.unlock();

	lcMeshLoader MeshLoader(Primitive->mMeshData, nullptr, lcMeshLoaderFlag::Optimize | lcMeshLoaderFlag::RequireAllIncludes);

	const auto LoadFailed = [this, Primitive](QString Error) -> lcResult<void>
	{
		Primitive->mMeshData.Clear();

		if (Error.isEmpty())
			Error = tr("Could not load included file '%1'.").arg(QString::fromLatin1(Primitive->mName));

		QMutexLocker LoadLock(&mPrimitiveMutex);
		Primitive->mLoadError = Error;
		Primitive->mState = lcPrimitiveState::NotLoaded;
		mPrimitiveLoaded.wakeAll();
		return lcUnexpected(std::move(Error));
	};

	lcLibraryPrimitive* LowPrimitive = nullptr;

	if (mZipFiles[static_cast<int>(lcZipFileType::Official)] && Primitive->mStud && !Primitive->mStudStyle)
	{
		if (strncmp(Primitive->mName, "8/", 2)) // todo: low LOD for loose file libraries
		{
			char Name[LC_PIECE_NAME_LEN];
			lcstrcpy(Name, "8/");
			lcstrcat(Name, Primitive->mName);
			lcstrupr(Name);

			LowPrimitive = FindPrimitive(Name); // todo: low primitives don't work with studlogo
		}
	}

	const lcResult<lcOpenedLibraryFile> File = OpenPrimitiveFile(Primitive);

	if (!File)
		return LoadFailed(File.error());

	if (!MeshLoader.LoadMesh(*File.value().File, LowPrimitive ? LC_MESHDATA_HIGH : LC_MESHDATA_SHARED))
		return LoadFailed(MeshLoader.GetError());

	if (LowPrimitive)
	{
		const lcResult<lcOpenedLibraryFile> LowFile = OpenPrimitiveFile(LowPrimitive);

		if (!LowFile)
			return LoadFailed(LowFile.error());

		if (!MeshLoader.LoadMesh(*LowFile.value().File, LC_MESHDATA_LOW))
			return LoadFailed(MeshLoader.GetError());
	}

	mPrimitiveMutex.lock();
	Primitive->mLoadError.clear();
	Primitive->mState = lcPrimitiveState::Loaded;
	mPrimitiveLoaded.wakeAll();
	mPrimitiveMutex.unlock();

	return lcResult<void>();
}

bool lcPiecesLibrary::PieceDataInCategory(lcPieceData* PieceData, const char* CategoryKeywords) const
{
	if (!PieceData->IsLibraryPiece())
		return false;

	const char* PieceName;
	if (PieceData->m_strDescription[0] == '~' || PieceData->m_strDescription[0] == '_')
		PieceName = PieceData->m_strDescription + 1;
	else
		PieceName = PieceData->m_strDescription;

	return lcMatchCategory(PieceName, CategoryKeywords);
}

void lcPiecesLibrary::GetCategoryEntries(int CategoryIndex, bool GroupPieces, std::vector<lcPieceData*>& SinglePieces, std::vector<lcPieceData*>& GroupedPieces)
{
	if (CategoryIndex >= 0 && CategoryIndex < static_cast<int>(gCategories.size()))
		GetCategoryEntries(gCategories[CategoryIndex].Keywords.constData(), GroupPieces, SinglePieces, GroupedPieces);
}

void lcPiecesLibrary::GetCategoryEntries(const char* CategoryKeywords, bool GroupPieces, std::vector<lcPieceData*>& SinglePieces, std::vector<lcPieceData*>& GroupedPieces)
{
	SinglePieces.clear();
	GroupedPieces.clear();

	for (lcPieceData* PieceData : mPieceManager->GetLibraryPieceData())
	{
		if (!PieceDataInCategory(PieceData, CategoryKeywords))
			continue;

		if (!GroupPieces)
		{
			SinglePieces.emplace_back(PieceData);
			continue;
		}

		// Check if it's a patterned piece.
		if (PieceData->IsDecorated())
		{
			lcPieceData* Parent = FindPieceData(PieceData->GetPatternParentId().c_str(), nullptr);

			if (Parent)
			{
				// Check if the parent was added as a single piece.
				auto ParentIt = std::find(SinglePieces.begin(), SinglePieces.end(), Parent);

				if (ParentIt != SinglePieces.end())
					SinglePieces.erase(ParentIt);

				if (std::find(GroupedPieces.begin(), GroupedPieces.end(), Parent) == GroupedPieces.end())
					GroupedPieces.emplace_back(Parent);
			}
			else
			{
				// Decorations without a plain parent remain individual entries.
				SinglePieces.emplace_back(PieceData);
			}
		}
		else
		{
			// Check if this piece has already been added to this category by one of its children.
			if (std::find(GroupedPieces.begin(), GroupedPieces.end(), PieceData) == GroupedPieces.end())
				SinglePieces.emplace_back(PieceData);
		}
	}
}

std::vector<lcPieceData*> lcPiecesLibrary::GetParts() const
{
	return mPieceManager->GetLibraryPieceData();
}

std::vector<lcPieceData*> lcPiecesLibrary::GetVisibleTrainTrackParts(const lcTrainTrackConnectionType& ConnectionType) const
{
	std::vector<lcPieceData*> Parts;

	for (lcPieceData* PieceData : mPieceManager->GetLibraryPieceData())
	{
		lcTrainTrackInfo* TrainTrackInfo = PieceData->GetTrainTrackInfo();

		if (TrainTrackInfo && TrainTrackInfo->IsVisible() && TrainTrackInfo->CanConnectTo(ConnectionType, true))
			Parts.emplace_back(PieceData);
	}

	return Parts;
}

std::vector<lcPieceData*> lcPiecesLibrary::GetPartsFromSet(const std::vector<std::string>& PartIds) const
{
	std::vector<lcPieceData*> Parts;
	Parts.reserve(PartIds.size());

	for (const std::string& PartId : PartIds)
	{
		if (lcPieceData* PieceData = mPieceManager->FindPieceData(PartId, nullptr))
			Parts.push_back(PieceData);
	}

	return Parts;
}

std::string lcPiecesLibrary::GetPartId(const lcPieceData* PieceData) const
{
	return mPieceManager->GetLibraryPieceDataName(PieceData);
}

bool lcPiecesLibrary::LoadBuiltinPieceLibrary()
{
	std::unique_ptr<lcDiskFile> File(new lcDiskFile(":/resources/library.zip"));

	if (!File->Open(QIODevice::ReadOnly) || !OpenArchive(std::move(File), lcZipFileType::Official))
		return false;

	lcMemFile PieceFile;

	for (lcPieceData* PieceData : mPieceManager->GetLibraryPieceData())
	{
		mZipFiles[static_cast<int>(PieceData->mZipFileType)]->ExtractFile(PieceData->mZipFileIndex, PieceFile, 256);
		PieceFile.Seek(0, SEEK_END);
		PieceFile.WriteU8(0);

		char* Src = (char*)PieceFile.mBuffer + 2;
		char* Dst = PieceData->m_strDescription;

		for (;;)
		{
			if (*Src != '\r' && *Src != '\n' && *Src && Dst - PieceData->m_strDescription < (int)sizeof(PieceData->m_strDescription) - 1)
			{
				*Dst++ = *Src++;
				continue;
			}

			*Dst = 0;
			break;
		}
	}

	ClassifyPieces(DirectoryFilePaths());

	lcLoadDefaultColors(mStudStyle);
	UpdateLoadingMeshColors();
	UpdateStudStyleSource();
	lcLoadDefaultCategories(true);
	lcSynthInit();
	lcTrainTrackInfo::Initialize(this);

	return true;
}
