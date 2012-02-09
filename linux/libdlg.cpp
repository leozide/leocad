// LibDlg.cpp : implementation file
//

#include <limits.h>
//#include "lc_global.h"
//#include "leocad.h"
#include "libdlg.h"
//#include "GroupDlg.h"
//#include "Print.h"
//#include "Tools.h"
#include "project.h"
#include "pieceinf.h"
#include "system.h"
#include "library.h"
#include "lc_application.h"

bool LibraryDialog::HandleCommand(int id) 
{
	switch (id)
	{
		case LC_LIBDLG_FILE_OPEN:
		{
			lcGetPiecesLibrary()->LoadCategories(NULL);
//			UpdateTree();
			return true;
		}

		case LC_LIBDLG_FILE_SAVE:
		{
			lcGetPiecesLibrary()->DoSaveCategories(false);
			return true;
		}

		case LC_LIBDLG_FILE_SAVEAS:
		{
			lcGetPiecesLibrary()->DoSaveCategories(true);
			return true;
		}

/*
		case LC_LIBDLG_FILE_PRINTCATALOG:
		{
			PRINT_PARAMS* param = (PRINT_PARAMS*)malloc(sizeof(PRINT_PARAMS));
			param->pParent = this;
			param->pMainFrame = (CFrameWnd*)AfxGetMainWnd();
			AfxBeginThread(PrintCatalogFunction, param);

			return true;
		}
*/

		case LC_LIBDLG_FILE_MERGEUPDATE:
		{
                        char filename[PATH_MAX];
			LC_FILEOPENDLG_OPTS opts;

			strcpy(opts.path, "");
			opts.type = LC_FILEOPENDLG_LUP;

			if (SystemDoDialog(LC_DLG_FILE_OPEN, filename))
			{
				lcGetPiecesLibrary()->LoadUpdate(filename);
//				UpdateTree();
			}

			return true;
		}

		case LC_LIBDLG_FILE_IMPORTPIECE:
		{
                        char filename[PATH_MAX];
			LC_FILEOPENDLG_OPTS opts;

			strcpy(opts.path, Sys_ProfileLoadString ("Default", "LDraw Pieces Path", ""));
			opts.type = LC_FILEOPENDLG_DAT;

			if (SystemDoDialog (LC_DLG_FILE_OPEN, filename))
			{
/*
				for (int i = 0; i < opts.numfiles; i++)
				{
					lcGetPiecesLibrary ()->ImportLDrawPiece (opts.filenames[i]);
					free (opts.filenames[i]);
				}
				free (opts.filenames);
*/
				FileDisk newbin, newidx, oldbin, oldidx;
				char file1[LC_MAXPATH], file2[LC_MAXPATH];

				strcpy(file1, lcGetPiecesLibrary()->GetLibraryPath());
				strcat(file1, "pieces-b.old");
				remove(file1);
				strcpy(file2, lcGetPiecesLibrary()->GetLibraryPath());
				strcat(file2, "pieces.bin");
				rename(file2, file1);

				if ((!oldbin.Open(file1, "rb")) || (!newbin.Open(file2, "wb")))
					break;

				strcpy(file1, lcGetPiecesLibrary()->GetLibraryPath());
				strcat(file1, "pieces-i.old");
				remove(file1);
				strcpy(file2, lcGetPiecesLibrary()->GetLibraryPath());
				strcat(file2, "pieces.idx");
				rename(file2, file1);

				if ((!oldidx.Open(file1, "rb")) || (!newidx.Open(file2, "wb")))
					break;

				if (!lcGetPiecesLibrary()->ImportLDrawPiece(filename, &newidx, &newbin, &oldidx, &oldbin))
					break;

//				Sys_ProfileSaveString ("Default", "LDraw Pieces Path", filename);

//				UpdateList();
			}

			return true;
		}

/*
		case LC_LIBDLG_FILE_TEXTURES:
		{
			CTexturesDlg dlg;
			dlg.DoModal();
		} break;
*/

		case LC_LIBDLG_CATEGORY_RESET:
		{
			if (SystemDoMessageBox("Are you sure you want to reset the categories?", LC_MB_YESNO | LC_MB_ICONQUESTION) == LC_YES)
			{
				lcGetPiecesLibrary()->ResetCategories();

//				UpdateList();
//				UpdateTree();
			}

			return true;
		}

		case LC_LIBDLG_CATEGORY_NEW:
		{
			LC_CATEGORYDLG_OPTS Opts;
			Opts.Name = "New Category";
			Opts.Keywords = "";

			if (SystemDoDialog(LC_DLG_EDITCATEGORY, &Opts))
			{
				lcGetPiecesLibrary()->AddCategory(Opts.Name, Opts.Keywords);
			}

//			UpdateTree();

			return true;
		}

		case LC_LIBDLG_CATEGORY_REMOVE:
		{
/*
			HTREEITEM Item = m_Tree.GetSelectedItem();

			if (Item == NULL)
				break;

			PiecesLibrary* Lib = lcGetPiecesLibrary();
			CString CategoryName = m_Tree.GetItemText(Item);
			int Index = Lib->FindCategoryIndex((const char*)CategoryName);

			if (Index == -1)
				break;

			char Msg[1024];
			String Name = Lib->GetCategoryName(Index);
			sprintf(Msg, "Are you sure you want to remove the %s category?", Name);

			if (SystemDoMessageBox(Msg, LC_MB_YESNO | LC_MB_ICONQUESTION) == LC_YES)
			{
				Lib->RemoveCategory(Index);
			}

			UpdateTree();
*/
			return true;
		}

		case LC_LIBDLG_CATEGORY_EDIT:
		{
/*
			HTREEITEM Item = m_Tree.GetSelectedItem();

			if (Item == NULL)
				break;

			PiecesLibrary* Lib = lcGetPiecesLibrary();
			CString CategoryName = m_Tree.GetItemText(Item);
			int Index = Lib->FindCategoryIndex((const char*)CategoryName);

			if (Index == -1)
				break;

			LC_CATEGORYDLG_OPTS Opts;
			Opts.Name = Lib->GetCategoryName(Index);
			Opts.Keywords = Lib->GetCategoryKeywords(Index);

			if (SystemDoDialog(LC_DLG_EDITCATEGORY, &Opts))
			{
				String OldName = Lib->GetCategoryName(Index);
				Lib->SetCategory(Index, Opts.Name, Opts.Keywords);
			}

			UpdateTree();

*/
			return true;
		}

		case LC_LIBDLG_PIECE_NEW:
		{
			return true;
		}

		case LC_LIBDLG_PIECE_EDIT:
		{
			return true;
		}

		case LC_LIBDLG_PIECE_DELETE:
		{
/*
			PtrArray<PieceInfo> Pieces;

			for (int i = 0; i < m_List.GetItemCount(); i++)
			{
				if (m_List.GetItemState(i, LVIS_SELECTED))
					Pieces.Add((PieceInfo*)m_List.GetItemData(i));
			}

			if (Pieces.GetSize() == 0)
				return true;

			if (SystemDoMessageBox ("Are you sure you want to permanently delete the selected pieces?", LC_MB_YESNO|LC_MB_ICONQUESTION) != LC_YES)
				return true;

			lcGetPiecesLibrary()->DeletePieces(Pieces);

			UpdateList();
*/
			return true;
		}
	}

//	return CDialog::OnCommand(wParam, lParam);
        return true;
}

