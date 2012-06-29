#ifndef _CLRPOPUP_H_
#define _CLRPOPUP_H_

// CColorPopup messages
#define CPN_SELENDOK		WM_USER + 1001 // Color Picker end OK
#define CPN_SELENDCANCEL	WM_USER + 1002 // Color Picker end (cancelled)

// forward declaration
class CColorPicker;

/////////////////////////////////////////////////////////////////////////////
// CColorPopup window

struct CColorPopupCell
{
	CRect Rect;
	COLORREF Color;
	int ColorIndex;
};

class CColorPopup : public CWnd
{
// Construction
public:
	CColorPopup();
	CColorPopup(CPoint p, int nColor, CWnd* pParentWnd, bool IgnoreMouse);
	void Initialise();

// Attributes
public:

// Operations
public:
	BOOL Create(CPoint p, int nColor, CWnd* pParentWnd);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorPopup)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CColorPopup();

protected:
	void SetWindowSize();
	void CalculateLayout();
	void CreateToolTips();
	void ChangeSelection(int nIndex);
	void EndSelection(int nMessage);
	void DrawCell(CDC* pDC, int nIndex);

// protected attributes
protected:
	int            m_nNumColumns, m_nNumRows;
	int            m_nBoxSize, m_nMargin;
	int            m_nCurrentSel;
	int            m_nChosenColorSel;
	CRect          m_WindowRect;
	CFont          m_Font;
	CPalette       m_Palette;
	int            m_nInitialColor, m_nColor;
	CToolTipCtrl   m_ToolTip;
	CWnd*          m_pParent;
	bool           m_IgnoreMouse;

	CArray<CRect, const CRect&> mGroups;
	CArray<CColorPopupCell, const CColorPopupCell&> mCells;

	// Generated message map functions
protected:
	//{{AFX_MSG(CColorPopup)
	afx_msg void OnNcDestroy();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnQueryNewPalette();
	afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnActivateApp(BOOL bActive, DWORD hTask);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // _CLRPOPUP_H_
