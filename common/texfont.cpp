#include "lc_global.h"
#include <string.h>
#include "texfont.h"
#include "lc_context.h"
#include "lc_texture.h"
#include "image.h"

static const unsigned char GlyphData[] =
{
	0x20, 0x21, 0x23, 0x24, 0x25, 0x26, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
	0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41,
	0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51,
	0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5d, 0x5f, 0x61, 0x62, 0x63, 0x64,
	0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74,
	0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7d
};

// ============================================================================

TexFont gTexFont;

TexFont::TexFont()
{
	memset(&mGlyphs, 0, sizeof(mGlyphs));
}

bool TexFont::Initialize(lcContext* Context)
{
	if (mTexture)
		return true;

	mFontHeight = 16;

	constexpr int Scale = 4, Padding = 8, TileWidth = 96, TileHeight = 80;
	mTextureWidth = mTextureHeight = 1024;
	Image Image;
	Image.Allocate(mTextureWidth, mTextureHeight, lcPixelFormat::L8A8);
	memset(Image.mData, 0, static_cast<size_t>(mTextureWidth) * mTextureHeight * 2);

	QFont Font(QStringLiteral("Arial"));
	Font.setPixelSize(56);
	Font.setStyleStrategy(QFont::PreferAntialias);
	const QFontMetrics Metrics(Font);

	int GlyphIndex = 0;
	for (const unsigned char Glyph : GlyphData)
	{
		const int Advance = Metrics.horizontalAdvance(QChar(Glyph));
		const int TileX = (GlyphIndex % 10) * TileWidth;
		const int TileY = (GlyphIndex / 10) * TileHeight;
		const int MaskWidth = lcMin(TileWidth, Advance + Padding * 2);
		std::vector<unsigned char> Mask(MaskWidth * TileHeight, 0);
		QImage GlyphImage(MaskWidth, TileHeight, QImage::Format_Alpha8);
		GlyphImage.fill(0);
		QPainter Painter(&GlyphImage);
		Painter.setRenderHint(QPainter::TextAntialiasing);
		Painter.setPen(Qt::white);
		Painter.setFont(Font);
		Painter.drawText(Padding, Padding + (mFontHeight * Scale - Metrics.height()) / 2 + Metrics.ascent(), QString(QChar(Glyph)));
		Painter.end();

		for (int y = 0; y < TileHeight; y++)
			for (int x = 0; x < MaskWidth; x++)
				Mask[y * MaskWidth + x] = GlyphImage.constScanLine(y)[x] >= 128;

		// A two-pass chamfer transform is compact and sufficiently accurate for
		// the small, padded glyph tiles. Distances are in high-resolution pixels.
		std::vector<int> ToInk(MaskWidth * TileHeight), ToOutside(MaskWidth * TileHeight);
		for (int i = 0; i < MaskWidth * TileHeight; i++)
		{
			ToInk[i] = Mask[i] ? 0 : 1 << 20;
			ToOutside[i] = Mask[i] ? 1 << 20 : 0;
		}
		auto Relax = [MaskWidth](std::vector<int>& Distance, bool Reverse)
		{
			for (int row = Reverse ? TileHeight - 1 : 0; Reverse ? row >= 0 : row < TileHeight; row += Reverse ? -1 : 1)
				for (int column = Reverse ? MaskWidth - 1 : 0; Reverse ? column >= 0 : column < MaskWidth; column += Reverse ? -1 : 1)
				{
					int& Value = Distance[row * MaskWidth + column];
					const int RowStep = Reverse ? 1 : -1, ColumnStep = Reverse ? 1 : -1;
					if (row + RowStep >= 0 && row + RowStep < TileHeight)
						Value = lcMin(Value, Distance[(row + RowStep) * MaskWidth + column] + 3);
					if (column + ColumnStep >= 0 && column + ColumnStep < MaskWidth)
						Value = lcMin(Value, Distance[row * MaskWidth + column + ColumnStep] + 3);
					if (row + RowStep >= 0 && row + RowStep < TileHeight && column + ColumnStep >= 0 && column + ColumnStep < MaskWidth)
						Value = lcMin(Value, Distance[(row + RowStep) * MaskWidth + column + ColumnStep] + 4);
				}
		};
		Relax(ToInk, false); Relax(ToInk, true);
		Relax(ToOutside, false); Relax(ToOutside, true);

		for (int y = 0; y < TileHeight; y++)
			for (int x = 0; x < MaskWidth; x++)
			{
				const int SignedDistance = (ToOutside[y * MaskWidth + x] - ToInk[y * MaskWidth + x]) / 3;
				const unsigned char Value = static_cast<unsigned char>(lcClamp(128 + SignedDistance * 8, 0, 255));
				unsigned char* Pixel = Image.mData + ((TileY + y) * mTextureWidth + TileX + x) * 2;
				Pixel[0] = Pixel[1] = Value;
			}

		mGlyphs[Glyph].Width = Advance / static_cast<float>(Scale);
		mGlyphs[Glyph].Offset = -Padding / static_cast<float>(Scale);
		mGlyphs[Glyph].RenderWidth = MaskWidth / static_cast<float>(Scale);
		mGlyphs[Glyph].RenderHeight = TileHeight / static_cast<float>(Scale);
		mGlyphs[Glyph].Left = TileX / static_cast<float>(mTextureWidth);
		mGlyphs[Glyph].Right = (TileX + MaskWidth) / static_cast<float>(mTextureWidth);
		mGlyphs[Glyph].Top = TileY / static_cast<float>(mTextureHeight);
		mGlyphs[Glyph].Bottom = (TileY + TileHeight) / static_cast<float>(mTextureHeight);
		GlyphIndex++;
	}

	mTexture = new lcTexture();
	mTexture->SetImage(std::move(Image), LC_TEXTURE_LINEAR);
	mTexture->Upload(Context);

	return true;
}

