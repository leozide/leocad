// LibDlg.cpp : implementation file
//

#include "stdafx.h"
#include "leocad.h"
#include "LibDlg.h"
#include "GroupDlg.h"
#include "Print.h"
#include "Tools.h"
#include "ProgDlg.h"
#include "project.h"
#include "pieceinf.h"
#include "globals.h"

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
	m_bReload = FALSE;
	m_bModified = FALSE;
	m_pDragImage = NULL;
	m_bDragging = FALSE;
	m_strFile = theApp.GetProfileString("Settings", "Groups", "");

	//{{AFX_DATA_INIT(CLibraryDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
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

	FileDisk idx;
	char filename[LC_MAXPATH];

	// Read the piece library index.
	strcpy(filename, project->GetLibraryPath());
	strcat(filename, "pieces.idx");
	if (!idx.Open(filename, "rb"))
		return FALSE;
	idx.Seek(34, SEEK_SET); // skip update byte

	m_Parts.SetSize(project->GetPieceLibraryCount());
	for (int i = 0; i < project->GetPieceLibraryCount(); i++)
	{
		PARTGROUPINFO* inf = &m_Parts[i];
		inf->info = project->GetPieceInfo(i);
		inf->group = inf->info->m_nGroups;

		idx.Seek(85, SEEK_CUR);
		idx.Read(&inf->defgroup , 4);
		idx.Seek(8, SEEK_CUR);
	}
	idx.Close();

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
			for (int i = 0; i < m_Parts.GetSize(); i++)
			{
				PARTGROUPINFO* inf = &m_Parts[i];
				inf->group = inf->defgroup;
			}
			m_strFile.Empty();
			m_nMaxGroups = 9;
			
			m_ImageList.DeleteImageList();
			m_ImageList.Create(IDB_PIECEBAR, 16, 0, 0x00ff00ff);
			
			CString str;
			for (i = 0; i < 32; i++)
			{
				str.LoadString (ID_PIECE_GROUP01 + i);
				strcpy (m_strGroups[i], str);
				m_nBitmaps[i] = min(i,9);
			}
			
			m_bModified = FALSE;
			UpdateList();
			UpdateTree();

			return TRUE;
		}

		case ID_LIBDLG_FILE_OPEN:
		{
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

			return TRUE;
		}

		case ID_LIBDLG_FILE_SAVE:
		{
			DoSave(FALSE);
			return TRUE;
		}

		case ID_LIBDLG_FILE_SAVEAS:
		{
			DoSave(TRUE);
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
			CFileDialog filedlg(TRUE, ".lup\0", NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
				"LeoCAD Library Updates (*.lup)|*.lup|All Files (*.*)|*.*||", this);
			if (filedlg.DoModal() != IDOK)
				return TRUE;

			CFile up;
			BYTE bt;
			if (!up.Open (filedlg.GetPathName(), CFile::modeRead | CFile::typeBinary))
				return TRUE;

			up.Seek(32, CFile::begin);
			up.Read(&bt, 1);
			if (bt != 2)
			{
				AfxMessageBox("Wrong version !", MB_ICONWARNING|MB_OK);
				return TRUE;
			}


/*
			up.Read(&bt, 1); // update number

			CWaitCursor wc;
			char p[260];
			GetModuleFileName (AfxGetInstanceHandle(), (char*)&p, 260);
			if (char* ptr = strrchr (p,'\\')) *ptr = 0;
			CString app = p;
			app += "\\";

			DeleteFile(app + "PIECES-B.OLD");
			DeleteFile(app + "PIECES-I.OLD");
			MoveFile(app + "PIECES.BIN", app + "PIECES-B.OLD");
			MoveFile(app + "PIECES.IDX", app + "PIECES-I.OLD");

			CFile oldbin,oldidx,newbin,newidx;
			if ((!oldbin.Open (app + "PIECES-B.OLD", CFile::modeRead | CFile::typeBinary)) ||
				(!oldidx.Open (app + "PIECES-I.OLD", CFile::modeRead | CFile::typeBinary)) ||
				(!newbin.Open (app + "PIECES.BIN", CFile::modeCreate | CFile::modeWrite | CFile::typeBinary)) ||
				(!newidx.Open (app + "PIECES.IDX", CFile::modeCreate | CFile::modeWrite | CFile::typeBinary)))
			{
				AfxMessageBox("Cannot open file.", MB_OK|MB_ICONERROR);
				return TRUE;
			}

			char tmp[200];
			WORD changes, count, newcount = 0, i, j;
			up.Seek(-(LONG)sizeof(changes), CFile::end);
			up.Read(&changes, sizeof(changes));
			up.Seek(34, CFile::begin);

			oldidx.Seek (-(LONG)(sizeof(count)), CFile::end);
			oldidx.Read (&count, sizeof(count));
			oldidx.Seek (0, CFile::begin);
			oldidx.Read(tmp, 33);
			newidx.Write(tmp, 33);
			oldbin.Read(tmp, 33);
			newbin.Write(tmp, 33);

			typedef struct { 
				char name[9];
				BYTE type;
				DWORD offset;
			} UPDATE_INFO;
			
			#define UPDATE_DELETE       0x00
			#define UPDATE_DESCRIPTION  0x01
			#define UPDATE_DRAWINFO     0x02
			#define UPDATE_NEWPIECE     0x04

			UPDATE_INFO* upinfo = (UPDATE_INFO*)malloc(sizeof(UPDATE_INFO)*changes);
			memset(upinfo, 0, sizeof(UPDATE_INFO)*changes);

			DWORD nextoff, binoff, cs;
			for (i = 0; i < changes; i++)
			{
				up.Read(&upinfo[i].name, 8);
				up.Read(&upinfo[i].type, 1);
				upinfo[i].offset = up.GetPosition();

				if ((upinfo[i].type & UPDATE_DESCRIPTION) ||
					(upinfo[i].type & UPDATE_NEWPIECE))
					up.Seek(64, CFile::current);

				if ((upinfo[i].type & UPDATE_DRAWINFO) ||
					(upinfo[i].type & UPDATE_NEWPIECE))
				{
					up.Seek(sizeof(short[6]), CFile::current);
					up.Read(&cs, sizeof(cs));
					up.Seek(cs, CFile::current);
				}
			}

			CProgressDlg dlg(_T("Updating Library"));
			dlg.Create(this);
			dlg.SetRange (0, count);

			void* membuf;
			for (i = 0; i < count; i++)
			{
				char name[9];
				name[8] = 0;
				oldidx.Read (&name, 8);

				dlg.StepIt();
				if(dlg.CheckCancelButton())
				if(AfxMessageBox(IDS_CANCEL_PROMPT, MB_YESNO) == IDYES)
				{
					free(upinfo);
					return TRUE;
				}

				for (j = 0; j < changes; j++)
				if (strcmp(name, upinfo[j].name) == 0)
				{
					if (upinfo[j].type == UPDATE_DELETE)
					{
						oldidx.Seek(64+sizeof(short[6])+sizeof(DWORD), CFile::current);
						break;
					}
					newcount++;
					up.Seek(upinfo[j].offset, CFile::begin);
					newidx.Write(name, 8);

					if (upinfo[j].type & UPDATE_DESCRIPTION)
					{
						up.Read(&tmp, 64);
						oldidx.Seek(64, CFile::current);
					}
					else
						oldidx.Read(&tmp, 64);
					newidx.Write(tmp, 64);
					dlg.SetStatus(tmp);

					if (upinfo[j].type & UPDATE_DRAWINFO)
					{
						up.Read(&tmp, sizeof(short[6]));
						oldidx.Seek(sizeof(short[6]), CFile::current);
					}
					else
						oldidx.Read(&tmp, sizeof(short[6]));
					newidx.Write(tmp, sizeof(short[6]));
					binoff = newbin.GetLength();
					newidx.Write(&binoff, sizeof(binoff));

					if (upinfo[j].type & UPDATE_DRAWINFO)
					{
						up.Read(&cs, sizeof(cs));
						oldidx.Seek(sizeof(binoff), CFile::current);

						membuf = malloc(cs);
						up.Read(membuf, cs);
						newbin.Write(membuf, cs);
						free (membuf);
					}
					else
					{
						oldidx.Read(&binoff, sizeof(binoff));

						if (i == count-1)
							nextoff = oldbin.GetLength();
						else
						{
							oldidx.Seek(72+sizeof(short[6]), CFile::current);
							oldidx.Read(&nextoff, sizeof(nextoff));
							oldidx.Seek(-(LONG)(72+sizeof(short[6])+sizeof(DWORD)), CFile::current);
						}

						membuf = malloc (nextoff - binoff);
						oldbin.Seek(binoff, CFile::begin);
						oldbin.Read(membuf, nextoff - binoff);
						newbin.Write(membuf, nextoff - binoff);
						free (membuf);
					}
					break;
				}

				// not changed, just copy
				if (j == changes)
				{
					newcount++;
					newidx.Write(name, 8);
					oldidx.Read(tmp, 64+sizeof(short[6]));
					newidx.Write(tmp, 64+sizeof(short[6]));
					binoff = newbin.GetLength();
					newidx.Write(&binoff, sizeof(binoff));
					oldidx.Read(&binoff, sizeof(binoff));

					tmp[64] = 0;
					dlg.SetStatus(tmp);

					if (i == count-1)
						nextoff = oldbin.GetLength();
					else
					{
						oldidx.Seek(72+sizeof(short[6]), CFile::current);
						oldidx.Read(&nextoff, sizeof(nextoff));
						oldidx.Seek(-(LONG)(72+sizeof(short[6])+sizeof(DWORD)), CFile::current);
					}

					membuf = malloc (nextoff - binoff);
					oldbin.Seek(binoff, CFile::begin);
					oldbin.Read(membuf, nextoff - binoff);
					newbin.Write(membuf, nextoff - binoff);
					free (membuf);
				}
			}

			// now add new pieces
			for (j = 0; j < changes; j++)
			if (upinfo[j].type == UPDATE_NEWPIECE)
			{
				newcount++;
				newidx.Write(upinfo[j].name, 8);
				up.Seek(upinfo[j].offset, CFile::begin);
				up.Read(&tmp, 64+sizeof(short[6]));
				newidx.Write(tmp, 64+sizeof(short[6]));
				binoff = newbin.GetLength();
				newidx.Write(&binoff, sizeof(binoff));

				up.Read(&cs, sizeof(cs));
				membuf = malloc(cs);
				up.Read(membuf, cs);
				newbin.Write(membuf, cs);
				free (membuf);
			}

			WORD moved;
			up.Seek(-(LONG)(sizeof(WORD)*2), CFile::end);
			up.Read(&moved, sizeof(WORD));
			cs = sizeof(WORD)+moved*16;
			up.Seek(-(LONG)(cs), CFile::current);
			membuf = malloc(cs);
			up.Read(membuf, cs);
			newidx.Write(membuf, cs);
			free (membuf);

			nextoff = newbin.GetLength();
			newidx.Write(&nextoff, sizeof(DWORD));
			newidx.Write(&newcount, sizeof(WORD));

			free(upinfo);
			oldidx.Close();
			oldbin.Close();
			newidx.Close();
			newbin.Close();
			up.Close();

// update m_Parts
			UpdateList();
			m_bReload = TRUE;
*/
			return TRUE;
		}

		case ID_LIBDLG_GROUP_INSERT:
		{
			HTREEITEM hti = m_Tree.GetSelectedItem();
			
			if (hti)
			{
				DWORD dw = m_Tree.GetItemData(hti);
				if (dw == 0) dw = 1;
				
				for (int i = 0; i < 32; i++)
					if ((DWORD)(1 << i) == dw)
					{
						dw = i;
						break;
					}
					
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

				CWaitCursor wc;
				m_bModified = TRUE;

				for (int i = 0; i < 32; i++)
					if ((DWORD)(1 << i) == dw)
					{
						dw = i;
						break;
					}

				for (i = dw; i < m_nMaxGroups; i++)
				{
					strcpy (m_strGroups[i], m_strGroups[i+1]);
					m_nBitmaps[i] = m_nBitmaps[i+1];
				}

				for (int j = 0; j < m_Parts.GetSize(); j++)
				{
					DWORD grp = m_Parts[j].group;

					for (i = dw+1; i < m_nMaxGroups; i++)
					{
						DWORD d = (1 << i);
						if (grp & d)
						{
							grp &= ~d;
							grp |= (1 << (i-1));
						}
					}
					m_Parts[j].group = grp;
				}

				m_nMaxGroups--;
				UpdateTree();
				UpdateList();
			}

			return TRUE;
		}

		case ID_LIBDLG_GROUP_RENAME:
		{
			HTREEITEM hti = m_Tree.GetSelectedItem();

			if (hti)
			{
				DWORD dw = m_Tree.GetItemData(hti);
				if (dw == 0)
					return TRUE;
				
				for (int i = 0; i < 32; i++)
					if ((DWORD)(1 << i) == dw)
					{
						dw = i;
						break;
					}
					
				CGroupDlg dlg(this);
				if (dlg.DoModal() == IDOK)
				{
					strcpy (m_strGroups[i], dlg.m_strName);
					UpdateTree();
					m_bModified = TRUE;
				}
			}

			return TRUE;
		}

		case ID_LIBDLG_GROUP_MOVEUP:
		{
			HTREEITEM hti = m_Tree.GetSelectedItem();

			if (hti)
			{
				DWORD dw = m_Tree.GetItemData(hti);
				if (dw == 0)
					dw = 1;

				for (int i = 0; i < 32; i++)
					if ((DWORD)(1 << i) == dw)
					{
						dw = i;
						break;
					}

				CWaitCursor wc;
				m_bModified = TRUE;

				char tmp[33];
				int j;
				strcpy (tmp, m_strGroups[i]);
				strcpy (m_strGroups[i], m_strGroups[i-1]);
				strcpy (m_strGroups[i-1], tmp);
				j = m_nBitmaps[i];
				m_nBitmaps[i] = m_nBitmaps[i-1];
				m_nBitmaps[i-1] = j;

				for (j = 0; j < m_Parts.GetSize(); j++)
				{
					DWORD grp = m_Parts[j].group;
					BOOL g1 = (grp & (1 << i)) != 0;
					BOOL g2 = (grp & (1 << i-1)) != 0;

					if (g1)
						grp |= (1 << (i-1));
					else
						grp &= ~(1 << (i-1));
					if (g2)
						grp |= (1 << i);
					else
						grp &= ~(1 << i);

					m_Parts[j].group = grp;
				}

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
				if (dw == 0)
					dw = 1;

				for (int i = 0; i < 32; i++)
					if ((DWORD)(1 << i) == dw)
					{
						dw = i;
						break;
					}

				CWaitCursor wc;
				m_bModified = TRUE;

				char tmp[33];
				int j;
				strcpy (tmp, m_strGroups[i]);
				strcpy (m_strGroups[i], m_strGroups[i+1]);
				strcpy (m_strGroups[i+1], tmp);
				j = m_nBitmaps[i];
				m_nBitmaps[i] = m_nBitmaps[i+1];
				m_nBitmaps[i+1] = j;

				for (j = 0; j < m_Parts.GetSize(); j++)
				{
					DWORD grp = m_Parts[j].group;
					BOOL g1 = (grp & (1 << i)) != 0;
					BOOL g2 = (grp & (1 << i+1)) != 0;

					if (g1)
						grp |= (1 << (i+1));
					else
						grp &= ~(1 << (i+1));
					if (g2)
						grp |= (1 << i);
					else
						grp &= ~(1 << i);

					m_Parts[j].group = grp;
				}

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
			if (AfxMessageBox("Are you sure you want to delete ?", MB_ICONQUESTION | MB_YESNO) != IDYES)
				return TRUE;

			CString str = project->GetLibraryPath();
			DeleteFile(str + "pieces-i.old");
			DeleteFile(str + "pieces-b.old");
			rename(str + "pieces.idx", str + "pieces-i.old");
			rename(str + "pieces.bin", str + "pieces-b.old");

			FileDisk oldbin, oldidx, newbin, newidx;
			if ((!oldbin.Open(str + "pieces-b.old", "rb")) ||
				(!oldidx.Open(str + "pieces-i.old", "rb")) ||
				(!newbin.Open(str + "pieces.bin", "wb")) ||
				(!newidx.Open(str + "pieces.idx", "wb")))
			{
				AfxMessageBox("Cannot open file.", MB_OK|MB_ICONERROR);
				return TRUE;
			}

			char tmp[200];
			unsigned short count, deleted = 0, j;

			oldidx.Seek(-(LONG)(sizeof(count)), SEEK_END);
			oldidx.Read(&count, sizeof(count));
			oldidx.Seek(0, SEEK_SET);
			oldidx.Read(tmp, 34);
			newidx.Write(tmp, 34);
			oldbin.Read(tmp, 32);
			newbin.Write(tmp, 32);

			CProgressDlg dlg("Deleting");
			dlg.Create(this);
			dlg.SetRange (0, count);

			for (j = 0; j < count; j++)
			{
				dlg.StepIt();
				dlg.SetStatus(m_Parts[j].info->m_strDescription);

				if (dlg.CheckCancelButton())
				if (AfxMessageBox(IDS_CANCEL_PROMPT, MB_YESNO) == IDYES)
					break;

				BOOL bDelete = FALSE;
				char name[9];
				name[8] = 0;
				oldidx.Read (&name, 8);

				for (int i = 0; i < m_List.GetItemCount(); i++)
					if (strcmp(name, m_Parts[m_List.GetItemData(i)].info->m_strName) == 0)
					{
						if (m_List.GetItemState(i, LVIS_SELECTED))
							bDelete = TRUE;
						break;
					}

				if (bDelete)
				{
					oldidx.Seek(64+12+13, SEEK_CUR);
					deleted++;
					continue;
				}

				newidx.Write(name, 8);
				oldidx.Read(tmp, 64+12+5);
				newidx.Write(tmp, 64+12+5);
				unsigned long binoff = newbin.GetLength(), size;
				newidx.Write(&binoff, sizeof(binoff));
				oldidx.Read(&binoff, sizeof(binoff));
				oldidx.Read(&size, sizeof(size));
				newidx.Write(&size, sizeof(size));

				void* membuf = malloc (size);
				oldbin.Seek(binoff, SEEK_SET);
				oldbin.Read(membuf, size);
				newbin.Write(membuf, size);
				free(membuf);
			}

			// list of moved pieces
			unsigned short moved;
			long seek;

			oldidx.Seek(-8, SEEK_END);
			oldidx.Read(&moved, 2);
			seek = 16*moved+2;
			oldidx.Seek(-seek, SEEK_SET);
			void* membuf = malloc(seek);
			oldidx.Read(membuf, seek);
			newidx.Write(membuf, seek);
			free(membuf);

			// info at the end
			unsigned long binoff = newbin.GetPosition();
			newidx.Write(&binoff, sizeof(binoff));
			count -= deleted;
			newidx.Write(&count, sizeof(count));

			// Reload the piece library index.
			newidx.Seek(34, SEEK_SET);

			m_Parts.SetSize(count);
			for (int i = 0; i < count; i++)
			{
				PARTGROUPINFO* inf = &m_Parts[i];
				inf->info = project->GetPieceInfo(i);
				inf->group = inf->info->m_nGroups;

				newidx.Seek(85, SEEK_CUR);
				newidx.Read(&inf->defgroup , 4);
				newidx.Seek(8, SEEK_CUR);
			}

			oldidx.Close();
			oldbin.Close();
			newidx.Close();
			newbin.Close();

			UpdateList();
			m_bReload = TRUE;

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

		for (int i = 0; i < m_Parts.GetSize(); i++)
		{
			if ((dw != 0) && ((dw & m_Parts[i].group) == 0))
				continue;

			LVITEM lvi;
			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			lvi.iItem = 0;
			lvi.iSubItem = 0;
			lvi.lParam = i;
			lvi.pszText = m_Parts[i].info->m_strDescription;
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

	for (int i = 0; i < m_nMaxGroups; i++)
	{
		TV_INSERTSTRUCT tvstruct;
		tvstruct.hParent = hRootItem;
		tvstruct.hInsertAfter = TVI_LAST;
		tvstruct.item.iImage = 0;
		tvstruct.item.iSelectedImage = 1;
		tvstruct.item.lParam = (LONG)(1 << i);
		tvstruct.item.pszText = m_strGroups[i];
		tvstruct.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM;
		m_Tree.InsertItem(&tvstruct);
	}
	m_Tree.Expand (hRootItem, TVE_EXPAND);
	m_Tree.SelectItem(hRootItem);
}

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
			BOOL bControl = (GetKeyState (VK_CONTROL) < 0);

			for (int i = 0; i < m_List.GetItemCount(); i++)
				if (m_List.GetItemState(i,LVIS_SELECTED))
				{
					m_bModified = TRUE;

					if (bControl)
						m_Parts[m_List.GetItemData(i)].group |= dw;
					else
						m_Parts[m_List.GetItemData(i)].group = dw;
				}
			m_Tree.SelectDropTarget (NULL);
			m_hDropItem = NULL;
			UpdateList();
		}
	}
	
	CDialog::OnLButtonUp(nFlags, point);
}

