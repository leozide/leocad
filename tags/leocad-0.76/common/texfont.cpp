//
// Texture Font
//

#include "lc_global.h"
#include "project.h"
#include "texfont.h"
#include "texture.h"
#include "library.h"
#include "file.h"
#include "lc_application.h"
#include "console.h"

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

void TexFont::PrintText(float Left, float Top, float Z, const char* Text) const
{
	float Height = m_nFontHeight;

	float Verts[4][3];
	float Coords[4][2];

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, Verts);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, Coords);

	while (*Text != 0)
	{
		int ch = *Text;

		Coords[0][0] = m_Glyphs[ch].left; Coords[0][1] = m_Glyphs[ch].top;
		Verts[0][0] = Left; Verts[0][1] = Top; Verts[0][2] = Z;
		Coords[1][0] = m_Glyphs[ch].left; Coords[1][1] = m_Glyphs[ch].bottom;
		Verts[1][0] = Left; Verts[1][1] = Top - Height; Verts[1][2] = Z;
		Coords[2][0] = m_Glyphs[ch].right; Coords[2][1] = m_Glyphs[ch].top;
		Verts[2][0] = Left + m_Glyphs[ch].width; Verts[2][1] = Top; Verts[2][2] = Z;
		Coords[3][0] = m_Glyphs[ch].right; Coords[3][1] = m_Glyphs[ch].bottom;
		Verts[3][0] = Left + m_Glyphs[ch].width; Verts[3][1] = Top - Height; Verts[3][2] = Z;

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		Left += m_Glyphs[ch].width;
		Text++;
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}
