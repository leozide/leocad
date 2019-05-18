#include "lc_global.h"
#include "lc_viewsphere.h"
#include "view.h"
#include "lc_context.h"
#include "lc_stringcache.h"
#include "lc_application.h"
#include "image.h"
#include "lc_texture.h"

lcTexture* lcViewSphere::mTexture;
lcVertexBuffer lcViewSphere::mVertexBuffer;
lcIndexBuffer lcViewSphere::mIndexBuffer;
const float lcViewSphere::mRadius = 1.0f;
const float lcViewSphere::mHighlightRadius = 0.35f;
const int lcViewSphere::mSubdivisions = 7;

lcViewSphere::lcViewSphere(View* View)
	: mView(View)
{
	mMouseDown = false;
}

lcMatrix44 lcViewSphere::GetViewMatrix() const
{
	lcMatrix44 ViewMatrix = mView->mCamera->mWorldView;
	ViewMatrix.SetTranslation(lcVector3(0, 0, 0));
	return ViewMatrix;
}

lcMatrix44 lcViewSphere::GetProjectionMatrix() const
{
	return lcMatrix44Ortho(-mRadius * 1.25f, mRadius * 1.25f, -mRadius * 1.25f, mRadius * 1.25f, -mRadius * 1.25f, mRadius * 1.25f);
}

void lcViewSphere::CreateResources(lcContext* Context)
{
	const int ImageSize = 128;
	mTexture = new lcTexture();

	const QString ViewNames[6] =
	{
		QCoreApplication::translate("ViewName", "Left"), QCoreApplication::translate("ViewName", "Right"), QCoreApplication::translate("ViewName", "Back"),
		QCoreApplication::translate("ViewName", "Front"), QCoreApplication::translate("ViewName", "Top"), QCoreApplication::translate("ViewName", "Bottom")
	};

	const QTransform ViewTransforms[6] =
	{
		QTransform(0, 1, 1, 0, 0, 0), QTransform(0, -1, -1, 0, ImageSize, ImageSize), QTransform(-1, 0, 0, 1, ImageSize, 0),
		QTransform(1, 0, 0, -1, 0, ImageSize), QTransform(1, 0, 0, -1, 0, ImageSize), QTransform(-1, 0, 0, 1, ImageSize, 0)
	};

	QImage PainterImage(ImageSize, ImageSize, QImage::Format_ARGB32);
	QPainter Painter;
	QFont Font("Helvetica", 20);
	std::vector<Image> Images;

	for (int ViewIdx = 0; ViewIdx < 6; ViewIdx++)
	{
		Image TextureImage;
		TextureImage.Allocate(ImageSize, ImageSize, LC_PIXEL_FORMAT_A8);

		Painter.begin(&PainterImage);
		Painter.fillRect(0, 0, PainterImage.width(), PainterImage.height(), QColor(0, 0, 0));
		Painter.setBrush(QColor(255, 255, 255));
		Painter.setPen(QColor(255, 255, 255));
		Painter.setFont(Font);
		Painter.setTransform(ViewTransforms[ViewIdx]);
		Painter.drawText(0, 0, PainterImage.width(), PainterImage.height(), Qt::AlignCenter, ViewNames[ViewIdx]);
		Painter.end();

		for (int y = 0; y < ImageSize; y++)
		{
			unsigned char* Dest = TextureImage.mData + (ImageSize - y - 1) * TextureImage.mWidth;

			for (int x = 0; x < ImageSize; x++)
				*Dest++ = qRed(PainterImage.pixel(x, y));
		}

		Images.emplace_back(std::move(TextureImage));
	}

	mTexture->SetImage(std::move(Images), LC_TEXTURE_CUBEMAP | LC_TEXTURE_LINEAR);

	lcVector3 Verts[(mSubdivisions + 1) * (mSubdivisions + 1) * 6];
	GLushort Indices[mSubdivisions * mSubdivisions * 6 * 6];

	lcMatrix44 Transforms[6] =
	{
		lcMatrix44(lcVector4(0.0f,  1.0f, 0.0f, 0.0f), lcVector4(0.0f,  0.0f, 1.0f, 0.0f), lcVector4(1.0f, 0.0f, 0.0f, 0.0f), lcVector4(1.0f,  0.0f,  0.0f, 1.0f)),
		lcMatrix44(lcVector4(0.0f, -1.0f, 0.0f, 0.0f), lcVector4(0.0f,  0.0f, 1.0f, 0.0f), lcVector4(1.0f, 0.0f, 0.0f, 0.0f), lcVector4(-1.0f,  0.0f,  0.0f, 1.0f)),
		lcMatrix44(lcVector4(-1.0f,  0.0f, 0.0f, 0.0f), lcVector4(0.0f,  0.0f, 1.0f, 0.0f), lcVector4(0.0f, 1.0f, 0.0f, 0.0f), lcVector4(0.0f,  1.0f,  0.0f, 1.0f)),
		lcMatrix44(lcVector4(1.0f,  0.0f, 0.0f, 0.0f), lcVector4(0.0f,  0.0f, 1.0f, 0.0f), lcVector4(0.0f, 1.0f, 0.0f, 0.0f), lcVector4(0.0f, -1.0f,  0.0f, 1.0f)),
		lcMatrix44(lcVector4(1.0f,  0.0f, 0.0f, 0.0f), lcVector4(0.0f,  1.0f, 0.0f, 0.0f), lcVector4(0.0f, 0.0f, 1.0f, 0.0f), lcVector4(0.0f,  0.0f,  1.0f, 1.0f)),
		lcMatrix44(lcVector4(1.0f,  0.0f, 0.0f, 0.0f), lcVector4(0.0f, -1.0f, 0.0f, 0.0f), lcVector4(0.0f, 0.0f, 1.0f, 0.0f), lcVector4(0.0f,  0.0f, -1.0f, 1.0f)),
	};

	const float Step = 2.0f / mSubdivisions;
	lcVector3* CurVert = Verts;

	for (int FaceIdx = 0; FaceIdx < 6; FaceIdx++)
	{
		for (int y = 0; y <= mSubdivisions; y++)
		{
			for (int x = 0; x <= mSubdivisions; x++)
			{
				lcVector3 Vert = lcMul31(lcVector3(Step * x - 1.0f, Step * y - 1.0f, 0.0f), Transforms[FaceIdx]);
				lcVector3 Vert2 = Vert * Vert;

				*CurVert++ = lcVector3(Vert.x * sqrt(1.0 - 0.5 * (Vert2.y + Vert2.z) + Vert2.y * Vert2.z / 3.0),
									   Vert.y * sqrt(1.0 - 0.5 * (Vert2.z + Vert2.x) + Vert2.z * Vert2.x / 3.0),
									   Vert.z * sqrt(1.0 - 0.5 * (Vert2.x + Vert2.y) + Vert2.x * Vert2.y / 3.0)
				) * mRadius;
			}
		}
	}

	GLushort* CurIndex = Indices;

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

	mVertexBuffer = Context->CreateVertexBuffer(sizeof(Verts), Verts);
	mIndexBuffer = Context->CreateIndexBuffer(sizeof(Indices), Indices);
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
	const lcPreferences& Preferences = lcGetPreferences();
	int ViewportSize = Preferences.mViewSphereSize;

	if (ViewportSize == 0)
		return;

	lcContext* Context = mView->mContext;
	int Width = mView->mWidth;
	int Height = mView->mHeight;
	lcViewSphereLocation Location = Preferences.mViewSphereLocation;

	int Left = (Location == lcViewSphereLocation::BOTTOM_LEFT || Location == lcViewSphereLocation::TOP_LEFT) ? 0 : Width - ViewportSize;
	int Bottom = (Location == lcViewSphereLocation::BOTTOM_LEFT || Location == lcViewSphereLocation::BOTTOM_RIGHT) ? 0 : Height - ViewportSize;
	Context->SetViewport(Left, Bottom, ViewportSize, ViewportSize);

	glDepthFunc(GL_ALWAYS);
	glEnable(GL_CULL_FACE);

	Context->SetVertexBuffer(mVertexBuffer);
	Context->SetVertexFormatPosition(3);
	Context->SetIndexBuffer(mIndexBuffer);

	Context->SetMaterial(LC_MATERIAL_UNLIT_COLOR);
	Context->SetColor(lcVector4(0.0f, 0.0f, 0.0f, 1.0f));

	float Scale = 1.005f + 2.0f / (float)ViewportSize;
	Context->SetWorldMatrix(lcMatrix44Scale(lcVector3(Scale, Scale, Scale)));
	Context->SetViewMatrix(GetViewMatrix());
	Context->SetProjectionMatrix(GetProjectionMatrix());

	Context->DrawIndexedPrimitives(GL_TRIANGLES, mSubdivisions * mSubdivisions * 6 * 6, GL_UNSIGNED_SHORT, 0);

	Context->SetMaterial(LC_MATERIAL_UNLIT_VIEW_SPHERE);
	Context->BindTextureCubeMap(mTexture->mTexture);

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

	const lcVector4 TextColor = lcVector4FromColor(Preferences.mViewSphereTextColor);
	const lcVector4 BackgroundColor = lcVector4FromColor(Preferences.mViewSphereColor);
	const lcVector4 HighlightColor = lcVector4FromColor(Preferences.mViewSphereHighlightColor);

	Context->SetHighlightParams(HighlightPosition, TextColor, BackgroundColor, HighlightColor);
	Context->DrawIndexedPrimitives(GL_TRIANGLES, mSubdivisions * mSubdivisions * 6 * 6, GL_UNSIGNED_SHORT, 0);

	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);

	Context->SetViewport(0, 0, Width, Height);
}

