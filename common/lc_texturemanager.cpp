#include "lc_global.h"
#include "lc_texturemanager.h"
#include "lc_library.h"
#include "lc_file.h"
#include "lc_string.h"

lcTextureBuildResult::~lcTextureBuildResult() = default;

lcTextureManager::~lcTextureManager()
{
	Clear();
}

void lcTextureManager::Clear()
{
	QMutexLocker Lock(&mMutex);

	for (lcTexture* Texture : mTextures)
		delete Texture;

	mTextures.clear();
}

void lcTextureManager::AddTextureReference(lcTexture* Texture)
{
	QMutexLocker Lock(&mMutex);

	Texture->AddRef();
}

lcTexture* lcTextureManager::AcquireTexture(const char* TextureName, const lcFileSourceSnapshot& Source, const QString& ProjectPath)
{
	if (!TextureName[0] || strlen(TextureName) >= LC_TEXTURE_NAME_LEN)
		return nullptr;

	if (!Source.EmbeddedData && Source.FilePath.isEmpty() && Source.ArchiveIndex == -1)
		return nullptr;

	QMutexLocker Lock(&mMutex);

	for (lcTexture* Texture : mTextures)
	{
		if (Texture->mEmbeddedData != Source.EmbeddedData || Texture->mFilePath != Source.FilePath || Texture->mArchiveType != Source.ArchiveType || Texture->mArchiveIndex != Source.ArchiveIndex)
			continue;

		Texture->AddRef();

		return Texture;
	}

	lcTexture* Texture = new lcTexture(LC_TEXTURE_MIPMAPS);

	lcstrcpy(Texture->mName, TextureName);
	Texture->mEmbeddedData = Source.EmbeddedData;
	Texture->mFilePath = Source.FilePath;
	Texture->mProjectPath = ProjectPath;
	Texture->mArchiveType = Source.ArchiveType;
	Texture->mArchiveIndex = Source.ArchiveIndex;
	Texture->SetTemporary(bool(Source.EmbeddedData) || !ProjectPath.isEmpty());
	Texture->AddRef();

	mTextures.push_back(Texture);

	return Texture;
}

lcFileSourceSnapshot lcTextureManager::SnapshotTextureSource(const lcTexture* Texture) const
{
	lcFileSourceSnapshot Source;

	Source.EmbeddedData = Texture->mEmbeddedData;
	Source.FilePath = Texture->mFilePath;
	Source.ArchiveType = Texture->mArchiveType;
	Source.ArchiveIndex = Texture->mArchiveIndex;

	return Source;
}

lcTextureBuildResult lcTextureManager::BuildTextureData(lcFile& File)
{
	lcTextureBuildResult Result = DecodeTextureData(File);

	if (Result.DecodedImage)
		Result.DecodedImage->ResizePow2();

	return Result;
}

lcTextureBuildResult lcTextureManager::DecodeTextureData(lcFile& File)
{
	lcTextureBuildResult Result;

	Result.DecodedImage.reset(new Image);

	if (!Result.DecodedImage->FileLoad(File))
	{
		Result.DecodedImage.reset();
		Result.Error = lcTextureLoadError::DecodeFailed;
	}

	return Result;
}

bool lcTextureManager::LoadTexture(lcTexture* Texture, lcFile& File)
{
	lcTextureBuildResult Result = DecodeTextureData(File);

	if (!Result.DecodedImage)
	{
		Texture->SetFailed(Result.Error, std::move(Result.ErrorDetails));
		return false;
	}

	Texture->SetImage(std::move(*Result.DecodedImage));

	return true;
}

void lcTextureManager::ReleaseTexture(lcTexture* Texture)
{
	QMutexLocker Lock(&mMutex);

	if (Texture->Release() == 0 && Texture->IsTemporary())
	{
		std::vector<lcTexture*>::iterator TextureIt = std::find(mTextures.begin(), mTextures.end(), Texture);
		if (TextureIt != mTextures.end())
			mTextures.erase(TextureIt);
		delete Texture;
	}
}
