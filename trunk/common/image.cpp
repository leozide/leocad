// Image I/O routines
//

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
// Function declarations (functions from the im_xxx.cpp files)

LC_IMAGE* OpenJPG (char* filename);
LC_IMAGE* OpenBMP (char* filename);
LC_IMAGE* OpenPNG (char* filename);
LC_IMAGE* OpenGIF (File* file);

bool SaveJPG (char* filename, LC_IMAGE* image, int quality, bool progressive);
bool SaveBMP (char* filename, LC_IMAGE* image, bool quantize);
bool SavePNG (char* filename, LC_IMAGE* image, bool transparent, bool interlaced, unsigned char* background);
bool SaveGIF (File* file, LC_IMAGE* image, bool transparent, bool interlaced, unsigned char* background);

// =============================================================================
// Static functions

static LC_IMAGE* ResizeImage(LC_IMAGE* image)
{
  int i, j;
  long shifted_x, shifted_y;
  if (image == NULL)
    return NULL;

  shifted_x = image->width;
  for (i = 0; ((i < 16) && (shifted_x != 0)); i++)
    shifted_x = shifted_x >> 1;
  shifted_x = (i != 0) ? 1 << (i-1) : 1;

  shifted_y = image->height;
  for (i = 0; ((i < 16) && (shifted_y != 0)); i++)
    shifted_y = shifted_y >> 1;
  shifted_y = (i != 0) ? 1 << (i-1) : 1;

  if ((shifted_x == image->width) && (shifted_y == image->height))
    return image;

  LC_IMAGE* newimage = (LC_IMAGE*)malloc(shifted_x*shifted_y*3+sizeof(LC_IMAGE));
  newimage->width = (unsigned short)shifted_x;
  newimage->height = (unsigned short)shifted_y;
  newimage->bits = (unsigned char*)newimage + sizeof(LC_IMAGE);
  memset(newimage->bits, 0, shifted_x*shifted_y*3);

  float accumx, accumy;
  int stx, sty;
  unsigned char *oldbits = (unsigned char*)image->bits,
                *newbits = (unsigned char*)newimage->bits;

  for (j = 0; j < image->height; j++)
  {
    accumy = (float)newimage->height*j/(float)image->height;
    sty = (int)floor(accumy);

    for (i = 0; i < image->width; i++)
    {
      accumx = (float)newimage->width*i/(float)image->width;
      stx = (int)floor(accumx);

      newbits[(stx+sty*newimage->width)*3] = oldbits[(i+j*image->width)*3];
      newbits[(stx+sty*newimage->width)*3+1] = oldbits[(i+j*image->width)*3+1];
      newbits[(stx+sty*newimage->width)*3+2] = oldbits[(i+j*image->width)*3+2];
    }
  }

  free(image);
  return newimage;
}

// =============================================================================
// Global functions

// Reads a file from disk
LC_IMAGE* OpenImage(char* filename)
{
  char ext[5];
  if (strlen(filename) != 0)
  {
    char *p = strrchr(filename, '.');
    if (p != NULL)
      strcpy (ext, p+1);
  }
  strlwr(ext);

#ifdef LC_HAVE_JPEGLIB
  if ((strcmp(ext, "jpg") == 0) || (strcmp (ext, "jpeg") == 0))
    return ResizeImage(OpenJPG(filename));
#endif

  if (strcmp(ext, "bmp") == 0)
    return ResizeImage(OpenBMP(filename));

#ifdef LC_HAVE_PNGLIB
  if (strcmp(ext, "png") == 0)
    return ResizeImage(OpenPNG(filename));
#endif

  if ((strcmp (ext, "gif") == 0) || (strcmp (ext, "tmp") == 0))
  {
    FileDisk file;
    if (!file.Open(filename, "rb"))
      return NULL;
    LC_IMAGE* image = ResizeImage(OpenGIF(&file));
    file.Close();
    return image;
  }

//	MessageBox (NULL, "Unknown File Format", "Error", MB_ICONSTOP);

  return NULL;
}

LC_IMAGE* OpenImage(File* file, unsigned char format)
{
  if (format != LC_IMAGE_GIF)
    return NULL;
  return OpenGIF(file);
}

bool SaveImage(char* filename, LC_IMAGE* image, LC_IMAGE_OPTS* opts)
{
  char ext[5];
  if (strlen(filename) != 0)
  {
    char *p = strrchr(filename, '.');
    if (p != NULL)
      strcpy(ext, p+1);
  }
  strlwr(ext);

#ifdef LC_HAVE_JPEGLIB
  if ((strcmp (ext, "jpg") == 0) || (strcmp (ext, "jpeg") == 0))
    return SaveJPG(filename, image, opts->quality, opts->interlaced);
#endif

  if (strcmp (ext, "gif") == 0)
  {
    FileDisk file;
    if (!file.Open(filename, "wb"))
      return false;

    bool ret = SaveGIF(&file, image, opts->transparent, opts->interlaced, opts->background);
    file.Close();
    return ret;
  }

  if (strcmp (ext, "bmp") == 0)
    return SaveBMP(filename, image, opts->truecolor == false);

#ifdef LC_HAVE_PNGLIB
  if (strcmp (ext, "png") == 0)
    return SavePNG(filename, image, opts->transparent, opts->interlaced, opts->background);
#endif

//	MessageBox (NULL, "Could not save file", "Error", MB_ICONSTOP);

  return false;
}

bool SaveImage(File* file, LC_IMAGE* image, LC_IMAGE_OPTS* opts)
{
  if (opts->format != LC_IMAGE_GIF)
    return false;
  return SaveGIF(file, image, opts->transparent, opts->interlaced, opts->background);
}



#ifdef LC_WINDOWS
#include "system.h"

#define AVIIF_KEYFRAME	0x00000010L // this frame is a key frame.
#define LPLPBI	LPBITMAPINFOHEADER *

static HANDLE  MakeDib (HBITMAP hbitmap, LC_IMAGE *image)
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
    unsigned char *src = (unsigned char*)image->bits + i * image->width * 3;
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
static void MakeFrames (LPLPBI alpbi, LC_IMAGE **images, int frames)
{
	HBITMAP hbitmap, hbitmapOld;
	HDC hdc, hdcMem;
	int i;
	RECT rc;

	hdc = GetDC (NULL);
	hdcMem = CreateCompatibleDC (NULL);

	hbitmap = CreateCompatibleBitmap (hdc, images[0]->width, images[0]->height);

	// Now walk through and make all the frames
	for ( i=0; i < frames; i++ )
  {
		hbitmapOld = SelectBitmap(hdcMem, hbitmap);
	
		// Fill the whole frame with white
		SetRect(&rc,0,0,images[0]->width,images[0]->height) ;
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

void SaveVideo(char* filename, LC_IMAGE** images, int count, float fps)
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
void SaveVideo(char* filename, LC_IMAGE** images, int count, float fps)
{
  //	SystemDoMessageBox("Format not supported under this platform.", LC_MB_OK|LC_MB_ERROR);
}
#endif
