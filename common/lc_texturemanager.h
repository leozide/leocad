#pragma once

#include "lc_assettypes.h"

// Creates and reuses textures, tracks their references, and prepares image data.
// The asset loader handles scheduling, OpenGL uploads, and retries.
class lcTextureManager
{
public:
	lcTextureManager() = default;
	~lcTextureManager();

	lcTextureManager(const lcTextureManager&) = delete;
	lcTextureManager(lcTextureManager&&) = delete;
	lcTextureManager& operator=(const lcTextureManager&) = delete;
	lcTextureManager& operator=(lcTextureManager&&) = delete;

	// Deletes all managed textures, regardless of reference count.
	// Stop workers and release mesh references before calling.
	void Clear();

	// Returns an existing texture for this source or creates one.
	// Call on the UI thread and release the returned reference when done.
	// Returns nullptr for an invalid name or missing source.
	lcTexture* AcquireTexture(const char* Name, const lcFileSource& Source, const QString& ProjectPath);

	// Keeps a texture alive for another owner.
	void AddTextureReference(lcTexture* Texture);

	// Releases an owner's reference. Unloads unused textures and
	// deletes temporary textures when their last reference is released.
	void ReleaseTexture(lcTexture* Texture);

	// Decodes and prepares image pixels for a later OpenGL upload.
	// Preserves the file position. Workers must use separate file instances.
	// Returns a DecodeFailed error if loading or preparation fails.
	lcTextureBuildResult BuildTextureData(lcFile& File);

private:
	// Reads an image from the current file position without advancing it.
	// Supports memory and disk files; returns DecodeFailed on failure.
	lcTextureBuildResult DecodeTextureData(lcFile& File);

	QMutex mMutex;
	std::vector<lcTexture*> mTextures;
};
