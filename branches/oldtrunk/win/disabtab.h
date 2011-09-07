////////////////////////////////////////////////////////////////
// 1998 Microsoft Systems Journal. 
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
// Compiles with Visual C++ 5.0 on Windows 95
// See DisabTab.cpp
// 
class CTabCtrlWithDisable : public CTabCtrl 
{
	DECLARE_DYNAMIC(CTabCtrlWithDisable)
public:
	BOOL m_bPrintOnly;
	CTabCtrlWithDisable();
	virtual ~CTabCtrlWithDisable();

	// functions you must implement/call
	BOOL IsTabEnabled(int iTab);					// you must override
	BOOL TranslatePropSheetMsg(MSG* pMsg);			// call from prop sheet
	BOOL SubclassDlgItem(UINT nID, CWnd* pParent);	// non-virtual override

	// helpers
	int	NextEnabledTab(int iTab, BOOL bWrap);	// get next enabled tab
	int	PrevEnabledTab(int iTab, BOOL bWrap);	// get prev enabled tab
	BOOL SetActiveTab(UINT iNewTab);			// set tab (fail if disabled)

protected:
	//{{AFX_MSG(CTabCtrlWithDisable)
	//}}AFX_MSG
	afx_msg	BOOL OnSelChanging(NMHDR* pNmh, LRESULT* pRes);

	DECLARE_MESSAGE_MAP()

	// MFC overrides
	virtual	BOOL PreTranslateMessage(MSG* pMsg);
	virtual	void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

	// override to draw text only; eg, colored text or different font
	virtual	void OnDrawText(CDC& dc, CRect rc, CString sText, BOOL bDisabled);
};

