#include "lc_global.h"
#include "lc_texture.h"
#include "lc_application.h"
#include "lc_library.h"
#include "image.h"
#include "lc_glextensions.h"

lcTexture* gGridTexture;

lcTexture* lcLoadTexture(const QString& FileName, int Flags)
{
	lcTexture* Texture = new lcTexture();

	if (!Texture->Load(FileName, Flags))
	{
		delete Texture;
		Texture = nullptr;
	}
	else
	{
		strcpy(Texture->mName, QFileInfo(FileName).baseName().toLatin1());
		Texture->SetTemporary(true);
	}

	return Texture;
}

void lcReleaseTexture(lcTexture* Texture)
{
	if (Texture && !Texture->Release())
		delete Texture;
}

lcTexture::lcTexture()
{
	mTexture = 0;
	mRefCount = 0;
	mTemporary = false;
}

lcTexture::~lcTexture()
{
	Unload();
}

void lcTexture::CreateGridTexture()
{
	constexpr int NumLevels = 9;
	mImages.resize(NumLevels);
	quint8* Previous = nullptr;

	for (int ImageLevel = 0; ImageLevel < NumLevels; ImageLevel++)
	{
		Image& GridImage = mImages[ImageLevel];
		const int GridSize = 256 >> ImageLevel;
		GridImage.Allocate(GridSize, GridSize, LC_PIXEL_FORMAT_A8);

		if (Previous)
		{
			const int PreviousGridSize = 2 * GridSize;

			for (int y = 0; y < GridSize - 1; y++)
			{
				for (int x = 0; x < GridSize - 1; x++)
				{
					const quint8 a = Previous[x * 2 + y * 2 * PreviousGridSize] > 64 ? 255 : 0;
					const quint8 b = Previous[x * 2 + 1 + y * 2 * PreviousGridSize] > 64 ? 255 : 0;
					const quint8 c = Previous[x * 2 + (y * 2 + 1) * PreviousGridSize] > 64 ? 255 : 0;
					const quint8 d = Previous[x * 2 + 1 + (y * 2 + 1) * PreviousGridSize] > 64 ? 255 : 0;
					GridImage.mData[x + y * GridSize] = (a + b + c + d) / 4;
				}

				int x = GridSize - 1;
				const quint8 a = Previous[x * 2 + y * 2 * PreviousGridSize];
				const quint8 c = Previous[x * 2 + (y * 2 + 1) * PreviousGridSize];
				GridImage.mData[x + y * GridSize] = (a + c) / 2;
			}

			int y = GridSize - 1;
			for (int x = 0; x < GridSize - 1; x++)
			{
				const quint8 a = Previous[x * 2 + y * 2 * PreviousGridSize];
				const quint8 b = Previous[x * 2 + 1 + y * 2 * PreviousGridSize];
				GridImage.mData[x + y * GridSize] = (a + b) / 2;
			}

			int x = GridSize - 1;
			GridImage.mData[x + y * GridSize] = Previous[x + y * PreviousGridSize];
		}
		else
		{
			const float Radius1 = (80 >> ImageLevel) * (80 >> ImageLevel);
			const float Radius2 = (72 >> ImageLevel) * (72 >> ImageLevel);
			quint8* TempBuffer = new quint8[GridSize * GridSize];

			for (int y = 0; y < GridSize; y++)
			{
				quint8* Pixel = TempBuffer + y * GridSize;
				memset(Pixel, 0, GridSize);

				const float y2 = (y - GridSize / 2) * (y - GridSize / 2);

				if (Radius1 <= y2)
					continue;

				if (Radius2 <= y2)
				{
					const int x1 = sqrtf(Radius1 - y2);

					for (int x = GridSize / 2 - x1; x < GridSize / 2 + x1; x++)
						Pixel[x] = 255;
				}
				else
				{
					const int x1 = sqrtf(Radius1 - y2);
					const int x2 = sqrtf(Radius2 - y2);

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
					const quint8 a = TempBuffer[x + y * GridSize];
					const quint8 b = TempBuffer[x + 1 + y * GridSize];
					const quint8 c = TempBuffer[x + (y + 1) * GridSize];
					const quint8 d = TempBuffer[x + 1 + (y + 1) * GridSize];
					GridImage.mData[x + y * GridSize] = (a + b + c + d) / 4;
				}

				int x = GridSize - 1;
				const quint8 a = TempBuffer[x + y * GridSize];
				const quint8 c = TempBuffer[x + (y + 1) * GridSize];
				GridImage.mData[x + y * GridSize] = (a + c) / 2;
			}

			int y = GridSize - 1;
			for (int x = 0; x < GridSize - 1; x++)
			{
				const quint8 a = TempBuffer[x + y * GridSize];
				const quint8 b = TempBuffer[x + 1 + y * GridSize];
				GridImage.mData[x + y * GridSize] = (a + b) / 2;
			}

			int x = GridSize - 1;
			GridImage.mData[x + y * GridSize] = TempBuffer[x + y * GridSize];
			delete[] TempBuffer;
		}

		Previous = GridImage.mData;
	}

	mRefCount = 1;
	mFlags = LC_TEXTURE_WRAPU | LC_TEXTURE_WRAPV | LC_TEXTURE_MIPMAPS | LC_TEXTURE_ANISOTROPIC;

	lcContext* Context = lcContext::GetGlobalOffscreenContext();
	Context->MakeCurrent();
	Upload(Context);
}

