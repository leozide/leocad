#include "lc_global.h"
#include "lc_piecemanager.h"
#include "lc_piecedata.h"
#include "lc_string.h"
#include "project.h"

lcPieceManager::~lcPieceManager()
{
	// Library teardown must unregister identities before destroying this registry.
	Q_ASSERT(mPieces.empty() && mProjectPieces.empty() && mPieceDataLoadErrors.empty());
}

std::string lcPieceManager::NormalizePieceDataName(const char* PieceName)
{
	std::string Name;

	for (const char* Src = PieceName; *Src && Name.size() < LC_PIECE_NAME_LEN - 1; Src++)
	{
		if (*Src == '\\')
			Name.push_back('/');
		else if (*Src >= 'a' && *Src <= 'z')
			Name.push_back(*Src + 'A' - 'a');
		else
			Name.push_back(*Src);
	}

	return Name;
}

void lcPieceManager::Clear()
{
	// Workers have drained. External project destruction may reenter through the facade.
	std::unordered_set<Project*> ExternalProjects;
	mMutex.lock();

	for (const auto& [PieceData, Owner] : mProjectPieces)
		if (PieceData->IsProject())
			ExternalProjects.insert(PieceData->GetProject());

	mMutex.unlock();

	for (;;)
	{
		mMutex.lock();

		const auto External = std::find_if(mProjectPieces.begin(), mProjectPieces.end(), [&ExternalProjects](const auto& Entry)
		{
			return Entry.first->IsProject() && ExternalProjects.find(Entry.second) == ExternalProjects.end();
		});
		lcPieceData* PieceData = nullptr;

		if (External != mProjectPieces.end())
			PieceData = External->first;
		else if (!mProjectPieces.empty())
			PieceData = mProjectPieces.begin()->first;
		else if (!mPieces.empty())
			PieceData = mPieces.begin()->second;

		mMutex.unlock();

		if (!PieceData)
			break;

		RemovePieceData(PieceData);
	}

	mErrorMutex.lock();
	mPieceDataLoadErrors.clear();
	mErrorMutex.unlock();
}

lcPieceData* lcPieceManager::FindPieceData(const std::string& Name, Project* OwnerProject) const
{
	QMutexLocker Lock(&mMutex);

	if (OwnerProject)
		if (lcPieceData* PieceData = OwnerProject->FindPieceData(Name))
			return PieceData;

	const auto PieceIt = mPieces.find(Name);

	return PieceIt != mPieces.end() ? PieceIt->second : nullptr;
}

lcPieceData* lcPieceManager::CreateLibraryPieceData(const std::string& Name, const char* FileName)
{
	QMutexLocker Lock(&mMutex);

	lcPieceData* PieceData = new lcPieceData(true);

	lcstrcpy(PieceData->mFileName, FileName);

	RegisterLibraryPieceData(Name, PieceData);

	return PieceData;
}

lcPieceData* lcPieceManager::CreatePieceData(const char* PieceName, Project* OwnerProject)
{
	QMutexLocker Lock(&mMutex);

	lcPieceData* PieceData = new lcPieceData(false);
	const std::string Name = NormalizePieceDataName(PieceName);

	PieceData->CreatePart(PieceName);

	if (OwnerProject)
		RegisterProjectPieceData(OwnerProject, Name, PieceData);
	else
		RegisterLibraryPieceData(Name, PieceData);

	return PieceData;
}

lcPieceData* lcPieceManager::AcquireModelPieceData(const char* PieceName, Project* OwnerProject, bool& Reused)
{
	QMutexLocker Lock(&mMutex);

	lcPieceData* PieceData = OwnerProject->FindPieceData(NormalizePieceDataName(PieceName));

	Reused = PieceData && !PieceData->IsModel();

	if (!Reused)
		PieceData = CreatePieceData(PieceName, OwnerProject);

	// Keep the identity alive through invalidation and adoption by the model.
	AddPieceDataReference(PieceData);

	return PieceData;
}

