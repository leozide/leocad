#include "lc_global.h"
#include "leocad.h"
#include "texdlg.h"
#include "library.h"
#include "project.h"
#include "globals.h"
#include "texture.h"
#include "lc_application.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTexturesDlg dialog


CTexturesDlg::CTexturesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTexturesDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTexturesDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CTexturesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTexturesDlg)
	DDX_Control(pDX, ID_LIBTEX_LIST, m_List);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTexturesDlg, CDialog)
	//{{AFX_MSG_MAP(CTexturesDlg)
	ON_BN_CLICKED(ID_LIBTEX_ADD, OnLibtexAdd)
	ON_BN_CLICKED(ID_LIBTEX_REMOVE, OnLibtexRemove)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTexturesDlg message handlers

BOOL CTexturesDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	UpdateList();
	
	return TRUE;
}

void CTexturesDlg::OnOK() 
{
	CDialog::OnOK();
}

void CTexturesDlg::OnLibtexAdd() 
{
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"All Image Files|*.bmp;*.gif;*.jpg;*.png|JPEG Files (*.jpg)|*.jpg|GIF Files (*.gif)|*.gif|BMP Files (*.bmp)|*.bmp|PNG Files (*.png)|*.png|All Files (*.*)|*.*||", this);

	if (dlg.DoModal() == IDOK)
	{
		lcGetPiecesLibrary()->ImportTexture(dlg.GetPathName());
		UpdateList();
	}
}

void CTexturesDlg::OnLibtexRemove() 
{
	int i, selected = 0;

	for (i = 0; i < m_List.GetCount(); i++)
		if (m_List.GetSel(i))
			selected++;

	// Nothing to be done
	if (selected == 0)
		return;

	char** names = (char**)malloc(selected*sizeof(char**));

	for (selected = 0, i = 0; i < m_List.GetCount(); i++)
	{
		if (m_List.GetSel(i))
		{
			names[selected] = (char*)m_List.GetItemDataPtr (i);;
			selected++;
		}
	}

	lcGetPiecesLibrary()->DeleteTextures(names, selected);

	free (names);

	UpdateList();
}

void CTexturesDlg::UpdateList()
{
	PiecesLibrary *pLib = lcGetPiecesLibrary();

	m_List.ResetContent();

	for (int i = 0; i < pLib->GetTextureCount(); i++)
	{
		int index = m_List.AddString (pLib->GetTexture(i)->m_strName);
		m_List.SetItemDataPtr(index, pLib->GetTexture(i)->m_strName);
	}
}
