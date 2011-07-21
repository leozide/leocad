//
//	texture.h
////////////////////////////////////////////////////

#ifndef _TEXTURE_H
#define _TEXTURE_H

class lcFile;

#include "opengl.h"

typedef enum { LC_INTENSITY, LC_RGB, LC_RGBA } LC_TEXTURE_TYPES;

class Texture
{
 public:
  Texture();
  ~Texture();

  void MakeCurrent()
  {
    if (m_nID != 0)
      glBindTexture(GL_TEXTURE_2D, m_nID);
  }

  bool IsLoaded()
    { return ((m_nID != 0) && (glIsTexture(m_nID) == GL_TRUE)); }
  void Load();
  bool LoadFromFile(const char* strFilename);
  void Unload();

  void LoadIndex(lcFile* idx);
  void AddRef();
  void DeRef();

  // Read-only
  char m_strName[9];
  u16 m_nWidth;
  u16 m_nHeight;

protected:
  bool FinishLoadImage(void *data);

  int m_nRef;
  GLuint m_nID;
  GLenum m_nFormat;
  u32 m_nOffset;
  u32 m_nFileSize;
};


#endif // _TEXTURE_H
