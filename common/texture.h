#ifndef _TEXTURE_H_
#define _TEXTURE_H_

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

  void LoadIndex(lcFile* idx);
  void AddRef(bool bFilter);
  void DeRef();

  // Read-only
  char m_strName[9];
  lcuint16 m_nWidth;
  lcuint16 m_nHeight;

protected:
  bool FinishLoadImage (bool bFilter, void *data);

  int m_nRef;
  GLuint m_nID;
  GLenum m_nFormat;
  lcuint32 m_nOffset;
  lcuint32 m_nFileSize;
};


#endif // _TEXTURE_H_
