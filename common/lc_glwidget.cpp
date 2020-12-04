#include "lc_global.h"
#include "lc_glwidget.h"
#include "lc_application.h"
#include "lc_context.h"

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
