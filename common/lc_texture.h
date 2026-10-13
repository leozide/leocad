#pragma once

#define LC_TEXTURE_WRAPU         0x01
#define LC_TEXTURE_WRAPV         0x02
#define LC_TEXTURE_MIPMAPS       0x04
#define LC_TEXTURE_CUBEMAP       0x08

#define LC_TEXTURE_POINT         0x00
#define LC_TEXTURE_LINEAR        0x10
#define LC_TEXTURE_BILINEAR      0x20
#define LC_TEXTURE_TRILINEAR     0x30
#define LC_TEXTURE_ANISOTROPIC   0x40
#define LC_TEXTURE_FILTER_MASK   0xf0
#define LC_TEXTURE_FILTER_SHIFT  4

#define LC_TEXTURE_NAME_LEN 256

#include "lc_filesource.h"

enum class lcTextureState
{
	Unrequested,
	Queued,
	Decoding,
	Decoded,
	RetryPending,
	Ready,
	Failed
};

enum class lcTextureLoadError
{
	None,
	DecodeFailed,
	ContextActivationFailed,
	ContextUnavailable,
	UploadFailed
};

class lcTexture
{
public:
	lcTexture(int Flags = 0);
	~lcTexture();

	lcTexture(const lcTexture&) = delete;
	lcTexture(lcTexture&&) = delete;
	lcTexture& operator=(const lcTexture&) = delete;
	lcTexture& operator=(lcTexture&&) = delete;

	void CreateGridTexture();
	static QImage PrepareImage(QImage Image);

	void SetImage(QImage Image, int Flags);
	void SetImages(std::vector<QImage>&& Images, int Flags);
	void DiscardDecodedImage();
	// Records a failure without discarding pixels retained for upload retries.
	void SetFailed(lcTextureLoadError Error, QString Details);
	void Upload(lcContext* Context);
	void Unload();

	bool HasImageData() const
	{
		return mTexture != 0 || !mImages.empty();
	}

	// Reference bookkeeping belongs to lcTextureManager.
	void AddRef()
	{
		mRefCount.ref();
	}

	// Reference bookkeeping belongs to lcTextureManager.
	bool Release()
	{
		const bool InUse = mRefCount.deref();

		if (!InUse)
			Unload();

		return InUse;
	}

	int GetRefCount() const
	{
		return mRefCount.loadRelaxed();
	}

	void SetTemporary(bool Temporary)
	{
		mTemporary = Temporary;
	}

	bool IsTemporary() const
	{
		return mTemporary;
	}

	bool NeedsUpload() const
	{
		return mState == lcTextureState::Decoded && mTexture == 0 && !mImages.empty();
	}

	bool IsReady() const
	{
		return mState == lcTextureState::Ready && mTexture != 0;
	}

	int GetFlags() const
	{
		return mFlags;
	}

	const QImage& GetImage(int Index) const
	{
		return mImages[Index];
	}

	size_t GetImageCount() const
	{
		return mImages.size();
	}

	int mWidth;
	int mHeight;
	char mName[LC_TEXTURE_NAME_LEN] = {};
	lcFileSource mSource;
	QString mProjectPath; // Directory used to scope temporary textures to their project.
	GLuint mTexture = 0;
	lcTextureState mState = lcTextureState::Unrequested;
	lcTextureLoadError mLoadFailure = lcTextureLoadError::None;
	QString mLoadFailureDetails;

protected:
	bool mTemporary = false;
	QAtomicInt mRefCount = 0;
	std::vector<QImage> mImages;
	int mFlags = 0;
};

extern lcTexture* gGridTexture;
