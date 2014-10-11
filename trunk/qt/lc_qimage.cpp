#include "lc_global.h"
#include "image.h"
#include "lc_file.h"
#include "system.h"

static void copyFromQImage(const QImage& src, Image& dest)
{
	bool alpha = src.hasAlphaChannel();
	dest.Allocate(src.width(), src.height(), alpha ? LC_PIXEL_FORMAT_R8G8B8A8 : LC_PIXEL_FORMAT_R8G8B8);

	lcuint8* bytes = (lcuint8*)dest.mData;

	for (int y = 0; y < dest.mHeight; y++)
	{
		for (int x = 0; x < dest.mWidth; x++)
		{
			QRgb pixel = src.pixel(x, y);

			*bytes++ = qRed(pixel);
			*bytes++ = qGreen(pixel);
			*bytes++ = qBlue(pixel);

			if (alpha)
				*bytes++ = qAlpha(pixel);
		}
	}
}

bool Image::FileLoad(lcMemFile& File)
{
	QImage image;

	unsigned char* buffer = File.mBuffer + File.mPosition;
	int bufferLength = File.mFileSize - File.mPosition;

	if (!image.loadFromData(buffer, bufferLength))
		return false;

	copyFromQImage(image, *this);

	return true;
}

bool Image::FileLoad(const char* FileName)
{
	QImage image;

	if (!image.load(FileName))
		return false;

	copyFromQImage(image, *this);

	return true;
}
