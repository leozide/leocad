#ifndef _IMAGE_H_
#define _IMAGE_H_

// Image Options
#define LC_IMAGE_TRANSPARENT	0x2000
#define LC_IMAGE_MASK		0x7000

enum LC_IMAGE_FORMAT
{
	LC_IMAGE_BMP,
	LC_IMAGE_JPG,
	LC_IMAGE_PNG
};

enum lcPixelFormat
{
	LC_PIXEL_FORMAT_INVALID,
	LC_PIXEL_FORMAT_A8,
	LC_PIXEL_FORMAT_L8A8,
	LC_PIXEL_FORMAT_R8G8B8,
	LC_PIXEL_FORMAT_R8G8B8A8
};

class Image
{
public:
	Image();
	virtual ~Image();

	int GetBPP() const;
	bool HasAlpha() const;

	bool FileLoad(lcMemFile& File);
	bool FileLoad(const char* FileName);

	void Resize(int Width, int Height);
	void ResizePow2();
	void Allocate(int Width, int Height, lcPixelFormat Format);
	void FreeData();

	int mWidth;
	int mHeight;
	lcPixelFormat mFormat;
	unsigned char* mData;
};

#endif // _IMAGE_H_
