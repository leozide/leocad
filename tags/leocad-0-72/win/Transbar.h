// TransToolBar.h : header file
//

#ifndef _TRANSBAR_H_
#define _TRANSBAR_H_

#define MAX_BUTTONS			32
// difference between width of button(48) and width of buttonimage(30)
#define LARGEBUTTON_DX		18


typedef struct _BUTTONDATA {
    int iBitmap;
    int idCommand;
    BYTE fsState;
    BYTE fsStyle;				// TBSTYLE_DROPDOWN, TBSTYLE_BUTTON
    LPTSTR lpszButtonText;		// string to display in button
	LPTSTR lpszTooltip;
} BUTTONDATA, FAR* LPBUTTONDATA;

typedef struct _TOOLBARDATA {
	DWORD dwStyle;				// TBSTYLE_FLAT, TBSTYLE_LIST
	int idControl;				// control id
	int idbDefault;				// bitmapresource 
	int idbHot;					// bitmapresource when the mouse moves over the button
	int iButtons;				// number of buttens in the toolbar
	int iButtonCX;				// width of the buttons
	int iButtonCY;				// heigth of the buttons
	HIMAGELIST himl;
	BUTTONDATA ButtonData[MAX_BUTTONS];	// structures describing the buttons
} TOOLBARDATA, FAR* LPTOOLBARDATA;

/////////////////////////////////////////////////////////////////////////////
// CTransToolBar window

class CTransToolBar : public CToolBar
{
// Construction
public:
	void SetButtonStyle(int nIndex, UINT nStyle);
	void _GetButton(int nIndex, TBBUTTON* pButton) const;
	void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);
	void _SetButton(int nIndex, TBBUTTON* pButton);
	BOOL Create(HWND hwndOwner);
	TOOLBARDATA m_ToolbarData;
	CTransToolBar();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTransToolBar)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTransToolBar();

	// Generated message map functions
protected:
	//{{AFX_MSG(CTransToolBar)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void OnUpdatePieceGroup(CCmdUI* pCmdUI);

	friend class CToolBar;
};

#endif // _TRANSBAR_H_

/////////////////////////////////////////////////////////////////////////////
