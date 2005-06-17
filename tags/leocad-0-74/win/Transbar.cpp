// TransToolBar.cpp : implementation file
//

#include "stdafx.h"
#include "TransBar.h"
#include "BarCmdUI.h"
#include "leocad.h"
#include "piecebar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTransToolBar

CTransToolBar::CTransToolBar()
{
}

CTransToolBar::~CTransToolBar()
{
}


BEGIN_MESSAGE_MAP(CTransToolBar, CControlBar)
	//{{AFX_MSG_MAP(CTransToolBar)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI_RANGE(ID_PIECE_GROUP01, ID_PIECE_GROUP32, OnUpdatePieceGroup)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTransToolBar message handlers

// Doesn't work if handled by the parent. Why ?
void CTransToolBar::OnUpdatePieceGroup(CCmdUI* pCmdUI)
{
	CPiecesBar* pBar = (CPiecesBar*)GetParent();
	pCmdUI->SetCheck((UINT)(ID_PIECE_GROUP01 + pBar->m_nCurGroup) == pCmdUI->m_nID);
}

/////
//
// CreateTransparentToolbar Function
//     Creates a transparent toolbar control based upon the owner's
//     window handle an incoming style bit field that is assumed
//     to be a combination of TBSTYLE_FLAT and TBSTYLE_LIST flags.
//
// Accepts:
//    HWND: The handle to the parent window.
//    DWORD:  Style values that are included in CreateWindowEx.
//
// Returns:
//    HWND to the newly created toolbar. The owner must resize it.
//
/////
BOOL CTransToolBar::Create(HWND hwndOwner)
{
	TBBUTTON	tbArray[MAX_BUTTONS];
	int			i, iBufferPix;

	// Create the toolbar control.
	if(!CWnd::CreateEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL,
							WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | 
							WS_CLIPSIBLINGS | CCS_NODIVIDER |
							CCS_NOPARENTALIGN | CCS_NORESIZE | m_ToolbarData.dwStyle, 
							0,0,0,0,  // Make it zero, Let owner resize it.
							hwndOwner, (HMENU)m_ToolbarData.idControl,
							NULL)) return FALSE; 
   
	SendMessage(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	SendMessage(TB_SETMAXTEXTROWS, 1, 0L);
	SendMessage(TB_SETBITMAPSIZE, 0, (LPARAM)MAKELONG(m_ToolbarData.iButtonCX, m_ToolbarData.iButtonCY));
	SendMessage(TB_SETIMAGELIST, 0, (LPARAM)m_ToolbarData.himl);

	// Loop to fill the array of TBBUTTON structures.
	for(i=0; i < m_ToolbarData.iButtons; i++)
	{
		tbArray[i].iBitmap   = m_ToolbarData.ButtonData[i].iBitmap;
		tbArray[i].idCommand = m_ToolbarData.ButtonData[i].idCommand;
		tbArray[i].fsState   = m_ToolbarData.ButtonData[i].fsState;
		tbArray[i].fsStyle   = m_ToolbarData.ButtonData[i].fsStyle;
		tbArray[i].dwData    = 0;
		tbArray[i].iString   = i;
	}

	// If this is a list style toolbar, add buffer pixels 
	// to make room for button text.
	iBufferPix=LARGEBUTTON_DX;

	// Add the buttons, then set the minimum and maximum button widths.
	SendMessage(TB_ADDBUTTONS, (UINT)m_ToolbarData.iButtons, (LPARAM)tbArray);

	SendMessage(TB_SETBUTTONWIDTH, 0, 
               (LPARAM)MAKELONG(m_ToolbarData.iButtonCX + iBufferPix,
               m_ToolbarData.iButtonCY + iBufferPix));

	return TRUE;

}

void CTransToolBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	CFrameWnd* pMain = (CFrameWnd*)AfxGetMainWnd();

	CFlatOrCoolBarCmdUI state;
	state.m_pOther = this;

	state.m_nIndexMax = (UINT)DefWindowProc(TB_BUTTONCOUNT, 0, 0);
	for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax; state.m_nIndex++)
	{
		// get buttons state
		TBBUTTON button;
		_GetButton(state.m_nIndex, &button);
		state.m_nID = button.idCommand;

		// ignore separators
		if (!(button.fsStyle & TBSTYLE_SEP))
		{
			// allow the toolbar itself to have update handlers
			if (CWnd::OnCmdMsg(state.m_nID, CN_UPDATE_COMMAND_UI, &state, NULL))
				continue;

			// allow the owner to process the update
			state.DoUpdate(pMain, bDisableIfNoHndler);
		}
	}
	// update the dialog controls added to the toolbar
	UpdateDialogControls(pMain, bDisableIfNoHndler);
}

void CTransToolBar::_SetButton(int nIndex, TBBUTTON* pButton)
{
	// get original button state
	TBBUTTON button;
	VERIFY(DefWindowProc(TB_GETBUTTON, nIndex, (LPARAM)&button));

	// prepare for old/new button comparsion
	button.bReserved[0] = 0;
	button.bReserved[1] = 0;
	pButton->fsState ^= TBSTATE_ENABLED;
	pButton->bReserved[0] = 0;
	pButton->bReserved[1] = 0;

	// nothing to do if they are the same
	if (memcmp(pButton, &button, sizeof(TBBUTTON)) != 0)
	{
		// don't redraw everything while setting the button
		DWORD dwStyle = GetStyle();
		ModifyStyle(WS_VISIBLE, 0);
		VERIFY(DefWindowProc(TB_DELETEBUTTON, nIndex, 0));
		VERIFY(DefWindowProc(TB_INSERTBUTTON, nIndex, (LPARAM)pButton));
		ModifyStyle(0, dwStyle & WS_VISIBLE);

		// invalidate appropriate parts
		if (((pButton->fsStyle ^ button.fsStyle) & TBSTYLE_SEP) ||
			((pButton->fsStyle & TBSTYLE_SEP) && pButton->iBitmap != button.iBitmap))
		{
			// changing a separator
			Invalidate(FALSE);
		}
		else
		{
			// invalidate just the button
			CRect rect;
			if (DefWindowProc(TB_GETITEMRECT, nIndex, (LPARAM)&rect))
				InvalidateRect(rect, TRUE);    // erase background
		}
		DefWindowProc(WM_PAINT, (WPARAM)::GetDC(GetSafeHwnd()), (LPARAM)0);
	}
}

void CTransToolBar::_GetButton(int nIndex, TBBUTTON* pButton) const
{
	CTransToolBar* pBar = (CTransToolBar*)this;
	VERIFY(pBar->DefWindowProc(TB_GETBUTTON, nIndex, (LPARAM)pButton));
	pButton->fsState ^= TBSTATE_ENABLED;
}

void CTransToolBar::SetButtonStyle(int nIndex, UINT nStyle)
{
	ASSERT_VALID(this);
	ASSERT(::IsWindow(m_hWnd));

	TBBUTTON button;
	_GetButton(nIndex, &button);
	button.fsStyle = (BYTE)LOWORD(nStyle);
	button.fsState = (BYTE)HIWORD(nStyle);
	_SetButton(nIndex, &button);

	m_bDelayedButtonLayout = TRUE;
}
