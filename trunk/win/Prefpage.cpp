// PrefPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PrefPage.h"
#include "Tools.h"
#include "defines.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CPreferencesGeneral, CPropertyPage)
IMPLEMENT_DYNCREATE(CPreferencesDetail, CPropertyPage)
IMPLEMENT_DYNCREATE(CPreferencesDrawing, CPropertyPage)
IMPLEMENT_DYNCREATE(CPreferencesScene, CPropertyPage)
IMPLEMENT_DYNCREATE(CPreferencesPrint, CPropertyPage)

/////////////////////////////////////////////////////////////////////////////
// CPreferencesGeneral property page

CPreferencesGeneral::CPreferencesGeneral() : CPropertyPage(CPreferencesGeneral::IDD)
{
	//{{AFX_DATA_INIT(CPreferencesGeneral)
	m_bZoom = FALSE;
	m_bSubparts = FALSE;
	m_bPreview = FALSE;
	m_nSaveTime = 0;
	m_bNumbers = FALSE;
	m_bGroup = FALSE;
	m_strFolder = _T("");
	m_bAutoSave = FALSE;
	m_bCombo = FALSE;
	//}}AFX_DATA_INIT
}

CPreferencesGeneral::~CPreferencesGeneral()
{
}

void CPreferencesGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencesGeneral)
	DDX_Control(pDX, IDC_GENDLG_MOUSE, m_ctlMouse);
	DDX_Check(pDX, IDC_GENDLG_ZOOM, m_bZoom);
	DDX_Check(pDX, IDC_GENDLG_SUBPARTS, m_bSubparts);
	DDX_Check(pDX, IDC_GENDLG_PREVIEW, m_bPreview);
	DDX_Text(pDX, IDC_GENDLG_SAVETIME, m_nSaveTime);
	DDV_MinMaxInt(pDX, m_nSaveTime, 1, 60);
	DDX_Check(pDX, IDC_GENDLG_NUMBERS, m_bNumbers);
	DDX_Check(pDX, IDC_GENDLG_GROUP, m_bGroup);
	DDX_Text(pDX, IDC_GENDLG_FOLDER, m_strFolder);
	DDX_Check(pDX, IDC_GENDLG_AUTOSAVE, m_bAutoSave);
	DDX_Check(pDX, IDC_GENDLG_COMBO, m_bCombo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPreferencesGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(CPreferencesGeneral)
	ON_BN_CLICKED(IDC_GENDLG_FOLDERBTN, OnFolderBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CPreferencesGeneral::OnFolderBrowse() 
{
	CString str;
	UpdateData(TRUE);
	if (FolderBrowse(&str, _T("Select default folder"), GetSafeHwnd()))
	{
		m_strFolder = str;
		UpdateData(FALSE);
	}
}

void CPreferencesGeneral::SetOptions(int nSaveInterval, int nMouse, char* strFolder)
{
	m_nSaveTime = nSaveInterval & ~LC_AUTOSAVE_FLAG;
	m_bAutoSave = (nSaveInterval & LC_AUTOSAVE_FLAG) != 0;
	m_nMouse = nMouse;
	m_strFolder = strFolder;

	int i = AfxGetApp()->GetProfileInt("Settings", "Piecebar Options", 
		PIECEBAR_PREVIEW|PIECEBAR_GROUP|PIECEBAR_COMBO|PIECEBAR_ZOOMPREVIEW);
	m_bPreview = (i & PIECEBAR_PREVIEW) != 0;
	m_bSubparts = (i & PIECEBAR_SUBPARTS) != 0;
	m_bGroup = (i & PIECEBAR_GROUP) != 0;
	m_bCombo = (i & PIECEBAR_COMBO) != 0;
	m_bNumbers = (i & PIECEBAR_PARTNUMBERS) != 0;
	m_bZoom = (i & PIECEBAR_ZOOMPREVIEW) != 0;
}

void CPreferencesGeneral::GetOptions(int* nSaveTime, int* nMouse, char* strFolder)
{
	if (m_bAutoSave) m_nSaveTime |= LC_AUTOSAVE_FLAG;
	*nSaveTime = m_nSaveTime;
	*nMouse = m_nMouse;
	strcpy(strFolder, m_strFolder);

	int i = 0;
	if (m_bPreview) i |= PIECEBAR_PREVIEW;
	if (m_bSubparts) i |= PIECEBAR_SUBPARTS;
	if (m_bGroup) i |= PIECEBAR_GROUP;
	if (m_bCombo) i |= PIECEBAR_COMBO;
	if (m_bNumbers) i |= PIECEBAR_PARTNUMBERS;
	if (m_bZoom) i |= PIECEBAR_ZOOMPREVIEW;
		
	AfxGetApp()->WriteProfileInt("Settings", "Piecebar Options", i);
}

BOOL CPreferencesGeneral::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	m_ctlMouse.SetRange(1,20);
	m_ctlMouse.SetPos(m_nMouse);
	
	return TRUE;
}

void CPreferencesGeneral::OnOK() 
{
	m_nMouse = m_ctlMouse.GetPos();
	CPropertyPage::OnOK();
}


/////////////////////////////////////////////////////////////////////////////
// CPreferencesDetail property page

CPreferencesDetail::CPreferencesDetail() : CPropertyPage(CPreferencesDetail::IDD)
{
	//{{AFX_DATA_INIT(CPreferencesDetail)
	m_bAntialiasing = FALSE;
	m_bDithering = FALSE;
	m_bEdges = FALSE;
	m_bLighting = FALSE;
	m_bLinear = FALSE;
	m_bSmooth = FALSE;
	m_fLineWidth = 0.0f;
	m_bBackground = FALSE;
	m_bFast = FALSE;
	m_bHidden = FALSE;
	m_bSolid = FALSE;
	m_bNoAlpha = FALSE;
	//}}AFX_DATA_INIT
}

CPreferencesDetail::~CPreferencesDetail()
{
}

void CPreferencesDetail::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencesDetail)
	DDX_Check(pDX, IDC_DETDLG_ANTIALIAS, m_bAntialiasing);
	DDX_Check(pDX, IDC_DETDLG_DITHER, m_bDithering);
	DDX_Check(pDX, IDC_DETDLG_EDGES, m_bEdges);
	DDX_Check(pDX, IDC_DETDLG_LIGHTING, m_bLighting);
	DDX_Check(pDX, IDC_DETDLG_LINEAR, m_bLinear);
	DDX_Check(pDX, IDC_DETDLG_SMOOTH, m_bSmooth);
	DDX_Text(pDX, IDC_DETDLG_LINE, m_fLineWidth);
	DDV_MinMaxFloat(pDX, m_fLineWidth, 0.f, 10.f);
	DDX_Check(pDX, IDC_DETDLG_BACKGROUND, m_bBackground);
	DDX_Check(pDX, IDC_DETDLG_FAST, m_bFast);
	DDX_Check(pDX, IDC_DETDLG_HIDDEN, m_bHidden);
	DDX_Check(pDX, IDC_DETDLG_SOLID, m_bSolid);
	DDX_Check(pDX, IDC_DETDLG_NOALPHA, m_bNoAlpha);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPreferencesDetail, CPropertyPage)
	//{{AFX_MSG_MAP(CPreferencesDetail)
	ON_BN_CLICKED(IDC_DETDLG_FAST, OnFastClick)
	ON_BN_CLICKED(IDC_DETDLG_SOLID, OnDetdlgSolid)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CPreferencesDetail::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	OnFastClick();
	OnDetdlgSolid();
	return TRUE;
}

