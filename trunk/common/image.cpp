#include "lc_global.h"
#include "image.h"
#include "opengl.h"

Image::Image ()
{
	mData = NULL;
	mWidth = 0;
	mHeight = 0;
	mAlpha = false;
}

Image::~Image ()
{
	FreeData();
}

void Image::FreeData ()
{
	free(mData);
	mData = NULL;
	mWidth = 0;
	mHeight = 0;
	mAlpha = false;
}

void Image::Allocate(int Width, int Height, bool Alpha)
{
  FreeData ();

	mWidth = Width;
	mHeight = Height;
	mAlpha = Alpha;

	if (mAlpha)
		mData = (unsigned char*)malloc(mWidth * mHeight * 4);
  else
		mData = (unsigned char*)malloc(mWidth * mHeight * 3);
}

void Image::ResizePow2 ()
{
  int i, shifted_x, shifted_y;

	shifted_x = mWidth;
  for (i = 0; ((i < 16) && (shifted_x != 0)); i++)
    shifted_x = shifted_x >> 1;
  shifted_x = (i != 0) ? 1 << (i-1) : 1;

	shifted_y = mHeight;
  for (i = 0; ((i < 16) && (shifted_y != 0)); i++)
    shifted_y = shifted_y >> 1;
  shifted_y = (i != 0) ? 1 << (i-1) : 1;

	if ((shifted_x != mWidth) || (shifted_y != mHeight))
    Resize (shifted_x, shifted_y);
}

void Image::Resize (int width, int height)
{
  int i, j, k, components, stx, sty;
  float accumx, accumy;
  unsigned char* bits;

	if (mAlpha)
    components = 4;
  else
    components = 3;

  bits = (unsigned char*)malloc (width * height * components);

	for (j = 0; j < mHeight; j++)
  {
		accumy = (float)height*j/(float)mHeight;
    sty = (int)floor(accumy);

		for (i = 0; i < mWidth; i++)
    {
			accumx = (float)width*i/(float)mWidth;
      stx = (int)floor(accumx);

      for (k = 0; k < components; k++)
				bits[(stx+sty*width)*components+k] = mData[(i+j*mWidth)*components+k];
    }
  }

	free (mData);
	mData = bits;
	mWidth = width;
	mHeight = height;
}

void Image::FromOpenGL(int Width, int Height)
{
	Allocate(Width, Height, true);

	lcuint8* Buffer = (lcuint8*)malloc(Width * Height * 4);

  glPixelStorei (GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, Buffer);

	for (int Row = 0; Row < Height; Row++)
		memcpy(mData + (Row * Width * 4), Buffer + ((Height - Row - 1) * Width * 4), Width * 4);

	free(Buffer);
}
