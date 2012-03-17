//
// BaseWnd class implementation for Linux
//

#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include "basewnd.h"
#include "dialogs.h"
#include "main.h"

BaseWnd::BaseWnd (BaseWnd *parent, int menu_count)
{
  m_pMenuItems = new BaseMenuItem[menu_count];
  memset(m_pMenuItems, 0, sizeof(BaseMenuItem[menu_count]));
  m_pParent = parent;

  m_pXID = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  if (parent != NULL)
    gtk_window_set_transient_for (GTK_WINDOW (m_pXID), GTK_WINDOW (parent->GetXID ()));
}

BaseWnd::~BaseWnd ()
{
  delete [] m_pMenuItems;
  m_pMenuItems = NULL;
}

void BaseWnd::BeginWait ()
{
  GdkCursor *cursor = gdk_cursor_new (GDK_WATCH);
  gdk_window_set_cursor (m_pXID->window, cursor);
  gdk_cursor_destroy (cursor);
}

void BaseWnd::EndWait ()
{
  gdk_window_set_cursor (m_pXID->window, NULL);
}

int BaseWnd::MessageBox (const char* text, const char* caption, int flags)
{
  return msgbox_execute (text, caption, flags);
}

void BaseWnd::ShowMenuItem (int id, bool show)
{
  if (!m_pMenuItems[id].widget)
    return;

  if (show)
    gtk_widget_show (m_pMenuItems[id].widget);
  else
    gtk_widget_hide (m_pMenuItems[id].widget);
}

void BaseWnd::EnableMenuItem (int id, bool enable)
{
  if (!m_pMenuItems[id].widget)
    return;

  gtk_widget_set_sensitive (m_pMenuItems[id].widget, enable);
}

void BaseWnd::CheckMenuItem (int id, bool check)
{
  if (!m_pMenuItems[id].widget)
    return;

  ignore_commands = true;
  gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM (m_pMenuItems[id].widget), check);  
  ignore_commands = false;
}
#include <stdio.h>

void BaseWnd::SetMenuItemText (int id, const char *text)
{
  gboolean underscore;
  gchar *r;
  const char *p;
  gchar *pattern;
  gint length;

  if (!m_pMenuItems[id].widget)
    return;

  length = strlen (text);
  pattern = g_new (gchar, length+1);
  
  underscore = FALSE;
  
  p = text;
  r = pattern;

  while (*p)
  {
    if (underscore)
    {
      if (*p == '&')
        *r++ = *p;
      else
      {
        *r++ = '_';
	*r++ = *p;
      }

      underscore = FALSE;
    }
    else
    {
      if (*p == '&')
        underscore = TRUE;
      else
	*r++ = *p;
    }
    p++;
  }
  *r = 0;

  gtk_label_set_text_with_mnemonic(GTK_LABEL(GTK_BIN(m_pMenuItems[id].widget)->child), pattern);
  g_free (pattern);
}