void CPreferencesDetail::SetOptions(DWORD dwDetail, float fLine)
{
	m_bAntialiasing = (dwDetail & LC_DET_ANTIALIAS) != 0;
	m_bDithering = (dwDetail & LC_DET_DITHER) != 0;
	m_bEdges = (dwDetail & LC_DET_BRICKEDGES) != 0;
	m_bLighting = (dwDetail & LC_DET_LIGHTING) != 0;
	m_bLinear =	(dwDetail & LC_DET_LINEAR) != 0;
	m_bSmooth =	(dwDetail & LC_DET_SMOOTH) != 0;
	m_bBackground = (dwDetail & LC_DET_BACKGROUND) != 0;
	m_bFast = (dwDetail & LC_DET_FAST) != 0;
	m_bHidden = (dwDetail & LC_DET_HIDDEN_LINE) != 0;
	m_bSolid = (dwDetail & LC_DET_BOX_FILL) != 0;
	m_bNoAlpha = (dwDetail & LC_DET_SCREENDOOR) != 0;
	m_fLineWidth = fLine;
}

void CPreferencesDetail::GetOptions(DWORD* dwDetail, float* fLine)
{
	*dwDetail = 0;
	if (m_bAntialiasing) *dwDetail |= LC_DET_ANTIALIAS;
	if (m_bDithering) *dwDetail |= LC_DET_DITHER;
	if (m_bEdges) *dwDetail |= LC_DET_BRICKEDGES;
	if (m_bLighting) *dwDetail |= LC_DET_LIGHTING;
	if (m_bLinear) *dwDetail |= LC_DET_LINEAR;
	if (m_bSmooth) *dwDetail |= LC_DET_SMOOTH;
	if (m_bBackground) *dwDetail |= LC_DET_BACKGROUND;
	if (m_bFast) *dwDetail |= LC_DET_FAST;
	if (m_bHidden) *dwDetail |= LC_DET_HIDDEN_LINE;
	if (m_bSolid) *dwDetail |= LC_DET_BOX_FILL;
	if (m_bNoAlpha) *dwDetail |= LC_DET_SCREENDOOR;
	*fLine = m_fLineWidth;
}

