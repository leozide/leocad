#ifndef _LC_TEXTURE_H_
#define _LC_TEXTURE_H_

#include "opengl.h"

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

class Image;

class lcTexture
{
public:
	lcTexture();
	~lcTexture();

	void CreateGridTexture();

	bool Load(const char* FileName, int Flags = 0);
	bool Load(lcMemFile& File, int Flags = 0);
	bool Load(Image& image, int Flags);
	bool Load(Image* images, int NumLevels, int Flags);
	void Unload();

	int AddRef()
	{
		mRefCount++;

		if (mRefCount == 1)
			Load();

		return mRefCount;
	}

	int Release()
	{
		mRefCount--;

		if (!mRefCount)
			Unload();

		return mRefCount;
	}

	int mWidth;
	int mHeight;
	char mName[LC_TEXTURE_NAME_LEN];
	GLuint mTexture;

protected:
	bool Load();

	int mRefCount;
};

extern lcTexture* gGridTexture;

#endif // _LC_TEXTURE_H_
