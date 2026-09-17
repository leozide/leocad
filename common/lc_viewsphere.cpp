#include "lc_global.h"
#include "lc_viewsphere.h"
#include "lc_view.h"
#include "camera.h"
#include "lc_context.h"
#include "lc_stringcache.h"
#include "lc_application.h"
#include "lc_glextensions.h"
#include "image.h"
#include "lc_texture.h"

lcTexture* lcViewSphere::mTexture;
lcVertexBuffer lcViewSphere::mVertexBuffer;
lcIndexBuffer lcViewSphere::mIndexBuffer;
int lcViewSphere::mSphereIndexCount;
const float lcViewSphere::mRadius = 1.0f;
const float lcViewSphere::mHighlightRadius = 0.35f;
const int lcViewSphere::mSubdivisions = 24;

lcViewSphere::lcViewSphere(lcView* View)
	: mView(View)
{
	UpdateSettings();
}

void lcViewSphere::UpdateSettings()
{
	const lcPreferences& Preferences = lcGetPreferences();

	switch (mView->GetViewType())
	{
	case lcViewType::View:
		mSize = Preferences.mViewSphereSize;
		mEnabled = Preferences.mViewSphereEnabled;
		mLocation = Preferences.mViewSphereLocation;
		break;

	case lcViewType::Preview:
		mSize = Preferences.mPreviewViewSphereSize;
		mEnabled = Preferences.mPreviewViewSphereEnabled;
		mLocation = Preferences.mPreviewViewSphereLocation;
		break;

	case lcViewType::Minifig:
	case lcViewType::PartsList:
	case lcViewType::Count:
		break;
	}
}

lcMatrix44 lcViewSphere::GetViewMatrix() const
{
	lcMatrix44 ViewMatrix = mView->GetCamera()->mWorldView;
	ViewMatrix.SetTranslation(lcVector3(0, 0, 0));
	return ViewMatrix;
}

lcMatrix44 lcViewSphere::GetProjectionMatrix() const
{
	return lcMatrix44Ortho(-mRadius * 1.25f, mRadius * 1.25f, -mRadius * 1.25f, mRadius * 1.25f, -mRadius * 1.25f, mRadius * 1.25f);
}

