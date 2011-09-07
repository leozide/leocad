BOOL CALLBACK _AfxAbortProc(HDC, int);
UINT AFXAPI AfxGetFileTitle(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax);

#define AFX_IDD_PRINTDLG                30722

typedef struct {
	CWnd* pParent;
	CFrameWnd* pMainFrame;
} PRINT_PARAMS;

class CPrintingDialog : public CDialog
{
public:
	//{{AFX_DATA(CPrintingDialog)
	enum { IDD = AFX_IDD_PRINTDLG };
	//}}AFX_DATA
	CPrintingDialog(CWnd* pParent);
	virtual ~CPrintingDialog();

	virtual BOOL OnInitDialog();
	virtual void OnCancel();
};

UINT PrintCatalogFunction (LPVOID pv);
UINT PrintPiecesFunction (LPVOID pv);

