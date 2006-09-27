//////////////////////////////////////////////////////////////////////////////
//
// RollupCtrl.cpp
//
// 
// Code Johann Nadalutti
// Mail: jnadalutti@hotmail.com
//
//////////////////////////////////////////////////////////////////////////////
//
// This code is free for personal and commercial use, providing this 
// notice remains intact in the source files and all eventual changes are
// clearly marked with comments.
//
// No warrantee of any kind, express or implied, is included with this
// software; use at your own risk, responsibility for damages (if any) to
// anyone resulting from the use of this software rests entirely with the
// user.
//
//////////////////////////////////////////////////////////////////////////////
// History
// --------
// #v1.0
//	31/03/01:	Created
//
// #v1.01
//	13/04/01:	Added ScrollToPage() method
//				Added automatic page visibility to ExpandPage() method
//				Added Mousewheel support
//	15/04/01:	Added mouse capture checking on WM_MOUSEMOVE dialog msg
//				Added SetCursor() on Dialog WM_SETCURSOR
//				Added MovePageAt() method
//	17/04/01:	Fixed Group Boxes displayed over Buttons
//	20/04/01:	Added IsPageExpanded() and IsPageExpanded() methods
//				Added PopupMenu
//				Added Button subclassing (now button's focus not drawn)
//
// #v1.02
//	10/06/02: Added pages dividing up into columns 
//	17/06/02: Added SetPageCaption() method
//	19/11/02: Fixed _RemovePage() method
//	29/10/03: Background color for AfxRegisterWndClass()
//
// Note
// -----
//	Dialog box width is
//		RollupCtrlClientRect.Width() - RC_SCROLLBARWIDTH - (RC_GRPBOXINDENT*2)
//
//
// Thanks to
// ----------
//
// PJ Arends, Ramon Smits, Uwe Keim, Daniel Madden, Do Quyet Tien,
// Ravi Bhavnani, Masaaki Onishi, Bernard Desfour ...
// and all others users for their comments.
//

#pragma once

#include <afxtempl.h>

/////////////////////////////////////////////////////////////////////////////
// CRollupCtrl structures and defines

	struct RC_PAGEINFO {
		CString		cstrCaption;
		CWnd*			pwndTemplate;
		CButton*	pwndButton;
		CButton*	pwndGroupBox;
		BOOL			bExpanded;
		BOOL			bEnable;
		BOOL			bAutoDestroyTpl;
		WNDPROC 	pOldDlgProc;		//Old wndTemplate(Dialog) window proc
		WNDPROC 	pOldButProc;		//Old wndTemplate(Button) window proc
	};

	#define	RC_PGBUTTONHEIGHT				18
	#define	RC_SCROLLBARWIDTH				6
	#define RC_CURSOR								MAKEINTRESOURCE(IDC_SIZENS)
	#define RC_MINCOLUMNWIDTH				16
	#define	RC_GRPBOXINDENT					6
	#define	RC_SCROLLBARCOLOR				RGB(150,180,180)

	//TrackMenu IDs
	#define RC_MID_EXPANDALL				0x100
	#define RC_MID_COLLAPSEALL			0x101
	#define RC_MID_STARTPAGES				0x102

/////////////////////////////////////////////////////////////////////////////
// CRollupCtrl window

class CRollupCtrl : public CWnd
{
	DECLARE_DYNCREATE(CRollupCtrl)

public:

	// Constructor-Destructor
	CRollupCtrl();
	virtual ~CRollupCtrl();

	// Methods
	BOOL	Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

	int		InsertPage(const char* caption, CDialog* pwndTemplate, BOOL bAutoDestroyTpl=TRUE, int idx=-1);	//Return page zero-based index
	int		InsertPage(const char* caption, UINT nIDTemplate, int idx=-1);	//Return page zero-based index
	int		InsertPage(const char* caption, UINT nIDTemplate, CRuntimeClass* rtc, int idx=-1);	//Return page zero-based index

	void	RemovePage(int idx);	//idx is a zero-based index
	void	RemoveAllPages();

	void	ExpandPage(int idx, BOOL bExpand=TRUE, BOOL bScrollToPage=TRUE);	//idx is a zero-based index
	void	ExpandAllPages(BOOL bExpand=TRUE);

	void	EnablePage(int idx, BOOL bEnable=TRUE);	//idx is a zero-based index
	void	EnableAllPages(BOOL bEnable=TRUE);

	const RC_PAGEINFO*	GetPageInfo(int idx);

	// New v1.01 Methods
	void	ScrollToPage(int idx, BOOL bAtTheTop=TRUE);
	int		MovePageAt(int idx, int newidx);	//newidx can be equal to -1 (move at end)

	BOOL	IsPageExpanded(int idx);
	BOOL	IsPageEnabled(int idx);
	int		GetPagesCount()		{ return (int)m_PageList.GetSize(); }

	// New v1.02 Methods
	BOOL	IsAutoColumnsEnabled()								{ return m_bEnabledAutoColumns;	}
	void	EnableAutoColumns(BOOL bEnable=TRUE);
	BOOL	SetColumnWidth(int nWidth);						//nWidth must be superior to RC_MINCOLUMNWIDTH
	BOOL	SetPageCaption(int idx, LPCSTR caption);

	//Helpers
	void	RecalLayout();

protected:

	// Internal methods
	int		GetPageIdxFromButtonHWND(HWND hwnd);
	void	_ExpandPage(RC_PAGEINFO* pi, BOOL bExpand);
	void	_EnablePage(RC_PAGEINFO* pi, BOOL bEnable);
	int		_InsertPage(const char* caption, CDialog* dlg, int idx, BOOL bAutoDestroyTpl);
	void	_RemovePage(int idx);

	// Datas
	CString	m_strMyClass;
	CArray	<RC_PAGEINFO*,RC_PAGEINFO*>		m_PageList;
	int			m_StartYPos, m_PageHeight;
	int			m_OldMouseYPos, m_SBOffset;
	CBrush	m_cbrush;
	CMenu		m_cmenuCtxt;
	int			m_nColumnWidth;
	BOOL		m_bEnabledAutoColumns;
	BOOL		m_bEnableGroups;

	// Window proc
	static LRESULT CALLBACK DlgWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK ButWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRollupCtrl)
	protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	//{{AFX_MSG(CRollupCtrl)
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint pos );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

