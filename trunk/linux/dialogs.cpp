// Linux Dialogs
//

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <GL/gl.h>
#include "gtktools.h"
#include "system.h"
#include "dialogs.h"
#include "typedefs.h"
#include "globals.h"
#include "piece.h"
#include "group.h"

static int def_ret = 0;
static int* cur_ret = NULL;

void dlg_end (int ret)
{
  *cur_ret = ret;
}

int dlg_domodal(GtkWidget* dlg, int def)
{
  int ret = -1, old_def = def_ret, *old_ret = cur_ret;
  def_ret = def;
  cur_ret = &ret;

  gtk_widget_show(dlg);
  gtk_grab_add(dlg);
  while (ret == -1)
    gtk_main_iteration ();
  gtk_grab_remove(dlg);
  gtk_widget_destroy(dlg);

  cur_ret = old_ret;
  def_ret = old_def;
  return ret;
}

void dlg_default_callback(GtkWidget *widget, gpointer data)
{
  *cur_ret = (int)data;
}

gint dlg_delete_callback(GtkWidget *widget, GdkEvent* event, gpointer data)
{
  *cur_ret = def_ret;
  return TRUE;
}

static bool read_float(GtkWidget* widget, float* value, float min_value, float max_value)
{
  char buf[256];

  strcpy (buf, gtk_entry_get_text (GTK_ENTRY (widget)));
  if (sscanf(buf, "%f", value) == 1)
  {
    if (*value >= min_value && *value <= max_value)
      return true;
  }

  sprintf (buf, "Please enter a value between %g and %g", min_value, max_value);
  msgbox_execute (buf, LC_MB_OK | LC_MB_ICONERROR);
  gtk_window_set_focus(GTK_WINDOW(gtk_widget_get_toplevel(widget)), widget);

  return false;
}

static void write_float(GtkWidget* widget, float value)
{
  char buf[16];
  sprintf (buf, "%g", value);
  gtk_entry_set_text (GTK_ENTRY (widget), buf);
}

static bool read_int(GtkWidget* widget, int* value, int min_value, int max_value)
{
  char buf[256];

  strcpy (buf, gtk_entry_get_text (GTK_ENTRY (widget)));
  if (sscanf(buf, "%d", value) == 1)
  {
    if (*value >= min_value && *value <= max_value)
      return true;
  }

  sprintf (buf, "Please enter a value between %d and %d", min_value, max_value);
  msgbox_execute (buf, LC_MB_OK | LC_MB_ICONERROR);
  gtk_window_set_focus(GTK_WINDOW(gtk_widget_get_toplevel(widget)), widget);

  return false;
}

static void write_int(GtkWidget* widget, int value)
{
  char buf[16];
  sprintf (buf, "%d", value);
  gtk_entry_set_text (GTK_ENTRY (widget), buf);
}

// Message box

int msgbox_execute(char* text, int flags)
{
  GtkWidget *window, *w, *vbox, *hbox;
  int mode = flags & LC_MB_TYPEMASK, ret;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (dlg_delete_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
  gtk_window_set_title (GTK_WINDOW (window), "LeoCAD");
  gtk_container_border_width (GTK_CONTAINER (window), 10);
  gtk_widget_realize (window);

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
			GTK_SIGNAL_FUNC (dlg_default_callback), GINT_TO_POINTER (LC_OK));
    GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (w);
    gtk_widget_show (w);
    ret = LC_OK;
  }
  else if (mode ==  LC_MB_OKCANCEL)
  {
    w = gtk_button_new_with_label ("Ok");
    gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (w), "clicked",
			GTK_SIGNAL_FUNC (dlg_default_callback), GINT_TO_POINTER (LC_OK));
    GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (w);
    gtk_widget_show (w);

    w = gtk_button_new_with_label ("Cancel");
    gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (w), "clicked",
			GTK_SIGNAL_FUNC (dlg_default_callback), GINT_TO_POINTER (LC_CANCEL));
    gtk_widget_show (w); 
    ret = LC_CANCEL;
  }
  else if (mode == LC_MB_YESNOCANCEL)
  {
    w = gtk_button_new_with_label ("Yes");
    gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (w), "clicked",
			GTK_SIGNAL_FUNC (dlg_default_callback), GINT_TO_POINTER (LC_YES));
    GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (w);
    gtk_widget_show (w);

    w = gtk_button_new_with_label ("No");
    gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (w), "clicked",
			GTK_SIGNAL_FUNC (dlg_default_callback), GINT_TO_POINTER (LC_NO));
    gtk_widget_show (w);

    w = gtk_button_new_with_label ("Cancel");
    gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (w), "clicked",
			GTK_SIGNAL_FUNC (dlg_default_callback), GINT_TO_POINTER (LC_CANCEL));
    gtk_widget_show (w); 
    ret = LC_CANCEL;
  }
  else /* if (mode == LC_MB_YESNO) */
  {
    w = gtk_button_new_with_label ("Yes");
    gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (w), "clicked",
			GTK_SIGNAL_FUNC (dlg_default_callback), GINT_TO_POINTER (LC_YES));
    GTK_WIDGET_SET_FLAGS (w, GTK_CAN_DEFAULT);
    gtk_widget_grab_default (w);
    gtk_widget_show (w);

    w = gtk_button_new_with_label ("No");
    gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT (w), "clicked",
			GTK_SIGNAL_FUNC (dlg_default_callback), GINT_TO_POINTER (LC_NO));
    gtk_widget_show (w);
    ret = LC_NO;
  }

  return dlg_domodal(window, ret);
}

// File Open/Save/Merge dialog

static char* filedlg_str;

static void filedlg_callback(GtkWidget *widget, gpointer data)
{
  if (data != NULL)
  {
    *cur_ret = LC_OK;
    strcpy(filedlg_str, gtk_file_selection_get_filename(GTK_FILE_SELECTION(data)));
  }
  else
    *cur_ret = LC_CANCEL;
}

int filedlg_execute(char* caption, char* filename)
{
  GtkWidget* dlg;
  dlg = gtk_file_selection_new (caption);
  filedlg_str = filename;

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(dlg)->ok_button),
     "clicked", (GtkSignalFunc)filedlg_callback, dlg);
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(dlg)->cancel_button),
     "clicked", (GtkSignalFunc)filedlg_callback, NULL);

//    gtk_file_selection_set_filename (GTK_FILE_SELECTION(filew),"penguin.png");

  return dlg_domodal(dlg, LC_CANCEL);
}

// Color Selection Dialog

static void colorseldlg_callback(GtkWidget *widget, gpointer data)
{
  if (data != NULL)
  {
    GtkWidget* dlg = gtk_widget_get_toplevel (widget);
    double dbl[3];
    float* color = (float*)data;

    gtk_color_selection_get_color (GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG (dlg)->colorsel), dbl);
    color[0] = dbl[0];
    color[1] = dbl[1];
    color[2] = dbl[2];
    *cur_ret = LC_OK;
  }
  else
    *cur_ret = LC_CANCEL;
}

int colorseldlg_execute(void* param)
{
  float* color = (float*)param;
  double dbl[3] = { color[0], color[1], color[2] };
  GtkWidget* dlg;

  dlg = gtk_color_selection_dialog_new ("Choose Color");
  gtk_color_selection_set_color (GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG (dlg)->colorsel), dbl);

  gtk_signal_connect (GTK_OBJECT (GTK_COLOR_SELECTION_DIALOG (dlg)->ok_button),
     "clicked", (GtkSignalFunc)colorseldlg_callback, param);
  gtk_signal_connect (GTK_OBJECT (GTK_COLOR_SELECTION_DIALOG (dlg)->cancel_button),
     "clicked", (GtkSignalFunc)colorseldlg_callback, NULL);

  return dlg_domodal(dlg, LC_CANCEL);
}

// Array Dialog

typedef struct
{
  void* data;
  GtkWidget *move_x, *move_y, *move_z;
  GtkWidget *rotate_x, *rotate_y, *rotate_z;
  GtkWidget *total;
  GtkWidget *radio1, *radio2, *radio3;
  GtkWidget *count1, *count2, *count3;
  GtkWidget *offset_x2, *offset_y2, *offset_z2;
  GtkWidget *offset_x3, *offset_y3, *offset_z3;
} LC_ARRAYDLG_STRUCT;

static void arraydlg_callback(GtkWidget *widget, gpointer data)
{
  LC_ARRAYDLG_STRUCT* s = (LC_ARRAYDLG_STRUCT*)data;
  LC_ARRAYDLG_OPTS* opts = (LC_ARRAYDLG_OPTS*)s->data;

  if (!read_float(s->move_x, &opts->fMove[0], -1000, 1000)) return;
  if (!read_float(s->move_y, &opts->fMove[1], -1000, 1000)) return;
  if (!read_float(s->move_z, &opts->fMove[2], -1000, 1000)) return;
  if (!read_float(s->rotate_x, &opts->fRotate[0], -360, 360)) return;
  if (!read_float(s->rotate_y, &opts->fRotate[1], -360, 360)) return;
  if (!read_float(s->rotate_z, &opts->fRotate[2], -360, 360)) return;
  if (!read_float(s->offset_x2, &opts->f2D[0], -1000, 1000)) return;
  if (!read_float(s->offset_y2, &opts->f2D[1], -1000, 1000)) return;
  if (!read_float(s->offset_z2, &opts->f2D[2], -1000, 1000)) return;
  if (!read_float(s->offset_x3, &opts->f3D[0], -1000, 1000)) return;
  if (!read_float(s->offset_y3, &opts->f3D[1], -1000, 1000)) return;
  if (!read_float(s->offset_z3, &opts->f3D[2], -1000, 1000)) return;

  int i;
  if (!read_int(s->count1, &i, 1, 1000)) return;
  opts->n1DCount = i;
  if (!read_int(s->count2, &i, 1, 1000)) return;
  opts->n2DCount = i;
  if (!read_int(s->count3, &i, 1, 1000)) return;
  opts->n3DCount = i;

  if (GTK_TOGGLE_BUTTON (s->radio1)->active)
    opts->nArrayDimension = 0;
  if (GTK_TOGGLE_BUTTON (s->radio2)->active)
    opts->nArrayDimension = 1;
  if (GTK_TOGGLE_BUTTON (s->radio3)->active)
    opts->nArrayDimension = 2;

  *cur_ret = LC_OK;
}

