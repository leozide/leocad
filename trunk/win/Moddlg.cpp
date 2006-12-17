// ModDlg.cpp: implementation of the CModifyDialog class. 
// 
////////////////////////////////////////////////////////////////////// 

#include "lc_global.h"
#include "leocad.h" 
#include "ModDlg.h" 
#include "tools.h"

#include "project.h"
#include "globals.h"
#include "defines.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "lc_application.h"

#ifdef _DEBUG 
#undef THIS_FILE 
static char THIS_FILE[]=__FILE__; 
#define new DEBUG_NEW 
#endif 

/////////////////////////////////////////////////////////////////////////////
// CModifyDialogBar

CModifyDialogBar::CModifyDialogBar()
{
	m_dwSCBStyle |= SCBS_SHOWEDGES;
}

CModifyDialogBar::~CModifyDialogBar()
{
}


BEGIN_MESSAGE_MAP(CModifyDialogBar, CSizingControlBarG)
	//{{AFX_MSG_MAP(CModifyDialogBar)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CModifyDialogBar message handlers

int CModifyDialogBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CSizingControlBarG::OnCreate(lpCreateStruct) == -1)
		return -1;

	SetSCBStyle(GetSCBStyle() | SCBS_SIZECHILD);

	m_ModifyDlg.Create(CModifyDialog::IDD, this);
	m_ModifyDlg.OnInitDialogBar();

	return 0;
}

CSize CModifyDialogBar::CalcDynamicLayout(int nLength, DWORD dwMode)
{
	// Update minimum size.
	CRect rc;

	m_ModifyDlg.GetDlgItem(IDC_MODIFY_CHILD)->GetWindowRect(&rc);
	ScreenToClient(&rc);

	int Width = rc.Width() + rc.left * 2 + 4;

	if (dwMode & (LM_LENGTHY | LM_VERTDOCK))
	{
		if (m_ModifyDlg.m_RollUp.m_PageHeight >= nLength - rc.top - 10)
			Width += RC_SCROLLBARWIDTH;
	}
	else
	{
		m_ModifyDlg.m_RollUp.GetWindowRect(&rc);
		ScreenToClient(&rc);

		if (m_ModifyDlg.m_RollUp.m_PageHeight >= rc.Height())
			Width += RC_SCROLLBARWIDTH;
	}

	m_szMinFloat.cx = Width + 4;

	return CSizingControlBarG::CalcDynamicLayout(nLength, dwMode);
}

CSize CModifyDialogBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	// Update minimum size.
	CRect rc;

	m_ModifyDlg.GetDlgItem(IDC_MODIFY_CHILD)->GetWindowRect(&rc);
	ScreenToClient(&rc);

	int Width = rc.Width() + rc.left * 2 + 14;

	if (!bHorz)
	{
		m_ModifyDlg.m_RollUp.GetWindowRect(&rc);
		ScreenToClient(&rc);

		if (m_ModifyDlg.m_RollUp.m_PageHeight >= rc.Height())
			Width += RC_SCROLLBARWIDTH;
	}

	m_szMinHorz.cx = m_szMinVert.cx = Width;

	return CSizingControlBarG::CalcFixedLayout(bStretch, bHorz);
}

void CModifyDialogBar::OnSize(UINT nType, int cx, int cy)
{
	CSizingControlBarG::OnSize(nType, cx, cy);
}

////////////////////////////////////////////////////////////////////// 
// Construction/Destruction 
////////////////////////////////////////////////////////////////////// 

IMPLEMENT_DYNAMIC(CModifyDialog, CDialog) 

BEGIN_MESSAGE_MAP(CModifyDialog, CDialog) 
	//{{AFX_MSG_MAP(CModifyDialog) 
	ON_BN_CLICKED(IDC_MODDLG_PIECE, OnModdlgPiece)
	ON_CBN_SELENDOK(IDC_MODDLG_LIST, OnSelendokModdlgList)
	ON_CBN_DROPDOWN(IDC_MODDLG_LIST, OnDropdownModdlgList)
	ON_BN_CLICKED(IDC_MODDLG_APPLY, OnModdlgApply)
	ON_WM_SHOWWINDOW()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_COMMAND_RANGE(ID_MODDLG_PIECES, ID_MODDLG_LIGHTS, OnMenuClick)
END_MESSAGE_MAP() 

CModifyDialog::CModifyDialog(CWnd* pParent) 
{
	m_pObject = NULL;
	m_CurrentType = -1;

	//{{AFX_DATA_INIT(CModifyDialog)
	m_strName = _T("");
	//}}AFX_DATA_INIT
} 

CModifyDialog::~CModifyDialog() 
{
}

