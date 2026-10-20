#pragma once

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
using lcPieceManagerMutex = QRecursiveMutex;
#else
// Use recursive QMutex for Qt 5 compatibility.
class lcPieceManagerMutex : public QMutex
{
public:
	lcPieceManagerMutex()
		: QMutex(QMutex::Recursive)
	{
	}
};
#endif

enum class lcZipFileType;

// Creates and indexes piece identities and manages their references and lifetime.
// The library handles source resolution, loading and cancellation.
// Mutations and borrowed identities belong to the UI thread. Workers may copy
// file metadata and access sparse errors; registry locking ends before teardown.
class lcPieceManager
{
public:
	lcPieceManager() = default;
	~lcPieceManager();

	lcPieceManager(const lcPieceManager&) = delete;
	lcPieceManager(lcPieceManager&&) = delete;
	lcPieceManager& operator=(const lcPieceManager&) = delete;
	lcPieceManager& operator=(lcPieceManager&&) = delete;

	// Normalizes case and slashes, preserving the piece name length limit.
	static std::string NormalizePieceDataName(const char* PieceName);

	// Deletes all managed identities, regardless of reference count.
	// Drain workers and release consumers before calling; the library must remain alive.
	void Clear();

	// Returns a borrowed project identity, falling back to the library. No creation or acquisition.
	// A null owner searches only the shared library registry.
	lcPieceData* FindPieceData(const std::string& Name, Project* OwnerProject) const;

	// Creates a library identity with a discovery index key and source filename.
	// The returned identity is registered with zero references; source metadata is supplied by the library.
	lcPieceData* CreateLibraryPieceData(const std::string& Name, const char* FileName);

	// Creates a non-library identity, registered locally or in the shared registry when ownerless.
	// Returns a borrowed identity with zero references. The caller supplies missing/external state.
	lcPieceData* CreatePieceData(const char* PieceName, Project* OwnerProject);

	// Creates or reuses a local identity and acquires one reference for the model.
	// The library adopts the reference and invalidates reused data after this returns.
	lcPieceData* AcquireModelPieceData(const char* PieceName, Project* OwnerProject, bool& Reused);

	// Renames the identity and its index key, rejecting collisions in the same scope.
	bool RenameModelPieceData(lcPieceData* PieceData, const char* NewName);

	// Keeps an identity alive for another owner. Never starts loading.
	void AddPieceDataReference(lcPieceData* PieceData);

	// Releases one reference, unloading library parts or deleting non-library identities at zero.
	// Returns the remaining count; zero may mean the pointer was deleted. Already-zero is a no-op.
	int ReleasePieceData(lcPieceData* PieceData);

	// Sparse errors are independently synchronized and cleared before resource teardown.
	void SetPieceDataLoadError(const lcPieceData* PieceData, QString Error);
	QString GetPieceDataLoadError(const lcPieceData* PieceData) const;
	void ClearPieceDataLoadError(const lcPieceData* PieceData);

	// Removes unreferenced non-library identities without changing library-part eviction policy.
	void RemoveTemporaryPieceData();

	// Detaches the project's borrowed index and deletes its unreferenced local identities.
	void ReleaseProjectPieceData(Project* OwnerProject);

	// Transfers registrations and borrowed indexes, preserving destination name collisions.
	// Deletes transferred identities that have no references.
	void TransferProjectPieceData(Project* Source, Project* Destination);

	// Returns the number of library registrations, including projectless missing identities.
	size_t GetLibraryPieceDataCount() const;

	// Returns the library index key for an identity, or an empty string if not registered there.
	std::string GetLibraryPieceDataName(const lcPieceData* PieceData) const;

	// Returns a borrowed snapshot in library name order. Caller prevents identity teardown.
	std::vector<lcPieceData*> GetLibraryPieceData() const;

	// Returns a borrowed snapshot of library and local identities. Caller prevents identity teardown.
	std::vector<lcPieceData*> GetAllPieceData() const;

	struct FileInfo
	{
		std::string FileName;
		lcZipFileType ZipFileType;
		int ZipFileIndex;
	};

	// Copies library file metadata under the registry lock, or returns no value if missing.
	// Workers use the copy after unlocking instead of accessing the registered identity.
	std::optional<FileInfo> GetPieceDataFileInfo(const std::string& Name) const;

private:
	// Register a newly created identity without changing its reference count.
	void RegisterLibraryPieceData(const std::string& Name, lcPieceData* PieceData);

	// Also updates the owning project's borrowed name index.
	void RegisterProjectPieceData(Project* OwnerProject, const std::string& Name, lcPieceData* PieceData);

	// Removes registry and borrowed project entries before destruction can reenter.
	void UnregisterPieceData(lcPieceData* PieceData);

	// Unregisters and deletes a non-library identity or an identity being cleared.
	void RemovePieceData(lcPieceData* PieceData);

	// Keeps library identities registered; deletes non-library identities.
	void UnloadPieceData(lcPieceData* PieceData);

	// Recursive for compound operations that reuse registry helpers. Never held during deletion.
	mutable lcPieceManagerMutex mMutex;
	std::map<std::string, lcPieceData*> mPieces;

	// An owner may be null after its project is destroyed while references remain.
	std::unordered_map<lcPieceData*, Project*> mProjectPieces;

	// No error lock is held while unloading or deleting identities.
	mutable QMutex mErrorMutex;
	std::unordered_map<const lcPieceData*, QString> mPieceDataLoadErrors;
};
