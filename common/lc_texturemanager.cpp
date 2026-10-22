#include "lc_global.h"
#include "lc_texturemanager.h"
#include "lc_file.h"
#include "lc_string.h"
#include "lc_texture.h"

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
	Texture->SetTemporary(bool(Source.EmbeddedData) || !ProjectPath.isEmpty());
	Texture->AddRef();

	mTextures.push_back(Texture);

	return Texture;
}

lcTextureBuildResult lcTextureManager::BuildTextureData(lcFile& File)
{
	lcTextureBuildResult Result = DecodeTextureData(File);

	if (!Result.DecodedImage.isNull())
	{
		Result.DecodedImage = lcTexture::PrepareImage(std::move(Result.DecodedImage));

		if (Result.DecodedImage.isNull())
			Result.Error = lcTextureLoadError::DecodeFailed;
	}

	return Result;
}

lcTextureBuildResult lcTextureManager::DecodeTextureData(lcFile& File)
{
	lcTextureBuildResult Result;

	bool Loaded = false;

	if (lcMemFile* MemoryFile = dynamic_cast<lcMemFile*>(&File))
	{
		if (MemoryFile->mBuffer && MemoryFile->mPosition < MemoryFile->mFileSize)
		{
			const size_t Length = MemoryFile->mFileSize - MemoryFile->mPosition;

			if (Length <= static_cast<size_t>(std::numeric_limits<int>::max()))
				Loaded = Result.DecodedImage.loadFromData(MemoryFile->mBuffer + MemoryFile->mPosition, static_cast<int>(Length));
		}
	}
	else if (lcDiskFile* DiskFile = dynamic_cast<lcDiskFile*>(&File))
	{
		QFile& Device = DiskFile->GetFile();
		const qint64 Position = Device.pos();

		if (Position >= 0)
		{
			Loaded = Result.DecodedImage.load(&Device, nullptr);
			const bool Restored = Device.seek(Position);
			Loaded = Loaded && Restored;
		}
	}

	if (!Loaded || Result.DecodedImage.isNull())
	{
		Result.DecodedImage = QImage();
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
