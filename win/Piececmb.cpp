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
  PiecesLibrary *pLib = project->GetPiecesLibrary ();
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
		{
			pInfo = pLib->GetPieceInfo(sel);

			if ((pBar->m_bGroups) && (pInfo->m_nGroups != 0))
				if ((pInfo->m_nGroups & (1 << pBar->m_nCurGroup)) == 0)
				{
					DWORD d = 1;
					for (int k = 1; k < 32; k++)
					{
						if ((pInfo->m_nGroups & d) != 0)
						{
							pBar->m_nCurGroup = k-1;
							pBar->m_wndPiecesList.UpdateList();
							k = 32;
						}
						else
							d *= 2;
					}
				}

			LV_FINDINFO lvfi;
			lvfi.flags = LVFI_PARAM;
			lvfi.lParam = (LPARAM)pInfo;

			sel = pBar->m_wndPiecesList.FindItem (&lvfi);
			pBar->m_wndPiecesList.SetItemState (sel, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
			pBar->m_wndPiecesList.EnsureVisible (sel, FALSE);
		}
		if (strlen (newstr) > 1)
		{
			SetWindowText (newstr);
			SetEditSel (n, -1);
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
// if enter pressed, add piece (postmessage to mainwnd)
			m_bAutoComplete = FALSE;
		}
	}

	return CComboBox::PreTranslateMessage(pMsg);
}

void CPiecesCombo::OnSelchange() 
{
	char str[66];
	CPiecesBar* pBar = (CPiecesBar*)GetParent();
  PiecesLibrary *pLib = project->GetPiecesLibrary ();

	if (!GetLBText (GetCurSel(), str))
		return;

	for (int i = 0; i < pLib->GetPieceCount(); i++)
	{
		PieceInfo* pInfo = pLib->GetPieceInfo(i);

		if (strcmp (str, pInfo->m_strDescription) == 0)
		{
			if ((pBar->m_bGroups) && (pInfo->m_nGroups != 0))
				if ((pInfo->m_nGroups & (1 << pBar->m_nCurGroup)) == 0)
				{
					DWORD d = 1;
					for (int k = 1; k < 32; k++)
					{
						if ((pInfo->m_nGroups & d) != 0)
						{
							pBar->m_nCurGroup = k-1;
							pBar->m_wndPiecesList.UpdateList();
							k = 32;
						}
						else
							d *= 2;
					}
				}

			LV_FINDINFO lvfi;
			lvfi.flags = LVFI_PARAM;
			lvfi.lParam = (LPARAM)pInfo;

			i = pBar->m_wndPiecesList.FindItem (&lvfi);
			pBar->m_wndPiecesList.SetItemState(i,LVIS_SELECTED | LVIS_FOCUSED , LVIS_SELECTED | LVIS_FOCUSED);
			pBar->m_wndPiecesList.EnsureVisible (i, FALSE);
			return;
		}
	}
}
