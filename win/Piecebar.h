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

/////////////////////////////////////////////////////////////////////////
// CPiecesBar control bar

class CPiecesBar : public CDockablePane
{
public:
	CPiecesBar();
	virtual ~CPiecesBar();

public:
	void AdjustLayout(int cx, int cy);

protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

	void OnSelChangeColor();
	afx_msg LONG OnSplitterMoved(UINT lParam, LONG wParam);

	DECLARE_MESSAGE_MAP()

public:
	BOOL m_bSubParts;
	BOOL m_bNumbers;
	int m_nPreviewHeight;

	CPiecePreview m_wndPiecePreview;
	CySplitterWnd m_wndSplitter;
	CTreeCtrl m_PiecesTree;
	CPiecesCombo m_wndPiecesCombo;
	CColorsList m_wndColorsList;

	void UpdatePiecesTree(bool SearchOnly);
	void UpdatePiecesTree(const char* OldCategory, const char* NewCategory);
	void SelectPiece(const char* Category, PieceInfo* Info);
	void RefreshPiecesTree();

protected:
	BOOL m_bNoContext;
	CFont m_Font;
};

/////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(PIECEBAR_H_INCLUDED)

