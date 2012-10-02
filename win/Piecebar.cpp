#include "lc_global.h"
#include "resource.h"
#include "piecebar.h"
#include "lc_library.h"
#include "pieceinf.h"
#include "project.h"
#include "globals.h"
#include "lc_application.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////
// CPiecesBar

int PiecesSortFunc(const PieceInfo* a, const PieceInfo* b, void* SortData)
{
	if (a->IsSubPiece())
	{
		if (b->IsSubPiece())
		{
			return strcmp(a->m_strDescription, b->m_strDescription);
		}
		else
		{
			return 1;
		}
	}
	else
	{
		if (b->IsSubPiece())
		{
			return -1;
		}
		else
		{
			return strcmp(a->m_strDescription, b->m_strDescription);
		}
	}

	return 0;
}

CPiecesBar::CPiecesBar()
{
	int i = AfxGetApp()->GetProfileInt("Settings", "Piecebar Options", 0);
	m_bSubParts = (i & PIECEBAR_SUBPARTS) != 0;
	m_nPreviewHeight = AfxGetApp()->GetProfileInt("Settings", "Preview Height", 93);
}

CPiecesBar::~CPiecesBar()
{
	AfxGetApp()->WriteProfileInt("Settings", "Preview Height", m_nPreviewHeight);
}

BEGIN_MESSAGE_MAP(CPiecesBar, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_LC_SPLITTER_MOVED, OnSplitterMoved)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////
// CPiecesBar message handlers

void CPiecesBar::AdjustLayout(int cx, int cy)
{
	if (!IsWindow(m_wndColorList.m_hWnd))
		return;

	int off = 161;
	m_wndColorList.SetWindowPos (NULL, 5, cy-off, cx-10, 156, SWP_NOZORDER);

	off += 30;
	m_wndPiecesCombo.SetWindowPos (NULL, 5, cy-off, cx-10, 140, SWP_NOZORDER);

	m_wndSplitter.SetWindowPos (NULL, 5, m_nPreviewHeight+6, cx-10, 4, SWP_NOZORDER);
	m_PiecesTree.SetWindowPos (NULL, 5, m_nPreviewHeight+10, cx-10, cy-off-15-m_nPreviewHeight, SWP_NOZORDER);
	m_wndPiecePreview.SetWindowPos (NULL, 5, 5, cx-10, m_nPreviewHeight, 0);
	m_wndPiecePreview.EnableWindow (TRUE);

	InvalidateRect(NULL, TRUE);
	m_wndPiecePreview.InvalidateRect(NULL, FALSE);
	m_wndSplitter.InvalidateRect(NULL, TRUE);
	m_PiecesTree.InvalidateRect(NULL, TRUE);

	m_wndPiecePreview.ShowWindow (SW_SHOW);
	m_wndSplitter.ShowWindow (SW_SHOW);
	m_wndPiecesCombo.ShowWindow(SW_SHOW);
}

int CPiecesBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_PiecesTree.Create(WS_VISIBLE|WS_TABSTOP|WS_BORDER|TVS_SHOWSELALWAYS|TVS_HASBUTTONS|TVS_HASLINES|TVS_LINESATROOT|TVS_INFOTIP, 
	                    CRect(0,0,0,0), this, IDW_PIECESTREE);

	m_wndColorList.Create(WS_VISIBLE | WS_TABSTOP | WS_CHILD | WS_BORDER, CRect(0,0,0,0), this, IDW_COLORSLIST);

	m_wndPiecesCombo.Create(CBS_DROPDOWN|CBS_SORT|CBS_HASSTRINGS|WS_VISIBLE|WS_CHILD|WS_VSCROLL|WS_TABSTOP, CRect (0,0,0,0), this, IDW_PIECESCOMBO);

	//  Create a font for the combobox
	LOGFONT logFont;
	memset(&logFont, 0, sizeof(logFont));

	if (!::GetSystemMetrics(SM_DBCSENABLED))
	{
		logFont.lfHeight = -10;
		logFont.lfWeight = 0;
		logFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
		lstrcpy(logFont.lfFaceName, "MS Sans Serif");
		if (m_Font.CreateFontIndirect(&logFont))
			m_wndPiecesCombo.SetFont(&m_Font);
	}
	else
	{
		m_Font.Attach(::GetStockObject(SYSTEM_FONT));
		m_wndPiecesCombo.SetFont(&m_Font);
	}

	m_wndPiecePreview.Create (NULL, NULL, WS_BORDER|WS_CHILD|WS_VISIBLE, CRect(0, 0, 0, 0), this, IDW_PIECEPREVIEW);
	
	CreateWindow("STATIC", "", WS_VISIBLE|WS_CHILD|SS_ETCHEDFRAME, 0, 0, 0, 0,
	             m_hWnd, (HMENU)IDW_PIECEBAR_SPLITTER, AfxGetInstanceHandle(), NULL);

	// y-splitter
	m_wndSplitter.BindWithControl(this, IDW_PIECEBAR_SPLITTER);
	m_wndSplitter.SetMinHeight(0, 0);
	m_wndSplitter.AttachAsAbovePane(IDW_PIECEPREVIEW);
	m_wndSplitter.AttachAsBelowPane(IDW_PIECESTREE);
	m_wndSplitter.RecalcLayout();

	return 0;
}

