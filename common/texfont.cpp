//
// Texture Font
//

#include "globals.h"
#include "project.h"
#include "texfont.h"
#include "texture.h"
#include "library.h"
#include "file.h"

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

  m_pTexture = project->GetPiecesLibrary()->FindTexture (buf);
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

void TexFont::PrintText (float left, float top, const char* text) const
{
  while (*text != 0)
  {
    glTexCoord2f (m_Glyphs[*text].left, m_Glyphs[*text].top);
    glVertex2f (left, top);
    glTexCoord2f (m_Glyphs[*text].left, m_Glyphs[*text].bottom);
    glVertex2f (left, top - m_nFontHeight);
    glTexCoord2f (m_Glyphs[*text].right, m_Glyphs[*text].bottom);
    glVertex2f (left + m_Glyphs[*text].width, top - m_nFontHeight);
    glTexCoord2f (m_Glyphs[*text].right, m_Glyphs[*text].top);
    glVertex2f (left + m_Glyphs[*text].width, top);

    left += m_Glyphs[*text].width;
    text++;
  }
}

// Temporary function to draw the axis icon text
void TexFont::PrintCharScaled (float scale, char ch) const
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
