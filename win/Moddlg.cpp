// ModDlg.cpp: implementation of the CModifyDialog class. 
// 
////////////////////////////////////////////////////////////////////// 

#include "stdafx.h" 
#include "leocad.h" 
#include "ModDlg.h" 

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

	CRect rc;
	m_ModifyDlg.GetClientRect(&rc);

	m_szMinHorz.cx = m_szMinVert.cx = rc.Width();
	m_szMinFloat.cx = rc.Width() + 4;

	m_szHorz = m_szVert = m_szFloat = CSize(rc.Width(), rc.Height());

	return 0;
}

////////////////////////////////////////////////////////////////////// 
// Construction/Destruction 
////////////////////////////////////////////////////////////////////// 

IMPLEMENT_DYNAMIC(CModifyDialog, CDialog) 

BEGIN_MESSAGE_MAP(CModifyDialog, CDialog) 
	//{{AFX_MSG_MAP(CModifyDialog) 
	ON_BN_CLICKED(IDC_MODDLG_PIECE, OnModdlgPiece)
	ON_CBN_SELENDOK(IDC_MODDLG_LIST, OnSelendokModdlgList)
	ON_BN_CLICKED(IDC_MODDLG_APPLY, OnModdlgApply)
	ON_CBN_DROPDOWN(IDC_MODDLG_LIST, OnDropdownModdlgList)
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
			// TODO: Lights.
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

	m_RollUp.MoveWindow(StaticRect.left, StaticRect.top, ClientRect.Width() - StaticRect.left * 2, ClientRect.Height() - StaticRect.top, TRUE);

	m_ctlCombo.GetWindowRect(&StaticRect);
	ScreenToClient(&StaticRect);
	m_ctlCombo.MoveWindow(StaticRect.left, StaticRect.top, ClientRect.Width() - StaticRect.left - 4, StaticRect.Height(), TRUE);

/*
	CRect Rect;

	if (!IsWindow(m_PieceDlg.m_hWnd))
		return;

	GetDlgItem(IDC_MODIFY_CHILD)->GetWindowRect(&Rect);

	// Recompute coordinates relative to parent window.
	ScreenToClient(&Rect);

	// Now move the child windows.
	m_PieceDlg.MoveWindow(Rect.left, Rect.top, Rect.Width(), Rect.Height(), TRUE);
	m_CameraDlg.MoveWindow(Rect.left, Rect.top, Rect.Width(), Rect.Height(), TRUE);
	m_LightDlg.MoveWindow(Rect.left, Rect.top, Rect.Width(), Rect.Height(), TRUE);
	*/
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
			// TODO: Lights.
			break;
	}
}

void CModifyDialog::OnDropdownModdlgList() 
{
	Piece* pPiece;
	Camera* pCamera;
	Light* pLight;
	int i;

	lcGetActiveProject()->GetArrays(&pPiece, &pCamera, &pLight);

	m_ctlCombo.ResetContent();

	switch (m_CurrentType)
	{
		case LC_OBJECT_PIECE:
		{
			for (; pPiece; pPiece = pPiece->m_pNext)
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
			for (; pCamera; pCamera = pCamera->m_pNext)
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
			for (; pLight; pLight = pLight->m_pNext)
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
		double d;
		if (_stscanf(szBuffer, _T("%lf"), &d) != 1)
		{
			AfxMessageBox(AFX_IDP_PARSE_REAL);
			pDX->Fail();            // throws exception
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
END_MESSAGE_MAP()


// CModifyPieceDlg message handlers
void CModifyPieceDlg::OnOK() 
{
}

void CModifyPieceDlg::OnCancel() 
{
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
		if (lcGetActiveProject()->IsAnimation())
		{
			m_From = piece->GetFrameShow();
			m_To = piece->GetFrameHide();
		}
		else
		{
			m_From = piece->GetStepShow();
			m_To = piece->GetStepHide();
		}

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
	strcpy(mod.name, ((CModifyDialog*)GetParent())->m_strName);

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
	m_UpX = 0.0f;
	m_UpY = 0.0f;
	m_UpZ = 0.0f;
	m_FOV = 0.0f;
	m_Near = 0.0f;
	m_Far = 0.0f;
	m_Ortho = false;
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
	DDX_Text_Float(pDX, IDC_MODDLG_UPX, m_UpX);
	DDX_Text_Float(pDX, IDC_MODDLG_UPY, m_UpY);
	DDX_Text_Float(pDX, IDC_MODDLG_UPZ, m_UpZ);
	DDX_Text_Float(pDX, IDC_MODDLG_FOV, m_FOV);
	DDX_Text(pDX, IDC_MODDLG_NEAR, m_Near);
	DDX_Text(pDX, IDC_MODDLG_FAR, m_Far);
	DDX_Check(pDX, IDC_MODDLG_ORTHO, m_Ortho);
	DDX_Check(pDX, IDC_MODDLG_HIDDEN, m_Hidden);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CModifyCameraDlg, CDialog)
END_MESSAGE_MAP()


// CModifyCameraDlg message handlers
void CModifyCameraDlg::OnOK() 
{
}

void CModifyCameraDlg::OnCancel() 
{
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
		m_UpX = 0.0f;
		m_UpY = 0.0f;
		m_UpZ = 0.0f;
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

		tmp = camera->GetUpVector();
		lcGetActiveProject()->ConvertToUserUnits(tmp);
		m_UpX = tmp[0];
		m_UpY = tmp[1];
		m_UpZ = tmp[2];

		m_FOV = camera->m_fovy;
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
	mod.Up = Vector3(m_UpX, m_UpY, m_UpZ);
	mod.fovy = m_FOV;
	mod.znear = m_Near;
	mod.zfar = m_Far;
	mod.hidden = (m_Hidden != FALSE);
	mod.ortho = (m_Ortho != FALSE);
	strcpy(mod.name, ((CModifyDialog*)GetParent())->m_strName);

	lcGetActiveProject()->HandleNotify(LC_CAMERA_MODIFIED, (unsigned long)&mod);
}

// CModifyLightDlg dialog
IMPLEMENT_DYNAMIC(CModifyLightDlg, CDialog)

CModifyLightDlg::CModifyLightDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CModifyLightDlg::IDD, pParent)
{
}

CModifyLightDlg::~CModifyLightDlg()
{
}

void CModifyLightDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CModifyLightDlg, CDialog)
END_MESSAGE_MAP()


// CModifyLightDlg message handlers
void CModifyLightDlg::OnOK() 
{
}

void CModifyLightDlg::OnCancel() 
{
}

