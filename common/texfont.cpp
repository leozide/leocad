//
// Texture Font
//

#include "lc_global.h"
#include "globals.h"
#include "project.h"
#include "texfont.h"
#include "texture.h"
#include "library.h"
#include "lc_file.h"
#include "lc_application.h"

#define LC_TEXFONT_FILE_VERSION 1 // LeoCAD 0.74
#define LC_TEXFONT_FILE_HEADER "LeoCAD Texture Font\0\0\0\0\0\0\0\0\0\0\0\0"

// ============================================================================

TexFont::TexFont ()
{
  m_bLoaded = false;
  m_pTexture = NULL;

  memset (&m_Glyphs, 0, sizeof (m_Glyphs));
}

TexFont::~TexFont ()
{
  if (m_pTexture != NULL)
    m_pTexture->DeRef ();
}

bool TexFont::FileLoad (File& file)
{
  unsigned char version;
  char buf[64];

  if (m_bLoaded)
  {
    console.PrintError ("Texture font already loaded.\n");
    return false;
  }

  file.Read (buf, 32);
  if (strncmp (buf, LC_TEXFONT_FILE_HEADER, 32) != 0)
  {
    console.PrintError ("Texture font file header mismatch.\n");
    return false;
  }

  file.ReadByte (&version, 1);
  if (version > LC_TEXFONT_FILE_VERSION)
  {
    console.PrintError ("Wrong version of texture font file.\n");
    return false;
  }

  memset (buf, 0, 32);
  file.Read (buf, 8);

  m_pTexture = lcGetPiecesLibrary()->FindTexture (buf);
  if (m_pTexture == NULL)
  {
    console.PrintError ("Cannot find texture for font %s.\n", buf);
    return false;
  }
  m_pTexture->AddRef (false);
 
  file.ReadByte (&m_nFontHeight, 1);

  for (;;)
  {
    unsigned char glyph;

    file.ReadByte (&glyph, 1);

    if (glyph == 0)
      break;

    file.ReadByte (&m_Glyphs[glyph].width, 1);
    file.ReadFloat (&m_Glyphs[glyph].left, 1);
    file.ReadFloat (&m_Glyphs[glyph].right, 1);
    file.ReadFloat (&m_Glyphs[glyph].top, 1);
    file.ReadFloat (&m_Glyphs[glyph].bottom, 1);

    m_Glyphs[glyph].left /= m_pTexture->m_nWidth;
    m_Glyphs[glyph].right /= m_pTexture->m_nWidth;
    m_Glyphs[glyph].top /= m_pTexture->m_nHeight;
    m_Glyphs[glyph].bottom /= m_pTexture->m_nHeight;
  }

  m_bLoaded = true;

  return true;
}

void TexFont::GetStringDimensions(int* cx, int* cy, const char* Text) const
{
	*cx = 0;
	*cy = m_nFontHeight;

  while (*Text != 0)
  {
		*cx += m_Glyphs[(int)(*Text)].width;
    Text++;
  }
}

void TexFont::PrintText(float Left, float Top, float ScaleX, float ScaleY, const char* Text) const
{
	float Height = m_nFontHeight * ScaleY;

  while (*Text != 0)
  {
    int ch = *Text;
    glTexCoord2f(m_Glyphs[ch].left, m_Glyphs[ch].top);
    glVertex2f(Left, Top);
    glTexCoord2f(m_Glyphs[ch].left, m_Glyphs[ch].bottom);
    glVertex2f(Left, Top - Height);
    glTexCoord2f(m_Glyphs[ch].right, m_Glyphs[ch].bottom);
    glVertex2f(Left + m_Glyphs[ch].width * ScaleX, Top - Height);
    glTexCoord2f(m_Glyphs[ch].right, m_Glyphs[ch].top);
    glVertex2f(Left + m_Glyphs[ch].width * ScaleX, Top);

    Left += m_Glyphs[ch].width * ScaleX;
    Text++;
  }
}

// Old function, should probably be removed.
void TexFont::PrintText(float Left, float Top, float Z, const char* Text) const
{
	while (*Text != 0)
	{
	  int ch = *Text;
		glTexCoord2f(m_Glyphs[ch].left, m_Glyphs[ch].top);
		glVertex3f(Left, Top, Z);
		glTexCoord2f(m_Glyphs[ch].left, m_Glyphs[ch].bottom);
		glVertex3f(Left, Top - m_nFontHeight, Z);
		glTexCoord2f(m_Glyphs[ch].right, m_Glyphs[ch].bottom);
		glVertex3f(Left + m_Glyphs[ch].width, Top - m_nFontHeight, Z);
		glTexCoord2f(m_Glyphs[ch].right, m_Glyphs[ch].top);
		glVertex3f(Left + m_Glyphs[ch].width, Top, Z);

		Left += m_Glyphs[ch].width;
		Text++;
	}
}

// Temporary function to draw the axis icon text
void TexFont::PrintCharScaled (float scale, int ch) const
{
  glTexCoord2f (m_Glyphs[ch].left, m_Glyphs[ch].top);
  glVertex2f (-scale * m_Glyphs[ch].width, scale * m_nFontHeight);
  glTexCoord2f (m_Glyphs[ch].left, m_Glyphs[ch].bottom);
  glVertex2f (-scale * m_Glyphs[ch].width, -scale * m_nFontHeight);
  glTexCoord2f (m_Glyphs[ch].right, m_Glyphs[ch].bottom);
  glVertex2f (scale * m_Glyphs[ch].width, -scale * m_nFontHeight);
  glTexCoord2f (m_Glyphs[ch].right, m_Glyphs[ch].top);
  glVertex2f (scale * m_Glyphs[ch].width, scale * m_nFontHeight);
}
