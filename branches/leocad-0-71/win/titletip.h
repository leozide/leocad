#if !defined(AFX_TITLETIP_H__FB05F243_E98F_11D0_82A3_20933B000000__INCLUDED_)
#define AFX_TITLETIP_H__FB05F243_E98F_11D0_82A3_20933B000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TitleTip.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTitleTip window

#define TITLETIP_CLASSNAME _T("LeoCADTitleTip")


class CTitleTip : public CWnd
{
// Construction
public:
        CTitleTip();

// Attributes
public:

// Operations
public:

// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CTitleTip)
        public:
        virtual BOOL PreTranslateMessage(MSG* pMsg);
        //}}AFX_VIRTUAL

// Implementation
public:
        void Show(CRect rectTitle, LPCTSTR lpszTitleText, int xoffset, BOOL bFocus);
        virtual BOOL Create(CWnd *pParentWnd);
        virtual ~CTitleTip();

protected:
        CWnd *m_pParentWnd;
        CRect m_rectTitle;


        // Generated message map functions
protected:
        //{{AFX_MSG(CTitleTip)
        afx_msg void OnMouseMove(UINT nFlags, CPoint point);
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations 
// immediately before the previous line.

#endif // !defined(AFX_TITLETIP_H__FB05F243_E98F_11D0_82A3_20933B000000__INCLUDED_)