void lcViewSphere::CreateResources(lcContext* Context)
{
	constexpr int SourceCellSize = 128, CellHeight = 64, CellBottom = (SourceCellSize - CellHeight) / 2;
	constexpr int AtlasWidth = SourceCellSize, AtlasHeight = 512;
	mTexture = new lcTexture();

	const QString ViewNames[6] =
	{
		QCoreApplication::translate("ViewName", "Left"), QCoreApplication::translate("ViewName", "Right"), QCoreApplication::translate("ViewName", "Back"),
		QCoreApplication::translate("ViewName", "Front"), QCoreApplication::translate("ViewName", "Top"), QCoreApplication::translate("ViewName", "Bottom")
	};

	QImage PainterImage(SourceCellSize, SourceCellSize, QImage::Format_ARGB32);
	QPainter Painter;
	QFont Font("Helvetica", gSupportsShaderObjects ? 36 : 20);
	const bool UseSDF = gSupportsShaderObjects;
	Image AtlasImage;
	std::vector<Image> BitmapFaces;
	if (UseSDF)
	{
		AtlasImage.Allocate(AtlasWidth, AtlasHeight, lcPixelFormat::A8);
		memset(AtlasImage.mData, 0, AtlasWidth * AtlasHeight);
	}
	else
		BitmapFaces.reserve(6);
	const QTransform BitmapTransforms[6] =
	{
		QTransform(0, 1, 1, 0, 0, 0), QTransform(0, -1, -1, 0, SourceCellSize, SourceCellSize),
		QTransform(-1, 0, 0, 1, SourceCellSize, 0), QTransform(1, 0, 0, -1, 0, SourceCellSize),
		QTransform(1, 0, 0, -1, 0, SourceCellSize), QTransform(-1, 0, 0, 1, SourceCellSize, 0)
	};
	auto ConvertToSDF = [](Image& TextureImage)
	{
		constexpr int Size = 128, Count = Size * Size;
		std::vector<int> ToInk(Count), ToOutside(Count);
		for (int i = 0; i < Count; i++) { ToInk[i] = TextureImage.mData[i] >= 128 ? 0 : 1 << 20; ToOutside[i] = TextureImage.mData[i] >= 128 ? 1 << 20 : 0; }
		auto Relax = [](std::vector<int>& Distance, bool Reverse)
		{
			for (int y = Reverse ? Size - 1 : 0; Reverse ? y >= 0 : y < Size; y += Reverse ? -1 : 1)
				for (int x = Reverse ? Size - 1 : 0; Reverse ? x >= 0 : x < Size; x += Reverse ? -1 : 1)
				{
					int& Value = Distance[y * Size + x]; const int Step = Reverse ? 1 : -1;
					if (y + Step >= 0 && y + Step < Size) Value = lcMin(Value, Distance[(y + Step) * Size + x] + 3);
					if (x + Step >= 0 && x + Step < Size) Value = lcMin(Value, Distance[y * Size + x + Step] + 3);
					if (y + Step >= 0 && y + Step < Size && x + Step >= 0 && x + Step < Size) Value = lcMin(Value, Distance[(y + Step) * Size + x + Step] + 4);
				}
		};
		Relax(ToInk, false); Relax(ToInk, true); Relax(ToOutside, false); Relax(ToOutside, true);
		for (int i = 0; i < Count; i++) TextureImage.mData[i] = static_cast<unsigned char>(lcClamp(128 + (ToOutside[i] - ToInk[i]) / 3 * 16, 0, 255));
	};

	for (int ViewIdx = 0; ViewIdx < 6; ViewIdx++)
	{
		Image TextureImage;
		TextureImage.Allocate(SourceCellSize, SourceCellSize, lcPixelFormat::A8);
		QFont LabelFont = Font;
		if (UseSDF)
		{
			// The shader displays the middle 80% of each cell. Leave six pixels
			// on either side for the SDF halo, including for translated labels.
			constexpr int MaxLabelWidth = 90;
			while (LabelFont.pointSize() > 1 && QFontMetrics(LabelFont).horizontalAdvance(ViewNames[ViewIdx]) > MaxLabelWidth)
				LabelFont.setPointSize(LabelFont.pointSize() - 1);
		}

		Painter.begin(&PainterImage);
		Painter.setRenderHint(QPainter::TextAntialiasing, UseSDF);
		Painter.fillRect(0, 0, PainterImage.width(), PainterImage.height(), QColor(0, 0, 0));
		Painter.setBrush(QColor(255, 255, 255));
		Painter.setPen(QColor(255, 255, 255));
		Painter.setFont(LabelFont);
		if (!UseSDF)
			Painter.setTransform(BitmapTransforms[ViewIdx]);
		Painter.drawText(0, 0, PainterImage.width(), PainterImage.height(), Qt::AlignCenter, ViewNames[ViewIdx]);
		Painter.end();

		for (int y = 0; y < SourceCellSize; y++)
		{
			unsigned char* Dest = TextureImage.mData + (SourceCellSize - y - 1) * TextureImage.mWidth;

			for (int x = 0; x < SourceCellSize; x++)
				*Dest++ = qRed(PainterImage.pixel(x, y));
		}
		if (UseSDF)
		{
			ConvertToSDF(TextureImage);
			// Keep the middle band and its SDF margin in a vertical atlas.
			const int AtlasY = ViewIdx * CellHeight;
			for (int y = 0; y < CellHeight; y++)
				memcpy(AtlasImage.mData + (AtlasY + y) * AtlasWidth,
					TextureImage.mData + (CellBottom + y) * SourceCellSize, SourceCellSize);
		}
		else
			BitmapFaces.emplace_back(std::move(TextureImage));
	}

	if (UseSDF)
		mTexture->SetImage(std::move(AtlasImage), LC_TEXTURE_LINEAR);
	else
		mTexture->SetImage(std::move(BitmapFaces), LC_TEXTURE_CUBEMAP | LC_TEXTURE_LINEAR);

	const int SphereVertexCount = (mSubdivisions + 1) * (mSubdivisions + 1) * 6;
	mSphereIndexCount = mSubdivisions * mSubdivisions * 6 * 6;
	std::vector<float> Verts(SphereVertexCount * (UseSDF ? 6 : 3));
	std::vector<GLushort> Indices(mSphereIndexCount);

	lcMatrix44 Transforms[6] =
	{
		lcMatrix44(lcVector4(0.0f,  1.0f, 0.0f, 0.0f), lcVector4(0.0f,  0.0f, 1.0f, 0.0f), lcVector4(1.0f, 0.0f, 0.0f, 0.0f), lcVector4(1.0f,  0.0f,  0.0f, 1.0f)),
		lcMatrix44(lcVector4(0.0f, -1.0f, 0.0f, 0.0f), lcVector4(0.0f,  0.0f, 1.0f, 0.0f), lcVector4(1.0f, 0.0f, 0.0f, 0.0f), lcVector4(-1.0f,  0.0f,  0.0f, 1.0f)),
		lcMatrix44(lcVector4(-1.0f,  0.0f, 0.0f, 0.0f), lcVector4(0.0f,  0.0f, 1.0f, 0.0f), lcVector4(0.0f, 1.0f, 0.0f, 0.0f), lcVector4(0.0f,  1.0f,  0.0f, 1.0f)),
		lcMatrix44(lcVector4(1.0f,  0.0f, 0.0f, 0.0f), lcVector4(0.0f,  0.0f, 1.0f, 0.0f), lcVector4(0.0f, 1.0f, 0.0f, 0.0f), lcVector4(0.0f, -1.0f,  0.0f, 1.0f)),
		lcMatrix44(lcVector4(1.0f,  0.0f, 0.0f, 0.0f), lcVector4(0.0f,  1.0f, 0.0f, 0.0f), lcVector4(0.0f, 0.0f, 1.0f, 0.0f), lcVector4(0.0f,  0.0f,  1.0f, 1.0f)),
		lcMatrix44(lcVector4(1.0f,  0.0f, 0.0f, 0.0f), lcVector4(0.0f, -1.0f, 0.0f, 0.0f), lcVector4(0.0f, 0.0f, 1.0f, 0.0f), lcVector4(0.0f,  0.0f, -1.0f, 1.0f)),
	};

	constexpr float Step = 2.0f / mSubdivisions;
	float* CurVert = Verts.data();
	auto ProjectToSphere = [](const lcVector3& Vert)
	{
		const lcVector3 Vert2 = Vert * Vert;
		return lcVector3(Vert.x * sqrt(1.0 - 0.5 * (Vert2.y + Vert2.z) + Vert2.y * Vert2.z / 3.0),
			Vert.y * sqrt(1.0 - 0.5 * (Vert2.z + Vert2.x) + Vert2.z * Vert2.x / 3.0),
			Vert.z * sqrt(1.0 - 0.5 * (Vert2.x + Vert2.y) + Vert2.x * Vert2.y / 3.0));
	};

	for (int FaceIdx = 0; FaceIdx < 6; FaceIdx++)
	{
		for (int y = 0; y <= mSubdivisions; y++)
		{
			for (int x = 0; x <= mSubdivisions; x++)
			{
				const lcVector3 Vert = lcMul31(lcVector3(Step * x - 1.0f, Step * y - 1.0f, 0.0f), Transforms[FaceIdx]);
				const lcVector3 SphereVert = ProjectToSphere(Vert) * mRadius;
				*CurVert++ = SphereVert.x;
				*CurVert++ = SphereVert.y;
				*CurVert++ = SphereVert.z;
				if (UseSDF)
				{
					*CurVert++ = x / static_cast<float>(mSubdivisions);
					*CurVert++ = y / static_cast<float>(mSubdivisions);
					*CurVert++ = static_cast<float>(FaceIdx);
				}
			}
		}
	}

	GLushort* CurIndex = Indices.data();

	for (int FaceIdx = 0; FaceIdx < 6; FaceIdx++)
	{
		const int FaceBase = FaceIdx * (mSubdivisions + 1) * (mSubdivisions + 1);

		for (int y = 0; y < mSubdivisions; y++)
		{
			int RowBase = FaceBase + (mSubdivisions + 1) * y;

			for (int x = 0; x < mSubdivisions; x++)
			{
				*CurIndex++ = RowBase + x;
				*CurIndex++ = RowBase + x + 1;
				*CurIndex++ = RowBase + x + (mSubdivisions + 1);

				*CurIndex++ = RowBase + x + 1;
				*CurIndex++ = RowBase + x + 1 + (mSubdivisions + 1);
				*CurIndex++ = RowBase + x + (mSubdivisions + 1);
			}
		}
	}

	mVertexBuffer = Context->CreateVertexBuffer(Verts.size() * sizeof(float), Verts.data());
	mIndexBuffer = Context->CreateIndexBuffer(Indices.size() * sizeof(GLushort), Indices.data());
}

