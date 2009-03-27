// BMPMenu.h : header file
// Version : 2.3

#include <afxtempl.h>

#ifndef _BMPMENU_H_
#define _BMPMENU_H_

// CBMPMenuData class. Fill this class structure to define a single menu item:

class CBMPMenuData
{
wchar_t *m_szMenuText;
public:
CBMPMenuData () {menuIconNormal=-1;xoffset=-1;bitmap=NULL;
               nFlags=0;nID=0;syncflag=0;m_szMenuText=NULL;};
void SetAnsiString(LPCSTR szAnsiString);
void SetWideString(const wchar_t *szWideString);
const wchar_t *GetWideString(void) {return m_szMenuText;};
~CBMPMenuData ();
CString GetString(void);//returns the menu text in ANSI or UNICODE
int xoffset;
int menuIconNormal;
UINT nFlags,nID,syncflag;
CImageList *bitmap;
};

//struct CMenuItemInfo : public MENUITEMINFO {
struct CMenuItemInfo : public 
//MENUITEMINFO 
#ifndef UNICODE   //SK: this fixes warning C4097: typedef-name 'MENUITEMINFO' used as synonym for class-name 'tagMENUITEMINFOA'
tagMENUITEMINFOA
#else
tagMENUITEMINFOW
#endif
    {
	CMenuItemInfo()
	{ memset(this, 0, sizeof(MENUITEMINFO));
	  cbSize = sizeof(MENUITEMINFO);
	}
};




typedef enum {Normal,TextOnly} HIGHLIGHTSTYLE;

#ifndef UNICODE
#define AppendODMenu AppendODMenuA
#define ModifyODMenu ModifyODMenuA
#else
#define AppendODMenu AppendODMenuW
#define ModifyODMenu ModifyODMenuW
#endif



class CBMPMenu : public CMenu  // Derived from CMenu
{
	// Construction
public:
	CBMPMenu(); 
	// Attributes
protected:
	CTypedPtrArray<CPtrArray, CBMPMenuData*> m_MenuList;  // Stores list of menu items 
	// When loading an owner-drawn menu using a Resource, CBMPMenu must keep track of
	// the popup menu's that it creates. Warning, this list *MUST* be destroyed
	// last item first :)
	CImageList m_List;
	
	CTypedPtrArray<CPtrArray, CBMPMenu*>  m_SubMenus;  // Stores list of sub-menus 
	// Operations
public: 
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCustomMenu)
	//}}AFX_VIRTUAL 
	// Implementation
public:
	static BOOL IsNewShell(void);
	void SetBitmapBackground(COLORREF color);
	void SetDisableOldStyle(void);
	void UnSetDisableOldStyle(void);
	BOOL GetDisableOldStyle(void);
	void UnSetBitmapBackground(void);
	int AddBitmapToImageList(CImageList *list,UINT nResourceID);
	BOOL LoadFromToolBar(UINT nID,UINT nToolBar,int& xoffset);
	void InsertSpaces(void);
	static LRESULT FindKeyboardShortcut(UINT nChar,UINT nFlags,CMenu *pMenu);
	static void UpdateMenu(CMenu *pmenu);
	BOOL IsMenu(CMenu *submenu);
	void DrawCheckMark(CDC *pDC,int x,int y,COLORREF color);
	void DrawRadioDot(CDC *pDC,int x,int y,COLORREF color);
	CBMPMenu *FindMenuOption(int nId,int& nLoc);
	CBMPMenuData *FindMenuOption(wchar_t *lpstrText);
	BOOL GetMenuText(UINT id,CString &string);
	CImageList *checkmaps;
	BOOL checkmapsshare;
	int m_selectcheck;
	int m_unselectcheck;
	void LoadCheckmarkBitmap(int unselect,int select);
	void DitherBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HBITMAP hbm, int nXSrc, int nYSrc);
	void DitherBlt2(CDC *drawdc, int nXDest, int nYDest, int nWidth, int nHeight, CBitmap &bmp, int nXSrc, int nYSrc);
	HBITMAP LoadSysColorBitmap(int nResourceId);
	
	virtual ~CBMPMenu();  // Virtual Destructor 
	// Drawing: 
	virtual void DrawItem( LPDRAWITEMSTRUCT);  // Draw an item
	virtual void MeasureItem( LPMEASUREITEMSTRUCT );  // Measure an item
	
	// Customizing:
	
	void SetIconSize (int, int);  // Set icon size
	
	BOOL AppendODMenuA(LPCSTR lpstrText, UINT nFlags = MF_OWNERDRAW, UINT nID = 0, int nIconNormal = -1); // Owner-Drawn Append 
	BOOL AppendODMenuW(wchar_t *lpstrText, UINT nFlags = MF_OWNERDRAW, UINT nID = 0, int nIconNormal = -1); // Owner-Drawn Append 
	
	BOOL ChangeMenuItemShortcut(const char *Shortcut, UINT nID);
	BOOL DeleteMenu(UINT nPosition, UINT nFlags);
	BOOL ModifyODMenuA(const char *lpstrText,UINT nID=0,int nIconNormal=-1);
	BOOL ModifyODMenuA(const char *lpstrText,const char *OptionText,int nIconNormal);
	BOOL ModifyODMenuW(wchar_t *lpstrText,UINT nID=0,int nIconNormal=-1);
	BOOL ModifyODMenuW(wchar_t *lpstrText,wchar_t *OptionText,int nIconNormal);
	CBMPMenuData *NewODMenu(UINT pos,UINT nFlags,UINT nID,CString string);
	void SynchronizeMenu(void);
	void CBMPMenu::InitializeMenuList(int value);
	void CBMPMenu::DeleteMenuList(void);
	CBMPMenuData *CBMPMenu::FindMenuList(UINT nID);
	virtual BOOL LoadMenu(LPCTSTR lpszResourceName);  // Load a menu
	virtual BOOL LoadMenu(int nResource);  // ... 
	void AddFromToolBar(CToolBar* pToolBar, int nResourceID);
	BOOL Draw3DCheckmark(CDC *dc, const CRect& rc,BOOL bSelected, HBITMAP hbmCheck);
	BOOL LoadToolbar(UINT nToolBar);
	BOOL LoadToolbars(const UINT *arID,int n);
	
	// Destoying:
	
	virtual BOOL DestroyMenu();
	
	// Generated message map functions
protected:
	int m_iconX,m_iconY;
	COLORREF m_bitmapBackground;
	BOOL m_bitmapBackgroundFlag;
	BOOL disable_old_style;
}; 

// ============================================================================

class CTitleMenu : public CMenu
{
public:
	CTitleMenu();
	virtual ~CTitleMenu();

// Operations
public:
	void SetMenuTitle(UINT ID, const char* Title);

// Implementation
public:
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMIS);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDIS);

protected:
	CFont m_Font;
	HFONT CreateTitleFont();
	CMap<int, int, const char*, const char*> m_TitleStrings;
};

#endif
