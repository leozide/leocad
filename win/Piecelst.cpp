// PieceLst.cpp : implementation file
//

#include "stdafx.h"
#include "leocad.h"
#include "PieceLst.h"
#include "PieceBar.h"
#include "project.h"
#include "pieceinf.h"
#include "globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPiecesList

static int CALLBACK ListViewCompareProc(LPARAM lP1, LPARAM lP2, LPARAM lParamSort)
{
	int ret;

	if ((lP1 < 0) || (lP2 < 0))
		return 0;

	if ((lParamSort & ~0xF0) == 0)
		ret = strcmpi(((PieceInfo*)lP1)->m_strDescription, ((PieceInfo*)lP2)->m_strDescription);
	else
		ret = strcmpi(((PieceInfo*)lP1)->m_strName, ((PieceInfo*)lP2)->m_strName);

	if (lParamSort & 0xF0)
		return ret;
	else
		return -ret;
}

CPiecesList::CPiecesList()
{
	// TODO: Load from registry
	memset(m_nLastPieces, 0, sizeof(m_nLastPieces));

	m_nSortedCol = 0;
	m_bAscending = FALSE;
}

CPiecesList::~CPiecesList()
{
	// TODO: save m_nLastPieces to registry
}


BEGIN_MESSAGE_MAP(CPiecesList, CListCtrl)
	//{{AFX_MSG_MAP(CPiecesList)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnclick)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnItemchanged)
	ON_WM_MOUSEMOVE()
	ON_WM_CREATE()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPiecesList message handlers

void CPiecesList::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	// set the sort order.
	if (pNMListView->iSubItem == m_nSortedCol)
		m_bAscending = !m_bAscending;
	else
		m_bAscending = FALSE;

	// save the column index.
	m_nSortedCol = pNMListView->iSubItem;

	if (m_bAscending)
		SortItems((PFNLVCOMPARE)ListViewCompareProc, m_nSortedCol);
	else
		SortItems((PFNLVCOMPARE)ListViewCompareProc, m_nSortedCol|0xF0);

	m_HeaderCtrl.SetSortImage(m_nSortedCol, m_bAscending);
	*pResult = 0;
}

void CPiecesList::OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if (pNMListView->uNewState & LVIS_SELECTED)
	{
		CPiecesBar* pBar = (CPiecesBar*)GetParent();

		LV_ITEM lvi;
		lvi.iItem = pNMListView->iItem;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_PARAM;
		GetItem (&lvi);

		// Let everybody know the selection changed.
		PieceInfo* pInfo = (PieceInfo*)lvi.lParam;
		project->SetCurrentPiece(pInfo);
		pBar->m_wndPiecePreview.SetPieceInfo(pInfo);
		pBar->m_wndPiecePreview.PostMessage(WM_PAINT);

		if (pBar->m_bGroups)
			m_nLastPieces[pBar->m_nCurGroup] = pNMListView->iItem;
	}

	*pResult = 0;
}

void CPiecesList::UpdateList()
{
	CWaitCursor wc;
	CPiecesBar* pBar = (CPiecesBar*)GetParent();

	SetRedraw (FALSE);
	DeleteAllItems();

	LV_ITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_PARAM;

	for (int i = 0; i < project->GetPieceLibraryCount(); i++)
	{
		PieceInfo* pInfo = project->GetPieceInfo(i);

		if ((pInfo->m_strDescription[0] == '~') && !pBar->m_bSubParts)
			continue;

		if ((!pBar->m_bGroups) ||
			((pInfo->m_nGroups & (DWORD)(1 << pBar->m_nCurGroup)) != 0))
		{
			TCHAR tmp[65], tmp2[10];
			strcpy (tmp, pInfo->m_strDescription);
			lvi.iItem = 0;
			lvi.iSubItem = 0;
			lvi.pszText = tmp;
			lvi.lParam = (LPARAM)pInfo;
			int idx = InsertItem(&lvi);

			strcpy (tmp2, pInfo->m_strName);
			SetItemText(idx, 1, tmp2);
		}
	}

	if (m_bAscending)
		SortItems((PFNLVCOMPARE)ListViewCompareProc, m_nSortedCol);
	else
		SortItems((PFNLVCOMPARE)ListViewCompareProc, m_nSortedCol|0xF0);

	EnsureVisible(m_nLastPieces[pBar->m_nCurGroup], FALSE);
	SetItemState (m_nLastPieces[pBar->m_nCurGroup], LVIS_SELECTED | LVIS_FOCUSED , LVIS_SELECTED | LVIS_FOCUSED);
	SetRedraw (TRUE);
	Invalidate();
}

int CPiecesList::CellRectFromPoint(CPoint & point, RECT* cellrect, int* col) const
{
	int colnum;
	
	if((GetStyle() & LVS_TYPEMASK) != LVS_REPORT)
		return -1;
	
	// Get the top and bottom row visible
	int row = GetTopIndex();
	int bottom = row + GetCountPerPage();
	if( bottom > GetItemCount() )
		bottom = GetItemCount();
	
	// Get the number of columns
	CHeaderCtrl* pHeader = (CHeaderCtrl*)GetDlgItem(0);
	if (!pHeader) return -1;
	int nColumnCount = pHeader->GetItemCount();
	
	// Loop through the visible rows
	for( ;row <=bottom;row++)
	{
		// Get bounding rect of item and check whether point falls in it.
		CRect rect;
		GetItemRect( row, &rect, LVIR_BOUNDS );
		if( rect.PtInRect(point) )
		{
			// Now find the column
			for (colnum = 0; colnum < nColumnCount; colnum++)
			{
				int colwidth = GetColumnWidth(colnum);
				if( point.x >= rect.left && 
					point.x <= (rect.left + colwidth ) )
				{
					// Found the column
					RECT rectClient;
					GetClientRect( &rectClient );
					if( point.x > rectClient.right )
						return -1;
					if( col ) 
						*col = colnum;
					rect.right = rect.left + colwidth;
					if( rect.right > rectClient.right ) 
						rect.right = rectClient.right;
					*cellrect = rect;
					return row;
				}
				rect.left += colwidth;
			}
		}
	}
	return -1;
}

void CPiecesList::OnMouseMove(UINT nFlags, CPoint point) 
{
	if( nFlags == 0 )
	{
		int row, col;
		RECT cellrect;
		row = CellRectFromPoint (point, &cellrect, &col);
		if (row != -1)
		{
			int offset = 7;
			if( col == 0 ) 
			{
				CRect rcLabel;
				GetItemRect(row, &rcLabel, LVIR_LABEL);
				offset = rcLabel.left - cellrect.left + offset / 2;
			}
			cellrect.top--;

			m_TitleTip.Show (cellrect, GetItemText(row, col), offset-1, GetItemState (row, LVIS_FOCUSED));
		}
	}
	
	CListCtrl::OnMouseMove(nFlags, point);
}

int CPiecesList::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_TitleTip.Create(this);

	return 0;
}

void CPiecesList::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar == VK_INSERT)
	{
		project->HandleCommand(LC_PIECE_INSERT, 0);

		CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
		CView* pView = pFrame->GetActiveView();
		pView->SetFocus();
	}
	else
		CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CPiecesList::SubclassHeader()
{
	// Get the window handle to the existing header control.
	HWND hWnd = GetDlgItem(0)->GetSafeHwnd();
	ASSERT(hWnd);

	// subclass the header control.
	m_HeaderCtrl.SubclassWindow(hWnd);
	m_HeaderCtrl.SetSortImage(0, FALSE);
}
