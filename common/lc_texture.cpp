#include "lc_global.h"
#include "lc_texture.h"
#include "lc_file.h"
#include "lc_application.h"
#include "lc_library.h"
#include "image.h"

lcTexture::lcTexture()
{
	mTexture = 0;
	mRefCount = 0;
}

lcTexture::~lcTexture()
{
	Unload();
}

bool lcTexture::Load()
{
	return lcGetPiecesLibrary()->LoadTexture(this);
}

bool lcTexture::Load(const char* FileName, int Flags)
{
	lcDiskFile File;

	if (!File.Open(FileName, "rb"))
		return false;

	return Load(File, Flags);
}

bool lcTexture::Load(lcFile& File, int Flags)
{
	Image image;

	if (!image.FileLoad(File))
		return false;

	image.ResizePow2();

	mWidth = image.Width();
	mHeight = image.Height();

	glGenTextures(1, &mTexture);

	glBindTexture(GL_TEXTURE_2D, mTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (Flags & LC_TEXTURE_WRAPU) ? GL_REPEAT : GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (Flags & LC_TEXTURE_WRAPV) ? GL_REPEAT : GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (Flags & LC_TEXTURE_MIPMAPS) ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	int Format = image.Alpha() ? GL_RGBA : GL_RGB;
	void* Data = image.GetData();

	glTexImage2D(GL_TEXTURE_2D, 0, image.Alpha() ? GL_RGBA : GL_RGB, mWidth, mHeight, 0, Format, GL_UNSIGNED_BYTE, Data);

	if (Flags & LC_TEXTURE_MIPMAPS)
	{
		int Width = mWidth;
		int Height = mHeight;
		int Components = (Format == GL_RGBA) ? 4 : 3;

		for (int Level = 1; ((Width != 1) || (Height != 1)); Level++)
		{
			GLubyte *Out, *In;
			int RowStride = Width * Components;

			Width = lcMax(1, Width >> 1);
			Height = lcMax(1, Height >> 1);

			In = Out = (GLubyte*)Data;

			for (int y = 0; y < Height; y++, In += RowStride)
				for (int x = 0; x < Width; x++, Out += Components, In += 2 * Components)
					for (int c = 0; c < Components; c++)
						Out[c] = (In[c] + In[c + Components] + In[RowStride] + In[c + RowStride + Components]) / 4;

			glTexImage2D(GL_TEXTURE_2D, Level, Components, Width, Height, 0, Format, GL_UNSIGNED_BYTE, Data);
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

void lcTexture::Unload()
{
	glDeleteTextures(1, &mTexture);
	mTexture = 0;
}
