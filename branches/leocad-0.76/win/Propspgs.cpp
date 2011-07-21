// PropsPgs.cpp : implementation file
//

#include "lc_global.h"
#include "resource.h"
#include "PropsPgs.h"
#include "defines.h"
#include "lc_colors.h"
#include "library.h"
#include "lc_application.h"
#include "lc_model.h"
#include "pieceinf.h"
#include "tools.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CPropertiesSummary, CPropertyPage)
IMPLEMENT_DYNCREATE(CPropertiesScene, CPropertyPage)
IMPLEMENT_DYNCREATE(CPropertiesPieces, CPropertyPage)


/////////////////////////////////////////////////////////////////////////////
// CPropertiesSummary property page

CPropertiesSummary::CPropertiesSummary() : CPropertyPage(CPropertiesSummary::IDD)
{
	//{{AFX_DATA_INIT(CPropertiesSummary)
	m_Name = _T("");
	m_Author = _T("");
	m_Comments = _T("");
	m_Description = _T("");
	//}}AFX_DATA_INIT
}

CPropertiesSummary::~CPropertiesSummary()
{
}

void CPropertiesSummary::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropertiesSummary)
	DDX_Text(pDX, IDC_PROP_SUM_NAME, m_Name);
	DDV_MaxChars(pDX, m_Name, 100);
	DDX_Text(pDX, IDC_PROP_SUM_AUTHOR, m_Author);
	DDV_MaxChars(pDX, m_Author, 100);
	DDX_Text(pDX, IDC_PROP_SUM_COMMENTS, m_Comments);
	DDV_MaxChars(pDX, m_Comments, 255);
	DDX_Text(pDX, IDC_PROP_SUM_DESCRIPTION, m_Description);
	DDV_MaxChars(pDX, m_Description, 100);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropertiesSummary, CPropertyPage)
	//{{AFX_MSG_MAP(CPropertiesSummary)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPropertiesScene property page

CPropertiesScene::CPropertiesScene() : CPropertyPage(CPropertiesScene::IDD)
{
	//{{AFX_DATA_INIT(CPropertiesScene)
	m_strBackground = _T("");
	m_bTile = FALSE;
	m_bFog = FALSE;
	m_nFogDensity = 0;
	m_bFloor = FALSE;
	m_nBackground = 0;
	//}}AFX_DATA_INIT
}

CPropertiesScene::~CPropertiesScene()
{
}

void CPropertiesScene::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropertiesScene)
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


