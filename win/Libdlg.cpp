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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const char ver_str[32] = "LeoCAD Group Configuration File";
static const float ver_flt = 0.3f;

/////////////////////////////////////////////////////////////////////////////
// CLibraryDlg dialog


CLibraryDlg::CLibraryDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLibraryDlg::IDD, pParent)
{
	m_pDragImage = NULL;
	m_bDragging = FALSE;

	//{{AFX_DATA_INIT(CLibraryDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CLibraryDlg::~CLibraryDlg()
{
	delete m_pDragImage;
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
	ON_NOTIFY(LVN_BEGINDRAG, IDC_LIBDLG_LIST, OnBeginDragList)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
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
	if (!m_wndToolBar.Create(this) ||
		!m_wndToolBar.LoadToolBar(IDR_LIBRARY))
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

	UpdateList();
	UpdateTree();

	return TRUE;
}

BOOL CLibraryDlg::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	switch (LOWORD(wParam))
	{
		case ID_LIBDLG_FILE_RESET:
		{
			m_Manager.HandleCommand (LC_LIBDLG_FILE_RESET, 0);

			m_ImageList.DeleteImageList();
			m_ImageList.Create(IDB_PIECEBAR, 16, 0, 0x00ff00ff);

			for (int i = 0; i < 32; i++)
				m_nBitmaps[i] = min(i,9);

			UpdateList();
			UpdateTree();

			return TRUE;
		}

		case ID_LIBDLG_FILE_OPEN:
		{
/*
			CFileDialog dlg(TRUE, ".lgf\0", NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
				"LeoCAD Group Files (*.lgf)|*.lgf|All Files (*.*)|*.*||",this);

			if (dlg.DoModal() == IDOK)
			{
				m_ImageList.DeleteImageList();
				m_strFile = dlg.GetPathName();
				CWaitCursor wait;
				CFile f(m_strFile, CFile::modeRead|CFile::shareDenyWrite);
				if (f.GetLength() != 0)
				{
					CArchive ar(&f, CArchive::load | CArchive::bNoFlushOnDelete);
					for (int i = 0; i < m_Parts.GetSize(); i++)
						m_Parts[i].group = m_Parts[i].defgroup;
			
					char tmp[32];
					float ver;
					CString str;
					int n;
					ar.Read (tmp, sizeof(tmp));
					ar >> ver;
					if (ver == 0.1f)
						ar >> i;
					else
					{
						memset(m_strGroups, 0, sizeof(m_strGroups));
						ar >> m_nMaxGroups;
						for (i = 0; i < m_nMaxGroups; i++)
						{
							ar.Read (m_strGroups[i], sizeof(m_strGroups[i]));

							if (ver > 0.2f)
								ar >> m_nBitmaps[i];
						}
					}

					if (ver > 0.2f)
						m_ImageList.Read(&ar);

					ar >> n;
					for (i = 0; i < n; i++)
					{
						char name[9], description[65];
						unsigned long group;

						ar.Read (name, sizeof(name));
						if (ver == 0.1f)
						{
							BYTE b;
							ar.Read (description, sizeof(description));
							ar >> b;
							group = 1 << b;
						}
						else
							ar >> group;
				
						for (int j = 0; j < m_Parts.GetSize(); j++)
							if (strcmp (m_Parts[j].info->m_strName, name) == 0)
							{
								m_Parts[j].group = group;
								break;
							}
					}
					ar.Close();
				}
				f.Close();

				if (m_ImageList.GetImageCount() == 0)
					m_ImageList.Create(IDB_PIECEBAR, 16, 0, 0x00ff00ff);

				m_bModified = FALSE;
				UpdateList();
				UpdateTree();
			}
*/
			return TRUE;
		}

		case ID_LIBDLG_FILE_SAVE:
		{
			m_Manager.DoSave(false);
			return TRUE;
		}

		case ID_LIBDLG_FILE_SAVEAS:
		{
			m_Manager.DoSave(true);
			return TRUE;
		}

		case ID_LIBDLG_FILE_LOADBITMAP:
		{

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
			m_Manager.HandleCommand (LC_LIBDLG_FILE_MERGEUPDATE, 0);
			UpdateList();

			return TRUE;
		}

		case ID_FILE_IMPORTPIECE:
		{
			CString filename;
			LC_LDRAW_PIECE piece;

			CFileDialog dlg(TRUE, ".dat\0", NULL,OFN_ALLOWMULTISELECT | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
				"LDraw Files (*.dat)|*.dat|All Files (*.*)|*.*||",this);
			dlg.m_ofn.lpstrFile = filename.GetBuffer(_MAX_PATH);
      dlg.m_ofn.nMaxFile = _MAX_PATH;

      if (dlg.DoModal() != IDOK)
				return TRUE;

      POSITION pos = dlg.GetStartPosition ();

      while (pos != NULL)
      {
        CString str = dlg.GetNextPathName (pos);

  			SystemDoWaitCursor(1);

	  		if (ReadLDrawPiece(str, &piece))
		  	{
			  	if (project->GetPiecesLibrary ()->FindPieceInfo (piece.name) != NULL)
				  	AfxMessageBox("Piece already exists in the library !", MB_OK|MB_ICONINFORMATION);

  				if (SaveLDrawPiece(&piece))
	  				AfxMessageBox("Piece successfully imported.", MB_OK|MB_ICONINFORMATION);
		  		else
			  		AfxMessageBox("Error saving library.", MB_OK|MB_ICONINFORMATION);
  			}
	  		else
		  		AfxMessageBox("Error reading file", MB_OK|MB_ICONINFORMATION);

  			SystemDoWaitCursor(-1);
	  		FreeLDrawPiece(&piece);
      }
// update m_Parts
			return TRUE;
		}

		case ID_LIBDLG_FILE_TEXTURES:
		{
			CTexturesDlg dlg;
			dlg.DoModal();
		} break;

		case ID_LIBDLG_GROUP_INSERT:
		{
/*
			HTREEITEM hti = m_Tree.GetSelectedItem();
			
			if (hti)
			{
				DWORD dw = m_Tree.GetItemData(hti);
				if (dw == 0)
					dw = 1;
				dw--;

				CGroupDlg dlg(this);
				if (dlg.DoModal() == IDOK)
				{
					CWaitCursor wc;
					m_bModified = TRUE;
						
					for (i = m_nMaxGroups; i > (int)dw; i--)
					{
						strcpy (m_strGroups[i], m_strGroups[i-1]);
						m_nBitmaps[i] = m_nBitmaps[i-1];
					}
					
					strcpy (m_strGroups[i], dlg.m_strName);
					m_nBitmaps[i] = 9;
					
					for (int j = 0; j < m_Parts.GetSize(); j++)
					{
						DWORD grp = m_Parts[j].group;
						
						for (i = m_nMaxGroups; i >= (int)dw; i--)
						{
							DWORD d = (1 << i);
							if (grp & d)
							{
								grp &= ~d;
								grp |= (1 << (i+1));
							}
						}
						m_Parts[j].group = grp;
					}
					m_nMaxGroups++;
					UpdateTree();
					UpdateList();
				}
			}
*/
			return TRUE;
		}
	
		case ID_LIBDLG_GROUP_DELETE:
		{
			HTREEITEM hti = m_Tree.GetSelectedItem();

			if (hti)
			{
				DWORD dw = m_Tree.GetItemData(hti);
				if (dw == 0)
					return TRUE;
				dw--;

				CWaitCursor wc;

				for (int i = dw; i < m_Manager.GetGroupCount(); i++)
					m_nBitmaps[i] = m_nBitmaps[i+1];

				m_Manager.HandleCommand (LC_LIBDLG_GROUP_DELETE, dw);

				UpdateTree();
				UpdateList();
			}

			return TRUE;
		}

		case ID_LIBDLG_GROUP_RENAME:
		{
/*
			HTREEITEM hti = m_Tree.GetSelectedItem();

			if (hti)
			{
				DWORD dw = m_Tree.GetItemData(hti);
				if (dw == 0)
					return TRUE;
				dw--;

				CGroupDlg dlg(this);
				if (dlg.DoModal() == IDOK)
				{
					strcpy (m_strGroups[i], dlg.m_strName);
					UpdateTree();
					m_bModified = TRUE;
				}
			}
*/
			return TRUE;
		}

		case ID_LIBDLG_GROUP_MOVEUP:
		{
			HTREEITEM hti = m_Tree.GetSelectedItem();

			if (hti)
			{
				DWORD dw = m_Tree.GetItemData(hti);
				int j;

				if (dw == 0)
					return TRUE;
				dw--;

				CWaitCursor wc;

				m_Manager.HandleCommand (LC_LIBDLG_GROUP_MOVEUP, dw);

				j = m_nBitmaps[dw];
				m_nBitmaps[dw] = m_nBitmaps[dw-1];
				m_nBitmaps[dw-1] = j;

				UpdateTree();
				UpdateList();
			}
			return TRUE;
		}

		case ID_LIBDLG_GROUP_MOVEDOWN:
		{
			HTREEITEM hti = m_Tree.GetSelectedItem();

			if (hti)
			{
				DWORD dw = m_Tree.GetItemData(hti);
				int j;

				if (dw == 0)
					return TRUE;
				dw--;

				CWaitCursor wc;

				m_Manager.HandleCommand (LC_LIBDLG_GROUP_MOVEDOWN, dw);

				j = m_nBitmaps[dw];
				m_nBitmaps[dw] = m_nBitmaps[dw+1];
				m_nBitmaps[dw+1] = j;

				UpdateTree();
				UpdateList();
			}
			return TRUE;
		}

		case ID_LIBDLG_PIECE_NEW:
		{
//				CPieceEditorDlg dlg;
//				dlg.DoModal();

			return TRUE;
		}

		case ID_LIBDLG_PIECE_EDIT:
		{

			return TRUE;
		}

		case ID_LIBDLG_PIECE_DELETE:
		{
			int i, sel = 0;

			for (i = 0; i < m_List.GetItemCount(); i++)
				if (m_List.GetItemState(i, LVIS_SELECTED))
					sel++;

			// Nothing to be done
			if (sel == 0)
				return TRUE;

			char** names = (char**)malloc(sel*sizeof(char**));

			for (sel = 0, i = 0; i < m_List.GetItemCount(); i++)
			{
				PieceInfo* info;
				lcuint32 group;

				if (m_List.GetItemState(i, LVIS_SELECTED))
				{
					m_Manager.GetPieceInfo (m_List.GetItemData(i), &info, &group);

					names[sel] = info->m_strName;
					sel++;
				}
			}

			m_Manager.DeletePieces (names, sel);
			free(names);

			UpdateList();

			return TRUE;
		}
	}

	return CDialog::OnCommand(wParam, lParam);
}