void lcPieceManager::AddPieceDataReference(lcPieceData* PieceData)
{
	QMutexLocker Lock(&mMutex);

	PieceData->AddRef();
}

int lcPieceManager::ReleasePieceData(lcPieceData* PieceData)
{
	QMutexLocker Lock(&mMutex);

	if (PieceData->GetRefCount() == 0)
		return 0;

	const int Remaining = PieceData->Release();

	Lock.unlock();

	if (Remaining == 0)
		UnloadPieceData(PieceData);

	return Remaining;
}

void lcPieceManager::RemoveTemporaryPieceData()
{
	// Requery after each deletion, since external projects can release other identities.
	for (;;)
	{
		mMutex.lock();
		lcPieceData* Unused = nullptr;

		for (const auto& [Name, PieceData] : mPieces)
			if (!PieceData->IsLibraryPiece() && PieceData->GetRefCount() == 0)
			{
				Unused = PieceData;
				break;
			}

		if (!Unused)
			for (const auto& [PieceData, Owner] : mProjectPieces)
				if (PieceData->GetRefCount() == 0)
				{
					Unused = PieceData;
					break;
				}

		mMutex.unlock();

		if (!Unused)
			break;

		RemovePieceData(Unused);
	}
}

void lcPieceManager::RemovePieceData(lcPieceData* PieceData)
{
	mMutex.lock();
	UnregisterPieceData(PieceData);
	mMutex.unlock();

	delete PieceData;
}

void lcPieceManager::UnloadPieceData(lcPieceData* PieceData)
{
	if (PieceData->IsLibraryPiece())
		PieceData->Unload();
	else
		RemovePieceData(PieceData);
}

void lcPieceManager::RegisterLibraryPieceData(const std::string& Name, lcPieceData* PieceData)
{
	mPieces[Name] = PieceData;
}

void lcPieceManager::RegisterProjectPieceData(Project* OwnerProject, const std::string& Name, lcPieceData* PieceData)
{
	OwnerProject->RegisterPieceData(Name, PieceData);

	mProjectPieces[PieceData] = OwnerProject;
}

void lcPieceManager::UnregisterPieceData(lcPieceData* PieceData)
{
	const auto Local = mProjectPieces.find(PieceData);

	if (Local != mProjectPieces.end())
	{
		if (Local->second)
			Local->second->UnregisterPieceData(PieceData);

		mProjectPieces.erase(Local);
	}
	else
	{
		const auto PieceIt = std::find_if(mPieces.begin(), mPieces.end(), [PieceData](const auto& Entry)
		{
			return Entry.second == PieceData;
		});

		if (PieceIt != mPieces.end())
			mPieces.erase(PieceIt);
	}
}

bool lcPieceManager::RenameModelPieceData(lcPieceData* PieceData, const char* NewName)
{
	QMutexLocker Lock(&mMutex);

	const std::string Name = NormalizePieceDataName(NewName);
	const auto Local = mProjectPieces.find(PieceData);

	if (Local != mProjectPieces.end())
	{
		if (Project* OwnerProject = Local->second)
		{
			lcPieceData* Existing = OwnerProject->FindPieceData(Name);

			if (Existing && Existing != PieceData)
				return false;

			OwnerProject->UnregisterPieceData(PieceData);
			OwnerProject->RegisterPieceData(Name, PieceData);
		}
	}
	else
	{
		lcPieceData* Existing = FindPieceData(Name, nullptr);

		if (Existing && Existing != PieceData)
			return false;

		UnregisterPieceData(PieceData);
		RegisterLibraryPieceData(Name, PieceData);
	}

	lcstrcpy(PieceData->mFileName, NewName);
	lcstrcpy(PieceData->m_strDescription, NewName);

	return true;
}

