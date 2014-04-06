#ifndef _TERRDLG_H_
#define _TERRDLG_H_

#include "TerrCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CTerrainDlg dialog

class Terrain;
class lcTerrainView;

class CTerrainDlg : public CDialog
{
// Construction
public:
	CTerrainDlg(Terrain* pTerrain, CWnd* pParent = NULL);   // standard constructor
	virtual ~CTerrainDlg();

// Dialog Data
	//{{AFX_DATA(CTerrainDlg)
	enum { IDD = IDD_TERRAIN };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTerrainDlg)
	public:
	virtual BOOL DestroyWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void CheckControl(UINT nID, BOOL bEnable);
	CToolBar m_wndToolBar;
	CTerrainCtrl m_Grid;
	CWnd* mTerrainWnd;

	lcTerrainView* mView;
	Terrain* mTerrain;

	// Generated message map functions
	//{{AFX_MSG(CTerrainDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG

	afx_msg LRESULT OnGridChange(WPARAM wParam, LPARAM lParam);
	
	DECLARE_MESSAGE_MAP()
};

#endif // _TERRDLG_H_
