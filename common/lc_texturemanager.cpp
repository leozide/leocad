#include "lc_global.h"
#include "lc_texturemanager.h"
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

lcTexture* lcTextureManager::AcquireTexture(const char* TextureName, const lcFileSource& Source, const QString& ProjectPath)
{
	if (!TextureName[0] || strlen(TextureName) >= LC_TEXTURE_NAME_LEN)
		return nullptr;

	if (!Source.EmbeddedData && Source.FilePath.isEmpty() && Source.ArchiveIndex == -1)
		return nullptr;

	QMutexLocker Lock(&mMutex);

	for (lcTexture* Texture : mTextures)
	{
		if (Texture->mSource != Source)
			continue;

		Texture->AddRef();

		return Texture;
	}

	lcTexture* Texture = new lcTexture(LC_TEXTURE_MIPMAPS);

	lcstrcpy(Texture->mName, TextureName);
	Texture->mSource = Source;
	Texture->mProjectPath = ProjectPath;
	Texture->SetTemporary(bool(Source.EmbeddedData) || !ProjectPath.isEmpty());
	Texture->AddRef();

	mTextures.push_back(Texture);

	return Texture;
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
