#include "lc_global.h"
#include "leocad.h"
#include <WindowsX.h>
#include "Print.h"
#include "project.h"
#include "pieceinf.h"
#include "globals.h"
#include "CADView.h"
#include "Tools.h"
#include "Piece.h"
#include "lc_library.h"
#include "lc_application.h"

// TODO: rewrite everything

static void PrintCatalogThread (CWnd* pParent, CFrameWndEx* pMainFrame)
{
	CCADView* pView = (CCADView*)pMainFrame->GetActiveView();
	CPrintDialog* PD = new CPrintDialog(FALSE, PD_ALLPAGES|PD_USEDEVMODECOPIES|PD_NOSELECTION|PD_ENABLEPRINTHOOK, pParent);
	lcPiecesLibrary *pLib = lcGetPiecesLibrary();

	int bricks = 0;
	for (int j = 0; j < pLib->mPieces.GetSize(); j++)
		if (pLib->mPieces[j]->m_strDescription[0] != '~')
			bricks++;
	int rows = theApp.GetProfileInt("Default", "Catalog Rows", 10);
	int cols = theApp.GetProfileInt("Default", "Catalog Columns", 3);
	PD->m_pd.lpfnPrintHook = PrintHookProc;
	PD->m_pd.nFromPage = PD->m_pd.nMinPage = 1; 
	PD->m_pd.nMaxPage = bricks/(rows*cols);
	if (bricks%(rows*cols) != 0) PD->m_pd.nMaxPage++;
	PD->m_pd.nToPage = PD->m_pd.nMaxPage;
	PD->m_pd.lCustData= (LONG)pMainFrame;

	// bring up the print dialog and allow user to change things
	if (theApp.DoPrintDialog(PD) != IDOK) return; 
	if (PD->m_pd.hDC == NULL) return;

	// update page range
	rows = theApp.GetProfileInt("Default","Catalog Rows", 10);
	cols = theApp.GetProfileInt("Default","Catalog Columns", 3);
	PD->m_pd.nMaxPage = bricks/(rows*cols);
	if (bricks%(rows*cols) != 0) PD->m_pd.nMaxPage++;

	// gather file to print to if print-to-file selected
	CString strOutput;
	if (PD->m_pd.Flags & PD_PRINTTOFILE)
	{
		// construct CFileDialog for browsing
		CString strDef(MAKEINTRESOURCE(AFX_IDS_PRINTDEFAULTEXT));
		CString strPrintDef(MAKEINTRESOURCE(AFX_IDS_PRINTDEFAULT));
		CString strFilter(MAKEINTRESOURCE(AFX_IDS_PRINTFILTER));
		CString strCaption(MAKEINTRESOURCE(AFX_IDS_PRINTCAPTION));
		CFileDialog dlg(FALSE, strDef, strPrintDef,
			OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, strFilter);
		dlg.m_ofn.lpstrTitle = strCaption;

		if (dlg.DoModal() != IDOK)
			return;

		// set output device to resulting path name
		strOutput = dlg.GetPathName();
	}

	DOCINFO docInfo;
	memset(&docInfo, 0, sizeof(DOCINFO));
	docInfo.cbSize = sizeof(DOCINFO);
	docInfo.lpszDocName = "LeoCAD pieces catalog";
	CString strPortName;
	int nFormatID;
	if (strOutput.IsEmpty())
	{
		docInfo.lpszOutput = NULL;
		strPortName = PD->GetPortName();
		nFormatID = AFX_IDS_PRINTONPORT;
	}
	else
	{
		docInfo.lpszOutput = strOutput;
		AfxGetFileTitle(strOutput, strPortName.GetBuffer(_MAX_PATH), _MAX_PATH);
		nFormatID = AFX_IDS_PRINTTOFILE;
	}

	// setup the printing DC
	SetAbortProc(PD->m_pd.hDC, _AfxAbortProc);

	// disable main window while printing & init printing status dialog
	pParent->EnableWindow(FALSE);
	CPrintingDialog dlgPrintStatus(NULL);

	CString strTemp;
	dlgPrintStatus.SetDlgItemText(AFX_IDC_PRINT_DOCNAME, "LeoCAD parts catalog");
	dlgPrintStatus.SetDlgItemText(AFX_IDC_PRINT_PRINTERNAME, PD->GetDeviceName());
	AfxFormatString1(strTemp, nFormatID, strPortName);
	dlgPrintStatus.SetDlgItemText(AFX_IDC_PRINT_PORTNAME, strTemp);
	dlgPrintStatus.ShowWindow(SW_SHOW);
	dlgPrintStatus.UpdateWindow();

	// start document printing process
	if (StartDoc(PD->m_pd.hDC, &docInfo) == SP_ERROR)
	{
		pParent->EnableWindow(TRUE);
		dlgPrintStatus.DestroyWindow();
		AfxMessageBox(AFX_IDP_FAILED_TO_START_PRINT);
		return;
	}

	// Guarantee values are in the valid range
	UINT nEndPage = PD->m_pd.nToPage;
	UINT nStartPage = PD->m_pd.nFromPage;

	if (PD->PrintAll())
	{
		nEndPage = PD->m_pd.nMaxPage;
		nStartPage = PD->m_pd.nMinPage;
	}

	if (nEndPage < PD->m_pd.nMinPage) nEndPage = PD->m_pd.nMinPage;
	if (nEndPage > PD->m_pd.nMaxPage) nEndPage = PD->m_pd.nMaxPage;
	if (nStartPage < PD->m_pd.nMinPage) nStartPage = PD->m_pd.nMinPage;
	if (nStartPage > PD->m_pd.nMaxPage) nStartPage = PD->m_pd.nMaxPage;

	int nStep = (nEndPage >= nStartPage) ? 1 : -1;
	nEndPage = nEndPage + nStep;

	VERIFY(strTemp.LoadString(AFX_IDS_PRINTPAGENUM));

	// begin page printing loop
	BOOL bError = FALSE;

	// set up drawing rect to entire page (in logical coordinates)
	CRect rectDraw (0, 0, GetDeviceCaps(PD->m_pd.hDC, HORZRES), GetDeviceCaps(PD->m_pd.hDC, VERTRES));
	DPtoLP(PD->m_pd.hDC, (LPPOINT)(RECT*)&rectDraw, 2);
	rectDraw.DeflateRect(
		GetDeviceCaps(PD->m_pd.hDC, LOGPIXELSX)*theApp.GetProfileInt("Default","Margin Left", 50)/100, 
		GetDeviceCaps(PD->m_pd.hDC, LOGPIXELSY)*theApp.GetProfileInt("Default","Margin Top", 50)/100,
		GetDeviceCaps(PD->m_pd.hDC, LOGPIXELSX)*theApp.GetProfileInt("Default","Margin Right", 50)/100, 
		GetDeviceCaps(PD->m_pd.hDC, LOGPIXELSY)*theApp.GetProfileInt("Default","Margin Bottom", 50)/100);
	int w = rectDraw.Width()/cols;
	int h = rectDraw.Height()/rows;

	// Creating Compatible Memory Device Context
	CDC *pMemDC = new CDC;
	if (!pMemDC->CreateCompatibleDC(pView->GetDC()))
		return;

	// Preparing bitmap header for DIB section
	BITMAPINFO bi;
	ZeroMemory(&bi, sizeof(BITMAPINFO));
	
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = w;
	bi.bmiHeader.biHeight = h;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 24;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biSizeImage = w * h * (24/8);
	bi.bmiHeader.biXPelsPerMeter = 2925;
	bi.bmiHeader.biYPelsPerMeter = 2925;
	bi.bmiHeader.biClrUsed = 0;
	bi.bmiHeader.biClrImportant = 0;
	
	LPBITMAPINFOHEADER lpbi[1];

	// Creating a DIB surface
	HBITMAP hBm, hBmOld;
	hBm = CreateDIBSection(pView->GetDC()->GetSafeHdc(), &bi, DIB_RGB_COLORS, 
		(void **)&lpbi, NULL, (DWORD)0);
	if (!hBm)
		return;

	// Selecting the DIB Surface
	hBmOld = (HBITMAP)::SelectObject(pMemDC->GetSafeHdc(), hBm);
	if (!hBmOld)
		return;

	// Setting up a Pixel format for the DIB surface
	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
			1,PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL | PFD_SUPPORT_GDI,
			PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16,
			0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 };
	int pixelformat = ChoosePixelFormat(pMemDC->m_hDC, &pfd);
	DescribePixelFormat(pMemDC->m_hDC, pixelformat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
	SetPixelFormat(pMemDC->m_hDC, pixelformat, &pfd);
	
	// Creating a OpenGL context
	HGLRC hmemrc = wglCreateContext(pMemDC->GetSafeHdc());
	
	// Setting up the current OpenGL context
	GL_DisableVertexBufferObject();
	wglMakeCurrent(pMemDC->GetSafeHdc(), hmemrc);
	double aspect = (float)w/(float)h;
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, w, h);

	// Sort pieces by description
	struct BRICKSORT {
		char name[64];
		int actual;
		struct BRICKSORT *next;
	} start, *node, *previous, *news;

	start.next = NULL;
	
	for (int j = 0; j < pLib->mPieces.GetSize(); j++)
	{
		char* desc = pLib->mPieces[j]->m_strDescription;

		if (desc[0] != '~')
			continue;

		// Find the correct location
		previous = &start;
		node = start.next;
		while ((node) && (strcmp(desc, node->name) > 0))
		{
			node = node->next;
			previous = previous->next;
		}
		
		news = (struct BRICKSORT*) malloc(sizeof(struct BRICKSORT));
		news->next = node;
		previous->next = news;
		strcpy(news->name, desc);
		news->actual = j;
	}
	
	node = start.next;

	if (PD->PrintRange())
	{
		for (int j = 0; j < (int)(nStartPage - 1)*rows*cols; j++)
			if (node)
				node = node->next;
	}

	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = -MulDiv(12, GetDeviceCaps(PD->m_pd.hDC, LOGPIXELSY), 72);
	lf.lfWeight = FW_REGULAR;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfQuality = PROOF_QUALITY;
	strcpy (lf.lfFaceName , "Arial");
	HFONT HeaderFont = CreateFontIndirect(&lf);
	HFONT OldFont = (HFONT)SelectObject(PD->m_pd.hDC, HeaderFont);
	SetBkMode(PD->m_pd.hDC, TRANSPARENT);
	SetTextColor(PD->m_pd.hDC, 0x000000);
	SetTextAlign (PD->m_pd.hDC, TA_TOP|TA_LEFT|TA_NOUPDATECP);

	SetTextColor (pMemDC->m_hDC, 0x000000);
	lf.lfHeight = -MulDiv(10, GetDeviceCaps(pMemDC->m_hDC, LOGPIXELSY), 72);
	lf.lfWeight = FW_BOLD;
	HFONT CatalogFont = CreateFontIndirect(&lf);
	HFONT OldMemFont = (HFONT)SelectObject(pMemDC->m_hDC, CatalogFont);
	HPEN hpOld = (HPEN)SelectObject(pMemDC->m_hDC,(HPEN)GetStockObject(BLACK_PEN));

	for (UINT nCurPage = nStartPage; nCurPage != nEndPage; nCurPage += nStep)
	{
		// write current page
		TCHAR szBuf[80];
		wsprintf(szBuf, strTemp, nCurPage);
		dlgPrintStatus.SetDlgItemText(AFX_IDC_PRINT_PAGENUM, szBuf);

		// attempt to start the current page
		if (StartPage(PD->m_pd.hDC) < 0)
		{
			bError = TRUE;
			break;
		}

		int printed = 0;
		// page successfully started, so now render the page
		for (int r = 0; r < rows; r++)
		for (int c = 0; c < cols; c++)
		{
			if (node == NULL) continue;
			printed++;
			glDepthFunc(GL_LEQUAL);
			glClearColor(1,1,1,1); 
			glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
			glEnable(GL_COLOR_MATERIAL);
			glDisable(GL_DITHER);
			glShadeModel(GL_FLAT);

			lcSetColor(lcGetActiveProject()->GetCurrentColor());

//			dlgPrintStatus.SetDlgItemText(AFX_IDC_PRINT_DOCNAME, node->name);
			node = node->next;
			PieceInfo* pInfo = pLib->mPieces[node->actual];
			pInfo->ZoomExtents(30.0f, (float)aspect);

			float pos[4] = { 0, 0, 10, 0 };
			glLightfv(GL_LIGHT0, GL_POSITION, pos);

			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
			glEnable(GL_DEPTH_TEST);

			FillRect(pMemDC->m_hDC, CRect(0,h,w,0), (HBRUSH)GetStockObject(WHITE_BRUSH));
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			pInfo->RenderPiece(lcGetActiveProject()->GetCurrentColor());
			glFlush();

			TextOut (pMemDC->m_hDC, 5, 5, pInfo->m_strDescription, strlen(pInfo->m_strDescription));
//			BitBlt(PD->m_pd.hDC, rectDraw.left+(w*c), rectDraw.top+(h*r), w, h, pMemDC->m_hDC, 0, 0, SRCCOPY);

			LPBITMAPINFOHEADER lpbi[1];
			lpbi[0] = (LPBITMAPINFOHEADER)GlobalLock(MakeDib(hBm, 24));
			BITMAPINFO bi;
			ZeroMemory(&bi, sizeof(BITMAPINFO));
			memcpy (&bi.bmiHeader, lpbi[0], sizeof(BITMAPINFOHEADER));
			SetStretchBltMode(PD->m_pd.hDC, COLORONCOLOR);
			StretchDIBits(PD->m_pd.hDC, rectDraw.left+(w*c), rectDraw.top+(h*r), 
				w, h, 0, 0, w, h, (LPBYTE) lpbi[0] + lpbi[0]->biSize + lpbi[0]->biClrUsed * sizeof(RGBQUAD),
				&bi, DIB_RGB_COLORS, SRCCOPY);
			if (lpbi[0]) GlobalFreePtr(lpbi[0]);
		}

		DWORD dwPrint = theApp.GetProfileInt("Settings","Print", PRINT_NUMBERS|PRINT_BORDER);
		if (dwPrint & PRINT_BORDER)
		for (int r = 0; r < rows; r++)
		for (int c = 0; c < cols; c++)
		{
			if (printed == 0) continue;
			printed--;
			if (r == 0)
			{
				MoveToEx(PD->m_pd.hDC, rectDraw.left+(w*c), rectDraw.top+(h*r), NULL);
				LineTo(PD->m_pd.hDC, rectDraw.left+(w*(c+1)), rectDraw.top+(h*r));
			}
			if (c == 0)
			{
				MoveToEx(PD->m_pd.hDC, rectDraw.left+(w*c), rectDraw.top+(h*r), NULL);
				LineTo(PD->m_pd.hDC, rectDraw.left+(w*c), rectDraw.top+(h*(r+1)));
			}
			
			MoveToEx(PD->m_pd.hDC, rectDraw.left+(w*(c+1)), rectDraw.top+(h*r), NULL);
			LineTo(PD->m_pd.hDC, rectDraw.left+(w*(c+1)), rectDraw.top+(h*(r+1)));
			MoveToEx(PD->m_pd.hDC, rectDraw.left+(w*c), rectDraw.top+(h*(r+1)), NULL);
			LineTo(PD->m_pd.hDC, rectDraw.left+(w*(c+1)), rectDraw.top+(h*(r+1)));
		}

		CRect r2 = rectDraw;
		r2.top -= GetDeviceCaps(PD->m_pd.hDC, LOGPIXELSY)*theApp.GetProfileInt("Default","Margin Top", 50)/200;
		r2.bottom += GetDeviceCaps(PD->m_pd.hDC, LOGPIXELSY)*theApp.GetProfileInt("Default","Margin Bottom", 50)/200;
		pView->PrintHeader(FALSE, PD->m_pd.hDC, r2, nCurPage, nEndPage, TRUE);
		pView->PrintHeader(TRUE, PD->m_pd.hDC, r2, nCurPage, nEndPage, TRUE);

		if (EndPage(PD->m_pd.hDC) < 0 || !_AfxAbortProc(PD->m_pd.hDC, 0))
		{
			bError = TRUE;
			break;
		}
	}

	SelectObject(pMemDC->m_hDC, hpOld);
	SelectObject(PD->m_pd.hDC, OldFont);
	DeleteObject(HeaderFont);
	SelectObject(pMemDC->m_hDC, OldMemFont);
	DeleteObject(CatalogFont);

	node = start.next;
	while (node)
	{
		previous = node;
		node = node->next;
		free(previous);
	}

	GL_EnableVertexBufferObject();
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hmemrc);
	SelectObject(pMemDC->GetSafeHdc(), hBmOld);
	DeleteObject(hBm);
	delete pMemDC;

	// cleanup document printing process
	if (!bError)
		EndDoc(PD->m_pd.hDC);
	else
		AbortDoc(PD->m_pd.hDC);

	pParent->EnableWindow();
	dlgPrintStatus.DestroyWindow();

    if (PD != NULL && PD->m_pd.hDC != NULL)
    {
        ::DeleteDC(PD->m_pd.hDC);
        PD->m_pd.hDC = NULL;
    }
    delete PD;
}