void CLibraryDlg::UpdateList()
{
	HTREEITEM hti = m_Tree.GetSelectedItem();
	m_List.DeleteAllItems();

	m_List.SetRedraw(FALSE);
	if (hti)
	{
		DWORD dw = m_Tree.GetItemData (hti);

		for (int i = 0; i < m_Manager.GetPieceCount(); i++)
		{
			PieceInfo* info;
			lcuint32 group;

			m_Manager.GetPieceInfo (i, &info, &group);

			if ((dw != 0) && (((1 << (dw-1)) & group) == 0))
				continue;

			LVITEM lvi;
			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			lvi.iItem = 0;
			lvi.iSubItem = 0;
			lvi.lParam = i;
			lvi.pszText = info->m_strDescription;
			m_List.InsertItem(&lvi);
		}
	}
	m_List.SetRedraw(TRUE);
}

void CLibraryDlg::UpdateTree()
{
	m_Tree.DeleteAllItems();

	TV_INSERTSTRUCT tvs;
	tvs.hParent = NULL;
	tvs.hInsertAfter = TVI_LAST;
	tvs.item.iImage = 1;
	tvs.item.iSelectedImage = 1;
	tvs.item.lParam = 0;
	tvs.item.pszText = _T("Groups");
	tvs.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM;
	HTREEITEM hRootItem = m_Tree.InsertItem(&tvs);

	for (int i = 0; i < m_Manager.GetGroupCount(); i++)
	{
		TV_INSERTSTRUCT tvstruct;
		tvstruct.hParent = hRootItem;
		tvstruct.hInsertAfter = TVI_LAST;
		tvstruct.item.iImage = 0;
		tvstruct.item.iSelectedImage = 1;
		tvstruct.item.lParam = i+1;
		tvstruct.item.pszText = m_Manager.GetGroupName(i);
		tvstruct.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM;
		m_Tree.InsertItem(&tvstruct);
	}
	m_Tree.Expand (hRootItem, TVE_EXPAND);
	m_Tree.SelectItem(hRootItem);
}
/*
BOOL CLibraryDlg::DoSave(BOOL bAskName)
{
	if (bAskName || m_strFile.IsEmpty())
	{
		CString Name;
		CFileDialog dlg(FALSE, ".lgf\0", NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
			"LeoCAD Group Files (*.lgf)|*.lgf|All Files (*.*)|*.*||",this);
		dlg.m_ofn.lpstrFile = Name.GetBuffer(_MAX_PATH);
		if (dlg.DoModal() != IDOK)
			return FALSE;
		Name.ReleaseBuffer();
		m_strFile = Name;
	}

	CWaitCursor wait;
	CFile f(m_strFile, CFile::modeCreate | CFile::modeReadWrite | CFile::shareExclusive);
	CArchive ar(&f,CArchive::store | CArchive::bNoFlushOnDelete);

	CString str;
	ar.Write (ver_str, sizeof(ver_str));
	ar << ver_flt;
	ar << m_nMaxGroups;
	for (int i = 0; i < m_nMaxGroups; i++)
	{
		ar.Write (m_strGroups[i], sizeof(m_strGroups[i]));
		ar << m_nBitmaps[i];
	}

	m_ImageList.Write(&ar);

	ar << (int) m_Parts.GetSize();
	for (i = 0; i < m_Parts.GetSize(); i++)
	{
		ar.Write (m_Parts[i].info->m_strName, sizeof(m_Parts[i].info->m_strName));
		ar << m_Parts[i].group;
	}
	ar.Close();
	f.Close();
	m_bModified = FALSE;
//		m_bLoaded = TRUE;
	return TRUE;
}
*/
void CLibraryDlg::OnSelChangedTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	UpdateList();
	*pResult = 0;
}

