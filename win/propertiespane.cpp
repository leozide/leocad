// propertiespane.cpp : implementation file
//

#include "stdafx.h"
#include "LeoCAD.h"
#include "propertiespane.h"

#include "project.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "matrix.h"
#include "lc_application.h"

BEGIN_MESSAGE_MAP(CPropertiesPane, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, OnPropertyChanged)
END_MESSAGE_MAP()

inline void UpdateProperty(CMFCPropertyGridProperty* Property, float Value)
{
	const COleVariant& Var = Property->GetValue();

	if (Var.vt != VT_EMPTY && Var.vt != VT_R4)
	{
		ASSERT(FALSE);
		return;
	}

	if ((float)Var.fltVal != Value)
		Property->SetValue((_variant_t)Value);
}

inline void UpdateProperty(CMFCPropertyGridProperty* Property, lcuint32 Value)
{
	const COleVariant& Var = Property->GetValue();

	if (Var.vt != VT_EMPTY && Var.vt != VT_UINT)
	{
		ASSERT(FALSE);
		return;
	}

	if (Var.ulVal != Value)
		Property->SetValue((_variant_t)Value);
}

inline void UpdateProperty(CMFCPropertyGridProperty* Property, bool Value)
{
	const COleVariant& Var = Property->GetValue();

	if (Var.vt != VT_EMPTY && Var.vt != VT_BOOL)
	{
		ASSERT(FALSE);
		return;
	}

	if (Var.boolVal != (VARIANT_BOOL)Value)
		Property->SetValue((_variant_t)Value);
}

CPropertiesPane::CPropertiesPane()
{
	mObject = NULL;
}

CPropertiesPane::~CPropertiesPane()
{
}

void CPropertiesPane::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
		return;
	CRect rectClient;
	GetClientRect(rectClient);
	m_wndPropList.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
}

int CPropertiesPane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	if (!m_wndPropList.Create(WS_VISIBLE | WS_CHILD, rectDummy, this, 2))
	{
		TRACE0("Failed to create Properties Grid \n");
		return -1;      // fail to create
	}

	InitPropList();

	AdjustLayout();
	return 0;
}

void CPropertiesPane::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CPropertiesPane::InitPropList()
{
	SetPropListFont();

	m_wndPropList.EnableHeaderCtrl(FALSE);
	m_wndPropList.EnableDescriptionArea();
	m_wndPropList.SetVSDotNetLook();
	m_wndPropList.MarkModifiedProperties();

	SetEmpty(true);
}

void CPropertiesPane::Update(Object* Focus)
{
	if (!Focus)
		SetEmpty();
	else
	{
		switch (Focus->GetType())
		{
		case LC_OBJECT_PIECE:
			SetPiece(Focus);
			break;

		case LC_OBJECT_CAMERA:
			SetCamera(Focus);
			break;

		case LC_OBJECT_CAMERA_TARGET:
			SetCamera(((CameraTarget*)Focus)->GetParent());
			break;

		case LC_OBJECT_LIGHT:
			SetLight(Focus);
			break;

		case LC_OBJECT_LIGHT_TARGET:
			SetLight(((LightTarget*)Focus)->GetParent());
			break;
		}
	}
}

void CPropertiesPane::SetEmpty(bool Force)
{
	if (!mObject && !Force)
		return;

	m_wndPropList.RemoveAll();

	CMFCPropertyGridProperty* Empty = new CMFCPropertyGridProperty(_T("Nothing selected"));

	m_wndPropList.AddProperty(Empty);

	mObject = NULL;
}

