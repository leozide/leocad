#pragma once

#include "lc_texture.h"

class lcFile;

struct lcTextureBuildResult
{
	lcTextureBuildResult() = default;
	~lcTextureBuildResult();

	lcTextureBuildResult(lcTextureBuildResult&&) = default;
	lcTextureBuildResult& operator=(lcTextureBuildResult&&) = default;
	lcTextureBuildResult(const lcTextureBuildResult&) = delete;
	lcTextureBuildResult& operator=(const lcTextureBuildResult&) = delete;

	QImage DecodedImage;
	lcTextureLoadError Error{};
	QString ErrorDetails;
};

// Owns texture identities, references, and CPU decoding of resolved sources.
// lcAssetLoader owns pending requests and asynchronous upload/publication.
class lcTextureManager
{
public:
	lcTextureManager() = default;
	~lcTextureManager();

	lcTextureManager(const lcTextureManager&) = delete;
	lcTextureManager(lcTextureManager&&) = delete;
	lcTextureManager& operator=(const lcTextureManager&) = delete;
	lcTextureManager& operator=(lcTextureManager&&) = delete;

	// The owner drains workers and releases mesh dependencies before clearing.
	void Clear();

	// UI-thread acquisition only; each returned reference must be released.
	lcTexture* AcquireTexture(const char* Name, const lcFileSource& Source, const QString& ProjectPath);

	void AddTextureReference(lcTexture* Texture);
	void ReleaseTexture(lcTexture* Texture);

	// Worker-safe private CPU build; never publishes or uploads the result.
	// Decodes from the current file position without advancing its cursor.
	lcTextureBuildResult BuildTextureData(lcFile& File);

private:
	lcTextureBuildResult DecodeTextureData(lcFile& File);

	QMutex mMutex;
	std::vector<lcTexture*> mTextures;
};
