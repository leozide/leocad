//
// This file holds all the dialogs that are called
// from the 'Pieces' submenu:
//
// - Group Name
// - Edit Groups
// - Minifig Wizard
//

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "opengl.h"
#include "gtkglarea.h"
#include "gtktools.h"
#include "system.h"
#include "typedefs.h"
#include "globals.h"
#include "dialogs.h"
#include "matrix.h"
#include "pieceinf.h"
#include "main.h"
#include "minifig.h"

// =========================================================
// Minifig Wizard

typedef struct
{
  MinifigWizard* opts;
  GtkWidget *pieces[LC_MFW_NUMITEMS];
  GtkWidget *colors[LC_MFW_NUMITEMS];
  GtkWidget *angles[LC_MFW_NUMITEMS];
  GtkWidget *preview;
} LC_MINIFIGDLG_STRUCT;

static gint minifigdlg_redraw (GtkWidget *widget, GdkEventExpose *event)
{
  LC_MINIFIGDLG_STRUCT* data;

  // Draw only last expose.
  if (event->count > 0)
    return TRUE;

  data = (LC_MINIFIGDLG_STRUCT*)gtk_object_get_data (GTK_OBJECT (widget), "minifig");

  if (!data)
    return true;

  if (!gtk_gl_area_make_current(GTK_GL_AREA(widget)))
    return TRUE;

  data->opts->Redraw ();

  gtk_gl_area_swapbuffers(GTK_GL_AREA(widget));
  gtk_gl_area_make_current(GTK_GL_AREA(drawing_area));                          

  return TRUE;
}

// Setup the OpenGL projection
static gint minifigdlg_resize (GtkWidget *widget, GdkEventConfigure *event)
{
  LC_MINIFIGDLG_STRUCT* data = (LC_MINIFIGDLG_STRUCT*)gtk_object_get_data (GTK_OBJECT (widget), "minifig");

  if (!data)
    return TRUE;

  if (!gtk_gl_area_make_current(GTK_GL_AREA(widget)))
    return TRUE;

  data->opts->Resize (widget->allocation.width, widget->allocation.height);

  gtk_gl_area_make_current(GTK_GL_AREA(drawing_area));                          

  return TRUE;
}

// User wants to add the minifig to the project
static void minifigdlg_ok(GtkWidget *widget, gpointer data)
{
  //  LC_MINIFIGDLG_STRUCT* s = (LC_MINIFIGDLG_STRUCT*)data;
  //  LC_MINIFIGDLG_OPTS* opts = (LC_MINIFIGDLG_OPTS*)s->data;

  dlg_end (LC_OK);
}

// A new color was selected from the menu
static void minifigdlg_color_response (GtkWidget *widget, gpointer data)
{
  LC_MINIFIGDLG_STRUCT* info;
  GtkWidget* button;
  int i;

  button = (GtkWidget*)gtk_object_get_data (GTK_OBJECT (widget), "button");
  info = (LC_MINIFIGDLG_STRUCT*)gtk_object_get_data (GTK_OBJECT (button), "info");

  if (!info)
    return;

  for (i = 0; i < 15; i++)
    if (info->colors[i] == button)
      break;

  info->opts->ChangeColor (i, GPOINTER_TO_INT (data));
  gtk_widget_draw (info->preview, NULL);
  set_button_pixmap2 (button, FlatColorArray[(int)data]);
}

// A color button was clicked
static void minifigdlg_color_clicked (GtkWidget *widget, gpointer data)
{
  int i;
  GtkWidget *menu, *menuitem;

  menu = gtk_menu_new ();

  for (i = 0; i < LC_MAXCOLORS; i++)
  {
    menuitem = gtk_menu_item_new_with_label (colornames[i]);
    gtk_widget_show (menuitem);
    gtk_menu_append (GTK_MENU (menu), menuitem);

    gtk_object_set_data (GTK_OBJECT (menuitem), "button", widget);
    gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			GTK_SIGNAL_FUNC (minifigdlg_color_response), (void*)i);
  }

  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, 0);
}

// A color button was exposed, draw the pixmap
static gint minifigdlg_color_expose (GtkWidget *widget)
{
  int* data = (int*)gtk_object_get_data (GTK_OBJECT (widget), "color");
  set_button_pixmap2 (widget, FlatColorArray[*data]);
  return TRUE;
}

