// PieceLst.cpp : implementation file
//

#include "stdafx.h"
#include "leocad.h"
#include "PieceLst.h"
#include "PieceBar.h"
#include "project.h"
#include "pieceinf.h"
#include "library.h"
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
	ClearGroups();
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

		CRect Rect;
		POINT MousePos;

		GetCursorPos(&MousePos);
		GetItemRect(pNMListView->iItem, &Rect, LVIR_BOUNDS);
		ScreenToClient(&MousePos);

		if (Rect.PtInRect(MousePos))
		{
			int row, col;
			RECT cellrect;
			row = CellRectFromPoint (CPoint(MousePos), &cellrect, &col);
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

				m_TitleTip.ShowWindow(SW_HIDE);
				m_TitleTip.Show (cellrect, GetItemText(row, col), offset-1, GetItemState (row, LVIS_FOCUSED));
			}
		}
	}
	else if (pNMListView->uOldState & LVIS_SELECTED)
	{
		CRect Rect;
		POINT MousePos;

		GetCursorPos(&MousePos);
		GetItemRect(pNMListView->iItem, &Rect, LVIR_BOUNDS);
		ScreenToClient(&MousePos);

		if (Rect.PtInRect(MousePos))
		{
			CWnd* CaptureWnd = GetCapture();
			if ((CaptureWnd != NULL) && (CaptureWnd ->m_hWnd == m_TitleTip.m_hWnd))
				ReleaseCapture();

			m_TitleTip.ShowWindow(SW_HIDE);
		}
	}

	*pResult = 0;
}

void CPiecesList::ClearGroups()
{
	for (int i = 0; i < m_Groups.GetSize(); i++)
		delete m_Groups[i];

	m_Groups.RemoveAll();
	m_Pieces.RemoveAll();
}

void CPiecesList::UpdateGroups()
{
	CPiecesBar* Bar = (CPiecesBar*)GetParent();
	PiecesLibrary *Lib = project->GetPiecesLibrary();
	int i, j;

	ClearGroups();

	for (i = 0; i < Lib->GetPieceCount(); i++)
	{
		PieceInfo* Info = Lib->GetPieceInfo(i);

		// Skip subparts if the user doesn't want to see them.
		if ((Info->m_strDescription[0] == '~') && !Bar->m_bSubParts)
			continue;

		// Skip pieces not in the current category.
		if (Bar->m_bGroups && ((Info->m_nGroups & (DWORD)(1 << Bar->m_nCurGroup)) == 0))
			continue;

		// There's nothing else to check if we're not grouping patterned pieces together.
		if (!m_GroupPatterns)
		{
			m_Pieces.Add(Info);
			continue;
		}

		// Check if it's a patterned piece.
		const char* Name = Info->m_strName;
		while (*Name)
		{
			if (!*Name || *Name < '0' || *Name > '9')
				break;

			if (*Name == 'P')
				break;

			Name++;
		}

		if (*Name == 'P')
		{
			PieceListGroup* Group = NULL;
			PieceInfo* Parent;

			char ParentName[9];
			strcpy(ParentName, Info->m_strName);
			ParentName[Name - Info->m_strName] = '\0';

			Parent = Lib->FindPieceInfo(ParentName);

			if (Parent)
			{
				// Search for the parent's group.
				for (j = 0; j < m_Groups.GetSize(); j++)
				{
					if (m_Groups[j]->Parent == Parent)
					{
						Group = m_Groups[j];
						break;
					}
				}

				// Check if the parent wasn't added as a single piece.
				if (!Group)
				{
					for (j = 0; j < m_Pieces.GetSize(); j++)
					{
						if (m_Pieces[j] == Parent)
						{
							m_Pieces.RemoveIndex(j);

							Group = new PieceListGroup();
							Group->Parent = Parent;
							Group->Collapsed = true;
							m_Groups.Add(Group);

							break;
						}
					}
				}

				// Create a new group for the parent.
				if (!Group)
				{
					Group = new PieceListGroup();
					Group->Parent = Parent;
					Group->Collapsed = true;
					m_Groups.Add(Group);
				}

				Group->Children.Add(Info);
			}
			else
			{
				m_Pieces.Add(Info);
			}
		}
		else
		{
			bool Found = false;

			// Add piece to the list if it's not there already,
			// this could be the parent whose child was added earlier.
			for (j = 0; j < m_Groups.GetSize(); j++)
			{
				if (m_Groups[j]->Parent == Info)
				{
					Found = true;
					break;
				}
			}

			if (!Found)
			{
				m_Pieces.Add(Info);
			}
		}
	}
}

