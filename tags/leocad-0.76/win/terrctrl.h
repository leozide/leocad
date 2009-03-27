#if !defined(AFX_TERRCTRL_H__15B2D2A1_D2FF_11D2_8204_EEB0809D9016__INCLUDED_)
#define AFX_TERRCTRL_H__15B2D2A1_D2FF_11D2_8204_EEB0809D9016__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TerrCtrl.h : header file
//

#include <afxtempl.h>

#define TERRAINCTRL_CLASSNAME _T("TerrainCtrl")

typedef struct {
	UINT state; // Cell state (selected/focus etc)
//	float height;
} GRIDCELL; 

typedef struct CELLID {
	int row, col;
    CELLID(int nRow = -1, int nCol = -1)
		: row(nRow), col(nCol) {}
	BOOL operator==(const CELLID rhs)
		{ return (row == rhs.row && col == rhs.col); }
	BOOL operator!=(const CELLID rhs)
		{ return (row != rhs.row || col != rhs.col); }
} CELLID;


// Cell states
#define GS_FOCUSED			0x0001
#define GS_SELECTED			0x0002
#define GS_DROPHILITED		0x0004
#define GS_READONLY			0x0008

// storage typedef for each row in the grid
typedef CArray<GRIDCELL, GRIDCELL> GRID_ROW;

/////////////////////////////////////////////////////////////////////////////
// CTerrainCtrl window

class CTerrainCtrl : public CWnd
{
// Construction
public:
	CTerrainCtrl();
	BOOL Create(const RECT& rect, CWnd* pParentWnd, UINT nID, DWORD dwStyle = WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VISIBLE);
	BOOL SubclassWindow(HWND hWnd);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTerrainCtrl)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	void SetControlPoints(int uCount, int vCount, float** pControl);
	virtual ~CTerrainCtrl();

	// Generated message map functions
protected:
	void EditCell(int nRow, int nCol, UINT nChar);
	BOOL GetCellRect(int nRow, int nCol, LPRECT pRect);
	BOOL GetCellRect(CELLID cell, LPRECT pRect);
	BOOL GetCellOrigin(int nRow, int nCol, LPPOINT p);
	BOOL GetCellOrigin(CELLID cell, LPPOINT p);
	BOOL RedrawCell(int nRow, int nCol, CDC* pDC = NULL);
	BOOL RedrawCell(CELLID cell, CDC* pDC = NULL);
	BOOL IsValid(CELLID cell);
	BOOL IsValid(int nRow, int nCol);
	BOOL IsCellVisible(CELLID cell);
	BOOL IsCellVisible(int nRow, int nCol);
	void EnsureVisible(CELLID cell);
	void EnsureVisible(int nRow, int nCol);
	void SetFocusCell(CELLID cell);
	void SetFocusCell(int nRow, int nCol);
	BOOL SetRowCount(int nRows);
	BOOL SetColumnCount(int nCols);

	CELLID GetCellFromPt(CPoint point, BOOL bAllowFixedCellCheck = TRUE);
	CELLID GetTopleftNonFixedCell();
	BOOL RegisterWindowClass();

	void OnDraw(CDC* pDC);
	void EraseBkgnd(CDC* pDC);
	BOOL DrawCell(CDC* pDC, int nRow, int nCol, CRect rect, BOOL bEraseBk = FALSE);
	BOOL DrawFixedCell(CDC* pDC, int nRow, int nCol, CRect rect, BOOL bEraseBk = FALSE);

	void ResetScrollBars();
	int  GetScrollPos32(int nBar, BOOL bGetTrackPos = FALSE);
	BOOL SetScrollPos32(int nBar, int nPos, BOOL bRedraw = TRUE);

	CFont m_Font;

	COLORREF m_crFixedBkColour;
	COLORREF m_crFixedTextColour;
	COLORREF m_crTextBkColour;

//	m_crTextColour, m_crGridColour;

	int m_nRows;
	int m_nCols;
	int m_nVScrollMax;
	int m_nHScrollMax;

	float** m_pControl;

	int m_MouseMode;

	CELLID m_idCurrentCell;
	CELLID m_LeftClickDownCell;
	CPoint m_LeftClickDownPoint;
	
	// Cell data
	CArray<GRID_ROW, GRID_ROW> m_RowData;

	//{{AFX_MSG(CTerrainCtrl)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	afx_msg LRESULT OnGetFont(WPARAM hFont, LPARAM lParam);
	afx_msg LRESULT OnEditClosed(WPARAM hFont, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

	enum eMouseModes { MOUSE_NOTHING, MOUSE_SELECT_ALL, MOUSE_SELECT_COL, MOUSE_SELECT_ROW,
					   MOUSE_SELECT_CELLS, MOUSE_SCROLLING_CELLS,
					   MOUSE_PREPARE_EDIT, MOUSE_PREPARE_DRAG, MOUSE_DRAGGING };
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TERRCTRL_H__15B2D2A1_D2FF_11D2_8204_EEB0809D9016__INCLUDED_)