int arraydlg_execute(void* param)
{
  GtkWidget *dlg;
  GtkWidget *vbox1, *vbox2;
  GtkWidget *hbox1, *hbox2;
  GtkWidget *frame, *table, *label, *button;
  GSList *radio_group = (GSList*)NULL;
  LC_ARRAYDLG_STRUCT s;
  s.data = param;

  dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
		      GTK_SIGNAL_FUNC (dlg_delete_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
  gtk_widget_set_usize (dlg, 450, 320);
  gtk_window_set_title (GTK_WINDOW (dlg), "Array Options");
  gtk_window_set_policy (GTK_WINDOW (dlg), FALSE, FALSE, FALSE);
  gtk_widget_realize (dlg);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (dlg), vbox1);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);

  frame = gtk_frame_new ("Transformation (Incremental)");
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (hbox1), frame, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);

  table = gtk_table_new (3, 4, FALSE);
  gtk_widget_show (table);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_table_set_col_spacings (GTK_TABLE (table), 20);

  label = gtk_label_new ("X");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1,
                    (GtkAttachOptions) 0, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  label = gtk_label_new ("Y");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);

  label = gtk_label_new ("Z");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 3, 4, 0, 1,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);

  label = gtk_label_new ("Move");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);

  s.move_x = gtk_entry_new ();
  gtk_widget_show (s.move_x);
  gtk_table_attach (GTK_TABLE (table), s.move_x, 1, 2, 1, 2,
                    (GtkAttachOptions) 0, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (s.move_x, 50, -2);
  gtk_entry_set_text (GTK_ENTRY (s.move_x), "0");

  s.move_y = gtk_entry_new ();
  gtk_widget_show (s.move_y);
  gtk_table_attach (GTK_TABLE (table), s.move_y, 2, 3, 1, 2,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (s.move_y, 50, -2);
  gtk_entry_set_text (GTK_ENTRY (s.move_y), "0");

  s.move_z = gtk_entry_new ();
  gtk_widget_show (s.move_z);
  gtk_table_attach (GTK_TABLE (table), s.move_z, 3, 4, 1, 2,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (s.move_z, 50, -2);
  gtk_entry_set_text (GTK_ENTRY (s.move_z), "0");

  label = gtk_label_new ("Rotate");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);

  s.rotate_x = gtk_entry_new ();
  gtk_widget_show (s.rotate_x);
  gtk_table_attach (GTK_TABLE (table), s.rotate_x, 1, 2, 2, 3,
                    (GtkAttachOptions) 0, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (s.rotate_x, 50, -2);
  gtk_entry_set_text (GTK_ENTRY (s.rotate_x), "0");

  s.rotate_y = gtk_entry_new ();
  gtk_widget_show (s.rotate_y);
  gtk_table_attach (GTK_TABLE (table), s.rotate_y, 2, 3, 2, 3,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (s.rotate_y, 50, -2);
  gtk_entry_set_text (GTK_ENTRY (s.rotate_y), "0");

  s.rotate_z = gtk_entry_new ();
  gtk_widget_show (s.rotate_z);
  gtk_table_attach (GTK_TABLE (table), s.rotate_z, 3, 4, 2, 3,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (s.rotate_z, 50, -2);
  gtk_entry_set_text (GTK_ENTRY (s.rotate_z), "0");

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (hbox1), vbox2, FALSE, FALSE, 0);
  gtk_widget_set_usize (vbox2, 115, 142);

  button = gtk_button_new_with_label ("OK");
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (arraydlg_callback), &s);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (button), 12);
  GtkAccelGroup *accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (dlg), accel_group);
  gtk_widget_add_accelerator (button, "clicked", accel_group,
                              GDK_Return, 0, GTK_ACCEL_VISIBLE);

  button = gtk_button_new_with_label ("Cancel");
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (dlg_default_callback), GINT_TO_POINTER (LC_CANCEL));
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (button), 12);
  gtk_widget_add_accelerator (button, "clicked", accel_group,
                              GDK_Escape, 0, GTK_ACCEL_VISIBLE);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);

  label = gtk_label_new ("Total:");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox2), label, TRUE, TRUE, 0);

  s.total = gtk_entry_new ();
  gtk_widget_show (s.total);
  gtk_box_pack_start (GTK_BOX (hbox2), s.total, TRUE, TRUE, 15);
  gtk_widget_set_usize (s.total, 50, -2);
  gtk_entry_set_editable (GTK_ENTRY (s.total), FALSE);

  frame = gtk_frame_new ("Dimensions");
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (vbox1), frame, TRUE, TRUE, 5);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);

  table = gtk_table_new (4, 5, FALSE);
  gtk_widget_show (table);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_table_set_col_spacings (GTK_TABLE (table), 20);

  label = gtk_label_new ("Count");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  label = gtk_label_new ("Offsets");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 3, 4, 0, 1, (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  s.radio1 = gtk_radio_button_new_with_label (radio_group, "1D");
  radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (s.radio1));
  gtk_widget_show (s.radio1);
  gtk_table_attach (GTK_TABLE (table), s.radio1, 0, 1, 1, 2,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);

  s.radio2 = gtk_radio_button_new_with_label (radio_group, "2D");
  radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (s.radio2));
  gtk_widget_show (s.radio2);
  gtk_table_attach (GTK_TABLE (table), s.radio2, 0, 1, 2, 3,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);

  s.radio3 = gtk_radio_button_new_with_label (radio_group, "3D");
  radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (s.radio3));
  gtk_widget_show (s.radio3);
  gtk_table_attach (GTK_TABLE (table), s.radio3, 0, 1, 3, 4,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);

  s.count1 = gtk_entry_new ();
  gtk_widget_show (s.count1);
  gtk_table_attach (GTK_TABLE (table), s.count1, 1, 2, 1, 2,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (s.count1, 50, -2);
  gtk_entry_set_text (GTK_ENTRY (s.count1), "10");

  s.count2 = gtk_entry_new ();
  gtk_widget_show (s.count2);
  gtk_table_attach (GTK_TABLE (table), s.count2, 1, 2, 2, 3,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (s.count2, 50, -2);
  gtk_entry_set_text (GTK_ENTRY (s.count2), "1");

  s.count3 = gtk_entry_new ();
  gtk_widget_show (s.count3);
  gtk_table_attach (GTK_TABLE (table), s.count3, 1, 2, 3, 4,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (s.count3, 50, -2);
  gtk_entry_set_text (GTK_ENTRY (s.count3), "1");

  label = gtk_label_new ("X");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);

  label = gtk_label_new ("Y");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 3, 4, 1, 2,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);

  label = gtk_label_new ("Z");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 4, 5, 1, 2,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);

  s.offset_x2 = gtk_entry_new ();
  gtk_widget_show (s.offset_x2);
  gtk_table_attach (GTK_TABLE (table), s.offset_x2, 2, 3, 2, 3,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (s.offset_x2, 50, -2);
  gtk_entry_set_text (GTK_ENTRY (s.offset_x2), "0");

  s.offset_y2 = gtk_entry_new ();
  gtk_widget_show (s.offset_y2);
  gtk_table_attach (GTK_TABLE (table), s.offset_y2, 3, 4, 2, 3,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (s.offset_y2, 50, -2);
  gtk_entry_set_text (GTK_ENTRY (s.offset_y2), "0");

  s.offset_z2 = gtk_entry_new ();
  gtk_widget_show (s.offset_z2);
  gtk_table_attach (GTK_TABLE (table), s.offset_z2, 4, 5, 2, 3,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (s.offset_z2, 50, -2);
  gtk_entry_set_text (GTK_ENTRY (s.offset_z2), "0");

  s.offset_x3 = gtk_entry_new ();
  gtk_widget_show (s.offset_x3);
  gtk_table_attach (GTK_TABLE (table), s.offset_x3, 2, 3, 3, 4,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (s.offset_x3, 50, -2);
  gtk_entry_set_text (GTK_ENTRY (s.offset_x3), "0");

  s.offset_y3 = gtk_entry_new ();
  gtk_widget_show (s.offset_y3);
  gtk_table_attach (GTK_TABLE (table), s.offset_y3, 3, 4, 3, 4,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (s.offset_y3, 50, -2);
  gtk_entry_set_text (GTK_ENTRY (s.offset_y3), "0");

  s.offset_z3 = gtk_entry_new ();
  gtk_widget_show (s.offset_z3);
  gtk_table_attach (GTK_TABLE (table), s.offset_z3, 4, 5, 3, 4,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (s.offset_z3, 50, -2);
  gtk_entry_set_text (GTK_ENTRY (s.offset_z3), "0");

  return dlg_domodal(dlg, LC_CANCEL);
}

// About Dialog

int aboutdlg_execute(void* param)
{
#include "pixmaps/icon32.xpm"
  GtkWidget *dlg;
  GtkWidget *vbox1, *vbox2, *hbox;
  GtkWidget *frame, *w;
  GtkWidget *table;
  char info[256];

  dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
		      GTK_SIGNAL_FUNC (dlg_delete_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
  gtk_widget_set_usize (dlg, 430, 190);
  gtk_window_set_title (GTK_WINDOW (dlg), "About LeoCAD");
  gtk_window_set_policy (GTK_WINDOW (dlg), FALSE, FALSE, FALSE);
  gtk_widget_realize (dlg);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (dlg), vbox1);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox, TRUE, TRUE, 0);
  gtk_widget_set_usize (hbox, -2, 60);

  w = new_pixmap (dlg, icon32);
  gtk_widget_show (w);
  gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);
  gtk_widget_set_usize (w, 32, 32);

  vbox2 = gtk_vbox_new (TRUE, 0);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);

  w = gtk_label_new ("LeoCAD for Linux Version 0.72");
  gtk_widget_show (w);
  gtk_box_pack_start (GTK_BOX (vbox2), w, FALSE, FALSE, 5);

  w = gtk_label_new ("Copyright (c) 1996-99, BT Software");
  gtk_widget_show (w);
  gtk_box_pack_start (GTK_BOX (vbox2), w, FALSE, FALSE, 5);

  table = gtk_table_new (1, 1, FALSE);
  gtk_widget_show (table);
  gtk_container_add (GTK_CONTAINER (hbox), table);
  gtk_container_border_width (GTK_CONTAINER (table), 5);

  w = gtk_button_new_with_label ("OK");
  gtk_widget_show (w);
  gtk_signal_connect (GTK_OBJECT (w), "clicked",
		      GTK_SIGNAL_FUNC (dlg_default_callback), GINT_TO_POINTER (LC_OK));
  gtk_table_attach (GTK_TABLE (table), w, 0, 1, 0, 1,
                    (GtkAttachOptions) GTK_EXPAND, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_widget_set_usize (w, 60, 40);

  gtk_widget_grab_focus (w);
  GtkAccelGroup *accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (dlg), accel_group);
  gtk_widget_add_accelerator (w, "clicked", accel_group,
                              GDK_Return, 0, GTK_ACCEL_VISIBLE);

  frame = gtk_frame_new ("System Information");
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (vbox1), frame, TRUE, TRUE, 0);
  gtk_widget_set_usize (frame, -2, 100);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_container_border_width (GTK_CONTAINER (vbox2), 5);

  strcpy(info, "OpenGL Version ");
  strcat(info, (const char*)glGetString(GL_VERSION));
  strcat(info, "\n");
  strcat(info, (const char*)glGetString(GL_RENDERER));
  strcat(info, " - ");
  strcat(info, (const char*)glGetString(GL_VENDOR));

  w = gtk_text_new (NULL, NULL);
  gtk_widget_show (w);
  gtk_box_pack_start (GTK_BOX (vbox2), w, TRUE, TRUE, 0);
  gtk_text_insert (GTK_TEXT (w), NULL, NULL, NULL,
                   info, strlen(info));

  return dlg_domodal(dlg, LC_OK);
}

// HTML Dialog

typedef struct
{
  void* data;
  GtkWidget *single, *multiple, *index;
  GtkWidget *list_end, *list_step, *images;
  GtkWidget *highlight, *directory;
} LC_HTMLDLG_STRUCT;

static void htmldlg_ok(GtkWidget *widget, gpointer data)
{
  LC_HTMLDLG_STRUCT* s = (LC_HTMLDLG_STRUCT*)data;
  LC_HTMLDLG_OPTS* opts = (LC_HTMLDLG_OPTS*)s->data;

  opts->singlepage = (GTK_TOGGLE_BUTTON (s->single)->active) ? true : false;
  opts->index = (GTK_TOGGLE_BUTTON (s->index)->active) ? true : false;
  opts->images = (GTK_TOGGLE_BUTTON (s->images)->active) ? true : false;
  opts->listend = (GTK_TOGGLE_BUTTON (s->list_end)->active) ? true : false;
  opts->liststep = (GTK_TOGGLE_BUTTON (s->list_step)->active) ? true : false;
  opts->hilite = (GTK_TOGGLE_BUTTON (s->highlight)->active) ? true : false;
  strcpy(opts->path, gtk_entry_get_text (GTK_ENTRY (s->directory)));

  *cur_ret = LC_OK;
}

static void htmldlg_images(GtkWidget *widget, gpointer data)
{
  LC_HTMLDLG_STRUCT* s = (LC_HTMLDLG_STRUCT*)data;
  LC_HTMLDLG_OPTS* opts = (LC_HTMLDLG_OPTS*)s->data;

  imageoptsdlg_execute(&opts->imdlg ,true);
}

int htmldlg_execute(void* param)
{
  GtkWidget *dlg;
  GtkWidget *vbox1, *vbox2;
  GtkWidget *hbox;
  GtkWidget *frame, *label, *button;
  GSList *radio_group = (GSList*)NULL;
  LC_HTMLDLG_STRUCT s;
  s.data = param;

  dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
		      GTK_SIGNAL_FUNC (dlg_delete_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
  gtk_widget_set_usize (dlg, 350, 320);
  gtk_window_set_title (GTK_WINDOW (dlg), "HTML Options");
  gtk_window_set_policy (GTK_WINDOW (dlg), FALSE, FALSE, FALSE);
  gtk_widget_realize (dlg);

  vbox1 = gtk_vbox_new (FALSE, 10);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (dlg), vbox1);
  gtk_container_border_width (GTK_CONTAINER (vbox1), 20);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, TRUE, 0);

  frame = gtk_frame_new ("Layout");
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
  gtk_widget_set_usize (frame, 220, 100);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_container_border_width (GTK_CONTAINER (vbox2), 5);

  s.single = gtk_radio_button_new_with_label (radio_group, "Single page");
  radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (s.single));
  gtk_widget_show (s.single);
  gtk_box_pack_start (GTK_BOX (vbox2), s.single, TRUE, TRUE, 0);

  s.multiple = gtk_radio_button_new_with_label (radio_group, "One step per page");
  radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (s.multiple));
  gtk_widget_show (s.multiple);
  gtk_box_pack_start (GTK_BOX (vbox2), s.multiple, TRUE, TRUE, 0);

  s.index = gtk_check_button_new_with_label ("Index page");
  gtk_widget_show (s.index);
  gtk_box_pack_start (GTK_BOX (vbox2), s.index, TRUE, TRUE, 0);

  vbox2 = gtk_vbox_new (FALSE, 5);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 10);
  gtk_widget_set_usize (vbox2, 85, -2);
  gtk_container_border_width (GTK_CONTAINER (vbox2), 5);

  button = gtk_button_new_with_label ("OK");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (htmldlg_ok), &s);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_set_usize (button, -2, 25);
  GtkAccelGroup *accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (dlg), accel_group);
  gtk_widget_add_accelerator (button, "clicked", accel_group,
                              GDK_Return, 0, GTK_ACCEL_VISIBLE);

  button = gtk_button_new_with_label ("Cancel");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (dlg_default_callback), GINT_TO_POINTER (LC_CANCEL));
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_set_usize (button, -2, 25);
  gtk_widget_add_accelerator (button, "clicked", accel_group,
                              GDK_Escape, 0, GTK_ACCEL_VISIBLE);

  button = gtk_button_new_with_label ("Images...");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (htmldlg_images), &s);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_set_usize (button, -2, 25);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, TRUE, 0);

  frame = gtk_frame_new ("Pieces List");
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
  gtk_widget_set_usize (frame, 220, 100);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_container_border_width (GTK_CONTAINER (vbox2), 5);

  s.list_step = gtk_check_button_new_with_label ("After each step");
  gtk_widget_show (s.list_step);
  gtk_box_pack_start (GTK_BOX (vbox2), s.list_step, TRUE, TRUE, 0);

  s.list_end = gtk_check_button_new_with_label ("At the end");
  gtk_widget_show (s.list_end);
  gtk_box_pack_start (GTK_BOX (vbox2), s.list_end, TRUE, TRUE, 0);

  s.images = gtk_check_button_new_with_label ("Create Images");
  gtk_widget_show (s.images);
  gtk_box_pack_start (GTK_BOX (vbox2), s.images, TRUE, TRUE, 0);

  s.highlight = gtk_check_button_new_with_label ("Highlight new pieces");
  gtk_widget_show (s.highlight);
  gtk_box_pack_start (GTK_BOX (vbox1), s.highlight, FALSE, TRUE, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, TRUE, 0);
  gtk_widget_set_usize (hbox, -2, 30);

  label = gtk_label_new ("Output directory");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  s.directory = gtk_entry_new ();
  gtk_widget_show (s.directory);
  gtk_box_pack_end (GTK_BOX (hbox), s.directory, FALSE, TRUE, 0);
  gtk_widget_set_usize (s.directory, 210, -2);

  return dlg_domodal(dlg, LC_CANCEL);
}