void TexFont::Reset()
{
	mTexture = 0;
}

void TexFont::GetStringDimensions(int* cx, int* cy, const char* Text) const
{
	*cy = mFontHeight;
	float Width = 0.0f;

	while (*Text != 0)
	{
		Width += mGlyphs[static_cast<unsigned char>(*Text)].Width;
		Text++;
	}

	*cx = static_cast<int>(Width + 0.5f);
}

void TexFont::PrintText(lcContext* Context, float Left, float Top, float Z, const char* Text) const
{
	const size_t Length = strlen(Text);

	if (!Length)
		return;

	float* Verts = new float[6 * 5 * Length];
	float* CurVert = Verts;

	while (*Text)
	{
		const unsigned char ch = static_cast<unsigned char>(*Text);
		const float GlyphLeft = Left + mGlyphs[ch].Offset;
		const float GlyphRight = GlyphLeft + mGlyphs[ch].RenderWidth;
		const float GlyphTop = Top - mGlyphs[ch].Offset;
		const float GlyphBottom = GlyphTop - mGlyphs[ch].RenderHeight;

		*CurVert++ = GlyphLeft;
		*CurVert++ = GlyphTop;
		*CurVert++ = Z;
		*CurVert++ = mGlyphs[ch].Left;
		*CurVert++ = mGlyphs[ch].Top;

		*CurVert++ = GlyphLeft;
		*CurVert++ = GlyphBottom;
		*CurVert++ = Z;
		*CurVert++ = mGlyphs[ch].Left;
		*CurVert++ = mGlyphs[ch].Bottom;

		*CurVert++ = GlyphRight;
		*CurVert++ = GlyphBottom;
		*CurVert++ = Z;
		*CurVert++ = mGlyphs[ch].Right;
		*CurVert++ = mGlyphs[ch].Bottom;

		*CurVert++ = GlyphRight;
		*CurVert++ = GlyphBottom;
		*CurVert++ = Z;
		*CurVert++ = mGlyphs[ch].Right;
		*CurVert++ = mGlyphs[ch].Bottom;

		*CurVert++ = GlyphRight;
		*CurVert++ = GlyphTop;
		*CurVert++ = Z;
		*CurVert++ = mGlyphs[ch].Right;
		*CurVert++ = mGlyphs[ch].Top;

		*CurVert++ = GlyphLeft;
		*CurVert++ = GlyphTop;
		*CurVert++ = Z;
		*CurVert++ = mGlyphs[ch].Left;
		*CurVert++ = mGlyphs[ch].Top;

		Left += mGlyphs[ch].Width;
		Text++;
	}

	Context->SetVertexBufferPointer(Verts);
	Context->SetVertexFormat(0, 3, 0, 2, 0, false);

	Context->DrawPrimitives(GL_TRIANGLES, 0, 6 * (GLsizei)Length);

	delete[] Verts;
}

