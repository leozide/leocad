// PrevView.h: interface for the CCADPreviewView class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PREVVIEW_H__FB79E301_F771_11D2_8204_C75E663DC515__INCLUDED_)
#define AFX_PREVVIEW_H__FB79E301_F771_11D2_8204_C75E663DC515__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <afxpriv.h>
#include "FlatBar.h"

class CCADView;

class CCADPreviewView : public CScrollView
{
	DECLARE_DYNCREATE(CCADPreviewView)

// Constructors
public:
	CCADPreviewView();
	BOOL SetPrintView(CCADView* pPrintView);

// Attributes
protected:
	CCADView* m_pOrigView;
	CCADView* m_pPrintView;
	CPreviewDC* m_pPreviewDC;  // Output and attrib DCs Set, not created
	CDC m_dcPrint;             // Actual printer DC

// Operations
	void SetZoomState(UINT nNewState, UINT nPage, CPoint point);
	void SetCurrentPage(UINT nPage, BOOL bClearRatios);

	// Returns TRUE if in a page rect. Returns the page index
	// in nPage and the point converted to 1:1 screen device coordinates
	BOOL FindPageRect(CPoint& point, UINT& nPage);


// Overridables
	virtual void OnActivateView(BOOL bActivate,
			CView* pActivateView, CView* pDeactiveView);

	// Returns .cx/.cy as the numerator/denominator pair for the ratio
	// using CSize for convenience
	virtual CSize CalcScaleRatio(CSize windowSize, CSize actualSize);

	virtual void PositionPage(UINT nPage);
	virtual void OnDisplayPageNumber(UINT nPage, UINT nPagesDisplayed);

// Implementation
public:
	virtual ~CCADPreviewView();
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
#ifdef _DEBUG
	void AssertValid() const;
	void Dump(CDumpContext& dc) const;
#endif
#ifdef _MAC
	virtual void CalcWindowRect(LPRECT lpClientRect);
#endif

protected:
	//{{AFX_MSG(CCADPreviewView)
	afx_msg void OnPreviewClose();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDraw(CDC* pDC);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnNextPage();
	afx_msg void OnPrevPage();
	afx_msg void OnPreviewPrint();
	afx_msg void OnZoomIn();
	afx_msg void OnZoomOut();
	afx_msg void OnUpdateNextPage(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePrevPage(CCmdUI* pCmdUI);
	afx_msg void OnUpdateZoomIn(CCmdUI* pCmdUI);
	afx_msg void OnUpdateZoomOut(CCmdUI* pCmdUI);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnPreviewOnepage();
	afx_msg void OnUpdatePreviewOnepage(CCmdUI* pCmdUI);
	afx_msg void OnPreviewTwopages();
	afx_msg void OnUpdatePreviewTwopages(CCmdUI* pCmdUI);
	//}}AFX_MSG

	void DoZoom(UINT nPage, CPoint point);
	void SetScaledSize(UINT nPage);
	CSize CalcPageDisplaySize();

	CPrintPreviewState* m_pPreviewState; // State to restore
	CFlatToolBar* m_pToolBar; // Toolbar for preview

	struct PAGE_INFO
	{
		CRect rectScreen; // screen rect (screen device units)
		CSize sizeUnscaled; // unscaled screen rect (screen device units)
		CSize sizeScaleRatio; // scale ratio (cx/cy)
		CSize sizeZoomOutRatio; // scale ratio when zoomed out (cx/cy)
	};

	PAGE_INFO* m_pPageInfo; // Array of page info structures
	PAGE_INFO m_pageInfoArray[2]; // Embedded array for the default implementation

	BOOL m_bPageNumDisplayed;// Flags whether or not page number has yet
								// been displayed on status line
	UINT m_nZoomOutPages; // number of pages when zoomed out
	UINT m_nZoomState;
	UINT m_nMaxPages; // for sanity checks
	UINT m_nCurrentPage;
	UINT m_nPages;
	int m_nSecondPageOffset; // used to shift second page position

	HCURSOR m_hMagnifyCursor;

	CSize m_sizePrinterPPI; // printer pixels per inch
	CPoint m_ptCenterPoint;
	CPrintInfo* m_pPreviewInfo;

	DECLARE_MESSAGE_MAP()

	friend class CView;
	friend class CCADView;
	friend BOOL CALLBACK _AfxPreviewCloseProcEx(CFrameWnd* pFrameWnd);
};

// Zoom States
#define ZOOM_OUT    0
#define ZOOM_MIDDLE 1
#define ZOOM_IN     2

#endif // !defined(AFX_PREVVIEW_H__FB79E301_F771_11D2_8204_C75E663DC515__INCLUDED_)
