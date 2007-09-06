// SizingControlBar.cpp : implementation file
//

#include "lc_global.h"
#include "afxpriv.h"    // for CDockContext
#include "resource.h"
#include "piecebar.h"
#include "library.h"
#include "pieceinf.h"
#include "project.h"
#include "lc_colors.h"
#include "lc_application.h"
#include "preview.h"

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
	m_bNumbers = (i & PIECEBAR_PARTNUMBERS) != 0;

	m_dwSCBStyle |= SCBS_SHOWEDGES;
	m_nPreviewHeight = AfxGetApp()->GetProfileInt("Settings", "Preview Height", 93);

	m_szMinHorz = m_szMinVert = m_szMinFloat = CSize(228, 200);
}

CPiecesBar::~CPiecesBar()
{
	AfxGetApp()->WriteProfileInt("Settings", "Preview Height", m_nPreviewHeight);
}

BEGIN_MESSAGE_MAP(CPiecesBar, CSizingControlBarG)
	//{{AFX_MSG_MAP(CPiecesBar)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
	ON_LBN_SELCHANGE(IDW_COLORSLIST, OnSelChangeColor)
	ON_MESSAGE(WM_LC_SPLITTER_MOVED, OnSplitterMoved)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////
// CPiecesBar message handlers

void CPiecesBar::OnSize(UINT nType, int cx, int cy) 
{
	CSizingControlBarG::OnSize(nType, cx, cy);

	if (!IsWindow(m_wndColorsList.m_hWnd))
		return;

	int off = LC_COLORLIST_NUM_ROWS*12+2+5;
	int ColorWidth = ((cx-2) / LC_COLORLIST_NUM_COLS) * LC_COLORLIST_NUM_COLS + 2;
	m_wndColorsList.SetWindowPos(NULL, (cx-ColorWidth)/2, cy-off, ColorWidth, LC_COLORLIST_NUM_ROWS*12+2, SWP_NOZORDER);

	off += 30;
	m_wndPiecesCombo.SetWindowPos (NULL, 5, cy-off, cx-10, 140, SWP_NOZORDER);

	m_wndSplitter.SetWindowPos(NULL, 5, m_nPreviewHeight+6, cx-10, 4, SWP_NOZORDER);
	m_PiecesTree.SetWindowPos(NULL, 5, m_nPreviewHeight+10, cx-10, cy-off-15-m_nPreviewHeight, SWP_NOZORDER);
	m_wndPiecePreview.SetWindowPos(NULL, 5, 5, cx-10, m_nPreviewHeight, 0);
	m_wndPiecePreview.EnableWindow(TRUE);
	m_wndPiecePreview.ShowWindow(SW_SHOW);
	m_wndSplitter.ShowWindow(SW_SHOW);

	m_wndPiecesCombo.ShowWindow(SW_SHOW);
}

int CPiecesBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CSizingControlBarG::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_PiecesTree.Create(WS_VISIBLE|WS_TABSTOP|WS_BORDER|TVS_SHOWSELALWAYS|TVS_HASBUTTONS|TVS_HASLINES|TVS_LINESATROOT|TVS_INFOTIP, 
	                    CRect(0,0,0,0), this, IDW_PIECESTREE);

	m_wndPiecesCombo.Create(CBS_DROPDOWN|CBS_SORT|CBS_HASSTRINGS|WS_VISIBLE|WS_CHILD|
	                        WS_VSCROLL|WS_TABSTOP, CRect(0,0,0,0), this, IDW_PIECESCOMBO);

	m_wndColorsList.Create(WS_VISIBLE|WS_TABSTOP|WS_CHILD, CRect(0, 0, 0, 0), this, IDW_COLORSLIST);

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

	m_wndPiecePreview.Create(NULL, NULL, WS_BORDER|WS_CHILD|WS_VISIBLE,
	                         CRect(0, 0, 0, 0), this, IDW_PIECEPREVIEW);

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

void CPiecesBar::OnSelChangeColor()
{
	lcGetActiveProject()->HandleNotify(LC_COLOR_CHANGED, m_wndColorsList.GetCurColor());
	m_wndPiecePreview.PostMessage(WM_PAINT);
}

