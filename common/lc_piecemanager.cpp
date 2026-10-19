#include "lc_global.h"
#include "lc_piecemanager.h"
#include "lc_piecedata.h"
#include "project.h"

lcPieceManager::~lcPieceManager()
{
	// Library teardown must unregister identities before destroying this registry.
	Q_ASSERT(mPieces.empty() && mProjectPieces.empty());
}

lcPieceData* lcPieceManager::FindPieceData(const std::string& Name, Project* OwnerProject) const
{
	if (OwnerProject)
		if (lcPieceData* PieceData = OwnerProject->FindPieceData(Name))
			return PieceData;

	return FindLibraryPieceData(Name);
}

lcPieceData* lcPieceManager::FindLibraryPieceData(const std::string& Name) const
{
	const auto PieceIt = mPieces.find(Name);

	return PieceIt != mPieces.end() ? PieceIt->second : nullptr;
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

bool lcPieceManager::RenameModelPieceData(lcPieceData* PieceData, const std::string& Name)
{
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
		lcPieceData* Existing = FindLibraryPieceData(Name);

		if (Existing && Existing != PieceData)
			return false;

		UnregisterPieceData(PieceData);
		RegisterLibraryPieceData(Name, PieceData);
	}

	return true;
}

std::vector<lcPieceData*> lcPieceManager::ReleaseProjectPieceData(Project* OwnerProject)
{
	std::vector<lcPieceData*> ProjectPieceData;

	OwnerProject->ClearPieceDataIndex();

	for (auto& [LocalData, Owner] : mProjectPieces)
	{
		if (Owner == OwnerProject)
		{
			Owner = nullptr;
			ProjectPieceData.push_back(LocalData);
		}
	}

	return ProjectPieceData;
}

std::vector<lcPieceData*> lcPieceManager::TransferProjectPieceData(Project* Source, Project* Destination)
{
	std::vector<lcPieceData*> ProjectPieceData;

	Source->TransferPieceDataIndexTo(Destination);

	for (auto& [LocalData, Owner] : mProjectPieces)
	{
		if (Owner == Source)
		{
			Owner = Destination;
			ProjectPieceData.push_back(LocalData);
		}
	}

	return ProjectPieceData;
}

size_t lcPieceManager::GetLibraryPieceDataCount() const
{
	return mPieces.size();
}

std::string lcPieceManager::GetLibraryPieceDataName(const lcPieceData* PieceData) const
{
	const auto PieceIt = std::find_if(mPieces.begin(), mPieces.end(), [PieceData](const auto& Entry)
	{
		return Entry.second == PieceData;
	});

	return PieceIt != mPieces.end() ? PieceIt->first : std::string();
}

std::vector<lcPieceData*> lcPieceManager::GetLibraryPieceData() const
{
	std::vector<lcPieceData*> LibraryPieceData;
	LibraryPieceData.reserve(mPieces.size());

	for (const auto& [Name, LibraryData] : mPieces)
		LibraryPieceData.push_back(LibraryData);

	return LibraryPieceData;
}

std::vector<std::pair<lcPieceData*, Project*>> lcPieceManager::GetProjectPieceData() const
{
	return std::vector<std::pair<lcPieceData*, Project*>>(mProjectPieces.begin(), mProjectPieces.end());
}

lcPieceData* lcPieceManager::GetFirstProjectPieceData() const
{
	return mProjectPieces.empty() ? nullptr : mProjectPieces.begin()->first;
}

std::vector<lcPieceData*> lcPieceManager::GetAllPieceData() const
{
	std::vector<lcPieceData*> AllPieceData = GetLibraryPieceData();
	AllPieceData.reserve(AllPieceData.size() + mProjectPieces.size());

	for (const auto& [LocalData, Owner] : mProjectPieces)
		AllPieceData.push_back(LocalData);

	return AllPieceData;
}

std::optional<lcPieceManager::FileInfo> lcPieceManager::GetPieceDataFileInfo(const std::string& Name) const
{
	const lcPieceData* PieceData = FindLibraryPieceData(Name);

	if (!PieceData)
		return std::nullopt;

	return FileInfo{ PieceData->mFileName, PieceData->mZipFileType, PieceData->mZipFileIndex };
}
