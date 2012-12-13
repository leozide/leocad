#if !defined(AFX_KEYEDIT_H__E567D1C8_2DC3_4E8F_BA7D_CF628525F55C__INCLUDED_)
#define AFX_KEYEDIT_H__E567D1C8_2DC3_4E8F_BA7D_CF628525F55C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// keyedit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CKeyEdit window

class CKeyEdit : public CEdit
{
// Construction
public:
	CKeyEdit();

// Attributes
public:
	char m_Key;
	bool m_Control;
	bool m_Shift;

	void ResetKey();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CKeyEdit)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CKeyEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CKeyEdit)
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_KEYEDIT_H__E567D1C8_2DC3_4E8F_BA7D_CF628525F55C__INCLUDED_)
