#ifndef _TEXFONT_H_
#define _TEXFONT_H_

class File;
class Texture;

#include "texture.h"

class TexFont
{
public:
	TexFont ();
	~TexFont ();

	bool IsLoaded () const
	{ return m_bLoaded; }
	void MakeCurrent ()
	{ if (m_bLoaded) m_pTexture->MakeCurrent (); }

	bool FileLoad(File& file);
	void PrintText(float left, float top, float z, const char* text) const;
	void PrintText(float Left, float Top, float ScaleX, float ScaleY, const char* Text) const;
	void PrintCharScaled(float scale, char ch) const;
	void GetStringDimensions(int* cx, int* cy, const char* Text) const;

protected:
	struct
	{
		unsigned char width;
		float left, right, top, bottom;
	} m_Glyphs[256];

	Texture* m_pTexture;
	unsigned char m_nFontHeight;
	bool m_bLoaded;
};

#endif // _TEXFONT_H_
