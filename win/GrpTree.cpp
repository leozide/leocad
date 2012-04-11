// GrpTree.cpp : implementation file
//

#include "stdafx.h"
#include "leocad.h"
#include "GrpTree.h"
#include "group.h"
#include "piece.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGroupEditTree

CGroupEditTree::CGroupEditTree()
{
  m_bDragging = FALSE;
}

CGroupEditTree::~CGroupEditTree()
{
}


BEGIN_MESSAGE_MAP(CGroupEditTree, CTreeCtrl)
	//{{AFX_MSG_MAP(CGroupEditTree)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBeginDrag)
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_NOTIFY_REFLECT(TVN_BEGINLABELEDIT, OnBeginLabelEdit)
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, OnEndLabelEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGroupEditTree message handlers

void CGroupEditTree::AddChildren(HTREEITEM hParent, Group* pGroup)
{
	int i;
	TV_INSERTSTRUCT tvstruct;
	tvstruct.hParent = hParent;
	tvstruct.hInsertAfter = TVI_SORT;

	for (i = 0; i < opts->groupcount; i++)
		if (opts->groupsgroups[i] == pGroup)
		{
			tvstruct.item.lParam = i + 0xFFFF;
			tvstruct.item.iImage = 0;
			tvstruct.item.iSelectedImage = 1;
			tvstruct.item.pszText = opts->groups[i]->m_strName;
			tvstruct.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;

			HTREEITEM hti = InsertItem(&tvstruct);
			AddChildren(hti, opts->groups[i]);
		}

	for (i = 0; i < opts->piececount; i++)
		if (opts->piecesgroups[i] == pGroup)
		{
			tvstruct.item.lParam = i;
			tvstruct.item.iImage = 2;
			tvstruct.item.iSelectedImage = 2;
			tvstruct.item.pszText = (char*)opts->pieces[i]->GetName();
			tvstruct.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			InsertItem(&tvstruct);
		}
}

void CGroupEditTree::OnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;
	GetEditControl()->LimitText(80);
	*pResult = 0;
}

void CGroupEditTree::OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;

	// Set result to TRUE to accept the changes
	*pResult = TRUE;
}

void CGroupEditTree::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	*pResult = 0;

	SetFocus();	// Receive WM_KEYDOWN
	m_hitemDrag = pNMTreeView->itemNew.hItem;
	m_hitemDrop = NULL;

	SelectItem(m_hitemDrag);
	if (!IsDropSource(m_hitemDrag))
		return;

	// get the image list for dragging
	m_pDragImage = CreateDragImage(m_hitemDrag);

	// CreateDragImage() returns NULL if no image list
	// associated with the tree view control
	if (!m_pDragImage)
		return;

	m_bDragging = TRUE;
	m_pDragImage->BeginDrag(0, CPoint(-15,-15));

	m_dropCursor = LoadCursor(NULL, IDC_ARROW);
	m_noDropCursor = LoadCursor(NULL, IDC_NO);

	POINT pt = pNMTreeView->ptDrag;
	ClientToScreen(&pt);
	m_pDragImage->DragEnter(NULL, pt);
	SetCapture();
	
	*pResult = 0;
}

void CGroupEditTree::OnMouseMove(UINT nFlags, CPoint point) 
{
	HTREEITEM hitem;
	UINT flags;

	if (m_bDragging)
	{
		POINT pt = point;
		ClientToScreen(&pt);
		CImageList::DragMove(pt);
		if ((hitem = HitTest(point, &flags)) != NULL)
		{
			CImageList::DragShowNolock(FALSE);
			m_hitemDrop = GetDropTarget(hitem);
			SelectDropTarget(m_hitemDrop);
			CImageList::DragShowNolock(TRUE);
		}
		else
			m_hitemDrop = NULL;

		if (m_hitemDrop)
			SetCursor(m_dropCursor);
		else
			SetCursor(m_noDropCursor);
	}

	CTreeCtrl::OnMouseMove(nFlags, point);
}

void CGroupEditTree::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CTreeCtrl::OnLButtonUp(nFlags, point);

	if (m_bDragging)
	{
		m_bDragging = FALSE;
		CImageList::DragLeave(this);
		CImageList::EndDrag();
		ReleaseCapture();

		delete m_pDragImage;

		// Remove drop target highlighting
		SelectDropTarget(NULL);

		if (m_hitemDrag == m_hitemDrop || m_hitemDrop == NULL)
			return;

		// If Drag item is an ancestor of Drop item then return
		HTREEITEM htiParent = m_hitemDrop;
		while ((htiParent = GetParentItem(htiParent)) != NULL)
			if (htiParent == m_hitemDrag)
				return;
		
		Expand(m_hitemDrop, TVE_EXPAND);

		DWORD source = GetItemData(m_hitemDrag);
		DWORD dest = GetItemData(m_hitemDrop) - 0xFFFF;
		TV_INSERTSTRUCT tvstruct;
		tvstruct.hParent = m_hitemDrop;
		tvstruct.hInsertAfter = TVI_SORT;
		tvstruct.item.lParam = source;
		tvstruct.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		DeleteItem(m_hitemDrag);

		if (source < 0xFFFF)
		{
			opts->piecesgroups[source] = opts->groups[dest];

			tvstruct.item.iImage = 2;
			tvstruct.item.iSelectedImage = 2;
			tvstruct.item.pszText = (char*)opts->pieces[source]->GetName();
			InsertItem(&tvstruct);
		}
		else
		{
			opts->groupsgroups[source - 0xFFFF] = opts->groups[dest];

			tvstruct.item.iImage = 0;
			tvstruct.item.iSelectedImage = 1;
			tvstruct.item.pszText = opts->groups[source - 0xFFFF]->m_strName;

			HTREEITEM hti = InsertItem(&tvstruct);
			AddChildren(hti, opts->groups[source - 0xFFFF]);
		}

//		AddChildren(m_hitemDrop, opts->groups[dest]);
	}
}

BOOL CGroupEditTree::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (GetEditControl() 
			&& (pMsg->wParam == VK_RETURN
			||  pMsg->wParam == VK_ESCAPE))
		{
			::TranslateMessage(pMsg);
			::DispatchMessage(pMsg);
			return TRUE;	// DO NOT process further
		}

		if (pMsg->wParam == VK_ESCAPE 
			&& m_bDragging)
		{
			m_bDragging = 0;
			CImageList::DragLeave(NULL);
			CImageList::EndDrag();
			ReleaseCapture();
			SelectDropTarget(NULL);
			delete m_pDragImage;
			return TRUE;	// DO NOT process further
		}
	}

	return CTreeCtrl::PreTranslateMessage(pMsg);
}

BOOL CGroupEditTree::IsDropSource(HTREEITEM /*hItem*/)
{
	return TRUE;
}

HTREEITEM CGroupEditTree::GetDropTarget(HTREEITEM hItem)
{
	if (GetItemData(hItem) < 0xFFFF)
		hItem = GetParentItem(hItem);

	// inhibit drop on the drop source or its parent
	if (hItem == m_hitemDrag || hItem == GetParentItem(m_hitemDrag))
		return NULL;

	HTREEITEM htiParent = hItem;
	while ((htiParent = GetParentItem(htiParent)) != NULL)
		if (htiParent == m_hitemDrag)
			return NULL;

	return hItem;
}