// Image Options Dialog

typedef struct
{
  void* data;
  bool from_htmldlg;
  GtkWidget *single, *multiple, *from, *to;
  GtkWidget *width, *height;
  GtkWidget *bitmap, *gif, *jpg, *png;
  GtkWidget *highcolor, *transparent, *progressive, *quality;
} LC_IMAGEOPTSDLG_STRUCT;

static void imageoptsdlg_ok(GtkWidget *widget, gpointer data)
{
  LC_IMAGEOPTSDLG_STRUCT* s = (LC_IMAGEOPTSDLG_STRUCT*)data;
  LC_IMAGEDLG_OPTS* opts = (LC_IMAGEDLG_OPTS*)s->data;
  int i;

  if ((GTK_TOGGLE_BUTTON (s->bitmap)->active))
    opts->imopts.format = LC_IMAGE_BMP;
  else if ((GTK_TOGGLE_BUTTON (s->gif)->active))
    opts->imopts.format = LC_IMAGE_GIF;
  else if ((GTK_TOGGLE_BUTTON (s->jpg)->active))
    opts->imopts.format = LC_IMAGE_JPG;
  else if ((GTK_TOGGLE_BUTTON (s->png)->active))
    opts->imopts.format = LC_IMAGE_PNG;
  opts->imopts.transparent = (GTK_TOGGLE_BUTTON (s->transparent)->active) ? true : false;
  opts->imopts.interlaced = (GTK_TOGGLE_BUTTON (s->progressive)->active) ? true : false;
  opts->imopts.truecolor = (GTK_TOGGLE_BUTTON (s->highcolor)->active) ? true : false;
  if (!read_int(s->quality, &i, 1, 100)) return;
  opts->imopts.quality = i;
  if (!read_int(s->height, &i, 1, 5000)) return;
  opts->height = i;
  if (!read_int(s->width, &i, 1, 5000)) return;
  opts->width = i;
  if (!read_int(s->from, &i, 1, 65535)) return;
  opts->from = i;
  if (!read_int(s->to, &i, 1, 65535)) return;
  opts->to = i;
  opts->multiple = (GTK_TOGGLE_BUTTON (s->multiple)->active) ? true : false;

  unsigned long image = opts->imopts.format;
  if (opts->imopts.interlaced)
    image |= LC_IMAGE_PROGRESSIVE;
  if (opts->imopts.transparent)
    image |= LC_IMAGE_TRANSPARENT;
  if (opts->imopts.truecolor)
    image |= LC_IMAGE_HIGHCOLOR;

  if (s->from_htmldlg)
  {
    SystemSetProfileInt("Default", "HTML Options", image);
    SystemSetProfileInt("Default", "HTML Width", opts->width);
    SystemSetProfileInt("Default", "HTML Height", opts->height);
  }
  else
  {
    SystemSetProfileInt("Default", "Image Options", image);
    SystemSetProfileInt("Default", "Image Width", opts->width);
    SystemSetProfileInt("Default", "Image Height", opts->height);
  }

  *cur_ret = LC_OK;
}

