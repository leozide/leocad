#ifndef _TIMECTRL_H_
#define _TIMECTRL_H_

class lcPiece;

struct CTimelineNode
{
	lcPiece* Piece;
	int y;
};

/////////////////////////////////////////////////////////////////////////////
// CTimelineCtrl window

class CTimelineCtrl : public CWnd
{
	enum LC_TIMELINE_TRACK_MODE
	{
		LC_TIMELINE_TRACK_NONE,
		LC_TIMELINE_TRACK_SHOW,
		LC_TIMELINE_TRACK_HIDE,
		LC_TIMELINE_TRACK_TIME
	};

// Construction
public:
	CTimelineCtrl();

	BOOL Create(const RECT& rect, CWnd* pParentWnd, UINT nID, DWORD dwStyle = WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VISIBLE);

// Attributes
protected:
	CFont m_Font;

	int m_TotalHeight;
	int m_HeaderHeight;
	int m_LineHeight;

	int m_TreeWidth;
	int m_StepWidth;

	u32 m_FilterStart;
	u32 m_FilterEnd;

	int m_TrackNode;
	u32 m_TrackTime;
	LC_TIMELINE_TRACK_MODE m_TrackMode;

	CArray<CTimelineNode, CTimelineNode> m_Nodes;

// Operations
public:
	void Repopulate();

protected:
	void DrawNode(CDC* pDC, int NodeIndex, int ScrollPos, const CRect& ClientRect);

	void ResetScrollBar();

	int FindNodeByPoint(const CPoint& point);
	CRect CalcTimeRect(CTimelineNode* Node, u32 Time);

	// Return the time at x in client coordinates.
	u32 GetTimeFromX(int x)
	{
		if (x < m_TreeWidth + m_StepWidth / 2)
			return 1;

		return (x - m_TreeWidth + m_StepWidth / 2) / m_StepWidth;
	}

	// Return the x position of a given time in client coordinates.
	int GetXFromTime(u32 Time)
	{
		return m_TreeWidth + Time * m_StepWidth;
	}

	void OnDraw(CDC* pDC);
	void EraseBkgnd(CDC* pDC);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTimelineCtrl)
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTimelineCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CTimelineCtrl)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // _TIMECTRL_H_
