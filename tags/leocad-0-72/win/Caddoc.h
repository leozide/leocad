// CADDoc.h : interface of the CCADDoc class
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _CADDOC_H_
#define _CADDOC_H_

class CCADDoc : public CDocument
{
protected: // create from serialization only
	CCADDoc();
	virtual ~CCADDoc();

	DECLARE_DYNCREATE(CCADDoc)

public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCADDoc)
	//}}AFX_VIRTUAL

// Generated message map functions
protected:
	//{{AFX_MSG(CCADDoc)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // _CADDOC_H_
