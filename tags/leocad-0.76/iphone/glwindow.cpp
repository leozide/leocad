#import "EAGLView.h"
#include <stdio.h>
#include "opengl.h"
#include "glwindow.h"

// =============================================================================
// GLWindow class

GLWindow::GLWindow(GLWindow *share)
{
	m_pShare = share;
	m_pData = NULL;
}

GLWindow::~GLWindow()
{
	DestroyContext();
}

bool GLWindow::Create(void *data)
{
	m_pData = data;
	OnInitialUpdate();
	return true;
}

void GLWindow::DestroyContext()
{
}

void GLWindow::OnInitialUpdate()
{
	MakeCurrent();
	GL_InitializeExtensions();
}

bool GLWindow::MakeCurrent()
{
	EAGLView* View = (EAGLView*)m_pData;
	[View MakeCurrent];
	
	return true;
}

void GLWindow::SwapBuffers()
{
	EAGLView* View = (EAGLView*)m_pData;
	[View SwapBuffers];
}

void GLWindow::Redraw(bool ForceRedraw)
{
	EAGLView* View = (EAGLView*)m_pData;
	[View setNeedsRedraw];
}

void GLWindow::CaptureMouse()
{
}

void GLWindow::ReleaseMouse()
{
}
