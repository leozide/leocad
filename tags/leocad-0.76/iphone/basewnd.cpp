//
// BaseWnd class implementation for iPhone
//

#include "basewnd.h"

BaseWnd::BaseWnd (BaseWnd *parent, int menu_count)
{
  m_pMenuItems = new BaseMenuItem[menu_count];
  memset(m_pMenuItems, 0, sizeof(BaseMenuItem[menu_count]));
  m_pParent = parent;

  m_pXID = NULL;
}

BaseWnd::~BaseWnd ()
{
  delete [] m_pMenuItems;
  m_pMenuItems = NULL;
}

void BaseWnd::BeginWait ()
{
}

void BaseWnd::EndWait ()
{
}

int BaseWnd::MessageBox (const char* text, const char* caption, int flags)
{
	int mode = (flags & LC_MB_TYPEMASK);
	int ret;

	if (mode == LC_MB_OK)
		ret = LC_OK;
	else if (mode == LC_MB_OKCANCEL)
		ret = LC_CANCEL;	
	else /* if (mode == LC_MB_YESNO) */
		ret = LC_NO;

	return ret;
}

void BaseWnd::ShowMenuItem (int id, bool show)
{
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
