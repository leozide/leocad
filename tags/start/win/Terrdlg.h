#if !defined(AFX_TERRDLG_H__51E24381_D9FA_11D2_8204_FB90F0F77F11__INCLUDED_)
#define AFX_TERRDLG_H__51E24381_D9FA_11D2_8204_FB90F0F77F11__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TerrDlg.h : header file
//

#include "TerrCtrl.h"
class Terrain;

/////////////////////////////////////////////////////////////////////////////
// CTerrainDlg dialog

class CTerrainWnd;

class CTerrainDlg : public CDialog
{
// Construction
public:
	CTerrainDlg(Terrain* pTerrain, bool bLinear, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CTerrainDlg)
	enum { IDD = IDD_TERRAIN };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTerrainDlg)
	public:
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
	CTerrainWnd* m_pTerrainWnd;

	bool m_bLinear;
	Terrain* m_pTerrain;

	// Generated message map functions
	//{{AFX_MSG(CTerrainDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

	afx_msg LRESULT OnGridChange(WPARAM wParam, LPARAM lParam);
	
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TERRDLG_H__51E24381_D9FA_11D2_8204_FB90F0F77F11__INCLUDED_)
