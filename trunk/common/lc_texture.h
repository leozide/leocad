#ifndef _LC_TEXTURE_H_
#define _LC_TEXTURE_H_

#include "opengl.h"

#define LC_TEXTURE_WRAPU   0x01
#define LC_TEXTURE_WRAPV   0x02
#define LC_TEXTURE_MIPMAPS 0x04

class lcTexture
{
public:
	lcTexture();
	~lcTexture();

	bool Load();
	bool Load(const char* FileName, int Flags = 0);
	bool Load(lcFile& File, int Flags = 0);
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
	char mName[LC_MAXPATH];
	GLuint mTexture;
	int mRefCount;
};

#endif // _LC_TEXTURE_H_