void lcViewSphere::DestroyResources(lcContext* Context)
{
	delete mTexture;
	mTexture = nullptr;
	Context->DestroyVertexBuffer(mVertexBuffer);
	Context->DestroyIndexBuffer(mIndexBuffer);
}

void lcViewSphere::Draw()
{
	UpdateSettings();

	if (!mSize || !mEnabled)
		return;

	lcContext* Context = mView->mContext;
	const float UIScale = mView->GetUIScale();
	const int Width = mView->GetWidth();
	const int Height = mView->GetHeight();
	const int ViewportSize = mSize * UIScale;
	const int Left = (mLocation == lcViewSphereLocation::BottomLeft || mLocation == lcViewSphereLocation::TopLeft) ? 0 : Width - ViewportSize;
	const int Bottom = (mLocation == lcViewSphereLocation::BottomLeft || mLocation == lcViewSphereLocation::BottomRight) ? 0 : Height - ViewportSize;
	Context->SetViewport(Left, Bottom, ViewportSize, ViewportSize);

	Context->SetDepthFunction(lcDepthFunction::Always);
	Context->EnableCullFace(true);

	Context->SetVertexBuffer(mVertexBuffer);
	if (gSupportsShaderObjects)
		Context->SetVertexFormat(0, 3, 0, 3, 0, false);
	else
		Context->SetVertexFormatPosition(3);
	Context->SetIndexBuffer(mIndexBuffer);

	Context->SetMaterial(lcMaterialType::UnlitColor);
	Context->SetColor(lcVector4(0.0f, 0.0f, 0.0f, 1.0f));

	float Scale = 1.005f + 2.0f / (float)ViewportSize;
	Context->SetWorldMatrix(lcMatrix44Scale(lcVector3(Scale, Scale, Scale)));
	Context->SetViewMatrix(GetViewMatrix());
	Context->SetProjectionMatrix(GetProjectionMatrix());

	Context->DrawIndexedPrimitives(GL_TRIANGLES, mSphereIndexCount, GL_UNSIGNED_SHORT, 0);

	Context->SetMaterial(lcMaterialType::UnlitViewSphere);
	if (gSupportsShaderObjects)
		Context->BindTexture2D(mTexture);
	else
		Context->BindTextureCubeMap(mTexture);

	Context->SetWorldMatrix(lcMatrix44Identity());
	Context->SetViewMatrix(GetViewMatrix());
	Context->SetProjectionMatrix(GetProjectionMatrix());

	lcVector4 HighlightPosition(0.0f, 0.0f, 0.0f, 0.0f);

	if (mIntersectionFlags.any())
	{
		for (int AxisIdx = 0; AxisIdx < 3; AxisIdx++)
		{
			if (mIntersectionFlags.test(2 * AxisIdx))
				HighlightPosition[AxisIdx] = 1.0f;
			else if (mIntersectionFlags.test(2 * AxisIdx + 1))
				HighlightPosition[AxisIdx] = -1.0f;
		}

		HighlightPosition = lcVector4(lcNormalize(lcVector3(HighlightPosition)), mHighlightRadius);
	}

	const lcPreferences& Preferences = lcGetPreferences();
	const lcVector4 TextColor = lcVector4FromColor(Preferences.mViewSphereTextColor);
	const lcVector4 BackgroundColor = lcVector4FromColor(Preferences.mViewSphereColor);
	const lcVector4 HighlightColor = lcVector4FromColor(Preferences.mViewSphereHighlightColor);

	Context->SetHighlightParams(HighlightPosition, TextColor, BackgroundColor, HighlightColor);
	Context->SetTextHaloColor(lcVector4FromColor(Preferences.mTextHaloColor));
	Context->DrawIndexedPrimitives(GL_TRIANGLES, mSphereIndexCount, GL_UNSIGNED_SHORT, 0);

	Context->SetDepthFunction(lcDepthFunction::LessEqual);
	Context->EnableCullFace(false);

	Context->SetViewport(0, 0, mView->GetWidth(), mView->GetHeight());
}

