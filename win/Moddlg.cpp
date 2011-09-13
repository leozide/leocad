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
#include "matrix.h"
#include "lc_application.h"

#ifdef _DEBUG 
#undef THIS_FILE 
static char THIS_FILE[]=__FILE__; 
#define new DEBUG_NEW 
#endif 

////////////////////////////////////////////////////////////////////// 
// Construction/Destruction 
////////////////////////////////////////////////////////////////////// 

IMPLEMENT_DYNAMIC(CModifyDialog, CPaneDialog) 

BEGIN_MESSAGE_MAP(CModifyDialog, CPaneDialog) 
	//{{AFX_MSG_MAP(CModifyDialog) 
	ON_BN_CLICKED(IDC_MODDLG_PIECE, OnModdlgPiece)
	ON_CBN_SELENDOK(IDC_MODDLG_LIST, OnSelendokModdlgList)
	ON_BN_CLICKED(IDC_MODDLG_APPLY, OnModdlgApply)
	ON_CBN_DROPDOWN(IDC_MODDLG_LIST, OnDropdownModdlgList)
	//}}AFX_MSG_MAP
	ON_COMMAND_RANGE(ID_MODDLG_PIECES, ID_MODDLG_LIGHTS, OnMenuClick)
END_MESSAGE_MAP() 

CModifyDialog::CModifyDialog() 
{
	m_pObject = NULL;
	m_nType = 255;

	//{{AFX_DATA_INIT(CModifyDialog)
	m_fRotX = 0.0f;
	m_fRotY = 0.0f;
	m_fRotZ = 0.0f;
	m_fPosX = 0.0f;
	m_fPosY = 0.0f;
	m_fPosZ = 0.0f;
	m_bHidden = FALSE;
	m_fFOV = 0.0f;
	m_nFrom = 0;
	m_nTo = 0;
	m_strName = _T("");
	m_fUpX = 0.0f;
	m_fUpY = 0.0f;
	m_fUpZ = 0.0f;
	m_fFar = 0.0f;
	m_fNear = 0.0f;
	//}}AFX_DATA_INIT
} 

CModifyDialog::~CModifyDialog() 
{ 
} 
/*
BOOL CModifyDialog::Create(CWnd * pParentWnd, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID) 
{ 
	// Let MFC Create the control 
	if(!CPaneDialog::Create(pParentWnd, lpszTemplateName, nStyle, nID)) 
		return FALSE; 

	// Since there is no WM_INITDIALOG message we have to call 
	// our own InitDialog function ourselves after m_hWnd is valid 
	if(!OnInitDialogBar()) 
		return FALSE; 
	return TRUE; 
} 

BOOL CModifyDialog::Create(CWnd * pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID) 
{ 
	//Let MFC Create the control 
	if(!CPaneDialog::Create(pParentWnd, nIDTemplate, nStyle, nID)) 
		return FALSE; 

	// Since there is no WM_INITDIALOG message we have to call 
	// our own InitDialog function ourselves after m_hWnd is valid 
	if(!OnInitDialogBar()) 
		return FALSE; 
	return TRUE; 
}
*/
BOOL CModifyDialog::OnInitDialogBar() 
{ 
	// Support for the MFC DDX model 
	UpdateData(FALSE); 
	m_ctlCombo.LimitText(80);
	m_ctlColor.SetColorIndex(0);
	UpdateControls(LC_OBJECT_PIECE);
	return TRUE; 
}

void CModifyDialog::DoDataExchange(CDataExchange* pDX) 
{ 
	//Derived Classes Overide this function 
	ASSERT(pDX); 
	
	CPaneDialog::DoDataExchange(pDX); 
	//{{AFX_DATA_MAP(CModifyDialog) 
	DDX_Control(pDX, IDC_MODDLG_LIST, m_ctlCombo);
	DDX_Control(pDX, IDC_MODDLG_COLOR, m_ctlColor);
	DDX_Text(pDX, IDC_MODDLG_ROTX, m_fRotX);
	DDX_Text(pDX, IDC_MODDLG_ROTY, m_fRotY);
	DDX_Text(pDX, IDC_MODDLG_ROTZ, m_fRotZ);
	DDX_Text(pDX, IDC_MODDLG_POSX, m_fPosX);
	DDX_Text(pDX, IDC_MODDLG_POSY, m_fPosY);
	DDX_Text(pDX, IDC_MODDLG_POSZ, m_fPosZ);
	DDX_Check(pDX, IDC_MODDLG_HIDDEN, m_bHidden);
	DDX_Text(pDX, IDC_MODDLG_FOV, m_fFOV);
	DDX_Text(pDX, IDC_MODDLG_FROM, m_nFrom);
	DDX_Text(pDX, IDC_MODDLG_TO, m_nTo);
	DDX_CBString(pDX, IDC_MODDLG_LIST, m_strName);
	DDV_MaxChars(pDX, m_strName, 80);
	DDX_Text(pDX, IDC_MODDLG_UPX, m_fUpX);
	DDX_Text(pDX, IDC_MODDLG_UPY, m_fUpY);
	DDX_Text(pDX, IDC_MODDLG_UPZ, m_fUpZ);
	DDX_Text(pDX, IDC_MODDLG_FAR, m_fFar);
//	DDV_MinMaxFloat(pDX, m_fFar, 5.f, 10000.f);
	DDX_Text(pDX, IDC_MODDLG_NEAR, m_fNear);
//	DDV_MinMaxFloat(pDX, m_fNear, 1.e-002f, 100.f);
	//}}AFX_DATA_MAP
} 