BOOL CPiecesBar::OnEraseBkgnd(CDC* pDC)
{
	CRect rectClient;
	GetClientRect(rectClient);

	CMFCVisualManager::GetInstance()->OnFillBarBackground(pDC, this, rectClient, rectClient);

	return TRUE;
}

void CPiecesBar::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout(cx, cy);
}

void CPiecesBar::OnSelChangeColor()
{
	lcGetActiveProject()->HandleNotify(LC_COLOR_CHANGED, m_wndColorList.GetColorIndex());
	m_wndPiecePreview.PostMessage(WM_PAINT);
}

LONG CPiecesBar::OnSplitterMoved(UINT lParam, LONG wParam)
{
	UNREFERENCED_PARAMETER(wParam);

	if (lParam == 0)
		m_bNoContext = TRUE;
	else
		m_nPreviewHeight += lParam;

	CRect rectClient;
	GetClientRect(rectClient);

	AdjustLayout(rectClient.Width(), rectClient.Height());

	return TRUE;
}

void CPiecesBar::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	UNREFERENCED_PARAMETER(pWnd);

	if (m_bNoContext)
		m_bNoContext = FALSE;
	else
	{
		CMenu menuPopups;
		menuPopups.LoadMenu(IDR_POPUPS);
		CMenu* pMenu = menuPopups.GetSubMenu(0);

		if (pMenu)
		{
			bool CategorySelected = false;

			CRect r;
			m_PiecesTree.GetWindowRect(&r);

			if (r.PtInRect(point))
			{
				HTREEITEM Item = m_PiecesTree.GetSelectedItem();

				if (Item != NULL)
				{
					lcPiecesLibrary *Lib = lcGetPiecesLibrary();
					CString CategoryName = m_PiecesTree.GetItemText(Item);
					int CategoryIndex = Lib->FindCategoryIndex((const char*)CategoryName);

					if (CategoryIndex != -1)
						CategorySelected = true;
				}

				pMenu->EnableMenuItem(ID_PIECEBAR_NEWCATEGORY, MF_BYCOMMAND | MF_ENABLED);
			}
			else
			{
				pMenu->EnableMenuItem(ID_PIECEBAR_NEWCATEGORY, MF_BYCOMMAND | MF_GRAYED);
			}

			pMenu->EnableMenuItem(ID_PIECEBAR_REMOVECATEGORY, MF_BYCOMMAND | (CategorySelected ? MF_ENABLED : MF_GRAYED));
			pMenu->EnableMenuItem(ID_PIECEBAR_EDITCATEGORY, MF_BYCOMMAND | (CategorySelected ? MF_ENABLED : MF_GRAYED));

			pMenu->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON, point.x, point.y, AfxGetMainWnd());
		}
	}
}

