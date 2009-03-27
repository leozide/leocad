#if !defined(AFX_COLOURPICKER_H__D0B75901_9830_11D1_9C0F_00A0243D1382__INCLUDED_)
#define AFX_COLOURPICKER_H__D0B75901_9830_11D1_9C0F_00A0243D1382__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// ColorPicker.h : header file
//

#include "ClrPopup.h"

/////////////////////////////////////////////////////////////////////////////
// CColorPicker window

//void AFXAPI DDX_ColorPicker(CDataExchange *pDX, int nIDC, COLORREF& crColor);

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
	BOOL m_bDefaultText;
	BOOL m_bCustomText;
	
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
    COLORREF m_crColor;

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

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COLOURPICKER_H__D0B75901_9830_11D1_9C0F_00A0243D1382__INCLUDED_)
