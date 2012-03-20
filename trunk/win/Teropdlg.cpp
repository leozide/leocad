#include "lc_global.h"
#include "leocad.h"
#include "TerOpDlg.h"
#include "Terrain.h"
#include "Tools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTerrainOptionsDlg dialog


CTerrainOptionsDlg::CTerrainOptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTerrainOptionsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTerrainOptionsDlg)
	m_nXPatches = 0;
	m_nYPatches = 0;
	m_fXSize = 0.0f;
	m_fYSize = 0.0f;
	m_bFlat = FALSE;
	m_bTexture = FALSE;
	m_strTexture = _T("");
	m_bSmooth = FALSE;
	//}}AFX_DATA_INIT
}


void CTerrainOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTerrainOptionsDlg)
	DDX_Control(pDX, IDC_TEROPT_COLOR, m_btnColor);
	DDX_Text(pDX, IDC_TEROPT_XPAT, m_nXPatches);
	DDV_MinMaxInt(pDX, m_nXPatches, 1, 1024);
	DDX_Text(pDX, IDC_TEROPT_YPAT, m_nYPatches);
	DDV_MinMaxInt(pDX, m_nYPatches, 1, 1024);
	DDX_Text(pDX, IDC_TEROPT_XSIZE, m_fXSize);
	DDV_MinMaxFloat(pDX, m_fXSize, 1.f, 1024.f);
	DDX_Text(pDX, IDC_TEROPT_YSIZE, m_fYSize);
	DDV_MinMaxFloat(pDX, m_fYSize, 1.f, 1024.f);
	DDX_Check(pDX, IDC_TEROPT_FLAT, m_bFlat);
	DDX_Check(pDX, IDC_TEROPT_TEXTURE, m_bTexture);
	DDX_Text(pDX, IDC_TEROPT_TEXTURENAME, m_strTexture);
	DDX_Check(pDX, IDC_TEROPT_SMOOTH, m_bSmooth);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTerrainOptionsDlg, CDialog)
	//{{AFX_MSG_MAP(CTerrainOptionsDlg)
	ON_BN_CLICKED(IDC_TEROPT_COLOR, OnTeroptColor)
	ON_BN_CLICKED(IDC_TEROPT_TEXTUREBROWSE, OnTeroptTexturebrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTerrainOptionsDlg message handlers

void CTerrainOptionsDlg::OnTeroptColor() 
{
	CColorDialog dlg(m_crTerrain);
	if (dlg.DoModal() == IDOK)
	{
		m_crTerrain = dlg.GetColor();
		DeleteObject(m_btnColor.SetBitmap(CreateColorBitmap (20, 10, m_crTerrain)));
	}
}

void CTerrainOptionsDlg::OnTeroptTexturebrowse() 
{
	CFileDialog dlg(TRUE, NULL, m_strTexture, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"All Image Files|*.bmp;*.gif;*.jpg;*.png|JPEG Files (*.jpg)|*.jpg|GIF Files (*.gif)|*.gif|BMP Files (*.bmp)|*.bmp|PNG Files (*.png)|*.png|All Files (*.*)|*.*||", this);
	if (dlg.DoModal() == IDOK)
	{
		UpdateData(TRUE);
		m_strTexture = dlg.GetPathName();
		UpdateData(FALSE);
	}
}

void CTerrainOptionsDlg::SetOptions(Terrain* pTerrain)
{
	pTerrain->GetPatchCount(&m_nXPatches, &m_nYPatches);
	pTerrain->GetSize(&m_fXSize, &m_fYSize);
	m_crTerrain = RGB(pTerrain->m_fColor[0]*255, pTerrain->m_fColor[1]*255, pTerrain->m_fColor[2]*255);
	m_strTexture = pTerrain->m_strTexture;
	m_bFlat = (pTerrain->m_nOptions & LC_TERRAIN_FLAT) != 0;
	m_bTexture = (pTerrain->m_nOptions & LC_TERRAIN_TEXTURE) != 0;
	m_bSmooth = (pTerrain->m_nOptions & LC_TERRAIN_SMOOTH) != 0;
}

void CTerrainOptionsDlg::GetOptions(Terrain* pTerrain)
{
	pTerrain->SetSize(m_fXSize, m_fYSize);
	pTerrain->SetPatchCount(m_nXPatches, m_nYPatches);
	pTerrain->m_fColor[0] = (float)GetRValue(m_crTerrain)/255;
	pTerrain->m_fColor[1] = (float)GetGValue(m_crTerrain)/255;
	pTerrain->m_fColor[2] = (float)GetBValue(m_crTerrain)/255;
	pTerrain->m_nOptions = 0;
	strcpy(pTerrain->m_strTexture, m_strTexture);

	if (m_bFlat) pTerrain->m_nOptions |= LC_TERRAIN_FLAT;
	if (m_bTexture) pTerrain->m_nOptions |= LC_TERRAIN_TEXTURE;
	if (m_bSmooth) pTerrain->m_nOptions |= LC_TERRAIN_SMOOTH;
}

BOOL CTerrainOptionsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	DeleteObject(m_btnColor.SetBitmap(CreateColorBitmap (20, 10, m_crTerrain)));
	
	return TRUE;
}
