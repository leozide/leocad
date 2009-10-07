#if !defined(AFX_TEROPDLG_H__16D85803_DBE0_11D2_8204_C46524CA8617__INCLUDED_)
#define AFX_TEROPDLG_H__16D85803_DBE0_11D2_8204_C46524CA8617__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TerOpDlg.h : header file
//

class Terrain;

/////////////////////////////////////////////////////////////////////////////
// CTerrainOptionsDlg dialog

class CTerrainOptionsDlg : public CDialog
{
// Construction
public:
	void GetOptions(Terrain* pTerrain);
	void SetOptions(Terrain* pTerrain);
	CTerrainOptionsDlg(CWnd* pParent = NULL);   // standard constructor
	COLORREF m_crTerrain;

// Dialog Data
	//{{AFX_DATA(CTerrainOptionsDlg)
	enum { IDD = IDD_TERRAIN_OPTIONS };
	CButton	m_btnColor;
	int		m_nXPatches;
	int		m_nYPatches;
	float	m_fXSize;
	float	m_fYSize;
	BOOL	m_bFlat;
	BOOL	m_bTexture;
	CString	m_strTexture;
	BOOL	m_bSmooth;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTerrainOptionsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CTerrainOptionsDlg)
	afx_msg void OnTeroptColor();
	afx_msg void OnTeroptTexturebrowse();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEROPDLG_H__16D85803_DBE0_11D2_8204_C46524CA8617__INCLUDED_)