int imageoptsdlg_execute(void* param, bool from_htmldlg)
{
  GtkWidget *dlg;
  GtkWidget *vbox1, *vbox2;
  GtkWidget *hbox1, *hbox2;
  GtkWidget *frame, *label, *button, *table;
  GSList *radio_group1 = (GSList*)NULL, *radio_group2 = (GSList*)NULL;
  LC_IMAGEOPTSDLG_STRUCT s;
  LC_IMAGEDLG_OPTS* opts = (LC_IMAGEDLG_OPTS*)param;
  s.data = param;
  s.from_htmldlg = from_htmldlg;

  dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
		      GTK_SIGNAL_FUNC (dlg_delete_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
  gtk_widget_set_usize (dlg, 400, 280);
  gtk_window_set_title (GTK_WINDOW (dlg), "Image Options");
  gtk_window_set_policy (GTK_WINDOW (dlg), FALSE, FALSE, FALSE);
  gtk_widget_realize (dlg);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);
  gtk_container_add (GTK_CONTAINER (dlg), hbox1);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox1);
  gtk_box_pack_start (GTK_BOX (hbox1), vbox1, TRUE, TRUE, 0);

  frame = gtk_frame_new ("Pictures");
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (vbox1), frame, FALSE, TRUE, 0);
  gtk_widget_set_usize (frame, 170, 130);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_container_border_width (GTK_CONTAINER (vbox2), 5);

  s.single = gtk_radio_button_new_with_label (radio_group1, "Single");
  radio_group1 = gtk_radio_button_group (GTK_RADIO_BUTTON (s.single));
  gtk_widget_show (s.single);
  gtk_box_pack_start (GTK_BOX (vbox2), s.single, TRUE, TRUE, 0);

  s.multiple = gtk_radio_button_new_with_label (radio_group1, "Multiple");
  radio_group1 = gtk_radio_button_group (GTK_RADIO_BUTTON (s.multiple));
  gtk_widget_show (s.multiple);
  gtk_box_pack_start (GTK_BOX (vbox2), s.multiple, TRUE, TRUE, 0);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);

  label = gtk_label_new ("From");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox2), label, TRUE, TRUE, 0);

  s.from = gtk_entry_new ();
  gtk_widget_show (s.from);
  gtk_box_pack_start (GTK_BOX (hbox2), s.from, FALSE, FALSE, 0);
  gtk_widget_set_usize (s.from, 50, -2);

  label = gtk_label_new ("to");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox2), label, TRUE, TRUE, 0);

  s.to = gtk_entry_new ();
  gtk_widget_show (s.to);
  gtk_box_pack_start (GTK_BOX (hbox2), s.to, FALSE, FALSE, 0);
  gtk_widget_set_usize (s.to, 50, -2);

  frame = gtk_frame_new ("Dimensions");
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (vbox1), frame, FALSE, TRUE, 0);
  gtk_widget_set_usize (frame, -2, 100);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);

  table = gtk_table_new (2, 2, FALSE);
  gtk_widget_show (table);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_container_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table), 10);

  s.width = gtk_entry_new ();
  gtk_widget_show (s.width);
  gtk_table_attach (GTK_TABLE (table), s.width, 1, 2, 0, 1,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (s.width, 50, -2);

  s.height = gtk_entry_new ();
  gtk_widget_show (s.height);
  gtk_table_attach (GTK_TABLE (table), s.height, 1, 2, 1, 2,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (s.height, 50, -2);

  label = gtk_label_new ("Height");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (label, 50, -2);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  label = gtk_label_new ("Width");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (label, 50, -2);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  hbox2 = gtk_hbox_new (FALSE, 5);
  gtk_widget_show (hbox2);
  gtk_box_pack_end (GTK_BOX (vbox1), hbox2, FALSE, TRUE, 10);
  gtk_container_border_width (GTK_CONTAINER (hbox2), 5);

  button = gtk_button_new_with_label ("OK");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (imageoptsdlg_ok), &s);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, TRUE, 25);
  gtk_widget_set_usize (button, 70, 25);
  GtkAccelGroup *accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (dlg), accel_group);
  gtk_widget_add_accelerator (button, "clicked", accel_group,
                              GDK_Return, 0, GTK_ACCEL_VISIBLE);

  button = gtk_button_new_with_label ("Cancel");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (dlg_default_callback), GINT_TO_POINTER (LC_CANCEL));
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, TRUE, 0);
  gtk_widget_set_usize (button, 70, -2);
  gtk_widget_add_accelerator (button, "clicked", accel_group,
                              GDK_Escape, 0, GTK_ACCEL_VISIBLE);

  frame = gtk_frame_new ("Format");
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (hbox1), frame, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (frame), vbox1);
  gtk_container_border_width (GTK_CONTAINER (vbox1), 5);

  s.bitmap = gtk_radio_button_new_with_label (radio_group2, "Bitmap");
  radio_group2 = gtk_radio_button_group (GTK_RADIO_BUTTON (s.bitmap));
  gtk_widget_show (s.bitmap);
  gtk_box_pack_start (GTK_BOX (vbox1), s.bitmap, TRUE, TRUE, 0);

  s.highcolor = gtk_check_button_new_with_label ("High color");
  gtk_widget_show (s.highcolor);
  gtk_box_pack_start (GTK_BOX (vbox1), s.highcolor, TRUE, TRUE, 0);

  s.gif = gtk_radio_button_new_with_label (radio_group2, "GIF");
  radio_group2 = gtk_radio_button_group (GTK_RADIO_BUTTON (s.gif));
  gtk_widget_show (s.gif);
  gtk_box_pack_start (GTK_BOX (vbox1), s.gif, TRUE, TRUE, 0);

  s.transparent = gtk_check_button_new_with_label ("Transparent");
  gtk_widget_show (s.transparent);
  gtk_box_pack_start (GTK_BOX (vbox1), s.transparent, TRUE, TRUE, 0);

  s.jpg = gtk_radio_button_new_with_label (radio_group2, "JPEG");
  radio_group2 = gtk_radio_button_group (GTK_RADIO_BUTTON (s.jpg));
  gtk_widget_show (s.jpg);
  gtk_box_pack_start (GTK_BOX (vbox1), s.jpg, TRUE, TRUE, 0);

  s.progressive = gtk_check_button_new_with_label ("Progressive");
  gtk_widget_show (s.progressive);
  gtk_box_pack_start (GTK_BOX (vbox1), s.progressive, TRUE, TRUE, 0);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox2, TRUE, TRUE, 0);

  label = gtk_label_new ("Quality");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox2), label, TRUE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  s.quality = gtk_entry_new ();
  gtk_widget_show (s.quality);
  gtk_box_pack_start (GTK_BOX (hbox2), s.quality, FALSE, FALSE, 0);
  gtk_widget_set_usize (s.quality, 50, -2);

  s.png = gtk_radio_button_new_with_label (radio_group2, "PNG");
  radio_group2 = gtk_radio_button_group (GTK_RADIO_BUTTON (s.png));
  gtk_widget_show (s.png);
  gtk_box_pack_start (GTK_BOX (vbox1), s.png, TRUE, TRUE, 0);


  switch (opts->imopts.format)
  {
    case LC_IMAGE_BMP:
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.bitmap), TRUE);
    break;

    case LC_IMAGE_GIF:
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.gif), TRUE);
    break;

    case LC_IMAGE_JPG:
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.jpg), TRUE);
    break;

    case LC_IMAGE_PNG:
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.png), TRUE);
    break;
  }

  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.transparent), opts->imopts.transparent);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.progressive), opts->imopts.interlaced);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.highcolor), opts->imopts.truecolor);

  write_int(s.quality, opts->imopts.quality);
  write_int(s.height, opts->height);
  write_int(s.width, opts->width);
  write_int(s.from, opts->from);
  write_int(s.to, opts->to);

  if (opts->multiple)
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.multiple), TRUE);
  else
    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.single), TRUE);

  return dlg_domodal(dlg, LC_CANCEL);
}

// POV-Ray Export Dialog

typedef struct
{
  void* data;
  GtkWidget *lgeo, *pov, *output, *render;
} LC_POVRAYDLG_STRUCT;

static void povraydlg_ok(GtkWidget *widget, gpointer data)
{
  LC_POVRAYDLG_STRUCT* s = (LC_POVRAYDLG_STRUCT*)data;
  LC_POVRAYDLG_OPTS* opts = (LC_POVRAYDLG_OPTS*)s->data;

  opts->render =  (GTK_TOGGLE_BUTTON (s->render)->active) ? true : false;
  strcpy(opts->povpath, gtk_entry_get_text (GTK_ENTRY (s->pov)));
  strcpy(opts->outpath, gtk_entry_get_text (GTK_ENTRY (s->output)));
  strcpy(opts->libpath, gtk_entry_get_text (GTK_ENTRY (s->lgeo)));

  *cur_ret = LC_OK;
}

int povraydlg_execute(void* param)
{
  GtkWidget *dlg;
  GtkWidget *vbox, *hbox1, *hbox2;
  GtkWidget *label, *button;
  LC_POVRAYDLG_STRUCT s;
  s.data = param;

  dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
		      GTK_SIGNAL_FUNC (dlg_delete_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
  gtk_widget_set_usize (dlg, 375, 190);
  gtk_window_set_title (GTK_WINDOW (dlg), "POV-Ray Export");
  gtk_window_set_policy (GTK_WINDOW (dlg), FALSE, FALSE, FALSE);
  gtk_widget_realize (dlg);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);
  gtk_container_add (GTK_CONTAINER (dlg), hbox1);

  vbox = gtk_vbox_new (FALSE, 10);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (hbox1), vbox, FALSE, TRUE, 0);
  gtk_widget_set_usize (vbox, 250, -2);
  gtk_container_border_width (GTK_CONTAINER (vbox), 10);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, TRUE, 0);

  label = gtk_label_new ("LGEO Path (optional)");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  s.lgeo = gtk_entry_new ();
  gtk_widget_show (s.lgeo);
  gtk_box_pack_start (GTK_BOX (vbox), s.lgeo, FALSE, TRUE, 0);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, TRUE, 0);

  label = gtk_label_new ("POV-Ray Executable");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  s.pov = gtk_entry_new ();
  gtk_widget_show (s.pov);
  gtk_box_pack_start (GTK_BOX (vbox), s.pov, FALSE, TRUE, 0);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, TRUE, 0);

  label = gtk_label_new ("Output File");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  s.output = gtk_entry_new ();
  gtk_widget_show (s.output);
  gtk_box_pack_start (GTK_BOX (vbox), s.output, FALSE, TRUE, 0);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (hbox1), vbox, FALSE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (vbox), 10);

  button = gtk_button_new_with_label ("OK");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (povraydlg_ok), &s);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, TRUE, 5);
  gtk_widget_set_usize (button, -2, 25);
  GtkAccelGroup *accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (dlg), accel_group);
  gtk_widget_add_accelerator (button, "clicked", accel_group,
                              GDK_Return, 0, GTK_ACCEL_VISIBLE);

  button = gtk_button_new_with_label ("Cancel");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (dlg_default_callback), GINT_TO_POINTER (LC_CANCEL));
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, TRUE, 5);
  gtk_widget_set_usize (button, -2, 25);
  gtk_widget_add_accelerator (button, "clicked", accel_group,
                              GDK_Escape, 0, GTK_ACCEL_VISIBLE);

  s.render = gtk_check_button_new_with_label ("Render Scene");
  gtk_widget_show (s.render);
  gtk_box_pack_start (GTK_BOX (vbox), s.render, FALSE, TRUE, 5);
  gtk_widget_set_sensitive (s.render, FALSE);

  return dlg_domodal(dlg, LC_CANCEL);
}

