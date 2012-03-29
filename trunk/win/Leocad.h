#ifndef _LEOCAD_H_
#define _LEOCAD_H_

#include "resource.h"       // main symbols

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
};

extern CCADApp theApp;

#ifdef _DEBUG
void wprintf(char *fmt, ...);
#endif

#endif // _LEOCAD_H_