void CPreferencesDetail::OnFastClick() 
{
	UpdateData (TRUE);
	if (m_bFast)
		GetDlgItem (IDC_DETDLG_BACKGROUND)->EnableWindow(TRUE);
	else
	{
		m_bBackground = FALSE;
		UpdateData(FALSE);
		GetDlgItem (IDC_DETDLG_BACKGROUND)->EnableWindow(FALSE);
	}
}

void CPreferencesDetail::OnDetdlgSolid() 
{
	UpdateData (TRUE);
	GetDlgItem (IDC_DETDLG_HIDDEN)->EnableWindow(!m_bSolid);
}

/////////////////////////////////////////////////////////////////////////////
// CPreferencesDrawing property page

CPreferencesDrawing::CPreferencesDrawing() : CPropertyPage(CPreferencesDrawing::IDD)
{
	//{{AFX_DATA_INIT(CPreferencesDrawing)
	m_nAngle = 0;
	m_bAxis = FALSE;
	m_bCentimeters = FALSE;
	m_bCollision = FALSE;
	m_bFixed = FALSE;
	m_bGrid = FALSE;
	m_nGridSize = 0;
	m_bLockX = FALSE;
	m_bLockY = FALSE;
	m_bLockZ = FALSE;
	m_bMove = FALSE;
	m_bPreview = FALSE;
	m_bSnapA = FALSE;
	m_bSnapX = FALSE;
	m_bSnapY = FALSE;
	m_bSnapZ = FALSE;
	m_b3DMouse = FALSE;
	//}}AFX_DATA_INIT
}

CPreferencesDrawing::~CPreferencesDrawing()
{
}

