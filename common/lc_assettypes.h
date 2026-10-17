#pragma once

class lcMesh;
struct lcEmbeddedData;
enum class lcStudStyle;
enum class lcZipFileType;

// Asset readiness; detailed scheduling progress belongs to loader requests.
enum class lcAssetState
{
	Unloaded,
	Loading,
	Loaded,
	Failed
};

// Scheduling urgency. The library's RequestPieceLoad() also waits when Blocking is requested.
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
