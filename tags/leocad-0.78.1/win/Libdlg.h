#ifndef _LIBDLG_H_
#define _LIBDLG_H_

#include "array.h"
#include "str.h"

class PieceInfo;

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

	bool ImportPieces(const ObjArray<String>& FileList);

	CToolBar m_wndToolBar;
	CImageList m_TreeImages;
	int m_SortColumn;

protected:
	// Generated message map functions
	//{{AFX_MSG(CLibraryDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelChangedTree(NMHDR* pNMHDR, LRESULT* pResult);
	virtual void OnCancel();
	virtual void OnOK();
	afx_msg void OnListColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	afx_msg BOOL OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // _LIBDLG_H_