/*
void CLibraryDlg::UpdateList()
{
	m_List.DeleteAllItems();
	m_List.SetRedraw(false);

	PiecesLibrary *Lib = lcGetPiecesLibrary();

	HTREEITEM CategoryItem = m_Tree.GetSelectedItem();
	CString CategoryName = m_Tree.GetItemText(CategoryItem);
	int CategoryIndex = Lib->FindCategoryIndex((const char*)CategoryName);

	if (CategoryIndex != -1)
	{
		PtrArray<PieceInfo> SinglePieces, GroupedPieces;

		Lib->GetCategoryEntries(CategoryIndex, false, SinglePieces, GroupedPieces);

		for (int i = 0; i < SinglePieces.GetSize(); i++)
		{
			PieceInfo* Info = SinglePieces[i];

			LVITEM lvi;
			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			lvi.iItem = 0;
			lvi.iSubItem = 0;
			lvi.lParam = (LPARAM)Info;
			lvi.pszText = Info->m_strDescription;
			int idx = m_List.InsertItem(&lvi);

			m_List.SetItemText(idx, 1, Info->m_strName);
		}
	}
	else
	{
		if (CategoryName == "Unassigned")
		{
			// Test each piece against all categories.
			for (int i = 0; i < Lib->GetPieceCount(); i++)
			{
				PieceInfo* Info = Lib->GetPieceInfo(i);

				for (int j = 0; j < Lib->GetNumCategories(); j++)
				{
					if (Lib->PieceInCategory(Info, Lib->GetCategoryKeywords(j)))
						break;
				}

				if (j == Lib->GetNumCategories())
				{
					LVITEM lvi;
					lvi.mask = LVIF_TEXT | LVIF_PARAM;
					lvi.iItem = 0;
					lvi.iSubItem = 0;
					lvi.lParam = (LPARAM)Info;
					lvi.pszText = Info->m_strDescription;
					int idx = m_List.InsertItem(&lvi);

					m_List.SetItemText(idx, 1, Info->m_strName);
				}
			}
		}
		else if (CategoryName == "Pieces")
		{
			for (int i = 0; i < Lib->GetPieceCount(); i++)
			{
				PieceInfo* Info = Lib->GetPieceInfo(i);

				LVITEM lvi;
				lvi.mask = LVIF_TEXT | LVIF_PARAM;
				lvi.iItem = 0;
				lvi.iSubItem = 0;
				lvi.lParam = (LPARAM)Info;
				lvi.pszText = Info->m_strDescription;
				int idx = m_List.InsertItem(&lvi);

				m_List.SetItemText(idx, 1, Info->m_strName);
			}
		}
	}

	m_List.SortItems((PFNLVCOMPARE)ListCompare, m_SortColumn);
	m_List.SetRedraw(true);
}

void CLibraryDlg::UpdateTree()
{
	m_Tree.SetRedraw(false);
	m_Tree.DeleteAllItems();

	HTREEITEM Root = m_Tree.InsertItem(TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_TEXT, "Pieces", 0, 1, 0, 0, 0, TVI_ROOT, TVI_SORT);

	PiecesLibrary *Lib = lcGetPiecesLibrary();
	for (int i = 0; i < Lib->GetNumCategories(); i++)
		m_Tree.InsertItem(TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_TEXT, Lib->GetCategoryName(i), 0, 1, 0, 0, 0, Root, TVI_SORT);

	m_Tree.InsertItem(TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_TEXT, "Unassigned", 0, 1, 0, 0, 0, Root, TVI_LAST);

	m_Tree.Expand(Root, TVE_EXPAND);
	m_Tree.SetRedraw(true);
	m_Tree.Invalidate();
}

void CLibraryDlg::OnSelChangedTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	UpdateList();
	*pResult = 0;
}

void CLibraryDlg::OnCancel() 
{
	// Check if it's ok to close the dialog
	if (!lcGetPiecesLibrary()->SaveCategories())
		return;

	CDialog::OnCancel();
}

void CLibraryDlg::OnOK() 
{
	// Check if it's ok to close the dialog
	if (!lcGetPiecesLibrary()->SaveCategories())
		return;

	CDialog::OnOK();
}

BOOL CLibraryDlg::ContinueModal()
{
	HTREEITEM h = m_Tree.GetSelectedItem();
	BOOL bValid = (h != m_Tree.GetRootItem()) && (h != NULL);

	EnableControl(LC_LIBDLG_GROUP_RENAME, bValid);
	EnableControl(LC_LIBDLG_GROUP_DELETE, bValid);

	return CDialog::ContinueModal();
}

void CLibraryDlg::EnableControl(UINT nID, BOOL bEnable)
{
	GetMenu()->GetSubMenu(1)->EnableMenuItem(nID, MF_BYCOMMAND | (bEnable ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	int state = m_wndToolBar.GetToolBarCtrl().GetState(nID) & ~TBSTATE_ENABLED;
	if (bEnable)
		state |= TBSTATE_ENABLED;
	m_wndToolBar.GetToolBarCtrl().SetState(nID, state);
}

BOOL CLibraryDlg::OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult)
{
	ASSERT(pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW);

	// allow top level routing frame to handle the message
	if (GetRoutingFrame() != NULL)
		return false;
	
	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	CString cstTipText;
	UINT nID = pNMHDR->idFrom;

	if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
		pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
	{
		// idFrom is actually the HWND of the tool
		nID = ((UINT)(WORD)::GetDlgCtrlID((HWND)nID));
	}
	
	if (nID != 0) // will be zero on a separator
	{
		cstTipText.LoadString(nID);
	}
	
	// Non-UNICODE Strings only are shown in the tooltip window...
	if (pNMHDR->code == TTN_NEEDTEXTA)
		lstrcpyn(pTTTA->szText, cstTipText, (sizeof(pTTTA->szText)/sizeof(pTTTA->szText[0])));
	else
		_mbstowcsz(pTTTW->szText, cstTipText, (sizeof(pTTTW->szText)/sizeof(pTTTW->szText[0])));
	
	*pResult = 0;
	
	// bring the tooltip window above other popup windows
	::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
		SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE);
	
	return true;    // message was handled
}

void CLibraryDlg::OnListColumnClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	// Save the column index.
	m_SortColumn = pNMListView->iSubItem;

	m_List.SortItems((PFNLVCOMPARE)ListCompare, m_SortColumn);

	*pResult = 0;
}
*/
