//
//	texture.h
////////////////////////////////////////////////////

#ifndef _TEXTURE_H
#define _TEXTURE_H

class File;

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
  void Load(bool bFilter);
  bool LoadFromFile(char* strFilename, bool bFilter);
  void Unload();

  void LoadIndex(File* idx);
  void AddRef(bool bFilter);
  void DeRef();

  // Read-only
  char m_strName[9];
  unsigned short m_nWidth;
  unsigned short m_nHeight;

protected:
  bool FinishLoadImage (bool bFilter, void *data);

  int m_nRef;
  GLuint m_nID;
  GLenum m_nFormat;
  unsigned long m_nOffset;
  unsigned long m_nFileSize;
};


#endif // _TEXTURE_H
