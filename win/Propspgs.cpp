// PropsPgs.cpp : implementation file
//

#include "lc_global.h"
#include "resource.h"
#include "PropsPgs.h"
#include "defines.h"
#include "lc_colors.h"
#include "library.h"
#include "lc_application.h"
#include "pieceinf.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CPropertiesSummary, CPropertyPage)
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
	CPropertiesPieces* Dlg = (CPropertiesPieces*)lParamData;
	int* Line1 = (int*)lP1;
	int* Line2 = (int*)lP2;

	// Keep the total row at the bottom of the list.
	if (lP1 == -1)
		return 1;
	else if (lP2 == -1)
		return -1;

	if (Dlg->m_SortColumn != 0)
	{
		int Num1, Num2;

		if (Dlg->m_SortColumn == Dlg->m_List.GetHeaderCtrl()->GetItemCount() - 1)
		{
			Num1 = 0;
			Num2 = 0;

			for (int i = 0; i < lcNumUserColors; i++)
			{
				Num1 += Line1[i];
				Num2 += Line2[i];
			}
		}
		else
		{
			int i;

			for (i = 0; i < lcNumUserColors; i++)
				if (Dlg->m_ColorColumn[i] == Dlg->m_SortColumn)
					break;

			Num1 = Line1[i];
			Num2 = Line2[i];
		}

		if (Num1 < Num2)
			return Dlg->m_SortAscending ? 1 : -1;
		else if (Num1 > Num2)
			return Dlg->m_SortAscending ? -1 : 1;
		else
			return 0;
	}

	int Index1 = (Line1 - Dlg->m_PiecesUsed) / lcNumUserColors;
	int Index2 = (Line2 - Dlg->m_PiecesUsed) / lcNumUserColors;

	PiecesLibrary* Lib = lcGetPiecesLibrary();
	PieceInfo* Info1 = Lib->GetPieceInfo(Index1);
	PieceInfo* Info2 = Lib->GetPieceInfo(Index2);

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
	int i, j;

	PiecesLibrary* Lib = lcGetPiecesLibrary();

	// Count the number of pieces used for each color.
	for (i = 0; i < Lib->GetPieceCount(); i++)
		for (j = 0; j < lcNumUserColors; j++)
			ColorTotal[j] += m_PiecesUsed[j + i * lcNumUserColors];

	// Add columns to the list header.
	m_List.InsertColumn(0, "Piece", LVCFMT_LEFT, 130, 0);

	int Column = 1;

	for (i = 0; i < lcNumUserColors; i++)
	{
		if (!ColorTotal[i])
			continue;

		m_ColorColumn[i] = Column;
		m_List.InsertColumn(Column, lcColorList[i].Name, LVCFMT_LEFT, 80, 0);
		Column++;
	}

	m_List.InsertColumn(Column, "Total", LVCFMT_LEFT, 60, 0);

	m_List.SetRedraw(FALSE);

	// Add pieces to the list.
	for (i = 0; i < Lib->GetPieceCount(); i++)
	{
		int* Line = m_PiecesUsed + i * lcNumUserColors;
		int LineTotal = 0;

		for (j = 0; j < lcNumUserColors; j++)
			LineTotal += Line[j];

		if (!LineTotal)
			continue;

		char name[256], tmp[256];
		LV_ITEM lvi;
		lvi.mask = LVIF_TEXT|LVIF_PARAM;
		lvi.iItem = 0;
		lvi.iSubItem = 0;
		lvi.pszText = name;
		lvi.lParam = (LPARAM)Line;
		strcpy(name, Lib->GetPieceInfo(i)->m_strDescription);

		int idx = m_List.InsertItem(&lvi);

		for (j = 0; j < lcNumUserColors; j++)
		{
			if (!Line[j])
				continue;

			sprintf(tmp, "%d", Line[j]);
			m_List.SetItemText(idx, m_ColorColumn[j], tmp);
		}

		sprintf (tmp, "%d", LineTotal);
		m_List.SetItemText(idx, Column, tmp);
	}

	// Add totals.
	char name[256], tmp[256];
	strcpy (name, "Total");
	LV_ITEM lvi;
	lvi.mask = LVIF_TEXT|LVIF_PARAM;
	lvi.iItem = m_List.GetItemCount();
	lvi.iSubItem = 0;
	lvi.pszText = name;
	lvi.lParam = -1;

	int idx = m_List.InsertItem(&lvi), total = 0;
	int Total = 0;

	for (i = 0; i < lcNumUserColors; i++)
	{
		if (!ColorTotal[i])
			continue;

		Total += ColorTotal[i];

		sprintf (tmp, "%d", ColorTotal[i]);
		m_List.SetItemText(idx, m_ColorColumn[i], tmp);
	}

	sprintf (tmp, "%d", Total);
	m_List.SetItemText(idx, Column, tmp);

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