void CPropertiesPane::SetPiece(Object* Focus)
{
	if (!mObject || mObject->GetType() != LC_OBJECT_PIECE)
	{
		m_wndPropList.RemoveAll();

		CMFCPropertyGridProperty* Position = new CMFCPropertyGridProperty(_T("Position"));

		CMFCPropertyGridProperty* PosX = new CMFCPropertyGridProperty(_T("X"), (_variant_t)0.0f, _T("The object's X coordinate"));
		Position->AddSubItem(PosX);

		CMFCPropertyGridProperty* PosY = new CMFCPropertyGridProperty(_T("Y"), (_variant_t)0.0f, _T("The object's Y coordinate"));
		Position->AddSubItem(PosY);

		CMFCPropertyGridProperty* PosZ = new CMFCPropertyGridProperty(_T("Z"), (_variant_t)0.0f, _T("The object's Z coordinate"));
		Position->AddSubItem(PosZ);

		m_wndPropList.AddProperty(Position);

		CMFCPropertyGridProperty* Rotation = new CMFCPropertyGridProperty(_T("Rotation"));

		CMFCPropertyGridProperty* RotX = new CMFCPropertyGridProperty(_T("X"), (_variant_t)0.0f, _T("The object's rotation around the X axis"));
		Rotation->AddSubItem(RotX);

		CMFCPropertyGridProperty* RotY = new CMFCPropertyGridProperty(_T("Y"), (_variant_t)0.0f, _T("The object's rotation around the Y axis"));
		Rotation->AddSubItem(RotY);

		CMFCPropertyGridProperty* RotZ = new CMFCPropertyGridProperty(_T("Z"), (_variant_t)0.0f, _T("The object's rotation around the Z axis"));
		Rotation->AddSubItem(RotZ);

		m_wndPropList.AddProperty(Rotation);

		CMFCPropertyGridProperty* Appearence = new CMFCPropertyGridProperty(_T("Appearance"));

		CMFCPropertyGridProperty* Show = new CMFCPropertyGridProperty(_T("Show"), (_variant_t)(lcuint32)0, _T("The time when this object becomes visible"));
		Appearence->AddSubItem(Show);

		CMFCPropertyGridProperty* Hide = new CMFCPropertyGridProperty(_T("Hide"), (_variant_t)(lcuint32)0, _T("The time when this object is hidden"));
		Appearence->AddSubItem(Hide);

		CMFCPropertyGridColorProperty* Color = new CMFCPropertyGridColorProperty(_T("Color"), RGB(210, 192, 254), NULL, _T("The object's color")); // TODO: only use LeoCAD colors on the color picker
		Color->EnableAutomaticButton(_T("Default"), ::GetSysColor(COLOR_3DFACE));
		Appearence->AddSubItem(Color);

		m_wndPropList.AddProperty(Appearence);
	}

	float Rot[4];
	Piece* pPiece = (Piece*)Focus;
	Vector3 Pos = pPiece->GetPosition();
	pPiece->GetRotation(Rot);
	Matrix mat(Rot, Pos);
	mat.ToEulerAngles(Rot);

	lcGetActiveProject()->ConvertToUserUnits(Pos);

	CMFCPropertyGridProperty* Position = m_wndPropList.GetProperty(0);
	UpdateProperty(Position->GetSubItem(0), Pos[0]);
	UpdateProperty(Position->GetSubItem(1), Pos[1]);
	UpdateProperty(Position->GetSubItem(2), Pos[2]);

	CMFCPropertyGridProperty* Rotation = m_wndPropList.GetProperty(1);
	UpdateProperty(Rotation->GetSubItem(0), Rot[0]);
	UpdateProperty(Rotation->GetSubItem(1), Rot[1]);
	UpdateProperty(Rotation->GetSubItem(2), Rot[2]);

	lcuint32 From, To;
	if (lcGetActiveProject()->IsAnimation())
	{
		From = pPiece->GetFrameShow();
		To = pPiece->GetFrameHide();
	}
	else
	{
		From = pPiece->GetStepShow();
		To = pPiece->GetStepHide();
	}

	CMFCPropertyGridProperty* Appearence = m_wndPropList.GetProperty(2);
	UpdateProperty(Appearence->GetSubItem(0), From);
	UpdateProperty(Appearence->GetSubItem(1), To);

	mObject = Focus;
}

