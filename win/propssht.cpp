// PropsSht.cpp : implementation file
//

#include "lc_global.h"
#include "resource.h"
#include "PropsSht.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropertiesSheet

IMPLEMENT_DYNAMIC(CPropertiesSheet, CPropertySheet)

CPropertiesSheet::CPropertiesSheet(bool ShowPieces, CWnd* pWndParent)
	 : CPropertySheet("", pWndParent)
{
	AddPage(&m_PageSummary);
	if (ShowPieces)
		AddPage(&m_PagePieces);
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
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


