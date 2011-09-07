// LibDlg.cpp : implementation file
//

#include "stdafx.h"
#include "leocad.h"
#include "LibDlg.h"
#include "GroupDlg.h"
#include "Print.h"
#include "Tools.h"
#include "texdlg.h"
#include "ProgDlg.h"
#include "project.h"
#include "pieceinf.h"
#include "globals.h"
#include "system.h"
#include "library.h"
#include "lc_application.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Function to sort the list control.
static int CALLBACK ListCompare(LPARAM lP1, LPARAM lP2, LPARAM lParamSort)
{
	int ret;

	if ((lP1 < 0) || (lP2 < 0))
		return 0;

	if ((lParamSort & ~0xF0) == 0)
		ret = strcmpi(((PieceInfo*)lP1)->m_strDescription, ((PieceInfo*)lP2)->m_strDescription);
	else
		ret = strcmpi(((PieceInfo*)lP1)->m_strName, ((PieceInfo*)lP2)->m_strName);

	return ret;
}

/////////////////////////////////////////////////////////////////////////////
// CLibraryDlg dialog

CLibraryDlg::CLibraryDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLibraryDlg::IDD, pParent)
{
	m_SortColumn = 0;

	//{{AFX_DATA_INIT(CLibraryDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CLibraryDlg::~CLibraryDlg()
{
}

void CLibraryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLibraryDlg)
	DDX_Control(pDX, IDC_LIBDLG_TREE, m_Tree);
	DDX_Control(pDX, IDC_LIBDLG_LIST, m_List);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLibraryDlg, CDialog)
	//{{AFX_MSG_MAP(CLibraryDlg)
	ON_NOTIFY(TVN_SELCHANGED, IDC_LIBDLG_TREE, OnSelChangedTree)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIBDLG_LIST, OnListColumnClick)
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLibraryDlg message handlers

BOOL CLibraryDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// Add the ToolBar.
	if (!m_wndToolBar.Create(this) || !m_wndToolBar.LoadToolBar(IDR_LIBRARY))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);

	// We need to resize the dialog to make room for control bars.
	// First, figure out how big the control bars are.
	CRect rcClientStart;
	CRect rcClientNow;
	GetClientRect(rcClientStart);
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0, reposQuery, rcClientNow);

	// Now move all the controls so they are in the same relative
	// position within the remaining client area as they would be
	// with no control bars.
	CPoint ptOffset(rcClientNow.left - rcClientStart.left, rcClientNow.top - rcClientStart.top); 

	CRect  rcChild;
	CWnd* pwndChild = GetWindow(GW_CHILD);
	while (pwndChild)
	{
		pwndChild->GetWindowRect(rcChild);
		ScreenToClient(rcChild);
		rcChild.OffsetRect(ptOffset);
		pwndChild->MoveWindow(rcChild, FALSE);
		pwndChild = pwndChild->GetNextWindow();
	}

	// Adjust the dialog window dimensions
	CRect rcWindow;
	GetWindowRect(rcWindow);
	rcWindow.right += rcClientStart.Width() - rcClientNow.Width();
	rcWindow.bottom += rcClientStart.Height() - rcClientNow.Height();
	MoveWindow(rcWindow, FALSE);

	// And position the control bars
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);

	m_TreeImages.Create(IDB_PARTICONS, 16, 0, RGB (0,128,128));
	m_Tree.SetImageList(&m_TreeImages, TVSIL_NORMAL);

	RECT rect;
	m_List.GetWindowRect(&rect);
	m_List.InsertColumn(0, "Name", LVCFMT_LEFT, rect.right - rect.left - GetSystemMetrics(SM_CXVSCROLL) - 4 - 60, 0);
	m_List.InsertColumn(1, "Number", LVCFMT_LEFT, 60, 1);

	UpdateList();
	UpdateTree();

	return TRUE;
}

