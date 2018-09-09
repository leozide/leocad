#include "lc_global.h"
#include "lc_viewcube.h"
#include "view.h"
#include "lc_context.h"
#include "texfont.h"
#include "lc_application.h"

//todo: move these
const float BoxSize = 10.0f;
const float EdgeSize = BoxSize * 1.0f / 3.0f;
const float CenterSize = BoxSize * 2.0f / 3.0f;

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

	const lcVector3 BoxVerts[56] =
	{
		lcVector3( BoxSize, -CenterSize, -CenterSize), lcVector3( BoxSize, CenterSize, -CenterSize), lcVector3( BoxSize, -CenterSize, CenterSize), lcVector3( BoxSize, CenterSize, CenterSize),
		lcVector3(-BoxSize, -CenterSize, -CenterSize), lcVector3(-BoxSize, CenterSize, -CenterSize), lcVector3(-BoxSize, -CenterSize, CenterSize), lcVector3(-BoxSize, CenterSize, CenterSize),
		lcVector3(-CenterSize,  BoxSize, -CenterSize), lcVector3(CenterSize,  BoxSize, -CenterSize), lcVector3(-CenterSize,  BoxSize, CenterSize), lcVector3(CenterSize,  BoxSize, CenterSize),
		lcVector3(-CenterSize, -BoxSize, -CenterSize), lcVector3(CenterSize, -BoxSize, -CenterSize), lcVector3(-CenterSize, -BoxSize, CenterSize), lcVector3(CenterSize, -BoxSize, CenterSize),
		lcVector3(-CenterSize, -CenterSize,  BoxSize), lcVector3(CenterSize, -CenterSize,  BoxSize), lcVector3(-CenterSize, CenterSize,  BoxSize), lcVector3(CenterSize, CenterSize,  BoxSize),
		lcVector3(-CenterSize, -CenterSize, -BoxSize), lcVector3(CenterSize, -CenterSize, -BoxSize), lcVector3(-CenterSize, CenterSize, -BoxSize), lcVector3(CenterSize, CenterSize, -BoxSize),

		lcVector3( BoxSize, -BoxSize, -CenterSize), lcVector3( BoxSize, BoxSize, -CenterSize), lcVector3( BoxSize, -BoxSize, CenterSize), lcVector3( BoxSize, BoxSize, CenterSize),
		lcVector3(-BoxSize, -BoxSize, -CenterSize), lcVector3(-BoxSize, BoxSize, -CenterSize), lcVector3(-BoxSize, -BoxSize, CenterSize), lcVector3(-BoxSize, BoxSize, CenterSize),
		lcVector3(-CenterSize,  BoxSize, -BoxSize), lcVector3(CenterSize,  BoxSize, -BoxSize), lcVector3(-CenterSize,  BoxSize, BoxSize), lcVector3(CenterSize,  BoxSize, BoxSize),
		lcVector3(-CenterSize, -BoxSize, -BoxSize), lcVector3(CenterSize, -BoxSize, -BoxSize), lcVector3(-CenterSize, -BoxSize, BoxSize), lcVector3(CenterSize, -BoxSize, BoxSize),
		lcVector3(-BoxSize, -CenterSize,  BoxSize), lcVector3(BoxSize, -CenterSize,  BoxSize), lcVector3(-BoxSize, CenterSize,  BoxSize), lcVector3(BoxSize, CenterSize,  BoxSize),
		lcVector3(-BoxSize, -CenterSize, -BoxSize), lcVector3(BoxSize, -CenterSize, -BoxSize), lcVector3(-BoxSize, CenterSize, -BoxSize), lcVector3(BoxSize, CenterSize, -BoxSize),

		lcVector3( BoxSize, -BoxSize, -BoxSize), lcVector3( BoxSize, BoxSize, -BoxSize), lcVector3( BoxSize, -BoxSize, BoxSize), lcVector3( BoxSize, BoxSize, BoxSize),
		lcVector3(-BoxSize, -BoxSize, -BoxSize), lcVector3(-BoxSize, BoxSize, -BoxSize), lcVector3(-BoxSize, -BoxSize, BoxSize), lcVector3(-BoxSize, BoxSize, BoxSize)
	};

	const GLushort BoxIndices[36 + 144 + 144 + 24] =
	{
		0, 1, 2, 3, 2, 1, 5, 6, 7, 6, 5, 4,
		10, 9, 8, 9, 10, 11, 13, 15, 14, 12, 13, 14, 
		16, 17, 18, 19, 18, 17, 21, 22, 23, 22, 21, 20,

		25, 3, 1, 3, 25, 27, 9, 11, 25, 27, 25, 11,
		0, 2, 24, 26, 24, 2, 24, 15, 13, 15, 24, 26,
		2, 3, 43, 2, 43, 41, 19, 17, 43, 41, 43, 17,
		47, 1, 0, 45, 47, 0, 47, 21, 23, 21, 47, 45,
		5, 7, 29, 31, 29, 7, 10, 8, 31, 8, 29, 31,
		28, 6, 4, 6, 28, 30, 12, 14, 28, 30, 28, 14,
		42, 7, 6, 40, 42, 6, 42, 16, 18, 16, 42, 40,
		4, 5, 46, 4, 46, 44, 22, 20, 46, 44, 46, 20,
		34, 11, 10, 34, 35, 11, 35, 34, 19, 18, 19, 34,
		8, 9, 32, 9, 33, 32, 23, 32, 33, 32, 23, 22,
		14, 15, 38, 15, 39, 38, 17, 38, 39, 38, 17, 16,
		36, 13, 12, 36, 37, 13, 37, 36, 21, 20, 21, 36,

		51, 3, 27, 43, 3, 51, 27, 11, 51, 51, 11, 35, 35, 19, 51, 51, 19, 43,
		25, 1, 49, 49, 1, 47, 49, 9, 25, 33, 9, 49, 49, 23, 33, 47, 23, 49,
		26, 2, 50, 50, 2, 41, 50, 15, 26, 39, 15, 50, 50, 17, 39, 41, 17, 50,
		48, 0, 24, 45, 0, 48, 24, 13, 48, 48, 13, 37, 37, 21, 48, 48, 21, 45,
		31, 7, 55, 55, 7, 42, 55, 10, 31, 34, 10, 55, 55, 18, 34, 42, 18, 55,
		53, 5, 29, 46, 5, 53, 29, 8, 53, 53, 8, 32, 32, 22, 53, 53, 22, 46,
		54, 6, 30, 40, 6, 54, 30, 14, 54, 54, 14, 38, 38, 16, 54, 54, 16, 40,
		28, 4, 52, 52, 4, 44, 52, 12, 28, 36, 12, 52, 52, 20, 36, 44, 20, 52,

		48, 52, 49, 53, 50, 54, 51, 55, 48, 49, 50, 51, 52, 53, 54, 55, 48, 50, 49, 51, 52, 54, 53, 55
	};

	Context->SetMaterial(LC_MATERIAL_UNLIT_COLOR);
	Context->SetWorldMatrix(lcMatrix44Identity());
	Context->SetViewMatrix(GetViewMatrix());
	Context->SetProjectionMatrix(GetProjectionMatrix());

	Context->SetVertexBufferPointer(BoxVerts);
	Context->SetVertexFormatPosition(3);
	Context->SetIndexBufferPointer(BoxIndices);

	glDepthFunc(GL_ALWAYS);
	glEnable(GL_CULL_FACE);

	Context->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	Context->DrawIndexedPrimitives(GL_TRIANGLES, 36 + 144 + 144, GL_UNSIGNED_SHORT, 0);

	int IntersectionType = mIntersectionFlags.count();
	if (IntersectionType == 1)
	{
		for (int FlagIdx = 0; FlagIdx < 6; FlagIdx++)
		{
			if (mIntersectionFlags.test(FlagIdx))
			{
				Context->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
				Context->DrawIndexedPrimitives(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, FlagIdx * 6 * 2);
				break;
			}
		}
	}
	else if (IntersectionType == 2)
	{
		int First;

		if (mIntersectionFlags.test(0))
		{
			if (mIntersectionFlags.test(2))
				First = 36;
			else if (mIntersectionFlags.test(3))
				First = 36 + 12;
			else if (mIntersectionFlags.test(4))
				First = 36 + 24;
			else
				First = 36 + 36;
		}
		else if (mIntersectionFlags.test(1))
		{
			if (mIntersectionFlags.test(2))
				First = 36 + 48;
			else if (mIntersectionFlags.test(3))
				First = 36 + 48 + 12;
			else if (mIntersectionFlags.test(4))
				First = 36 + 48 + 24;
			else
				First = 36 + 48 + 36;
		}
		else if (mIntersectionFlags.test(2))
		{
			if (mIntersectionFlags.test(4))
				First = 36 + 96;
			else
				First = 36 + 96 + 12;
		}
		else
		{
			if (mIntersectionFlags.test(4))
				First = 36 + 96 + 24;
			else
				First = 36 + 96 + 36;
		}

		Context->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
		Context->DrawIndexedPrimitives(GL_TRIANGLES, 12, GL_UNSIGNED_SHORT, First * 2);
	}
	else if (IntersectionType == 3)
	{
		int First = 36 + 144;

		if (mIntersectionFlags.test(1))
			First += 72;

		if (mIntersectionFlags.test(3))
			First += 36;

		if (mIntersectionFlags.test(5))
			First += 18;

		Context->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
		Context->DrawIndexedPrimitives(GL_TRIANGLES, 18, GL_UNSIGNED_SHORT, First * 2);
	}

	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);

	Context->SetColor(0.0f, 0.0f, 0.0f, 1.0f);
	Context->DrawIndexedPrimitives(GL_LINES, 24, GL_UNSIGNED_SHORT, (36 + 144 + 144) * 2);

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
			if (mIntersection[AxisIdx] > BoxSize * 2 / 3)
				IntersectionFlags.set(2 * AxisIdx);
			else if (mIntersection[AxisIdx] < -BoxSize * 2 / 3)
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
