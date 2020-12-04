#include "lc_global.h"
#include "lc_glwidget.h"
#include "lc_application.h"
#include "lc_context.h"
#include "camera.h"
#include "texfont.h"

lcGLWidget::lcGLWidget()
{
	mContext = new lcContext();
}

lcGLWidget::~lcGLWidget()
{
	if (mDeleteContext)
		delete mContext;
}

void lcGLWidget::SetContext(lcContext* Context)
{
	if (mDeleteContext)
		delete mContext;

	mContext = Context;
	mDeleteContext = false;
}

void lcGLWidget::MakeCurrent()
{
	mWidget->makeCurrent();
}

void lcGLWidget::Redraw()
{
	mWidget->update();
}

void lcGLWidget::SetCursor(lcCursor CursorType)
{
	if (mCursor == CursorType)
		return;

	struct lcCursorInfo
	{
		int x, y;
		const char* Name;
	};

	const lcCursorInfo Cursors[] =
	{
		{  0,  0, "" },                                 // lcCursor::Default
		{  8,  3, ":/resources/cursor_insert" },        // lcCursor::Brick
		{ 15, 15, ":/resources/cursor_light" },         // lcCursor::Light
		{  7, 10, ":/resources/cursor_spotlight" },     // lcCursor::Spotlight
		{ 15,  9, ":/resources/cursor_camera" },        // lcCursor::Camera
		{  0,  2, ":/resources/cursor_select" },        // lcCursor::Select
		{  0,  2, ":/resources/cursor_select_add" },    // lcCursor::SelectAdd
		{  0,  2, ":/resources/cursor_select_remove" }, // lcCursor::SelectRemove
		{ 15, 15, ":/resources/cursor_move" },          // lcCursor::Move
		{ 15, 15, ":/resources/cursor_rotate" },        // lcCursor::Rotate
		{ 15, 15, ":/resources/cursor_rotatex" },       // lcCursor::RotateX
		{ 15, 15, ":/resources/cursor_rotatey" },       // lcCursor::RotateY
		{  0, 10, ":/resources/cursor_delete" },        // lcCursor::Delete
		{ 14, 14, ":/resources/cursor_paint" },         // lcCursor::Paint
		{  1, 13, ":/resources/cursor_color_picker" },  // lcCursor::ColorPicker
		{ 15, 15, ":/resources/cursor_zoom" },          // lcCursor::Zoom
		{  9,  9, ":/resources/cursor_zoom_region" },   // lcCursor::ZoomRegion
		{ 15, 15, ":/resources/cursor_pan" },           // lcCursor::Pan
		{ 15, 15, ":/resources/cursor_roll" },          // lcCursor::Roll
		{ 15, 15, ":/resources/cursor_rotate_view" },   // lcCursor::RotateView
	};

	static_assert(LC_ARRAY_COUNT(Cursors) == static_cast<int>(lcCursor::Count), "Array size mismatch");

	QGLWidget* widget = (QGLWidget*)mWidget;

	if (CursorType > lcCursor::Default && CursorType < lcCursor::Count)
	{
		const lcCursorInfo& Cursor = Cursors[static_cast<int>(CursorType)];
		widget->setCursor(QCursor(QPixmap(Cursor.Name), Cursor.x, Cursor.y));
		mCursor = CursorType;
	}
	else
	{
		widget->unsetCursor();
		mCursor = lcCursor::Default;
	}
}

lcMatrix44 lcGLWidget::GetProjectionMatrix() const
{
	float AspectRatio = (float)mWidth / (float)mHeight;

	if (mCamera->IsOrtho())
	{
		float OrthoHeight = mCamera->GetOrthoHeight() / 2.0f;
		float OrthoWidth = OrthoHeight * AspectRatio;

		return lcMatrix44Ortho(-OrthoWidth, OrthoWidth, -OrthoHeight, OrthoHeight, mCamera->m_zNear, mCamera->m_zFar * 4);
	}
	else
		return lcMatrix44Perspective(mCamera->m_fovy, AspectRatio, mCamera->m_zNear, mCamera->m_zFar);
}

