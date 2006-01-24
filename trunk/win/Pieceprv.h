#if !defined(AFX_PIECEPRV_H__FBCDC803_4059_11D2_8202_C2FDEFF7E618__INCLUDED_)
#define AFX_PIECEPRV_H__FBCDC803_4059_11D2_8202_C2FDEFF7E618__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PiecePrv.h : header file
//

class PieceInfo;
class PiecePreview;

/////////////////////////////////////////////////////////////////////////////
// CPiecePreview window

class CPiecePreview : public CWnd
{
// Construction
public:
	CPiecePreview();

// Attributes
public:
	void SetPieceInfo(PieceInfo* pInfo);
	PieceInfo* GetPieceInfo () const;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPiecePreview)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPiecePreview();

	// Generated message map functions
protected:
	PiecePreview* m_Preview;

	//{{AFX_MSG(CPiecePreview)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PIECEPRV_H__FBCDC803_4059_11D2_8202_C2FDEFF7E618__INCLUDED_)
