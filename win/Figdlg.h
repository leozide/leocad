#if !defined(AFX_FIGDLG_H__80DF3A21_90D2_11D1_A740_444553540000__INCLUDED_)
#define AFX_FIGDLG_H__80DF3A21_90D2_11D1_A740_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// FigDlg.h : header file
//

#include "ClrPick.h"

/////////////////////////////////////////////////////////////////////////////
// CMinifigDlg dialog

class CMinifigWnd;

class CMinifigDlg : public CDialog
{
// Construction
public:
	CMinifigDlg(void* param, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMinifigDlg)
	enum { IDD = IDD_MINIFIG };
	CColorPicker	m_clrHandLeft;
	CColorPicker	m_clrTorso;
	CColorPicker	m_clrToolLeft;
	CColorPicker	m_clrShoeLeft;
	CColorPicker	m_clrLegLeft;
	CColorPicker	m_clrHead;
	CColorPicker	m_clrArmLeft;
	CColorPicker	m_clrToolRight;
	CColorPicker	m_clrShoeRight;
	CColorPicker	m_clrLegRight;
	CColorPicker	m_clrHips;
	CColorPicker	m_clrHandRight;
	CColorPicker	m_clrArmRight;
	CColorPicker	m_clrNeck;
	CColorPicker	m_clrHat;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMinifigDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CMinifigWnd* m_pMFWnd;
	void* m_pParam;

	afx_msg LONG OnColorSelEndOK(UINT lParam, LONG wParam);
	void OnPieceSelEndOK(UINT nID);
	void OnChangeAngle(UINT nID);

	// Generated message map functions
	//{{AFX_MSG(CMinifigDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FIGDLG_H__80DF3A21_90D2_11D1_A740_444553540000__INCLUDED_)
