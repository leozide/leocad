// Image I/O routines
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
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

  if ((strcmp(ext, "jpg") == 0) || (strcmp (ext, "jpeg") == 0))
    return ResizeImage(OpenJPG(filename));
  if (strcmp(ext, "bmp") == 0)
    return ResizeImage(OpenBMP(filename));
  if (strcmp(ext, "png") == 0)
    return ResizeImage(OpenPNG(filename));
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

  if ((strcmp (ext, "jpg") == 0) || (strcmp (ext, "jpeg") == 0))
    return SaveJPG(filename, image, opts->quality, opts->interlaced);

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

  if (strcmp (ext, "png") == 0)
    return SavePNG(filename, image, opts->transparent, opts->interlaced, opts->background);

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
//#include <windows.h>
//#include <vfw.h>

#define AVIIF_KEYFRAME	0x00000010L // this frame is a key frame.

void SaveVideo(char* filename, LC_IMAGE** images, int count, float fps)
{
/*
	AVISTREAMINFO strhdr;
	PAVIFILE pfile = NULL;
	PAVISTREAM ps = NULL, psCompressed = NULL;
	AVICOMPRESSOPTIONS opts;
	AVICOMPRESSOPTIONS FAR * aopts[1] = { &opts };
	_fmemset(&opts, 0, sizeof(opts));

	// first let's make sure we are running on 1.1
	WORD wVer = HIWORD(VideoForWindowsVersion());
	if (wVer < 0x010a)
	{
		AfxMessageBox("Video for Windows 1.1 or later required", MB_OK|MB_ICONSTOP);
		return;
	}

	AVIFileInit();

	if (AVIFileOpen(&pfile, fn, OF_WRITE | OF_CREATE, NULL) == AVIERR_OK)
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
		if (AVISaveOptions(AfxGetMainWnd()->m_hWnd, 0, 1, &ps, (LPAVICOMPRESSOPTIONS FAR *) &aopts))
		if (AVIMakeCompressedStream(&psCompressed, ps, &opts, NULL) == AVIERR_OK)
		if (AVIStreamSetFormat(psCompressed, 0, plpbi[0], plpbi[0]->biSize + plpbi[0]->biClrUsed * sizeof(RGBQUAD)) == AVIERR_OK)
		{
			float fPause = (float)AfxGetApp()->GetProfileInt("Default", "AVI Pause", 100)/100;
			int time = (int)(fPause * 15);
///////////// set FPS
			time = 1;
		
			for (int i = 0; i < nCount; i++) 
			{
				if (AVIStreamWrite(psCompressed, i * time, 1, 
					(LPBYTE) plpbi[i] +	plpbi[i]->biSize + plpbi[i]->biClrUsed * sizeof(RGBQUAD),
					plpbi[i]->biSizeImage, i%5 ? 0 : AVIIF_KEYFRAME, NULL, NULL) != AVIERR_OK)
					break;
			}
		}
	}
	
	// Now close the file
	if (ps) AVIStreamClose(ps);
	if (psCompressed) AVIStreamClose(psCompressed);
	if (pfile) AVIFileClose(pfile);
	AVIFileExit();
*/
}
#else
void SaveVideo(char* filename, LC_IMAGE** images, int count, float fps)
{
  //	SystemDoMessageBox("Format not supported under this platform.", LC_MB_OK|LC_MB_ERROR);
}
#endif