void CPreferencesDrawing::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencesDrawing)
	DDX_Text(pDX, IDC_AIDDLG_ANGLE, m_nAngle);
	DDV_MinMaxInt(pDX, m_nAngle, 1, 180);
	DDX_Check(pDX, IDC_AIDDLG_AXIS, m_bAxis);
	DDX_Check(pDX, IDC_AIDDLG_CENTIMETERS, m_bCentimeters);
	DDX_Check(pDX, IDC_AIDDLG_COLLISION, m_bCollision);
	DDX_Check(pDX, IDC_AIDDLG_FIXEDKEYS, m_bFixed);
	DDX_Check(pDX, IDC_AIDDLG_GRID, m_bGrid);
	DDX_Text(pDX, IDC_AIDDLG_GRIDSIZE, m_nGridSize);
	DDX_Check(pDX, IDC_AIDDLG_LOCKX, m_bLockX);
	DDX_Check(pDX, IDC_AIDDLG_LOCKY, m_bLockY);
	DDX_Check(pDX, IDC_AIDDLG_LOCKZ, m_bLockZ);
	DDX_Check(pDX, IDC_AIDDLG_MOVE, m_bMove);
	DDX_Check(pDX, IDC_AIDDLG_PREVIEW, m_bPreview);
	DDX_Check(pDX, IDC_AIDDLG_SNAPA, m_bSnapA);
	DDX_Check(pDX, IDC_AIDDLG_SNAPX, m_bSnapX);
	DDX_Check(pDX, IDC_AIDDLG_SNAPY, m_bSnapY);
	DDX_Check(pDX, IDC_AIDDLG_SNAPZ, m_bSnapZ);
	DDX_Check(pDX, IDC_AIDDLG_3DMOUSE, m_b3DMouse);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPreferencesDrawing, CPropertyPage)
	//{{AFX_MSG_MAP(CPreferencesDrawing)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CPreferencesDrawing::SetOptions(unsigned long dwSnap, unsigned short nAngle, unsigned short nGrid)
{
	m_nAngle = nAngle;
	m_nGridSize = nGrid;

	m_bAxis = (dwSnap & LC_DRAW_AXIS) != 0;
	m_bCentimeters = (dwSnap & LC_DRAW_CM_UNITS) != 0;
//		m_bCollision = (dwSnap & LC_DRAW_COLLISION) != 0;
	m_bFixed = (dwSnap & LC_DRAW_MOVEAXIS) != 0;
	m_bGrid = (dwSnap & LC_DRAW_GRID) != 0;
	m_bLockX = (dwSnap & LC_DRAW_LOCK_X) != 0;
	m_bLockY = (dwSnap & LC_DRAW_LOCK_Y) != 0;
	m_bLockZ = (dwSnap & LC_DRAW_LOCK_Z) != 0;
	m_bMove = (dwSnap & LC_DRAW_MOVE) != 0;
	m_bPreview = (dwSnap & LC_DRAW_PREVIEW) != 0;
	m_bSnapA = (dwSnap & LC_DRAW_SNAP_A) != 0;
	m_bSnapX = (dwSnap & LC_DRAW_SNAP_X) != 0;
	m_bSnapY = (dwSnap & LC_DRAW_SNAP_Y) != 0;
	m_bSnapZ = (dwSnap & LC_DRAW_SNAP_Z) != 0;
	m_b3DMouse = (dwSnap & LC_DRAW_3DMOUSE) != 0;
}

void CPreferencesDrawing::GetOptions(unsigned long* dwSnap, unsigned short* nAngle, unsigned short* nGrid)
{
	*nAngle = m_nAngle;
	*nGrid = m_nGridSize;

	*dwSnap = 0;
	if (m_bAxis) *dwSnap |= LC_DRAW_AXIS;
	if (m_bCentimeters) *dwSnap |= LC_DRAW_CM_UNITS;
//		if (m_bCollision) *dwSnap |= LC_DRAW_COLLISION;
	if (m_bFixed) *dwSnap |= LC_DRAW_MOVEAXIS;
	if (m_bGrid) *dwSnap |= LC_DRAW_GRID;
	if (m_bLockX) *dwSnap |= LC_DRAW_LOCK_X;
	if (m_bLockY) *dwSnap |= LC_DRAW_LOCK_Y;
	if (m_bLockZ) *dwSnap |= LC_DRAW_LOCK_Z;
	if (m_bMove) *dwSnap |= LC_DRAW_MOVE;
	if (m_bPreview) *dwSnap |= LC_DRAW_PREVIEW;
	if (m_bSnapA) *dwSnap |= LC_DRAW_SNAP_A;
	if (m_bSnapX) *dwSnap |= LC_DRAW_SNAP_X;
	if (m_bSnapY) *dwSnap |= LC_DRAW_SNAP_Y;
	if (m_bSnapZ) *dwSnap |= LC_DRAW_SNAP_Z;
	if (m_b3DMouse) *dwSnap |= LC_DRAW_3DMOUSE;
}

/////////////////////////////////////////////////////////////////////////////
// CPreferencesScene property page

CPreferencesScene::CPreferencesScene() : CPropertyPage(CPreferencesScene::IDD)
{
	//{{AFX_DATA_INIT(CPreferencesScene)
	m_strBackground = _T("");
	m_bTile = FALSE;
	m_bFog = FALSE;
	m_nFogDensity = 0;
	m_bFloor = FALSE;
	m_nBackground = 0;
	//}}AFX_DATA_INIT
}

