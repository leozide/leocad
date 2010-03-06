#ifndef _COLORLST_H_
#define _COLORLST_H_

class CColorToolTipCtrl : public CToolTipCtrl
{
	DECLARE_DYNAMIC(CColorToolTipCtrl)

public:
	CColorToolTipCtrl();
	virtual ~CColorToolTipCtrl();

protected:
	afx_msg void OnNotifyShow(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
};

class CColorTab
{
public:
	CColorTab(const char* Text) : m_Text(Text) { }
	~CColorTab() { }

	void Draw(CDC& dc, CFont& Font, BOOL Selected, BOOL Focus);
	void GetTrapezoid(const CRect& rc, CPoint* pts) const;

	CString	m_Text;
	CRect m_Rect;
	CRgn m_Rgn;
};

struct CColorEntry
{
	CRect Rect;
	const char* Name;
	COLORREF Color;
	int Index;
};

class CColorList : public CWnd
{
public:
	CColorList();
	virtual ~CColorList();

	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

	void SetCurColor(int Index)
	{
		for (int i = 0; i < m_Colors.GetSize(); i++)
		{
			if (m_Colors[i].Index == Index)
			{
				SelectColor(i);
				break;
			}
		}
	}

	int GetCurColor() const
	{
		return m_Colors[m_CurColor].Index;
	}

	void UpdateColorConfig();

protected:
	CColorToolTipCtrl m_ToolTip;

	CFont m_NormalFont;
	CFont m_SelectedFont;

	CTypedPtrArray<CPtrArray, CColorTab*> m_Tabs;
	int m_CurTab;

	CArray<CColorEntry, const CColorEntry&> m_Colors;
	int m_CurColor;

	int m_ColorCols;
	int m_ColorRows;

	void UpdateLayout();
	void SelectTab(int Tab);
	void SelectColor(int Color);

	bool m_ColorFocus;

	CPoint m_MouseDown;
	BOOL m_Tracking;

	void Draw(CDC& dc);

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
