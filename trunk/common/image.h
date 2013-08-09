#ifndef _IMAGE_H_
#define _IMAGE_H_

// Image Options
//#define LC_IMAGE_PROGRESSIVE	0x1000
#define LC_IMAGE_TRANSPARENT	0x2000
//#define LC_IMAGE_HIGHCOLOR	0x4000
#define LC_IMAGE_MASK		0x7000

enum LC_IMAGE_FORMAT
{
	LC_IMAGE_BMP,
	LC_IMAGE_JPG,
	LC_IMAGE_PNG
};

class Image
{
public:
	Image();
	virtual ~Image();

	bool FileSave(lcMemFile& File, LC_IMAGE_FORMAT Format, bool Transparent) const;
	bool FileSave(const char* FileName, LC_IMAGE_FORMAT Format, bool Transparent) const;
	bool FileLoad(lcMemFile& File);
	bool FileLoad(const char* FileName);

	void Resize(int Width, int Height);
	void ResizePow2();
	void FromOpenGL(int Width, int Height);
	void Allocate(int Width, int Height, bool Alpha);
	void FreeData();

	int mWidth;
	int mHeight;
	bool mAlpha;
	unsigned char* mData;
};

#endif // _IMAGE_H_
