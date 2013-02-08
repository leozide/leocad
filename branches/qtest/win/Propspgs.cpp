#include "lc_global.h"
#include "lc_colors.h"
#include "resource.h"
#include "PropsPgs.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CPropertiesGeneral, CPropertyPage)
IMPLEMENT_DYNCREATE(CPropertiesSummary, CPropertyPage)
IMPLEMENT_DYNCREATE(CPropertiesPieces, CPropertyPage)


/////////////////////////////////////////////////////////////////////////////
// CPropertiesGeneral property page

CPropertiesGeneral::CPropertiesGeneral() : CPropertyPage(CPropertiesGeneral::IDD)
{
	//{{AFX_DATA_INIT(CPropertiesGeneral)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPropertiesGeneral::~CPropertiesGeneral()
{
}

void CPropertiesGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropertiesGeneral)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropertiesGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(CPropertiesGeneral)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CPropertiesGeneral::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	WIN32_FIND_DATA fd;

	if (FindFirstFile(m_strFilename, &fd) != INVALID_HANDLE_VALUE)
	{
		char tf[] = "%A, %B %d, %Y %I:%M:%S %p";
		SetDlgItemText(IDC_PROP_GEN_DOSNAME, m_strFilename.Right(m_strFilename.GetLength() - m_strFilename.ReverseFind('\\') - 1));
		SetDlgItemText(IDC_PROP_GEN_LOCATION, m_strFilename.Left(m_strFilename.ReverseFind('\\') + 1));

		CString str;
		str.Format("%.1fKB (%d bytes)", (float)fd.nFileSizeLow/1024, fd.nFileSizeLow);
		SetDlgItemText(IDC_PROP_GEN_SIZE, str);
		CTime timeFile(fd.ftCreationTime);
		str = timeFile.Format (tf);
		SetDlgItemText(IDC_PROP_GEN_CREATED, str);
		timeFile = fd.ftLastWriteTime;
		str = timeFile.Format (tf);
		SetDlgItemText(IDC_PROP_GEN_MODIFIED, str);
		timeFile = fd.ftLastAccessTime;
		str = timeFile.Format ("%A, %B %d, %Y");
		SetDlgItemText(IDC_PROP_GEN_ACCESSED, str);
	}
	else
	{
		char* str = "(Unknown)";
		SetDlgItemText(IDC_PROP_GEN_CREATED, str);
		SetDlgItemText(IDC_PROP_GEN_MODIFIED, str);
		SetDlgItemText(IDC_PROP_GEN_ACCESSED, str);
	}

	UpdateData (FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// CPropertiesSummary property page

CPropertiesSummary::CPropertiesSummary() : CPropertyPage(CPropertiesSummary::IDD)
{
	//{{AFX_DATA_INIT(CPropertiesSummary)
	m_strAuthor = _T("");
	m_strComments = _T("");
	m_strDescription = _T("");
	//}}AFX_DATA_INIT
}

CPropertiesSummary::~CPropertiesSummary()
{
}

void CPropertiesSummary::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropertiesSummary)
	DDX_Text(pDX, IDC_PROP_SUM_AUTHOR, m_strAuthor);
	DDV_MaxChars(pDX, m_strAuthor, 100);
	DDX_Text(pDX, IDC_PROP_SUM_COMMENTS, m_strComments);
	DDV_MaxChars(pDX, m_strComments, 255);
	DDX_Text(pDX, IDC_PROP_SUM_DESCRIPTION, m_strDescription);
	DDV_MaxChars(pDX, m_strDescription, 100);
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

	mColorColumn = new int[gColorList.GetSize() + 1];
}

CPropertiesPieces::~CPropertiesPieces()
{
	delete[] mColorColumn;
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


BOOL CPropertiesPieces::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	m_List.ModifyStyle(LVS_SORTASCENDING | LVS_SORTDESCENDING, 0L);

	int NumColors = gColorList.GetSize();
	char tmp[256];

	int NumColumns = 0;
	m_List.InsertColumn(0, "Piece", LVCFMT_LEFT, 130, 0);

	for (int ColorIdx = 0; ColorIdx < mNumColors; ColorIdx++)
	{
		if (mPieceColorCount[mNumPieces * (mNumColors + 1) + ColorIdx])
		{
			mColorColumn[ColorIdx] = NumColumns;
			NumColumns++;

			m_List.InsertColumn(NumColumns, gColorList[ColorIdx].Name, LVCFMT_LEFT, 80, 0);
		}
		else
			mColorColumn[ColorIdx] = -1;
	}

	mColorColumn[mNumColors] = NumColumns;
	NumColumns++;
	m_List.InsertColumn(NumColumns, "Total", LVCFMT_LEFT, 60, 0);

	char name[256];
	LV_ITEM lvi;
	lvi.mask = LVIF_TEXT|LVIF_PARAM;
	lvi.iItem = 0;
	lvi.iSubItem = 0;
	lvi.pszText = name;

	for (int PieceIdx = 0; PieceIdx < mNumPieces + 1; PieceIdx++)
	{
		if (!mPieceColorCount[PieceIdx * (mNumColors + 1) + mNumColors])
			continue;

		if (PieceIdx != mNumPieces)
		{
			strcpy(name, mPieceNames[PieceIdx]);
			lvi.lParam = PieceIdx;
		}
		else
		{
			strcpy(name, "Total");
			lvi.iItem = m_List.GetItemCount();
			lvi.lParam = -1;
		}

		int idx = m_List.InsertItem(&lvi);

		for (int ColorIdx = 0; ColorIdx < mNumColors + 1; ColorIdx++)
		{
			if (!mPieceColorCount[PieceIdx * (mNumColors + 1) + ColorIdx])
				continue;

			sprintf(tmp, "%d", mPieceColorCount[PieceIdx * (mNumColors + 1) + ColorIdx]);
			m_List.SetItemText(idx, mColorColumn[ColorIdx] + 1, tmp);
		}
	}

	return TRUE;
}

struct COMPARE_DATA
{
	CPropertiesPieces* Page;
	int Color;
};

static int CALLBACK ListViewCompareProc(LPARAM lP1, LPARAM lP2, LPARAM lParamData)
{
	COMPARE_DATA* data = (COMPARE_DATA*)lParamData;
	CPropertiesPieces* Page = data->Page;

	// First column.
	if (data->Color == -1)
	{
		// Keep "Total" row at the bottom.
		if (lP1 == -1)
			return 1;
		else if (lP2 == -1)
			return -1;

		return strcmpi(Page->mPieceNames[lP1], Page->mPieceNames[lP2]);
	}

	if (lP1 == -1)
		return 1;
	else if (lP2 == -1)
		return -1;

	int a = Page->mPieceColorCount[lP1 * (Page->mNumColors + 1) + data->Color];
	int b = Page->mPieceColorCount[lP2 * (Page->mNumColors + 1) + data->Color];

	if (a == b)
		return 0;
	else if (a < b)
		return -1;
	else
		return 1;
}

void CPropertiesPieces::OnColumnclickPropPiecesList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	COMPARE_DATA data;

	data.Page = this;
	data.Color = -1;

	if (pNMListView->iSubItem != 0)
	{
		for (int ColorIdx = 0; ColorIdx < mNumColors + 1; ColorIdx++)
		{
			if (mColorColumn[ColorIdx] == pNMListView->iSubItem - 1)
			{
				data.Color = ColorIdx;
				break;
			}
		}
	}

	m_List.SortItems((PFNLVCOMPARE)ListViewCompareProc, (LPARAM)&data);

	*pResult = 0;
}
