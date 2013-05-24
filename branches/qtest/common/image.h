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

	bool FileSave(lcMemFile& File, LC_IMAGE_FORMAT Format, bool Transparent, unsigned char* BackgroundColor) const;
	bool FileSave(const char* FileName, LC_IMAGE_FORMAT Format, bool Transparent, unsigned char* BackgroundColor) const;
	bool FileLoad(lcMemFile& file);
	bool FileLoad(const char* filename);

	void Resize(int width, int height);
	void ResizePow2();
	void FromOpenGL(int width, int height);
	void Allocate(int width, int height, bool alpha);

	int Width() const
		{ return m_nWidth; }
	int Height() const
		{ return m_nHeight; }
	int Alpha() const
		{ return m_bAlpha; }
	unsigned char* GetData() const
		{ return m_pData; }

	int m_nWidth;
	int m_nHeight;
	bool m_bAlpha;
	unsigned char* m_pData;

protected:
	void FreeData();
};

void SaveVideo(char* filename, Image *images, int count, float fps);

#endif // _IMAGE_H_
