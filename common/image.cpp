#include "lc_global.h"
#include "image.h"
#include "lc_file.h"

static void CopyFromQImage(const QImage& Src, Image& Dest)
{
	const bool Alpha = Src.hasAlphaChannel();
	Dest.Allocate(Src.width(), Src.height(), Alpha ? lcPixelFormat::R8G8B8A8 : lcPixelFormat::R8G8B8);

	quint8* Bytes = (quint8*)Dest.mData;

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
	mData = nullptr;
	mWidth = 0;
	mHeight = 0;
	mFormat = lcPixelFormat::Invalid;
}

Image::Image(Image&& Other)
{
	mData = Other.mData;
	mWidth = Other.mWidth;
	mHeight = Other.mHeight;
	mFormat = Other.mFormat;

	Other.mData = nullptr;
	Other.mWidth = 0;
	Other.mHeight = 0;
	Other.mFormat = lcPixelFormat::Invalid;
}

Image::~Image()
{
	FreeData();
}

int Image::GetBPP() const
{
	switch (mFormat)
	{
	case lcPixelFormat::Invalid:
		return 0;
	case lcPixelFormat::A8:
		return 1;
	case lcPixelFormat::L8A8:
		return 2;
	case lcPixelFormat::R8G8B8:
		return 3;
	case lcPixelFormat::R8G8B8A8:
		return 4;
	}

	return 0;
}

bool Image::HasAlpha() const
{
	switch (mFormat)
	{
	case lcPixelFormat::Invalid:
		return false;
	case lcPixelFormat::A8:
		return true;
	case lcPixelFormat::L8A8:
		return true;
	case lcPixelFormat::R8G8B8:
		return false;
	case lcPixelFormat::R8G8B8A8:
		return true;
	}

	return 0;
}

void Image::FreeData()
{
	free(mData);
	mData = nullptr;
	mWidth = 0;
	mHeight = 0;
	mFormat = lcPixelFormat::Invalid;
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
	unsigned char* bits = nullptr;

	components = GetBPP();
	const int BufferSize = width * height * components;

	if (BufferSize)
	{
		bits = (unsigned char*)malloc(BufferSize);

		if (bits)
		{
			for (j = 0; j < mHeight; j++)
			{
				accumy = (float)height*j / (float)mHeight;
				sty = (int)floor(accumy);

				for (i = 0; i < mWidth; i++)
				{
					accumx = (float)width*i / (float)mWidth;
					stx = (int)floor(accumx);

					for (k = 0; k < components; k++)
						bits[(stx + sty*width)*components + k] = mData[(i + j * mWidth) * components + k];
				}
			}
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

	const unsigned char* Buffer = File.mBuffer + File.mPosition;
	const size_t BufferLength = File.mFileSize - File.mPosition;

	if (!Image.loadFromData(Buffer, (int)BufferLength))
		return false;

	CopyFromQImage(Image, *this);

	return true;
}

bool Image::FileLoad(const QString& FileName)
{
	QImage Image;

	if (!Image.load(FileName))
		return false;

	CopyFromQImage(Image, *this);

	return true;
}
