// LeoCAD.h : main header file for the LEOCAD application
//

#if !defined(AFX_LEOCAD_H__195E1F4A_3FF2_11D2_8202_D2B1707B2D1B__INCLUDED_)
#define AFX_LEOCAD_H__195E1F4A_3FF2_11D2_8202_D2B1707B2D1B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CCADApp:
// See LeoCAD.cpp for the implementation of this class
//

class CCADApp : public CWinAppEx
{
public:
	CCADApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCADApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	void OnUpdateRecentFileMenu(CCmdUI* /*pCmdUI*/) {};
	void UpdateMRU(char names[4][MAX_PATH]);
	void RegisterLeoCADShellFileTypes();

	//{{AFX_MSG(CCADApp)
	afx_msg void OnHelpUpdates();
	afx_msg void OnHelpHomePage();
	afx_msg void OnHelpEmail();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	HANDLE m_hMutex;
};

extern CCADApp theApp;

#ifdef _DEBUG
void wprintf(char *fmt, ...);
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LEOCAD_H__195E1F4A_3FF2_11D2_8202_D2B1707B2D1B__INCLUDED_)
