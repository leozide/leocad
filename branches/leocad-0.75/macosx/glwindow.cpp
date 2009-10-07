#include <OpenGL/gl.h>
#include <AGL/agl.h>
#include <Carbon.h>
#include <stdlib.h>
#include <string.h>
#include "glwindow.h"
#include "opengl.h"

typedef struct
{
	AGLContext Context;
	WindowPtr Window;
	DMExtendedNotificationUPP WindowEDMUPP;
	HIViewRef View;
} GLWindowPrivate;

static void SetWindowSize(GLWindow* glw)
{
	GLWindowPrivate* Priv = (GLWindowPrivate*)glw->GetData();

	Rect r;
	GetWindowPortBounds(Priv->Window, &r);

	HIRect bounds;
	HIViewGetBounds(Priv->View, &bounds);

	GLint bufferRect[4];
	bufferRect[0] = (int)bounds.origin.x;
	bufferRect[1] = (int)((r.bottom - r.top) - bounds.origin.y - bounds.size.height);
	bufferRect[2] = (int)bounds.size.width;
	bufferRect[3] = (int)bounds.size.height;

	aglSetInteger(Priv->Context, AGL_BUFFER_RECT, bufferRect);
	aglEnable(Priv->Context, AGL_BUFFER_RECT);

	glw->OnSize(bufferRect[2], bufferRect[3]);
}

static pascal OSStatus GLWindowEventHandler(EventHandlerCallRef Handler, EventRef Event, void* UserData)
{
	OSStatus result = eventNotHandledErr;
	GLWindow* glw = (GLWindow*)UserData;
	GLWindowPrivate* Priv = (GLWindowPrivate*)glw->GetData();
	UInt32 EventKind; 
	WindowRef wnd = NULL;
	SInt16 WindowPart;

	EventKind = GetEventKind(Event);

	switch (EventKind)
	{
		case kEventWindowActivated:
			break;

		case kEventWindowShown:
			if (Priv->Window == FrontWindow())
				SetUserFocusWindow(Priv->Window);
		break;

		case kEventWindowDrawContent:
			glw->OnDraw();
			result = noErr;
			break; 

		case kEventMouseDown:
		{
			EventMouseButton button;
			UInt32 modifiers;
			Point pt;

			GetEventParameter(Event, kEventParamMouseButton, typeMouseButton, NULL, sizeof(EventMouseButton), NULL, &button);
			GetEventParameter(Event, kEventParamMouseLocation, typeQDPoint, NULL, sizeof(Point), NULL, &pt);
			GetEventParameter(Event, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);

			bool Control = (modifiers & controlKey) != 0;
			bool Shift = (modifiers & shiftKey) != 0;

			WindowPart = FindWindow(pt, &wnd);

			if (wnd != NULL)
				SelectWindow(wnd);

			switch (WindowPart)
			{
				case inDrag:
				{
					BitMap bits;
					DragWindow(wnd, pt, &GetQDGlobalsScreenBits(&bits)->bounds);
					aglUpdateContext(Priv->Context);
					result = noErr;
				} break;

				case inContent:
				{
					SetPort(GetWindowPort(Priv->Window));
					GlobalToLocal(&pt);

					int x = pt.h;
					int y = glw->GetHeight() - pt.v;

					if (button == kEventMouseButtonPrimary)
						glw->OnLeftButtonDown(x, y, Control, Shift);
					else if (button == kEventMouseButtonSecondary)
						glw->OnRightButtonDown(x, y, Control, Shift);
					result = noErr;
				} break;
			} 
		} break;

		case kEventWindowBoundsChanged:
		{
			SetWindowSize(glw);
			result = noErr;
		} break;
	}

	return result;
}

// handle display config changes meaing we need to update the GL context via the resize function and check for windwo dimension changes
// also note we redraw the content here as it could be lost in a display config change
void HandleWindowDMEvent (void* UserData, short Message, void* NotifyData)
{
	// Post change notifications only.
	if (Message == kDMNotifyEvent)
	{
		GLWindow* glw = (GLWindow*)UserData;

		if (glw)
		{
			GLWindowPrivate* Priv = (GLWindowPrivate*)glw->GetData();

			// have a valid OpenGl window
			Rect rectPort;
			CGRect viewRect = {{0.0f, 0.0f}, {0.0f, 0.0f}};
			GetWindowPortBounds (Priv->Window, &rectPort);
			viewRect.size.width = (float) (rectPort.right - rectPort.left);
			viewRect.size.height = (float) (rectPort.bottom - rectPort.top);
//			resizeGL (pContextInfo->aglContext, &pContextInfo->camera, pContextInfo->shapeSize, viewRect);
			InvalWindowRect (Priv->Window,  &rectPort); // force redrow
		}
	}
}

