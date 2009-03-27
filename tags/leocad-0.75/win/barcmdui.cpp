////////////////////////////////////////////////////////////////
// The following stuff is to make the command update UI mechanism
// work properly for flat tool bars. The main idea is to convert
// a "checked" button state into a "pressed" button state. Changed 
// lines marked with "PD"

#include "StdAfx.h"
#include "BarCmdUI.h"

void CFlatOrCoolBarCmdUI::Enable(BOOL bOn)
{
	m_bEnableChanged = TRUE;
	CToolBar* pToolBar = (CToolBar*)m_pOther;
	ASSERT(pToolBar != NULL);
	ASSERT_KINDOF(CToolBar, pToolBar);
	ASSERT(m_nIndex < m_nIndexMax);

	UINT nNewStyle = pToolBar->GetButtonStyle(m_nIndex) & ~TBBS_DISABLED;
	if (!bOn)
	{
		nNewStyle |= TBBS_DISABLED;
		// WINBUG: If a button is currently pressed and then is disabled
		// COMCTL32.DLL does not unpress the button, even after the mouse
		// button goes up!  We work around this bug by forcing TBBS_PRESSED
		// off when a button is disabled.
		nNewStyle &= ~TBBS_PRESSED;
	}
	ASSERT(!(nNewStyle & TBBS_SEPARATOR));
	pToolBar->SetButtonStyle(m_nIndex, nNewStyle);
}

// Take your pick:
#define MYTBBS_CHECKED TBBS_CHECKED		// use "checked" state
//#define MYTBBS_CHECKED TBBS_PRESSED	// use pressed state

//////////////////
// This is the only function that has changed: instead of TBBS_CHECKED,
// I use TBBS_PRESSED--PD
//
void CFlatOrCoolBarCmdUI::SetCheck(int nCheck)
{
	ASSERT(nCheck >= 0 && nCheck <= 2); // 0=>off, 1=>on, 2=>indeterminate
	CToolBar* pToolBar = (CToolBar*)m_pOther;
	ASSERT(pToolBar != NULL);
	ASSERT_KINDOF(CToolBar, pToolBar);
	ASSERT(m_nIndex < m_nIndexMax);


	UINT nOldStyle = pToolBar->GetButtonStyle(m_nIndex); // PD
	UINT nNewStyle = nOldStyle &
				~(MYTBBS_CHECKED | TBBS_PRESSED |TBBS_INDETERMINATE); // PD

	// fix check & hot bug
	if ((pToolBar->SendMessage (TB_GETHOTITEM,0,0) == (int)m_nIndex)
		&& (nCheck == 1))
		nNewStyle |= TBBS_PRESSED;
	else
	{
	if (nCheck == 1)
		nNewStyle |= MYTBBS_CHECKED; // PD
	else if (nCheck == 2)
		nNewStyle |= TBBS_INDETERMINATE;
	}

	// Following is to fix display bug for TBBS_CHECKED:
	// If new state is unchecked, repaint--but only if style actually changing.
	// (Otherwise will end up with flicker)
	// 
	if (nNewStyle != nOldStyle) {
		ASSERT(!(nNewStyle & TBBS_SEPARATOR));
		pToolBar->SetButtonStyle(m_nIndex, nNewStyle);
		pToolBar->Invalidate();
	}
}

void CFlatOrCoolBarCmdUI::SetText(LPCTSTR)
{
	// ignore for now, but you should really set the text
}
