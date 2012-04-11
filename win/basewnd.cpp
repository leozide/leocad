//
// Base window class
//

#include "stdafx.h"
#include "basewnd.h"

BaseWnd::BaseWnd (BaseWnd *parent, int menu_count)
{
  m_pMenuItems = new BaseMenuItem[menu_count];
  m_pParent = parent;
}

BaseWnd::~BaseWnd ()
{
  delete [] m_pMenuItems;
}

static HCURSOR g_hcurWaitCursorRestore; // old cursor to restore after wait cursor

void BaseWnd::BeginWait ()
{
  HCURSOR hcurPrev = SetCursor (LoadCursor (NULL, IDC_WAIT));
  g_hcurWaitCursorRestore = hcurPrev;
}

void BaseWnd::EndWait ()
{
  SetCursor (g_hcurWaitCursorRestore);
}

int BaseWnd::MessageBox (const char* text, const char* caption, int flags)
{
  return m_pXID->MessageBox (text, caption, flags);
}

void BaseWnd::ShowMenuItem (int id, bool show)
{
  /*
	CBMPMenu* pMenu = (CBMPMenu*)GetMainMenu(0);
	CMenu* pMenu = pFrame->GetMenu();
	return pMenu->GetSubMenu(nIndex);
  */
}

void BaseWnd::EnableMenuItem (int id, bool enable)
{
}

void BaseWnd::CheckMenuItem (int id, bool check)
{
}

void BaseWnd::SetMenuItemText (int id, const char *text)
{
}