void CPiecesBar::SelectPiece(const char* Category, PieceInfo* Info)
{
	HTREEITEM Item = m_PiecesTree.GetChildItem(TVI_ROOT);
	const char* PieceName = Info->m_strDescription;

	// Find the category and make sure it's expanded.
	while (Item != NULL)
	{
		CString Name = m_PiecesTree.GetItemText(Item);

		if (Name == Category)
		{
			m_PiecesTree.Expand(Item, TVE_EXPAND);
			break;
		}

		Item = m_PiecesTree.GetNextSiblingItem(Item);
	}

	if (Item == NULL)
		return;

	// Expand the piece group if it's patterned.
	if (Info->IsPatterned())
	{
		PieceInfo* Parent;

		// Find the parent of this patterned piece and expand it.
		char ParentName[LC_PIECE_NAME_LEN];
		strcpy(ParentName, Info->m_strName);
		*strchr(ParentName, 'P') = '\0';

		Parent = lcGetPiecesLibrary()->FindPiece(ParentName, false);

		if (Parent)
		{
			Item = m_PiecesTree.GetChildItem(Item);

			while (Item != NULL)
			{
				CString Name = m_PiecesTree.GetItemText(Item);

				if (Name == Parent->m_strDescription)
				{
					m_PiecesTree.Expand(Item, TVE_EXPAND);

					// If both descriptions begin with the same text, only show the difference.
					if (!strncmp(Info->m_strDescription, Parent->m_strDescription, strlen(Parent->m_strDescription)))
						PieceName = Info->m_strDescription + strlen(Parent->m_strDescription) + 1;

					break;
				}

				Item = m_PiecesTree.GetNextSiblingItem(Item);
			}
		}
	}

	// Find the piece.
	Item = m_PiecesTree.GetChildItem(Item);

	while (Item != NULL)
	{
		CString Name = m_PiecesTree.GetItemText(Item);

		if (Name == PieceName)
		{
			m_PiecesTree.SelectItem(Item);
			return;
		}

		Item = m_PiecesTree.GetNextSiblingItem(Item);
	}
}

void CPiecesBar::UpdatePiecesTree(const char* OldCategory, const char* NewCategory)
{
	if (OldCategory && NewCategory)
	{
		HTREEITEM Item = m_PiecesTree.GetChildItem(TVI_ROOT);

		while (Item != NULL)
		{
			CString Name = m_PiecesTree.GetItemText(Item);

			if (Name == OldCategory)
				break;

			Item = m_PiecesTree.GetNextSiblingItem(Item);
		}

		if (Item == NULL)
			return;

		m_PiecesTree.SetItemText(Item, NewCategory);

		m_PiecesTree.EnsureVisible(Item);
		if (m_PiecesTree.GetItemState(Item, TVIS_EXPANDED) & TVIS_EXPANDED)
		{
			m_PiecesTree.Expand(Item, TVE_COLLAPSE | TVE_COLLAPSERESET);
			m_PiecesTree.Expand(Item, TVE_EXPAND);
		}
	}
	else if (NewCategory)
	{
		TVINSERTSTRUCT Insert;

		memset(&Insert, 0, sizeof(Insert));
		Insert.hParent = TVI_ROOT;
		Insert.hInsertAfter = TVI_SORT;
		Insert.item.mask = TVIF_CHILDREN|TVIF_PARAM|TVIF_TEXT;
		Insert.item.pszText = (LPSTR)NewCategory;
		Insert.item.cChildren = 1;

		HTREEITEM Item = m_PiecesTree.InsertItem(&Insert);
		m_PiecesTree.EnsureVisible(Item);
	}
	else if (OldCategory)
	{
		HTREEITEM Item = m_PiecesTree.GetChildItem(TVI_ROOT);

		while (Item != NULL)
		{
			CString Name = m_PiecesTree.GetItemText(Item);

			if (Name == OldCategory)
				break;

			Item = m_PiecesTree.GetNextSiblingItem(Item);
		}

		if (Item == NULL)
			return;

		m_PiecesTree.DeleteItem(Item);
	}
}