UINT PrintCatalogFunction (LPVOID pv)
{
	PRINT_PARAMS* param = (PRINT_PARAMS*)pv;
	PrintCatalogThread (param->pParent, param->pMainFrame);
	param->pParent->EnableWindow (TRUE);
	free(param);
	return 0;
}

int PiecesUsedSortFunc(const lcPiecesUsedEntry& a, const lcPiecesUsedEntry& b, void* Data)
{
	if (a.ColorIndex == b.ColorIndex)
	{
		if (a.Info < b.Info)
			return -1;
		else
			return 1;
	}
	else
	{
		if (a.ColorIndex < b.ColorIndex)
			return -1;
		else
			return 1;
	}

	return 0;
}

void FormatHeader(CString& Result, UINT& Align, const char* Format, const char* Title, const char* Author, const char* Description, int CurPage, int TotalPages)
{
	char Buffer[128];
	const char* Ptr = Format;

	Result.Empty();
	Align = DT_CENTER;

	while (*Ptr)
	{
		if (*Ptr != '&')
		{
			Result.AppendChar(*Ptr);
			Ptr++;
			continue;
		}

		if (Ptr[1] == '&')
		{
			Result.AppendChar(*Ptr);
			Ptr++;
			Ptr++;
			continue;
		}

		switch (Ptr[1])
		{
		case 'L':
			Align = DT_LEFT;
			Ptr++;
			break;

		case 'C':
			Align = DT_CENTER;
			Ptr++;
			break;

		case 'R':
			Align = DT_RIGHT;
			Ptr++;
			break;

		case 'F':
			Result.Append(Title);
			Ptr++;
			break;

		case 'A':
			Result.Append(Author);
			Ptr++;
			break;

		case 'N':
			Result.Append(Description);
			Ptr++;
			break;

		case 'D':
			_strdate(Buffer);
			Result.Append(Buffer);
			Ptr++;
			break;

		case 'T':
			_strtime(Buffer);
			Result.Append(Buffer);
			Ptr++;
			break;

		case 'P':
			sprintf(Buffer, "%d", CurPage);
			Result.Append(Buffer);
			Ptr++;
			break;

		case 'O':
			sprintf(Buffer, "%d", TotalPages);
			Result.Append(Buffer);
			Ptr++;
			break;

		case 0:
		default:
			Result.AppendChar(*Ptr);
			break;
		}

		Ptr++;
	}
}