// Preferences Dialog

typedef struct
{
  void* data;
  GtkWidget *det_edges, *det_dither, *det_lighting, *det_smooth;
  GtkWidget *det_antialias, *det_linear, *det_screen, *det_fast;
  GtkWidget *det_solid, *det_hidden, *det_background, *det_width;
  GtkWidget *draw_grid, *draw_gridunits, *draw_axis, *draw_preview;
  GtkWidget *draw_snapx, *draw_snapy, *draw_snapz, *draw_angle;
  GtkWidget *draw_anglesnap, *draw_centimeter, *draw_collision;
  GtkWidget *draw_move, *draw_fixed;
  GtkWidget *draw_lockx, *draw_locky, *draw_lockz;
  GtkWidget *scn_solid, *scn_gradient, *scn_image, *scn_imagename;
  GtkWidget *scn_tile, *scn_fog, *scn_floor, *scn_density;
  GtkWidget *scn_clrbackground, *scn_clrgrad1, *scn_clrgrad2, *scn_clrfog, *scn_clrambient;
} LC_PREFERENCESDLG_STRUCT;

static void preferencesdlg_ok(GtkWidget *widget, gpointer data)
{
  LC_PREFERENCESDLG_STRUCT* s = (LC_PREFERENCESDLG_STRUCT*)data;
  LC_PREFERENCESDLG_OPTS* opts = (LC_PREFERENCESDLG_OPTS*)s->data;

  unsigned long detail = 0;
  float line_width;
  if (GTK_TOGGLE_BUTTON (s->det_edges)->active) detail |= LC_DET_BRICKEDGES;
  if (GTK_TOGGLE_BUTTON (s->det_dither)->active) detail |= LC_DET_DITHER;
  if (GTK_TOGGLE_BUTTON (s->det_lighting)->active) detail |= LC_DET_LIGHTING;
  if (GTK_TOGGLE_BUTTON (s->det_smooth)->active) detail |= LC_DET_SMOOTH;
  if (GTK_TOGGLE_BUTTON (s->det_antialias)->active) detail |= LC_DET_ANTIALIAS;
  if (GTK_TOGGLE_BUTTON (s->det_linear)->active) detail |= LC_DET_LINEAR;
  if (GTK_TOGGLE_BUTTON (s->det_screen)->active) detail |= LC_DET_SCREENDOOR;
  if (GTK_TOGGLE_BUTTON (s->det_fast)->active) detail |= LC_DET_FAST;
  if (GTK_TOGGLE_BUTTON (s->det_solid)->active) detail |= LC_DET_BOX_FILL;
  if (GTK_TOGGLE_BUTTON (s->det_hidden)->active) detail |= LC_DET_HIDDEN_LINE;
  if (GTK_TOGGLE_BUTTON (s->det_background)->active) detail |= LC_DET_BACKGROUND;
  if (!read_float(s->det_width, &line_width, 0.5f, 5.0f)) return;

  unsigned long snap = 0;
  int grid_size, angle_snap;
  if (GTK_TOGGLE_BUTTON (s->draw_grid)->active) snap |= LC_DRAW_GRID;
  if (GTK_TOGGLE_BUTTON (s->draw_axis)->active) snap |= LC_DRAW_AXIS;
  if (GTK_TOGGLE_BUTTON (s->draw_preview)->active) snap |= LC_DRAW_PREVIEW;
  if (GTK_TOGGLE_BUTTON (s->draw_snapx)->active) snap |= LC_DRAW_SNAP_X;
  if (GTK_TOGGLE_BUTTON (s->draw_snapy)->active) snap |= LC_DRAW_SNAP_Y;
  if (GTK_TOGGLE_BUTTON (s->draw_snapz)->active) snap |= LC_DRAW_SNAP_Z;
  if (GTK_TOGGLE_BUTTON (s->draw_angle)->active) snap |= LC_DRAW_SNAP_A;
  if (GTK_TOGGLE_BUTTON (s->draw_centimeter)->active) snap |= LC_DRAW_CM_UNITS;
  //  if (GTK_TOGGLE_BUTTON (s->draw_collision)->active) snap |= LC_DRAW_COLLISION;
  if (GTK_TOGGLE_BUTTON (s->draw_move)->active) snap |= LC_DRAW_MOVE;
  if (GTK_TOGGLE_BUTTON (s->draw_fixed)->active) snap |= LC_DRAW_MOVEAXIS;
  if (GTK_TOGGLE_BUTTON (s->draw_lockx)->active) snap |= LC_DRAW_LOCK_X;
  if (GTK_TOGGLE_BUTTON (s->draw_locky)->active) snap |= LC_DRAW_LOCK_Y;
  if (GTK_TOGGLE_BUTTON (s->draw_lockz)->active) snap |= LC_DRAW_LOCK_Z;
  if (!read_int(s->draw_gridunits, &grid_size, 2, 1000)) return;
  if (!read_int(s->draw_anglesnap, &angle_snap, 1, 180)) return;

  int fog;
  unsigned long scene = 0;
  if (GTK_TOGGLE_BUTTON (s->scn_gradient)->active) scene |= LC_SCENE_GRADIENT;
  if (GTK_TOGGLE_BUTTON (s->scn_image)->active) scene |= LC_SCENE_BG;
  if (GTK_TOGGLE_BUTTON (s->scn_tile)->active) scene |= LC_SCENE_BG_TILE;
  if (GTK_TOGGLE_BUTTON (s->scn_fog)->active) scene |= LC_SCENE_FOG;
  if (GTK_TOGGLE_BUTTON (s->scn_floor)->active) scene |= LC_SCENE_FLOOR;
  read_int(s->scn_density, &fog, 1, 100);

  strcpy(opts->strBackground, gtk_entry_get_text (GTK_ENTRY (s->scn_imagename)));
  opts->nDetail = detail;
  opts->fLineWidth = line_width;
  opts->nSnap = snap;
  opts->nAngleSnap = angle_snap;
  opts->nGridSize = grid_size;
  opts->nScene = scene;
  opts->fDensity = (float)fog/100;

  // int nMouse;
  // int nSaveInterval;
  // char strPath[LC_MAXPATH];

  *cur_ret = LC_OK;
}

static void preferencesdlg_color(GtkWidget *widget, gpointer data)
{
  if (colorseldlg_execute(data) == LC_OK)
    set_button_pixmap (widget, (float*)data);
}

static void preferencesdlg_realize(GtkWidget *widget)
{
  void* data = gtk_object_get_data (GTK_OBJECT (widget), "color_flt");
  set_button_pixmap (widget, (float*)data);
}