void CPiecesBar::UpdatePiecesTree(bool SearchOnly)
{
	lcPiecesLibrary *Lib = lcGetPiecesLibrary();

	if (SearchOnly)
	{
		HTREEITEM Item = m_PiecesTree.GetChildItem(TVI_ROOT);

		while (Item != NULL)
		{
			CString Name = m_PiecesTree.GetItemText(Item);

			if (Name == "Search Results")
				break;

			Item = m_PiecesTree.GetNextSiblingItem(Item);
		}

		if (Item == NULL)
		{
			TVINSERTSTRUCT Insert;

			memset(&Insert, 0, sizeof(Insert));
			Insert.hParent = TVI_ROOT;
			Insert.hInsertAfter = TVI_LAST;
			Insert.item.mask = TVIF_CHILDREN|TVIF_PARAM|TVIF_TEXT;
			Insert.item.cChildren = 1;
			Insert.item.pszText = "Search Results";
			Item = m_PiecesTree.InsertItem(&Insert);
		}

		m_PiecesTree.Expand(Item, TVE_COLLAPSE | TVE_COLLAPSERESET);
		m_PiecesTree.EnsureVisible(Item);
		m_PiecesTree.Expand(Item, TVE_EXPAND);
	}
	else
	{
		m_PiecesTree.SetRedraw(FALSE);
		m_PiecesTree.DeleteAllItems();

		TVINSERTSTRUCT Insert;

		memset(&Insert, 0, sizeof(Insert));
		Insert.hParent = TVI_ROOT;
		Insert.hInsertAfter = TVI_SORT;
		Insert.item.mask = TVIF_CHILDREN|TVIF_PARAM|TVIF_TEXT;
		Insert.item.cChildren = 1;

		for (int i = 0; i < Lib->mCategories.GetSize(); i++)
		{
			if (Lib->mCategories[i].Name == "Search Results")
				continue;

			Insert.item.pszText = (LPSTR)Lib->mCategories[i].Name;
			m_PiecesTree.InsertItem(&Insert);
		}

		Insert.item.pszText = "Search Results";
		Insert.hInsertAfter = TVI_LAST;
		m_PiecesTree.InsertItem(&Insert);

		m_PiecesTree.SetRedraw(TRUE);
		m_PiecesTree.Invalidate();
	}
}

void CPiecesBar::RefreshPiecesTree()
{
	HTREEITEM Item = m_PiecesTree.GetChildItem(TVI_ROOT);

	while (Item != NULL)
	{
		if ((m_PiecesTree.GetItemState(Item, TVIF_STATE) & TVIS_EXPANDED) != 0)
		{
			m_PiecesTree.Expand(Item, TVE_COLLAPSE | TVE_COLLAPSERESET);
			m_PiecesTree.Expand(Item, TVE_EXPAND);
		}

		Item = m_PiecesTree.GetNextSiblingItem(Item);
	}
}

