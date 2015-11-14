#include "lc_global.h"
#include "lc_texture.h"
#include "lc_file.h"
#include "lc_application.h"
#include "lc_library.h"
#include "image.h"
#include "system.h"
#include "lc_glextensions.h"

lcTexture* gGridTexture;

lcTexture* lcLoadTexture(const QString& FileName, int Flags)
{
	lcTexture* Texture = new lcTexture();

	if (!Texture->Load(FileName.toLatin1().constData(), Flags)) // todo: qstring
	{
		delete Texture;
		Texture = NULL;
	}

	return Texture;
}

void lcReleaseTexture(lcTexture* Texture)
{
	if (Texture && Texture->Release() == 0)
		delete Texture;
}

lcTexture::lcTexture()
{
	mTexture = 0;
	mRefCount = 0;
}

lcTexture::~lcTexture()
{
	Unload();
}

void lcTexture::CreateGridTexture()
{
	const int NumLevels = 9;
	Image GridImages[NumLevels];
	lcuint8* Previous = NULL;

	for (int ImageLevel = 0; ImageLevel < NumLevels; ImageLevel++)
	{
		Image& GridImage = GridImages[ImageLevel];
		const int GridSize = 256 >> ImageLevel;
		GridImage.Allocate(GridSize, GridSize, LC_PIXEL_FORMAT_A8);

		if (Previous)
		{
			int PreviousGridSize = 2 * GridSize;

			for (int y = 0; y < GridSize - 1; y++)
			{
				for (int x = 0; x < GridSize - 1; x++)
				{
					lcuint8 a = Previous[x * 2 + y * 2 * PreviousGridSize] > 64 ? 255 : 0;
					lcuint8 b = Previous[x * 2 + 1 + y * 2 * PreviousGridSize] > 64 ? 255 : 0;
					lcuint8 c = Previous[x * 2 + (y * 2 + 1) * PreviousGridSize] > 64 ? 255 : 0;
					lcuint8 d = Previous[x * 2 + 1 + (y * 2 + 1) * PreviousGridSize] > 64 ? 255 : 0;
					GridImage.mData[x + y * GridSize] = (a + b + c + d) / 4;
				}

				int x = GridSize - 1;
				lcuint8 a = Previous[x * 2 + y * 2 * PreviousGridSize];
				lcuint8 c = Previous[x * 2 + (y * 2 + 1) * PreviousGridSize];
				GridImage.mData[x + y * GridSize] = (a + c) / 2;
			}

			int y = GridSize - 1;
			for (int x = 0; x < GridSize - 1; x++)
			{
				lcuint8 a = Previous[x * 2 + y * 2 * PreviousGridSize];
				lcuint8 b = Previous[x * 2 + 1 + y * 2 * PreviousGridSize];
				GridImage.mData[x + y * GridSize] = (a + b) / 2;
			}

			int x = GridSize - 1;
			GridImage.mData[x + y * GridSize] = Previous[x + y * PreviousGridSize];
		}
		else
		{
			const float Radius1 = (80 >> ImageLevel) * (80 >> ImageLevel);
			const float Radius2 = (72 >> ImageLevel) * (72 >> ImageLevel);
			lcuint8* TempBuffer = new lcuint8[GridSize * GridSize];

			for (int y = 0; y < GridSize; y++)
			{
				lcuint8* Pixel = TempBuffer + y * GridSize;
				memset(Pixel, 0, GridSize);

				const float y2 = (y - GridSize / 2) * (y - GridSize / 2);

				if (Radius1 <= y2)
					continue;

				if (Radius2 <= y2)
				{
					int x1 = sqrtf(Radius1 - y2);

					for (int x = GridSize / 2 - x1; x < GridSize / 2 + x1; x++)
						Pixel[x] = 255;
				}
				else
				{
					int x1 = sqrtf(Radius1 - y2);
					int x2 = sqrtf(Radius2 - y2);

					for (int x = GridSize / 2 - x1; x < GridSize / 2 - x2; x++)
						Pixel[x] = 255;

					for (int x = GridSize / 2 + x2; x < GridSize / 2 + x1; x++)
						Pixel[x] = 255;
				}
			}

			for (int y = 0; y < GridSize - 1; y++)
			{
				for (int x = 0; x < GridSize - 1; x++)
				{
					lcuint8 a = TempBuffer[x + y * GridSize];
					lcuint8 b = TempBuffer[x + 1 + y * GridSize];
					lcuint8 c = TempBuffer[x + (y + 1) * GridSize];
					lcuint8 d = TempBuffer[x + 1 + (y + 1) * GridSize];
					GridImage.mData[x + y * GridSize] = (a + b + c + d) / 4;
				}

				int x = GridSize - 1;
				lcuint8 a = TempBuffer[x + y * GridSize];
				lcuint8 c = TempBuffer[x + (y + 1) * GridSize];
				GridImage.mData[x + y * GridSize] = (a + c) / 2;
			}

			int y = GridSize - 1;
			for (int x = 0; x < GridSize - 1; x++)
			{
				lcuint8 a = TempBuffer[x + y * GridSize];
				lcuint8 b = TempBuffer[x + 1 + y * GridSize];
				GridImage.mData[x + y * GridSize] = (a + b) / 2;
			}

			int x = GridSize - 1;
			GridImage.mData[x + y * GridSize] = TempBuffer[x + y * GridSize];
			delete[] TempBuffer;
		}

		Previous = GridImage.mData;
	}

	Load(GridImages, NumLevels, LC_TEXTURE_WRAPU | LC_TEXTURE_WRAPV | LC_TEXTURE_MIPMAPS | LC_TEXTURE_ANISOTROPIC);

    mRefCount = 1;
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

	if (gSupportsAnisotropic && FilterFlags == LC_TEXTURE_ANISOTROPIC)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, lcMin(4.0f, gMaxAnisotropy));

	int Format;
	switch (images[0].mFormat)
	{
    default:
    case LC_PIXEL_FORMAT_INVALID:
        LC_ASSERT(false);
        Format = 0;
        break;
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
			int RowStride = Width * Components;

			Width = lcMax(1, Width >> 1);
			Height = lcMax(1, Height >> 1);

			if (NumLevels == 1)
			{
				GLubyte *Out, *In;

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