lcVector3 lcGLWidget::ProjectPoint(const lcVector3& Point) const
{
	int Viewport[4] = { 0, 0, mWidth, mHeight };
	return lcProjectPoint(Point, mCamera->mWorldView, GetProjectionMatrix(), Viewport);
}

lcVector3 lcGLWidget::UnprojectPoint(const lcVector3& Point) const
{
	int Viewport[4] = { 0, 0, mWidth, mHeight };
	return lcUnprojectPoint(Point, mCamera->mWorldView, GetProjectionMatrix(), Viewport);
}

void lcGLWidget::UnprojectPoints(lcVector3* Points, int NumPoints) const
{
	int Viewport[4] = { 0, 0, mWidth, mHeight };
	lcUnprojectPoints(Points, NumPoints, mCamera->mWorldView, GetProjectionMatrix(), Viewport);
}

void lcGLWidget::DrawBackground() const
{
	const lcPreferences& Preferences = lcGetPreferences();

	if (!Preferences.mBackgroundGradient)
	{
		lcVector3 BackgroundColor = lcVector3FromColor(Preferences.mBackgroundSolidColor);
		glClearColor(BackgroundColor[0], BackgroundColor[1], BackgroundColor[2], 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		return;
	}

	lcContext* Context = mContext;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Context->SetDepthWrite(false);
	glDisable(GL_DEPTH_TEST);

	float ViewWidth = (float)mWidth;
	float ViewHeight = (float)mHeight;

	Context->SetWorldMatrix(lcMatrix44Identity());
	Context->SetViewMatrix(lcMatrix44Translation(lcVector3(0.375, 0.375, 0.0)));
	Context->SetProjectionMatrix(lcMatrix44Ortho(0.0f, ViewWidth, 0.0f, ViewHeight, -1.0f, 1.0f));

	Context->SetSmoothShading(true);

	const lcVector3 Color1 = lcVector3FromColor(Preferences.mBackgroundGradientColorTop);
	const lcVector3 Color2 = lcVector3FromColor(Preferences.mBackgroundGradientColorBottom);

	float Verts[] =
	{
		ViewWidth, ViewHeight, Color1[0], Color1[1], Color1[2], 1.0f,
		0.0f,      ViewHeight, Color1[0], Color1[1], Color1[2], 1.0f,
		0.0f,      0.0f,       Color2[0], Color2[1], Color2[2], 1.0f,
		ViewWidth, 0.0f,       Color2[0], Color2[1], Color2[2], 1.0f
	};

	Context->SetMaterial(lcMaterialType::UnlitVertexColor);
	Context->SetVertexBufferPointer(Verts);
	Context->SetVertexFormat(0, 2, 0, 0, 4, false);

	Context->DrawPrimitives(GL_TRIANGLE_FAN, 0, 4);

	Context->SetSmoothShading(false);

	glEnable(GL_DEPTH_TEST);
	Context->SetDepthWrite(true);
}

void lcGLWidget::DrawAxes() const
{
//	glClear(GL_DEPTH_BUFFER_BIT);

	const float Verts[28 * 3] =
	{
	     0.00f,  0.00f,  0.00f, 20.00f,  0.00f,  0.00f, 12.00f,  3.00f,  0.00f, 12.00f,  2.12f,  2.12f,
	    12.00f,  0.00f,  3.00f, 12.00f, -2.12f,  2.12f, 12.00f, -3.00f,  0.00f, 12.00f, -2.12f, -2.12f,
	    12.00f,  0.00f, -3.00f, 12.00f,  2.12f, -2.12f,  0.00f, 20.00f,  0.00f,  3.00f, 12.00f,  0.00f,
	     2.12f, 12.00f,  2.12f,  0.00f, 12.00f,  3.00f, -2.12f, 12.00f,  2.12f, -3.00f, 12.00f,  0.00f,
	    -2.12f, 12.00f, -2.12f,  0.00f, 12.00f, -3.00f,  2.12f, 12.00f, -2.12f,  0.00f,  0.00f, 20.00f,
	     0.00f,  3.00f, 12.00f,  2.12f,  2.12f, 12.00f,  3.00f,  0.00f, 12.00f,  2.12f, -2.12f, 12.00f,
	     0.00f, -3.00f, 12.00f, -2.12f, -2.12f, 12.00f, -3.00f,  0.00f, 12.00f, -2.12f,  2.12f, 12.00f,
	};

	const GLushort Indices[78] =
	{
	     0,  1,  0, 10,  0, 19,
	     1,  2,  3,  1,  3,  4,  1,  4,  5,  1,  5,  6,  1,  6,  7,  1,  7,  8,  1,  8,  9,  1,  9,  2,
	    10, 11, 12, 10, 12, 13, 10, 13, 14, 10, 14, 15, 10, 15, 16, 10, 16, 17, 10, 17, 18, 10, 18, 11,
	    19, 20, 21, 19, 21, 22, 19, 22, 23, 19, 23, 24, 19, 24, 25, 19, 25, 26, 19, 26, 27, 19, 27, 20
	};

	lcMatrix44 TranslationMatrix = lcMatrix44Translation(lcVector3(30.375f, 30.375f, 0.0f));
	lcMatrix44 WorldViewMatrix = mCamera->mWorldView;
	WorldViewMatrix.SetTranslation(lcVector3(0, 0, 0));

	mContext->SetMaterial(lcMaterialType::UnlitColor);
	mContext->SetWorldMatrix(lcMatrix44Identity());
	mContext->SetViewMatrix(lcMul(WorldViewMatrix, TranslationMatrix));
	mContext->SetProjectionMatrix(lcMatrix44Ortho(0, mWidth, 0, mHeight, -50, 50));

	mContext->SetVertexBufferPointer(Verts);
	mContext->SetVertexFormatPosition(3);
	mContext->SetIndexBufferPointer(Indices);

	mContext->SetColor(0.0f, 0.0f, 0.0f, 1.0f);
	mContext->DrawIndexedPrimitives(GL_LINES, 6, GL_UNSIGNED_SHORT, 0);

	mContext->SetColor(0.8f, 0.0f, 0.0f, 1.0f);
	mContext->DrawIndexedPrimitives(GL_TRIANGLES, 24, GL_UNSIGNED_SHORT, 6 * 2);
	mContext->SetColor(0.0f, 0.8f, 0.0f, 1.0f);
	mContext->DrawIndexedPrimitives(GL_TRIANGLES, 24, GL_UNSIGNED_SHORT, (6 + 24) * 2);
	mContext->SetColor(0.0f, 0.0f, 0.8f, 1.0f);
	mContext->DrawIndexedPrimitives(GL_TRIANGLES, 24, GL_UNSIGNED_SHORT, (6 + 48) * 2);

	mContext->SetMaterial(lcMaterialType::UnlitTextureModulate);
	mContext->SetViewMatrix(TranslationMatrix);
	mContext->BindTexture2D(gTexFont.GetTexture());
	glEnable(GL_BLEND);

	float TextBuffer[6 * 5 * 3];
	lcVector3 PosX = lcMul30(lcVector3(25.0f, 0.0f, 0.0f), WorldViewMatrix);
	gTexFont.GetGlyphTriangles(PosX.x, PosX.y, PosX.z, 'X', TextBuffer);
	lcVector3 PosY = lcMul30(lcVector3(0.0f, 25.0f, 0.0f), WorldViewMatrix);
	gTexFont.GetGlyphTriangles(PosY.x, PosY.y, PosY.z, 'Y', TextBuffer + 5 * 6);
	lcVector3 PosZ = lcMul30(lcVector3(0.0f, 0.0f, 25.0f), WorldViewMatrix);
	gTexFont.GetGlyphTriangles(PosZ.x, PosZ.y, PosZ.z, 'Z', TextBuffer + 5 * 6 * 2);

	mContext->SetVertexBufferPointer(TextBuffer);
	mContext->SetVertexFormat(0, 3, 0, 2, 0, false);

	mContext->SetColor(lcVector4FromColor(lcGetPreferences().mAxesColor));
	mContext->DrawPrimitives(GL_TRIANGLES, 0, 6 * 3);

	glDisable(GL_BLEND);
}
