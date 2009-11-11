#ifndef _TIMEBAR_H_
#define _TIMEBAR_H_

#include "sizecbar.h"
#include "scbarg.h"
#include "timectrl.h"
#include "lc_message.h"

class CTimeBar : public CSizingControlBarG, public lcListener
{
public:
	CTimeBar();

// Overrides
public:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTimeBar)
	public:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTimeBar();

public:
	CTimelineCtrl m_TimeCtrl;

	virtual void ProcessMessage(lcMessageType Message, void* Data);

// Generated message map functions
protected:
	//{{AFX_MSG(CTimeBar)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif // _TIMEBAR_H_
