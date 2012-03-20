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
#include "config.h"
#include "image.h"
#include "file.h"

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

bool Image::FileLoad (File& file)
{
  unsigned char buf[8];

  // Read a few bytes
  if (file.Read (buf, 8) != 8)
    return false;
  file.Seek (-8, SEEK_CUR);

  // Check for the BMP header
  if ((buf[0] == 'B') && (buf[1] == 'M'))
  {
    if (!LoadBMP (file))
      return false;

    return true;
  }

#ifdef LC_HAVE_JPEGLIB
  if ((buf[0] == 0xFF) && (buf[1] == 0xD8))
  {
    if (!LoadJPG (file))
      return false;

    return true;
  }
#endif

#ifdef LC_HAVE_PNGLIB
  const unsigned char png_signature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };

  // Check for the PNG header
  if (memcmp (buf, png_signature, 8) == 0)
  {
    if (!LoadPNG (file))
      return false;

    return true;
  }
#endif

  // Check for the GIF header
  if ((buf[0] == 'G') && (buf[1] == 'I') && (buf[2] == 'F') &&
      (buf[3] == '8') && ((buf[4] == '7') || (buf[4] == '9')) &&
      (buf[5] == 'a'))
  {
    if (!LoadGIF (file))
      return false;

    return true;
  }

//	MessageBox (NULL, "Unknown File Format", "Error", MB_ICONSTOP);
  return false;
}

bool Image::FileLoad (const char* filename)
{
  FileDisk file;
  
  if (!file.Open (filename, "rb"))
    return false;

  return FileLoad (file);
}

bool Image::FileSave (File& file, LC_IMAGE_OPTS* opts) const
{
  switch (opts->format)
  {
#ifdef LC_HAVE_JPEGLIB
  case LC_IMAGE_JPG:
    return SaveJPG (file, opts->quality, opts->interlaced);
#endif

  case LC_IMAGE_GIF:
    return SaveGIF (file, opts->transparent, opts->interlaced, opts->background);

  case LC_IMAGE_BMP:
    return SaveBMP (file, opts->truecolor == false);

#ifdef LC_HAVE_PNGLIB
  case LC_IMAGE_PNG:
    return SavePNG (file, opts->transparent, opts->interlaced, opts->background);
#endif

  default:
    break;
  }

//	MessageBox (NULL, "Could not save file", "Error", MB_ICONSTOP);

  return false;
}

bool Image::FileSave (const char* filename, LC_IMAGE_OPTS* opts) const
{
  char name[LC_MAXPATH], ext[5], *p;
  FileDisk file;
  bool needext = false;

  strcpy (name, filename);
  p = name + strlen (name) - 1;

  while ((p > name) && (*p != '/') && (*p != '\\') && (*p != '.'))
    p--;

  if (*p != '.')
    needext = true;
  else
  {
    if (strlen (p) > 5)
      needext = true;
    else
    {
      strcpy (ext, p+1);
      strlwr (ext);

      if (strcmp (ext, "bmp") == 0)
        opts->format = LC_IMAGE_BMP;
      else if (strcmp (ext, "gif") == 0)
        opts->format = LC_IMAGE_GIF;
#ifdef LC_HAVE_JPEGLIB
      else if (strcmp (ext, "jpg") == 0)
        opts->format = LC_IMAGE_JPG;
      else if (strcmp (ext, "jpeg") == 0)
        opts->format = LC_IMAGE_JPG;
#endif
#ifdef LC_HAVE_PNGLIB
      else if (strcmp (ext, "png") == 0)
        opts->format = LC_IMAGE_PNG;
#endif
      else
        needext = true;
    }
  }

  if (needext)
  {
    // no extension, add from the options
    switch (opts->format)
    {
    case LC_IMAGE_BMP:
      strcat (name, ".bmp");
      break;
    case LC_IMAGE_GIF:
      strcat (name, ".gif");
      break;
#ifdef LC_HAVE_JPEGLIB
    case LC_IMAGE_JPG:
      strcat (name, ".jpg");
      break;
#endif
#ifdef LC_HAVE_PNGLIB
    case LC_IMAGE_PNG:
      strcat (name, ".png");
      break;
#endif
    default:
      return false;
    }
  }

  if (!file.Open (name, "wb"))
    return false;

  return FileSave (file, opts);
}