CPreferencesScene::~CPreferencesScene()
{
}

void CPreferencesScene::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencesScene)
	DDX_Control(pDX, IDC_SCNDLG_GRAD1, m_btnGrad1);
	DDX_Control(pDX, IDC_SCNDLG_GRAD2, m_btnGrad2);
	DDX_Control(pDX, IDC_SCNDLG_AMBIENTLIGHT, m_btnAmbient);
	DDX_Control(pDX, IDC_SCNDLG_FOGCOLOR, m_btnFog);
	DDX_Control(pDX, IDC_SCNDLG_BGCOLOR, m_btnBackground);
	DDX_Text(pDX, IDC_SCNDLG_BGIMAGE, m_strBackground);
	DDX_Check(pDX, IDC_SCNDLG_BGTILE, m_bTile);
	DDX_Check(pDX, IDC_SCNDLG_FOG, m_bFog);
	DDX_Text(pDX, IDC_SCNDLG_FOGDENSITY, m_nFogDensity);
	DDV_MinMaxByte(pDX, m_nFogDensity, 0, 100);
	DDX_Check(pDX, IDC_SCNDLG_TERRAIN, m_bFloor);
	DDX_Radio(pDX, IDC_SCNDLG_SOLID, m_nBackground);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPreferencesScene, CPropertyPage)
	//{{AFX_MSG_MAP(CPreferencesScene)
	ON_BN_CLICKED(IDC_SCNDLG_BGIMAGE_BROWSE, OnBackgroundBrowse)
	ON_BN_CLICKED(IDC_SCNDLG_BGCOLOR, OnBackgroundColor)
	ON_BN_CLICKED(IDC_SCNDLG_AMBIENTLIGHT, OnAmbientLight)
	ON_BN_CLICKED(IDC_SCNDLG_FOGCOLOR, OnFogColor)
	ON_BN_CLICKED(IDC_SCNDLG_SKYCOLOR1, OnGradColor1)
	ON_BN_CLICKED(IDC_SCNDLG_SKYCOLOR2, OnGradColor2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CPreferencesScene::OnBackgroundBrowse() 
{
	CFileDialog dlg(TRUE, NULL, m_strBackground, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"All Image Files|*.bmp;*.gif;*.jpg;*.png|JPEG Files (*.jpg)|*.jpg|GIF Files (*.gif)|*.gif|BMP Files (*.bmp)|*.bmp|PNG Files (*.png)|*.png|All Files (*.*)|*.*||", this);
	if (dlg.DoModal() == IDOK)
	{
		UpdateData(TRUE);
		m_strBackground = dlg.GetPathName();
		UpdateData(FALSE);
	}
}

void CPreferencesScene::OnBackgroundColor() 
{
	CColorDialog dlg(m_crBackground);
	if (dlg.DoModal() == IDOK)
	{
		m_crBackground = dlg.GetColor();
		DeleteObject(m_btnBackground.SetBitmap(CreateColorBitmap (20, 10, m_crBackground)));
	}
}

void CPreferencesScene::OnAmbientLight() 
{
	CColorDialog dlg(m_crAmbient);
	if (dlg.DoModal() == IDOK)
	{
		m_crAmbient = dlg.GetColor();
		DeleteObject(m_btnAmbient.SetBitmap(CreateColorBitmap (20, 10, m_crAmbient)));
	}
}

void CPreferencesScene::OnFogColor() 
{
	CColorDialog dlg(m_crFog);
	if (dlg.DoModal() == IDOK)
	{
		m_crFog = dlg.GetColor();
		DeleteObject(m_btnFog.SetBitmap(CreateColorBitmap (20, 10, m_crFog)));
	}
}

void CPreferencesScene::OnGradColor1() 
{
	CColorDialog dlg(m_crGrad1);
	if (dlg.DoModal() == IDOK)
	{
		m_crGrad1 = dlg.GetColor();
		DeleteObject(m_btnGrad1.SetBitmap(CreateColorBitmap (20, 10, m_crGrad1)));
	}
}

void CPreferencesScene::OnGradColor2() 
{
	CColorDialog dlg(m_crGrad2);
	if (dlg.DoModal() == IDOK)
	{
		m_crGrad2 = dlg.GetColor();
		DeleteObject(m_btnGrad2.SetBitmap(CreateColorBitmap (20, 10, m_crGrad2)));
	}
}

BOOL CPreferencesScene::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	m_btnAmbient.SetBitmap(CreateColorBitmap (20, 10, m_crAmbient));
	m_btnBackground.SetBitmap(CreateColorBitmap (20, 10, m_crBackground));
	m_btnFog.SetBitmap(CreateColorBitmap (20, 10, m_crFog));
	m_btnGrad1.SetBitmap(CreateColorBitmap (20, 10, m_crGrad1));
	m_btnGrad2.SetBitmap(CreateColorBitmap (20, 10, m_crGrad2));
	
	return TRUE;
}