LONG CPiecesBar::OnSplitterMoved(UINT lParam, LONG wParam)
{
	UNREFERENCED_PARAMETER(wParam);

	if (lParam == 0)
		m_bNoContext = TRUE;
	else
		m_nPreviewHeight += lParam;

	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////
// CPiecesBar implementation helpers

void CPiecesBar::OnUpdateCmdUI(CFrameWnd * pTarget, BOOL bDisableIfNoHndler)
{
//	CWnd::OnUpdateCmdUI(pTarget, FALSE);
//	int nID = ID_PIECE_GROUP02;

//	int sta = m_wndGroupsBar.GetToolBarCtrl().GetState(nID) & ~TBSTATE_ENABLED;
//	if (bEnable)
//		sta |= TBSTATE_ENABLED|TBSTATE_CHECKED;
//	m_wndGroupsBar.GetToolBarCtrl().SetState(nID, sta);

	UpdateDialogControls(pTarget, FALSE);
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
					PiecesLibrary *Lib = lcGetPiecesLibrary();
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
		char ParentName[9];
		strcpy(ParentName, Info->m_strName);
		*strchr(ParentName, 'P') = '\0';

		Parent = lcGetPiecesLibrary()->FindPieceInfo(ParentName);

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
		HTREEITEM Item;
		Item = m_PiecesTree.InsertItem(TVIF_CHILDREN|TVIF_PARAM|TVIF_TEXT, NewCategory, 0, 0, 0, 0, 0, TVI_ROOT, TVI_SORT);
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
	PiecesLibrary* Lib = lcGetPiecesLibrary();

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
			Item = m_PiecesTree.InsertItem(TVIF_CHILDREN|TVIF_PARAM|TVIF_TEXT, "Search Results", 0, 0, 0, 0, 0, TVI_ROOT, TVI_LAST);
		}

		m_PiecesTree.Expand(Item, TVE_COLLAPSE | TVE_COLLAPSERESET);
		m_PiecesTree.EnsureVisible(Item);
		m_PiecesTree.Expand(Item, TVE_EXPAND);
	}
	else
	{
		m_PiecesTree.SetRedraw(FALSE);
		m_PiecesTree.DeleteAllItems();

		for (int i = 0; i < Lib->GetNumCategories(); i++)
		{
			if (Lib->GetCategoryName(i) == "Search Results")
				continue;

			m_PiecesTree.InsertItem(TVIF_CHILDREN|TVIF_PARAM|TVIF_TEXT, Lib->GetCategoryName(i), 0, 0, 0, 0, 0, TVI_ROOT, TVI_SORT);
		}

		m_PiecesTree.InsertItem(TVIF_CHILDREN|TVIF_PARAM|TVIF_TEXT, "Models", 0, 0, 0, 0, 0, TVI_ROOT, TVI_LAST);
		m_PiecesTree.InsertItem(TVIF_CHILDREN|TVIF_PARAM|TVIF_TEXT, "Search Results", 0, 0, 0, 0, 0, TVI_ROOT, TVI_LAST);

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
			void* Selection = (void*)Notify->itemNew.lParam;

			if (Selection)
				g_App->m_PiecePreview->SetSelection(Selection);
		}
		else if (Notify->hdr.code == TVN_BEGINDRAG)
		{
			if (Notify->itemNew.lParam)
			{
				m_PiecesTree.SelectItem(Notify->itemNew.hItem);

				lcGetActiveProject()->BeginPieceDrop();

				// Force a cursor update.
				CFrameWnd* pFrame = (CFrameWnd*)AfxGetMainWnd();
				CView* pView = pFrame->GetActiveView();
				pView->PostMessage(WM_LC_SET_CURSOR, 0, 0);
			}
		}
		else if (Notify->hdr.code == TVN_GETINFOTIP)
		{
			LPNMTVGETINFOTIP Tip = (LPNMTVGETINFOTIP)lParam;
			HTREEITEM Item = Tip->hItem;
			void* Data = (void*)m_PiecesTree.GetItemData(Item);

			if (Data)
			{
				if (lcGetActiveProject()->m_ModelList.FindIndex((lcModel*)Data) != -1)
				{
					lcModel* Model = (lcModel*)Data;
					_snprintf(Tip->pszText, Tip->cchTextMax, "%s", (const char*)Model->m_Name);
				}
				else
				{
					PieceInfo* Info = (PieceInfo*)Data;
					_snprintf(Tip->pszText, Tip->cchTextMax, "%s (%s)", Info->m_strDescription, Info->m_strName);
				}
			}
		}
		else if (Notify->hdr.code == TVN_ITEMEXPANDING)
		{
			if (Notify->action == TVE_EXPAND)
			{
				m_PiecesTree.SetRedraw(FALSE);

				PiecesLibrary *Lib = lcGetPiecesLibrary();

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

				if (Notify->itemNew.lParam == NULL)
				{
					HTREEITEM CategoryItem = Notify->itemNew.hItem;
					CString CategoryName = m_PiecesTree.GetItemText(CategoryItem);

					if (CategoryName == "Models")
					{
						// List models.
						Project* project = lcGetActiveProject();
						bool Empty = true;

						for (int i = 0; i < project->m_ModelList.GetSize(); i++)
						{
							lcModel* Model = project->m_ModelList[i];

							if ((Model == project->m_ActiveModel) || (Model->IsSubModel(project->m_ActiveModel)))
								continue;

							Empty = false;

							m_PiecesTree.InsertItem(TVIF_PARAM|TVIF_TEXT, Model->m_Name, 0, 0, 0, 0, (LPARAM)Model, CategoryItem, TVI_LAST);
						}

						if (Empty)
							m_PiecesTree.InsertItem(TVIF_PARAM|TVIF_TEXT, "No Models", 0, 0, 0, 0, (LPARAM)NULL, CategoryItem, TVI_LAST);
					}
					else 
					{
						// Expanding a category item.
						int CategoryIndex = Lib->FindCategoryIndex((const char*)CategoryName);

						lcPtrArray<PieceInfo> SinglePieces, GroupedPieces;

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
									m_PiecesTree.InsertItem(TVIF_CHILDREN|TVIF_PARAM|TVIF_TEXT, Info->m_strDescription, 0, 0, 0, 0, (LPARAM)Info, CategoryItem, TVI_LAST);
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
				}
				else
				{
					PieceInfo* Parent = (PieceInfo*)Notify->itemNew.lParam;

					HTREEITEM CategoryItem = m_PiecesTree.GetParentItem(Notify->itemNew.hItem);
					CString CategoryName = m_PiecesTree.GetItemText(CategoryItem);
					int CategoryIndex = Lib->FindCategoryIndex((const char*)CategoryName);

					lcPtrArray<PieceInfo> Pieces;
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
							if (!Lib->PieceInCategory(Info, Lib->GetCategoryKeywords(CategoryIndex)))
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

	return CSizingControlBarG::OnNotify(wParam, lParam, pResult);
}