void CLibraryDlg::OnBeginDragList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	// save the index of the item being dragged in m_nDragIndex
	m_nDragIndex = pNMListView->iItem;
	POINT pt;
	pt.x = 8;
	pt.y = 8;

	// create a drag image
	if(m_pDragImage)
		delete m_pDragImage;
	
	m_pDragImage = m_List.CreateDragImage (m_nDragIndex, &pt);
	ASSERT (m_pDragImage);
	// changes the cursor to the drag image (DragMove() is still required in 
	// OnMouseMove())
	VERIFY (m_pDragImage->BeginDrag (0, CPoint (8, 8)));
	VERIFY (m_pDragImage->DragEnter (GetDesktopWindow (), pNMListView->ptAction));
	// set dragging flag
	m_bDragging = TRUE;
	m_hDropItem = NULL;

	// capture all mouse messages
	SetCapture ();
	
	*pResult = 0;
}

void CLibraryDlg::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (m_bDragging)
	{
		CPoint pt (point);
		ClientToScreen (&pt);
		// move the drag image and unlock window updates
		VERIFY (m_pDragImage->DragMove (pt));
		VERIFY (m_pDragImage->DragShowNolock (FALSE));

		m_Tree.ScreenToClient (&pt);
	
		UINT uFlags;
		// get the item that is below cursor and highlight it
		m_hDropItem = m_Tree.HitTest(pt, &uFlags);
		m_Tree.SelectDropTarget(m_hDropItem);

		// lock window updates
		VERIFY (m_pDragImage->DragShowNolock (TRUE));
	}
	
	CDialog::OnMouseMove(nFlags, point);
}

