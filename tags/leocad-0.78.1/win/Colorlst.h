#ifndef _COLORLST_H_
#define _COLORLST_H_

class CColorToolTipCtrl : public CMFCToolTipCtrl
{
	DECLARE_DYNAMIC(CColorToolTipCtrl)

public:
	CColorToolTipCtrl(CMFCToolTipInfo* pParams = NULL)
		: CMFCToolTipCtrl(pParams)
	{
	}

	virtual ~CColorToolTipCtrl()
	{
	}

protected:
	virtual CSize GetIconSize();
	virtual BOOL OnDrawIcon(CDC* pDC, CRect rectImage);

	DECLARE_MESSAGE_MAP()
};

struct CColorListGroup
{
	CRect Rect;
};

struct CColorListCell
{
	CRect Rect;
	COLORREF Color;
	int ColorIndex;
};

class CColorList : public CWnd
{
public:
	CColorList();
	virtual ~CColorList();

	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

	void SetColorIndex(int ColorIndex)
	{
		for (int CellIdx = 0; CellIdx < mCells.GetSize(); CellIdx++)
		{
			if (mCells[CellIdx].ColorIndex == ColorIndex)
			{
				SelectCell(CellIdx);
				return;
			}
		}
	}

	int GetColorIndex() const
	{
		return mCells[mCurCell].ColorIndex;
	}

protected:
	CArray<CColorListGroup, const CColorListGroup&> mGroups;
	CArray<CColorListCell, const CColorListCell&> mCells;

	int mColumns;
	int mRows;

	int mCurCell;
	CPoint mMouseDown;
	BOOL mTracking;

	CColorToolTipCtrl mToolTip;

	void UpdateColors();
	void UpdateLayout();
	void Draw(CDC& dc);
	void SelectCell(int CellIdx);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorList)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	//{{AFX_MSG(CColorList)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg UINT OnGetDlgCode();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif // _COLORLST_H_