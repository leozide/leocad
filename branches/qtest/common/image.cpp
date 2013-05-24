// Image I/O routines
//

#include "lc_global.h"
#include "opengl.h"
#ifdef LC_WINDOWS
#include <windows.h>
#include <windowsx.h>
//#include <mmsystem.h>
#include <vfw.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "image.h"
#include "lc_file.h"

// =============================================================================
// Image functions

Image::Image ()
{
  m_nWidth = 0;
  m_nHeight = 0;
  m_bAlpha = false;
  m_pData = NULL;
}

Image::~Image ()
{
  free (m_pData);
}

void Image::FreeData ()
{
  m_nWidth = 0;
  m_nHeight = 0;
  m_bAlpha = false;
  free (m_pData);
  m_pData = NULL;
}

void Image::Allocate (int width, int height, bool alpha)
{
  FreeData ();

  m_nWidth = width;
  m_nHeight = height;
  m_bAlpha = alpha;

  if (m_bAlpha)
    m_pData = (unsigned char*)malloc (width * height * 4);
  else
    m_pData = (unsigned char*)malloc (width * height * 3);
}

void Image::ResizePow2 ()
{
  int i, shifted_x, shifted_y;

  shifted_x = m_nWidth;
  for (i = 0; ((i < 16) && (shifted_x != 0)); i++)
    shifted_x = shifted_x >> 1;
  shifted_x = (i != 0) ? 1 << (i-1) : 1;

  shifted_y = m_nHeight;
  for (i = 0; ((i < 16) && (shifted_y != 0)); i++)
    shifted_y = shifted_y >> 1;
  shifted_y = (i != 0) ? 1 << (i-1) : 1;

  if ((shifted_x != m_nWidth) || (shifted_y != m_nHeight))
    Resize (shifted_x, shifted_y);
}

void Image::Resize (int width, int height)
{
  int i, j, k, components, stx, sty;
  float accumx, accumy;
  unsigned char* bits;

  if (m_bAlpha)
    components = 4;
  else
    components = 3;

  bits = (unsigned char*)malloc (width * height * components);

  for (j = 0; j < m_nHeight; j++)
  {
    accumy = (float)height*j/(float)m_nHeight;
    sty = (int)floor(accumy);

    for (i = 0; i < m_nWidth; i++)
    {
      accumx = (float)width*i/(float)m_nWidth;
      stx = (int)floor(accumx);

      for (k = 0; k < components; k++)
        bits[(stx+sty*width)*components+k] = m_pData[(i+j*m_nWidth)*components+k];
    }
  }

  free (m_pData);
  m_pData = bits;
  m_nWidth = width;
  m_nHeight = height;
}

void Image::FromOpenGL (int width, int height)
{
  unsigned char *buf;
  buf = (unsigned char*)malloc (width*height*3);

  FreeData ();

  m_pData = (unsigned char*)malloc (width*height*3);
  m_nWidth = width;
  m_nHeight = height;
  m_bAlpha = false;

  glPixelStorei (GL_PACK_ALIGNMENT, 1);
  glReadPixels (0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buf);

  for (int row = 0; row < height; row++)
    memcpy (m_pData + (row*width*3), buf + ((height-row-1)*width*3), width*3);

  free (buf);
}
