#ifndef __MFWND_H__
#define __MFWND_H__

// MFWind.h : header file
//

#include "system.h"

/////////////////////////////////////////////////////////////////////////////
// CMinifigWnd window

class CMinifigWnd : public CWnd
{
// Construction
public:
	CMinifigWnd();

// Attributes
protected:
	HGLRC m_hrc;
	CClientDC* m_pDC;
	CPalette* m_pPal;

// Operations
public:
	void DrawScene();
	int InitGL();
	LC_MINIFIGDLG_OPTS* m_pFig;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMinifigWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMinifigWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMinifigWnd)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // __MFWND_H__
