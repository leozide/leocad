#include "lc_global.h"
#include "leocad.h"
#include "Terrain.h"
#include "TerrWnd.h"
#include "TerrDlg.h"
#include "TerOpDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTerrainDlg dialog


CTerrainDlg::CTerrainDlg(Terrain* pTerrain, bool bLinear, CWnd* pParent /*=NULL*/)
	: CDialog(CTerrainDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTerrainDlg)
	//}}AFX_DATA_INIT

	mView = new lcTerrainView(NULL, pTerrain);
	mTerrainWnd = NULL;
	mTerrain = pTerrain;
	m_bLinear = bLinear;
}

CTerrainDlg::~CTerrainDlg()
{
	delete mView;
	delete mTerrainWnd;
}

void CTerrainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTerrainDlg)
	//}}AFX_DATA_MAP

	DDX_Control(pDX, IDC_GRID, m_Grid);
}


BEGIN_MESSAGE_MAP(CTerrainDlg, CDialog)
	//{{AFX_MSG_MAP(CTerrainDlg)
	ON_WM_SIZE()
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_LC_EDIT_CLOSED, OnGridChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTerrainDlg message handlers

BOOL CTerrainDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// Add the ToolBar.
	if (!m_wndToolBar.Create(this) || !m_wndToolBar.LoadToolBar(IDR_TERRAIN))
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

	// Adjust the dialog window dimensions
	CRect rcWindow;
	GetWindowRect(rcWindow);
	rcWindow.right += rcClientStart.Width() - rcClientNow.Width();
	rcWindow.bottom += rcClientStart.Height() - rcClientNow.Height();
	MoveWindow(rcWindow, FALSE);

	// And position the control bars
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);

	HINSTANCE hInst = AfxGetInstanceHandle();
	WNDCLASS wndcls;

LRESULT CALLBACK GLWindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define OPENGL_CLASSNAME _T("LeoCADOpenGLClass")
#define TERRAIN_CLASSNAME _T("LeoCADTerrainOpenGLClass")

	// check if our class is registered
	if(!(GetClassInfo (hInst, TERRAIN_CLASSNAME, &wndcls)))
	{
		if (GetClassInfo (hInst, OPENGL_CLASSNAME, &wndcls))
		{
			// set our class name
			wndcls.lpszClassName = TERRAIN_CLASSNAME;
			wndcls.lpfnWndProc = GLWindowProc;
			wndcls.hCursor = NULL;

			// register class
			if (!AfxRegisterClass(&wndcls))
				AfxThrowResourceException();
		}
		else
			AfxThrowResourceException();
	}

	mTerrainWnd = new CWnd;
	mTerrainWnd->CreateEx(0, TERRAIN_CLASSNAME, "LeoCAD", WS_BORDER | WS_CHILD | WS_VISIBLE, CRect(0, 0, 20, 20), this, 0, mView);

	mView->LoadTexture(m_bLinear);

	m_Grid.SetControlPoints(mTerrain->GetCountU(), mTerrain->GetCountV(), mTerrain->GetControlPoints());

	PostMessage(WM_COMMAND, ID_TERDLG_EDIT_ZOOM);

	return TRUE;
}

BOOL CTerrainDlg::DestroyWindow() 
{
	if (mTerrainWnd)
	{
		mTerrainWnd->DestroyWindow();
		delete mTerrainWnd;
		mTerrainWnd = NULL;
	}
	
	return CDialog::DestroyWindow();
}

LRESULT CTerrainDlg::OnGridChange(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	m_Grid.InvalidateRect(NULL, FALSE);

	mTerrain->SetControlPoints();
	mTerrain->Tesselate();

	mView->Redraw();

	return TRUE;
}

