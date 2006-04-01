// keyedit.cpp : implementation file
//

#include "stdafx.h"
#include "leocad.h"
#include "keyedit.h"
#include "keyboard.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CKeyEdit

CKeyEdit::CKeyEdit()
{
	m_Key = 0;
	m_Control = false;
	m_Shift = false;
}

CKeyEdit::~CKeyEdit()
{
}


BEGIN_MESSAGE_MAP(CKeyEdit, CEdit)
	//{{AFX_MSG_MAP(CKeyEdit)
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CKeyEdit message handlers

void CKeyEdit::ResetKey()
{
	m_Key = 0;
	m_Control = false;
	m_Shift = false;

	SetWindowText("");
}

void CKeyEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	
	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CKeyEdit::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{

	CEdit::OnKeyUp(nChar, nRepCnt, nFlags);
}

BOOL CKeyEdit::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message == WM_KEYDOWN)
	{
		// If the keys are for the dialog box or Windows, pass them.
		if ((pMsg->wParam == VK_TAB) || (pMsg->wParam == VK_ESCAPE))
		{
//			DoErasingStuff (hHotKeyEdit);
//			return CEdit::PreTranslateMessage(pMsg);
		}
/*
		else if (pMsg->wParam == VK_BACK && !Control && !Shift)
		{
			// If backspace, then erase the edit control and disable the Assign button.
//			DoErasingStuff (hHotKeyEdit);
//			EnableWindow (GetDlgItem (GetParent (hHotKeyEdit), IDD_INSTALL), FALSE);
			SetWindowText("");

			return true;
		}
*/
		else
		{
			CString Text;

			m_Control = (GetKeyState(VK_CONTROL) < 0);
			m_Shift = (GetKeyState(VK_SHIFT) < 0);

			if (m_Control)
				Text += "Ctrl+";

			if (m_Shift)
				Text += "Shift+";

			const char* KeyName = GetKeyName(pMsg->wParam);

			if (KeyName)
			{
				Text += KeyName;
				m_Key = pMsg->wParam;
			}
			else
			{
				m_Key = 0;
			}

			SetWindowText(Text);

			return true;
		}
	}

	return CEdit::PreTranslateMessage(pMsg);
}