bool lcViewSphere::OnLeftButtonDown()
{
	if (!mSize || !mEnabled)
		return false;

	mIntersectionFlags = GetIntersectionFlags(mIntersection);

	if (!mIntersectionFlags.any())
		return false;

	mMouseDownX = mView->GetMouseX();
	mMouseDownY = mView->GetMouseY();
	mMouseDown = true;

	return true;
}

bool lcViewSphere::OnLeftButtonUp()
{
	if (!mSize || !mEnabled)
		return false;

	if (!mMouseDown)
		return false;

	mMouseDown = false;

	if (!mIntersectionFlags.any())
		return false;

	lcVector3 Position(0.0f, 0.0f, 0.0f);

	for (int AxisIdx = 0; AxisIdx < 3; AxisIdx++)
	{
		if (mIntersectionFlags.test(AxisIdx * 2))
			Position[AxisIdx] = 1250.0f;
		else if (mIntersectionFlags.test(AxisIdx * 2 + 1))
			Position[AxisIdx] = -1250.0f;
	}

	mView->SetViewpoint(Position);

	return true;
}

bool lcViewSphere::OnMouseMove()
{
	if (!mSize || !mEnabled)
		return false;

	if (IsDragging())
	{
		mIntersectionFlags.reset();
		mView->StartOrbitTracking();
		return true;
	}

	if (mView->IsTracking())
		return false;

	std::bitset<6> IntersectionFlags = GetIntersectionFlags(mIntersection);

	if (IntersectionFlags != mIntersectionFlags)
	{
		mIntersectionFlags = IntersectionFlags;
		mView->Redraw();
	}

	return mIntersectionFlags.any();
}