void CPreferencesScene::SetOptions(unsigned long nScene, float fDensity, char* strBackground, float* fBackground, float* fFog, float* fAmbient, float* fGrad1, float* fGrad2)
{
	if ((nScene & LC_SCENE_BG) != 0) m_nBackground = 2;
	if ((nScene & LC_SCENE_GRADIENT) != 0) m_nBackground = 1;
	m_bTile = (nScene & LC_SCENE_BG_TILE) != 0;
	m_bFog = (nScene & LC_SCENE_FOG) != 0;
	m_bFloor = (nScene & LC_SCENE_FLOOR) != 0;
	m_nFogDensity = (BYTE)(fDensity*100);
	m_strBackground = strBackground;
	m_crBackground = RGB(fBackground[0]*255, fBackground[1]*255, fBackground[2]*255);
	m_crFog = RGB(fFog[0]*255, fFog[1]*255, fFog[2]*255);
	m_crAmbient = RGB(fAmbient[0]*255, fAmbient[1]*255, fAmbient[2]*255);
	m_crGrad1 = RGB(fGrad1[0]*255, fGrad1[1]*255, fGrad1[2]*255);
	m_crGrad2 = RGB(fGrad2[0]*255, fGrad2[1]*255, fGrad2[2]*255);
}

void CPreferencesScene::GetOptions(unsigned long* nScene, float* fDensity, char* strBackground, float* fBackground, float* fFog, float* fAmbient, float* fGrad1, float* fGrad2)
{
	*nScene = 0;
	if (m_nBackground == 2) *nScene |= LC_SCENE_BG;
	if (m_nBackground == 1)	*nScene |= LC_SCENE_GRADIENT;
	if (m_bTile) *nScene |= LC_SCENE_BG_TILE;
	if (m_bFog) *nScene |= LC_SCENE_FOG;
	if (m_bFloor) *nScene |= LC_SCENE_FLOOR;
	*fDensity = (float)m_nFogDensity/100;
	strcpy(strBackground, (LPCSTR)m_strBackground);
	fBackground[0] = (float)GetRValue(m_crBackground)/255;
	fBackground[1] = (float)GetGValue(m_crBackground)/255;
	fBackground[2] = (float)GetBValue(m_crBackground)/255;
	fFog[0] = (float)GetRValue(m_crFog)/255;
	fFog[1] = (float)GetGValue(m_crFog)/255;
	fFog[2] = (float)GetBValue(m_crFog)/255;
	fAmbient[0] = (float)GetRValue(m_crAmbient)/255;
	fAmbient[1] = (float)GetGValue(m_crAmbient)/255;
	fAmbient[2] = (float)GetBValue(m_crAmbient)/255;
	fGrad1[0] = (float)GetRValue(m_crGrad1)/255;
	fGrad1[1] = (float)GetGValue(m_crGrad1)/255;
	fGrad1[2] = (float)GetBValue(m_crGrad1)/255;
	fGrad2[0] = (float)GetRValue(m_crGrad2)/255;
	fGrad2[1] = (float)GetGValue(m_crGrad2)/255;
	fGrad2[2] = (float)GetBValue(m_crGrad2)/255;
}

/////////////////////////////////////////////////////////////////////////////
// CPreferencesPrint property page

