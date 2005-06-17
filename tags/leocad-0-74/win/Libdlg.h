#if !defined(AFX_LIBDLG_H__BABBE241_AF9C_11D2_8203_DC3ED7F79C17__INCLUDED_)
#define AFX_LIBDLG_H__BABBE241_AF9C_11D2_8203_DC3ED7F79C17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// LibDlg.h : header file
//

#include "libman.h"

class PieceInfo;

typedef struct {
	PieceInfo* info;
	unsigned long group;
	unsigned long defgroup;
} PARTGROUPINFO;

#include <afxtempl.h>  // CArray;

/////////////////////////////////////////////////////////////////////////////
// CLibraryDlg dialog

class CLibraryDlg : public CDialog
{
// Construction
public:
	CLibraryDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLibraryDlg();

// Dialog Data
	//{{AFX_DATA(CLibraryDlg)
	enum { IDD = IDD_LIBRARY };
	CTreeCtrl	m_Tree;
	CListCtrl	m_List;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLibraryDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	void EnableControl(UINT nID, BOOL bEnable);
	BOOL ContinueModal();
	void UpdateTree();
	void UpdateList();

//	BYTE m_nMaxGroups;
	int m_nBitmaps[32];
	CImageList m_ImageList;

	CToolBar m_wndToolBar;
	CImageList m_TreeImages;

	CImageList* m_pDragImage;
	BOOL		m_bDragging;
	int			m_nDragIndex;
	HTREEITEM	m_hDropItem;

protected:
	LibraryManager m_Manager;

protected:
	// Generated message map functions
	//{{AFX_MSG(CLibraryDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelChangedTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBeginDragList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	virtual void OnCancel();
	virtual void OnOK();
	//}}AFX_MSG

	afx_msg BOOL OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LIBDLG_H__BABBE241_AF9C_11D2_8203_DC3ED7F79C17__INCLUDED_)