bool lcViewSphere::IsDragging() const
{
	int InputStateX = mView->GetMouseX();
	int InputStateY = mView->GetMouseY();
	return mMouseDown && (qAbs(mMouseDownX - InputStateX) > 3 || qAbs(mMouseDownY - InputStateY) > 3);
}

std::bitset<6> lcViewSphere::GetIntersectionFlags(lcVector3& Intersection) const
{
	const float UIScale = mView->GetUIScale();
	const int Width = mView->GetWidth();
	const int Height = mView->GetHeight();
	const int ViewportSize = mSize * UIScale;
	const int Left = (mLocation == lcViewSphereLocation::BottomLeft || mLocation == lcViewSphereLocation::TopLeft) ? 0 : Width - ViewportSize;
	const int Bottom = (mLocation == lcViewSphereLocation::BottomLeft || mLocation == lcViewSphereLocation::BottomRight) ? 0 : Height - ViewportSize;
	const int x = mView->GetMouseX() - Left;
	const int y = mView->GetMouseY() - Bottom;
	std::bitset<6> IntersectionFlags;

	if (x < 0 || x > Width || y < 0 || y > Height)
		return IntersectionFlags;

	lcVector3 StartEnd[2] = { lcVector3(x, y, 0), lcVector3(x, y, 1) };
	const int Viewport[4] = { 0, 0, ViewportSize, ViewportSize };

	lcUnprojectPoints(StartEnd, 2, GetViewMatrix(), GetProjectionMatrix(), Viewport);

	float Distance;
	if (lcSphereRayMinIntersectDistance(lcVector3(0.0f, 0.0f, 0.0f), mRadius, StartEnd[0], StartEnd[1], &Distance))
	{
		Intersection = (StartEnd[0] + (StartEnd[1] - StartEnd[0]) * Distance) / mRadius;

		auto CheckIntersection = [&]()
		{
			for (int Axis1Idx = 0; Axis1Idx < 6; Axis1Idx++)
			{
				lcVector3 Point1(0.0f, 0.0f, 0.0f);

				Point1[Axis1Idx / 2] = Axis1Idx % 2 ? -1.0f : 1.0f;

				if (lcLengthSquared(Point1 - Intersection) < mHighlightRadius * mHighlightRadius)
				{
					IntersectionFlags.set(Axis1Idx);
					return;
				}

				for (int Axis2Idx = 0; Axis2Idx < 6; Axis2Idx++)
				{
					if (Axis1Idx / 2 == Axis2Idx / 2)
						continue;

					lcVector3 Point2(0.0f, 0.0f, 0.0f);
					Point2[Axis1Idx / 2] = Axis1Idx % 2 ? -0.70710678118f : 0.70710678118f;
					Point2[Axis2Idx / 2] = Axis2Idx % 2 ? -0.70710678118f : 0.70710678118f;

					if (lcLengthSquared(Point2 - Intersection) < mHighlightRadius * mHighlightRadius)
					{
						IntersectionFlags.set(Axis1Idx);
						IntersectionFlags.set(Axis2Idx);
						return;
					}

					for (int Axis3Idx = 0; Axis3Idx < 6; Axis3Idx++)
					{
						if (Axis1Idx / 2 == Axis3Idx / 2 || Axis2Idx / 2 == Axis3Idx / 2)
							continue;

						lcVector3 Point3(0.0f, 0.0f, 0.0f);
						Point3[Axis1Idx / 2] = Axis1Idx % 2 ? -0.57735026919f : 0.57735026919f;
						Point3[Axis2Idx / 2] = Axis2Idx % 2 ? -0.57735026919f : 0.57735026919f;
						Point3[Axis3Idx / 2] = Axis3Idx % 2 ? -0.57735026919f : 0.57735026919f;

						if (lcLengthSquared(Point3 - Intersection) < mHighlightRadius * mHighlightRadius)
						{
							IntersectionFlags.set(Axis1Idx);
							IntersectionFlags.set(Axis2Idx);
							IntersectionFlags.set(Axis3Idx);
							return;
						}
					}
				}
			}
		};

		CheckIntersection();
	}

	return IntersectionFlags;
}
