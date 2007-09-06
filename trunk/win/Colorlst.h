#ifndef _COLORLST_H_
#define _COLORLST_H_

#define LC_COLORLIST_NUM_ROWS 6
#define LC_COLORLIST_NUM_COLS 13

class CColorTab
{
public:
	CColorTab(const char* Text) : m_Text(Text) { }
	~CColorTab() { }

	void Draw(CDC& dc, CFont& Font, BOOL Selected);
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

protected:
	CToolTipCtrl m_ToolTip;

	CFont m_NormalFont;
	CFont m_SelectedFont;

	CPtrArray m_Tabs;
	int m_CurTab;

	CArray<CColorEntry, const CColorEntry&> m_Colors;
	int m_CurColor;

	int m_ColorCols;
	int m_ColorRows;

	void UpdateLayout();
	void SelectTab(int Tab);
	void SelectColor(int Color);

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
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg UINT OnGetDlgCode();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // _COLORLST_H_