CPreferencesPrint::CPreferencesPrint() : CPropertyPage(CPreferencesPrint::IDD)
{
	//{{AFX_DATA_INIT(CPreferencesPrint)
	m_fBottom = 0.0f;
	m_fLeft = 0.0f;
	m_fRight = 0.0f;
	m_fTop = 0.0f;
	m_bNumbers = FALSE;
	m_strHeader = _T("");
	m_strFooter = _T("");
	m_bBorder = FALSE;
	m_nInstCols = 0;
	m_nInstRows = 0;
	m_nCatCols = 0;
	m_nCatRows = 0;
	//}}AFX_DATA_INIT
}

CPreferencesPrint::~CPreferencesPrint()
{
}

void CPreferencesPrint::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencesPrint)
	DDX_Text(pDX, IDC_PRNDLG_MARGIN_BOTTOM, m_fBottom);
	DDX_Text(pDX, IDC_PRNDLG_MARGIN_LEFT, m_fLeft);
	DDX_Text(pDX, IDC_PRNDLG_MARGIN_RIGHT, m_fRight);
	DDX_Text(pDX, IDC_PRNDLG_MARGIN_TOP, m_fTop);
	DDX_Check(pDX, IDC_PRNDLG_NUMBERS, m_bNumbers);
	DDX_Text(pDX, IDC_PRNDLG_HEADER, m_strHeader);
	DDX_Text(pDX, IDC_PRNDLG_FOOTER, m_strFooter);
	DDX_Check(pDX, IDC_PRNDLG_BORDER, m_bBorder);
	DDX_Text(pDX, IDC_PRNDLG_INST_COLS, m_nInstCols);
	DDV_MinMaxInt(pDX, m_nInstCols, 1, 20);
	DDX_Text(pDX, IDC_PRNDLG_INST_ROWS, m_nInstRows);
	DDV_MinMaxInt(pDX, m_nInstRows, 1, 30);
	DDX_Text(pDX, IDC_PRNDLG_CAT_COLS, m_nCatCols);
	DDV_MinMaxInt(pDX, m_nCatCols, 1, 40);
	DDX_Text(pDX, IDC_PRNDLG_CAT_ROWS, m_nCatRows);
	DDV_MinMaxInt(pDX, m_nCatRows, 1, 50);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPreferencesPrint, CPropertyPage)
	//{{AFX_MSG_MAP(CPreferencesPrint)
	ON_BN_CLICKED(IDC_PRNDLG_FOOTERBTN, OnFooterButton)
	ON_BN_CLICKED(IDC_PRNDLG_HEADERBTN, OnFooterButton)
	//}}AFX_MSG_MAP
	ON_COMMAND_RANGE(ID_PRINT_FILENAME, ID_PRINT_RIGHTALIGN, OnHeaderClick)
END_MESSAGE_MAP()


static HBITMAP CreateArrow()
{
	HWND hwndDesktop = GetDesktopWindow(); 
	HDC hdcDesktop = GetDC(hwndDesktop); 
	HDC hdcMem = CreateCompatibleDC(hdcDesktop); 
	HBRUSH hbr = CreateSolidBrush(GetSysColor(COLOR_BTNFACE)); 
	HBITMAP hbm = CreateCompatibleBitmap(hdcDesktop, 4, 7);
	HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbm); 

	// Paint the bitmap
	FillRect(hdcMem, CRect(0, 0, 4, 7), hbr);
	COLORREF c = GetSysColor(COLOR_BTNTEXT);

	for (int x = 0; x < 4; x++)
	for (int y = x; y < 7-x; y++)
		SetPixel(hdcMem, x, y, c);

	// Clean up
	SelectObject(hdcMem, hbmOld); 
	DeleteObject(hbr); 
	DeleteDC(hdcMem); 
	ReleaseDC(hwndDesktop, hdcDesktop); 
	return hbm;
}

BOOL CPreferencesPrint::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	SendDlgItemMessage(IDC_PRNDLG_HEADERBTN, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)CreateArrow());
	SendDlgItemMessage(IDC_PRNDLG_FOOTERBTN, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)CreateArrow());
	
	return TRUE;
}

void CPreferencesPrint::OnFooterButton() 
{
	CMenu menu;
	CMenu* pPopup;
	RECT rc;
	int i = SendDlgItemMessage (IDC_PRNDLG_HEADERBTN, BM_GETSTATE, 0, 0);
	::GetWindowRect(::GetDlgItem(m_hWnd, (i & BST_FOCUS) ? IDC_PRNDLG_HEADERBTN : IDC_PRNDLG_FOOTERBTN), &rc);
	menu.LoadMenu(IDR_POPUPS);
	pPopup = menu.GetSubMenu(3);
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, rc.right, rc.top, this);
}