bool lcTexture::Load()
{
	return lcGetPiecesLibrary()->LoadTexture(this);
}

bool lcTexture::Load(const QString& FileName, int Flags)
{
	mImages.resize(1);

	if (!mImages[0].FileLoad(FileName))
		return false;

	return Load(Flags);
}

bool lcTexture::Load(lcMemFile& File, int Flags)
{
	mImages.resize(1);

	if (!mImages[0].FileLoad(File))
		return false;

	return Load(Flags);
}

void lcTexture::SetImage(Image* Image, int Flags)
{
	mImages.clear();
	mImages.emplace_back(std::move(*Image));

	Load(Flags);
}

void lcTexture::SetImage(std::vector<Image>&& Images, int Flags)
{
	mImages = std::move(Images);

	Load(Flags);
}

void lcTexture::Upload(lcContext* Context)
{
	mWidth = mImages[0].mWidth;
	mHeight = mImages[0].mHeight;

	if (!mTexture)
		glGenTextures(1, &mTexture);

	constexpr int Filters[2][5] =
	{
		{ GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR },
		{ GL_NEAREST, GL_LINEAR, GL_LINEAR, GL_LINEAR, GL_LINEAR  },
	};

	const int FilterFlags = mFlags & LC_TEXTURE_FILTER_MASK;
	const int FilterIndex = FilterFlags >> LC_TEXTURE_FILTER_SHIFT;
	const int MipIndex = mFlags & LC_TEXTURE_MIPMAPS ? 0 : 1;

    unsigned int Faces, Target;

	if ((mFlags & LC_TEXTURE_CUBEMAP) == 0)
	{
		Faces = 1;
		Target = GL_TEXTURE_2D;
		Context->BindTexture2D(mTexture);
	}
	else
	{
		Faces = 6;
		Target = GL_TEXTURE_CUBE_MAP;
		Context->BindTextureCubeMap(mTexture);
	}

	glTexParameteri(Target, GL_TEXTURE_WRAP_S, (mFlags & LC_TEXTURE_WRAPU) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(Target, GL_TEXTURE_WRAP_T, (mFlags & LC_TEXTURE_WRAPV) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri(Target, GL_TEXTURE_MIN_FILTER, Filters[MipIndex][FilterIndex]);
	glTexParameteri(Target, GL_TEXTURE_MAG_FILTER, Filters[1][FilterIndex]);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	if (gSupportsAnisotropic && FilterFlags == LC_TEXTURE_ANISOTROPIC)
		glTexParameterf(Target, GL_TEXTURE_MAX_ANISOTROPY_EXT, lcMin(4.0f, gMaxAnisotropy));

	int Format;
	switch (mImages[0].mFormat)
	{
    default:
    case LC_PIXEL_FORMAT_INVALID:
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

	int CurrentImage = 0;
	if (mFlags & LC_TEXTURE_CUBEMAP)
		Target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;

    for (size_t FaceIdx = 0; FaceIdx < Faces; FaceIdx++)
	{
		void* Data = mImages[CurrentImage].mData;
		glTexImage2D(Target, 0, Format, mWidth, mHeight, 0, Format, GL_UNSIGNED_BYTE, Data);

		if (mFlags & LC_TEXTURE_MIPMAPS || FilterFlags >= LC_TEXTURE_BILINEAR)
		{
			int Width = mWidth;
			int Height = mHeight;
			int Components = mImages[CurrentImage].GetBPP();

			for (int Level = 1; ((Width != 1) || (Height != 1)); Level++)
			{
				int RowStride = Width * Components;

				Width = lcMax(1, Width >> 1);
				Height = lcMax(1, Height >> 1);

				if (mImages.size() == Faces)
				{
					GLubyte *Out, *In;

					In = Out = (GLubyte*)Data;

					for (int y = 0; y < Height; y++, In += RowStride)
						for (int x = 0; x < Width; x++, Out += Components, In += 2 * Components)
							for (int c = 0; c < Components; c++)
								Out[c] = (In[c] + In[c + Components] + In[RowStride] + In[c + RowStride + Components]) / 4;
				}
				else
					Data = mImages[++CurrentImage].mData;

				glTexImage2D(Target, Level, Format, Width, Height, 0, Format, GL_UNSIGNED_BYTE, Data);
			}

			if (mImages.size() == Faces)
				CurrentImage++;
		}
		else
			CurrentImage++;

		Target++;
	}

	if ((mFlags & LC_TEXTURE_CUBEMAP) == 0)
		Context->UnbindTexture2D(mTexture);
	else
		Context->UnbindTextureCubeMap(mTexture);
}

bool lcTexture::Load(int Flags)
{
	for (Image& Image : mImages)
		Image.ResizePow2();
	mFlags = Flags;

	if (QThread::currentThread() == qApp->thread())
	{
		lcContext* Context = lcContext::GetGlobalOffscreenContext();
		Context->MakeCurrent();
		Upload(Context);
	}

	return true;
}

void lcTexture::Unload()
{
	if (mTexture)
		glDeleteTextures(1, &mTexture);
	mTexture = 0;
}
