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
	Image image;

	if (!image.FileLoad(FileName))
		return false;

	return Load(image, Flags);
}

bool lcTexture::Load(lcMemFile& File, int Flags)
{
	Image image;

	if (!image.FileLoad(File))
		return false;

	return Load(image, Flags);
}

bool lcTexture::Load(Image* images, int NumLevels, int Flags)
{
	mWidth = images[0].mWidth;
	mHeight = images[0].mHeight;

	glGenTextures(1, &mTexture);

	int Filters[2][5] = 
	{
		{ GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR },
		{ GL_NEAREST, GL_LINEAR, GL_LINEAR, GL_LINEAR, GL_LINEAR  },
	};

	int FilterFlags = Flags & LC_TEXTURE_FILTER_MASK;
	int FilterIndex = FilterFlags >> LC_TEXTURE_FILTER_SHIFT;
	int MipIndex = Flags & LC_TEXTURE_MIPMAPS ? 0 : 1;

	glBindTexture(GL_TEXTURE_2D, mTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (Flags & LC_TEXTURE_WRAPU) ? GL_REPEAT : GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (Flags & LC_TEXTURE_WRAPV) ? GL_REPEAT : GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Filters[MipIndex][FilterIndex]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Filters[1][FilterIndex]);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	if (GL_SupportsAnisotropic && FilterFlags == LC_TEXTURE_ANISOTROPIC)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, lcMin(4.0f, GL_MaxAnisotropy));

	int Format;
	switch (images[0].mFormat)
	{
	case LC_PIXEL_FORMAT_A8:
		Format = GL_ALPHA;
		break;
	case LC_PIXEL_FORMAT_L8A8:
		Format = GL_LUMINANCE_ALPHA;
		break;
	case LC_PIXEL_FORMAT_R8G8B8:
		Format = GL_RGB;
		break;
	case LC_PIXEL_FORMAT_R8G8B8A8:
		Format = GL_RGBA;
		break;
	}

	void* Data = images[0].mData;
	glTexImage2D(GL_TEXTURE_2D, 0, Format, mWidth, mHeight, 0, Format, GL_UNSIGNED_BYTE, Data);

	if (Flags & LC_TEXTURE_MIPMAPS)
	{
		int Width = mWidth;
		int Height = mHeight;
		int Components = images[0].GetBPP();

		for (int Level = 1; ((Width != 1) || (Height != 1)); Level++)
		{
			Width = lcMax(1, Width >> 1);
			Height = lcMax(1, Height >> 1);

			if (NumLevels == 1)
			{
				GLubyte *Out, *In;
				int RowStride = Width * Components;

				In = Out = (GLubyte*)Data;

				for (int y = 0; y < Height; y++, In += RowStride)
					for (int x = 0; x < Width; x++, Out += Components, In += 2 * Components)
						for (int c = 0; c < Components; c++)
							Out[c] = (In[c] + In[c + Components] + In[RowStride] + In[c + RowStride + Components]) / 4;
			}
			else
				Data = images[Level].mData;

			glTexImage2D(GL_TEXTURE_2D, Level, Format, Width, Height, 0, Format, GL_UNSIGNED_BYTE, Data);
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

bool lcTexture::Load(Image& image, int Flags)
{
	image.ResizePow2();

	return Load(&image, 1, Flags);
}

void lcTexture::Unload()
{
	if (mTexture)
		glDeleteTextures(1, &mTexture);
	mTexture = 0;
}
