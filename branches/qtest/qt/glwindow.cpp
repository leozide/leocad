#include "lc_global.h"
#include "glwindow.h"
#include "opengl.h"
#include <QtOpenGL>

void GL_InitializeExtensions()
{
}

GLWindow::GLWindow(GLWindow *share)
{
	m_pShare = share;
	m_pData = NULL;
}

GLWindow::~GLWindow()
{
}

bool GLWindow::CreateFromWindow(void *data)
{
	m_pData = data;

	return true;
}

void GLWindow::DestroyContext()
{
}

void GLWindow::OnInitialUpdate()
{
	MakeCurrent();

	GL_InitializeSharedExtensions();
//	GL_InitializeExtensions();

//	if (WindowMultisample)
//		glEnable(GL_MULTISAMPLE_ARB);
}

bool GLWindow::MakeCurrent()
{
	QGLWidget* Widget = (QGLWidget*)m_pData;

	Widget->makeCurrent();

	return true;
}

void GLWindow::SwapBuffers()
{
}

void GLWindow::Redraw(bool ForceRedraw)
{
	QGLWidget* Widget = (QGLWidget*)m_pData;

	Widget->updateGL();
}

void GLWindow::CaptureMouse()
{
}

void GLWindow::ReleaseMouse()
{
}

void GLWindow::SetCursor(LC_CURSOR_TYPE Type)
{
}