void CLibraryDlg::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if (m_bDragging)
	{
		// release mouse capture
		VERIFY (::ReleaseCapture ());
		m_bDragging = FALSE;
		// end dragging
		VERIFY (m_pDragImage->DragLeave (GetDesktopWindow ()));
		m_pDragImage->EndDrag ();

		if (m_hDropItem)
		{
			DWORD dw = m_Tree.GetItemData(m_hDropItem);
			bool bControl = (GetKeyState (VK_CONTROL) < 0);

			for (int i = 0; i < m_List.GetItemCount(); i++)
				if (m_List.GetItemState(i,LVIS_SELECTED))
					m_Manager.SetPieceGroup(m_List.GetItemData(i), dw, bControl);

			m_Tree.SelectDropTarget (NULL);
			m_hDropItem = NULL;
			UpdateList();
		}
	}
	
	CDialog::OnLButtonUp(nFlags, point);
}

void CLibraryDlg::OnCancel() 
{
	// Check if it's ok to close the dialog
	if (!m_Manager.SaveModified ())
		return;

	CDialog::OnCancel();
}

void CLibraryDlg::OnOK() 
{
	// Check if it's ok to close the dialog
	if (!m_Manager.SaveModified ())
		return;
	
	CDialog::OnOK();
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

BOOL CLibraryDlg::ContinueModal()
{
	HTREEITEM h = m_Tree.GetSelectedItem();
	DWORD dw = m_Tree.GetItemData (h);
	BOOL bValid = (h != m_Tree.GetRootItem()) && (h != NULL);

	EnableControl(ID_LIBDLG_GROUP_INSERT, m_Manager.GetGroupCount() < 32);
	EnableControl(ID_LIBDLG_GROUP_RENAME, bValid);
	EnableControl(ID_LIBDLG_GROUP_DELETE, bValid && m_Manager.GetGroupCount() != 1);
	EnableControl(ID_LIBDLG_GROUP_MOVEUP, dw > 1);
	EnableControl(ID_LIBDLG_GROUP_MOVEDOWN, (dw != 0) && ((int)dw != m_Manager.GetGroupCount()));

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

/*
CImageList* CTreeCtrlEx::CreateDragImageEx(HTREEITEM hItem)
{
	CRect rect;
	GetItemRect(hItem, rect, LVIR_LABEL);
	rect.top = rect.left = 0;
	
	// Create bitmap
	CClientDC dc(this);
	CDC memDC;

	if(!memDC.CreateCompatibleDC(&dc))
		return NULL;
	
	CBitmap bitmap;
	if(!bitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height()))
		return NULL;
	
	CBitmap* pOldMemDCBitmap = memDC.SelectObject(&bitmap);
	CFont* pOldFont = memDC.SelectObject(GetFont());
	memDC.FillSolidRect(&rect, RGB(0, 255, 0)); // Here green is used as mask color	
	memDC.SetTextColor(GetSysColor(COLOR_GRAYTEXT));
	memDC.TextOut(rect.left, rect.top, GetItemText(hItem));
	memDC.SelectObject(pOldFont);
	memDC.SelectObject(pOldMemDCBitmap);

	// Create imagelist
	CImageList* pImageList = new CImageList;
	pImageList->Create(rect.Width(), rect.Height(),
		ILC_COLOR | ILC_MASK, 0, 1);
	pImageList->Add(&bitmap, RGB(0, 255, 0)); // Here green is used as mask color

	return pImageList;
}
*/
