// ColorLst.cpp : implementation file
//

#include "lc_global.h"
#include "leocad.h"
#include "ColorLst.h"
#include "lc_colors.h"

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
	int Color = lpDIS->itemID;

	if ((!(lpDIS->itemState & ODS_SELECTED) && (lpDIS->itemAction & ODA_SELECT)) || (lpDIS->itemAction & ODA_DRAWENTIRE))
	{
		SetBkColor(lpDIS->hDC, LC_COLOR_RGB(Color));
		ExtTextOut(lpDIS->hDC, 0, 0, ETO_OPAQUE, &lpDIS->rcItem, NULL, 0, NULL);

		if (LC_COLOR_TRANSLUCENT(Color))
		{
			for (int x = lpDIS->rcItem.left; x < lpDIS->rcItem.right; x++)
			{
				int y;

				for (y = lpDIS->rcItem.top + x % 4; y < lpDIS->rcItem.bottom; y += 4)
					SetPixelV(lpDIS->hDC, x,y, RGB(255,255,255));

				for (y = lpDIS->rcItem.bottom - x % 4; y > lpDIS->rcItem.top; y -= 4)
					SetPixelV(lpDIS->hDC, x,y, RGB(255,255,255));
			}
		}
	}

	// Item is selected, draw a border around it.
	if ((lpDIS->itemState & ODS_SELECTED) && (lpDIS->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)))
	{
		COLORREF cr = LC_COLOR_RGB(Color);
		HBRUSH hbr = CreateSolidBrush(RGB(255-GetRValue(cr), 255-GetGValue(cr), 255-GetBValue(cr)));
		FrameRect(lpDIS->hDC, &lpDIS->rcItem, hbr);
		DeleteObject(hbr);
	}
}

void CColorsList::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	RECT rc;
	GetClientRect(&rc);

	lpMeasureItemStruct->itemHeight = 12;
	lpMeasureItemStruct->itemWidth = (rc.right - rc.left) / LC_COLORLIST_NUM_COLS;
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

	int ColumnWidth = lpCreateStruct->cx / LC_COLORLIST_NUM_COLS;

	for (int i = 0; i < LC_COLORLIST_NUM_COLS; i++)
	{
		CRect rect(i*ColumnWidth, 0, (i+1)*ColumnWidth, 12);

		for (int j = 0; j < LC_COLORLIST_NUM_ROWS; j++)
		{
			int Index = i*LC_COLORLIST_NUM_ROWS+j;
			m_ToolTip.AddTool(this, lcColorList[Index].Name, rect, Index+1);
			rect.OffsetRect(0, 12);
		}
	}

	return 0;
}

void CColorsList::OnSize(UINT nType, int cx, int cy)
{
	CListBox::OnSize(nType, cx, cy);

	int ColumnWidth = cx / LC_COLORLIST_NUM_COLS;

	for (int i = 0; i < LC_COLORLIST_NUM_COLS; i++)
	{
		CRect rect(i*ColumnWidth, 0, (i+1)*ColumnWidth, 12);

		for (int j = 0; j < LC_COLORLIST_NUM_ROWS; j++)
		{
			int Index = i*LC_COLORLIST_NUM_ROWS+j;
			m_ToolTip.SetToolRect(this, Index+1, rect);
			rect.OffsetRect(0, 12);
		}
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
