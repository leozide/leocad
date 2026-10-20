#pragma once

class lcPieceData;
class lcTexture;
class lcPiecesLibrary;
class lcMesh;
struct lcEmbeddedData;
enum class lcStudStyle;
enum class lcZipFileType;

// Scoped asset reference. Construction from a borrowed pointer and copying retain;
// moving transfers ownership. Use and destroy on the UI thread before library unload.
// Ownership operations are instantiated for piece definitions and textures in the cpp.
template<typename T>
class lcAssetRef
{
public:
	lcAssetRef() = default;
	explicit lcAssetRef(T* Asset);
	lcAssetRef(const lcAssetRef& OtherRef);
	lcAssetRef(lcAssetRef&& OtherRef) noexcept;
	~lcAssetRef();

	lcAssetRef& operator=(const lcAssetRef& OtherRef);
	lcAssetRef& operator=(lcAssetRef&& OtherRef) noexcept;

	T* Get() const
	{
		return mAsset;
	}

	T* operator->() const
	{
		return mAsset;
	}

	T& operator*() const
	{
		return *mAsset;
	}

	explicit operator bool() const
	{
		return mAsset != nullptr;
	}

	friend bool operator==(const lcAssetRef& LeftRef, const lcAssetRef& RightRef) noexcept
	{
		return LeftRef.mAsset == RightRef.mAsset;
	}

	friend bool operator!=(const lcAssetRef& LeftRef, const lcAssetRef& RightRef) noexcept
	{
		return !(LeftRef == RightRef);
	}

	friend bool operator<(const lcAssetRef& LeftRef, const lcAssetRef& RightRef) noexcept
	{
		return std::less<T*>()(LeftRef.mAsset, RightRef.mAsset);
	}

	friend bool operator==(const lcAssetRef& LeftRef, const T* Right) noexcept
	{
		return LeftRef.mAsset == Right;
	}

	friend bool operator!=(const lcAssetRef& LeftRef, const T* Right) noexcept
	{
		return !(LeftRef == Right);
	}

	friend bool operator==(const T* Left, const lcAssetRef& RightRef) noexcept
	{
		return Left == RightRef.mAsset;
	}

	friend bool operator!=(const T* Left, const lcAssetRef& RightRef) noexcept
	{
		return !(Left == RightRef);
	}

	// True when nonempty and another reference exists. Query on the UI thread.
	bool HasOtherReferences() const;

	void Reset();
	T* Release();
	void Swap(lcAssetRef& OtherRef) noexcept;

private:
	friend class lcPiecesLibrary;

	lcAssetRef(T* Asset, lcPiecesLibrary* Library);
	// The library uses this only for a reference it has already acquired.
	static lcAssetRef Adopt(T* Asset, lcPiecesLibrary* Library);
	static void Retain(T* Asset, lcPiecesLibrary* Library);
	static void Release(T* Asset, lcPiecesLibrary* Library);

	T* mAsset = nullptr;
	lcPiecesLibrary* mLibrary = nullptr;
};

using lcPieceDataRef = lcAssetRef<lcPieceData>;
using lcTextureRef = lcAssetRef<lcTexture>;

// Asset readiness; detailed scheduling progress belongs to loader requests.
enum class lcAssetState
{
	Unloaded,
	Loading,
	Loaded,
	Failed
};

// Scheduling urgency only. Use the library's wait APIs for synchronous loading.
// Queued requests only promote and keep their priority until completion or cancellation.
enum class lcLoadPriority
{
	Low,      // Background work, such as thumbnails and settings rebuilds.
	Normal,   // Visible or interactive work.
	Blocking  // Assets required by a synchronous operation.
};

// A resolved disk path, archive entry, or embedded file. Copying it does not
// snapshot file contents; workers open their own file from the copied source.
struct lcFileSource
{
	bool operator==(const lcFileSource& Other) const
	{
		return EmbeddedData == Other.EmbeddedData && FilePath == Other.FilePath && ArchiveType == Other.ArchiveType && ArchiveIndex == Other.ArchiveIndex;
	}

	bool operator!=(const lcFileSource& Other) const
	{
		return !(*this == Other);
	}

	std::shared_ptr<const lcEmbeddedData> EmbeddedData;
	QString FilePath;
	int ArchiveType = -1;
	int ArchiveIndex = -1;
};

enum class lcTextureLoadError
{
	None,
	DecodeFailed,
	ContextActivationFailed,
	ContextUnavailable,
	UploadFailed
};

// Copied on the UI thread so workers do not consult live models or project lookup.
struct lcPieceSourceSnapshot
{
	lcPieceSourceSnapshot();

	QString FileName;
	QString LibraryDirectory;
	QString CachePath;
	std::vector<QString> TextureSearchDirectories;
	std::map<QString, std::shared_ptr<const lcEmbeddedData>> EmbeddedData;
	QStringList InlineMeshLines;
	bool InlineModel = false;
	lcZipFileType ZipFileType;
	int ZipFileIndex = -1;
	lcStudStyle StudStyle;
	bool StudCylinderColorEnabled = false;
};

// Private worker output; meshes and dependencies are published only on the UI thread.
struct lcPieceBuildResult
{
	lcPieceBuildResult();
	~lcPieceBuildResult();

	lcPieceBuildResult(lcPieceBuildResult&&) noexcept;
	lcPieceBuildResult& operator=(lcPieceBuildResult&&) noexcept;
	lcPieceBuildResult(const lcPieceBuildResult&) = delete;
	lcPieceBuildResult& operator=(const lcPieceBuildResult&) = delete;

	std::unique_ptr<lcMesh> Mesh;
	bool FromCache = false;
	bool EmptyGeometry = false;
	QString Error;
};

// Image pixels and any error produced while preparing a texture.
struct lcTextureBuildResult
{
	lcTextureBuildResult() = default;
	~lcTextureBuildResult();

	lcTextureBuildResult(lcTextureBuildResult&&) = default;
	lcTextureBuildResult& operator=(lcTextureBuildResult&&) = default;
	lcTextureBuildResult(const lcTextureBuildResult&) = delete;
	lcTextureBuildResult& operator=(const lcTextureBuildResult&) = delete;

	QImage DecodedImage;
	lcTextureLoadError Error = lcTextureLoadError::None;
	QString ErrorDetails;
};