int preferencesdlg_execute(void* param)
{
  GtkWidget *dlg;
  GtkWidget *vbox1, *vbox2, *hbox;
  GtkWidget *frame, *label, *button, *table, *notebook;
  GSList *table_group = NULL;
  LC_PREFERENCESDLG_STRUCT s;
  LC_PREFERENCESDLG_OPTS* opts = (LC_PREFERENCESDLG_OPTS*)param;
  s.data = param;

  dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
		      GTK_SIGNAL_FUNC (dlg_delete_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
  gtk_widget_set_usize (dlg, 450, 300);
  gtk_window_set_title (GTK_WINDOW (dlg), "Preferences");
  gtk_window_set_policy (GTK_WINDOW (dlg), FALSE, FALSE, FALSE);
  gtk_widget_realize (dlg);

  vbox1 = gtk_vbox_new (FALSE, 7);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (dlg), vbox1);
  gtk_container_border_width (GTK_CONTAINER (vbox1), 7);

  notebook = gtk_notebook_new ();
  gtk_widget_show (notebook);
  gtk_box_pack_start (GTK_BOX (vbox1), notebook, TRUE, TRUE, 0);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (notebook), vbox2);

  table = gtk_table_new (7, 2, TRUE);
  gtk_widget_show (table);
  gtk_container_add (GTK_CONTAINER (notebook), table);
  gtk_container_border_width (GTK_CONTAINER (table), 5);

  s.det_edges = gtk_check_button_new_with_label ("Draw edges");
  gtk_widget_show (s.det_edges);
  gtk_table_attach (GTK_TABLE (table), s.det_edges, 0, 1, 0, 1,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.det_dither = gtk_check_button_new_with_label ("Dithering");
  gtk_widget_show (s.det_dither);
  gtk_table_attach (GTK_TABLE (table), s.det_dither, 0, 1, 1, 2,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.det_lighting = gtk_check_button_new_with_label ("Lighting");
  gtk_widget_show (s.det_lighting);
  gtk_table_attach (GTK_TABLE (table), s.det_lighting, 0, 1, 2, 3,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.det_smooth = gtk_check_button_new_with_label ("Smooth shading");
  gtk_widget_show (s.det_smooth);
  gtk_table_attach (GTK_TABLE (table), s.det_smooth, 0, 1, 3, 4,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.det_antialias = gtk_check_button_new_with_label ("Anti-aliasing");
  gtk_widget_show (s.det_antialias);
  gtk_table_attach (GTK_TABLE (table), s.det_antialias, 0, 1, 4, 5,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.det_linear = gtk_check_button_new_with_label ("Linear filtering");
  gtk_widget_show (s.det_linear);
  gtk_table_attach (GTK_TABLE (table), s.det_linear, 0, 1, 5, 6,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.det_screen = gtk_check_button_new_with_label ("Screen door transparency");
  gtk_widget_show (s.det_screen);
  gtk_table_attach (GTK_TABLE (table), s.det_screen, 0, 1, 6, 7,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.det_fast = gtk_check_button_new_with_label ("Fast rendering");
  gtk_widget_show (s.det_fast);
  gtk_table_attach (GTK_TABLE (table), s.det_fast, 1, 2, 0, 1,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.det_solid = gtk_check_button_new_with_label ("Draw solid boxes");
  gtk_widget_show (s.det_solid);
  gtk_table_attach (GTK_TABLE (table), s.det_solid, 1, 2, 1, 2,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.det_hidden = gtk_check_button_new_with_label ("Remove hidden lines");
  gtk_widget_show (s.det_hidden);
  gtk_table_attach (GTK_TABLE (table), s.det_hidden, 1, 2, 2, 3,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.det_background = gtk_check_button_new_with_label ("Background rendering");
  gtk_widget_show (s.det_background);
  gtk_table_attach (GTK_TABLE (table), s.det_background, 1, 2, 3, 4,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_widget_show (hbox);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 4, 5,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  label = gtk_label_new ("Line width");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  s.det_width = gtk_entry_new ();
  gtk_widget_show (s.det_width);
  gtk_box_pack_start (GTK_BOX (hbox), s.det_width, FALSE, FALSE, 0);
  gtk_widget_set_usize (s.det_width, 50, -2);

  table = gtk_table_new (8, 2, TRUE);
  gtk_widget_show (table);
  gtk_container_add (GTK_CONTAINER (notebook), table);
  gtk_container_border_width (GTK_CONTAINER (table), 5);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_widget_show (hbox);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 0, 1,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.draw_grid = gtk_check_button_new_with_label ("Base grid");
  gtk_widget_show (s.draw_grid);
  gtk_box_pack_start (GTK_BOX (hbox), s.draw_grid, FALSE, FALSE, 0);

  s.draw_gridunits = gtk_entry_new ();
  gtk_widget_show (s.draw_gridunits);
  gtk_box_pack_start (GTK_BOX (hbox), s.draw_gridunits, FALSE, FALSE, 0);
  gtk_widget_set_usize (s.draw_gridunits, 50, -2);

  label = gtk_label_new ("units");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  s.draw_axis = gtk_check_button_new_with_label ("Axis icon");
  gtk_widget_show (s.draw_axis);
  gtk_table_attach (GTK_TABLE (table), s.draw_axis, 0, 1, 1, 2,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.draw_preview = gtk_check_button_new_with_label ("Preview position");
  gtk_widget_show (s.draw_preview);
  gtk_table_attach (GTK_TABLE (table), s.draw_preview, 0, 1, 2, 3,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.draw_snapx = gtk_check_button_new_with_label ("Snap X");
  gtk_widget_show (s.draw_snapx);
  gtk_table_attach (GTK_TABLE (table), s.draw_snapx, 0, 1, 3, 4,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.draw_snapy = gtk_check_button_new_with_label ("Snap Y");
  gtk_widget_show (s.draw_snapy);
  gtk_table_attach (GTK_TABLE (table), s.draw_snapy, 0, 1, 4, 5,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.draw_snapz = gtk_check_button_new_with_label ("Snap Z");
  gtk_widget_show (s.draw_snapz);
  gtk_table_attach (GTK_TABLE (table), s.draw_snapz, 0, 1, 5, 6,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_widget_show (hbox);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 6, 7,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.draw_angle = gtk_check_button_new_with_label ("Angle snap");
  gtk_widget_show (s.draw_angle);
  gtk_box_pack_start (GTK_BOX (hbox), s.draw_angle, FALSE, FALSE, 0);

  s.draw_anglesnap = gtk_entry_new ();
  gtk_widget_show (s.draw_anglesnap);
  gtk_box_pack_start (GTK_BOX (hbox), s.draw_anglesnap, FALSE, FALSE, 0);
  gtk_widget_set_usize (s.draw_anglesnap, 50, -2);

  label = gtk_label_new ("degrees");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  s.draw_centimeter = gtk_check_button_new_with_label ("Centimeter units");
  gtk_widget_show (s.draw_centimeter);
  gtk_table_attach (GTK_TABLE (table), s.draw_centimeter, 0, 1, 7, 8,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.draw_collision = gtk_check_button_new_with_label ("Collision detection");
  gtk_widget_show (s.draw_collision);
  gtk_table_attach (GTK_TABLE (table), s.draw_collision, 1, 2, 0, 1,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);
  gtk_widget_set_sensitive (s.draw_collision, FALSE);

  s.draw_move = gtk_check_button_new_with_label ("Switch to move after insert");
  gtk_widget_show (s.draw_move);
  gtk_table_attach (GTK_TABLE (table), s.draw_move, 1, 2, 1, 2,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.draw_fixed = gtk_check_button_new_with_label ("Fixed direction keys");
  gtk_widget_show (s.draw_fixed);
  gtk_table_attach (GTK_TABLE (table), s.draw_fixed, 1, 2, 2, 3,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.draw_lockx = gtk_check_button_new_with_label ("Lock X");
  gtk_widget_show (s.draw_lockx);
  gtk_table_attach (GTK_TABLE (table), s.draw_lockx, 1, 2, 3, 4,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.draw_locky = gtk_check_button_new_with_label ("Lock Y");
  gtk_widget_show (s.draw_locky);
  gtk_table_attach (GTK_TABLE (table), s.draw_locky, 1, 2, 4, 5,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.draw_lockz = gtk_check_button_new_with_label ("Lock Z");
  gtk_widget_show (s.draw_lockz);
  gtk_table_attach (GTK_TABLE (table), s.draw_lockz, 1, 2, 5, 6,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_widget_show (hbox);
  gtk_container_add (GTK_CONTAINER (notebook), hbox);
  gtk_container_border_width (GTK_CONTAINER (hbox), 5);

  frame = gtk_frame_new ("Background");
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

  table = gtk_table_new (4, 3, FALSE);
  gtk_widget_show (table);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_set_usize (table, 220, -2);
  gtk_container_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);

  s.scn_solid = gtk_radio_button_new_with_label (table_group, "Solid color");
  table_group = gtk_radio_button_group (GTK_RADIO_BUTTON (s.scn_solid));
  gtk_widget_show (s.scn_solid);
  gtk_table_attach (GTK_TABLE (table), s.scn_solid, 0, 1, 0, 1,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)GTK_EXPAND, 0, 0);

  s.scn_gradient = gtk_radio_button_new_with_label (table_group, "Gradient");
  table_group = gtk_radio_button_group (GTK_RADIO_BUTTON (s.scn_gradient));
  gtk_widget_show (s.scn_gradient);
  gtk_table_attach (GTK_TABLE (table), s.scn_gradient, 0, 1, 1, 2,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)GTK_EXPAND, 0, 0);

  s.scn_image = gtk_radio_button_new_with_label (table_group, "Image");
  table_group = gtk_radio_button_group (GTK_RADIO_BUTTON (s.scn_image));
  gtk_widget_show (s.scn_image);
  gtk_table_attach (GTK_TABLE (table), s.scn_image, 0, 1, 2, 3,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)GTK_EXPAND, 0, 0);

  s.scn_imagename = gtk_entry_new ();
  gtk_widget_show (s.scn_imagename);
  gtk_table_attach (GTK_TABLE (table), s.scn_imagename, 0, 3, 3, 4,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.scn_clrbackground = button = gtk_button_new_with_label ("");
  gtk_widget_show (button);
  gtk_table_attach (GTK_TABLE (table), button, 1, 2, 0, 1,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (button, 50, 30);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (preferencesdlg_color), opts->fBackground);
  gtk_signal_connect (GTK_OBJECT (button), "realize",
		      GTK_SIGNAL_FUNC(preferencesdlg_realize), NULL);
  gtk_object_set_data (GTK_OBJECT (button), "color_flt", opts->fBackground);

  s.scn_clrgrad1 = button = gtk_button_new_with_label ("");
  gtk_widget_show (button);
  gtk_table_attach (GTK_TABLE (table), button, 1, 2, 1, 2,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (button, 50, 30);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (preferencesdlg_color), opts->fGrad1);
  gtk_signal_connect (GTK_OBJECT (button), "realize",
		      GTK_SIGNAL_FUNC(preferencesdlg_realize), NULL);
  gtk_object_set_data (GTK_OBJECT (button), "color_flt", opts->fGrad1);

  s.scn_clrgrad2 = button = gtk_button_new_with_label ("");
  gtk_widget_show (button);
  gtk_table_attach (GTK_TABLE (table), button, 2, 3, 1, 2,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (button, 50, 30);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (preferencesdlg_color), opts->fGrad2);
  gtk_signal_connect (GTK_OBJECT (button), "realize",
		      GTK_SIGNAL_FUNC(preferencesdlg_realize), NULL);
  gtk_object_set_data (GTK_OBJECT (button), "color_flt", opts->fGrad2);

  s.scn_tile = gtk_check_button_new_with_label ("Tile");
  gtk_widget_show (s.scn_tile);
  gtk_table_attach (GTK_TABLE (table), s.scn_tile, 1, 2, 2, 3,
                    (GtkAttachOptions) GTK_FILL, (GtkAttachOptions) 0, 0, 0);

  frame = gtk_frame_new ("Environment");
  gtk_widget_show (frame);
  gtk_box_pack_end (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_set_usize (frame, 175, -2);

  table = gtk_table_new (5, 3, TRUE);
  gtk_widget_show (table);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_container_border_width (GTK_CONTAINER (table), 5);

  s.scn_fog = gtk_check_button_new_with_label ("Fog");
  gtk_widget_show (s.scn_fog);
  gtk_table_attach (GTK_TABLE (table), s.scn_fog, 0, 3, 0, 1,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  label = gtk_label_new ("Color");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  label = gtk_label_new ("Density");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 2, 3,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (label), 1.93715e-07, 0.5);

  label = gtk_label_new ("Ambient light");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 0, 2, 3, 4,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  s.scn_floor = gtk_check_button_new_with_label ("Draw floor");
  gtk_widget_show (s.scn_floor);
  gtk_table_attach (GTK_TABLE (table), s.scn_floor, 0, 3, 4, 5,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  s.scn_clrfog = button = gtk_button_new_with_label ("");
  gtk_widget_show (button);
  gtk_table_attach (GTK_TABLE (table), button, 2, 3, 1, 2,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (button, 50, 30);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (preferencesdlg_color), opts->fFog);
  gtk_signal_connect (GTK_OBJECT (button), "realize",
		      GTK_SIGNAL_FUNC(preferencesdlg_realize), NULL);
  gtk_object_set_data (GTK_OBJECT (button), "color_flt", opts->fFog);

  s.scn_clrambient = button = gtk_button_new_with_label ("");
  gtk_widget_show (button);
  gtk_table_attach (GTK_TABLE (table), button, 2, 3, 3, 4,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (button, 50, 30);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (preferencesdlg_color), opts->fAmbient);
  gtk_signal_connect (GTK_OBJECT (button), "realize",
		      GTK_SIGNAL_FUNC(preferencesdlg_realize), NULL);
  gtk_object_set_data (GTK_OBJECT (button), "color_flt", opts->fAmbient);

  s.scn_density = gtk_entry_new ();
  gtk_widget_show (s.scn_density);
  gtk_table_attach (GTK_TABLE (table), s.scn_density, 2, 3, 2, 3,
                    (GtkAttachOptions) 0, (GtkAttachOptions) 0, 0, 0);
  gtk_widget_set_usize (s.scn_density, 50, -2);

  label = gtk_label_new ("General");
  gtk_widget_show (label);
  set_notebook_tab (notebook, 0, label);

  label = gtk_label_new ("Details");
  gtk_widget_show (label);
  set_notebook_tab (notebook, 1, label);

  label = gtk_label_new ("Drawing Aids");
  gtk_widget_show (label);
  set_notebook_tab (notebook, 2, label);

  label = gtk_label_new ("Scene");
  gtk_widget_show (label);
  set_notebook_tab (notebook, 3, label);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, TRUE, 0);

  button = gtk_button_new_with_label ("OK");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (preferencesdlg_ok), &s);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_usize (button, 80, 25);
  GtkAccelGroup *accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (dlg), accel_group);
  gtk_widget_add_accelerator (button, "clicked", accel_group,
                              GDK_Return, 0, GTK_ACCEL_VISIBLE);

  button = gtk_button_new_with_label ("Cancel");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (dlg_default_callback), GINT_TO_POINTER (LC_CANCEL));
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_usize (button, 80, 25);
  gtk_widget_add_accelerator (button, "clicked", accel_group,
                              GDK_Escape, 0, GTK_ACCEL_VISIBLE);

  button = gtk_button_new_with_label ("Make Default");
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_usize (button, -2, 25);
  gtk_widget_set_sensitive (button, FALSE);

  // Set initial values
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.det_edges),
			       (opts->nDetail & LC_DET_BRICKEDGES) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.det_dither),
			       (opts->nDetail & LC_DET_DITHER) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.det_lighting),
			       (opts->nDetail & LC_DET_LIGHTING) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.det_smooth),
			       (opts->nDetail & LC_DET_SMOOTH) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.det_antialias),
			       (opts->nDetail & LC_DET_ANTIALIAS) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.det_linear),
			       (opts->nDetail & LC_DET_LINEAR) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.det_screen),
			       (opts->nDetail & LC_DET_SCREENDOOR) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.det_fast),
			       (opts->nDetail & LC_DET_FAST) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.det_solid),
			       (opts->nDetail & LC_DET_BOX_FILL) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.det_hidden),
			       (opts->nDetail & LC_DET_HIDDEN_LINE) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.det_background),
			       (opts->nDetail & LC_DET_BACKGROUND) ? TRUE : FALSE);
  write_float(s.det_width, opts->fLineWidth);

  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.draw_grid),
			       (opts->nSnap & LC_DRAW_GRID) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.draw_axis),
			       (opts->nSnap & LC_DRAW_AXIS) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.draw_preview),
			       (opts->nSnap & LC_DRAW_PREVIEW) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.draw_snapx),
			       (opts->nSnap & LC_DRAW_SNAP_X) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.draw_snapy),
			       (opts->nSnap & LC_DRAW_SNAP_Y) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.draw_snapz),
			       (opts->nSnap & LC_DRAW_SNAP_Z) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.draw_angle),
			       (opts->nSnap & LC_DRAW_SNAP_A) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.draw_centimeter),
			       (opts->nSnap & LC_DRAW_CM_UNITS) ? TRUE : FALSE);
  //  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.draw_collision),
  //			       (opts->nSnap & LC_DRAW_COLLISION) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.draw_move),
			       (opts->nSnap & LC_DRAW_MOVE) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.draw_fixed),
			       (opts->nSnap & LC_DRAW_MOVEAXIS) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.draw_lockx),
			       (opts->nSnap & LC_DRAW_LOCK_X) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.draw_locky),
			       (opts->nSnap & LC_DRAW_LOCK_Y) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.draw_lockz),
			       (opts->nSnap & LC_DRAW_LOCK_Z) ? TRUE : FALSE);
  write_int(s.draw_gridunits, opts->nGridSize);
  write_int(s.draw_anglesnap, opts->nAngleSnap);

  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.scn_gradient),
			       (opts->nScene & LC_SCENE_GRADIENT) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.scn_image),
			       (opts->nScene & LC_SCENE_BG) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.scn_tile),
			       (opts->nScene & LC_SCENE_BG_TILE) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.scn_fog),
			       (opts->nScene & LC_SCENE_FOG) ? TRUE : FALSE);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (s.scn_floor),
			       (opts->nScene & LC_SCENE_FLOOR) ? TRUE : FALSE);
  gtk_entry_set_text (GTK_ENTRY (s.scn_imagename), opts->strBackground);
  write_int(s.scn_density, (int)(opts->fDensity*100));

  return dlg_domodal(dlg, LC_CANCEL);
}