BOOL CModifyDialog::OnInitDialogBar()
{
	m_RollUp.Create(WS_VISIBLE|WS_CHILD, CRect(4,4,187,362), this, 2);

	m_PieceDlg.Create(CModifyPieceDlg::IDD, &m_RollUp);
	m_CameraDlg.Create(CModifyCameraDlg::IDD, &m_RollUp);
	m_LightDlg.Create(CModifyLightDlg::IDD, &m_RollUp);

	PositionChildren();

	// Support for the MFC DDX model
	UpdateData(FALSE);

	m_ctlCombo.LimitText(80);
	UpdateControls(LC_OBJECT_PIECE);

	return TRUE; 
}

void CModifyDialog::DoDataExchange(CDataExchange* pDX)
{
	//Derived Classes Overide this function 
	ASSERT(pDX);

	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModifyDialog)
	DDX_Control(pDX, IDC_MODDLG_LIST, m_ctlCombo);
	DDX_Text(pDX, IDC_MODDLG_LIST, m_strName);
	DDV_MaxChars(pDX, m_strName, 80);
	//}}AFX_DATA_MAP
}

void CModifyDialog::OnUpdateCmdUI(CFrameWnd * pTarget, BOOL /*bDisableIfNoHndler*/)
{
	UpdateDialogControls(pTarget, FALSE);
}

void CModifyDialog::UpdateInfo(Object* pObject)
{
	if ((GetStyle() & WS_VISIBLE) == 0)
		return;

	if (pObject == NULL)
		pObject = lcGetActiveProject()->GetFocusObject();

	m_pObject = pObject;

	if (m_pObject == NULL)
	{
		/*
		m_fPosX = m_fPosY = m_fPosZ = 0.0f;
		m_fRotX = m_fRotY = m_fRotZ = 0.0f;
		m_fUpX = m_fUpY = m_fUpZ = 0.0f;
		UpdateData(FALSE);
		*/
		return;
	}
	else
	{
		UpdateControls(m_pObject->GetType());
	}

	switch (m_CurrentType)
	{
		case LC_OBJECT_PIECE:
		{
			m_PieceDlg.UpdateInfo((Piece*)m_pObject);
			m_ctlCombo.SetWindowText(((Piece*)m_pObject)->GetName());
		} break;

		case LC_OBJECT_CAMERA:
		case LC_OBJECT_CAMERA_TARGET:
		{
			Camera* camera;

			if (m_CurrentType == LC_OBJECT_CAMERA)
				camera = (Camera*)m_pObject;
			else
				camera = ((CameraTarget*)m_pObject)->GetParent();

			m_CameraDlg.UpdateInfo(camera);
			m_ctlCombo.SetWindowText(((Camera*)m_pObject)->GetName());
		} break;

		case LC_OBJECT_LIGHT:
		case LC_OBJECT_LIGHT_TARGET:
		{
			Light* light;

			if (m_CurrentType == LC_OBJECT_LIGHT)
				light = (Light*)m_pObject;
			else
				light = ((LightTarget*)m_pObject)->GetParent();

			m_LightDlg.UpdateInfo(light);
			m_ctlCombo.SetWindowText(((Light*)m_pObject)->GetName());
		} break;
	}
}

void CModifyDialog::OnModdlgPiece() 
{
	CMenu menu;
	CMenu* pPopup;
	RECT rc;
	::GetWindowRect(::GetDlgItem(m_hWnd, IDC_MODDLG_PIECE), &rc);
	menu.LoadMenu(IDR_POPUPS);
	pPopup = menu.GetSubMenu(5);
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, rc.right, rc.top, this);
}

void CModifyDialog::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);

	if (bShow)
		PositionChildren();
}

void CModifyDialog::OnMove(int x, int y)
{
	CDialog::OnMove(x, y);

	// Avoid calling before window creation.
	if (IsWindowVisible())
		PositionChildren();
}

void CModifyDialog::OnSize(UINT nType, int cx, int cy)
{
	PositionChildren();
}

void CModifyDialog::PositionChildren()
{
	CRect StaticRect, ClientRect;

	if (!IsWindow(m_PieceDlg.m_hWnd))
		return;

	GetDlgItem(IDC_MODIFY_CHILD)->GetWindowRect(&StaticRect);
	GetClientRect(&ClientRect);

	// Recompute coordinates relative to parent window.
	ScreenToClient(&StaticRect);

	m_RollUp.MoveWindow(StaticRect.left-1, StaticRect.top, ClientRect.Width() - (StaticRect.left-1) * 2, ClientRect.Height() - StaticRect.top, TRUE);

	m_ctlCombo.GetWindowRect(&StaticRect);
	ScreenToClient(&StaticRect);
	m_ctlCombo.MoveWindow(StaticRect.left, StaticRect.top, ClientRect.Width() - StaticRect.left - 4, StaticRect.Height(), TRUE);
}