void CModifyDialog::OnUpdateCmdUI(CFrameWnd * pTarget, BOOL /*bDisableIfNoHndler*/)
{
	UpdateDialogControls(pTarget, FALSE);
}

void CModifyDialog::UpdateInfo(Object* pObject)
{
	if (!IsVisible())
		return;

	if (pObject == NULL)
		pObject = lcGetActiveProject()->GetFocusObject();

	m_pObject = pObject;

	if (m_pObject == NULL)
	{
		m_fPosX = m_fPosY = m_fPosZ = 0.0f;
		m_fRotX = m_fRotY = m_fRotZ = 0.0f;
		m_fUpX = m_fUpY = m_fUpZ = 0.0f;
		UpdateData(FALSE);
		return;
	}
	else
	{
		UpdateControls(m_pObject->GetType());
	}

	switch (m_nType)
	{
		case LC_OBJECT_PIECE:
		{
			float rot[4];
			Piece* pPiece = (Piece*)m_pObject;
			Vector3 Pos = pPiece->GetPosition();
			pPiece->GetRotation(rot);
			Matrix mat(rot, Pos);
			mat.ToEulerAngles(rot);

			lcGetActiveProject()->ConvertToUserUnits(Pos);

			m_fPosX = Pos[0];
			m_fPosY = Pos[1];
			m_fPosZ = Pos[2];
			m_fRotX = rot[0];
			m_fRotY = rot[1];
			m_fRotZ = rot[2];

			if (lcGetActiveProject()->IsAnimation())
			{
				m_nFrom = pPiece->GetFrameShow();
				m_nTo = pPiece->GetFrameHide();
			}
			else
			{
				m_nFrom = pPiece->GetStepShow();
				m_nTo = pPiece->GetStepHide();
			}

			m_bHidden = pPiece->IsHidden();
			m_ctlColor.SetColorIndex(pPiece->GetColor());
			UpdateData(FALSE);
			m_ctlCombo.SetWindowText(pPiece->GetName());
		} break;

		case LC_OBJECT_CAMERA:
		case LC_OBJECT_CAMERA_TARGET:
		{
			Vector3 tmp;
			Camera* pCamera;
			if (m_nType == LC_OBJECT_CAMERA)
				pCamera = (Camera*)m_pObject;
			else
				pCamera = ((CameraTarget*)m_pObject)->GetParent();

			tmp = pCamera->GetEyePosition();
			lcGetActiveProject()->ConvertToUserUnits(tmp);
			m_fPosX = tmp[0];
			m_fPosY = tmp[1];
			m_fPosZ = tmp[2];

			tmp = pCamera->GetTargetPosition();
			lcGetActiveProject()->ConvertToUserUnits(tmp);
			m_fRotX = tmp[0];
			m_fRotY = tmp[1];
			m_fRotZ = tmp[2];

			tmp = pCamera->GetUpVector();
			lcGetActiveProject()->ConvertToUserUnits(tmp);
			m_fUpX = tmp[0];
			m_fUpY = tmp[1];
			m_fUpZ = tmp[2];

			m_fFOV = pCamera->m_fovy;
			m_fNear = pCamera->m_zNear;
			m_fFar = pCamera->m_zFar;
			m_bHidden = !pCamera->IsVisible();
			UpdateData(FALSE);
			m_ctlCombo.SetWindowText(pCamera->GetName());
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

void CModifyDialog::UpdateControls(BYTE nType)
{
	if (m_nType == nType)
		return;

	DeleteObject((HBITMAP)SendDlgItemMessage(IDC_MODDLG_PIECE, BM_GETIMAGE, IMAGE_BITMAP, 0));

	UINT id = IDB_PIECE;
	if (nType == LC_OBJECT_CAMERA || nType == LC_OBJECT_CAMERA_TARGET)
		id = IDB_CAMERA;
	if (nType == LC_OBJECT_LIGHT || nType == LC_OBJECT_LIGHT_TARGET)
		id = IDB_LIGHT;
	SendDlgItemMessage(IDC_MODDLG_PIECE, BM_SETIMAGE, IMAGE_BITMAP, 
		(LPARAM)LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(id)));

	m_ctlCombo.SetWindowText("");

	switch (nType)
	{
		case LC_OBJECT_PIECE:
		{
			GetDlgItem(IDC_MODDLG_ROTATION)->SetWindowText(_T("Rotation")); 
			GetDlgItem(IDC_MODDLG_STEPFROM)->SetWindowText(_T("Step"));
			GetDlgItem(IDC_MODDLG_STEPTO)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_MODDLG_FROM)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_MODDLG_TO)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_MODDLG_COLOR)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_MODDLG_FOV)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_MODDLG_FOVSTATIC)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_MODDLG_UPX)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_MODDLG_UPY)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_MODDLG_UPZ)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_MODDLG_PLANESSTATIC)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_MODDLG_NEAR)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_MODDLG_FAR)->ShowWindow(SW_HIDE);
		} break;

		case LC_OBJECT_CAMERA:
		case LC_OBJECT_CAMERA_TARGET:
		{
			GetDlgItem(IDC_MODDLG_ROTATION)->SetWindowText(_T("Target"));
			GetDlgItem(IDC_MODDLG_STEPFROM)->SetWindowText(_T("Up"));
			GetDlgItem(IDC_MODDLG_STEPTO)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_MODDLG_FROM)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_MODDLG_TO)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_MODDLG_COLOR)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_MODDLG_FOV)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_MODDLG_FOVSTATIC)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_MODDLG_UPX)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_MODDLG_UPY)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_MODDLG_UPZ)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_MODDLG_PLANESSTATIC)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_MODDLG_NEAR)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_MODDLG_FAR)->ShowWindow(SW_SHOW);
		} break;

		case LC_OBJECT_LIGHT: case LC_OBJECT_LIGHT_TARGET:
		{
			// TODO: Lights.
		} break;
	}

	m_nType = nType;
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

	switch (m_nType)
	{
		case LC_OBJECT_PIECE: 
		{
			LC_PIECE_MODIFY mod;

			mod.piece = (Piece*)m_pObject;
			mod.Position = Vector3(m_fPosX, m_fPosY, m_fPosZ);
			lcGetActiveProject()->ConvertFromUserUnits(mod.Position);
			mod.Rotation = Vector3(m_fRotX, m_fRotY, m_fRotZ);
			mod.from = m_nFrom;
			mod.to = m_nTo;
			mod.hidden = (m_bHidden != FALSE);
			mod.color = m_ctlColor.GetColorIndex();
			strcpy(mod.name, m_strName);

			lcGetActiveProject()->HandleNotify(LC_PIECE_MODIFIED, (unsigned long)&mod);
		} break;

		case LC_OBJECT_CAMERA: case LC_OBJECT_CAMERA_TARGET:
		{
			LC_CAMERA_MODIFY mod;

			mod.camera = (Camera*)m_pObject;
			mod.hidden = (m_bHidden != FALSE);
			mod.Eye = Vector3(m_fPosX, m_fPosY, m_fPosZ);
			mod.Target = Vector3(m_fRotX, m_fRotY, m_fRotZ);
			mod.Up = Vector3(m_fUpX, m_fUpY, m_fUpZ);
			lcGetActiveProject()->ConvertFromUserUnits(mod.Eye);
			lcGetActiveProject()->ConvertFromUserUnits(mod.Target);
			lcGetActiveProject()->ConvertFromUserUnits(mod.Up);
			mod.fovy = m_fFOV;
			mod.znear = m_fNear;
			mod.zfar = m_fFar;

			lcGetActiveProject()->HandleNotify(LC_CAMERA_MODIFIED, (unsigned long)&mod);
		} break;

		case LC_OBJECT_LIGHT: case LC_OBJECT_LIGHT_TARGET:
		{
			// TODO: Lights.
		} break;
	}
//	pDoc->UpdateAllViews(NULL);
}

/*
void CModifyDialog::OnModdlgClose() 
{
	AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_VIEW_MODIFY_BAR);
}
*/

void CModifyDialog::OnDropdownModdlgList() 
{
	Piece* pPiece;
	Camera* pCamera;
	Light* pLight;
	int i;

	lcGetActiveProject()->GetArrays(&pPiece, &pCamera, &pLight);

	m_ctlCombo.ResetContent();

	switch (m_nType)
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
