#if !defined(AFX_SORTHEAD_H__56F42684_B169_11D3_A5D1_AA1A6BED9842__INCLUDED_)
#define AFX_SORTHEAD_H__56F42684_B169_11D3_A5D1_AA1A6BED9842__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SortHead.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSortHeaderCtrl window

class CSortHeaderCtrl : public CHeaderCtrl
{
// Construction
public:
	CSortHeaderCtrl();

// Attributes
protected:
	int m_nSortCol;
	BOOL m_bSortAsc;

// Operations
public:
	int SetSortImage(int nCol, BOOL bAsc);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSortHeaderCtrl)
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSortHeaderCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CSortHeaderCtrl)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SORTHEAD_H__56F42684_B169_11D3_A5D1_AA1A6BED9842__INCLUDED_)
