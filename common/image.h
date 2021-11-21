#pragma once

// Image Options
#define LC_IMAGE_TRANSPARENT	0x2000
//#define LC_IMAGE_MASK		0x7000

enum class lcPixelFormat
{
	Invalid,
	A8,
	L8A8,
	R8G8B8,
	R8G8B8A8
};

class Image
{
public:
	Image();
	Image(Image&& Other);
	~Image();

	Image(const Image&) = delete;
	Image& operator=(const Image&) = delete;
	Image& operator=(Image&&) = delete;

	int GetBPP() const;
	bool HasAlpha() const;

	bool FileLoad(lcMemFile& File);
	bool FileLoad(const QString& FileName);

	void Resize(int Width, int Height);
	void ResizePow2();
	void Allocate(int Width, int Height, lcPixelFormat Format);
	void FreeData();

	int mWidth;
	int mHeight;
	lcPixelFormat mFormat;
	unsigned char* mData;
};

