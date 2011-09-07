#if !defined(AFX_PIECECMB_H__557A4642_404A_11D2_8202_C2FDEFF7E618__INCLUDED_)
#define AFX_PIECECMB_H__557A4642_404A_11D2_8202_C2FDEFF7E618__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PieceCmb.h : header file
//

class PieceInfo;

/////////////////////////////////////////////////////////////////////////////
// CPiecesCombo window

class CPiecesCombo : public CComboBox
{
// Construction
public:
	CPiecesCombo();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPiecesCombo)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPiecesCombo();

	// Generated message map functions
protected:
	BOOL m_bAutoComplete;
	void SelectPiece(PieceInfo* Info);

	//{{AFX_MSG(CPiecesCombo)
	afx_msg void OnEditupdate();
	afx_msg void OnSelchange();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PIECECMB_H__557A4642_404A_11D2_8202_C2FDEFF7E618__INCLUDED_)