// New piece was selected
static void minifigdlg_piece_changed (GtkWidget *widget, gpointer data)
{
  LC_MINIFIGDLG_STRUCT* info;
  int i, piece_type;
  char* desc;

  info = (LC_MINIFIGDLG_STRUCT*)gtk_object_get_data (GTK_OBJECT (widget), "info");
  if (info == NULL)
    return;

  for (i = 0; i < LC_MFW_NUMITEMS; i++)
    if (GTK_COMBO (info->pieces[i])->entry == widget)
    {
      piece_type = i;
      break;
    }

  desc = gtk_entry_get_text (GTK_ENTRY (widget));

  info->opts->ChangePiece (i, desc);

  gtk_widget_draw (info->preview, NULL);
}

static void adj_changed (GtkAdjustment *adj, gpointer data)
{
  LC_MINIFIGDLG_STRUCT* info = (LC_MINIFIGDLG_STRUCT*)data;
  int i;

  for (i = 0; i < LC_MFW_NUMITEMS; i++)
    if (info->angles[i] != NULL)
      if (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (info->angles[i])) == adj)
	break;

  if (i == LC_MFW_NUMITEMS)
    return;

  info->opts->ChangeAngle (i, gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (info->angles[i])));

  if (info->preview != NULL)
    gtk_widget_draw (info->preview, NULL);
}

// Create a combo box with a color selection control
static void minifigdlg_createpair (LC_MINIFIGDLG_STRUCT* info, int idx, int num, GtkWidget* table)
{
  GtkWidget *combo, *color, *spin;
  GtkObject *adj;

  combo = info->pieces[num] = gtk_combo_new ();
  gtk_widget_show (combo);
  gtk_table_attach (GTK_TABLE (table), combo, 0, 1, idx, idx+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_widget_set_usize (combo, 60, 25);
  gtk_entry_set_editable (GTK_ENTRY (GTK_COMBO (combo)->entry), FALSE);
  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (combo)->entry), "changed",
                      GTK_SIGNAL_FUNC (minifigdlg_piece_changed), NULL);
  gtk_object_set_data (GTK_OBJECT (GTK_COMBO (combo)->entry), "info", info);

  color = info->colors[num] = gtk_button_new_with_label ("");
  gtk_widget_set_events (color, GDK_EXPOSURE_MASK);
  gtk_widget_show (color);
  gtk_object_set_data (GTK_OBJECT (color), "color", &info->opts->m_Colors[num]);
  gtk_object_set_data (GTK_OBJECT (color), "info", info);
  gtk_widget_set_usize (color, 40, 25);
  gtk_signal_connect (GTK_OBJECT (color), "expose_event",
		      GTK_SIGNAL_FUNC (minifigdlg_color_expose), NULL);
  gtk_signal_connect (GTK_OBJECT (color), "clicked",
		      GTK_SIGNAL_FUNC (minifigdlg_color_clicked), info);
  gtk_table_attach (GTK_TABLE (table), color, 1, 2, idx, idx+1,
                    (GtkAttachOptions) GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);

  if ((num == LC_MFW_TORSO) || (num == LC_MFW_HIPS))
    return;

  adj = gtk_adjustment_new (0, -180, 180, 1, 10, 10);
  gtk_signal_connect (adj, "value_changed", GTK_SIGNAL_FUNC (adj_changed), info);

  spin = info->angles[num] = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 1, 0);
  gtk_widget_show (spin);
  gtk_object_set_data (GTK_OBJECT (color), "info", info);
  //  gtk_widget_set_usize (spin, 40, -1);
  gtk_table_attach (GTK_TABLE (table), spin, 2, 3, idx, idx+1,
                    (GtkAttachOptions) GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spin), TRUE);
}

