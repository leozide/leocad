#include "lc_global.h"
#include "image.h"
#include "lc_file.h"

static void CopyFromQImage(const QImage& Src, Image& Dest)
{
	bool Alpha = Src.hasAlphaChannel();
	Dest.Allocate(Src.width(), Src.height(), Alpha ? LC_PIXEL_FORMAT_R8G8B8A8 : LC_PIXEL_FORMAT_R8G8B8);

	lcuint8* Bytes = (lcuint8*)Dest.mData;

	for (int y = 0; y < Dest.mHeight; y++)
	{
		for (int x = 0; x < Dest.mWidth; x++)
		{
			QRgb Pixel = Src.pixel(x, y);

			*Bytes++ = qRed(Pixel);
			*Bytes++ = qGreen(Pixel);
			*Bytes++ = qBlue(Pixel);

			if (Alpha)
				*Bytes++ = qAlpha(Pixel);
		}
	}
}

Image::Image()
{
	mData = NULL;
	mWidth = 0;
	mHeight = 0;
	mFormat = LC_PIXEL_FORMAT_INVALID;
}

Image::~Image()
{
	FreeData();
}

int Image::GetBPP() const
{
	switch (mFormat)
	{
	case LC_PIXEL_FORMAT_INVALID:
		return 0;
	case LC_PIXEL_FORMAT_A8:
		return 1;
	case LC_PIXEL_FORMAT_L8A8:
		return 2;
	case LC_PIXEL_FORMAT_R8G8B8:
		return 3;
	case LC_PIXEL_FORMAT_R8G8B8A8:
		return 4;
	}

	return 0;
}

bool Image::HasAlpha() const
{
	switch (mFormat)
	{
	case LC_PIXEL_FORMAT_INVALID:
		return false;
	case LC_PIXEL_FORMAT_A8:
		return true;
	case LC_PIXEL_FORMAT_L8A8:
		return true;
	case LC_PIXEL_FORMAT_R8G8B8:
		return false;
	case LC_PIXEL_FORMAT_R8G8B8A8:
		return true;
	}

	return 0;
}

void Image::FreeData()
{
	free(mData);
	mData = NULL;
	mWidth = 0;
	mHeight = 0;
	mFormat = LC_PIXEL_FORMAT_INVALID;
}

void Image::Allocate(int Width, int Height, lcPixelFormat Format)
{
	FreeData();

	mWidth = Width;
	mHeight = Height;
	mFormat = Format;
	mData = (unsigned char*)malloc(mWidth * mHeight * GetBPP());
}

void Image::ResizePow2()
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

void Image::Resize(int width, int height)
{
	int i, j, k, components, stx, sty;
	float accumx, accumy;
	unsigned char* bits;

	components = GetBPP();

	bits = (unsigned char*)malloc(width * height * components);

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

	free(mData);
	mData = bits;
	mWidth = width;
	mHeight = height;
}

bool Image::FileLoad(lcMemFile& File)
{
	QImage Image;

	unsigned char* Buffer = File.mBuffer + File.mPosition;
	int BufferLength = File.mFileSize - File.mPosition;

	if (!Image.loadFromData(Buffer, BufferLength))
		return false;

	CopyFromQImage(Image, *this);

	return true;
}

bool Image::FileLoad(const char* FileName)
{
	QImage Image;

	if (!Image.load(FileName))
		return false;

	CopyFromQImage(Image, *this);

	return true;
}