void CPreferencesPrint::OnHeaderClick(UINT nID)
{
	char c[3] = { '&', 0, 0 };
	switch (nID)
	{
	case ID_PRINT_FILENAME:		c[1] = 'F'; break;
	case ID_PRINT_PAGENUMBER:	c[1] = 'P'; break;
	case ID_PRINT_CURRENTTIME:	c[1] = 'T'; break;
	case ID_PRINT_CURRENTDATE:	c[1] = 'D'; break;
	case ID_PRINT_LEFTALIGN:	c[1] = 'L'; break;
	case ID_PRINT_CENTER:		c[1] = 'C'; break;
	case ID_PRINT_RIGHTALIGN:	c[1] = 'R'; break;
	case ID_PRINT_AUTHOR:		c[1] = 'A'; break;
	case ID_PRINT_DESCRIPTION:	c[1] = 'N'; break;
	}
	int i = SendDlgItemMessage (IDC_PRNDLG_HEADERBTN, BM_GETSTATE, 0, 0);
	SendDlgItemMessage ((i & BST_FOCUS) ? IDC_PRNDLG_HEADER : IDC_PRNDLG_FOOTER, EM_REPLACESEL, TRUE, (LPARAM)&c);
	::SetFocus (::GetDlgItem(m_hWnd, (i & BST_FOCUS) ? IDC_PRNDLG_HEADER : IDC_PRNDLG_FOOTER));
}

void CPreferencesPrint::SetOptions(CString strHeader, CString strFooter)
{
	DWORD dwPrint = AfxGetApp()->GetProfileInt("Settings","Print", PRINT_NUMBERS|PRINT_BORDER);
	m_bNumbers = (dwPrint & PRINT_NUMBERS) != 0;
	m_bBorder = (dwPrint & PRINT_BORDER) != 0;

	m_strHeader = strHeader;
	m_strFooter = strFooter;

	m_fLeft = (float)AfxGetApp()->GetProfileInt("Default","Margin Left", 50)/100;
	m_fTop = (float)AfxGetApp()->GetProfileInt("Default","Margin Top", 50)/100;
	m_fRight = (float)AfxGetApp()->GetProfileInt("Default","Margin Right", 50)/100; 
	m_fBottom = (float)AfxGetApp()->GetProfileInt("Default","Margin Bottom", 50)/100;
	m_nInstRows = AfxGetApp()->GetProfileInt("Default","Print Rows", 1);
	m_nInstCols = AfxGetApp()->GetProfileInt("Default","Print Columns", 1);
	m_nCatRows = AfxGetApp()->GetProfileInt("Default","Catalog Rows", 10);
	m_nCatCols = AfxGetApp()->GetProfileInt("Default","Catalog Columns", 3);
}

void CPreferencesPrint::GetOptions(char* strHeader, char* strFooter)
{
	DWORD dwPrint = 0;
	if (m_bNumbers) dwPrint |= PRINT_NUMBERS;
	if (m_bBorder) dwPrint |= PRINT_BORDER;
	AfxGetApp()->WriteProfileInt("Settings","Print", dwPrint);

	strcpy(strHeader, (LPCSTR)m_strHeader);
	strcpy(strFooter, (LPCSTR)m_strFooter);

	AfxGetApp()->WriteProfileInt("Default","Margin Left", (int)(m_fLeft*100));
	AfxGetApp()->WriteProfileInt("Default","Margin Top", (int)(m_fTop*100));
	AfxGetApp()->WriteProfileInt("Default","Margin Right", (int)(m_fRight*100)); 
	AfxGetApp()->WriteProfileInt("Default","Margin Bottom", (int)(m_fBottom*100));
	AfxGetApp()->WriteProfileInt("Default","Print Rows", m_nInstRows);
	AfxGetApp()->WriteProfileInt("Default","Print Columns", m_nInstCols);
	AfxGetApp()->WriteProfileInt("Default","Catalog Rows", m_nCatRows);
	AfxGetApp()->WriteProfileInt("Default","Catalog Columns", m_nCatCols);
}