int minifigdlg_execute(void* param)
{
  int attrlist[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 16, 0 };
  LC_MINIFIGDLG_STRUCT s;
  GtkWidget *dlg;
  GtkWidget *vbox, *hbox, *frame, *table;
  GtkWidget *button;
  int i;

  memset (&s, 0, sizeof (s));
  s.opts = (MinifigWizard*)param;

  dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (main_window));
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
		      GTK_SIGNAL_FUNC (dlg_delete_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
  gtk_widget_set_usize (dlg, 600, 360);
  gtk_window_set_title (GTK_WINDOW (dlg), "Minifig Wizard");
  //  gtk_window_set_policy (GTK_WINDOW (dlg), FALSE, FALSE, FALSE);
  gtk_widget_realize (dlg);

  vbox = gtk_vbox_new (FALSE, 10);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (dlg), vbox);
  gtk_container_border_width (GTK_CONTAINER (vbox), 5);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  table = gtk_table_new (8, 3, FALSE);
  gtk_widget_show (table);
  gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 5);

  minifigdlg_createpair (&s, 0, LC_MFW_HAT, table);
  minifigdlg_createpair (&s, 1, LC_MFW_NECK, table);
  minifigdlg_createpair (&s, 2, LC_MFW_RIGHT_ARM, table);
  minifigdlg_createpair (&s, 3, LC_MFW_RIGHT_HAND, table);
  minifigdlg_createpair (&s, 4, LC_MFW_RIGHT_TOOL, table);
  minifigdlg_createpair (&s, 5, LC_MFW_HIPS, table);
  minifigdlg_createpair (&s, 6, LC_MFW_RIGHT_LEG, table);
  minifigdlg_createpair (&s, 7, LC_MFW_RIGHT_SHOE, table);

  // Create new OpenGL widget.
  s.preview = gtk_gl_area_share_new (attrlist, GTK_GL_AREA (drawing_area));
  gtk_widget_set_events (GTK_WIDGET (s.preview), GDK_EXPOSURE_MASK);

  gtk_signal_connect (GTK_OBJECT (s.preview), "expose_event", 
      GTK_SIGNAL_FUNC (minifigdlg_redraw), NULL);
  gtk_signal_connect (GTK_OBJECT (s.preview), "configure_event",
      GTK_SIGNAL_FUNC (minifigdlg_resize), NULL);

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_container_add (GTK_CONTAINER (hbox), frame);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

  gtk_widget_set_usize (GTK_WIDGET (s.preview), 150, 300);
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (s.preview));
  gtk_widget_show (GTK_WIDGET (s.preview));
  gtk_object_set_data (GTK_OBJECT (s.preview), "minifig", &s);

  table = gtk_table_new (7, 3, FALSE);
  gtk_widget_show (table);
  gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 5);

  minifigdlg_createpair (&s, 0, LC_MFW_HEAD, table);
  minifigdlg_createpair (&s, 1, LC_MFW_TORSO, table);
  minifigdlg_createpair (&s, 2, LC_MFW_LEFT_ARM, table);
  minifigdlg_createpair (&s, 3, LC_MFW_LEFT_HAND, table);
  minifigdlg_createpair (&s, 4, LC_MFW_LEFT_TOOL, table);
  minifigdlg_createpair (&s, 5, LC_MFW_LEFT_LEG, table);
  minifigdlg_createpair (&s, 6, LC_MFW_LEFT_SHOE, table);

  hbox = gtk_hbox_new (FALSE, 10);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);

  button = gtk_button_new_with_label ("OK");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (minifigdlg_ok), &s);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, TRUE, 0);
  gtk_widget_set_usize (button, 70, 25);
  GtkAccelGroup *accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (dlg), accel_group);
  gtk_widget_add_accelerator (button, "clicked", accel_group,
                              GDK_Return, 0, GTK_ACCEL_VISIBLE);

  button = gtk_button_new_with_label ("Cancel");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (dlg_default_callback), GINT_TO_POINTER (LC_CANCEL));
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, TRUE, 0);
  gtk_widget_set_usize (button, 70, 25);
  gtk_widget_add_accelerator (button, "clicked", accel_group,
			      GDK_Escape, 0, GTK_ACCEL_VISIBLE);

  // Fill the combo boxes with the available pieces
  for (i = 0; i < LC_MFW_NUMITEMS; i++)
  {
    GList* names = NULL;
    int count;
    char **list;
    s.opts->GetDescriptions (i, &list, &count);

    for (int j = 0; j < count; j++)
      names = g_list_append (names, list[j]);

    if (names != NULL)
    {
      gtk_combo_set_popdown_strings (GTK_COMBO (s.pieces[i]), names);
      g_list_free (names);
    }
    free (list);
  }

  gtk_list_select_item (GTK_LIST (GTK_COMBO (s.pieces[LC_MFW_HAT])->list), 7);
  gtk_list_select_item (GTK_LIST (GTK_COMBO (s.pieces[LC_MFW_HEAD])->list), 4);
  gtk_list_select_item (GTK_LIST (GTK_COMBO (s.pieces[LC_MFW_TORSO])->list), 22);

  return dlg_domodal(dlg, LC_CANCEL);
}