static void PrintPiecesThread(void* pv)
{
	CFrameWndEx* pFrame = (CFrameWndEx*)pv;
	CView* pView = pFrame->GetActiveView();
	CPrintDialog PD(FALSE, PD_ALLPAGES|PD_USEDEVMODECOPIES|PD_NOPAGENUMS|PD_NOSELECTION, pFrame);

	if (theApp.DoPrintDialog(&PD) != IDOK)
		return; 

	if (PD.m_pd.hDC == NULL)
		return;

	Project* project = lcGetActiveProject();
	ObjArray<lcPiecesUsedEntry> PiecesUsed;

	project->GetPiecesUsed(PiecesUsed);
	PiecesUsed.Sort(PiecesUsedSortFunc, NULL);

	// gather file to print to if print-to-file selected
	CString strOutput;
	if (PD.m_pd.Flags & PD_PRINTTOFILE)
	{
		CString strDef(MAKEINTRESOURCE(AFX_IDS_PRINTDEFAULTEXT));
		CString strPrintDef(MAKEINTRESOURCE(AFX_IDS_PRINTDEFAULT));
		CString strFilter(MAKEINTRESOURCE(AFX_IDS_PRINTFILTER));
		CString strCaption(MAKEINTRESOURCE(AFX_IDS_PRINTCAPTION));
		CFileDialog dlg(FALSE, strDef, strPrintDef,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, strFilter);
		dlg.m_ofn.lpstrTitle = strCaption;
		if (dlg.DoModal() != IDOK)
			return;
		strOutput = dlg.GetPathName();
	}

	CString DocName;
	char* Ext = strrchr(project->m_strTitle, '.');
	DocName.Format("LeoCAD - %.*s BOM", Ext ? Ext - project->m_strTitle : strlen(project->m_strTitle), project->m_strTitle);
	DOCINFO docInfo;
	memset(&docInfo, 0, sizeof(DOCINFO));
	docInfo.cbSize = sizeof(DOCINFO);
	docInfo.lpszDocName = DocName;
	CString strPortName;

	int nFormatID;
	if (strOutput.IsEmpty())
	{
		docInfo.lpszOutput = NULL;
		strPortName = PD.GetPortName();
		nFormatID = AFX_IDS_PRINTONPORT;
	}
	else
	{
		docInfo.lpszOutput = strOutput;
		AfxGetFileTitle(strOutput, strPortName.GetBuffer(_MAX_PATH), _MAX_PATH);
		nFormatID = AFX_IDS_PRINTTOFILE;
	}

	SetAbortProc(PD.m_pd.hDC, _AfxAbortProc);
	pFrame->EnableWindow(FALSE);
	CPrintingDialog dlgPrintStatus(NULL);

	CString strTemp;
	dlgPrintStatus.SetDlgItemText(AFX_IDC_PRINT_DOCNAME, DocName);
	dlgPrintStatus.SetDlgItemText(AFX_IDC_PRINT_PRINTERNAME, PD.GetDeviceName());
	AfxFormatString1(strTemp, nFormatID, strPortName);
	dlgPrintStatus.SetDlgItemText(AFX_IDC_PRINT_PORTNAME, strTemp);
	dlgPrintStatus.ShowWindow(SW_SHOW);
	dlgPrintStatus.UpdateWindow();

	if (StartDoc(PD.m_pd.hDC, &docInfo) == SP_ERROR)
	{
		pFrame->EnableWindow(TRUE);
		dlgPrintStatus.DestroyWindow();
		AfxMessageBox(AFX_IDP_FAILED_TO_START_PRINT);
		return;
	}

	int ResX = GetDeviceCaps(PD.m_pd.hDC, LOGPIXELSX);
	int ResY = GetDeviceCaps(PD.m_pd.hDC, LOGPIXELSY);

	CRect RectDraw(0, 0, GetDeviceCaps(PD.m_pd.hDC, HORZRES), GetDeviceCaps(PD.m_pd.hDC, VERTRES));
	DPtoLP(PD.m_pd.hDC, (LPPOINT)(RECT*)&RectDraw, 2);
	RectDraw.DeflateRect((int)(ResX*(float)theApp.GetProfileInt("Default","Margin Left", 50)/100.0f),
	                     (int)(ResY*(float)theApp.GetProfileInt("Default","Margin Top", 50)/100.0f),
	                     (int)(ResX*(float)theApp.GetProfileInt("Default","Margin Right", 50)/100.0f),
	                     (int)(ResY*(float)theApp.GetProfileInt("Default","Margin Bottom", 50)/100.0f));

	CRect HeaderRect = RectDraw;
 	HeaderRect.top -= (int)(ResY*theApp.GetProfileInt("Default", "Margin Top", 50) / 200.0f);
	HeaderRect.bottom += (int)(ResY*theApp.GetProfileInt("Default", "Margin Bottom", 50) / 200.0f);

	int RowsPerPage = AfxGetApp()->GetProfileInt("Default", "Catalog Rows", 10);
	int ColsPerPage = AfxGetApp()->GetProfileInt("Default", "Catalog Columns", 3);
	int PicHeight = RectDraw.Height() / RowsPerPage;
	int PicWidth = RectDraw.Width() / ColsPerPage;
	int TotalRows = (PiecesUsed.GetSize() + ColsPerPage - 1) / ColsPerPage;
	int TotalPages = (TotalRows + RowsPerPage - 1) / RowsPerPage;
	int RowHeight = RectDraw.Height() / RowsPerPage;
	int ColWidth = RectDraw.Width() / ColsPerPage;

	PD.m_pd.nMinPage = 1;
	PD.m_pd.nMaxPage = TotalPages + 1;

	UINT EndPage = PD.m_pd.nToPage;
	UINT StartPage = PD.m_pd.nFromPage;
	if (PD.PrintAll())
	{
		EndPage = PD.m_pd.nMaxPage;
		StartPage = PD.m_pd.nMinPage;
	}

	lcClamp(EndPage, PD.m_pd.nMinPage, PD.m_pd.nMaxPage);
	lcClamp(StartPage, PD.m_pd.nMinPage, PD.m_pd.nMaxPage);
	int StepPage = (EndPage >= StartPage) ? 1 : -1;

	VERIFY(strTemp.LoadString(AFX_IDS_PRINTPAGENUM));

	// begin page printing loop
	BOOL bError = FALSE;

	// Creating Compatible Memory Device Context
	CDC *pMemDC = new CDC;
	if (!pMemDC->CreateCompatibleDC(pView->GetDC()))
		return;

	BITMAPINFO bi;
	ZeroMemory(&bi, sizeof(BITMAPINFO));
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = PicWidth;
	bi.bmiHeader.biHeight = PicHeight;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 24;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biSizeImage = PicWidth * PicHeight * 3;
	bi.bmiHeader.biXPelsPerMeter = 2925;
	bi.bmiHeader.biYPelsPerMeter = 2925;
	bi.bmiHeader.biClrUsed = 0;
	bi.bmiHeader.biClrImportant = 0;
	
	LPBITMAPINFOHEADER lpbi[1];

	HBITMAP hBm, hBmOld;
    hBm = CreateDIBSection(pView->GetDC()->GetSafeHdc(), &bi, DIB_RGB_COLORS, (void **)&lpbi, NULL, (DWORD)0);
	if (!hBm)
		return;
	hBmOld = (HBITMAP)::SelectObject(pMemDC->GetSafeHdc(), hBm);

	PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL | PFD_SUPPORT_GDI,
			PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 };
	int pixelformat = ChoosePixelFormat(pMemDC->m_hDC, &pfd);
	DescribePixelFormat(pMemDC->m_hDC, pixelformat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
	SetPixelFormat(pMemDC->m_hDC, pixelformat, &pfd);
	HGLRC hmemrc = wglCreateContext(pMemDC->GetSafeHdc());
	wglMakeCurrent(pMemDC->GetSafeHdc(), hmemrc);

	GL_DisableVertexBufferObject();
	float Aspect = (float)PicWidth/(float)PicHeight;

	glViewport(0, 0, PicWidth, PicHeight);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.5f, 0.1f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(1, 1, 1, 1); 

	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = -MulDiv(12, ResY, 72);
	lf.lfWeight = FW_REGULAR;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfQuality = PROOF_QUALITY;
	strcpy (lf.lfFaceName , "Arial");

	HFONT HeaderFont = CreateFontIndirect(&lf);
	HFONT OldFont = (HFONT)SelectObject(PD.m_pd.hDC, HeaderFont);
	SetBkMode(PD.m_pd.hDC, TRANSPARENT);
	SetTextColor(PD.m_pd.hDC, 0x000000);
	SetTextAlign(PD.m_pd.hDC, TA_CENTER|TA_NOUPDATECP);

	DWORD PrintOptions = AfxGetApp()->GetProfileInt("Settings", "Print", PRINT_NUMBERS | PRINT_BORDER/*|PRINT_NAMES*/);
	bool DrawNames = 1;//(PrintOptions & PRINT_NAMES) != 0;
	bool Horizontal = 1;//(PrintOptions & PRINT_HORIZONTAL) != 0;

	pMemDC->SetTextColor(0x000000);
	pMemDC->SetBkMode(TRANSPARENT);
//	lf.lfHeight = -MulDiv(40, GetDeviceCaps(pMemDC->m_hDC, LOGPIXELSY), 72);
//	lf.lfWeight = FW_BOLD;
	HFONT CatalogFont = CreateFontIndirect(&lf);
	lf.lfHeight = -MulDiv(80, GetDeviceCaps(pMemDC->m_hDC, LOGPIXELSY), 72);
	HFONT CountFont = CreateFontIndirect(&lf);
	HFONT OldMemFont = (HFONT)SelectObject(pMemDC->m_hDC, CatalogFont);
	HPEN hpOld = (HPEN)SelectObject(pMemDC->m_hDC, GetStockObject(BLACK_PEN));

	for (UINT CurPage = StartPage; CurPage != EndPage; CurPage += StepPage)
	{
		TCHAR szBuf[80];
		wsprintf(szBuf, strTemp, CurPage);
		dlgPrintStatus.SetDlgItemText(AFX_IDC_PRINT_PAGENUM, szBuf);
		if (::StartPage(PD.m_pd.hDC) < 0)
		{
			bError = TRUE;
			break;
		}

		// Draw header and footer.
		SelectObject(PD.m_pd.hDC, HeaderFont);

		CString Header;
		UINT Align;

		FormatHeader(Header, Align, AfxGetApp()->GetProfileString("Default", "Catalog Header", ""), project->m_strTitle, project->m_strAuthor, project->m_strDescription, CurPage, TotalPages);
		Align |= DT_TOP|DT_SINGLELINE;

		DrawText(PD.m_pd.hDC, (LPCTSTR)Header, Header.GetLength(), HeaderRect, Align);

		FormatHeader(Header, Align, AfxGetApp()->GetProfileString("Default", "Catalog Footer", "Page &P"), project->m_strTitle, project->m_strAuthor, project->m_strDescription, CurPage, TotalPages);
		Align |= DT_BOTTOM|DT_SINGLELINE;

		DrawText(PD.m_pd.hDC, (LPCTSTR)Header, Header.GetLength(), HeaderRect, Align);

		int StartPiece = (CurPage - 1) * RowsPerPage * ColsPerPage;
		int EndPiece = lcMin(StartPiece + RowsPerPage * ColsPerPage, PiecesUsed.GetSize());

		for (int CurPiece = StartPiece; CurPiece < EndPiece; CurPiece++)
		{
			FillRect(pMemDC->m_hDC, CRect(0, PicHeight, PicWidth, 0), (HBRUSH)GetStockObject(WHITE_BRUSH));
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			PieceInfo* pInfo = PiecesUsed[CurPiece].Info;
			pInfo->ZoomExtents(30.0f, Aspect);

			pInfo->RenderPiece(PiecesUsed[CurPiece].ColorIndex);
			glFinish();

			// Draw description text at the bottom.
			CRect TextRect(0, 0, PicWidth, PicHeight);
	
			if (DrawNames)
			{
				SelectObject(pMemDC->m_hDC, CatalogFont);
				pMemDC->DrawText(pInfo->m_strDescription, strlen(pInfo->m_strDescription), TextRect, DT_CALCRECT | DT_WORDBREAK);

				TextRect.OffsetRect(0, PicHeight - TextRect.Height() - 5);
				pMemDC->DrawText(pInfo->m_strDescription, strlen(pInfo->m_strDescription), TextRect, DT_WORDBREAK);
			}

			// Draw count.
			SelectObject(pMemDC->m_hDC, CountFont);
			TextRect = CRect(0, 0, PicWidth, TextRect.top);
			TextRect.DeflateRect(5, 5);

			char CountStr[16];
			sprintf(CountStr, "%dx", PiecesUsed[CurPiece].Count);
			pMemDC->DrawText(CountStr, strlen(CountStr), TextRect, DT_BOTTOM | DT_LEFT | DT_SINGLELINE);

			LPBITMAPINFOHEADER lpbi[1];
			lpbi[0] = (LPBITMAPINFOHEADER)GlobalLock(MakeDib(hBm, 24));
			BITMAPINFO bi;
			ZeroMemory(&bi, sizeof(BITMAPINFO));
			memcpy (&bi.bmiHeader, lpbi[0], sizeof(BITMAPINFOHEADER));
			SetStretchBltMode(PD.m_pd.hDC, COLORONCOLOR);

			int CurRow, CurCol;

			if (Horizontal)
			{
				CurRow = (CurPiece - StartPiece) / ColsPerPage;
				CurCol = (CurPiece - StartPiece) % ColsPerPage;
			}
			else
			{
				CurRow = (CurPiece - StartPiece) % RowsPerPage;
				CurCol = (CurPiece - StartPiece) / RowsPerPage;
			}
			
			int Left = RectDraw.left + ColWidth * CurCol + (ColWidth - PicWidth) / 2;
			int Top = RectDraw.top + RowHeight * CurRow + (RowHeight - PicHeight) / 2;

			StretchDIBits(PD.m_pd.hDC, Left, Top, PicWidth, PicHeight, 0, 0, PicWidth, PicHeight, 
			              (LPBYTE)lpbi[0] + lpbi[0]->biSize + lpbi[0]->biClrUsed * sizeof(RGBQUAD), &bi, DIB_RGB_COLORS, SRCCOPY);
			if (lpbi[0])
				GlobalFreePtr(lpbi[0]);
		}

		if (::EndPage(PD.m_pd.hDC) < 0 || !_AfxAbortProc(PD.m_pd.hDC, 0))
		{
			bError = TRUE;
			break;
		}
	}

	SelectObject(pMemDC->m_hDC, hpOld);
	SelectObject(PD.m_pd.hDC, OldFont);
	DeleteObject(HeaderFont);
	SelectObject(pMemDC->m_hDC, OldMemFont);
	DeleteObject(CatalogFont);
	DeleteObject(CountFont);

	GL_EnableVertexBufferObject();
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hmemrc);
	SelectObject(pMemDC->GetSafeHdc(), hBmOld);
	DeleteObject(hBm);
	delete pMemDC;

	if (!bError)
		EndDoc(PD.m_pd.hDC);
	else
		AbortDoc(PD.m_pd.hDC);

	pFrame->EnableWindow();
	dlgPrintStatus.DestroyWindow();

	if (PD.m_pd.hDC != NULL)
    {
		::DeleteDC(PD.m_pd.hDC);
		PD.m_pd.hDC = NULL;
    }
}

UINT PrintPiecesFunction (LPVOID pv)
{
	PrintPiecesThread(pv);
	CFrameWndEx* pFrame = (CFrameWndEx*)pv;
	pFrame->EnableWindow(TRUE);
	return 0;
}
