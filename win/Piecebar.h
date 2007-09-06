// piecebar.h : header file
//
/////////////////////////////////////////////////////////////////////////

#if !defined(PIECEBAR_H_INCLUDED)
#define PIECEBAR_H_INCLUDED

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "ColorLst.h"
#include "PieceCmb.h"
#include "PiecePrv.h"
#include "Splitter.h"
#include "sizecbar.h"
#include "scbarg.h"

/////////////////////////////////////////////////////////////////////////
// CPiecesBar control bar

class CPiecesBar : public CSizingControlBarG
{
public:
	CPiecesBar();

// Overrides
public:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPiecesBar)
	public:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPiecesBar();

public:
	BOOL m_bSubParts;
	BOOL m_bNumbers;
	CColorList    m_wndColorsList;
	CPiecesCombo  m_wndPiecesCombo;
	CPiecePreview m_wndPiecePreview;
	CySplitterWnd m_wndSplitter;

	CTreeCtrl m_PiecesTree;

	void UpdatePiecesTree(bool SearchOnly);
	void UpdatePiecesTree(const char* OldCategory, const char* NewCategory);
	void SelectPiece(const char* Category, PieceInfo* Info);
	void RefreshPiecesTree();

// Generated message map functions
protected:
	BOOL m_bNoContext;
	CFont m_Font;
	int m_nPreviewHeight;
	void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);
	void OnSelChangeColor();
	//{{AFX_MSG(CPiecesBar)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG

	afx_msg LONG OnSplitterMoved(UINT lParam, LONG wParam);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(PIECEBAR_H_INCLUDED)

