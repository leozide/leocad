/////////////////////////////////////////////////////////////////////////////
// InPlaceEdit.h : header file

#if !defined(AFX_INPLACEEDIT_H__ECD42821_16DF_11D1_992F_895E185F9C72__INCLUDED_)
#define AFX_INPLACEEDIT_H__ECD42821_16DF_11D1_992F_895E185F9C72__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


/////////////////////////////////////////////////////////////////////////////
// CInPlaceEdit window
 
class CInPlaceEdit : public CEdit
{
// Construction
public:
	CInPlaceEdit(CWnd* pParent, CRect& rect, DWORD dwStyle, UINT nID,
				 float* pHeight, CString sInitText, UINT nFirstChar);

// Attributes
public:
 
// Operations
public:
	 void EndEdit();
 
// Overrides
	 // ClassWizard generated virtual function overrides
	 //{{AFX_VIRTUAL(CInPlaceEdit)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL
 
// Implementation
public:
	 virtual ~CInPlaceEdit();
 
// Generated message map functions
protected:
	void ResizeControl();
	//{{AFX_MSG(CInPlaceEdit)
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	float*  m_pHeight;
	CString m_sInitText;
	UINT	m_nLastChar;
	BOOL	m_bExitOnArrows;
};
 
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INPLACEEDIT_H__ECD42821_16DF_11D1_992F_895E185F9C72__INCLUDED_)
