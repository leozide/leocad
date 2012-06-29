#ifndef _CLRPICK_H_
#define _CLRPICK_H_

#include "ClrPopup.h"

class CColorPicker : public CButton
{
// Construction
public:
    CColorPicker();
    DECLARE_DYNCREATE(CColorPicker);

// Attributes
public:
	int GetColorIndex();
	void SetColorIndex(int nColor); 
	
// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CColorPicker)
	public:
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	protected:
    virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CColorPicker();

protected:
    void SetWindowSize();

// protected attributes
protected:
    BOOL m_bActive; // Is the dropdown active?
    CRect m_ArrowRect;
	int m_nColor;

    // Generated message map functions
protected:
    //{{AFX_MSG(CColorPicker)
    afx_msg BOOL OnClicked();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    //}}AFX_MSG
    afx_msg LONG OnSelEndOK(UINT lParam, LONG wParam);
    afx_msg LONG OnSelEndCancel(UINT lParam, LONG wParam);

    DECLARE_MESSAGE_MAP()
};

#endif // _CLRPICK_H_
