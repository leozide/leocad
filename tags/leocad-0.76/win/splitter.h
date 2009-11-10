// Splitter.h : header file
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_XYSPLITTERWND_H__8E3E5264_02A9_11D2_BF99_000021000B7C__INCLUDED_)
#define AFX_XYSPLITTERWND_H__8E3E5264_02A9_11D2_BF99_000021000B7C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CxSplitterWnd window

class CxSplitterWnd : public CWnd
{
// Construction
public:
	CxSplitterWnd();

// Attributes
public:

	// Operations
public:
	BOOL BindWithControl(CWnd *parent, DWORD ctrlId);
	void Unbind(void);

	void SetMinWidth(int left, int right);

	BOOL AttachAsLeftPane(DWORD ctrlId);
	BOOL AttachAsRightPane(DWORD ctrlId);
	BOOL DetachAllPanes(void);
	void RecalcLayout(void);

protected:
	BOOL GetMouseClipRect(LPRECT rectClip, CPoint point);

	CWnd        *m_Parent;
	CDWordArray m_leftIds, m_rightIds;
	int         m_minLeft, m_minRight;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxSplitterWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CxSplitterWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(CxSplitterWnd)
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CySplitterWnd window

class CySplitterWnd : public CWnd
{
// Construction
public:
	CySplitterWnd();

// Attributes
public:

// Operations
public:
	BOOL BindWithControl(CWnd *parent, DWORD ctrlId);
	void Unbind(void);

	void SetMinHeight(int above, int below);

	BOOL AttachAsAbovePane(DWORD ctrlId);
	BOOL AttachAsBelowPane(DWORD ctrlId);
	BOOL DetachAllPanes(void);
	void RecalcLayout(void);

protected:
	BOOL GetMouseClipRect(LPRECT rectClip, CPoint point);

	CWnd        *m_Parent;
	CDWordArray m_aboveIds, m_belowIds;
	int         m_minAbove, m_minBelow;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CySplitterWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CySplitterWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(CySplitterWnd)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XYSPLITTERWND_H__8E3E5264_02A9_11D2_BF99_000021000B7C__INCLUDED_)
