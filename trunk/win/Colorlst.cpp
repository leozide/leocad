#include "lc_global.h"
#include "leocad.h"
#include "ColorLst.h"
#include "globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CColorsList

CColorsList::CColorsList()
{
	m_bLowRes = FALSE;
}

CColorsList::~CColorsList()
{
}


BEGIN_MESSAGE_MAP(CColorsList, CListBox)
	//{{AFX_MSG_MAP(CColorsList)
	ON_WM_CREATE()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorsList message handlers

void CColorsList::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	int x = lpDIS->itemID;
	if (x%2 == 0)
		x/=2;
	else
		x = ((x-1)/2)+14;

	if ((!(lpDIS->itemState & ODS_SELECTED) &&
		(lpDIS->itemAction & ODA_SELECT)) ||
		(lpDIS->itemAction & ODA_DRAWENTIRE))
	{
		if (m_bLowRes)
		{
			HBRUSH hbr = CreateSolidBrush(RGB(FlatColorArray[x][0], FlatColorArray[x][1], FlatColorArray[x][2]));
			FillRect(lpDIS->hDC, &lpDIS->rcItem, hbr);
			DeleteObject (hbr);
		}
		else
		{
			SetBkColor(lpDIS->hDC, RGB(FlatColorArray[x][0], FlatColorArray[x][1], FlatColorArray[x][2]));
			ExtTextOut(lpDIS->hDC, 0, 0, ETO_OPAQUE, &lpDIS->rcItem, NULL, 0, NULL);
		}

		if (x > 13 && x < 22)
		for (x = lpDIS->rcItem.left; x < lpDIS->rcItem.right; x++)
		{
			for (int y = lpDIS->rcItem.top; y < lpDIS->rcItem.bottom; y+=4)
			{
				if (y == lpDIS->rcItem.top) y += x%4;
				SetPixelV (lpDIS->hDC, x,y,RGB(255,255,255));
			}
			for (int y = lpDIS->rcItem.bottom; y > lpDIS->rcItem.top; y-=4)
			{
				if (y == lpDIS->rcItem.bottom) y-= x%4;
				SetPixelV (lpDIS->hDC, x,y,RGB(255,255,255));
			}
		}
	}
	
	// item has been selected - hilite frame
	if ((lpDIS->itemState & ODS_SELECTED) &&
		(lpDIS->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)))
	{
		HBRUSH hbr = CreateSolidBrush(RGB(255-FlatColorArray[x][0], 255-FlatColorArray[x][1], 255-FlatColorArray[x][2]));
		FrameRect(lpDIS->hDC, &lpDIS->rcItem, hbr);
		DeleteObject (hbr);
	}
}

void CColorsList::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	lpMeasureItemStruct->itemHeight = 12;
	lpMeasureItemStruct->itemWidth = 15;
}

BOOL CColorsList::PreTranslateMessage(MSG* pMsg) 
{
	if (m_ToolTip.m_hWnd)
		m_ToolTip.RelayEvent(pMsg);

	return CListBox::PreTranslateMessage(pMsg);
}

int CColorsList::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CListBox::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_ToolTip.Create(this);
//	m_ToolTip.Activate(TRUE);

	for (int i = 0; i < 14; i++)
	{
		CRect rect (i*15,0,(i+1)*15,12);
		m_ToolTip.AddTool(this, IDS_COLOR01+i, rect, 1);
		rect.OffsetRect (0,12);
		m_ToolTip.AddTool(this, IDS_COLOR15+i, rect, 1);
	}
	
	return 0;
}

void CColorsList::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar == VK_INSERT)
	{
//		project->HandleCommand(LC_PIECE_INSERT, 0);

		CFrameWndEx* pFrame = (CFrameWndEx*)AfxGetMainWnd();
		pFrame->PostMessage(WM_COMMAND, ID_PIECE_INSERT, 0);

		CView* pView = pFrame->GetActiveView();
		pView->SetFocus();
	}
	else
		CListBox::OnKeyDown(nChar, nRepCnt, nFlags);
}
