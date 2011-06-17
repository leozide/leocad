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

class CPiecesBar : public CControlBar
{
public:
	CPiecesBar();

// Attributes
public:
	CSize m_sizeHorz;
	CSize m_sizeVert;
	CSize m_sizeFloat;
	BOOL IsHorzDocked() const;
	BOOL IsVertDocked() const;

// Operations
public:

// Overrides
public:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPiecesBar)
	public:
	virtual BOOL Create(LPCTSTR lpszWindowName, CWnd* pParentWnd, CSize sizeDefault, BOOL bHasGripper, UINT nID, DWORD dwStyle = WS_CHILD | WS_VISIBLE | CBRS_TOP);
	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);
	virtual CSize CalcDynamicLayout(int nLength, DWORD dwMode);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPiecesBar();

protected:
	// implementation helpers
	void StartTracking();
	void StopTracking(BOOL bAccept);
	void OnTrackUpdateSize(CPoint& point);
	void OnTrackInvertTracker();
	virtual CSize CalcMaxSize();
	virtual BOOL QueryDragFullWindows() const;

protected:
	// used for resizing
	CSize   m_sizeMin;
	CSize   m_sizeMax;
	CPoint  m_ptOld;
	CRect   m_rectBorder;
	BOOL    m_bTracking;
	BOOL    m_bDragShowContent;
	CSize   m_sizeOld;

	BOOL    m_bInRecalcNC;
	UINT    m_nDockBarID;
	int     m_cxEdge;
	BOOL    m_bHasGripper;
	int     m_cyGripper;
	CRect   m_rectGripper;

	void LoadState();
	void SaveState();

public:
	BOOL m_bSubParts;
	BOOL m_bNumbers;
	CColorsList   m_wndColorsList;
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
	afx_msg void OnPaint();
	afx_msg void OnNcPaint();
	afx_msg void OnWindowPosChanged(WINDOWPOS FAR* lpwndpos);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
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

