#ifndef _TEXFONT_H_
#define _TEXFONT_H_

class File;
class Texture;

class TexFont
{
public:
  TexFont ();
  ~TexFont ();

  bool IsLoaded () const
    { return m_bLoaded; }

  bool FileLoad (File& file);
  void PrintText (float left, float top, const char* text) const;
  void PrintCharScaled (float scale, char ch) const;

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
