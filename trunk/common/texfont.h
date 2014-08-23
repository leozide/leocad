#ifndef _TEXFONT_H_
#define _TEXFONT_H_

#include "opengl.h"

class TexFont
{
public:
	TexFont();
	~TexFont();

	bool IsLoaded() const
	{
		return mTexture != 0;
	}

	void MakeCurrent()
	{
		glBindTexture(GL_TEXTURE_2D, mTexture);
	}

	bool Load();
	void Release();

	void PrintText(float Left, float Top, float Z, const char* Text) const;
	void GetStringDimensions(int* cx, int* cy, const char* Text) const;

protected:
	struct
	{
		unsigned char width;
		float left, right, top, bottom;
	} mGlyphs[256];

	GLuint mTexture;
	int mTextureWidth;
	int mTextureHeight;
	int mFontHeight;
	int mRefCount;
};

extern TexFont gTexFont;

#endif // _TEXFONT_H_