void CPiecesList::AddPiece(PieceInfo* Info, int ImageIndex)
{
	LV_ITEM lvi;

	if (ImageIndex == -1)
		lvi.mask = LVIF_TEXT | LVIF_PARAM;
	else
		lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;

	TCHAR tmp[65], tmp2[10];
	strcpy(tmp, Info->m_strDescription);
	lvi.iImage = ImageIndex;
	lvi.iItem = 0;
	lvi.iSubItem = 0;
	lvi.pszText = tmp;
	lvi.lParam = (LPARAM)Info;
	int idx = InsertItem(&lvi);

	strcpy(tmp2, Info->m_strName);
	SetItemText(idx, 1, tmp2);
}

void CPiecesList::UpdateList(bool Repopulate)
{
	CWaitCursor wc;
	int i;

	if (Repopulate)
		UpdateGroups();

	SetRedraw(FALSE);
	DeleteAllItems();

/*
	PiecesLibrary *pLib = project->GetPiecesLibrary();

	for (int i = 0; i < pLib->GetPieceCount(); i++)
	{
		PieceInfo* pInfo = pLib->GetPieceInfo(i);

		// Skip subparts if the user doesn't want to see them.
		if ((pInfo->m_strDescription[0] == '~') && !pBar->m_bSubParts)
			continue;

		// Skip pieces not in the current category.
		if (pBar->m_bGroups && ((pInfo->m_nGroups & (DWORD)(1 << pBar->m_nCurGroup)) == 0))
			continue;

		TCHAR tmp[65], tmp2[10];
		strcpy(tmp, pInfo->m_strDescription);
		lvi.iItem = 0;
		lvi.iSubItem = 0;
		lvi.pszText = tmp;
		lvi.lParam = (LPARAM)pInfo;
		lvi.iImage = 1;
		int idx = InsertItem(&lvi);

		strcpy(tmp2, pInfo->m_strName);
		SetItemText(idx, 1, tmp2);
	}
*/

	for (i = 0; i < m_Groups.GetSize(); i++)
	{
		PieceListGroup* Group = m_Groups[i];

		AddPiece(Group->Parent, (Group->Collapsed) ? 0 : 1);

		if (!Group->Collapsed)
		{
			for (int j = 0; j < Group->Children.GetSize(); j++)
			{
				AddPiece(Group->Children[j], 3);
			}
		}
	}

	for (i = 0; i < m_Pieces.GetSize(); i++)
	{
		AddPiece(m_Pieces[i], 3);
	}

	if (m_bAscending)
		SortItems((PFNLVCOMPARE)ListViewCompareProc, m_nSortedCol);
	else
		SortItems((PFNLVCOMPARE)ListViewCompareProc, m_nSortedCol|0xF0);

	CPiecesBar* pBar = (CPiecesBar*)GetParent();
	EnsureVisible(m_nLastPieces[pBar->m_nCurGroup], FALSE);
	SetItemState(m_nLastPieces[pBar->m_nCurGroup], LVIS_SELECTED | LVIS_FOCUSED , LVIS_SELECTED | LVIS_FOCUSED);
	SetRedraw(TRUE);
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
	if (nFlags == 0)
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

			m_TitleTip.Show(cellrect, GetItemText(row, col), offset-1, GetItemState(row, LVIS_FOCUSED) && (col == 0));
		}
	}
	
	CListCtrl::OnMouseMove(nFlags, point);
}

int CPiecesList::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_Images.Create(IDB_PARTICONS, 16, 0, RGB (0,128,128));
	SetImageList(&m_Images, LVSIL_SMALL);

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
	else if (nChar == VK_RIGHT)
	{
		int Index = GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
		PieceInfo* Info = (PieceInfo*)GetItemData(Index);

		for (int i = 0; i < m_Groups.GetSize(); i++)
		{
			PieceListGroup* Group = m_Groups[i];

			if (Group->Parent == Info)
				Group->Collapsed = false;
		}

		UpdateList(false);
	}
	else if (nChar == VK_LEFT)
	{
		int Index = GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
		PieceInfo* Info = (PieceInfo*)GetItemData(Index);

		for (int i = 0; i < m_Groups.GetSize(); i++)
		{
			PieceListGroup* Group = m_Groups[i];

			if (Group->Parent == Info)
				Group->Collapsed = true;
		}

		UpdateList(false);
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