void lcPieceManager::ReleaseProjectPieceData(Project* OwnerProject)
{
	QMutexLocker Lock(&mMutex);

	std::vector<lcPieceData*> Unused;

	OwnerProject->ClearPieceDataIndex();

	for (auto& [PieceData, Owner] : mProjectPieces)
	{
		if (Owner == OwnerProject)
		{
			Owner = nullptr;

			if (PieceData->GetRefCount() == 0)
				Unused.push_back(PieceData);
		}
	}

	Lock.unlock();

	for (lcPieceData* PieceData : Unused)
		UnloadPieceData(PieceData);
}

void lcPieceManager::TransferProjectPieceData(Project* Source, Project* Destination)
{
	QMutexLocker Lock(&mMutex);

	std::vector<lcPieceData*> Unused;

	Source->TransferPieceDataIndexTo(Destination);

	for (auto& [PieceData, Owner] : mProjectPieces)
	{
		if (Owner == Source)
		{
			Owner = Destination;

			if (PieceData->GetRefCount() == 0)
				Unused.push_back(PieceData);
		}
	}

	Lock.unlock();

	for (lcPieceData* PieceData : Unused)
		UnloadPieceData(PieceData);
}

size_t lcPieceManager::GetLibraryPieceDataCount() const
{
	QMutexLocker Lock(&mMutex);

	return mPieces.size();
}

std::string lcPieceManager::GetLibraryPieceDataName(const lcPieceData* PieceData) const
{
	QMutexLocker Lock(&mMutex);

	const auto PieceIt = std::find_if(mPieces.begin(), mPieces.end(), [PieceData](const auto& Entry)
	{
		return Entry.second == PieceData;
	});

	return PieceIt != mPieces.end() ? PieceIt->first : std::string();
}

std::vector<lcPieceData*> lcPieceManager::GetLibraryPieceData() const
{
	QMutexLocker Lock(&mMutex);

	std::vector<lcPieceData*> LibraryPieceData;

	LibraryPieceData.reserve(mPieces.size());

	for (const auto& [Name, LibraryData] : mPieces)
		LibraryPieceData.push_back(LibraryData);

	return LibraryPieceData;
}

std::vector<lcPieceData*> lcPieceManager::GetAllPieceData() const
{
	QMutexLocker Lock(&mMutex);

	std::vector<lcPieceData*> AllPieceData = GetLibraryPieceData();
	AllPieceData.reserve(AllPieceData.size() + mProjectPieces.size());

	for (const auto& [LocalData, Owner] : mProjectPieces)
		AllPieceData.push_back(LocalData);

	return AllPieceData;
}

std::optional<lcPieceManager::FileInfo> lcPieceManager::GetPieceDataFileInfo(const std::string& Name) const
{
	QMutexLocker Lock(&mMutex);

	const lcPieceData* PieceData = FindPieceData(Name, nullptr);

	if (!PieceData)
		return std::nullopt;

	return FileInfo{ PieceData->mFileName, PieceData->mZipFileType, PieceData->mZipFileIndex };
}

void lcPieceManager::SetPieceDataLoadError(const lcPieceData* PieceData, QString Error)
{
	QMutexLocker ErrorLock(&mErrorMutex);

	if (Error.isEmpty())
		mPieceDataLoadErrors.erase(PieceData);
	else
		mPieceDataLoadErrors.insert_or_assign(PieceData, std::move(Error));
}

QString lcPieceManager::GetPieceDataLoadError(const lcPieceData* PieceData) const
{
	QMutexLocker ErrorLock(&mErrorMutex);

	const auto It = mPieceDataLoadErrors.find(PieceData);

	return It == mPieceDataLoadErrors.end() ? QString() : It->second;
}

void lcPieceManager::ClearPieceDataLoadError(const lcPieceData* PieceData)
{
	QMutexLocker ErrorLock(&mErrorMutex);

	mPieceDataLoadErrors.erase(PieceData);
}