// Properties Dialog

typedef struct
{
  void* data;
  GtkWidget *sum_author, *sum_description, *sum_comments;
} LC_PROPERTIESDLG_STRUCT;

static void propertiesdlg_ok(GtkWidget *widget, gpointer data)
{
  LC_PROPERTIESDLG_STRUCT* s = (LC_PROPERTIESDLG_STRUCT*)data;
  LC_PROPERTIESDLG_OPTS* opts = (LC_PROPERTIESDLG_OPTS*)s->data;

  strcpy(opts->strAuthor, gtk_entry_get_text (GTK_ENTRY (s->sum_author)));
  strcpy(opts->strDescription, gtk_entry_get_text (GTK_ENTRY (s->sum_description)));
  char* comments = gtk_editable_get_chars(GTK_EDITABLE(s->sum_comments), 0, -1);
  strcpy(opts->strComments, comments);
  g_free(comments);

  *cur_ret = LC_OK;
}

int propertiesdlg_execute(void* param)
{
  GtkWidget *dlg;
  GtkWidget *vbox1, *vbox2, *hbox;
  GtkWidget *label, *button, *table, *notebook, *list, *scroll_win;
  LC_PROPERTIESDLG_STRUCT s;
  LC_PROPERTIESDLG_OPTS* opts = (LC_PROPERTIESDLG_OPTS*)param;
  s.data = param;

  struct stat buf;
  bool exist = (stat(opts->strFilename, &buf) != -1);
  char* ptr = strrchr(opts->strFilename, '/');
  char text[512];
  strcpy(text, opts->strTitle);
  strcat(text, " Properties");

  dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
		      GTK_SIGNAL_FUNC (dlg_delete_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
  gtk_widget_set_usize (dlg, 450, 280);
  gtk_window_set_title (GTK_WINDOW (dlg), text);
  gtk_window_set_policy (GTK_WINDOW (dlg), FALSE, FALSE, FALSE);
  gtk_widget_realize (dlg);

  vbox1 = gtk_vbox_new (FALSE, 7);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (dlg), vbox1);
  gtk_container_border_width (GTK_CONTAINER (vbox1), 7);

  notebook = gtk_notebook_new ();
  gtk_widget_show (notebook);
  gtk_box_pack_start (GTK_BOX (vbox1), notebook, TRUE, TRUE, 0);

  table = gtk_table_new (6, 2, FALSE);
  gtk_widget_show (table);
  gtk_container_add (GTK_CONTAINER (notebook), table);
  gtk_container_border_width (GTK_CONTAINER (table), 15);
  gtk_table_set_col_spacings (GTK_TABLE (table), 40);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 0, 1,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  label = gtk_label_new (ptr ? ptr+1 : "(not saved)");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  if (ptr)
  {
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 1, 2,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);
    *ptr = 0;
    label = gtk_label_new (opts->strFilename);
    *ptr = '/';

    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  }

  if (exist)
  {
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 2, 3,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

    sprintf(text, "%.1fKB (%d bytes)", (float)buf.st_size/1024, (int)buf.st_size);
    label = gtk_label_new (text);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 3, 4,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

    struct passwd *pwd = getpwuid(buf.st_uid);
    sprintf(text, "%s (%s)", pwd->pw_name, pwd->pw_gecos);
  
    label = gtk_label_new (text);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 4, 5,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

    strcpy(text, ctime(&buf.st_mtime));
    while (text[strlen(text)-1] < '0')
      text[strlen(text)-1] = 0;
    label = gtk_label_new (text);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, 5, 6,
                    (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

    strcpy(text, ctime(&buf.st_atime));
    while (text[strlen(text)-1] < '0')
      text[strlen(text)-1] = 0;
    label = gtk_label_new (text);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  }

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 0, 1,
                    (GtkAttachOptions)GTK_FILL, (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  label = gtk_label_new ("File name:");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 1, 2,
                    (GtkAttachOptions) GTK_FILL, (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  label = gtk_label_new ("Location:");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 2, 3,
                    (GtkAttachOptions) GTK_FILL, (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  label = gtk_label_new ("Size:");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 3, 4,
                    (GtkAttachOptions) GTK_FILL, (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  label = gtk_label_new ("Owner:");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 4, 5,
                    (GtkAttachOptions) GTK_FILL, (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  label = gtk_label_new ("Modified:");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_table_attach (GTK_TABLE (table), hbox, 0, 1, 5, 6,
                    (GtkAttachOptions) GTK_FILL, (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

  label = gtk_label_new ("Accessed:");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  vbox2 = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (notebook), vbox2);
  gtk_container_border_width (GTK_CONTAINER (vbox2), 5);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, TRUE, 0);

  label = gtk_label_new ("Author");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  s.sum_author = gtk_entry_new ();
  gtk_widget_show (s.sum_author);
  gtk_box_pack_start (GTK_BOX (vbox2), s.sum_author, FALSE, FALSE, 0);
  gtk_entry_set_text (GTK_ENTRY (s.sum_author), opts->strAuthor);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

  label = gtk_label_new ("Description");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  s.sum_description = gtk_entry_new ();
  gtk_widget_show (s.sum_description);
  gtk_box_pack_start (GTK_BOX (vbox2), s.sum_description, FALSE, FALSE, 0);
  gtk_entry_set_text (GTK_ENTRY (s.sum_description), opts->strDescription);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

  label = gtk_label_new ("Comments");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  s.sum_comments = gtk_text_new (NULL, NULL);
  gtk_widget_show (s.sum_comments);
  gtk_box_pack_start (GTK_BOX (vbox2), s.sum_comments, TRUE, TRUE, 0);
  gtk_text_set_editable (GTK_TEXT (s.sum_comments), TRUE);
  gtk_widget_realize (s.sum_comments);
  gtk_text_insert (GTK_TEXT (s.sum_comments), NULL, NULL, NULL,
                   opts->strComments, strlen(opts->strComments));

  int i, j, col[LC_MAXCOLORS], totalcount[LC_MAXCOLORS];
  memset (&totalcount, 0, sizeof (totalcount));
  for (i = 0; i < opts->lines; i++)
    for (j = 0; j < LC_MAXCOLORS; j++)
      totalcount[j] += opts->count[i*LC_MAXCOLORS+j];

  int ID = 2;
  for (i = 0; i < LC_MAXCOLORS; i++)
    if (totalcount[i])
      ID++;

  scroll_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scroll_win), 
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_show (scroll_win);
  gtk_container_add (GTK_CONTAINER (notebook), scroll_win);

  list = gtk_clist_new (ID);
  gtk_widget_show (list);
  gtk_container_add (GTK_CONTAINER (scroll_win), list);
  gtk_container_border_width (GTK_CONTAINER (list), 5);
  gtk_clist_set_column_width (GTK_CLIST (list), 0, 80);
  gtk_clist_column_titles_show (GTK_CLIST (list));

  label = gtk_label_new ("Piece");
  gtk_widget_show (label);
  gtk_clist_set_column_widget (GTK_CLIST (list), 0, label);

  for (ID = 0, i = 0; i < LC_MAXCOLORS; i++)
    if (totalcount[i])
    {
      ID++;
      col[i] = ID;

      label = gtk_label_new (colornames[i]);
      gtk_widget_show (label);
      gtk_clist_set_column_widget (GTK_CLIST (list), ID, label);
    }
  ID++;
  label = gtk_label_new ("Total");
  gtk_widget_show (label);
  gtk_clist_set_column_widget (GTK_CLIST (list), ID, label);

  char* row[LC_MAXCOLORS+2];
  for (i = 1; i <= ID; i++)
    row[i] = (char*)malloc(65);

  for (i = 0; i < opts->lines; i++)
  {
    int total = 0;

    for (j = 0; j < LC_MAXCOLORS; j++)
      total += opts->count[i*LC_MAXCOLORS+j];

    if (total == 0)
      continue;

    row[0] = opts->names[i];
    for (j = 1; j < ID; j++)
      row[j][0] = 0;

    for (j = 0; j < LC_MAXCOLORS; j++)
      if (opts->count[i*LC_MAXCOLORS+j])
	sprintf (row[col[j]], "%d", opts->count[i*LC_MAXCOLORS+j]);

    sprintf (row[ID], "%d", total);
    gtk_clist_append (GTK_CLIST(list), row);
  }
  gtk_clist_sort (GTK_CLIST(list));

  row[0] = "Total";
  int total = 0;

  for (i = 0; i < LC_MAXCOLORS; i++)
    if (totalcount[i])
    {
      sprintf (row[col[i]], "%d", totalcount[i]);
      total += totalcount[i];
    }

  sprintf (row[ID], "%d", total);
  gtk_clist_append (GTK_CLIST(list), row);

  for (i = 1; i <= ID; i++)
    free(row[i]);

  label = gtk_label_new ("General");
  gtk_widget_show (label);
  set_notebook_tab (notebook, 0, label);

  label = gtk_label_new ("Summary");
  gtk_widget_show (label);
  set_notebook_tab (notebook, 1, label);

  label = gtk_label_new ("Pieces Used");
  gtk_widget_show (label);
  set_notebook_tab (notebook, 2, label);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, FALSE, 5);

  button = gtk_button_new_with_label ("Cancel");
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (dlg_default_callback), GINT_TO_POINTER (LC_CANCEL));
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 5);
  gtk_widget_set_usize (button, 70, 25);
  GtkAccelGroup *accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (dlg), accel_group);
  gtk_widget_add_accelerator (button, "clicked", accel_group,
                              GDK_Escape, 0, GTK_ACCEL_VISIBLE);

  button = gtk_button_new_with_label ("OK");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (propertiesdlg_ok), &s);
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_usize (button, 70, 25);
  gtk_widget_add_accelerator (button, "clicked", accel_group,
                              GDK_Return, 0, GTK_ACCEL_VISIBLE);

  return dlg_domodal(dlg, LC_CANCEL);
}

