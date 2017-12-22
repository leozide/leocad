#pragma once

#define LC_TEXTURE_WRAPU         0x01
#define LC_TEXTURE_WRAPV         0x02
#define LC_TEXTURE_MIPMAPS       0x04

#define LC_TEXTURE_POINT         0x00
#define LC_TEXTURE_LINEAR        0x10
#define LC_TEXTURE_BILINEAR      0x20
#define LC_TEXTURE_TRILINEAR     0x30
#define LC_TEXTURE_ANISOTROPIC   0x40
#define LC_TEXTURE_FILTER_MASK   0xf0
#define LC_TEXTURE_FILTER_SHIFT  4

#define LC_TEXTURE_NAME_LEN 256

#include "image.h"

class lcTexture
{
public:
	lcTexture();
	~lcTexture();

	void CreateGridTexture();

	bool Load(const QString& FileName, int Flags = 0);
	bool Load(lcMemFile& File, int Flags = 0);
	void Upload();
	void Unload();

	void AddRef()
	{
		mRefCount.ref();

		if (mRefCount == 1)
			Load();
	}

	bool Release()
	{
		bool InUse = mRefCount.deref();

		if (!InUse)
			Unload();

		return InUse;
	}

	void SetTemporary(bool Temporary)
	{
		mTemporary = Temporary;
	}

	bool IsTemporary() const
	{
		return mTemporary;
	}

	int mWidth;
	int mHeight;
	char mName[LC_TEXTURE_NAME_LEN];
	GLuint mTexture;

protected:
	bool Load();
	bool Load(int Flags);

	bool mTemporary;
	QAtomicInt mRefCount;
	std::vector<Image> mImages;
	int mFlags;
};

lcTexture* lcLoadTexture(const QString& FileName, int Flags);
void lcReleaseTexture(lcTexture* Texture);

extern lcTexture* gGridTexture;

