#include "lc_global.h"
#include "resource.h"
#include "PropsPgs.h"
#include "defines.h"

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
}

CPropertiesPieces::~CPropertiesPieces()
{
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

	char tmp[64];
	int i, j;
	memset (&totalcount, 0, sizeof (totalcount));
	for (i = 0; i < lines; i++)
		for (j = 0; j < LC_MAXCOLORS; j++)
			totalcount[j] += count[i*LC_MAXCOLORS+j];

	int ID = 0;
	m_List.InsertColumn(0, "Piece", LVCFMT_LEFT, 130, 0);
	for (i = 0; i < LC_MAXCOLORS; i++)
		if (totalcount[i])
		{
			col[i] = ID;
			ID++;

			CString str;
			str.LoadString(IDS_COLOR01 + i);
			m_List.InsertColumn(ID, (LPCSTR)str, LVCFMT_LEFT, 80, 0);
		}
		else
			col[i] = -1;
	ID++;
	m_List.InsertColumn(ID, "Total", LVCFMT_LEFT, 60, 0);

	for (i = 0; i < lines; i++)
	{
		int total = 0;

		for (j = 0; j < LC_MAXCOLORS; j++)
			total += count[i*LC_MAXCOLORS+j];

		if (total == 0)
			continue;

		char name[65];
		LV_ITEM lvi;
		lvi.mask = LVIF_TEXT|LVIF_PARAM;
		lvi.iItem = 0;
		lvi.iSubItem = 0;
		lvi.pszText = name;
		lvi.lParam = i;
		strcpy (name, names[i]);
		int idx = m_List.InsertItem(&lvi);

		for (j = 0; j < LC_MAXCOLORS; j++)
//			if (totalcount[j])
			if (count[i*LC_MAXCOLORS+j])
			{
				sprintf (tmp, "%d", count[i*LC_MAXCOLORS+j]);
				lvi.iItem = idx;
				lvi.pszText = tmp;
				m_List.SetItemText(idx, col[j] + 1, tmp);
			}

		sprintf (tmp, "%d", total);
		lvi.iItem = idx;
		lvi.pszText = tmp;
		m_List.SetItemText(idx, ID, tmp);
	}

	m_List.ModifyStyle(LVS_SORTASCENDING | LVS_SORTDESCENDING, 0L);

	char name[65];
	strcpy (name, "Total");
	LV_ITEM lvi;
	lvi.mask = LVIF_TEXT|LVIF_PARAM;
	lvi.iItem = m_List.GetItemCount();
	lvi.iSubItem = 0;
	lvi.pszText = name;
	lvi.lParam = -1;
	int idx = m_List.InsertItem(&lvi), total = 0;

	for (i = 0; i < LC_MAXCOLORS; i++)
		if (totalcount[i])
		{
			sprintf (tmp, "%d", totalcount[i]);
			lvi.iItem = idx;
			lvi.pszText = tmp;
			m_List.SetItemText(idx, col[i] + 1, tmp);
			total += totalcount[i];
		}

	sprintf (tmp, "%d", total);
	lvi.iItem = idx;
	lvi.pszText = tmp;
	m_List.SetItemText(idx, ID, tmp);
	
	return TRUE;
}

typedef struct
{
	CPropertiesPieces* page;
	int color;
} COMPARE_DATA;

static int CALLBACK ListViewCompareProc(LPARAM lP1, LPARAM lP2, LPARAM lParamData)
{
	int i, a, b;
	COMPARE_DATA* data = (COMPARE_DATA*)lParamData;

	if (data->color == -1)
	{
		// check if we're comparing the "total" row
		if (lP1 == -1)
			return 1;
		else if (lP2 == -1)
			return -1;

		return strcmpi(data->page->names[lP1], data->page->names[lP2]);
	}

	// last column
	if (data->color == LC_MAXCOLORS)
	{
		a = b = 0;
		for (i = 0; i < LC_MAXCOLORS; i++)
		{
			a += data->page->count[lP1*LC_MAXCOLORS+i];
			b += data->page->count[lP2*LC_MAXCOLORS+i];
		}
	}
	else
	{
		if (lP1 == -1)
			a = data->page->totalcount[data->color];
		else
			a = data->page->count[lP1*LC_MAXCOLORS+data->color];
		
		if (lP2 == -1)
			b = data->page->totalcount[data->color];
		else
			b = data->page->count[lP2*LC_MAXCOLORS+data->color];
	}

	if (a == b)
		return 0;

	if (a < b)
		return -1;
	else
		return 1;
}

void CPropertiesPieces::OnColumnclickPropPiecesList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int i;
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	COMPARE_DATA data;

	data.page = this;

	if (pNMListView->iSubItem == 0)
		data.color = -1;
	else
	{
		for (i = 0; i < LC_MAXCOLORS; i++)
			if (col[i] == pNMListView->iSubItem-1)
				break;

		data.color = i;
	}

	m_List.SortItems((PFNLVCOMPARE)ListViewCompareProc, (LPARAM)&data);

	*pResult = 0;
}
