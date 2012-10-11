#include "lc_global.h"
#include "lc_texture.h"
#include "lc_file.h"
#include "lc_application.h"
#include "lc_library.h"
#include "image.h"

lcTexture::lcTexture()
{
	mTexture = 0;
	mRefCount = 0;
}

lcTexture::~lcTexture()
{
	Unload();
}

bool lcTexture::Load()
{
	return lcGetPiecesLibrary()->LoadTexture(this);
}

bool lcTexture::Load(lcFile& File)
{
	Image image;

	if (!image.FileLoad(File))
		return false;

	image.ResizePow2();

	glGenTextures(1, &mTexture);

	glBindTexture(GL_TEXTURE_2D, mTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexImage2D(GL_TEXTURE_2D, 0, image.Alpha() ? GL_RGBA : GL_RGB, image.Width(), image.Height(), 0, image.Alpha() ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, image.GetData());

	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

void lcTexture::Unload()
{
	glDeleteTextures(1, &mTexture);
	mTexture = 0;
}






#include <string.h>
#include <stdlib.h>

// =============================================================================
// Static functions

static void* ResizeImage (GLubyte* old_image, int components, int srcw, int srch, int destw, int desth)
{
  int i, j, k;
  float sx, sy;
  GLubyte* new_image;

  new_image = (GLubyte*)malloc (destw*desth*components*sizeof(GLubyte));
  if (new_image == NULL)
    return NULL;

  if (destw > 1)
    sx = (GLfloat) (srcw-1) / (GLfloat) (destw-1);
  else
    sx = (GLfloat) (srcw-1);
  if (desth > 1)
    sy = (GLfloat) (srch-1) / (GLfloat) (desth-1);
  else
    sy = (GLfloat) (srch-1);

  for (i = 0; i < desth; i++)
  {
    GLint ii = (GLint)(i * sy);
    for (j = 0; j < destw; j++)
    {
      GLint jj = (GLint)(j * sx);
      GLubyte *src = old_image + (ii * srcw + jj) * components;
      GLubyte *dst = new_image + (i * destw + j) * components;

      for (k = 0; k < components; k++)
	*dst++ = *src++;
    }
  }

  return new_image;
}

/////////////////////////////////////////////////////////////////////////////
// Texture construction/destruction

// Only called for the background image, use LoadIndex()
Texture::Texture()
{
  m_nRef = 1;
  m_nID = 0;
}

Texture::~Texture()
{
}

/////////////////////////////////////////////////////////////////////////////
// Texture attributes

void Texture::AddRef(bool bFilter)
{
  if (m_nRef == 0)
    Load(bFilter);

  m_nRef++;
}

void Texture::DeRef()
{
  m_nRef--;
  if (m_nRef == 0)
    Unload();
}

/////////////////////////////////////////////////////////////////////////////
// Load methods

void Texture::LoadIndex(lcFile* idx)
{
  lcuint8 bt;

  // TODO: don't change ref. if reloading
  m_nRef = 0;
  m_nID = 0;

  idx->ReadBuffer(m_strName, 8);
  idx->ReadU16(&m_nWidth, 1);
  idx->ReadU16(&m_nHeight, 1);
  idx->ReadU8(&bt, 1);

  switch (bt)
  {
  case LC_INTENSITY:
    m_nFormat = GL_LUMINANCE_ALPHA;
    m_nFileSize = m_nWidth*m_nHeight;
    break;
  case LC_RGB:
    m_nFormat = GL_RGB;
    m_nFileSize = m_nWidth*m_nHeight*3;
    break;
  case LC_RGBA:
    m_nFormat = GL_RGBA;
    m_nFileSize = m_nWidth*m_nHeight*4;
    break;
  }

  idx->ReadU32(&m_nOffset, 1);
}

void Texture::Unload()
{
  if (m_nID != 0)
    glDeleteTextures(1, &m_nID);
  m_nID = 0;
}

// Load from textures.bin file
void Texture::Load(bool bFilter)
{
	/*
  char filename[LC_MAXPATH];
  lcDiskFile bin;
  void* bits;

  strcpy(filename, lcGetPiecesLibrary()->GetLibraryPath());
  strcat(filename, "textures.bin");
  if (!bin.Open(filename, "rb"))
    return;

  if (m_nFormat == GL_LUMINANCE_ALPHA)
    bits = malloc (m_nFileSize*2);
  else
    bits = malloc (m_nFileSize);

  bin.Seek (m_nOffset, SEEK_SET);
  bin.ReadBuffer(bits, m_nFileSize);

  FinishLoadImage (bFilter, bits);

  free(bits);
  */
}

bool Texture::LoadFromFile (char* strFilename, bool bFilter)
{
  Image image;
  
  if (image.FileLoad (strFilename))
  {
    image.ResizePow2 ();

    m_nWidth = image.Width ();
    m_nHeight = image.Height ();

    if (image.Alpha ())
      m_nFormat = GL_RGBA;
    else
      m_nFormat = GL_RGB;

    if (FinishLoadImage (bFilter, image.GetData ()) == true)
      return true;
  }

  if (m_nID != 0)
  {
    glDeleteTextures(1, &m_nID);
    m_nID = 0;
  }

  m_nWidth = 0;
  m_nHeight = 0;
  m_nFileSize = 0;

  return false;
}

bool Texture::FinishLoadImage (bool bFilter, void *data)
{
  GLint w, h, level, maxsize;
  GLint i, j, k, pow2;
  GLint components;

  if (data == NULL || m_nWidth < 1 || m_nHeight < 1)
    return false;

  if (m_nID == 0)
    glGenTextures(1, &m_nID);

  glBindTexture(GL_TEXTURE_2D, m_nID);
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  //  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bFilter ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bFilter ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  switch (m_nFormat)
  {
  case GL_LUMINANCE_ALPHA: components = 2; break;
  case GL_RGB: components = 3; break;
  case GL_RGBA: components = 4; break;
  default: return false;
  }

  // create an alpha channel for the texture 
  if (m_nFormat == GL_LUMINANCE_ALPHA)
    for (i = m_nWidth*m_nHeight-1; i >= 0; i--)
      ((GLubyte*)data)[i*2+1] = ((GLubyte*)data)[i*2] = ((GLubyte*)data)[i];

  glGetIntegerv (GL_MAX_TEXTURE_SIZE, &maxsize);

  for (pow2 = 1; pow2 < m_nWidth; pow2 = pow2 << 1);
  w = (pow2 == m_nWidth) ? m_nWidth : (pow2 << 1);
 
  for (pow2 = 1; pow2 < m_nHeight; pow2 = pow2 << 1);
  h = (pow2 == m_nHeight) ? m_nHeight : (pow2 << 1);

  if (w > maxsize) w = maxsize;
  if (h > maxsize) h = maxsize;

  if (w != m_nWidth || h != m_nHeight)
  {
    data = ResizeImage ((GLubyte*)data, components, m_nWidth, m_nHeight, w, h);
    m_nWidth = w;
    m_nHeight = h;
    if (data == NULL)
      return false;
  }
  else
  {
    void *tmp = malloc (w*h*components);
    memcpy (tmp, data, w*h*components);
    data = tmp;
  }

  glTexImage2D (GL_TEXTURE_2D, 0, components, w, h, 0, m_nFormat, GL_UNSIGNED_BYTE, data);

  if (bFilter)
    for (level = 1; ((w != 1) || (h != 1)); level++)
    {
      GLubyte *out, *in;
      int row;

      row = w * components;
      if (w != 1) w >>= 1;
      if (h != 1) h >>= 1;
      in = out = (GLubyte*)data;

      for (i = 0; i < h; i++, in+=row)
	for (j = 0; j < w; j++, out+=components, in+=2*components)
	  for (k = 0; k < components; k++)
	    out[k] = (in[k] + in[k+components] + in[row] + in[row+k+components])>>2;

      glTexImage2D (GL_TEXTURE_2D, level, components, w, h, 0, m_nFormat, GL_UNSIGNED_BYTE, data);
    }

  free (data);
  return true;
}
