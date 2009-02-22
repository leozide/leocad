// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#if !defined(AFX_STDAFX_H__195E1F4C_3FF2_11D2_8202_D2B1707B2D1B__INCLUDED_)
#define AFX_STDAFX_H__195E1F4C_3FF2_11D2_8202_D2B1707B2D1B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define _CRT_SECURE_NO_DEPRECATE 1

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

#if _MSC_VER >= 1400

#ifndef WINVER                // Allow use of features specific to Windows XP or later.
#define WINVER 0x0501         // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT          // Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501   // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINDOWS        // Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE             // Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600      // Change this to the appropriate value to target other versions of IE.
#endif

#endif

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcoll.h>
#include <afxtempl.h>

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#if _MFC_VER >= 0x0710
typedef DWORD ACTIVATEAPPPARAM;
#else
typedef HTASK ACTIVATEAPPPARAM;
#endif

#if _MFC_VER < 0x0800
#define _strcmpi strcmp
#define _strlwr strlwr
#define _strupr strupr
#else
#define snprintf _snprintf
#endif

#if _MFC_VER >= 0x0800
typedef LRESULT NCHITTESTRETURN;
#else
typedef UINT NCHITTESTRETURN;
#endif

#include "opengl.h"

#define IDT_LC_SAVETIMER		(WM_USER+200)
#define IDT_LC_WHEELTIMER		(WM_USER+201)

#define WM_LC_UPDATE_LIST		(WM_USER+210)
#define WM_LC_SPLITTER_MOVED	(WM_USER+211)
#define WM_LC_POPUP_CLOSE		(WM_USER+212)
#define WM_LC_UPDATE_STEP_RANGE	(WM_USER+213)
#define WM_LC_SET_STEP			(WM_USER+214)
//	#define WM_LC_FRAMECLOSED		(WM_USER+215)
#define WM_LC_WHEEL_PAN			(WM_USER+216)
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

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__195E1F4C_3FF2_11D2_8202_D2B1707B2D1B__INCLUDED_)