void TexFont::GetTriangles(const lcMatrix44& Transform, const char* Text, float* Buffer) const
{
	float Width = 0.0f;

	for (const char* ch = Text; *ch; ch++)
	{
		const unsigned char Glyph = static_cast<unsigned char>(*ch);
		Width += mGlyphs[Glyph].Width;
	}

	float Left = -Width / 2.0f;
	const float Top = mFontHeight / 2.0f;
	const float Z = 0.0f;

	while (*Text)
	{
		const unsigned char ch = static_cast<unsigned char>(*Text);

		const float GlyphLeft = Left + mGlyphs[ch].Offset;
		const float GlyphRight = GlyphLeft + mGlyphs[ch].RenderWidth;
		const float GlyphTop = Top - mGlyphs[ch].Offset;
		const float GlyphBottom = GlyphTop - mGlyphs[ch].RenderHeight;
		lcVector3 Points[4] =
		{
			lcVector3(GlyphLeft, GlyphTop, Z),
			lcVector3(GlyphLeft, GlyphBottom, Z),
			lcVector3(GlyphRight, GlyphBottom, Z),
			lcVector3(GlyphRight, GlyphTop, Z),
		};

		for (int PointIdx = 0; PointIdx < 4; PointIdx++)
			Points[PointIdx] = lcMul31(Points[PointIdx], Transform);

		*Buffer++ = Points[0].x;
		*Buffer++ = Points[0].y;
		*Buffer++ = Points[0].z;
		*Buffer++ = mGlyphs[ch].Left;
		*Buffer++ = mGlyphs[ch].Top;

		*Buffer++ = Points[1].x;
		*Buffer++ = Points[1].y;
		*Buffer++ = Points[1].z;
		*Buffer++ = mGlyphs[ch].Left;
		*Buffer++ = mGlyphs[ch].Bottom;

		*Buffer++ = Points[2].x;
		*Buffer++ = Points[2].y;
		*Buffer++ = Points[2].z;
		*Buffer++ = mGlyphs[ch].Right;
		*Buffer++ = mGlyphs[ch].Bottom;

		*Buffer++ = Points[2].x;
		*Buffer++ = Points[2].y;
		*Buffer++ = Points[2].z;
		*Buffer++ = mGlyphs[ch].Right;
		*Buffer++ = mGlyphs[ch].Bottom;

		*Buffer++ = Points[3].x;
		*Buffer++ = Points[3].y;
		*Buffer++ = Points[3].z;
		*Buffer++ = mGlyphs[ch].Right;
		*Buffer++ = mGlyphs[ch].Top;

		*Buffer++ = Points[0].x;
		*Buffer++ = Points[0].y;
		*Buffer++ = Points[0].z;
		*Buffer++ = mGlyphs[ch].Left;
		*Buffer++ = mGlyphs[ch].Top;

		Left += mGlyphs[ch].Width;
		Text++;
	}
}

void TexFont::GetGlyphTriangles(float Left, float Top, float Z, int Glyph, float* Buffer) const
{
	Left -= mGlyphs[Glyph].Width / 2.0f;
	Top += mFontHeight / 2.0f;
	const float GlyphLeft = Left + mGlyphs[Glyph].Offset;
	const float GlyphRight = GlyphLeft + mGlyphs[Glyph].RenderWidth;
	const float GlyphTop = Top - mGlyphs[Glyph].Offset;
	const float GlyphBottom = GlyphTop - mGlyphs[Glyph].RenderHeight;

	*Buffer++ = GlyphLeft;
	*Buffer++ = GlyphTop;
	*Buffer++ = Z;
	*Buffer++ = mGlyphs[Glyph].Left;
	*Buffer++ = mGlyphs[Glyph].Top;

	*Buffer++ = GlyphLeft;
	*Buffer++ = GlyphBottom;
	*Buffer++ = Z;
	*Buffer++ = mGlyphs[Glyph].Left;
	*Buffer++ = mGlyphs[Glyph].Bottom;

	*Buffer++ = GlyphRight;
	*Buffer++ = GlyphBottom;
	*Buffer++ = Z;
	*Buffer++ = mGlyphs[Glyph].Right;
	*Buffer++ = mGlyphs[Glyph].Bottom;

	*Buffer++ = GlyphRight;
	*Buffer++ = GlyphBottom;
	*Buffer++ = Z;
	*Buffer++ = mGlyphs[Glyph].Right;
	*Buffer++ = mGlyphs[Glyph].Bottom;

	*Buffer++ = GlyphRight;
	*Buffer++ = GlyphTop;
	*Buffer++ = Z;
	*Buffer++ = mGlyphs[Glyph].Right;
	*Buffer++ = mGlyphs[Glyph].Top;

	*Buffer++ = GlyphLeft;
	*Buffer++ = GlyphTop;
	*Buffer++ = Z;
	*Buffer++ = mGlyphs[Glyph].Left;
	*Buffer++ = mGlyphs[Glyph].Top;
}
