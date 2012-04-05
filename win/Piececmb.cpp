// PieceCmb.cpp : implementation file
//

#include "stdafx.h"
#include "leocad.h"
#include "PieceCmb.h"
#include "PieceBar.h"
#include "pieceinf.h"
#include "project.h"
#include "globals.h"
#include "library.h"
#include "lc_application.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPiecesCombo

CPiecesCombo::CPiecesCombo()
{
	m_bAutoComplete = TRUE;
}

CPiecesCombo::~CPiecesCombo()
{
}


BEGIN_MESSAGE_MAP(CPiecesCombo, CComboBox)
	//{{AFX_MSG_MAP(CPiecesCombo)
	ON_CONTROL_REFLECT(CBN_EDITUPDATE, OnEditupdate)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnSelchange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPiecesCombo message handlers

void CPiecesCombo::OnEditupdate() 
{
	if (!m_bAutoComplete)
		return;

	char str[66];
	PiecesLibrary *pLib = lcGetPiecesLibrary();
	CPiecesBar* pBar = (CPiecesBar*)GetParent();
	PieceInfo* pInfo;

	if (int n = GetWindowText(str, 65))
	{
		char newstr[66];
		int sel = -1;
		strcpy (newstr, "Z");
		for (int i = 0; i < pLib->GetPieceCount(); i++)
		{
			pInfo = pLib->GetPieceInfo(i);

			if ((pInfo->m_strDescription[0] == '~') && !pBar->m_bSubParts)
				continue;

			if (_strnicmp (str, pInfo->m_strDescription, n) == 0)
			{
				if (_stricmp (newstr, pInfo->m_strDescription) > 0)
				{
					strcpy (newstr, pInfo->m_strDescription);
					sel = i;
				}
			}
			else
				if (_strnicmp (str, pInfo->m_strName, n) == 0)
				{
					if (_stricmp (newstr, pInfo->m_strName) > 0)
					{
						strcpy (newstr, pInfo->m_strName);
						sel = i;
					}
				}
		}

		if (sel >= 0)
			SelectPiece(pLib->GetPieceInfo(sel));

		if (strlen (newstr) > 1)
		{
			SetWindowText(newstr);
			SetEditSel(n, -1);
		}
	}
}

BOOL CPiecesCombo::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message == WM_KEYDOWN)
	{
		m_bAutoComplete = TRUE;
		int nVirtKey = (int) pMsg->wParam;
		if (nVirtKey == VK_DELETE || nVirtKey == VK_BACK)
		{
			CEdit* Edit = (CEdit*)GetWindow(GW_CHILD);
			m_bAutoComplete = FALSE;
			Edit->ReplaceSel("");
		}
		else if (nVirtKey == VK_RETURN)
		{
		  PiecesLibrary* Lib = lcGetPiecesLibrary();
			CString str;

			GetWindowText(str);

			int Index = Lib->FindCategoryIndex("Search Results");

			if (Index == -1)
				Lib->AddCategory("Search Results", (const char*)str);
			else
				Lib->SetCategory(Index, "Search Results", (const char*)str);
		}
	}

	return CComboBox::PreTranslateMessage(pMsg);
}

void CPiecesCombo::OnSelchange() 
{
	char str[66];
	CPiecesBar* pBar = (CPiecesBar*)GetParent();
  PiecesLibrary *pLib = lcGetPiecesLibrary();

	if (!GetLBText (GetCurSel(), str))
		return;

	for (int i = 0; i < pLib->GetPieceCount(); i++)
	{
		PieceInfo* pInfo = pLib->GetPieceInfo(i);

		if (strcmp(str, pInfo->m_strDescription) == 0)
			SelectPiece(pInfo);
	}
}

void CPiecesCombo::SelectPiece(PieceInfo* Info)
{
  PiecesLibrary *Lib = lcGetPiecesLibrary();
	CPiecesBar* Bar = (CPiecesBar*)GetParent();

	int Index = Lib->GetFirstCategory(Info);

	if (Index != -1)
		Bar->SelectPiece(Lib->GetCategoryName(Index), Info);
}