// =========================================================

// 'Edit Groups' dialog

typedef struct
{
  void* data;

} LC_GROUPEDITDLG_STRUCT;

static void groupeditdlg_ok(GtkWidget *widget, gpointer data)
{
  //  LC_GROUPEDITDLG_STRUCT* s = (LC_GROUPEDITDLG_STRUCT*)data;
  //  LC_GROUPEDITDLG_OPTS* opts = (LC_GROUPEDITDLG_OPTS*)s->data;

  *cur_ret = LC_OK;
}

void groupeditdlg_addchildren(GtkWidget *tree, Group *pGroup, LC_GROUPEDITDLG_OPTS *opts)
{
  int i;
  GtkWidget *item, *subtree;

  // Add the groups
  for (i = 0; i < opts->groupcount; i++)
    if (opts->groupsgroups[i] == pGroup)
    {
      item = gtk_tree_item_new_with_label (opts->groups[i]->m_strName);
      /*
      tvstruct.item.lParam = i + 0xFFFF;
      tvstruct.item.iImage = 0;
      tvstruct.item.iSelectedImage = 1;
      tvstruct.item.pszText = opts->groups[i]->m_strName;
      tvstruct.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
      */

      gtk_tree_append (GTK_TREE(tree), item);
      gtk_widget_show (item);

      subtree = gtk_tree_new();
      gtk_tree_set_selection_mode (GTK_TREE(subtree), GTK_SELECTION_SINGLE);
      gtk_tree_set_view_mode (GTK_TREE(subtree), GTK_TREE_VIEW_ITEM);
      gtk_tree_item_set_subtree (GTK_TREE_ITEM(item), subtree);

      groupeditdlg_addchildren(subtree, opts->groups[i], opts);
    }

  // Add the pieces
  for (i = 0; i < opts->piececount; i++)
    if (opts->piecesgroups[i] == pGroup)
    {
      /*
      tvstruct.item.lParam = i;
      tvstruct.item.iImage = 2;
      tvstruct.item.iSelectedImage = 2;
      tvstruct.item.pszText = (char*)opts->pieces[i]->GetName();
      tvstruct.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
      */
      item = gtk_tree_item_new_with_label ((char*)opts->pieces[i]->GetName());
      gtk_tree_append (GTK_TREE(tree), item);
      gtk_widget_show (item);
    }
}

int groupeditdlg_execute(void* param)
{
  GtkWidget *dlg;
  GtkWidget *vbox, *hbox;
  GtkWidget *button, *tree, *scrolled_win;
  LC_GROUPEDITDLG_STRUCT s;
  s.data = param;

  dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
		      GTK_SIGNAL_FUNC (dlg_delete_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
  gtk_widget_set_usize (dlg, 450, 280);
  gtk_window_set_title (GTK_WINDOW (dlg), "Edit Groups");
  gtk_window_set_policy (GTK_WINDOW (dlg), FALSE, FALSE, FALSE);
  gtk_widget_realize (dlg);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (dlg), vbox);
  gtk_container_border_width (GTK_CONTAINER (vbox), 5);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_usize (scrolled_win, 150, 200);
  gtk_container_add (GTK_CONTAINER(vbox), scrolled_win);
  gtk_widget_show (scrolled_win);

  tree = gtk_tree_new ();
  gtk_widget_show (tree);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(scrolled_win), tree);
  gtk_container_set_border_width (GTK_CONTAINER (tree), 5);
  gtk_tree_set_selection_mode (GTK_TREE (tree), GTK_SELECTION_BROWSE);

  hbox = gtk_hbox_new (FALSE, 10);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);

  button = gtk_button_new_with_label ("OK");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (groupeditdlg_ok), &s);
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

  groupeditdlg_addchildren(tree, NULL, (LC_GROUPEDITDLG_OPTS*)param);

  return dlg_domodal(dlg, LC_CANCEL);
}

// =========================================================

// Group Dialog

typedef struct
{
  void* data;
  GtkWidget* entry;
} LC_GROUPDLG_STRUCT;

static void groupdlg_ok(GtkWidget *widget, gpointer data)
{
  LC_GROUPDLG_STRUCT* s = (LC_GROUPDLG_STRUCT*)data;
  char* name = (char*)s->data;

  strcpy (name, gtk_entry_get_text (GTK_ENTRY (s->entry)));

  *cur_ret = LC_OK;
}

int groupdlg_execute(void* param)
{
  GtkWidget *dlg;
  GtkWidget *vbox, *hbox;
  GtkWidget *button;
  LC_GROUPDLG_STRUCT s;
  s.data = param;

  dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
		      GTK_SIGNAL_FUNC (dlg_delete_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
  gtk_widget_set_usize (dlg, 250, 100);
  gtk_window_set_title (GTK_WINDOW (dlg), "Group Name");
  gtk_window_set_policy (GTK_WINDOW (dlg), FALSE, FALSE, FALSE);
  gtk_widget_realize (dlg);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (dlg), vbox);
  gtk_container_border_width (GTK_CONTAINER (vbox), 5);

  s.entry =  gtk_entry_new_with_max_length (64);
  gtk_widget_show (s.entry);
  gtk_box_pack_start (GTK_BOX (vbox), s.entry, TRUE, FALSE, 0);
  gtk_widget_set_usize (s.entry, 50, -2);
  gtk_entry_set_text (GTK_ENTRY (s.entry), (char*)param);

  hbox = gtk_hbox_new (FALSE, 10);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);

  button = gtk_button_new_with_label ("OK");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (groupdlg_ok), &s);
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

  return dlg_domodal(dlg, LC_CANCEL);
}





