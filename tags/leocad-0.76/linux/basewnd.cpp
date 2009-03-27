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
  GtkWidget *window, *w, *vbox, *hbox;
  GtkAccelGroup *group;
  int mode = (flags & LC_MB_TYPEMASK), ret, loop = 1;
  guint tmp_key;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
                      GTK_SIGNAL_FUNC (dialog_delete_callback), NULL);
  //  gtk_signal_connect (GTK_OBJECT (window), "destroy",
  //                      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
  gtk_window_set_title (GTK_WINDOW (window), caption);
  gtk_container_border_width (GTK_CONTAINER (window), 10);
  gtk_object_set_data (GTK_OBJECT (window), "loop", &loop);
  gtk_object_set_data (GTK_OBJECT (window), "ret", &ret);

  if (m_pXID != NULL)
    gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (m_pXID));
  //  gtk_widget_realize (window);

  group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), group);

  vbox = gtk_vbox_new (FALSE, 10);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);
 
  w = gtk_label_new (text);
  gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 2);
  gtk_label_set_justify (GTK_LABEL (w), GTK_JUSTIFY_LEFT);
  gtk_widget_show (w);
 
  w = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 2);
  gtk_widget_show (w);
 
  hbox = gtk_hbox_new (FALSE, 10);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);
  gtk_widget_show (hbox);
 
  if (mode == LC_MB_OK)
  {
    w = gtk_button_new_with_label ("Ok");
    gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (w), "clicked",
                        GTK_SIGNAL_FUNC (dialog_button_callback), GINT_TO_POINTER (LC_OK));
    GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (w);
    gtk_widget_show (w);
    ret = LC_OK;
  }
  else if (mode == LC_MB_OKCANCEL)
  {
    w = gtk_button_new_with_label ("Ok");
    gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (w), "clicked",
                        GTK_SIGNAL_FUNC (dialog_button_callback), GINT_TO_POINTER (LC_OK));
    GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (w);
    gtk_widget_show (w);

    w = gtk_button_new_with_label ("Cancel");
    gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (w), "clicked",
                        GTK_SIGNAL_FUNC (dialog_button_callback), GINT_TO_POINTER (LC_CANCEL));
    gtk_widget_show (w);
    ret = LC_CANCEL;
  }
  else if (mode == LC_MB_YESNOCANCEL)
  {
    w = gtk_button_new_with_label ("");
    tmp_key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (w)->child), "_Yes");
    gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (w), "clicked",
                        GTK_SIGNAL_FUNC (dialog_button_callback), GINT_TO_POINTER (LC_YES));
    gtk_widget_add_accelerator (w, "clicked", group, tmp_key, (GdkModifierType)0, (GtkAccelFlags)0);
    GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (w);
    gtk_widget_show (w);
    gtk_window_set_focus (GTK_WINDOW (window), w);

    w = gtk_button_new_with_label ("");
    tmp_key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (w)->child), "_No");
    gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (w), "clicked",
                        GTK_SIGNAL_FUNC (dialog_button_callback), GINT_TO_POINTER (LC_NO));
    gtk_widget_add_accelerator (w, "clicked", group, tmp_key, (GdkModifierType)0, (GtkAccelFlags)0);
    gtk_widget_show (w);

    w = gtk_button_new_with_label ("");
    tmp_key = gtk_label_parse_uline (GTK_LABEL (GTK_BIN (w)->child), "_Cancel");
    gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (w), "clicked",
                        GTK_SIGNAL_FUNC (dialog_button_callback), GINT_TO_POINTER (LC_CANCEL));
    gtk_widget_add_accelerator (w, "clicked", group, tmp_key, (GdkModifierType)0, (GtkAccelFlags)0);
    gtk_widget_add_accelerator (w, "clicked", group, GDK_Escape, (GdkModifierType)0, (GtkAccelFlags)0);
    gtk_widget_show (w);
    ret = LC_CANCEL;
  }
  else /* if (mode == LC_MB_YESNO) */
  {
    w = gtk_button_new_with_label ("Yes");
    gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (w), "clicked",
                        GTK_SIGNAL_FUNC (dialog_button_callback), GINT_TO_POINTER (LC_YES));
    GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (w);
    gtk_widget_show (w);
 
    w = gtk_button_new_with_label ("No");
    gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (w), "clicked",
                        GTK_SIGNAL_FUNC (dialog_button_callback), GINT_TO_POINTER (LC_NO));
    gtk_widget_show (w);
    ret = LC_NO;
  }

  gtk_widget_show (window);
  gtk_grab_add (window);

  while (loop)
    gtk_main_iteration ();

  gtk_grab_remove (window);
  gtk_widget_destroy (window);

  return ret;
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