BEGIN_MESSAGE_MAP(CPropertiesScene, CPropertyPage)
	//{{AFX_MSG_MAP(CPropertiesScene)
	ON_BN_CLICKED(IDC_SCNDLG_BGIMAGE_BROWSE, OnBackgroundBrowse)
	ON_BN_CLICKED(IDC_SCNDLG_BGCOLOR, OnBackgroundColor)
	ON_BN_CLICKED(IDC_SCNDLG_AMBIENTLIGHT, OnAmbientLight)
	ON_BN_CLICKED(IDC_SCNDLG_FOGCOLOR, OnFogColor)
	ON_BN_CLICKED(IDC_SCNDLG_SKYCOLOR1, OnGradColor1)
	ON_BN_CLICKED(IDC_SCNDLG_SKYCOLOR2, OnGradColor2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CPropertiesScene::OnBackgroundBrowse() 
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

void CPropertiesScene::OnBackgroundColor() 
{
	CColorDialog dlg(m_crBackground);
	if (dlg.DoModal() == IDOK)
	{
		m_crBackground = dlg.GetColor();
		DeleteObject(m_btnBackground.SetBitmap(CreateColorBitmap(20, 10, m_crBackground)));
	}
}

void CPropertiesScene::OnAmbientLight() 
{
	CColorDialog dlg(m_crAmbient);
	if (dlg.DoModal() == IDOK)
	{
		m_crAmbient = dlg.GetColor();
		DeleteObject(m_btnAmbient.SetBitmap(CreateColorBitmap(20, 10, m_crAmbient)));
	}
}

void CPropertiesScene::OnFogColor() 
{
	CColorDialog dlg(m_crFog);
	if (dlg.DoModal() == IDOK)
	{
		m_crFog = dlg.GetColor();
		DeleteObject(m_btnFog.SetBitmap(CreateColorBitmap(20, 10, m_crFog)));
	}
}

void CPropertiesScene::OnGradColor1() 
{
	CColorDialog dlg(m_crGrad1);
	if (dlg.DoModal() == IDOK)
	{
		m_crGrad1 = dlg.GetColor();
		DeleteObject(m_btnGrad1.SetBitmap(CreateColorBitmap(20, 10, m_crGrad1)));
	}
}

void CPropertiesScene::OnGradColor2() 
{
	CColorDialog dlg(m_crGrad2);
	if (dlg.DoModal() == IDOK)
	{
		m_crGrad2 = dlg.GetColor();
		DeleteObject(m_btnGrad2.SetBitmap(CreateColorBitmap(20, 10, m_crGrad2)));
	}
}

BOOL CPropertiesScene::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	m_btnAmbient.SetBitmap(CreateColorBitmap(20, 10, m_crAmbient));
	m_btnBackground.SetBitmap(CreateColorBitmap(20, 10, m_crBackground));
	m_btnFog.SetBitmap(CreateColorBitmap(20, 10, m_crFog));
	m_btnGrad1.SetBitmap(CreateColorBitmap(20, 10, m_crGrad1));
	m_btnGrad2.SetBitmap(CreateColorBitmap(20, 10, m_crGrad2));
	
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CPropertiesPieces property page

CPropertiesPieces::CPropertiesPieces() : CPropertyPage(CPropertiesPieces::IDD)
{
	//{{AFX_DATA_INIT(CPropertiesPieces)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_ColorColumn = new int[lcNumUserColors];
}

CPropertiesPieces::~CPropertiesPieces()
{
	delete[] m_ColorColumn;
}

void CPropertiesPieces::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropertiesPieces)
	DDX_Control(pDX, IDC_PROP_PIECES_LIST, m_List);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPropertiesPieces, CPropertyPage)
	//{{AFX_MSG_MAP(CPropertiesPieces)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_PROP_PIECES_LIST, OnColumnclickPropPiecesList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static int CALLBACK ListViewCompareProc(LPARAM lP1, LPARAM lP2, LPARAM lParamData)
{
	// Keep the total row at the bottom of the list.
	if (lP1 == 0)
		return 1;
	else if (lP2 == 0)
		return -1;

	CPropertiesPieces* Dlg = (CPropertiesPieces*)lParamData;

	if (Dlg->m_SortColumn != 0)
	{
		LVFINDINFO FindInfo;
		FindInfo.flags = LVFI_PARAM;

		FindInfo.lParam = lP1;
		int Row1 = Dlg->m_List.FindItem(&FindInfo);
		FindInfo.lParam = lP2;
		int Row2 = Dlg->m_List.FindItem(&FindInfo);

		int Num1 = atoi(Dlg->m_List.GetItemText(Row1, Dlg->m_SortColumn));
		int Num2 = atoi(Dlg->m_List.GetItemText(Row2, Dlg->m_SortColumn));

		if (Num1 < Num2)
			return Dlg->m_SortAscending ? 1 : -1;
		else if (Num1 > Num2)
			return Dlg->m_SortAscending ? -1 : 1;
		else
			return 0;
	}

	PieceInfo* Info1 = (PieceInfo*)lP1;
	PieceInfo* Info2 = (PieceInfo*)lP2;

	if (Dlg->m_SortAscending)
		return _strcmpi(Info1->m_strDescription, Info2->m_strDescription);
	else
		return - _strcmpi(Info1->m_strDescription, Info2->m_strDescription);
}

