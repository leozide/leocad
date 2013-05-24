#include "lc_global.h"
#include "image.h"
#include "lc_file.h"

static void copyToQImage(const Image& src, QImage& dest)
{
	dest = QImage(src.m_nWidth, src.m_nHeight, src.m_bAlpha ? QImage::Format_ARGB32 : QImage::Format_RGB32);

	lcuint8* bytes = (lcuint8*)src.m_pData;

	if (src.m_bAlpha)
	{
		for (int y = 0; y < src.m_nHeight; y++)
		{
			for (int x = 0; x < src.m_nWidth; x++)
			{
				dest.setPixel(x, y, qRgba(bytes[0], bytes[1], bytes[2], bytes[3]));
				bytes += 4;
			}
		}
	}
	else
	{
		for (int y = 0; y < src.m_nHeight; y++)
		{
			for (int x = 0; x < src.m_nWidth; x++)
			{
				dest.setPixel(x, y, qRgb(bytes[0], bytes[1], bytes[2]));
				bytes += 3;
			}
		}
	}
}

static void copyFromQImage(const QImage& src, Image& dest)
{
	dest.Allocate(src.width(), src.height(), src.hasAlphaChannel());

	lcuint8* bytes = (lcuint8*)dest.m_pData;

	for (int y = 0; y < dest.m_nHeight; y++)
	{
		for (int x = 0; x < dest.m_nWidth; x++)
		{
			QRgb pixel = src.pixel(x, y);

			*bytes++ = qRed(pixel);
			*bytes++ = qGreen(pixel);
			*bytes++ = qBlue(pixel);

			if (dest.m_bAlpha)
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

// TODO: transparency
bool Image::FileSave(lcMemFile& File, LC_IMAGE_FORMAT Format, bool Transparent, unsigned char* BackgroundColor) const
{
	QImage image;

	copyToQImage(*this, image);

	QByteArray byteArray;
	QBuffer buffer(&byteArray);
	buffer.open(QIODevice::WriteOnly);

	const char* formatString;

	switch (Format)
	{
	case LC_IMAGE_BMP:
		formatString = "BMP";
		break;
	case LC_IMAGE_JPG:
		formatString = "JPG";
		break;
	default:
	case LC_IMAGE_PNG:
		formatString = "PNG";
		break;
	}

	if (!image.save(&buffer, formatString))
		return false;

	File.WriteBuffer(byteArray.constData(), byteArray.size());

	return true;
}

bool Image::FileSave(const char* FileName, LC_IMAGE_FORMAT Format, bool Transparent, unsigned char* BackgroundColor) const
{
	QImage image;

	copyToQImage(*this, image);

	const char* formatString;

	switch (Format)
	{
	case LC_IMAGE_BMP:
		formatString = "BMP";
		break;
	case LC_IMAGE_JPG:
		formatString = "JPG";
		break;
	default:
	case LC_IMAGE_PNG:
		formatString = "PNG";
		break;
	}

	char name[LC_MAXPATH], ext[5], *p;
	bool needext = false;

	strcpy(name, FileName);
	p = name + strlen(name) - 1;

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
			strcpy(ext, p+1);
			strlwr(ext);

			if (strcmp(ext, "bmp") == 0)
				Format = LC_IMAGE_BMP;
			else if (strcmp(ext, "jpg") == 0)
				Format = LC_IMAGE_JPG;
			else if (strcmp(ext, "jpeg") == 0)
				Format = LC_IMAGE_JPG;
			else if (strcmp(ext, "png") == 0)
				Format = LC_IMAGE_PNG;
			else
				needext = true;
		}
	}

	if (needext)
	{
		switch (Format)
		{
		case LC_IMAGE_BMP:
			strcat (name, ".bmp");
			break;
		case LC_IMAGE_JPG:
			strcat (name, ".jpg");
			break;
		default:
		case LC_IMAGE_PNG:
			strcat (name, ".png");
			break;
		}
	}

	return image.save(name, formatString);
}