bool lcViewSphere::OnLeftButtonDown()
{
	const lcPreferences& Preferences = lcGetPreferences();
	if (Preferences.mViewSphereSize == 0)
		return false;

	mIntersectionFlags = GetIntersectionFlags(mIntersection);

	if (!mIntersectionFlags.any())
		return false;

	mMouseDownX = mView->mInputState.x;
	mMouseDownY = mView->mInputState.y;
	mMouseDown = true;

	return true;
}

bool lcViewSphere::OnLeftButtonUp()
{
	const lcPreferences& Preferences = lcGetPreferences();
	if (Preferences.mViewSphereSize == 0)
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
	const lcPreferences& Preferences = lcGetPreferences();
	if (Preferences.mViewSphereSize == 0)
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
	return mMouseDown && (qAbs(mMouseDownX - mView->mInputState.x) > 3 || qAbs(mMouseDownY - mView->mInputState.y) > 3);
}

std::bitset<6> lcViewSphere::GetIntersectionFlags(lcVector3& Intersection) const
{
	const lcPreferences& Preferences = lcGetPreferences();
	lcViewSphereLocation Location = Preferences.mViewSphereLocation;

	int Width = mView->mWidth;
	int Height = mView->mHeight;
	int ViewportSize = Preferences.mViewSphereSize;
	int Left = (Location == lcViewSphereLocation::BOTTOM_LEFT || Location == lcViewSphereLocation::TOP_LEFT) ? 0 : Width - ViewportSize;
	int Bottom = (Location == lcViewSphereLocation::BOTTOM_LEFT || Location == lcViewSphereLocation::BOTTOM_RIGHT) ? 0 : Height - ViewportSize;
	int x = mView->mInputState.x - Left;
	int y = mView->mInputState.y - Bottom;
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
