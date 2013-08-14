#ifndef _LC_TEXTURE_H_
#define _LC_TEXTURE_H_

#include "opengl.h"

#define LC_TEXTURE_WRAPU   0x01
#define LC_TEXTURE_WRAPV   0x02
#define LC_TEXTURE_MIPMAPS 0x04

#define LC_TEXTURE_NAME_LEN 256

class Image;

class lcTexture
{
public:
	lcTexture();
	~lcTexture();

	bool Load(const char* FileName, int Flags = 0);
	bool Load(lcMemFile& File, int Flags = 0);
	bool Load(Image& image, int Flags);
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

#endif // _LC_TEXTURE_H_