void CPropertiesPane::ModifyPiece()
{
	LC_PIECE_MODIFY Modify;

	Modify.piece = (Piece*)mObject;

	CMFCPropertyGridProperty* Position = m_wndPropList.GetProperty(0);
	Modify.Position = Vector3(Position->GetSubItem(0)->GetValue().fltVal, Position->GetSubItem(1)->GetValue().fltVal, Position->GetSubItem(2)->GetValue().fltVal);
	lcGetActiveProject()->ConvertFromUserUnits(Modify.Position);

	CMFCPropertyGridProperty* Rotation = m_wndPropList.GetProperty(1);
	Modify.Rotation = Vector3(Rotation->GetSubItem(0)->GetValue().fltVal, Rotation->GetSubItem(1)->GetValue().fltVal, Rotation->GetSubItem(2)->GetValue().fltVal);

	CMFCPropertyGridProperty* Appearence = m_wndPropList.GetProperty(2);
	Modify.from = Appearence->GetSubItem(0)->GetValue().ulVal;
	Modify.to = Appearence->GetSubItem(1)->GetValue().ulVal;
	Modify.hidden = FALSE;
	Modify.color = Modify.piece->GetColor();
	strcpy(Modify.name, Modify.piece->GetName());

	lcGetActiveProject()->HandleNotify(LC_PIECE_MODIFIED, (unsigned long)&Modify);
}

void CPropertiesPane::SetCamera(Object* Focus)
{
	if (!mObject || mObject->GetType() != LC_OBJECT_CAMERA)
	{
		m_wndPropList.RemoveAll();

		CMFCPropertyGridProperty* Position = new CMFCPropertyGridProperty(_T("Position"));

		CMFCPropertyGridProperty* PosX = new CMFCPropertyGridProperty(_T("X"), (_variant_t)0.0f, _T("The camera's X coordinate"));
		Position->AddSubItem(PosX);

		CMFCPropertyGridProperty* PosY = new CMFCPropertyGridProperty(_T("Y"), (_variant_t)0.0f, _T("The camera's Y coordinate"));
		Position->AddSubItem(PosY);

		CMFCPropertyGridProperty* PosZ = new CMFCPropertyGridProperty(_T("Z"), (_variant_t)0.0f, _T("The camera's Z coordinate"));
		Position->AddSubItem(PosZ);

		m_wndPropList.AddProperty(Position);

		CMFCPropertyGridProperty* Target = new CMFCPropertyGridProperty(_T("Target"));

		CMFCPropertyGridProperty* TargetX = new CMFCPropertyGridProperty(_T("X"), (_variant_t)0.0f, _T("The camera target's X coordinate"));
		Target->AddSubItem(TargetX);

		CMFCPropertyGridProperty* TargetY = new CMFCPropertyGridProperty(_T("Y"), (_variant_t)0.0f, _T("The camera target's Y coordinate"));
		Target->AddSubItem(TargetY);

		CMFCPropertyGridProperty* TargetZ = new CMFCPropertyGridProperty(_T("Z"), (_variant_t)0.0f, _T("The camera target's Z coordinate"));
		Target->AddSubItem(TargetZ);

		m_wndPropList.AddProperty(Target);

		CMFCPropertyGridProperty* Up = new CMFCPropertyGridProperty(_T("Up"));

		CMFCPropertyGridProperty* UpX = new CMFCPropertyGridProperty(_T("X"), (_variant_t)0.0f, _T("The camera's up vector X"));
		Up->AddSubItem(UpX);

		CMFCPropertyGridProperty* UpY = new CMFCPropertyGridProperty(_T("Y"), (_variant_t)0.0f, _T("The camera's up vector Y"));
		Up->AddSubItem(UpY);

		CMFCPropertyGridProperty* UpZ = new CMFCPropertyGridProperty(_T("Z"), (_variant_t)0.0f, _T("The camera's up vector Z"));
		Up->AddSubItem(UpZ);

		m_wndPropList.AddProperty(Up);

		CMFCPropertyGridProperty* Settings = new CMFCPropertyGridProperty(_T("Settings"));

		CMFCPropertyGridProperty* FOV = new CMFCPropertyGridProperty(_T("FOV"), (_variant_t)0.0f, _T("The camera's field of view"));
		Settings->AddSubItem(FOV);

		CMFCPropertyGridProperty* Near = new CMFCPropertyGridProperty(_T("Near"), (_variant_t)0.0f, _T("The camera's near plane"));
		Settings->AddSubItem(Near);

		CMFCPropertyGridProperty* Far = new CMFCPropertyGridProperty(_T("Far"), (_variant_t)0.0f, _T("The camera's far plane"));
		Settings->AddSubItem(Far);

		CMFCPropertyGridProperty* Visible = new CMFCPropertyGridProperty(_T("Visible"), (_variant_t)true, _T("Draw the camera"));
		Settings->AddSubItem(Visible);

		m_wndPropList.AddProperty(Settings);
	}

	Camera* pCamera = (Camera*)Focus;

	Vector3 Pos = pCamera->GetEyePosition();
	lcGetActiveProject()->ConvertToUserUnits(Pos);

	CMFCPropertyGridProperty* Position = m_wndPropList.GetProperty(0);
	UpdateProperty(Position->GetSubItem(0), Pos[0]);
	UpdateProperty(Position->GetSubItem(1), Pos[1]);
	UpdateProperty(Position->GetSubItem(2), Pos[2]);

	Vector3 Target = pCamera->GetTargetPosition();
	lcGetActiveProject()->ConvertToUserUnits(Target);

	CMFCPropertyGridProperty* TargetProp = m_wndPropList.GetProperty(1);
	UpdateProperty(TargetProp->GetSubItem(0), Target[0]);
	UpdateProperty(TargetProp->GetSubItem(1), Target[1]);
	UpdateProperty(TargetProp->GetSubItem(2), Target[2]);

	Vector3 Up = pCamera->GetUpVector();

	CMFCPropertyGridProperty* UpProp = m_wndPropList.GetProperty(2);
	UpdateProperty(UpProp->GetSubItem(0), Up[0]);
	UpdateProperty(UpProp->GetSubItem(1), Up[1]);
	UpdateProperty(UpProp->GetSubItem(2), Up[2]);

	CMFCPropertyGridProperty* SettingsProp = m_wndPropList.GetProperty(3);
	UpdateProperty(SettingsProp->GetSubItem(0), pCamera->m_fovy);
	UpdateProperty(SettingsProp->GetSubItem(1), pCamera->m_zNear);
	UpdateProperty(SettingsProp->GetSubItem(2), pCamera->m_zFar);
	UpdateProperty(SettingsProp->GetSubItem(3), pCamera->IsVisible());

	mObject = Focus;
}

