// ColorLst.cpp : implementation file
//

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
}

CColorsList::~CColorsList()
{
}


BEGIN_MESSAGE_MAP(CColorsList, CListBox)
	//{{AFX_MSG_MAP(CColorsList)
	ON_WM_CREATE()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
	ON_WM_SIZE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorsList message handlers

void CColorsList::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	int x = lpDIS->itemID;
	if (x%2 == 0)
		x /= 2;
	else
		x = ((x-1)/2)+14;

	if ((!(lpDIS->itemState & ODS_SELECTED) && (lpDIS->itemAction & ODA_SELECT)) ||
	    (lpDIS->itemAction & ODA_DRAWENTIRE))
	{
		SetBkColor(lpDIS->hDC, RGB(FlatColorArray[x][0], FlatColorArray[x][1], FlatColorArray[x][2]));
		ExtTextOut(lpDIS->hDC, 0, 0, ETO_OPAQUE, &lpDIS->rcItem, NULL, 0, NULL);

		if (x > 13 && x < 22)
		for (x = lpDIS->rcItem.left; x < lpDIS->rcItem.right; x++)
		{
			int y;

			for (y = lpDIS->rcItem.top; y < lpDIS->rcItem.bottom; y+=4)
			{
				if (y == lpDIS->rcItem.top) y += x%4;
				SetPixelV (lpDIS->hDC, x,y,RGB(255,255,255));
			}
			for (y = lpDIS->rcItem.bottom; y > lpDIS->rcItem.top; y-=4)
			{
				if (y == lpDIS->rcItem.bottom) y-= x%4;
				SetPixelV (lpDIS->hDC, x,y,RGB(255,255,255));
			}
		}
	}

	// Item has been selected - hilite frame
	if ((lpDIS->itemState & ODS_SELECTED) &&
		(lpDIS->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)))
	{
		HBRUSH hbr = CreateSolidBrush(RGB(255-FlatColorArray[x][0], 255-FlatColorArray[x][1], 255-FlatColorArray[x][2]));
		FrameRect(lpDIS->hDC, &lpDIS->rcItem, hbr);
		DeleteObject(hbr);
	}
}

void CColorsList::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	RECT rc;
	GetClientRect(&rc);

	lpMeasureItemStruct->itemHeight = 12;
	lpMeasureItemStruct->itemWidth = (rc.right - rc.left) / 14;
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

	int ColumnWidth = lpCreateStruct->cx / 14;

	for (int i = 0; i < 14; i++)
	{
		CRect rect(i*ColumnWidth, 0, (i+1)*ColumnWidth, 12);
		m_ToolTip.AddTool(this, IDS_COLOR01+i, rect, i+1);
		rect.OffsetRect(0, 12);
		m_ToolTip.AddTool(this, IDS_COLOR15+i, rect, i+15);
	}

	return 0;
}

void CColorsList::OnSize(UINT nType, int cx, int cy)
{
	CListBox::OnSize(nType, cx, cy);

	int ColumnWidth = cx / 14;

	for (int i = 0; i < 14; i++)
	{
		CRect rect(i*ColumnWidth, 0, (i+1)*ColumnWidth, 12);
		m_ToolTip.SetToolRect(this, i+1, rect);
		rect.OffsetRect(0, 12);
		m_ToolTip.SetToolRect(this, i+15, rect);
	}
}

void CColorsList::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_INSERT)
	{
		CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
		pFrame->PostMessage(WM_COMMAND, ID_PIECE_INSERT, 0);

		CView* pView = pFrame->GetActiveView();
		pView->SetFocus();
	}
	else
		CListBox::OnKeyDown(nChar, nRepCnt, nFlags);
}
