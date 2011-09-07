#include <stdlib.h>
#include "basewnd.h"

BaseWnd::BaseWnd (BaseWnd *parent, int menu_count)
{
  m_pParent = parent;
}

BaseWnd::~BaseWnd ()
{
}

void BaseWnd::BeginWait ()
{
}

void BaseWnd::EndWait ()
{
}

int BaseWnd::MessageBox (const char* text, const char* caption, int flags)
{
  return 0;
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