void CModifyDialog::UpdateControls(int Type)
{
	if (m_CurrentType == Type)
		return;

	DeleteObject((HBITMAP)SendDlgItemMessage(IDC_MODDLG_PIECE, BM_GETIMAGE, IMAGE_BITMAP, 0));

	UINT id = IDB_PIECE;
	if (Type == LC_OBJECT_CAMERA || Type == LC_OBJECT_CAMERA_TARGET)
		id = IDB_CAMERA;
	if (Type == LC_OBJECT_LIGHT || Type == LC_OBJECT_LIGHT_TARGET)
		id = IDB_LIGHT;
	SendDlgItemMessage(IDC_MODDLG_PIECE, BM_SETIMAGE, IMAGE_BITMAP, 
	                   (LPARAM)LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(id)));

	m_ctlCombo.SetWindowText("");

	m_RollUp.RemoveAllPages();

	switch (Type)
	{
	case LC_OBJECT_PIECE:
		m_RollUp.InsertPage("", &m_PieceDlg, FALSE);
		break;

	case LC_OBJECT_CAMERA:
	case LC_OBJECT_CAMERA_TARGET:
		m_RollUp.InsertPage("", &m_CameraDlg, FALSE);
		break;

	case LC_OBJECT_LIGHT:
	case LC_OBJECT_LIGHT_TARGET:
		m_RollUp.InsertPage("", &m_LightDlg, FALSE);
		break;
	}

	m_RollUp.ExpandAllPages();

	m_CurrentType = Type;
}

void CModifyDialog::OnMenuClick(UINT nID)
{
	m_pObject = NULL;
	switch (nID - ID_MODDLG_PIECES)
	{
	case 0: UpdateControls(LC_OBJECT_PIECE); break;
	case 1: UpdateControls(LC_OBJECT_CAMERA); break;
	case 2: UpdateControls(LC_OBJECT_LIGHT); break;
	}
}

void CModifyDialog::OnSelendokModdlgList() 
{
	void* pNew = m_ctlCombo.GetItemDataPtr(m_ctlCombo.GetCurSel());
	if ((pNew != m_pObject) && (pNew != (void*)-1))
		UpdateInfo((Object*)pNew);
}

void CModifyDialog::OnModdlgApply() 
{
	if (m_pObject == NULL)
		return;

	UpdateData(TRUE);

	switch (m_CurrentType)
	{
		case LC_OBJECT_PIECE:
			m_PieceDlg.Apply((Piece*)m_pObject);
			break;

		case LC_OBJECT_CAMERA:
		case LC_OBJECT_CAMERA_TARGET:
			m_CameraDlg.Apply((Camera*)m_pObject);
			break;

		case LC_OBJECT_LIGHT:
		case LC_OBJECT_LIGHT_TARGET:
			m_LightDlg.Apply((Light*)m_pObject);
			break;
	}
}

void CModifyDialog::OnDropdownModdlgList() 
{
	Piece* pPiece = lcGetActiveProject()->GetActiveModel()->m_Pieces;
	Camera* pCamera = lcGetActiveProject()->GetActiveModel()->m_Cameras;
	Light* pLight = lcGetActiveProject()->GetActiveModel()->m_Lights;
	int i;

	m_ctlCombo.ResetContent();

	switch (m_CurrentType)
	{
		case LC_OBJECT_PIECE:
		{
			for (; pPiece; pPiece = (Piece*)pPiece->m_Next)
			{
				i = m_ctlCombo.AddString(pPiece->GetName());
				m_ctlCombo.SetItemDataPtr(i, pPiece);
			}

			if (m_pObject)
				m_ctlCombo.SelectString(-1, ((Piece*)m_pObject)->GetName());
		} break;

		case LC_OBJECT_CAMERA:
		case LC_OBJECT_CAMERA_TARGET:
		{
			for (; pCamera; pCamera = (Camera*)pCamera->m_Next)
			{
				i = m_ctlCombo.AddString(pCamera->GetName());
				m_ctlCombo.SetItemDataPtr(i, pCamera);
			}

			if (m_pObject)
				m_ctlCombo.SelectString(-1, ((Camera*)m_pObject)->GetName());
		} break;

		case LC_OBJECT_LIGHT:
		case LC_OBJECT_LIGHT_TARGET:
		{
			for (; pLight; pLight = (Light*)pLight->m_Next)
			{
				i = m_ctlCombo.AddString(pLight->GetName());
				m_ctlCombo.SetItemDataPtr(i, pLight);
			}

			if (m_pObject)
				m_ctlCombo.SelectString(-1, ((Light*)m_pObject)->GetName());
		} break;
	}
}

