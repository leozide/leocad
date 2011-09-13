// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#define _WIN32_WINNT 0x500
#define _CRT_SECURE_NO_WARNINGS

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // MFC support for ribbons and control bars

#include "opengl.h"

#define IDT_LC_SAVETIMER		(WM_USER+200)
#define IDT_LC_WHEELTIMER		(WM_USER+201)

#define WM_LC_UPDATE_LIST		(WM_USER+210)
#define WM_LC_SPLITTER_MOVED	(WM_USER+211)
#define WM_LC_POPUP_CLOSE		(WM_USER+212)
#define WM_LC_UPDATE_STEP_RANGE	(WM_USER+213)
#define WM_LC_SET_STEP			(WM_USER+214)
//	#define WM_LC_FRAMECLOSED		(WM_USER+215)
#define WM_LC_ADD_COMBO_STRING	(WM_USER+217)
#define WM_LC_UPDATE_INFO		(WM_USER+218) // wParam = object, lParam = type
#define WM_LC_UPDATE_SETTINGS	(WM_USER+219) // Preferences changed
#define WM_LC_EDIT_CLOSED		(WM_USER+220) // Terrain grid was edited
#define WM_LC_SET_CURSOR		(WM_USER+221) // New action, must change the view cursor

#define PRINT_NUMBERS	0x001
//	#define PRINT_BRIGHT	0x002
//	#define PRINT_QUALITY	0x004
#define PRINT_BORDER	0x008

// Piecebar settings
//#define PIECEBAR_PREVIEW      0x01 // Show piece preview
#define PIECEBAR_SUBPARTS     0x02 // Show subparts
//#define PIECEBAR_GROUP        0x04 // Group pieces
//#define PIECEBAR_ZOOMPREVIEW  0x08 // Zoom piece preview
//#define PIECEBAR_COMBO        0x10 // Show combobox
#define PIECEBAR_PARTNUMBERS  0x20 // Show part numbers
