// CADView.h : interface of the CCADView class
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _CADVIEW_H_
#define _CADVIEW_H_

class View;
class CCADDoc;

class CCADView : public CView
{
protected: // create from serialization only
	CCADView();
	DECLARE_DYNCREATE(CCADView)

// Attributes
public:
	CCADDoc* GetDocument();
	void PrintHeader(BOOL bFooter, HDC hDC, CRect rc, UINT nCurPage, UINT nMaxPage, BOOL bCatalog);

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCADView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCADView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	HCURSOR m_hCursor;
  View* m_pView;

	void* m_pPixels;
	//{{AFX_MSG(CCADView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnFilePrintPreview();
	//}}AFX_MSG
	afx_msg void OnDropDown(NMHDR* pNotifyStruct, LRESULT* pResult);
	afx_msg LONG OnSetStep(UINT lParam, LONG wParam);
	afx_msg LONG OnAutoPan(UINT lParam, LONG wParam);
	afx_msg LONG OnChangeCursor(UINT lParam, LONG wParam);

	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in CADView.cpp
inline CCADDoc* CCADView::GetDocument()
	{ return (CCADDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // _CADVIEW_H_