#include <locale.h>
static void DDX_Text_Float(CDataExchange* pDX, int nIDC, float& value)
{
	pDX->PrepareEditCtrl(nIDC);
	HWND hWndCtrl;
	pDX->m_pDlgWnd->GetDlgItem(nIDC, &hWndCtrl);
	TCHAR szBuffer[400];
	if (pDX->m_bSaveAndValidate)
	{
		::GetWindowText(hWndCtrl, szBuffer, sizeof(szBuffer)/sizeof(szBuffer[0]));
		double d = 0;
		if (_stscanf(szBuffer, _T("%lf"), &d) != 1)
		{
//			AfxMessageBox(AFX_IDP_PARSE_REAL);
//			pDX->Fail();            // throws exception
		}
		value = (float)d;
	}
	else
	{
		_stprintf(szBuffer, _T("%.2f"), value);
		int nNewLen = lstrlen(szBuffer);

		lconv* lc = localeconv();

		// crop zeros
		TCHAR* dot = strrchr(szBuffer, lc->decimal_point[0]);
		if (dot)
		{
			TCHAR* ch = &szBuffer[nNewLen-1];

			while (ch >= dot)
			{
				if (*ch != '0' && *ch != lc->decimal_point[0])
					break;

				*ch-- = 0;
				nNewLen--;
			}
		}

		TCHAR szOld[256];
		// fast check to see if text really changes (reduces flash in controls)
		if (nNewLen > sizeof(szOld)/sizeof(szOld[0]) ||
			::GetWindowText(hWndCtrl, szOld, sizeof(szOld)/sizeof(szOld[0])) != nNewLen ||
			lstrcmp(szOld, szBuffer) != 0)
		{
			// change it
			::SetWindowText(hWndCtrl, szBuffer);
		}
	}
}

// CModifyPieceDlg dialog
IMPLEMENT_DYNAMIC(CModifyPieceDlg, CDialog)

CModifyPieceDlg::CModifyPieceDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CModifyPieceDlg::IDD, pParent)
{
	m_PosX = 0.0f;
	m_PosY = 0.0f;
	m_PosZ = 0.0f;
	m_RotX = 0.0f;
	m_RotY = 0.0f;
	m_RotZ = 0.0f;
	m_Hidden = false;
	m_From = 0;
	m_To = 0;
}

CModifyPieceDlg::~CModifyPieceDlg()
{
}

void CModifyPieceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CModifyDialog)
	DDX_Text_Float(pDX, IDC_MODDLG_POSX, m_PosX);
	DDX_Text_Float(pDX, IDC_MODDLG_POSY, m_PosY);
	DDX_Text_Float(pDX, IDC_MODDLG_POSZ, m_PosZ);
	DDX_Text_Float(pDX, IDC_MODDLG_ROTX, m_RotX);
	DDX_Text_Float(pDX, IDC_MODDLG_ROTY, m_RotY);
	DDX_Text_Float(pDX, IDC_MODDLG_ROTZ, m_RotZ);
	DDX_Check(pDX, IDC_MODDLG_HIDDEN, m_Hidden);
	DDX_Text(pDX, IDC_MODDLG_FROM, m_From);
	DDX_Text(pDX, IDC_MODDLG_TO, m_To);
	DDX_Control(pDX, IDC_MODDLG_COLOR, m_Color);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CModifyPieceDlg, CDialog)
	//{{AFX_MSG_MAP(CModifyPieceDlg)
	ON_EN_KILLFOCUS(IDC_MODDLG_POSX, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_POSY, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_POSZ, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_ROTX, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_ROTY, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_ROTZ, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_FROM, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_TO, OnDataChange)
	ON_BN_CLICKED(IDC_MODDLG_HIDDEN, OnDataChange)
	ON_MESSAGE_VOID(CPN_SELENDOK, OnDataChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// CModifyPieceDlg message handlers
void CModifyPieceDlg::OnOK() 
{
	((CModifyDialog*)GetParent()->GetParent())->PostMessage(WM_COMMAND, IDC_MODDLG_APPLY);
}

void CModifyPieceDlg::OnCancel() 
{
}

void CModifyPieceDlg::OnDataChange()
{
	((CModifyDialog*)GetParent()->GetParent())->PostMessage(WM_COMMAND, IDC_MODDLG_APPLY);
}

void CModifyPieceDlg::UpdateInfo(Piece* piece)
{
	if (piece == NULL)
	{
		m_PosX = m_PosY = m_PosZ = 0.0f;
		m_RotX = m_RotY = m_RotZ = 0.0f;
		m_From = m_To = 0;
		m_Hidden = false;
		m_Color.SetColorIndex(0);
	}
	else
	{
		// Position.
		Vector3 Pos = piece->GetPosition();
		lcGetActiveProject()->ConvertToUserUnits(Pos);

		m_PosX = Pos[0];
		m_PosY = Pos[1];
		m_PosZ = Pos[2];

		// Rotation.
		float rot[4];
		piece->GetRotation(rot);
		Matrix33 mat = MatrixFromAxisAngle(Vector3(rot[0], rot[1], rot[2]), rot[3] * LC_DTOR);
		Vector3 Rot = MatrixToEulerAngles(mat) * LC_RTOD;

		m_RotX = Rot[0];
		m_RotY = Rot[1];
		m_RotZ = Rot[2];

		// Steps.
		m_From = piece->GetTimeShow();
		m_To = piece->GetTimeHide();

		m_Hidden = piece->IsHidden();
		m_Color.SetColorIndex(piece->GetColor());
	}

	UpdateData(FALSE);
}

void CModifyPieceDlg::Apply(Piece* piece)
{
	UpdateData(TRUE);

	LC_PIECE_MODIFY mod;

	mod.piece = piece;
	mod.Position = Vector3(m_PosX, m_PosY, m_PosZ);
	lcGetActiveProject()->ConvertFromUserUnits(mod.Position);
	mod.Rotation = Vector3(m_RotX, m_RotY, m_RotZ);
	mod.from = m_From;
	mod.to = m_To;
	mod.hidden = (m_Hidden != FALSE);
	mod.color = m_Color.GetColorIndex();
	strcpy(mod.name, ((CModifyDialog*)GetParent()->GetParent())->m_strName);

	lcGetActiveProject()->HandleNotify(LC_PIECE_MODIFIED, (unsigned long)&mod);
}

// CModifyCameraDlg dialog
IMPLEMENT_DYNAMIC(CModifyCameraDlg, CDialog)

CModifyCameraDlg::CModifyCameraDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CModifyCameraDlg::IDD, pParent)
{
	m_PosX = 0.0f;
	m_PosY = 0.0f;
	m_PosZ = 0.0f;
	m_TargetX = 0.0f;
	m_TargetY = 0.0f;
	m_TargetZ = 0.0f;
	m_Roll = 0.0f;
	m_FOV = 0.0f;
	m_Clip = false;
	m_Near = 0.0f;
	m_Far = 0.0f;
	m_Ortho = false;
	m_Cone = false;
	m_Hidden = false;
}

CModifyCameraDlg::~CModifyCameraDlg()
{
}

void CModifyCameraDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CModifyDialog)
	DDX_Text_Float(pDX, IDC_MODDLG_POSX, m_PosX);
	DDX_Text_Float(pDX, IDC_MODDLG_POSY, m_PosY);
	DDX_Text_Float(pDX, IDC_MODDLG_POSZ, m_PosZ);
	DDX_Text_Float(pDX, IDC_MODDLG_TARGETX, m_TargetX);
	DDX_Text_Float(pDX, IDC_MODDLG_TARGETY, m_TargetY);
	DDX_Text_Float(pDX, IDC_MODDLG_TARGETZ, m_TargetZ);
	DDX_Text_Float(pDX, IDC_MODDLG_ROLL, m_Roll);
	DDX_Text_Float(pDX, IDC_MODDLG_FOV, m_FOV);
	DDX_Check(pDX, IDC_MODDLG_CLIP, m_Clip);
	DDX_Text(pDX, IDC_MODDLG_NEAR, m_Near);
	DDX_Text(pDX, IDC_MODDLG_FAR, m_Far);
	DDX_Check(pDX, IDC_MODDLG_ORTHO, m_Ortho);
	DDX_Check(pDX, IDC_MODDLG_CONE, m_Cone);
	DDX_Check(pDX, IDC_MODDLG_HIDDEN, m_Hidden);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CModifyCameraDlg, CDialog)
	//{{AFX_MSG_MAP(CModifyPieceDlg)
	ON_EN_KILLFOCUS(IDC_MODDLG_POSX, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_POSY, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_POSZ, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_TARGETX, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_TARGETY, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_TARGETZ, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_ROLL, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_FOV, OnDataChange)
	ON_BN_CLICKED(IDC_MODDLG_CLIP, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_NEAR, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_FAR, OnDataChange)
	ON_BN_CLICKED(IDC_MODDLG_ORTHO, OnDataChange)
	ON_BN_CLICKED(IDC_MODDLG_CONE, OnDataChange)
	ON_BN_CLICKED(IDC_MODDLG_HIDDEN, OnDataChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// CModifyCameraDlg message handlers
void CModifyCameraDlg::OnOK() 
{
	((CModifyDialog*)GetParent()->GetParent())->PostMessage(WM_COMMAND, IDC_MODDLG_APPLY);
}

void CModifyCameraDlg::OnCancel() 
{
}

void CModifyCameraDlg::OnDataChange()
{
	((CModifyDialog*)GetParent()->GetParent())->PostMessage(WM_COMMAND, IDC_MODDLG_APPLY);
}

void CModifyCameraDlg::UpdateInfo(Camera* camera)
{
	if (camera == NULL)
	{
		m_PosX = 0.0f;
		m_PosY = 0.0f;
		m_PosZ = 0.0f;
		m_TargetX = 0.0f;
		m_TargetY = 0.0f;
		m_TargetZ = 0.0f;
		m_Roll = 0.0f;
		m_FOV = 0.0f;
		m_Near = 0.0f;
		m_Far = 0.0f;
		m_Ortho = false;
		m_Hidden = false;
	}
	else
	{
		Vector3 tmp;

		tmp = camera->GetEyePosition();
		lcGetActiveProject()->ConvertToUserUnits(tmp);
		m_PosX = tmp[0];
		m_PosY = tmp[1];
		m_PosZ = tmp[2];

		tmp = camera->GetTargetPosition();
		lcGetActiveProject()->ConvertToUserUnits(tmp);
		m_TargetX = tmp[0];
		m_TargetY = tmp[1];
		m_TargetZ = tmp[2];

		m_Roll = camera->GetRoll() * LC_RTOD;

		m_FOV = camera->m_fovy;
		m_Clip = (camera->m_nState & LC_CAMERA_AUTO_CLIP) == 0;
		m_Near = camera->m_zNear;
		m_Far = camera->m_zFar;
		m_Ortho = camera->IsOrtho();
		m_Hidden = !camera->IsVisible();
	}

	UpdateData(FALSE);
}

void CModifyCameraDlg::Apply(Camera* camera)
{
	UpdateData(TRUE);

	LC_CAMERA_MODIFY mod;

	mod.camera = camera;
	mod.Eye = Vector3(m_PosX, m_PosY, m_PosZ);
	lcGetActiveProject()->ConvertFromUserUnits(mod.Eye);
	mod.Target = Vector3(m_TargetX, m_TargetY, m_TargetZ);
	lcGetActiveProject()->ConvertFromUserUnits(mod.Target);
	mod.Roll = m_Roll * LC_DTOR;
	mod.fovy = m_FOV;
	mod.znear = m_Near;
	mod.zfar = m_Far;
	mod.ortho = (m_Ortho != FALSE);
	mod.cone = (m_Cone != FALSE);
	mod.hidden = (m_Hidden != FALSE);
	mod.clip = (m_Clip != FALSE);
	strcpy(mod.name, ((CModifyDialog*)GetParent()->GetParent())->m_strName);

	lcGetActiveProject()->HandleNotify(LC_CAMERA_MODIFIED, (unsigned long)&mod);
}

// CModifyLightDlg dialog
IMPLEMENT_DYNAMIC(CModifyLightDlg, CDialog)

CModifyLightDlg::CModifyLightDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CModifyLightDlg::IDD, pParent)
{
	m_PosX = 0.0f;
	m_PosY = 0.0f;
	m_PosZ = 0.0f;
	m_TargetX = 0.0f;
	m_TargetY = 0.0f;
	m_TargetZ = 0.0f;
	m_AmbientColor = Vector3(0, 0, 0);
	m_DiffuseColor = Vector3(0, 0, 0);
	m_SpecularColor = Vector3(0, 0, 0);
	m_Constant = 0.0f;
	m_Linear = 0.0f;
	m_Quadratic = 0.0f;
	m_Exponent = 0.0f;
	m_Cutoff = 0.0f;
	m_Hidden = false;
}

CModifyLightDlg::~CModifyLightDlg()
{
}

void CModifyLightDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CModifyDialog)
	DDX_Text_Float(pDX, IDC_MODDLG_POSX, m_PosX);
	DDX_Text_Float(pDX, IDC_MODDLG_POSY, m_PosY);
	DDX_Text_Float(pDX, IDC_MODDLG_POSZ, m_PosZ);
	DDX_Text_Float(pDX, IDC_MODDLG_TARGETX, m_TargetX);
	DDX_Text_Float(pDX, IDC_MODDLG_TARGETY, m_TargetY);
	DDX_Text_Float(pDX, IDC_MODDLG_TARGETZ, m_TargetZ);
	DDX_Control(pDX, IDC_MODDLG_AMBIENT, m_Ambient);
	DDX_Control(pDX, IDC_MODDLG_DIFFUSE, m_Diffuse);
	DDX_Control(pDX, IDC_MODDLG_SPECULAR, m_Specular);
	DDX_Text_Float(pDX, IDC_MODDLG_CONSTANT, m_Constant);
	DDX_Text_Float(pDX, IDC_MODDLG_LINEAR, m_Linear);
	DDX_Text_Float(pDX, IDC_MODDLG_QUADRATIC, m_Quadratic);
	DDX_Text_Float(pDX, IDC_MODDLG_CUTOFF, m_Cutoff);
	DDX_Text_Float(pDX, IDC_MODDLG_EXPONENT, m_Exponent);
	DDX_Check(pDX, IDC_MODDLG_HIDDEN, m_Hidden);
	//}}AFX_DATA_MAP

	if (!pDX->m_bSaveAndValidate)
	{
		COLORREF Ambient = RGB(m_AmbientColor[0]*255, m_AmbientColor[1]*255, m_AmbientColor[2]*255);
		DeleteObject(m_Ambient.SetBitmap(CreateColorBitmap(20, 10, Ambient)));

		COLORREF Diffuse = RGB(m_DiffuseColor[0]*255, m_DiffuseColor[1]*255, m_DiffuseColor[2]*255);
		DeleteObject(m_Diffuse.SetBitmap(CreateColorBitmap(20, 10, Diffuse)));

		COLORREF Specular = RGB(m_SpecularColor[0]*255, m_SpecularColor[1]*255, m_SpecularColor[2]*255);
		DeleteObject(m_Specular.SetBitmap(CreateColorBitmap(20, 10, Specular)));
	}
}


BEGIN_MESSAGE_MAP(CModifyLightDlg, CDialog)
	ON_EN_KILLFOCUS(IDC_MODDLG_POSX, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_POSY, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_POSZ, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_TARGETX, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_TARGETY, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_TARGETZ, OnDataChange)
	ON_BN_CLICKED(IDC_MODDLG_AMBIENT, OnAmbient)
	ON_BN_CLICKED(IDC_MODDLG_DIFFUSE, OnDiffuse)
	ON_BN_CLICKED(IDC_MODDLG_SPECULAR, OnSpecular)
	ON_EN_KILLFOCUS(IDC_MODDLG_CONSTANT, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_LINEAR, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_QUADRATIC, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_CUTOFF, OnDataChange)
	ON_EN_KILLFOCUS(IDC_MODDLG_EXPONENT, OnDataChange)
	ON_BN_CLICKED(IDC_MODDLG_HIDDEN, OnDataChange)
END_MESSAGE_MAP()


// CModifyLightDlg message handlers
void CModifyLightDlg::OnOK() 
{
	((CModifyDialog*)GetParent()->GetParent())->PostMessage(WM_COMMAND, IDC_MODDLG_APPLY);
}

void CModifyLightDlg::OnCancel() 
{
}

void CModifyLightDlg::OnDataChange()
{
	((CModifyDialog*)GetParent()->GetParent())->PostMessage(WM_COMMAND, IDC_MODDLG_APPLY);
}

void CModifyLightDlg::OnAmbient()
{
	COLORREF Color = RGB(m_AmbientColor[0]*255, m_AmbientColor[1]*255, m_AmbientColor[2]*255);
	CColorDialog dlg(Color);
	if (dlg.DoModal() == IDOK)
	{
		Color = dlg.GetColor();
		m_AmbientColor = Vector3(GetRValue(Color), GetGValue(Color), GetBValue(Color)) / 255.0f;
		DeleteObject(m_Ambient.SetBitmap(CreateColorBitmap(20, 10, Color)));
		OnDataChange();
	}
}

void CModifyLightDlg::OnDiffuse()
{
	COLORREF Color = RGB(m_DiffuseColor[0]*255, m_DiffuseColor[1]*255, m_DiffuseColor[2]*255);
	CColorDialog dlg(Color);
	if (dlg.DoModal() == IDOK)
	{
		Color = dlg.GetColor();
		m_DiffuseColor = Vector3(GetRValue(Color), GetGValue(Color), GetBValue(Color)) / 255.0f;
		DeleteObject(m_Diffuse.SetBitmap(CreateColorBitmap(20, 10, Color)));
		OnDataChange();
	}
}

void CModifyLightDlg::OnSpecular()
{
	COLORREF Color = RGB(m_SpecularColor[0]*255, m_SpecularColor[1]*255, m_SpecularColor[2]*255);
	CColorDialog dlg(Color);
	if (dlg.DoModal() == IDOK)
	{
		Color = dlg.GetColor();
		m_SpecularColor = Vector3(GetRValue(Color), GetGValue(Color), GetBValue(Color)) / 255.0f;
		DeleteObject(m_Specular.SetBitmap(CreateColorBitmap(20, 10, Color)));
		OnDataChange();
	}
}

