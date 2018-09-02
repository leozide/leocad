#include "lc_global.h"
#include "lc_viewcube.h"
#include "view.h"
#include "lc_context.h"
#include "texfont.h"
#include "lc_application.h"

//todo: move these
const float BoxSize = 10.0f;

lcViewCube::lcViewCube(View* View)
	: mView(View)
{
	mIntersectionFlags = 0;
}

lcMatrix44 lcViewCube::GetViewMatrix() const
{
	lcMatrix44 ViewMatrix = mView->mCamera->mWorldView;
	ViewMatrix.SetTranslation(lcVector3(0, 0, 0));
	return ViewMatrix;
}

lcMatrix44 lcViewCube::GetProjectionMatrix() const
{
	return lcMatrix44Ortho(-BoxSize * 2, BoxSize * 2, -BoxSize * 2, BoxSize * 2, -50, 50);
}

void lcViewCube::Draw()
{
	const lcPreferences& Preferences = lcGetPreferences();
	lcViewCubeLocation Location = Preferences.mViewCubeLocation;

	if (Location == lcViewCubeLocation::DISABLED)
		return;

	lcContext* Context = mView->mContext;
	int Width = mView->mWidth;
	int Height = mView->mHeight;
	int ViewportSize = Preferences.mViewCubeSize;

	int Left = (Location == lcViewCubeLocation::BOTTOM_LEFT || Location == lcViewCubeLocation::TOP_LEFT) ? 0 : Width - ViewportSize;
	int Bottom = (Location == lcViewCubeLocation::BOTTOM_LEFT || Location == lcViewCubeLocation::BOTTOM_RIGHT) ? 0 : Height - ViewportSize;
	Context->SetViewport(Left, Bottom, ViewportSize, ViewportSize);

	const lcVector3 BoxVerts[8] =
	{
		lcVector3(-BoxSize, -BoxSize, -BoxSize), lcVector3(-BoxSize, BoxSize, -BoxSize), lcVector3(BoxSize, BoxSize, -BoxSize), lcVector3(BoxSize, -BoxSize, -BoxSize),
		lcVector3(-BoxSize, -BoxSize,  BoxSize), lcVector3(-BoxSize, BoxSize,  BoxSize), lcVector3(BoxSize, BoxSize,  BoxSize), lcVector3(BoxSize, -BoxSize,  BoxSize)
	};

	const GLushort BoxIndices[36 + 24] =
	{
		0, 1, 2, 0, 2, 3, 7, 6, 5, 7, 5, 4, 0, 1, 5, 0, 5, 4, 2, 3, 7, 2, 7, 6, 0, 3, 7, 0, 7, 4, 1, 2, 6, 1, 6, 5,
		0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7
	};

	Context->SetMaterial(LC_MATERIAL_UNLIT_COLOR);
	Context->SetWorldMatrix(lcMatrix44Identity());
	Context->SetViewMatrix(GetViewMatrix());
	Context->SetProjectionMatrix(GetProjectionMatrix());

	Context->SetVertexBufferPointer(BoxVerts);
	Context->SetVertexFormatPosition(3);
	Context->SetIndexBufferPointer(BoxIndices);

	Context->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	Context->DrawIndexedPrimitives(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
	Context->SetColor(0.0f, 0.0f, 0.0f, 1.0f);
	Context->DrawIndexedPrimitives(GL_LINES, 24, GL_UNSIGNED_SHORT, 36 * 2);

	glDisable(GL_DEPTH_TEST); // TODO: don't disable depth, fix z fighting

	int IntersectionType = mIntersectionFlags.count();
	if (IntersectionType == 1)
	{
		lcVector3 Center(0.0f, 0.0f, 0.0f), Dx(0.0f, 0.0f, 0.0f), Dy(0.0f, 0.0f, 0.0f);
		int AxisIdx;

		for (AxisIdx = 0; AxisIdx < 3; AxisIdx++)
		{
			if (mIntersectionFlags.test(AxisIdx * 2))
			{
				Center[AxisIdx] = -BoxSize;
				break;
			}
			else if (mIntersectionFlags.test(AxisIdx * 2 + 1))
			{
				Center[AxisIdx] = BoxSize;
				break;
			}
		}

		Dx[(AxisIdx + 1) % 3] = BoxSize * 2 / 3;
		Dy[(AxisIdx + 2) % 3] = BoxSize * 2 / 3;

		lcVector3 FaceVerts[] =
		{
			Center - Dx - Dy, Center + Dx - Dy, Center - Dx + Dy, Center + Dx + Dy
		};

		Context->SetVertexBufferPointer(FaceVerts);
		Context->SetVertexFormatPosition(3);

		Context->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
		Context->DrawPrimitives(GL_TRIANGLE_STRIP, 0, 4);
	}
	else if (IntersectionType == 2)
	{
		lcVector3 Center(0.0f, 0.0f, 0.0f), Dx(0.0f, 0.0f, 0.0f), Dy(0.0f, 0.0f, 0.0f), Dz(0.0f, 0.0f, 0.0f);
		int Corners = 0;

		for (int AxisIdx = 0; AxisIdx < 3; AxisIdx++)
		{
			if (mIntersectionFlags.test(AxisIdx * 2))
			{
				if (!Corners++)
					Dx[AxisIdx] = BoxSize * 1 / 3;
				else
					Dy[AxisIdx] = BoxSize * 1 / 3;

				Center[AxisIdx] = -BoxSize;
			}
			else if (mIntersectionFlags.test(AxisIdx * 2 + 1))
			{
				if (!Corners++)
					Dx[AxisIdx] = -BoxSize * 1 / 3;
				else
					Dy[AxisIdx] = -BoxSize * 1 / 3;

				Center[AxisIdx] = BoxSize;
			}
			else
				Dz[AxisIdx] = BoxSize * 2 / 3;
		}

		lcVector3 FaceVerts[] =
		{
			Center - Dz, Center + Dx - Dz, Center + Dz, Center + Dx + Dz,
			Center - Dz, Center + Dy - Dz, Center + Dz, Center + Dy + Dz
		};

		Context->SetVertexBufferPointer(FaceVerts);
		Context->SetVertexFormatPosition(3);

		Context->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
		Context->DrawPrimitives(GL_TRIANGLE_STRIP, 0, 4);
		Context->DrawPrimitives(GL_TRIANGLE_STRIP, 4, 4);
	}
	else if (IntersectionType == 3)
	{
		lcVector3 Signs;

		for (int AxisIdx = 0; AxisIdx < 3; AxisIdx++)
		{
			if (mIntersectionFlags.test(AxisIdx * 2))
				Signs[AxisIdx] = -1.0f;
			else
				Signs[AxisIdx] = 1.0f;
		}

		lcVector3 Center = Signs * BoxSize;
		lcVector3 Dx = lcVector3(BoxSize * 1 / 3 * Signs[0], 0.0f, 0.0f);
		lcVector3 Dy = lcVector3(0.0f, BoxSize * 1 / 3 * Signs[1], 0.0f);
		lcVector3 Dz = lcVector3(0.0f, 0.0f, BoxSize * 1 / 3 * Signs[2]);

		lcVector3 FaceVerts[] =
		{
			Center - Dz, Center - Dx - Dz, Center, Center - Dx,
			Center - Dz, Center - Dy - Dz, Center, Center - Dy,
			Center - Dx, Center - Dy - Dx, Center, Center - Dy
		};

		Context->SetVertexBufferPointer(FaceVerts);
		Context->SetVertexFormatPosition(3);

		Context->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
		Context->DrawPrimitives(GL_TRIANGLE_STRIP, 0, 4);
		Context->DrawPrimitives(GL_TRIANGLE_STRIP, 4, 4);
		Context->DrawPrimitives(GL_TRIANGLE_STRIP, 8, 4);
	}

	glEnable(GL_DEPTH_TEST);

	Context->SetMaterial(LC_MATERIAL_UNLIT_TEXTURE_MODULATE);
	Context->BindTexture2D(gTexFont.GetTexture());
	glEnable(GL_BLEND);

	const char* ViewNames[6] = { "Front", "Back", "Top", "Bottom", "Left", "Right" };

	int MaxText = 0;

	for (int FaceIdx = 0; FaceIdx < 6; FaceIdx++)
	{
		int Width, Height;
		gTexFont.GetStringDimensions(&Width, &Height, ViewNames[FaceIdx]);
		if (Width > MaxText)
			MaxText = Width;
		if (Height > MaxText)
			MaxText = Height;
	}

	float Scale = (BoxSize * 2.0f - 4.0f) / (float)MaxText;

	lcMatrix44 ViewMatrices[6] =
	{
		lcMatrix44(lcVector4(Scale,  0.0f, 0.0f, 0.0f), lcVector4(0.0f,  0.0f, Scale, 0.0f), lcVector4(0.0f, 1.0f, 0.0f, 0.0f), lcVector4(0.0f, -BoxSize - 0.01f, 0.0f, 1.0f)),
		lcMatrix44(lcVector4(-Scale,  0.0f, 0.0f, 0.0f), lcVector4(0.0f,  0.0f, Scale, 0.0f), lcVector4(0.0f, 1.0f, 0.0f, 0.0f), lcVector4(0.0f,  BoxSize + 0.01f, 0.0f, 1.0f)),
		lcMatrix44(lcVector4(Scale,  0.0f, 0.0f, 0.0f), lcVector4(0.0f,  Scale, 0.0f, 0.0f), lcVector4(0.0f, 0.0f, 1.0f, 0.0f), lcVector4(0.0f, 0.0f,  BoxSize + 0.01f, 1.0f)),
		lcMatrix44(lcVector4(Scale,  0.0f, 0.0f, 0.0f), lcVector4(0.0f, -Scale, 0.0f, 0.0f), lcVector4(0.0f, 0.0f, 1.0f, 0.0f), lcVector4(0.0f, 0.0f, -BoxSize - 0.01f, 1.0f)),
		lcMatrix44(lcVector4(0.0f,  Scale, 0.0f, 0.0f), lcVector4(0.0f,  0.0f, Scale, 0.0f), lcVector4(1.0f, 0.0f, 0.0f, 0.0f), lcVector4(BoxSize + 0.01f, 0.0f, 0.0f, 1.0f)),
		lcMatrix44(lcVector4(0.0f, -Scale, 0.0f, 0.0f), lcVector4(0.0f,  0.0f, Scale, 0.0f), lcVector4(1.0f, 0.0f, 0.0f, 0.0f), lcVector4(-BoxSize - 0.01f, 0.0f, 0.0f, 1.0f))
	};

	float TextBuffer[256 * 5 * 3];
	int CharsWritten = 0;

	for (int FaceIdx = 0; FaceIdx < 6; FaceIdx++)
	{
		const char* ViewName = ViewNames[FaceIdx];
		gTexFont.GetTriangles(ViewMatrices[FaceIdx], ViewName, TextBuffer + CharsWritten * 2 * 3 * 5);
		CharsWritten += strlen(ViewName);
	}

	Context->SetVertexBufferPointer(TextBuffer);
	Context->SetVertexFormat(0, 3, 0, 2, 0, false);

	Context->SetColor(0.0f, 0.0f, 0.0f, 1.0f);
	Context->DrawPrimitives(GL_TRIANGLES, 0, CharsWritten * 2 * 3);

	glDisable(GL_BLEND);

	Context->SetViewport(0, 0, Width, Height);
}

bool lcViewCube::OnLeftButtonUp()
{
	const lcPreferences& Preferences = lcGetPreferences();
	if (Preferences.mViewCubeLocation == lcViewCubeLocation::DISABLED)
		return false;

	if (!mIntersectionFlags.any())
		return false;

	lcVector3 Position(0.0f, 0.0f, 0.0f);

	for (int AxisIdx = 0; AxisIdx < 3; AxisIdx++)
	{
		if (mIntersectionFlags.test(AxisIdx * 2))
			Position[AxisIdx] = -BoxSize;
		else if (mIntersectionFlags.test(AxisIdx * 2 + 1))
			Position[AxisIdx] = BoxSize;
	}

	mView->SetViewpoint(Position);

	return true;
}

bool lcViewCube::OnMouseMove()
{
	const lcPreferences& Preferences = lcGetPreferences();
	lcViewCubeLocation Location = Preferences.mViewCubeLocation;

	if (Location == lcViewCubeLocation::DISABLED)
		return false;

	int Width = mView->mWidth;
	int Height = mView->mHeight;
	int ViewportSize = Preferences.mViewCubeSize;
	int Left = (Location == lcViewCubeLocation::BOTTOM_LEFT || Location == lcViewCubeLocation::TOP_LEFT) ? 0 : Width - ViewportSize;
	int Bottom = (Location == lcViewCubeLocation::BOTTOM_LEFT || Location == lcViewCubeLocation::BOTTOM_RIGHT) ? 0 : Height - ViewportSize;
	int x = mView->mInputState.x - Left;
	int y = mView->mInputState.y - Bottom;

	if (x < 0 || x > Width || y < 0 || y > Height)
	{
		if (mIntersectionFlags.any())
		{
			mIntersectionFlags.reset();
			mView->Redraw();
		}

		return false;
	}

	lcVector3 StartEnd[2] = { lcVector3(x, y, 0), lcVector3(x, y, 1) };
	const int Viewport[4] = { 0, 0, ViewportSize, ViewportSize };

	lcUnprojectPoints(StartEnd, 2, GetViewMatrix(), GetProjectionMatrix(), Viewport);
	std::bitset<6> IntersectionFlags;

	float Distance;
	if (lcBoundingBoxRayIntersectDistance(lcVector3(-BoxSize, -BoxSize, -BoxSize), lcVector3(BoxSize, BoxSize, BoxSize), StartEnd[0], StartEnd[1], &Distance, &mIntersection))
	{
		for (int AxisIdx = 0; AxisIdx < 3; AxisIdx++)
		{
			if (mIntersection[AxisIdx] < -BoxSize * 2 / 3)
				IntersectionFlags.set(2 * AxisIdx);
			else if (mIntersection[AxisIdx] > BoxSize * 2 / 3)
				IntersectionFlags.set(2 * AxisIdx + 1);
		}
	}

	if (IntersectionFlags != mIntersectionFlags)
	{
		mIntersectionFlags = IntersectionFlags;
		mView->Redraw();
	}

	return true;
}
