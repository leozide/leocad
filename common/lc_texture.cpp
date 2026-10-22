#include "lc_global.h"
#include "lc_texture.h"
#include "lc_context.h"
#include "lc_glextensions.h"

lcTexture* gGridTexture;

lcTexture::lcTexture(int Flags)
    : mFlags(Flags)
{
}

lcTexture::~lcTexture()
{
	Unload();
}

void lcTexture::CreateGridTexture()
{
	constexpr int NumLevels = 9;
	mImages.resize(NumLevels);
	const QImage* Previous = nullptr;

	for (int ImageLevel = 0; ImageLevel < NumLevels; ImageLevel++)
	{
		QImage& GridImage = mImages[ImageLevel];
		const int GridSize = 256 >> ImageLevel;
		GridImage = QImage(GridSize, GridSize, QImage::Format_Alpha8);

		if (Previous)
		{
			for (int y = 0; y < GridSize - 1; y++)
			{
				for (int x = 0; x < GridSize - 1; x++)
				{
					const quint8 a = Previous->constScanLine(y * 2)[x * 2] > 64 ? 255 : 0;
					const quint8 b = Previous->constScanLine(y * 2)[x * 2 + 1] > 64 ? 255 : 0;
					const quint8 c = Previous->constScanLine(y * 2 + 1)[x * 2] > 64 ? 255 : 0;
					const quint8 d = Previous->constScanLine(y * 2 + 1)[x * 2 + 1] > 64 ? 255 : 0;
					GridImage.scanLine(y)[x] = (a + b + c + d) / 4;
				}

				int x = GridSize - 1;
				const quint8 a = Previous->constScanLine(y * 2)[x * 2];
				const quint8 c = Previous->constScanLine(y * 2 + 1)[x * 2];
				GridImage.scanLine(y)[x] = (a + c) / 2;
			}

			int y = GridSize - 1;
			for (int x = 0; x < GridSize - 1; x++)
			{
				const quint8 a = Previous->constScanLine(y * 2)[x * 2];
				const quint8 b = Previous->constScanLine(y * 2)[x * 2 + 1];
				GridImage.scanLine(y)[x] = (a + b) / 2;
			}

			int x = GridSize - 1;
			GridImage.scanLine(y)[x] = Previous->constScanLine(y)[x];
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
					GridImage.scanLine(y)[x] = (a + b + c + d) / 4;
				}

				int x = GridSize - 1;
				const quint8 a = TempBuffer[x + y * GridSize];
				const quint8 c = TempBuffer[x + (y + 1) * GridSize];
				GridImage.scanLine(y)[x] = (a + c) / 2;
			}

			int y = GridSize - 1;
			for (int x = 0; x < GridSize - 1; x++)
			{
				const quint8 a = TempBuffer[x + y * GridSize];
				const quint8 b = TempBuffer[x + 1 + y * GridSize];
				GridImage.scanLine(y)[x] = (a + b) / 2;
			}

			int x = GridSize - 1;
			GridImage.scanLine(y)[x] = TempBuffer[x + y * GridSize];
			delete[] TempBuffer;
		}

		Previous = &GridImage;
	}

	mRefCount = 1;
	mFlags = LC_TEXTURE_WRAPU | LC_TEXTURE_WRAPV | LC_TEXTURE_MIPMAPS | LC_TEXTURE_ANISOTROPIC;
	mState = lcAssetState::Loading;

	lcContext* Context = lcContext::GetGlobalOffscreenContext();
	Context->MakeCurrent();
	Upload(Context);
}

