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
		GridImage.Allocate(GridSize, GridSize, lcPixelFormat::A8);

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
	Image Image;

	if (!Image.FileLoad(FileName))
		return false;

	SetImage(std::move(Image), Flags);

	return true;
}

bool lcTexture::Load(lcMemFile& File, int Flags)
{
	Image Image;

	if (!Image.FileLoad(File))
		return false;

	SetImage(std::move(Image), Flags);

	return true;
}

void lcTexture::SetImage(Image&& Image, int Flags)
{
	mImages.clear();
	mImages.emplace_back(std::move(Image));
	mFlags = Flags;

	LoadImages();
}

void lcTexture::SetImage(std::vector<Image>&& Images, int Flags)
{
	mImages = std::move(Images);
	mFlags = Flags;

	LoadImages();
}

void lcTexture::Upload(lcContext* Context)
{
	if (!NeedsUpload())
		return;

	mWidth = mImages[0].mWidth;
	mHeight = mImages[0].mHeight;

	Context->UploadTexture(this);

	mImages.clear();
}

bool lcTexture::LoadImages()
{
	for (Image& Image : mImages)
		Image.ResizePow2();

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