BOOL CPiecesBar::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	if (wParam == IDW_PIECESTREE)
	{
		LPNMTREEVIEW Notify = (LPNMTREEVIEW)lParam;

		if (Notify->hdr.code == TVN_SELCHANGED)
		{
			PieceInfo* Info = (PieceInfo*)Notify->itemNew.lParam;

			if (Info != NULL)
			{
				lcGetActiveProject()->SetCurrentPiece(Info);
				m_wndPiecePreview.SetPieceInfo(Info);
				m_wndPiecePreview.PostMessage(WM_PAINT);
			}
		}
		else if (Notify->hdr.code == TVN_BEGINDRAG)
		{
			PieceInfo* Info = (PieceInfo*)Notify->itemNew.lParam;

			if (Info != NULL)
			{
				lcGetActiveProject()->BeginPieceDrop(Info);

				// Force a cursor update.
				CFrameWndEx* pFrame = (CFrameWndEx*)AfxGetMainWnd();
				CView* pView = pFrame->GetActiveView();
				pView->PostMessage(WM_LC_SET_CURSOR, 0, 0);
			}
		}
		else if (Notify->hdr.code == TVN_GETINFOTIP)
		{
			LPNMTVGETINFOTIP Tip = (LPNMTVGETINFOTIP)lParam;
			HTREEITEM Item = Tip->hItem;
			PieceInfo* Info = (PieceInfo*)m_PiecesTree.GetItemData(Item);

			if (Info != NULL)
			{
				_snprintf(Tip->pszText, Tip->cchTextMax, "%s (%s)", Info->m_strDescription, Info->m_strName);
			}
		}
		else if (Notify->hdr.code == TVN_ITEMEXPANDING)
		{
			if (Notify->action == TVE_EXPAND)
			{
				m_PiecesTree.SetRedraw(FALSE);

				lcPiecesLibrary* Lib = lcGetPiecesLibrary();

				// Remove all children.
				HTREEITEM Item = Notify->itemNew.hItem;

				if (m_PiecesTree.ItemHasChildren(Item))
				{
					HTREEITEM NextItem;
					HTREEITEM ChildItem = m_PiecesTree.GetChildItem(Item);

					while (ChildItem != NULL)
					{
						NextItem = m_PiecesTree.GetNextItem(ChildItem, TVGN_NEXT);
						m_PiecesTree.DeleteItem(ChildItem);
						ChildItem = NextItem;
					}
				}

				// Check if we're expanding a category item.
				if (Notify->itemNew.lParam == NULL)
				{
					HTREEITEM CategoryItem = Notify->itemNew.hItem;
					CString CategoryName = m_PiecesTree.GetItemText(CategoryItem);
					int CategoryIndex = Lib->FindCategoryIndex((const char*)CategoryName);

					PtrArray<PieceInfo> SinglePieces, GroupedPieces;

					if (CategoryIndex != -1)
					{
						int i;

						Lib->GetCategoryEntries(CategoryIndex, true, SinglePieces, GroupedPieces);

						// Merge and sort the arrays.
						SinglePieces += GroupedPieces;
						SinglePieces.Sort(PiecesSortFunc, NULL);

						for (i = 0; i < SinglePieces.GetSize(); i++)
						{
							PieceInfo* Info = SinglePieces[i];

							if (!m_bSubParts && Info->IsSubPiece())
								continue;

							if (GroupedPieces.FindIndex(Info) == -1)
								m_PiecesTree.InsertItem(TVIF_PARAM|TVIF_TEXT, Info->m_strDescription, 0, 0, 0, 0, (LPARAM)Info, CategoryItem, TVI_LAST);
							else
							{
								TVINSERTSTRUCT Insert;

								memset(&Insert, 0, sizeof(Insert));
								Insert.hParent = CategoryItem;
								Insert.hInsertAfter = TVI_LAST;
								Insert.item.mask = TVIF_CHILDREN|TVIF_PARAM|TVIF_TEXT;
								Insert.item.pszText = (LPSTR)Info->m_strDescription;
								Insert.item.cChildren = 1;
								Insert.item.lParam = (LPARAM)Info;

								m_PiecesTree.InsertItem(&Insert);
							}
						}
					}

					if (CategoryName == "Search Results")
					{
						// Let the user know if the search is empty.
						if ((SinglePieces.GetSize() == 0) && (GroupedPieces.GetSize() == 0))
						{
							m_PiecesTree.InsertItem(TVIF_PARAM|TVIF_TEXT, "No pieces found", 0, 0, 0, 0, 0, CategoryItem, TVI_SORT);
						}
					}
				}
				else
				{
					PieceInfo* Parent = (PieceInfo*)Notify->itemNew.lParam;

					HTREEITEM CategoryItem = m_PiecesTree.GetParentItem(Notify->itemNew.hItem);
					CString CategoryName = m_PiecesTree.GetItemText(CategoryItem);
					int CategoryIndex = Lib->FindCategoryIndex((const char*)CategoryName);

					PtrArray<PieceInfo> Pieces;
					Lib->GetPatternedPieces(Parent, Pieces);

					Pieces.Sort(PiecesSortFunc, NULL);
					HTREEITEM ParentItem = Notify->itemNew.hItem;

					for (int i = 0; i < Pieces.GetSize(); i++)
					{
						PieceInfo* Info = Pieces[i];

						if (!m_bSubParts && Info->IsSubPiece())
							continue;

						if (CategoryIndex != -1)
						{
							if (!Lib->PieceInCategory(Info, Lib->mCategories[CategoryIndex].Keywords))
								continue;
						}

						// If both descriptions begin with the same text, only show the difference.
						if (!strncmp(Info->m_strDescription, Parent->m_strDescription, strlen(Parent->m_strDescription)))
							m_PiecesTree.InsertItem(TVIF_PARAM|TVIF_TEXT, Info->m_strDescription + strlen(Parent->m_strDescription) + 1, 0, 0, 0, 0, (LPARAM)Info, ParentItem, TVI_LAST);
						else
							m_PiecesTree.InsertItem(TVIF_PARAM|TVIF_TEXT, Info->m_strDescription, 0, 0, 0, 0, (LPARAM)Info, ParentItem, TVI_LAST);
					}
				}

				m_PiecesTree.SetRedraw(TRUE);
				m_PiecesTree.Invalidate();
			}
			else if (Notify->action == TVE_COLLAPSE)
			{
				m_PiecesTree.Expand(Notify->itemNew.hItem, TVE_COLLAPSE | TVE_COLLAPSERESET);
			}
		}
	}

	return CDockablePane::OnNotify(wParam, lParam, pResult);
}
