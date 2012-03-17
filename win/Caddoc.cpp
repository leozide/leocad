// CADDoc.cpp : implementation of the CCADDoc class
//

#include "stdafx.h"
#include "LeoCAD.h"
#include "CADDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCADDoc

IMPLEMENT_DYNCREATE(CCADDoc, CDocument)

BEGIN_MESSAGE_MAP(CCADDoc, CDocument)
	//{{AFX_MSG_MAP(CCADDoc)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCADDoc construction/destruction

CCADDoc::CCADDoc()
{
}

CCADDoc::~CCADDoc()
{
}

/////////////////////////////////////////////////////////////////////////////
// CCADDoc commands
