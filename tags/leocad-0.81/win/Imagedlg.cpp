#include "lc_global.h"
#include "leocad.h"
#include "ImageDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CImageDlg dialog


CImageDlg::CImageDlg(BOOL bHTML, void* param, CWnd* pParent /*=NULL*/)
	: CDialog(CImageDlg::IDD, pParent)
{
	m_bHTML = bHTML;
	opts = (LC_IMAGEDLG_OPTS*)param;

	//{{AFX_DATA_INIT(CImageDlg)
	m_nFormat = opts->imopts.format;
	m_bTransparent = opts->imopts.transparent;
	m_bProgressive = opts->imopts.interlaced;
	m_bHighcolor = opts->imopts.truecolor;
	m_nQuality = opts->imopts.quality;
	m_fPause = opts->imopts.pause;
	m_nHeight = opts->height;
	m_nWidth = opts->width;
	m_nFrom = opts->from;
	m_nTo = opts->to;
	m_nSingle = opts->multiple ? 1 : 0;
	m_strFilename = bHTML ? _T("") : opts->filename;
	//}}AFX_DATA_INIT
}


void CImageDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CImageDlg)
	DDX_Radio(pDX, IDC_IMGDLG_BMP, m_nFormat);
	DDX_Check(pDX, IDC_IMGDLG_TRANSPARENT, m_bTransparent);
	DDX_Check(pDX, IDC_IMGDLG_PROGRESSIVE, m_bProgressive);
	DDX_Text(pDX, IDC_IMGDLG_HEIGHT, m_nHeight);
	DDX_Check(pDX, IDC_IMGDLG_HIGHCOLOR, m_bHighcolor);
	DDX_Text(pDX, IDC_IMGDLG_QUALITY, m_nQuality);
	DDX_Text(pDX, IDC_IMGDLG_WIDTH, m_nWidth);
	DDX_Text(pDX, IDC_IMGDLG_FROM, m_nFrom);
	DDX_Text(pDX, IDC_IMGDLG_PAUSE, m_fPause);
	DDX_Radio(pDX, IDC_IMGDLG_SINGLE, m_nSingle);
	DDX_Text(pDX, IDC_IMGDLG_TO, m_nTo);
	DDX_Text(pDX, IDC_IMGDLG_FILENAME, m_strFilename);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CImageDlg, CDialog)
	//{{AFX_MSG_MAP(CImageDlg)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_IMGDLG_BROWSE, &CImageDlg::OnBnClickedImgdlgBrowse)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CImageDlg message handlers

BOOL CImageDlg::OnInitDialog() 
{
	if (m_bHTML)
	{
		UINT u[8] = { IDC_IMGDLG_SINGLE, IDC_IMGDLG_MULTIPLE, IDC_IMGDLG_FROM, IDC_IMGDLG_TO, IDC_IMGDLG_AVI, IDC_IMGDLG_PAUSE, IDC_IMGDLG_FILENAME, IDC_IMGDLG_BROWSE };

		for (int i = 0; i < 8; i++)
			GetDlgItem(u[i])->EnableWindow(FALSE);
	}

	CDialog::OnInitDialog();
	return TRUE;
}

void CImageDlg::OnOK() 
{
	if (!UpdateData(TRUE))
		return;

	opts->imopts.format = m_nFormat;
	opts->imopts.transparent = m_bTransparent != 0;
	opts->imopts.interlaced = m_bProgressive != 0;
	opts->imopts.truecolor = m_bHighcolor != 0;
	opts->imopts.quality = m_nQuality;
	opts->imopts.pause = m_fPause;
	opts->height = m_nHeight;
	opts->width = m_nWidth;
	opts->from = m_nFrom;
	opts->to = m_nTo;
	opts->multiple = m_nSingle != 0;
	
	CDialog::OnOK();
}

void CImageDlg::OnBnClickedImgdlgBrowse()
{
	CFileDialog dlg(FALSE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER,
	                "All Images|*.gif;*.jpg;*.jpeg;*.bmp;*.png;*.avi|GIF Files (*.gif)|*.gif|JPEG Files (*.jpg;*.jpeg)|*.jpg;*.jpeg|Bitmap Files (*.bmp)|*.bmp|PNG Files (*.png)|*.png|AVI Files (*.avi)|*.avi|All Files (*.*)|*.*||");

	if (dlg.DoModal() == IDOK)
	{
		UpdateData (TRUE);
		m_strFilename = dlg.GetPathName();
		UpdateData (FALSE);
	}
}
