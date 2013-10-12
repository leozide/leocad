#include "lc_global.h"
#include "lc_glwidget.h"
#include "texfont.h"
#include "lc_texture.h"
#include "image.h"
#include "opengl.h"

int lcGLWidget::mNumWidgets;
lcTexture* lcGLWidget::mGridTexture;
TexFont* lcGLWidget::mDefaultFont;

void lcGLWidget::InitializeGL()
{
	mNumWidgets++;

	if (mNumWidgets != 1)
		return;

	GL_InitializeExtensions(this);
	CreateGridTexture();

	mDefaultFont = new TexFont();
	mDefaultFont->Initialize();
}

void lcGLWidget::CreateGridTexture()
{
	mGridTexture = new lcTexture();

	const int NumLevels = 9;
	Image GridImages[NumLevels];

	for (int ImageLevel = 0; ImageLevel < NumLevels; ImageLevel++)
	{
		Image& GridImage = GridImages[ImageLevel];

		const int GridSize = 256 >> ImageLevel;
		const float Radius1 = (80 >> ImageLevel) * (80 >> ImageLevel);
		const float Radius2 = (72 >> ImageLevel) * (72 >> ImageLevel);

		GridImage.Allocate(GridSize, GridSize, LC_PIXEL_FORMAT_A8);
		lcuint8* BlurBuffer = new lcuint8[GridSize * GridSize];

		for (int y = 0; y < GridSize; y++)
		{
			lcuint8* Pixel = GridImage.mData + y * GridSize;
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
				lcuint8 a = GridImage.mData[x + y * GridSize];
				lcuint8 b = GridImage.mData[x + 1 + y * GridSize];
				lcuint8 c = GridImage.mData[x + (y + 1) * GridSize];
				lcuint8 d = GridImage.mData[x + 1 + (y + 1) * GridSize];
				BlurBuffer[x + y * GridSize] = (a + b + c + d) / 4;
			}

			int x = GridSize - 1;
			lcuint8 a = GridImage.mData[x + y * GridSize];
			lcuint8 c = GridImage.mData[x + (y + 1) * GridSize];
			BlurBuffer[x + y * GridSize] = (a + c) / 2;
		}

		int y = GridSize - 1;
		for (int x = 0; x < GridSize - 1; x++)
		{
			lcuint8 a = GridImage.mData[x + y * GridSize];
			lcuint8 b = GridImage.mData[x + 1 + y * GridSize];
			BlurBuffer[x + y * GridSize] = (a + b) / 2;
		}

		int x = GridSize - 1;
		BlurBuffer[x + y * GridSize] = GridImage.mData[x + y * GridSize];

		memcpy(GridImage.mData, BlurBuffer, GridSize * GridSize);
		delete[] BlurBuffer;
	}

	mGridTexture->Load(GridImages, NumLevels, LC_TEXTURE_WRAPU | LC_TEXTURE_WRAPV | LC_TEXTURE_MIPMAPS | LC_TEXTURE_ANISOTROPIC);
}

void lcGLWidget::ShutdownGL()
{
	mNumWidgets--;

	if (mNumWidgets != 0)
		return;

	delete mDefaultFont;
	mDefaultFont = NULL;

	delete mGridTexture;
	mGridTexture = NULL;
}
