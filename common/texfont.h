#ifndef _TEXFONT_H_
#define _TEXFONT_H_

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

	bool Initialize();
	void PrintText(float left, float top, float z, const char* text) const;
	void PrintText(float Left, float Top, float ScaleX, float ScaleY, const char* Text) const;
	void PrintCharScaled(float scale, int ch) const;
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
};

#endif // _TEXFONT_H_