BOOL CTerrainDlg::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	switch (LOWORD(wParam))
	{
		case ID_TERDLG_FILE_OPEN:
		{
			CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, 
				"LeoCAD Terrain Files (*.ltr)|*.ltr|All Files (*.*)|*.*||", this);

			if (dlg.DoModal() == IDOK)
			{
				CWaitCursor wait;
				CFile f (dlg.GetPathName(), CFile::modeRead | CFile::typeBinary | CFile::shareDenyWrite);
				if (f.GetLength() != 0)
				{
					f.Seek(32, CFile::begin);
					CArchive ar (&f, CArchive::load | CArchive::bNoFlushOnDelete);
					Serialize(ar);
					ar.Close();
				}
				f.Close();
			}

			return TRUE;
		}

		case ID_TERDLG_FILE_SAVE:
		{
			return TRUE;
		}

		case ID_TERDLG_FILE_SAVEAS:
		{
			return TRUE;
		}

		case ID_TERDLG_EDIT_RANDOM:
		{
			mTerrain->GenerateRandom();
			mTerrain->m_nOptions &= ~LC_TERRAIN_FLAT;
			m_Grid.SetControlPoints(mTerrain->GetCountU(), mTerrain->GetCountV(), mTerrain->GetControlPoints());
			mView->Redraw();
			return TRUE;
		}

		case ID_TERDLG_EDIT_PREFERENCES:
		{
			CTerrainOptionsDlg dlg;
			dlg.SetOptions(mTerrain);

			if (dlg.DoModal() == IDOK)
			{
				dlg.GetOptions(mTerrain);
				m_Grid.SetControlPoints(mTerrain->GetCountU(), mTerrain->GetCountV(), mTerrain->GetControlPoints());
				mTerrain->Tesselate();
				mView->LoadTexture(m_bLinear);

				mView->Redraw();
			}

			return TRUE;
		}
		
		case ID_TERDLG_EDIT_RESETCAMERA:
		{
			mView->ResetCamera();
			mView->Redraw();
			return TRUE;
		}

		case ID_TERDLG_EDIT_ZOOM:
		case ID_TERDLG_EDIT_PAN:
		case ID_TERDLG_EDIT_ROTATE:
		{
			CheckControl(mView->mAction + ID_TERDLG_EDIT_ZOOM, FALSE);
			mView->SetAction(LOWORD(wParam) - ID_TERDLG_EDIT_ZOOM);
			CheckControl(mView->mAction + ID_TERDLG_EDIT_ZOOM, TRUE);

			GetMenu()->GetSubMenu(1)->CheckMenuRadioItem(ID_TERDLG_EDIT_ZOOM, ID_TERDLG_EDIT_ROTATE, mView->mAction + ID_TERDLG_EDIT_ZOOM, MF_BYCOMMAND);

			return TRUE;
		}

		case ID_TERDLG_HELP:
		{
			MessageBox("The terrain is a Bezier surface divided in cubic (4x4) patches, "
				"this means that the points along the black lines will always be part of the surface but the others probably won't.\n"
				"For better results check the box \"Smooth\" or select a texture in the options dialog.\n"
				"If you want to have an infinite plane (like in versions before 0.64), check the box \"Flat\".\n"
				"The grid control has some minor drawing bugs, I'll fix them later. :)\n\n"
				"I'll add something better to the help system later.", "Quick Help", MB_OK|MB_ICONINFORMATION);

			return TRUE;
		}
	}

	return CDialog::OnCommand(wParam, lParam);
}

void CTerrainDlg::CheckControl(UINT nID, BOOL bEnable)
{
//	GetMenu()->GetSubMenu(1)->CheckMenuItem(nID, MF_BYCOMMAND | (bEnable ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
	int state = m_wndToolBar.GetToolBarCtrl().GetState(nID);
	if (bEnable)
		state |= TBSTATE_CHECKED;
	else
		state &= ~TBSTATE_CHECKED;

	m_wndToolBar.GetToolBarCtrl().SetState(nID, state);
}

BOOL CTerrainDlg::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message == WM_KEYDOWN)
	if ((pMsg->wParam == VK_ESCAPE) || (pMsg->wParam == VK_RETURN))
		return TRUE;
	
	return CDialog::PreTranslateMessage(pMsg);
}

void CTerrainDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);

	// Fix the toolbar and menu.
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);

	if (mTerrainWnd)
	{
		CRect rcClient;

		// Find out how much space is left.
		RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0, reposQuery, rcClient);

		m_Grid.MoveWindow (rcClient.left + 5, rcClient.top + 5, rcClient.Width()/2 - 7, rcClient.Height() - 10);
		mTerrainWnd->MoveWindow (rcClient.left + rcClient.Width()/2 + 2, rcClient.top + 5, rcClient.Width()/2 - 7, rcClient.Height() - 10);
	}
}

void CTerrainDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);

	if (bShow)
	{
		CRect rcWindow;
		GetWindowRect(rcWindow);
		OnSize(SIZE_RESTORED, rcWindow.Width(), rcWindow.Height());
	}
}