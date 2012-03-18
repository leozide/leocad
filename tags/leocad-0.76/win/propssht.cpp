// PropsSht.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PropsSht.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropertiesSheet

IMPLEMENT_DYNAMIC(CPropertiesSheet, CPropertySheet)

CPropertiesSheet::CPropertiesSheet(CWnd* pWndParent)
	 : CPropertySheet("", pWndParent)
{
	AddPage(&m_PageGeneral);
	AddPage(&m_PageSummary);
	AddPage(&m_PagePieces);
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	SetActivePage(1);
}

CPropertiesSheet::~CPropertiesSheet()
{
}


BEGIN_MESSAGE_MAP(CPropertiesSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CPropertiesSheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPropertiesSheet message handlers