void CModifyLightDlg::UpdateInfo(Light* light)
{
	GetDlgItem(IDC_MODDLG_TARGETX)->EnableWindow(TRUE);
	GetDlgItem(IDC_MODDLG_TARGETY)->EnableWindow(TRUE);
	GetDlgItem(IDC_MODDLG_TARGETZ)->EnableWindow(TRUE);
	GetDlgItem(IDC_MODDLG_CONSTANT)->EnableWindow(TRUE);
	GetDlgItem(IDC_MODDLG_LINEAR)->EnableWindow(TRUE);
	GetDlgItem(IDC_MODDLG_QUADRATIC)->EnableWindow(TRUE);

	if (light == NULL)
	{
		m_PosX = 0.0f;
		m_PosY = 0.0f;
		m_PosZ = 0.0f;
		m_TargetX = 0.0f;
		m_TargetY = 0.0f;
		m_TargetZ = 0.0f;
		m_AmbientColor = Vector3(0, 0, 0);
		m_DiffuseColor = Vector3(0, 0, 0);
		m_SpecularColor = Vector3(0, 0, 0);
		m_Constant = 0.0f;
		m_Linear = 0.0f;
		m_Quadratic = 0.0f;
		m_Exponent = 0.0f;
		m_Cutoff = 0.0f;
		m_Hidden = false;
	}
	else
	{
		Vector3 tmp;

		bool Omni = (light->GetTarget() == NULL);
		bool Spot = (light->GetTarget() != NULL) && (light->GetSpotCutoff() != 180.0f);

		tmp = light->GetPosition();
		lcGetActiveProject()->ConvertToUserUnits(tmp);
		m_PosX = tmp[0];
		m_PosY = tmp[1];
		m_PosZ = tmp[2];

		if (Omni)
		{
			m_TargetX = 0.0f;
			m_TargetY = 0.0f;
			m_TargetZ = 0.0f;
			GetDlgItem(IDC_MODDLG_TARGETX)->EnableWindow(FALSE);
			GetDlgItem(IDC_MODDLG_TARGETY)->EnableWindow(FALSE);
			GetDlgItem(IDC_MODDLG_TARGETZ)->EnableWindow(FALSE);
		}
		else
		{
			tmp = light->GetTargetPosition();
			lcGetActiveProject()->ConvertToUserUnits(tmp);
			m_TargetX = tmp[0];
			m_TargetY = tmp[1];
			m_TargetZ = tmp[2];
		}

		m_AmbientColor = light->GetAmbientColor();
		m_DiffuseColor = light->GetDiffuseColor();
		m_SpecularColor = light->GetSpecularColor();

		if (Omni || Spot)
		{
			m_Constant = light->GetConstantAttenuation();
			m_Linear = light->GetLinearAttenuation();
			m_Quadratic = light->GetQuadraticAttenuation();
		}
		else
		{
			m_Constant = 1.0f;
			m_Linear = 0.0f;
			m_Quadratic = 0.0f;
			GetDlgItem(IDC_MODDLG_CONSTANT)->EnableWindow(FALSE);
			GetDlgItem(IDC_MODDLG_LINEAR)->EnableWindow(FALSE);
			GetDlgItem(IDC_MODDLG_QUADRATIC)->EnableWindow(FALSE);
		}

		m_Cutoff = light->GetSpotCutoff();
		m_Exponent = light->GetSpotExponent();

		m_Hidden = !light->IsVisible();
	}

	UpdateData(FALSE);
}

void CModifyLightDlg::Apply(Light* light)
{
	UpdateData(TRUE);

	LC_LIGHT_MODIFY mod;

	mod.light = light;
	mod.Position = Vector3(m_PosX, m_PosY, m_PosZ);
	lcGetActiveProject()->ConvertFromUserUnits(mod.Position);
	mod.Target = Vector3(m_TargetX, m_TargetY, m_TargetZ);
	lcGetActiveProject()->ConvertFromUserUnits(mod.Target);

	mod.AmbientColor = m_AmbientColor;
	mod.DiffuseColor = m_DiffuseColor;
	mod.SpecularColor = m_SpecularColor;
	mod.ConstantAttenuation = m_Constant;
	mod.LinearAttenuation = m_Linear;
	mod.QuadraticAttenuation = m_Quadratic;
	mod.SpotCutoff = m_Cutoff;
	mod.SpotExponent = m_Exponent;
	mod.Hidden = (m_Hidden != FALSE);

	strcpy(mod.name, ((CModifyDialog*)GetParent()->GetParent())->m_strName);

	lcGetActiveProject()->HandleNotify(LC_LIGHT_MODIFIED, (unsigned long)&mod);
}