// ============================================================================
// GLWindow class

GLWindow::GLWindow(GLWindow* Share)
{
	m_nRef = 0;
	m_pShare = Share;
	m_pData = (GLWindowPrivate*)malloc(sizeof(GLWindowPrivate));
	memset(m_pData, 0, sizeof(GLWindowPrivate));
}

GLWindow::~GLWindow()
{
	free(m_pData);
}

bool GLWindow::Create(void* Data)
{
	GLWindowPrivate* Priv = (GLWindowPrivate*)m_pData;
	GLint attrib[] = { AGL_RGBA, AGL_DOUBLEBUFFER, AGL_DEPTH_SIZE, 16, AGL_NONE };
	AGLPixelFormat fmt;
	AGLContext ctx;
	WindowPtr wnd = (WindowPtr)HIViewGetWindow((HIViewRef)Data);

	// Check for existance of OpenGL.
	if ((Ptr)aglChoosePixelFormat == (Ptr)kUnresolvedCFragSymbolAddress)
		return false;

	// Get an appropriate pixel format.
	fmt = aglChoosePixelFormat(NULL, 0, attrib);

	if (fmt == NULL) 
		return false;

	// Create an AGL context.
	ctx = aglCreateContext(fmt, NULL);

	if (ctx == NULL)
		return false;

	GrafPtr port = NULL;	GetPort(&port);	SetPort((GrafPtr)GetWindowPort(wnd));
	// Attach the CGrafPtr to the context.
	if (aglSetDrawable(ctx, GetWindowPort(wnd)))
	{
		if (!aglSetCurrentContext(ctx))
		{
			aglSetDrawable(ctx, NULL);
			return false;
		}
	}
	else
		return false;

	// Pixel format is no longer needed.
	aglDestroyPixelFormat(fmt);

	// Install event handler.
	EventHandlerUPP myHandlerUPP = NewEventHandlerUPP(GLWindowEventHandler);

	EventTypeSpec EventList[] =
	{
		{ kEventClassWindow, kEventWindowShown },
		{ kEventClassWindow, kEventWindowActivated },
		{ kEventClassWindow, kEventWindowDrawContent },
		{ kEventClassWindow, kEventWindowBoundsChanged },
		{ kEventClassMouse, kEventMouseDown },
	};

	InstallStandardEventHandler(GetWindowEventTarget(wnd)); 
	InstallWindowEventHandler(wnd, myHandlerUPP, GetEventTypeCount(EventList), EventList, this, NULL);

	// Ensure we know when display configs are changed
	ProcessSerialNumber psn = { 0, kCurrentProcess };
	Priv->WindowEDMUPP = NewDMExtendedNotificationUPP(HandleWindowDMEvent);
	DMRegisterExtendedNotifyProc(Priv->WindowEDMUPP, this, 0, &psn);

	// Save context information.
	Priv->Context = ctx;
	Priv->Window = wnd;
	Priv->View = (HIViewRef)Data;

	SetPort(port);

	// Since we're attaching to an existing window, force the first update here.
	SetWindowSize(this);
	OnInitialUpdate();
	Redraw();

	return true;
}

void GLWindow::DestroyContext()
{
	GLWindowPrivate* Priv = (GLWindowPrivate*)m_pData;

	DisposeDMExtendedNotificationUPP(Priv->WindowEDMUPP);
	Priv->WindowEDMUPP = NULL;

  aglSetCurrentContext(Priv->Context);
  aglSetDrawable(Priv->Context, NULL);
  aglSetCurrentContext(NULL);
  aglDestroyContext(Priv->Context);

	Priv->Context = NULL;
	Priv->Window = NULL;
}

void GLWindow::OnInitialUpdate()
{
}

bool GLWindow::MakeCurrent()
{
	GLWindowPrivate* Priv = (GLWindowPrivate*)m_pData;
	return aglSetCurrentContext(Priv->Context);
}

void GLWindow::SwapBuffers()
{
	GLWindowPrivate* Priv = (GLWindowPrivate*)m_pData;
	aglSwapBuffers(Priv->Context);
}

void GLWindow::Redraw()
{
	GLWindowPrivate* Priv = (GLWindowPrivate*)m_pData;
	Rect r;
	GetWindowPortBounds(Priv->Window, &r);
	InvalWindowRect(Priv->Window, &r);
}

void GLWindow::CaptureMouse()
{
}

void GLWindow::ReleaseMouse()
{
}
