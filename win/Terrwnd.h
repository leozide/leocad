#if !defined(AFX_TERRWND_H__F6E3C623_D3F0_11D2_8204_9F077F19091C__INCLUDED_)
#define AFX_TERRWND_H__F6E3C623_D3F0_11D2_8204_9F077F19091C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TerrWnd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTerrainWnd window

class Terrain;
class Camera;

class CTerrainWnd : public CWnd
{
// Construction
public:
	CTerrainWnd(Terrain* pTerrain);

// Attributes
public:
	int m_nAction;
	void LoadTexture(bool linear);

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTerrainWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	void ResetCamera();
	virtual ~CTerrainWnd();

	// Generated message map functions
protected:
	SIZE m_szView;
	HGLRC m_hglRC;
	CClientDC* m_pDC;
	CPalette* m_pPalette;
	Terrain* m_pTerrain;
	Camera* m_pCamera;

	CPoint m_ptMouse;

	//{{AFX_MSG(CTerrainWnd)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

	enum TERRAIN_ACTIONS { TERRAIN_ZOOM, TERRAIN_PAN, TERRAIN_ROTATE };
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TERRWND_H__F6E3C623_D3F0_11D2_8204_9F077F19091C__INCLUDED_)
