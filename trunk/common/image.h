#ifndef _IMAGE_H_
#define _IMAGE_H_

class File;

#include "typedefs.h"

class Image
{
public:
  Image ();
  virtual ~Image ();

  bool FileSave (File& file, LC_IMAGE_OPTS* opts) const;
  bool FileSave (const char* filename, LC_IMAGE_OPTS* opts) const;
  bool FileLoad (File& file);
  bool FileLoad (const char* filename);

  void Resize (int width, int height);
  void ResizePow2 ();
  void FromOpenGL (int width, int height);

  int Width () const
    { return m_nWidth; }
  int Height () const
    { return m_nHeight; }
  int Alpha () const
    { return m_bAlpha; }
  unsigned char* GetData () const
    { return m_pData; }

protected:
  void FreeData ();

  bool LoadJPG (File& file);
  bool LoadBMP (File& file);
  bool LoadPNG (File& file);
  bool LoadGIF (File& file);

  bool SaveJPG (File& file, int quality, bool progressive) const;
  bool SaveBMP (File& file, bool quantize) const;
  bool SavePNG (File& file, bool transparent, bool interlaced, unsigned char* background) const;
  bool SaveGIF (File& file, bool transparent, bool interlaced, unsigned char* background) const;

  int m_nWidth;
  int m_nHeight;
  bool m_bAlpha;
  unsigned char* m_pData;
};

void SaveVideo(char* filename, Image *images, int count, float fps);

#endif // _IMAGE_H_