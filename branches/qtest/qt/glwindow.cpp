#include "lc_global.h"
#include "glwindow.h"
#include "opengl.h"
#include <QtOpenGL>

GLWindow::GLWindow(GLWindow *share)
{
	mCursorType = LC_CURSOR_DEFAULT;
	mWidget = NULL;
}

GLWindow::~GLWindow()
{
}

bool GLWindow::CreateFromWindow(void *data)
{
	mWidget = data;

	return true;
}

void GLWindow::DestroyContext()
{
}

void GLWindow::OnInitialUpdate()
{
	MakeCurrent();

	GL_InitializeSharedExtensions(mWidget);

//	if (WindowMultisample)
//		glEnable(GL_MULTISAMPLE_ARB);
}

bool GLWindow::MakeCurrent()
{
	QGLWidget* Widget = (QGLWidget*)mWidget;

	Widget->makeCurrent();

	return true;
}

void GLWindow::Redraw(bool ForceRedraw)
{
	QGLWidget* widget = (QGLWidget*)mWidget;

	widget->updateGL();
}

void GLWindow::CaptureMouse()
{
	QGLWidget* widget = (QGLWidget*)mWidget;

	widget->grabMouse();
}

void GLWindow::ReleaseMouse()
{
	QGLWidget* widget = (QGLWidget*)mWidget;

	widget->releaseMouse();
}

void GLWindow::SetCursor(LC_CURSOR_TYPE CursorType)
{
	if (mCursorType == CursorType)
		return;

	const char* Cursors[LC_CURSOR_COUNT] =
	{
		"",                                   // LC_CURSOR_DEFAULT
		":/resources/cursor_insert",          // LC_CURSOR_BRICK
		":/resources/cursor_light",           // LC_CURSOR_LIGHT
		":/resources/cursor_spotlight",       // LC_CURSOR_SPOTLIGHT
		":/resources/cursor_camera",          // LC_CURSOR_CAMERA
		":/resources/cursor_select",          // LC_CURSOR_SELECT
		":/resources/cursor_select_multiple", // LC_CURSOR_SELECT_GROUP
		":/resources/cursor_move",            // LC_CURSOR_MOVE
		":/resources/cursor_rotate",          // LC_CURSOR_ROTATE
		":/resources/cursor_rotatex",         // LC_CURSOR_ROTATEX
		":/resources/cursor_rotatey",         // LC_CURSOR_ROTATEY
		":/resources/cursor_delete",          // LC_CURSOR_DELETE
		":/resources/cursor_paint",           // LC_CURSOR_PAINT
		":/resources/cursor_zoom",            // LC_CURSOR_ZOOM
		":/resources/cursor_zoom_region",     // LC_CURSOR_ZOOM_REGION
		":/resources/cursor_pan",             // LC_CURSOR_PAN
		":/resources/cursor_roll",            // LC_CURSOR_ROLL
		":/resources/cursor_rotate_view",     // LC_CURSOR_ROTATE_VIEW
	};

	int Offsets[LC_CURSOR_COUNT][2] =
	{
		{  0,  0 }, // LC_CURSOR_DEFAULT
		{  8,  3 }, // LC_CURSOR_BRICK
		{ 15, 15 }, // LC_CURSOR_LIGHT
		{  7, 10 }, // LC_CURSOR_SPOTLIGHT
		{ 15,  9 }, // LC_CURSOR_CAMERA
		{  0,  2 }, // LC_CURSOR_SELECT
		{  0,  2 }, // LC_CURSOR_SELECT_GROUP
		{ 15, 15 }, // LC_CURSOR_MOVE
		{ 15, 15 }, // LC_CURSOR_ROTATE
		{ 15, 15 }, // LC_CURSOR_ROTATEX
		{ 15, 15 }, // LC_CURSOR_ROTATEY
		{  0, 10 }, // LC_CURSOR_DELETE
		{ 14, 14 }, // LC_CURSOR_PAINT
		{ 15, 15 }, // LC_CURSOR_ZOOM
		{  9,  9 }, // LC_CURSOR_ZOOM_REGION
		{ 15, 15 }, // LC_CURSOR_PAN
		{ 15, 15 }, // LC_CURSOR_ROLL
		{ 15, 15 }, // LC_CURSOR_ROTATE_VIEW
	};

	QGLWidget* widget = (QGLWidget*)mWidget;

	if (CursorType != LC_CURSOR_DEFAULT && CursorType < LC_CURSOR_COUNT)
	{
		widget->setCursor(QCursor(QPixmap(Cursors[CursorType]), Offsets[CursorType][0], Offsets[CursorType][1]));
		mCursorType = CursorType;
	}
	else
	{
		widget->unsetCursor();
		mCursorType = LC_CURSOR_DEFAULT;
	}
}
