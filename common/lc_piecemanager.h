#pragma once

enum class lcZipFileType;

// Stores and indexes library and project piece registrations.
// The library handles references, source resolution, unloading and deletion.
// Calls use normalized names and the library's existing thread/lock contracts.
class lcPieceManager
{
public:
	lcPieceManager() = default;
	~lcPieceManager();

	lcPieceManager(const lcPieceManager&) = delete;
	lcPieceManager(lcPieceManager&&) = delete;
	lcPieceManager& operator=(const lcPieceManager&) = delete;
	lcPieceManager& operator=(lcPieceManager&&) = delete;

	// Returns a borrowed project identity, falling back to the library. No creation or acquisition.
	lcPieceData* FindPieceData(const std::string& Name, Project* OwnerProject) const;

	// Returns a borrowed library identity, or nullptr. Does not search project registrations.
	lcPieceData* FindLibraryPieceData(const std::string& Name) const;

	// Registers a library identity under its normalized name without acquiring a reference.
	void RegisterLibraryPieceData(const std::string& Name, lcPieceData* PieceData);

	// Registers a local identity and updates its owner's borrowed name index.
	void RegisterProjectPieceData(Project* OwnerProject, const std::string& Name, lcPieceData* PieceData);

	// Removes the registration and any borrowed project index entries. Does not delete the identity.
	void UnregisterPieceData(lcPieceData* PieceData);

	// Changes the index key, rejecting collisions with another identity in the same scope.
	// The library updates metadata under the same lock.
	bool RenameModelPieceData(lcPieceData* PieceData, const std::string& Name);

	// Clears the project's borrowed index and detaches its registrations.
	// Returns affected identities without releasing their references.
	std::vector<lcPieceData*> ReleaseProjectPieceData(Project* OwnerProject);

	// Transfers registrations and borrowed indexes, preserving destination name collisions.
	// Returns affected identities without releasing their references.
	std::vector<lcPieceData*> TransferProjectPieceData(Project* Source, Project* Destination);

	// Returns the number of library registrations, including projectless missing identities.
	size_t GetLibraryPieceDataCount() const;

	// Returns the library index key for an identity, or an empty string if not registered there.
	std::string GetLibraryPieceDataName(const lcPieceData* PieceData) const;

	// Returns a borrowed snapshot in library name order. Caller prevents identity teardown.
	std::vector<lcPieceData*> GetLibraryPieceData() const;

	// Returns borrowed local identities and their owners in unspecified order.
	// An owner may be null; caller prevents identity teardown while using the snapshot.
	std::vector<std::pair<lcPieceData*, Project*>> GetProjectPieceData() const;

	// Returns a borrowed local identity, or nullptr. Used during reentrant teardown;
	// query again after each deletion rather than retaining a snapshot across callbacks.
	lcPieceData* GetFirstProjectPieceData() const;

	// Returns a borrowed snapshot of library and local identities. Caller prevents identity teardown.
	std::vector<lcPieceData*> GetAllPieceData() const;

	struct FileInfo
	{
		std::string FileName;
		lcZipFileType ZipFileType;
		int ZipFileIndex;
	};

	// Copies library file metadata under the library lock, or returns no value if missing.
	// Workers use the copy after unlocking instead of accessing the registered identity.
	std::optional<FileInfo> GetPieceDataFileInfo(const std::string& Name) const;

private:
	std::map<std::string, lcPieceData*> mPieces;

	// An owner may be null after its project is destroyed while references remain.
	std::unordered_map<lcPieceData*, Project*> mProjectPieces;
};