BOOL CLibraryDlg::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	switch (LOWORD(wParam))
	{
		case ID_LIBDLG_FILE_OPEN:
		{
			lcGetPiecesLibrary()->LoadCategories(NULL);
			UpdateTree();
			return TRUE;
		}

		case ID_LIBDLG_FILE_SAVE:
		{
			lcGetPiecesLibrary()->DoSaveCategories(false);
			return TRUE;
		}

		case ID_LIBDLG_FILE_SAVEAS:
		{
			lcGetPiecesLibrary()->DoSaveCategories(true);
			return TRUE;
		}

		case ID_LIBDLG_FILE_PRINTCATALOG:
		{
			PRINT_PARAMS* param = (PRINT_PARAMS*)malloc(sizeof(PRINT_PARAMS));
			param->pParent = this;
			param->pMainFrame = (CFrameWnd*)AfxGetMainWnd();
			AfxBeginThread(PrintCatalogFunction, param);

			return TRUE;
		}

		case ID_LIBDLG_FILE_MERGEUPDATE:
		{
			LC_FILEOPENDLG_OPTS opts;

			strcpy(opts.path, "");
			opts.type = LC_FILEOPENDLG_LUP;

			if (SystemDoDialog(LC_DLG_FILE_OPEN, &opts))
			{
				lcGetPiecesLibrary()->LoadUpdate((char*)opts.filenames);

				free(opts.filenames);

				UpdateTree();
			}

			return TRUE;
		}

		case ID_FILE_IMPORTPIECE:
		{
			LC_FILEOPENDLG_OPTS opts;

			strcpy(opts.path, Sys_ProfileLoadString ("Default", "LDraw Pieces Path", ""));
			opts.type = LC_FILEOPENDLG_DAT;

			if (SystemDoDialog (LC_DLG_FILE_OPEN, &opts))
			{
				for (int i = 0; i < opts.numfiles; i++)
				{
					lcGetPiecesLibrary ()->ImportLDrawPiece (opts.filenames[i]);
					free (opts.filenames[i]);
				}

				free (opts.filenames);
				Sys_ProfileSaveString ("Default", "LDraw Pieces Path", opts.path);

				UpdateList();
			}

			return TRUE;
		}

		case ID_LIBDLG_FILE_TEXTURES:
		{
			CTexturesDlg dlg;
			dlg.DoModal();
		} break;

		case ID_LIBDLG_CATEGORY_RESET:
		{
			if (SystemDoMessageBox("Are you sure you want to reset the categories?", LC_MB_YESNO | LC_MB_ICONQUESTION) == LC_YES)
			{
				lcGetPiecesLibrary()->ResetCategories();

				UpdateList();
				UpdateTree();
			}

			return TRUE;
		}

		case ID_LIBDLG_CATEGORY_NEW:
		{
			LC_CATEGORYDLG_OPTS Opts;
			Opts.Name = "New Category";
			Opts.Keywords = "";

			if (SystemDoDialog(LC_DLG_EDITCATEGORY, &Opts))
			{
				lcGetPiecesLibrary()->AddCategory(Opts.Name, Opts.Keywords);
			}

			UpdateTree();

			return TRUE;
		}

		case ID_LIBDLG_CATEGORY_REMOVE:
		{
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

			return TRUE;
		}

		case ID_LIBDLG_CATEGORY_EDIT:
		{
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

			return TRUE;
		}

		case ID_LIBDLG_PIECE_NEW:
		{
			return TRUE;
		}

		case ID_LIBDLG_PIECE_EDIT:
		{
			return TRUE;
		}

		case ID_LIBDLG_PIECE_DELETE:
		{
			PtrArray<PieceInfo> Pieces;

			for (int i = 0; i < m_List.GetItemCount(); i++)
			{
				if (m_List.GetItemState(i, LVIS_SELECTED))
					Pieces.Add((PieceInfo*)m_List.GetItemData(i));
			}

			if (Pieces.GetSize() == 0)
				return TRUE;

			if (SystemDoMessageBox ("Are you sure you want to permanently delete the selected pieces?", LC_MB_YESNO|LC_MB_ICONQUESTION) != LC_YES)
				return TRUE;

			lcGetPiecesLibrary()->DeletePieces(Pieces);

			UpdateList();

			return TRUE;
		}
	}

	return CDialog::OnCommand(wParam, lParam);
}

void CLibraryDlg::UpdateList()
{
	m_List.DeleteAllItems();
	m_List.SetRedraw(FALSE);

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
				int j;

				for (j = 0; j < Lib->GetNumCategories(); j++)
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
	m_List.SetRedraw(TRUE);
}

void CLibraryDlg::UpdateTree()
{
	m_Tree.SetRedraw(FALSE);
	m_Tree.DeleteAllItems();

	HTREEITEM Root = m_Tree.InsertItem(TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_TEXT, "Pieces", 0, 1, 0, 0, 0, TVI_ROOT, TVI_SORT);

	PiecesLibrary *Lib = lcGetPiecesLibrary();
	for (int i = 0; i < Lib->GetNumCategories(); i++)
		m_Tree.InsertItem(TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_TEXT, Lib->GetCategoryName(i), 0, 1, 0, 0, 0, Root, TVI_SORT);

	m_Tree.InsertItem(TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_TEXT, "Unassigned", 0, 1, 0, 0, 0, Root, TVI_LAST);

	m_Tree.Expand(Root, TVE_EXPAND);
	m_Tree.SetRedraw(TRUE);
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

	EnableControl(ID_LIBDLG_GROUP_RENAME, bValid);
	EnableControl(ID_LIBDLG_GROUP_DELETE, bValid);

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
		return FALSE;
	
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
	
	return TRUE;    // message was handled
}

void CLibraryDlg::OnListColumnClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	// Save the column index.
	m_SortColumn = pNMListView->iSubItem;

	m_List.SortItems((PFNLVCOMPARE)ListCompare, m_SortColumn);

	*pResult = 0;
}