void CPropertiesPane::ModifyCamera()
{
}

void CPropertiesPane::SetLight(Object* Focus)
{
	if (!mObject || mObject->GetType() != LC_OBJECT_LIGHT)
	{
		m_wndPropList.RemoveAll();

		CMFCPropertyGridProperty* Position = new CMFCPropertyGridProperty(_T("Position"));

		CMFCPropertyGridProperty* PosX = new CMFCPropertyGridProperty(_T("X"), (_variant_t)0.0f, _T("The object's X coordinate"));
		Position->AddSubItem(PosX);

		CMFCPropertyGridProperty* PosY = new CMFCPropertyGridProperty(_T("Y"), (_variant_t)0.0f, _T("The object's Y coordinate"));
		Position->AddSubItem(PosY);

		CMFCPropertyGridProperty* PosZ = new CMFCPropertyGridProperty(_T("Z"), (_variant_t)0.0f, _T("The object's Z coordinate"));
		Position->AddSubItem(PosZ);

		m_wndPropList.AddProperty(Position);
	}
}

void CPropertiesPane::ModifyLight()
{
}

void CPropertiesPane::SetPropListFont()
{
	::DeleteObject(m_fntPropList.Detach());

	LOGFONT lf;
	afxGlobalData.fontRegular.GetLogFont(&lf);

	NONCLIENTMETRICS info;
	info.cbSize = sizeof(info);

	afxGlobalData.GetNonClientMetrics(info);

	lf.lfHeight = info.lfMenuFont.lfHeight;
	lf.lfWeight = info.lfMenuFont.lfWeight;
	lf.lfItalic = info.lfMenuFont.lfItalic;

	m_fntPropList.CreateFontIndirect(&lf);

	m_wndPropList.SetFont(&m_fntPropList);
}

LRESULT CPropertiesPane::OnPropertyChanged(WPARAM wParam, LPARAM lParam )
{
	CMFCPropertyGridProperty* Property = (CMFCPropertyGridProperty*)lParam;

	if (!mObject)
		return 0;

	switch (mObject->GetType())
	{
	case LC_OBJECT_PIECE:
		ModifyPiece();
		break;

	case LC_OBJECT_CAMERA:
	case LC_OBJECT_CAMERA_TARGET:
		ModifyCamera();

	case LC_OBJECT_LIGHT:
	case LC_OBJECT_LIGHT_TARGET:
		ModifyLight();
	}

	return 0;
}
