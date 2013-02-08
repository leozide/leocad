#include "lc_global.h"
#include "leocad.h"
#include "PieceCmb.h"
#include "PieceBar.h"
#include "pieceinf.h"
#include "project.h"
#include "globals.h"
#include "lc_library.h"
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

	char str[128];
	int Length = GetWindowText(str, 128);

	if (!Length)
		return;

	lcPiecesLibrary *pLib = lcGetPiecesLibrary();
	CPiecesBar* pBar = (CPiecesBar*)GetParent();
	PieceInfo* NewPiece = NULL;
	bool DescriptionMatch = false;

	for (int i = 0; i < pLib->mPieces.GetSize(); i++)
	{
		PieceInfo* Info = pLib->mPieces[i];

		if ((Info->m_strDescription[0] == '~') && !pBar->m_bSubParts)
			continue;

		if (_strnicmp(str, Info->m_strDescription, Length) == 0)
		{
			if (!NewPiece || _stricmp(NewPiece->m_strDescription, Info->m_strDescription) > 0)
			{
				NewPiece = Info;
				DescriptionMatch = true;
			}
		}
		else if (_strnicmp(str, Info->m_strName, Length) == 0)
		{
			if (!NewPiece || _stricmp(NewPiece->m_strName, Info->m_strName) > 0)
			{
				NewPiece = Info;
				DescriptionMatch = false;
			}
		}
	}

	if (NewPiece)
	{
		SelectPiece(NewPiece);
		SetWindowText(DescriptionMatch ? NewPiece->m_strDescription : NewPiece->m_strName);
		SetEditSel(Length, -1);
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
			lcPiecesLibrary* Lib = lcGetPiecesLibrary();
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
	char str[128];
	CPiecesBar* pBar = (CPiecesBar*)GetParent();
	lcPiecesLibrary* pLib = lcGetPiecesLibrary();

	if (GetLBTextLen(GetCurSel()) >= sizeof(str))
		return;

	if (!GetLBText(GetCurSel(), str))
		return;

	for (int i = 0; i < pLib->mPieces.GetSize(); i++)
	{
		PieceInfo* pInfo = pLib->mPieces[i];

		if (strcmp(str, pInfo->m_strDescription) == 0)
			SelectPiece(pInfo);
		else if (strcmp(str, pInfo->m_strName) == 0)
			SelectPiece(pInfo);
	}
}

void CPiecesCombo::SelectPiece(PieceInfo* Info)
{
	lcPiecesLibrary* Lib = lcGetPiecesLibrary();
	CPiecesBar* Bar = (CPiecesBar*)GetParent();

	int Index = Lib->GetFirstPieceCategory(Info);

	if (Index != -1)
		Bar->SelectPiece(Lib->mCategories[Index].Name, Info);
}