// =============================================================================
// Global functions

#ifdef LC_WINDOWS
#include "system.h"

#define AVIIF_KEYFRAME	0x00000010L // this frame is a key frame.
#define LPLPBI	LPBITMAPINFOHEADER *

static HANDLE  MakeDib (HBITMAP hbitmap, Image& image)
{
	HANDLE              hdib ;
	HDC                 hdc ;
	BITMAP              bitmap ;
	UINT                wLineLen ;
	DWORD               dwSize ;
	DWORD               wColSize ;
	LPBITMAPINFOHEADER  lpbi ;
	LPBYTE              lpBits ;
  UINT bits = 24;
	int i, j;

	GetObject(hbitmap,sizeof(BITMAP),&bitmap) ;

	// DWORD align the width of the DIB
	// Figure out the size of the colour table
	// Calculate the size of the DIB
	wLineLen = (bitmap.bmWidth*bits+31)/32 * 4;
	wColSize = sizeof(RGBQUAD)*((bits <= 8) ? 1<<bits : 0);
	dwSize = sizeof(BITMAPINFOHEADER) + wColSize +
		(DWORD)(UINT)wLineLen*(DWORD)(UINT)bitmap.bmHeight;

	// Allocate room for a DIB and set the LPBI fields
	hdib = GlobalAlloc(GHND,dwSize);
	if (!hdib)
		return hdib ;

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hdib) ;

	lpbi->biSize = sizeof(BITMAPINFOHEADER) ;
	lpbi->biWidth = bitmap.bmWidth ;
	lpbi->biHeight = bitmap.bmHeight ;
	lpbi->biPlanes = 1 ;
	lpbi->biBitCount = (WORD) bits ;
	lpbi->biCompression = BI_RGB ;
	lpbi->biSizeImage = dwSize - sizeof(BITMAPINFOHEADER) - wColSize ;
	lpbi->biXPelsPerMeter = 0 ;
	lpbi->biYPelsPerMeter = 0 ;
	lpbi->biClrUsed = (bits <= 8) ? 1<<bits : 0;
	lpbi->biClrImportant = 0 ;

	// Get the bits from the bitmap and stuff them after the LPBI
	lpBits = (LPBYTE)(lpbi+1)+wColSize ;

	hdc = CreateCompatibleDC(NULL) ;

	GetDIBits(hdc,hbitmap,0,bitmap.bmHeight,lpBits,(LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

  for (i = 0; i < lpbi->biHeight; i++)
  {
    unsigned char *src = image.GetData() + i * image.Width() * 3;
    unsigned char *dst = lpBits + (lpbi->biHeight - i - 1) * wLineLen;

    for (j = 0; j < lpbi->biWidth; j++)
    {
      dst[0] = src[2];
      dst[1] = src[1];
      dst[2] = src[0];

      src += 3;
      dst += 3;
    }
  }

	// Fix this if GetDIBits messed it up....
	lpbi->biClrUsed = (bits <= 8) ? 1<<bits : 0;

	DeleteDC(hdc) ;
	GlobalUnlock(hdib);

	return hdib ;
}

// Walk through our array of LPBI's and free them
static void FreeFrames(LPLPBI alpbi, int frames)
{
	int w;

	if (!alpbi[0])
		return;

  // Don't free a frame if it's a duplicate of the previous one
	for (w=0; w < frames; w++)
		if (alpbi[w] && alpbi[w] != alpbi[w-1])
			GlobalFreePtr(alpbi[w]);

	for (w=0; w < frames; w++)
		alpbi[w] = NULL;
}

// Fill an array of LPBI's with the frames for this movie
static void MakeFrames (LPLPBI alpbi, Image *images, int frames)
{
	HBITMAP hbitmap, hbitmapOld;
	HDC hdc, hdcMem;
	int i;
	RECT rc;

	hdc = GetDC (NULL);
	hdcMem = CreateCompatibleDC (NULL);

	hbitmap = CreateCompatibleBitmap (hdc, images[0].Width (), images[0].Height ());

	// Now walk through and make all the frames
	for ( i=0; i < frames; i++ )
  {
		hbitmapOld = SelectBitmap(hdcMem, hbitmap);
	
		// Fill the whole frame with white
		SetRect(&rc,0,0,images[0].Width (), images[0].Height ());
		FillRect(hdcMem,&rc,GetStockBrush(WHITE_BRUSH));

		SelectBitmap(hdcMem, hbitmapOld);

    // Make this into a DIB and stuff it into the array
		alpbi[i] = (LPBITMAPINFOHEADER)GlobalLock(MakeDib(hbitmap, images[i]));

		// For an error, just duplicate the last frame if we can
		if (alpbi[i] == NULL && i )
			alpbi[i] = alpbi[i-1] ;
	}

	// Select all the old objects back and delete resources
	DeleteBitmap(hbitmap) ;
	DeleteObject(hdcMem) ;
	ReleaseDC(NULL,hdc) ;
}

void SaveVideo(char* filename, Image *images, int count, float fps)
{
	AVISTREAMINFO strhdr;
	PAVIFILE pfile = NULL;
	PAVISTREAM ps = NULL, psCompressed = NULL;
	AVICOMPRESSOPTIONS opts;
	AVICOMPRESSOPTIONS FAR * aopts[1] = { &opts };
	LPBITMAPINFOHEADER *plpbi;
	_fmemset(&opts, 0, sizeof(opts));

	// first let's make sure we are running on 1.1
	WORD wVer = HIWORD(VideoForWindowsVersion());
	if (wVer < 0x010a)
	{
		SystemDoMessageBox("Video for Windows 1.1 or later required", MB_OK|MB_ICONSTOP);
		return;
	}

	AVIFileInit();

  plpbi = (LPBITMAPINFOHEADER*) malloc (count*sizeof (LPBITMAPINFOHEADER));
  MakeFrames (plpbi, images, count);

	if (AVIFileOpen(&pfile, filename, OF_WRITE | OF_CREATE, NULL) == AVIERR_OK)
	{
		// Fill in the header for the video stream.
		_fmemset(&strhdr, 0, sizeof(strhdr));
		strhdr.fccType = streamtypeVIDEO;
		strhdr.fccHandler = 0;
		strhdr.dwScale = 1;
/////////////// set FPS
		strhdr.dwRate = 15;	 // 15 fps
		strhdr.dwSuggestedBufferSize  = plpbi[0]->biSizeImage;
		SetRect(&strhdr.rcFrame, 0, 0, (int) plpbi[0]->biWidth, (int) plpbi[0]->biHeight);

		// And create the stream.
		if (AVIFileCreateStream(pfile, &ps, &strhdr) == AVIERR_OK)
		if (AVISaveOptions(NULL, 0, 1, &ps, (LPAVICOMPRESSOPTIONS FAR *) &aopts))
//		if (AVISaveOptions(AfxGetMainWnd()->m_hWnd, 0, 1, &ps, (LPAVICOMPRESSOPTIONS FAR *) &aopts))
		if (AVIMakeCompressedStream(&psCompressed, ps, &opts, NULL) == AVIERR_OK)
		if (AVIStreamSetFormat(psCompressed, 0, plpbi[0], plpbi[0]->biSize + plpbi[0]->biClrUsed * sizeof(RGBQUAD)) == AVIERR_OK)
		{
			float fPause = (float)Sys_ProfileLoadInt("Default", "AVI Pause", 100)/100;
			int time = (int)(fPause * 15);
///////////// set FPS
			time = 1;
		
			for (int i = 0; i < count; i++) 
			{
				if (AVIStreamWrite(psCompressed, i * time, 1, 
					(LPBYTE) plpbi[i] +	plpbi[i]->biSize + plpbi[i]->biClrUsed * sizeof(RGBQUAD),
					plpbi[i]->biSizeImage, i%5 ? 0 : AVIIF_KEYFRAME, NULL, NULL) != AVIERR_OK)
					break;
			}
		}
	}
	
  FreeFrames (plpbi, count);

	// Now close the file
	if (ps) AVIStreamClose(ps);
	if (psCompressed) AVIStreamClose(psCompressed);
	if (pfile) AVIFileClose(pfile);
	AVIFileExit();
}
#else
void SaveVideo(char* filename, Image *images, int count, float fps)
{
  //	SystemDoMessageBox("Format not supported under this platform.", LC_MB_OK|LC_MB_ERROR);
}
#endif