BOOL CPropertiesPieces::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	int* ColorTotal = new int[lcNumUserColors];
	memset(ColorTotal, 0, lcNumUserColors * sizeof(int));
	memset(m_ColorColumn, 0, lcNumUserColors * sizeof(int));
	char Count[256];
	int i;

	PiecesLibrary* Lib = lcGetPiecesLibrary();

	// Count the number of pieces used for each color.
	for (int EntryIdx = 0; EntryIdx < m_PiecesUsed.GetSize(); EntryIdx++)
	{
		lcPiecesUsedEntry& Entry = m_PiecesUsed[EntryIdx];
		ColorTotal[Entry.Color] += Entry.Count;
	}

	// Add columns to the list header.
	m_List.InsertColumn(0, "Piece", LVCFMT_LEFT, 130, 0);

	int Column = 1;

	for (i = 0; i < lcNumUserColors; i++)
	{
		if (!ColorTotal[i])
			continue;

		m_ColorColumn[i] = Column;
		m_List.InsertColumn(Column, g_ColorList[i].Name, LVCFMT_LEFT, 80, 0);
		Column++;
	}

	m_List.InsertColumn(Column, "Total", LVCFMT_LEFT, 60, 0);

	m_List.SetRedraw(FALSE);

	// Add pieces to the list.
	for (int EntryIdx = 0; EntryIdx < m_PiecesUsed.GetSize(); EntryIdx++)
	{
		lcPiecesUsedEntry& Entry = m_PiecesUsed[EntryIdx];

		LVFINDINFO FindInfo;
		FindInfo.flags = LVFI_PARAM;
		FindInfo.lParam = (LPARAM)Entry.Info;

		int Row = m_List.FindItem(&FindInfo);

		if (Row == -1)
		{
			LV_ITEM Item;
			Item.mask = LVIF_TEXT|LVIF_PARAM;
			Item.iItem = 0;
			Item.iSubItem = 0;
			Item.pszText = Entry.Info->m_strDescription;
			Item.lParam = (LPARAM)Entry.Info;

			Row = m_List.InsertItem(&Item);

			int Total = 0;
			for (int CountEntryIdx = 0; CountEntryIdx < m_PiecesUsed.GetSize(); CountEntryIdx++)
			{
				lcPiecesUsedEntry& CountEntry = m_PiecesUsed[CountEntryIdx];
				if (CountEntry.Info == Entry.Info)
					Total += CountEntry.Count;
			}

			sprintf(Count, "%d", Total);
			m_List.SetItemText(Row, Column, Count);
		}

		sprintf(Count, "%d", Entry.Count);
		m_List.SetItemText(Row, m_ColorColumn[Entry.Color], Count);
	}

	// Add totals.
	LV_ITEM Item;
	Item.mask = LVIF_TEXT|LVIF_PARAM;
	Item.iItem = m_List.GetItemCount();
	Item.iSubItem = 0;
	Item.pszText = "Total";
	Item.lParam = 0;

	int Row = m_List.InsertItem(&Item);
	int Total = 0;

	for (i = 0; i < lcNumUserColors; i++)
	{
		if (!ColorTotal[i])
			continue;

		Total += ColorTotal[i];

		sprintf(Count, "%d", ColorTotal[i]);
		m_List.SetItemText(Row, m_ColorColumn[i], Count);
	}

	sprintf(Count, "%d", Total);
	m_List.SetItemText(Row, Column, Count);

	delete[] ColorTotal;

	m_SortColumn = 0;
	m_SortAscending = true;

	m_List.SortItems((PFNLVCOMPARE)ListViewCompareProc, (LPARAM)this);

	m_List.SetRedraw(TRUE);
	m_List.Invalidate();

	return TRUE;
}

void CPropertiesPieces::OnColumnclickPropPiecesList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if (m_SortColumn == pNMListView->iSubItem)
		m_SortAscending = !m_SortAscending;
	else
		m_SortColumn = pNMListView->iSubItem;

	m_List.SetRedraw(FALSE);

	m_List.SortItems((PFNLVCOMPARE)ListViewCompareProc, (LPARAM)this);

	m_List.SetRedraw(TRUE);
	m_List.Invalidate();

	*pResult = 0;
}