QImage lcTexture::PrepareImage(QImage Image)
{
	if (Image.isNull())
		return QImage();

	QImage::Format Format = QImage::Format_Alpha8;
	if (Image.format() != QImage::Format_Alpha8)
		Format = Image.hasAlphaChannel() ? QImage::Format_RGBA8888 : QImage::Format_RGB888;
	int Width = 1;
	int Height = 1;

	while (Width <= Image.width() / 2 && Width < 32768)
		Width *= 2;
	while (Height <= Image.height() / 2 && Height < 32768)
		Height *= 2;

	if (Width != Image.width() || Height != Image.height())
		Image = Image.scaled(Width, Height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

	return Image.convertToFormat(Format);
}

void lcTexture::SetImage(QImage SourceImage, int Flags)
{
	QImage TextureImage = PrepareImage(std::move(SourceImage));

	if (TextureImage.isNull())
	{
		SetFailed(lcTextureLoadError::DecodeFailed, QString());
		return;
	}

	mImages.clear();
	mImages.emplace_back(std::move(TextureImage));
	mFlags = Flags;
	mState = lcAssetState::Loading;
}

void lcTexture::DiscardDecodedImage()
{
	mImages.clear();
}

void lcTexture::SetFailed(lcTextureLoadError Error, QString Details)
{
	mState = lcAssetState::Failed;
	mLoadFailure = Error;
	mLoadFailureDetails = std::move(Details);
}

QString lcTexture::GetLoadError() const
{
	if (mState != lcAssetState::Failed)
		return QString();

	if (!mLoadFailureDetails.isEmpty())
		return mLoadFailureDetails;

	switch (mLoadFailure)
	{
	case lcTextureLoadError::DecodeFailed:
		return tr("Could not load texture '%1'.").arg(QString::fromLatin1(mName));

	case lcTextureLoadError::ContextActivationFailed:
		return tr("Could not make the shared GL context current for texture upload.");

	case lcTextureLoadError::ContextUnavailable:
		return tr("No shared GL context is available for texture upload.");

	case lcTextureLoadError::UploadFailed:
		return tr("Texture upload failed.");

	case lcTextureLoadError::None:
		return tr("Texture '%1' failed to load.").arg(QString::fromLatin1(mName));
	}

	return tr("Texture '%1' failed to load.").arg(QString::fromLatin1(mName));
}

void lcTexture::SetImages(std::vector<QImage>&& Images, int Flags)
{
	if (Images.empty())
	{
		SetFailed(lcTextureLoadError::DecodeFailed, QString());
		return;
	}

	for (QImage& Image : Images)
	{
		Image = PrepareImage(std::move(Image));

		if (Image.isNull())
		{
			SetFailed(lcTextureLoadError::DecodeFailed, QString());
			return;
		}
	}

	mImages = std::move(Images);
	mFlags = Flags;
	mState = lcAssetState::Loading;
}

void lcTexture::Upload(lcContext* Context)
{
	if (!NeedsUpload())
		return;

	mWidth = mImages[0].width();
	mHeight = mImages[0].height();

	const bool Uploaded = Context->UploadTexture(this);

	if (Uploaded && mTexture)
	{
		mImages.clear();
		mState = lcAssetState::Loaded;
		mLoadFailure = lcTextureLoadError::None;
		mLoadFailureDetails.clear();
	}
	else
	{
		if (mTexture)
		{
			QOpenGLContext::currentContext()->functions()->glDeleteTextures(1, &mTexture);

			mTexture = 0;
		}

		SetFailed(lcTextureLoadError::UploadFailed, QString());
	}
}

void lcTexture::Unload()
{
	if (mTexture)
	{
		QOpenGLContext* GLContext = QOpenGLContext::currentContext();

		if (GLContext)
			GLContext->functions()->glDeleteTextures(1, &mTexture);
		else if (qApp && QThread::currentThread() == qApp->thread())
		{
			if (lcContext* Context = lcContext::GetGlobalOffscreenContext())
			{
				Context->MakeCurrent();

				if (QOpenGLContext* OffscreenContext = QOpenGLContext::currentContext())
				{
					OffscreenContext->functions()->glDeleteTextures(1, &mTexture);
					OffscreenContext->doneCurrent();
				}
			}
		}
	}

	mTexture = 0;
	mState = lcAssetState::Unloaded;
	mLoadFailure = lcTextureLoadError::None;
	mLoadFailureDetails.clear();
}