void CLibraryDlg::OnCancel() 
{
	if (m_bModified)
		if (AfxMessageBox("Discard changes ?", MB_YESNO|MB_ICONQUESTION) == IDNO)
			return;

	CDialog::OnCancel();

	if (m_pDragImage)
		delete m_pDragImage;
}

void CLibraryDlg::OnOK() 
{
	if (m_bModified)
	switch(AfxMessageBox ("Save changes ?", MB_YESNOCANCEL|MB_ICONQUESTION))
	{
		case IDYES:
		{
			if (DoSave(FALSE) == FALSE)
				return;
		} break;
		case IDNO:
			break;
		case IDCANCEL: 
			return;
	}
	
	CDialog::OnOK();

	for (int i = 0; i < m_Parts.GetSize(); i++)
	{
		PARTGROUPINFO* inf = &m_Parts[i];
		inf->info->m_nGroups = inf->group;
	}
	theApp.WriteProfileString("Settings", "Groups", m_strFile);

	if (m_pDragImage)
		delete m_pDragImage;
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

	EnableControl(ID_LIBDLG_GROUP_INSERT, m_nMaxGroups < 32);
	EnableControl(ID_LIBDLG_GROUP_RENAME, bValid);
	EnableControl(ID_LIBDLG_GROUP_DELETE, bValid && m_nMaxGroups != 1);
	EnableControl(ID_LIBDLG_GROUP_MOVEUP, dw > 1);
	EnableControl(ID_LIBDLG_GROUP_MOVEDOWN, (dw != 0) && (dw != (DWORD)(1 << m_nMaxGroups-1)));

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
