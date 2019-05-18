#include "lc_global.h"
#include "lc_stringcache.h"
#include "lc_texture.h"
#include "lc_context.h"

lcStringCache gStringCache;

lcStringCache::lcStringCache()
{
	mTexture = nullptr;
	mRefCount = 0;
}

lcStringCache::~lcStringCache()
{
	delete mTexture;
}

void lcStringCache::AddRef(lcContext* Context)
{
	Q_UNUSED(Context);

	mRefCount++;

	if (mRefCount == 1)
		mTexture = new lcTexture();
}

void lcStringCache::Release(lcContext* Context)
{
	mRefCount--;

	if (mRefCount == 0)
	{
		Context->UnbindTexture2D(mTexture->mTexture); // todo: unbind from all contexts

		delete mTexture;
		mTexture = nullptr;
	}
}

void lcStringCache::CacheStrings(const QStringList& Strings)
{
	bool Update = false;

	for (const QString& String : Strings)
	{
		if (mStrings.find(String) == mStrings.end())
		{
			mStrings[String] = lcStringCacheEntry();
			Update = true;
		}
	}

	if (!Update)
		return;

	Image TextureImage;
	TextureImage.Allocate(256, 256, LC_PIXEL_FORMAT_L8A8);

	QImage Image(128, 128, QImage::Format_ARGB32);
	QPainter Painter;
	QFont Font("Helvetica", 20);
	int DestX = 0, DestY = 0, DestHeight = 0;
	memset(TextureImage.mData, 0, TextureImage.mWidth * TextureImage.mHeight * 2);

	for (auto& Entry : mStrings)
	{
		QRect SourceRect;

		Painter.begin(&Image);
		Painter.fillRect(0, 0, Image.width(), Image.height(), QColor(0, 0, 0));
		Painter.setBrush(QColor(255, 255, 255));
		Painter.setPen(QColor(255, 255, 255));
		Painter.setFont(Font);
		Painter.drawText(0, 0, Image.width(), Image.height(), 0, Entry.first, &SourceRect);
		Painter.end();

		if (DestX + SourceRect.width() + 2 > TextureImage.mWidth)
		{
			DestX = 0;
			DestY += DestHeight + 2;
			DestHeight = 0;
		}

		lcStringCacheEntry& String = Entry.second;

		if (SourceRect.width() + 2 > TextureImage.mWidth || DestY + SourceRect.height() + 2 > TextureImage.mHeight || DestY + SourceRect.height() + 2 > TextureImage.mHeight)
		{
			memset(&String, 0, sizeof(String));
			break;
		}

		String.Top = TextureImage.mHeight - DestY - 2;
		String.Bottom = String.Top - SourceRect.height() + 1;
		String.Left = DestX + 1;
		String.Right = String.Left + SourceRect.width() - 2;

		for (int y = SourceRect.top(); y < SourceRect.bottom(); y++)
		{
			unsigned char* Dest = TextureImage.mData + ((String.Top - y) * TextureImage.mWidth + String.Left) * 2;

			for (int x = SourceRect.left(); x < SourceRect.right(); x++)
			{
				*Dest = *(Dest + 1) = qRed(Image.pixel(x, y));
				Dest += 2;
			}
		}

		DestX += SourceRect.width() + 2;
		DestHeight = qMax(DestHeight, SourceRect.height());
	}

	mTexture->SetImage(&TextureImage, LC_TEXTURE_LINEAR);
}

void lcStringCache::GetStringDimensions(int* cx, int* cy, const QString& String) const
{
	const auto& Entry = mStrings.find(String);

	if (Entry != mStrings.end())
	{
		const lcStringCacheEntry& FontString = Entry->second;
		*cx = FontString.Right - FontString.Left;
		*cy = FontString.Top - FontString.Bottom;
	}
	else
		*cx = *cy = 0;
}

void lcStringCache::DrawStrings(lcContext* Context, const lcMatrix44* Transforms, const QStringList& Strings) const
{
	float* Verts = (float*)alloca(Strings.size() * 6 * 5 * sizeof(float));
	float* Buffer = Verts;

	for (int StringIdx = 0; StringIdx < Strings.size(); StringIdx++)
	{
		const auto& Entry = mStrings.find(Strings[StringIdx]);

		if (Entry == mStrings.end())
			continue;

		const lcStringCacheEntry& FontString = Entry->second;

		float u0 = (float)FontString.Left / (float)(mTexture->mWidth - 1);
		float u1 = (float)FontString.Right / (float)(mTexture->mWidth - 1);
		float v0 = (float)FontString.Bottom / (float)(mTexture->mHeight - 1);
		float v1 = (float)FontString.Top / (float)(mTexture->mHeight - 1);

		float Width = FontString.Right - FontString.Left;
		float Height = FontString.Top - FontString.Bottom;
		float Left = -Width / 2.0f;
		float Top = Height / 2.0f;
		float Z = 0.0f;

		lcVector3 Points[4] =
		{
			lcVector3(Left, Top, Z),
			lcVector3(Left, Top - Height, Z),
			lcVector3(Left + Width, Top - Height, Z),
			lcVector3(Left + Width, Top, Z),
		};

		for (int PointIdx = 0; PointIdx < 4; PointIdx++)
			Points[PointIdx] = lcMul31(Points[PointIdx], Transforms[StringIdx]);

		*Buffer++ = Points[0].x;
		*Buffer++ = Points[0].y;
		*Buffer++ = Points[0].z;
		*Buffer++ = u0;
		*Buffer++ = v1;

		*Buffer++ = Points[1].x;
		*Buffer++ = Points[1].y;
		*Buffer++ = Points[1].z;
		*Buffer++ = u0;
		*Buffer++ = v0;

		*Buffer++ = Points[2].x;
		*Buffer++ = Points[2].y;
		*Buffer++ = Points[2].z;
		*Buffer++ = u1;
		*Buffer++ = v0;

		*Buffer++ = Points[2].x;
		*Buffer++ = Points[2].y;
		*Buffer++ = Points[2].z;
		*Buffer++ = u1;
		*Buffer++ = v0;

		*Buffer++ = Points[3].x;
		*Buffer++ = Points[3].y;
		*Buffer++ = Points[3].z;
		*Buffer++ = u1;
		*Buffer++ = v1;

		*Buffer++ = Points[0].x;
		*Buffer++ = Points[0].y;
		*Buffer++ = Points[0].z;
		*Buffer++ = u0;
		*Buffer++ = v1;
	}

	Context->SetVertexBufferPointer(Verts);
	Context->SetVertexFormat(0, 3, 0, 2, 0, false);

	Context->BindTexture2D(mTexture->mTexture);
	Context->SetColor(0.0f, 0.0f, 0.0f, 1.0f);
	Context->DrawPrimitives(GL_TRIANGLES, 0, Strings.size() * 6);
}

