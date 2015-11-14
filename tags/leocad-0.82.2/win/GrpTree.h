#if !defined(AFX_GRPTREE_H__BA679E06_52FE_11D3_A5D0_814EF6A81B31__INCLUDED_)
#define AFX_GRPTREE_H__BA679E06_52FE_11D3_A5D0_814EF6A81B31__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// GrpTree.h : header file
//

#include "typedefs.h"

/////////////////////////////////////////////////////////////////////////////
// CGroupEditTree window

class CGroupEditTree : public CTreeCtrl
{
// Construction
public:
	CGroupEditTree();

// Attributes
protected:
	CImageList* m_pDragImage;
	BOOL m_bDragging;
	HTREEITEM m_hitemDrag, m_hitemDrop;
	HCURSOR m_dropCursor, m_noDropCursor;

// Operations
public:
	LC_GROUPEDITDLG_OPTS* opts;
	void AddChildren(HTREEITEM hParent, Group* pGroup);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGroupEditTree)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGroupEditTree();

	// Generated message map functions
protected:
	HTREEITEM GetDropTarget(HTREEITEM hItem);
	BOOL IsDropSource(HTREEITEM hItem);
	//{{AFX_MSG(CGroupEditTree)
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRPTREE_H__BA679E06_52FE_11D3_A5D0_814EF6A81B31__INCLUDED_)
