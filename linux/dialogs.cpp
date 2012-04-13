#include "lc_global.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include "opengl.h"
#include "gtkmisc.h"
#include "gtktools.h"
#include "system.h"
#include "dialogs.h"
#include "typedefs.h"
#include "globals.h"
#include "piece.h"
#include "group.h"
#include "main.h"
#include "message.h"
#include "project.h"
#include "libdlg.h"

// =============================================================================
// Helper functions

static bool read_float(GtkWidget* widget, float* value, float min_value, float max_value)
{
	char buf[256];

	strcpy(buf, gtk_entry_get_text(GTK_ENTRY(widget)));
	if (sscanf(buf, "%f", value) == 1)
	{
		if (*value >= min_value && *value <= max_value)
			return true;
	}

	sprintf(buf, "Please enter a value between %g and %g", min_value, max_value);
	msgbox_execute(buf, "LeoCAD", LC_MB_OK | LC_MB_ICONERROR);
	gtk_window_set_focus(GTK_WINDOW(gtk_widget_get_toplevel(widget)), widget);

	return false;
}

static void write_float(GtkWidget* widget, float value)
{
	char buf[16];
	sprintf(buf, "%g", value);
	gtk_entry_set_text(GTK_ENTRY(widget), buf);
}

static bool read_int(GtkWidget* widget, int* value, int min_value, int max_value)
{
	char buf[256];

	strcpy(buf, gtk_entry_get_text(GTK_ENTRY(widget)));
	if (sscanf(buf, "%d", value) == 1)
	{
		if (*value >= min_value && *value <= max_value)
			return true;
	}

	sprintf(buf, "Please enter a value between %d and %d", min_value, max_value);
	msgbox_execute(buf, "LeoCAD", LC_MB_OK | LC_MB_ICONERROR);
	gtk_window_set_focus(GTK_WINDOW(gtk_widget_get_toplevel(widget)), widget);

	return false;
}

static void write_int(GtkWidget* widget, int value)
{
	char buf[16];
	sprintf(buf, "%d", value);
	gtk_entry_set_text(GTK_ENTRY(widget), buf);
}

// =============================================================================
// MessageBox

int msgbox_execute(const char* text, const char *caption, int flags)
{
	GtkWidget* dlg;
	GtkMessageType Message;

	switch (flags & LC_MB_ICONMASK)
	{
	case LC_MB_ICONERROR:
		Message = GTK_MESSAGE_ERROR;
		break;
	case LC_MB_ICONQUESTION:
		Message = GTK_MESSAGE_QUESTION;
		break;
	case LC_MB_ICONWARNING:
		Message = GTK_MESSAGE_WARNING;
		break;
	default:
	case LC_MB_ICONINFORMATION:
		Message = GTK_MESSAGE_INFO;
		break;
	}

	dlg = gtk_message_dialog_new(GTK_WINDOW(((GtkWidget*)(*main_window))),
	                             GTK_DIALOG_MODAL, Message, GTK_BUTTONS_NONE, "%s", text);
	gtk_window_set_title(GTK_WINDOW(dlg), caption);

	switch (flags & LC_MB_TYPEMASK)
	{
	default:
	case LC_MB_OK:
		gtk_dialog_add_button(GTK_DIALOG(dlg), GTK_STOCK_OK, GTK_RESPONSE_OK);
		break;
	case LC_MB_OKCANCEL:
		gtk_dialog_add_button(GTK_DIALOG(dlg), GTK_STOCK_OK, GTK_RESPONSE_OK);
		gtk_dialog_add_button(GTK_DIALOG(dlg), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
		break;
	case LC_MB_YESNOCANCEL:
		gtk_dialog_add_button(GTK_DIALOG(dlg), GTK_STOCK_YES, GTK_RESPONSE_YES);
		gtk_dialog_add_button(GTK_DIALOG(dlg), GTK_STOCK_NO, GTK_RESPONSE_NO);
		gtk_dialog_add_button(GTK_DIALOG(dlg), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
		break;
	case LC_MB_YESNO:
		gtk_dialog_add_button(GTK_DIALOG(dlg), GTK_STOCK_YES, GTK_RESPONSE_YES);
		gtk_dialog_add_button(GTK_DIALOG(dlg), GTK_STOCK_NO, GTK_RESPONSE_NO);
		break;
	}

	gint result = gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);

	switch (result)
	{
	case GTK_RESPONSE_OK: return LC_OK;
	case GTK_RESPONSE_CANCEL: return LC_CANCEL;
	case GTK_RESPONSE_YES: return LC_YES;
	case GTK_RESPONSE_NO: return LC_NO;
	}

	switch (flags & LC_MB_TYPEMASK)
	{
	default:
	case LC_MB_OK: return LC_OK;
	case LC_MB_OKCANCEL: return LC_CANCEL;
	case LC_MB_YESNOCANCEL: return LC_CANCEL;
	case LC_MB_YESNO: return LC_NO;
	}
}

// =============================================================================
// TODO: deprecated, delete

void dialog_button_callback (GtkWidget *widget, gpointer data)
{
  GtkWidget *parent;
  int *loop, *ret;

  parent = gtk_widget_get_toplevel (widget);
  loop = (int*)gtk_object_get_data (GTK_OBJECT (parent), "loop");
  ret = (int*)gtk_object_get_data (GTK_OBJECT (parent), "ret");

  *loop = 0;
  *ret = GPOINTER_TO_INT (data);
}

gint dialog_delete_callback (GtkWidget *widget, GdkEvent* event, gpointer data)
{
  int *loop;

  gtk_widget_hide (widget);
  loop = (int*)gtk_object_get_data (GTK_OBJECT (widget), "loop");
  *loop = 0;

  return TRUE;
}

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
  *cur_ret = GPOINTER_TO_INT(data);
}

gint dlg_delete_callback(GtkWidget *widget, GdkEvent* event, gpointer data)
{
  *cur_ret = def_ret;
  return TRUE;
}

// =============================================================================
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

static void arraydlg_adjtotal(GtkWidget *widget, gpointer data)
{
	LC_ARRAYDLG_STRUCT* s = (LC_ARRAYDLG_STRUCT*)data;
	LC_ARRAYDLG_OPTS* opts = (LC_ARRAYDLG_OPTS*)s->data;
	char ctot[16];

	if (GTK_TOGGLE_BUTTON(s->radio1)->active)
		opts->nArrayDimension = 0;
	else if (GTK_TOGGLE_BUTTON(s->radio2)->active)
		opts->nArrayDimension = 1;
	else if (GTK_TOGGLE_BUTTON(s->radio3)->active)
		opts->nArrayDimension = 2;

	gint tot = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s->count1)) *
	           ((opts->nArrayDimension > 0) ? gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s->count2)) : 1) *
	           ((opts->nArrayDimension > 1) ? gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(s->count3)) : 1);

	sprintf(ctot, "%i", tot);

	gtk_entry_set_text(GTK_ENTRY(s->total), ctot);
}

static void arraydlg_radiotoggled(GtkWidget *widget, gpointer data)
{
	LC_ARRAYDLG_STRUCT* s = (LC_ARRAYDLG_STRUCT*)data;
	LC_ARRAYDLG_OPTS* opts = (LC_ARRAYDLG_OPTS*)s->data;

	if (GTK_TOGGLE_BUTTON(widget)->active == FALSE)
		return;

	arraydlg_adjtotal(widget, data);

	if (opts->nArrayDimension > 0)
	{
		gtk_widget_set_sensitive(s->count2, TRUE);
		gtk_widget_set_sensitive(s->offset_x2, TRUE);
		gtk_widget_set_sensitive(s->offset_y2, TRUE);
		gtk_widget_set_sensitive(s->offset_z2, TRUE);
	}
	else
	{
		gtk_widget_set_sensitive(s->count2, FALSE);
		gtk_widget_set_sensitive(s->offset_x2, FALSE);
		gtk_widget_set_sensitive(s->offset_y2, FALSE);
		gtk_widget_set_sensitive(s->offset_z2, FALSE);
	}

	if (opts->nArrayDimension > 1)
	{
		gtk_widget_set_sensitive(s->count3, TRUE);
		gtk_widget_set_sensitive(s->offset_x3, TRUE);
		gtk_widget_set_sensitive(s->offset_y3, TRUE);
		gtk_widget_set_sensitive(s->offset_z3, TRUE);
	}
	else
	{
		gtk_widget_set_sensitive(s->count3, FALSE);
		gtk_widget_set_sensitive(s->offset_x3, FALSE);
		gtk_widget_set_sensitive(s->offset_y3, FALSE);
		gtk_widget_set_sensitive(s->offset_z3, FALSE);
	}
}

int arraydlg_execute(void* param)
{
	GtkObject *adj;
	GtkWidget *dlg;
	GtkWidget *vbox1, *hbox1;
	GtkWidget *frame, *table, *label;
	GSList *radio_group = (GSList*)NULL;
	LC_ARRAYDLG_STRUCT s;
	s.data = param;
	int ret;

	dlg = gtk_dialog_new_with_buttons("Array Options", GTK_WINDOW(((GtkWidget*)(*main_window))),
	                                  (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
	                                  GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(dlg), 5);

	vbox1 = GTK_DIALOG(dlg)->vbox;
	gtk_box_set_spacing(GTK_BOX(vbox1), 10);

	hbox1 = gtk_hbox_new(FALSE, 10);
	gtk_widget_show(hbox1);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);

	frame = gtk_frame_new("Transformation (Incremental)");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(hbox1), frame, TRUE, TRUE, 0);

	table = gtk_table_new(3, 4, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_table_set_row_spacings(GTK_TABLE(table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table), 10);
	gtk_container_border_width(GTK_CONTAINER(table), 5);

	label = gtk_label_new("X");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 0, 1);

	label = gtk_label_new("Y");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, 0, 1);

	label = gtk_label_new("Z");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 3, 4, 0, 1);

	label = gtk_label_new("Move");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);

	adj = gtk_adjustment_new(0, -1000, 1000, 10, 10, 0);
	s.move_x = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(s.move_x);
	gtk_table_attach_defaults(GTK_TABLE(table), s.move_x, 1, 2, 1, 2);
	gtk_entry_set_width_chars(GTK_ENTRY(s.move_x), 4);
 
	adj = gtk_adjustment_new(0, -1000, 1000, 10, 10, 0);
	s.move_y = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(s.move_y);
	gtk_table_attach_defaults(GTK_TABLE(table), s.move_y, 2, 3, 1, 2);
	gtk_entry_set_width_chars(GTK_ENTRY(s.move_y), 4);

	adj = gtk_adjustment_new(0, -1000, 1000, 4, 12, 0);
	s.move_z = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(s.move_z);
	gtk_table_attach_defaults(GTK_TABLE(table), s.move_z, 3, 4, 1, 2);
	gtk_entry_set_width_chars(GTK_ENTRY(s.move_z), 4);

	label = gtk_label_new("Rotate");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);

	adj = gtk_adjustment_new(0, -180, 180, 1, 10, 0);
	s.rotate_x = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(s.rotate_x), TRUE);
	gtk_widget_show(s.rotate_x);
	gtk_table_attach_defaults(GTK_TABLE(table), s.rotate_x, 1, 2, 2, 3);
	gtk_entry_set_width_chars(GTK_ENTRY(s.rotate_x), 4);

	adj = gtk_adjustment_new(0, -180, 180, 1, 10, 0);
	s.rotate_y = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(s.rotate_y), TRUE);
	gtk_widget_show(s.rotate_y);
	gtk_table_attach_defaults(GTK_TABLE(table), s.rotate_y, 2, 3, 2, 3);
	gtk_entry_set_width_chars(GTK_ENTRY(s.rotate_y), 4);

	adj = gtk_adjustment_new(0, -180, 180, 1, 10, 0);
	s.rotate_z = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(s.rotate_z), TRUE);
	gtk_widget_show(s.rotate_z);
	gtk_table_attach_defaults(GTK_TABLE(table), s.rotate_z, 3, 4, 2, 3);
	gtk_entry_set_width_chars(GTK_ENTRY(s.rotate_x), 4);

	label = gtk_label_new("Total:");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox1), label, FALSE, TRUE, 0);

	s.total = gtk_entry_new();
	gtk_widget_show(s.total);
	gtk_box_pack_start(GTK_BOX(hbox1), s.total, FALSE, TRUE, 0);
	gtk_entry_set_width_chars(GTK_ENTRY(s.total), 4);
	gtk_entry_set_editable(GTK_ENTRY(s.total), FALSE);

	frame = gtk_frame_new("Dimensions");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox1), frame, TRUE, TRUE, 0);
	gtk_container_border_width(GTK_CONTAINER(frame), 10);

	table = gtk_table_new(4, 5, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 10);
	gtk_table_set_col_spacings(GTK_TABLE(table), 20);

	label = gtk_label_new("Count");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 0, 1);

	label = gtk_label_new("Offsets");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 3, 4, 0, 1);

	s.radio1 = gtk_radio_button_new_with_label(radio_group, "1D");
	radio_group = gtk_radio_button_group(GTK_RADIO_BUTTON(s.radio1));
	gtk_widget_show(s.radio1);
	gtk_table_attach_defaults(GTK_TABLE(table), s.radio1, 0, 1, 1, 2);
	gtk_signal_connect(GTK_OBJECT(s.radio1), "toggled", GTK_SIGNAL_FUNC(arraydlg_radiotoggled), &s);

	s.radio2 = gtk_radio_button_new_with_label(radio_group, "2D");
	radio_group = gtk_radio_button_group(GTK_RADIO_BUTTON(s.radio2));
	gtk_widget_show(s.radio2);
	gtk_table_attach_defaults(GTK_TABLE(table), s.radio2, 0, 1, 2, 3);
	gtk_signal_connect(GTK_OBJECT(s.radio2), "toggled", GTK_SIGNAL_FUNC(arraydlg_radiotoggled), &s);

	s.radio3 = gtk_radio_button_new_with_label(radio_group, "3D");
	radio_group = gtk_radio_button_group(GTK_RADIO_BUTTON(s.radio3));
	gtk_widget_show(s.radio3);
	gtk_table_attach_defaults(GTK_TABLE(table), s.radio3, 0, 1, 3, 4);
	gtk_signal_connect(GTK_OBJECT(s.radio3), "toggled", GTK_SIGNAL_FUNC(arraydlg_radiotoggled), &s);

	adj = gtk_adjustment_new(10, 1, 1000, 1, 10, 0);
	s.count1 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(s.count1);
	gtk_table_attach_defaults(GTK_TABLE(table), s.count1, 1, 2, 1, 2);
	gtk_entry_set_width_chars(GTK_ENTRY(s.count1), 4);
	gtk_signal_connect(GTK_OBJECT(adj), "value_changed", GTK_SIGNAL_FUNC(arraydlg_adjtotal), &s);

	adj = gtk_adjustment_new(1, 1, 1000, 1, 10, 0);
	s.count2 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(s.count2);
	gtk_table_attach_defaults(GTK_TABLE(table), s.count2, 1, 2, 2, 3);
	gtk_entry_set_width_chars(GTK_ENTRY(s.count2), 4);
	gtk_signal_connect(GTK_OBJECT(adj), "value_changed", GTK_SIGNAL_FUNC(arraydlg_adjtotal), &s);

	adj = gtk_adjustment_new(1, 1, 1000, 1, 10, 0);
	s.count3 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(s.count3);
	gtk_table_attach_defaults(GTK_TABLE(table), s.count3, 1, 2, 3, 4);
	gtk_entry_set_width_chars(GTK_ENTRY(s.count3), 4);
	gtk_signal_connect(GTK_OBJECT(adj), "value_changed", GTK_SIGNAL_FUNC(arraydlg_adjtotal), &s);

	label = gtk_label_new("X");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, 1, 2);

	label = gtk_label_new("Y");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 3, 4, 1, 2);

	label = gtk_label_new("Z");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 4, 5, 1, 2);

	adj = gtk_adjustment_new(0, -1000, 1000, 10, 10, 0);
	s.offset_x2 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(s.offset_x2);
	gtk_table_attach_defaults(GTK_TABLE(table), s.offset_x2, 2, 3, 2, 3);
	gtk_entry_set_width_chars(GTK_ENTRY(s.offset_x2), 4);

	adj = gtk_adjustment_new(0, -1000, 1000, 10, 10, 0);
	s.offset_y2 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(s.offset_y2);
	gtk_table_attach_defaults(GTK_TABLE(table), s.offset_y2, 3, 4, 2, 3);
	gtk_entry_set_width_chars(GTK_ENTRY(s.offset_y2), 4);

	adj = gtk_adjustment_new(0, -1000, 1000, 4, 12, 0);
	s.offset_z2 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(s.offset_z2);
	gtk_table_attach_defaults(GTK_TABLE(table), s.offset_z2, 4, 5, 2, 3);
	gtk_entry_set_width_chars(GTK_ENTRY(s.offset_z2), 4);

	adj = gtk_adjustment_new(0, -1000, 1000, 10, 10, 0);
	s.offset_x3 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(s.offset_x3);
	gtk_table_attach_defaults(GTK_TABLE(table), s.offset_x3, 2, 3, 3, 4);
	gtk_entry_set_width_chars(GTK_ENTRY(s.offset_x3), 4);

	adj = gtk_adjustment_new(0, -1000, 1000, 10, 10, 0);
	s.offset_y3 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(s.offset_y3);
	gtk_table_attach_defaults(GTK_TABLE(table), s.offset_y3, 3, 4, 3, 4);
	gtk_entry_set_width_chars(GTK_ENTRY(s.offset_y3), 4);

	adj = gtk_adjustment_new(0, -1000, 1000, 4, 12, 0);
	s.offset_z3 = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
	gtk_widget_show(s.offset_z3);
	gtk_table_attach_defaults(GTK_TABLE(table), s.offset_z3, 4, 5, 3, 4);
	gtk_entry_set_width_chars(GTK_ENTRY(s.offset_z3), 4);

	// Initialize dialog
	arraydlg_radiotoggled(s.radio1, &s);

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
	{
		ret = LC_OK;

		LC_ARRAYDLG_OPTS* opts = (LC_ARRAYDLG_OPTS*)s.data;

		if (!read_float(s.move_x, &opts->fMove[0], -1000, 1000) || !read_float(s.move_y, &opts->fMove[1], -1000, 1000) ||
		    !read_float(s.move_z, &opts->fMove[2], -1000, 1000) || !read_float(s.rotate_x, &opts->fRotate[0], -360, 360) ||
		    !read_float(s.rotate_y, &opts->fRotate[1], -360, 360) || !read_float(s.rotate_z, &opts->fRotate[2], -360, 360) ||
		    !read_float(s.offset_x2, &opts->f2D[0], -1000, 1000) || !read_float(s.offset_y2, &opts->f2D[1], -1000, 1000) ||
		    !read_float(s.offset_z2, &opts->f2D[2], -1000, 1000) || !read_float(s.offset_x3, &opts->f3D[0], -1000, 1000) ||
		    !read_float(s.offset_y3, &opts->f3D[1], -1000, 1000) || !read_float(s.offset_z3, &opts->f3D[2], -1000, 1000))
		    ret = LC_CANCEL;

		int Count1, Count2, Count3;

		if (!read_int(s.count1, &Count1, 1, 1000) || !read_int(s.count2, &Count2, 1, 1000) ||
		    !read_int(s.count3, &Count3, 1, 1000))
		    ret = LC_CANCEL;

		opts->n1DCount = Count1;
		opts->n2DCount = Count2;
		opts->n3DCount = Count3;

		if (GTK_TOGGLE_BUTTON (s.radio1)->active)
			opts->nArrayDimension = 0;
		else if (GTK_TOGGLE_BUTTON (s.radio2)->active)
			opts->nArrayDimension = 1;
		else if (GTK_TOGGLE_BUTTON (s.radio3)->active)
			opts->nArrayDimension = 2;
	}
	else
		ret = LC_CANCEL;

	gtk_widget_destroy(dlg);

	return ret;
}

// =============================================================================
// About Dialog

int aboutdlg_execute(void* param)
{
#include "pixmaps/icon32.xpm"
	GtkWidget *dlg, *vbox1, *vbox2, *hbox, *frame, *scr, *w;
	GtkTextBuffer *buffer;
	char info[512], buf[64];
	GLboolean valueb;
	GLint value;

	dlg = gtk_dialog_new_with_buttons("About LeoCAD", GTK_WINDOW(((GtkWidget*)(*main_window))),
	                                  (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
	                                  GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_has_separator(GTK_DIALOG(dlg), false);
	gtk_container_set_border_width(GTK_CONTAINER(dlg), 5);
	gtk_widget_realize(dlg);

	vbox1 = GTK_DIALOG(dlg)->vbox;
	gtk_box_set_spacing(GTK_BOX(vbox1), 10);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, TRUE, 0);

	w = new_pixmap(dlg, icon32);
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, TRUE, 5);
	gtk_widget_set_usize(w, 32, 32);

	vbox2 = gtk_vbox_new(TRUE, 0);
	gtk_widget_show(vbox2);
	gtk_box_pack_start(GTK_BOX(hbox), vbox2, TRUE, TRUE, 10);

	w = gtk_label_new("LeoCAD for "LC_VERSION_OSNAME" Version "LC_VERSION_TEXT);
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vbox2), w, FALSE, FALSE, 5);

	w = gtk_label_new("Copyright (c) 1996-2012, BT Software");
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(vbox2), w, FALSE, FALSE, 5);

	w = gtk_label_new(NULL);
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, TRUE, 5);
	gtk_widget_set_usize(w, 32, 32);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, TRUE, 0);

	w = gtk_link_button_new_with_label("http://www.leocad.org", "http://www.leocad.org");
	gtk_widget_show(w);
	gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, FALSE, 0);

	frame = gtk_frame_new("System Information");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox1), frame, TRUE, TRUE, 0);
	gtk_widget_set_usize(frame, -2, 150);

	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox2);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	gtk_container_border_width(GTK_CONTAINER(vbox2), 5);

	strcpy(info, "OpenGL Version ");
	strcat(info, (const char*)glGetString(GL_VERSION));
	strcat(info, "\n");
	strcat(info, (const char*)glGetString(GL_RENDERER));
	strcat(info, " - ");
	strcat(info, (const char*)glGetString(GL_VENDOR));

	strcat(info, "\n\nDisplay info:\n");
	glGetIntegerv(GL_INDEX_BITS, &value);
	sprintf(buf, "Bits per pixel: %i", value);
	strcat(info, buf);
	glGetIntegerv(GL_RED_BITS, &value);
	sprintf(buf, " (%i", value);
	strcat(info, buf);
	glGetIntegerv(GL_GREEN_BITS, &value);
	sprintf(buf, "-%i", value);
	strcat(info, buf);
	glGetIntegerv(GL_BLUE_BITS, &value);
	sprintf(buf, "-%i", value);
	strcat(info, buf);
	glGetIntegerv(GL_ALPHA_BITS, &value);
	sprintf(buf, "-%i)\n", value);
	strcat(info, buf);
	glGetIntegerv(GL_DEPTH_BITS, &value);
	sprintf(buf, "Depth buffer bits: %i\n", value);
	strcat(info, buf);
	glGetIntegerv(GL_STENCIL_BITS, &value);
	sprintf(buf, "Stencil bits: %i\n", value);
	strcat(info, buf);
	glGetIntegerv(GL_AUX_BUFFERS, &value);
	sprintf(buf, "Auxillary buffers: %i\n", value);
	strcat(info, buf);
	glGetBooleanv(GL_STEREO, &valueb);
	strcat(info, "Stereoscopic viewing: ");
	if (valueb == GL_TRUE)
		strcat(info, "Yes.\n");
	else
		strcat(info, "No.\n");

	strcat(info, "Vertex Buffer Objects: ");
	if (GL_HasVertexBufferObject())
		strcat(info, "Supported.\n");
	else
		strcat(info, "Not supported.\n");

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
	sprintf(buf, "Maximum texture size: %ix%i", value, value);
	strcat(info, buf);

	scr = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scr);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scr), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox2), scr, TRUE, TRUE, 0);

	w = gtk_text_view_new();
	gtk_widget_show(w);
	gtk_container_add(GTK_CONTAINER(scr), w);

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
	gtk_text_buffer_set_text(buffer, info, -1);

	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);

	return LC_OK;
}

// =============================================================================
// HTML Dialog

struct LC_HTMLDLG_STRUCT
{
	void* data;
	GtkWidget *dlg;
	GtkWidget *single, *multiple, *index;
	GtkWidget *list_end, *list_step, *images;
	GtkWidget *highlight, *htmlext, *directory;
};

static void htmldlg_images(GtkWidget *widget, gpointer data)
{
	LC_HTMLDLG_STRUCT* s = (LC_HTMLDLG_STRUCT*)data;
	LC_HTMLDLG_OPTS* opts = (LC_HTMLDLG_OPTS*)s->data;

	imageoptsdlg_execute(s->dlg, &opts->imdlg ,true);
}

static void htmldlg_layout(GtkWidget *widget, gpointer data)
{
	LC_HTMLDLG_STRUCT* s = (LC_HTMLDLG_STRUCT*)data;

	gtk_widget_set_sensitive(s->index, !GTK_TOGGLE_BUTTON(s->single)->active);
}

static void htmldlg_list (GtkWidget *widget, gpointer data)
{
	LC_HTMLDLG_STRUCT* s = (LC_HTMLDLG_STRUCT*)data;

	gtk_widget_set_sensitive(s->images, GTK_TOGGLE_BUTTON(s->list_end)->active || GTK_TOGGLE_BUTTON(s->list_step)->active);
}

static void htmldlg_browse_output(GtkWidget *widget, gpointer data)
{
	LC_HTMLDLG_STRUCT* s = (LC_HTMLDLG_STRUCT*)data;
	GtkWidget* dlg;

	dlg = gtk_file_chooser_dialog_new("Select output path", GTK_WINDOW(s->dlg), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
	                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), gtk_entry_get_text(GTK_ENTRY(s->directory)));

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
	{
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
		gtk_entry_set_text(GTK_ENTRY(s->directory), filename);
		g_free(filename);
	}

	gtk_widget_destroy(dlg);
}

int htmldlg_execute(void* param)
{
	GtkWidget *dlg;
	GtkWidget *vbox, *vbox2, *hbox;
	GtkWidget *frame, *label, *button;
	GSList *radio_group = (GSList*)NULL;
	LC_HTMLDLG_STRUCT s;
	LC_HTMLDLG_OPTS* opts = (LC_HTMLDLG_OPTS*)param;
	s.data = param;
	int ret;

	dlg = gtk_dialog_new_with_buttons("HTML Options", GTK_WINDOW(((GtkWidget*)(*main_window))),
	                                  (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
	                                  GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(dlg), 5);
	s.dlg = dlg;

	vbox = GTK_DIALOG(dlg)->vbox;
	gtk_box_set_spacing(GTK_BOX(vbox), 10);

	frame = gtk_frame_new("Layout");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox2);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	gtk_container_border_width(GTK_CONTAINER(vbox2), 5);

	s.single = gtk_radio_button_new_with_label(radio_group, "Single page");
	radio_group = gtk_radio_button_group(GTK_RADIO_BUTTON(s.single));
	gtk_widget_show(s.single);
	gtk_box_pack_start(GTK_BOX(vbox2), s.single, TRUE, TRUE, 0);

	s.multiple = gtk_radio_button_new_with_label(radio_group, "One step per page");
	radio_group = gtk_radio_button_group(GTK_RADIO_BUTTON(s.multiple));
	gtk_widget_show(s.multiple);
	gtk_box_pack_start(GTK_BOX(vbox2), s.multiple, TRUE, TRUE, 0);

	s.index = gtk_check_button_new_with_label("Index page");
	gtk_widget_show(s.index);
	gtk_box_pack_start(GTK_BOX(vbox2), s.index, TRUE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, TRUE, 0);

	button = gtk_button_new_with_label("Images...");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(htmldlg_images), &s);
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);

	frame = gtk_frame_new("Pieces List");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox2);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	gtk_container_border_width(GTK_CONTAINER(vbox2), 5);

	s.list_step = gtk_check_button_new_with_label("After each step");
	gtk_widget_show(s.list_step);
	gtk_box_pack_start(GTK_BOX(vbox2), s.list_step, TRUE, TRUE, 0);

	s.list_end = gtk_check_button_new_with_label("At the end");
	gtk_widget_show(s.list_end);
	gtk_box_pack_start(GTK_BOX(vbox2), s.list_end, TRUE, TRUE, 0);

	s.images = gtk_check_button_new_with_label("Create Images");
	gtk_widget_show(s.images);
	gtk_box_pack_start(GTK_BOX(vbox2), s.images, TRUE, TRUE, 0);

	s.highlight = gtk_check_button_new_with_label("Highlight new pieces");
	gtk_widget_show(s.highlight);
	gtk_box_pack_start(GTK_BOX(vbox), s.highlight, TRUE, TRUE, 0);

	s.htmlext = gtk_check_button_new_with_label("Save files with .html extension");
	gtk_widget_show(s.htmlext);
	gtk_box_pack_start(GTK_BOX(vbox), s.htmlext, TRUE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, FALSE, 0);

	label = gtk_label_new("Output directory");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	s.directory = gtk_entry_new();
	gtk_widget_show(s.directory);
	gtk_box_pack_start(GTK_BOX(hbox), s.directory, TRUE, TRUE, 0);

	button = gtk_button_new_with_label("...");
	gtk_widget_show(button);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(htmldlg_browse_output), &s);

	gtk_signal_connect(GTK_OBJECT(s.single), "toggled", GTK_SIGNAL_FUNC(htmldlg_layout), &s);
	gtk_signal_connect(GTK_OBJECT(s.multiple), "toggled", GTK_SIGNAL_FUNC(htmldlg_layout), &s);
	gtk_signal_connect(GTK_OBJECT(s.list_step), "toggled", GTK_SIGNAL_FUNC(htmldlg_list), &s);
	gtk_signal_connect(GTK_OBJECT(s.list_end), "toggled", GTK_SIGNAL_FUNC(htmldlg_list), &s);

	gtk_entry_set_text(GTK_ENTRY(s.directory), opts->path);
	if (opts->singlepage)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(s.single), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(s.multiple), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(s.index), opts->index);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(s.images), opts->images);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(s.list_end), opts->listend);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(s.list_step), opts->liststep);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(s.highlight), opts->highlight);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(s.htmlext), opts->htmlext);

	gtk_toggle_button_toggled(GTK_TOGGLE_BUTTON(s.single));
	gtk_toggle_button_toggled(GTK_TOGGLE_BUTTON(s.list_step));

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
	{
		ret = LC_OK;

		opts->singlepage = (GTK_TOGGLE_BUTTON(s.single)->active) ? true : false;
		opts->index = (GTK_TOGGLE_BUTTON(s.index)->active) ? true : false;
		opts->images = (GTK_TOGGLE_BUTTON(s.images)->active) ? true : false;
		opts->listend = (GTK_TOGGLE_BUTTON(s.list_end)->active) ? true : false;
		opts->liststep = (GTK_TOGGLE_BUTTON(s.list_step)->active) ? true : false;
		opts->highlight = (GTK_TOGGLE_BUTTON(s.highlight)->active) ? true : false;
		opts->htmlext = (GTK_TOGGLE_BUTTON(s.htmlext)->active) ? true : false;
		strcpy(opts->path, gtk_entry_get_text(GTK_ENTRY(s.directory)));
	}
	else
		ret = LC_CANCEL;

	gtk_widget_destroy(dlg);

	return ret;
}

// =============================================================================
// Image Options Dialog

struct LC_IMAGEOPTSDLG_STRUCT
{
	void* data;
	bool from_htmldlg;
	GtkWidget *single, *multiple, *from, *to;
	GtkWidget *width, *height;
	GtkWidget *bitmap, *gif, *jpg, *png;
	GtkWidget *highcolor, *transparent, *progressive, *quality;
};

int imageoptsdlg_execute(GtkWidget* parent, void* param, bool from_htmldlg)
{
	GtkWidget *dlg;
	GtkWidget *vbox, *vbox1, *vbox2;
	GtkWidget *hbox1, *hbox2;
	GtkWidget *frame, *label, *table;
	GSList *radio_group1 = (GSList*)NULL, *radio_group2 = (GSList*)NULL;
	LC_IMAGEOPTSDLG_STRUCT s;
	LC_IMAGEDLG_OPTS* opts = (LC_IMAGEDLG_OPTS*)param;
	s.data = param;
	s.from_htmldlg = from_htmldlg;
	int ret;

	dlg = gtk_dialog_new_with_buttons("Image Options", GTK_WINDOW(parent),
	                                  (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
	                                  GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(dlg), 5);

	vbox = GTK_DIALOG(dlg)->vbox;
	gtk_box_set_spacing(GTK_BOX(vbox), 10);

	hbox1 = gtk_hbox_new(FALSE, 10);
	gtk_widget_show(hbox1);
	gtk_container_add(GTK_CONTAINER(vbox), hbox1);

	vbox1 = gtk_vbox_new(FALSE, 10);
	gtk_widget_show(vbox1);
	gtk_box_pack_start(GTK_BOX(hbox1), vbox1, TRUE, TRUE, 0);

	frame = gtk_frame_new("Pictures");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox1), frame, TRUE, TRUE, 0);

	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox2);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	gtk_container_border_width(GTK_CONTAINER(vbox2), 5);

	s.single = gtk_radio_button_new_with_label(radio_group1, "Single");
	radio_group1 = gtk_radio_button_group(GTK_RADIO_BUTTON(s.single));
	gtk_widget_show(s.single);
	gtk_box_pack_start(GTK_BOX(vbox2), s.single, TRUE, TRUE, 0);

	s.multiple = gtk_radio_button_new_with_label(radio_group1, "Multiple");
	radio_group1 = gtk_radio_button_group(GTK_RADIO_BUTTON(s.multiple));
	gtk_widget_show(s.multiple);
	gtk_box_pack_start(GTK_BOX(vbox2), s.multiple, TRUE, TRUE, 0);

	hbox2 = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox2);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox2, TRUE, TRUE, 0);

	label = gtk_label_new("From");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox2), label, TRUE, TRUE, 0);

	s.from = gtk_entry_new();
	gtk_widget_show(s.from);
	gtk_box_pack_start(GTK_BOX(hbox2), s.from, FALSE, FALSE, 0);
	gtk_entry_set_width_chars(GTK_ENTRY(s.from), 3);

	label = gtk_label_new("to");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox2), label, TRUE, TRUE, 0);

	s.to = gtk_entry_new();
	gtk_widget_show(s.to);
	gtk_box_pack_start(GTK_BOX(hbox2), s.to, FALSE, FALSE, 0);
	gtk_entry_set_width_chars(GTK_ENTRY(s.to), 3);

	frame = gtk_frame_new("Dimensions");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox1), frame, TRUE, TRUE, 0);

	table = gtk_table_new(2, 2, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table), 10);

	s.width = gtk_entry_new();
	gtk_widget_show(s.width);
	gtk_table_attach(GTK_TABLE(table), s.width, 1, 2, 0, 1, (GtkAttachOptions)GTK_EXPAND, (GtkAttachOptions)GTK_EXPAND, 0, 0);
	gtk_entry_set_width_chars(GTK_ENTRY(s.width), 5);

	s.height = gtk_entry_new();
	gtk_widget_show(s.height);
	gtk_table_attach(GTK_TABLE(table), s.height, 1, 2, 1, 2, (GtkAttachOptions)GTK_EXPAND, (GtkAttachOptions)GTK_EXPAND, 0, 0);
	gtk_entry_set_width_chars(GTK_ENTRY(s.height), 5);

	label = gtk_label_new("Height");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, (GtkAttachOptions)GTK_EXPAND, (GtkAttachOptions)GTK_EXPAND, 0, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	label = gtk_label_new("Width");
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, (GtkAttachOptions)GTK_EXPAND, (GtkAttachOptions)GTK_EXPAND, 0, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	frame = gtk_frame_new("Format");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(hbox1), frame, TRUE, TRUE, 0);

	vbox1 = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(vbox1);
	gtk_container_add(GTK_CONTAINER(frame), vbox1);

	s.bitmap = gtk_radio_button_new_with_label(radio_group2, "Bitmap");
	radio_group2 = gtk_radio_button_group(GTK_RADIO_BUTTON(s.bitmap));
	gtk_widget_show(s.bitmap);
	gtk_box_pack_start(GTK_BOX(vbox1), s.bitmap, TRUE, TRUE, 0);

	s.highcolor = gtk_check_button_new_with_label("High color");
	gtk_widget_show(s.highcolor);
	gtk_box_pack_start(GTK_BOX(vbox1), s.highcolor, TRUE, TRUE, 0);

	s.gif = gtk_radio_button_new_with_label(radio_group2, "GIF");
	radio_group2 = gtk_radio_button_group(GTK_RADIO_BUTTON(s.gif));
	gtk_widget_show(s.gif);
	gtk_box_pack_start(GTK_BOX(vbox1), s.gif, TRUE, TRUE, 0);

	s.transparent = gtk_check_button_new_with_label("Transparent");
	gtk_widget_show(s.transparent);
	gtk_box_pack_start(GTK_BOX(vbox1), s.transparent, TRUE, TRUE, 0);

	s.jpg = gtk_radio_button_new_with_label(radio_group2, "JPEG");
	radio_group2 = gtk_radio_button_group(GTK_RADIO_BUTTON(s.jpg));
	gtk_widget_show(s.jpg);
	gtk_box_pack_start(GTK_BOX(vbox1), s.jpg, TRUE, TRUE, 0);

	s.progressive = gtk_check_button_new_with_label("Progressive");
	gtk_widget_show(s.progressive);
	gtk_box_pack_start(GTK_BOX(vbox1), s.progressive, TRUE, TRUE, 0);

	hbox2 = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox2);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox2, TRUE, TRUE, 0);

	s.quality = gtk_entry_new();
	gtk_widget_show(s.quality);
	gtk_box_pack_end(GTK_BOX(hbox2), s.quality, FALSE, FALSE, 0);
	gtk_entry_set_width_chars(GTK_ENTRY(s.quality), 3);

	label = gtk_label_new("Quality");
	gtk_widget_show(label);
	gtk_box_pack_end(GTK_BOX(hbox2), label, TRUE, TRUE, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	s.png = gtk_radio_button_new_with_label(radio_group2, "PNG");
	radio_group2 = gtk_radio_button_group(GTK_RADIO_BUTTON(s.png));
	gtk_widget_show(s.png);
	gtk_box_pack_start(GTK_BOX(vbox1), s.png, TRUE, TRUE, 0);

	switch (opts->imopts.format)
	{
	case LC_IMAGE_BMP:
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.bitmap), TRUE);
		break;

	case LC_IMAGE_GIF:
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.gif), TRUE);
		break;

	case LC_IMAGE_JPG:
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.jpg), TRUE);
		break;

	case LC_IMAGE_PNG:
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.png), TRUE);
		break;
	}

	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.transparent), opts->imopts.transparent);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.progressive), opts->imopts.interlaced);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.highcolor), opts->imopts.truecolor);

	write_int(s.quality, opts->imopts.quality);
	write_int(s.height, opts->height);
	write_int(s.width, opts->width);
	write_int(s.from, opts->from);
	write_int(s.to, opts->to);

	if (opts->multiple)
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.multiple), TRUE);
	else
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.single), TRUE);

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
	{
		int i;

		ret = LC_OK;

		if ((GTK_TOGGLE_BUTTON(s.bitmap)->active))
			opts->imopts.format = LC_IMAGE_BMP;
		else if ((GTK_TOGGLE_BUTTON(s.gif)->active))
			opts->imopts.format = LC_IMAGE_GIF;
		else if ((GTK_TOGGLE_BUTTON(s.jpg)->active))
			opts->imopts.format = LC_IMAGE_JPG;
		else if ((GTK_TOGGLE_BUTTON(s.png)->active))
			opts->imopts.format = LC_IMAGE_PNG;

		opts->imopts.transparent = (GTK_TOGGLE_BUTTON(s.transparent)->active) ? true : false;
		opts->imopts.interlaced = (GTK_TOGGLE_BUTTON(s.progressive)->active) ? true : false;
		opts->imopts.truecolor = (GTK_TOGGLE_BUTTON(s.highcolor)->active) ? true : false;

		if (!read_int(s.quality, &i, 1, 100))
			ret = LC_CANCEL;
		opts->imopts.quality = i;
		if (!read_int(s.height, &i, 1, 5000))
			ret = LC_CANCEL;
		opts->height = i;
		if (!read_int(s.width, &i, 1, 5000))
			ret = LC_CANCEL;
		opts->width = i;
		if (!read_int(s.from, &i, 1, 65535))
			ret = LC_CANCEL;
		opts->from = i;
		if (!read_int(s.to, &i, 1, 65535))
			ret = LC_CANCEL;
		opts->to = i;
		opts->multiple = (GTK_TOGGLE_BUTTON(s.multiple)->active) ? true : false;

		unsigned long image = opts->imopts.format;
		if (opts->imopts.interlaced)
			image |= LC_IMAGE_PROGRESSIVE;
		if (opts->imopts.transparent)
			image |= LC_IMAGE_TRANSPARENT;
		if (opts->imopts.truecolor)
			image |= LC_IMAGE_HIGHCOLOR;

		if (s.from_htmldlg)
		{
			Sys_ProfileSaveInt("Default", "HTML Options", image);
			Sys_ProfileSaveInt("Default", "HTML Width", opts->width);
			Sys_ProfileSaveInt("Default", "HTML Height", opts->height);
		}
		else
		{
			Sys_ProfileSaveInt("Default", "Image Options", image);
			Sys_ProfileSaveInt("Default", "Image Width", opts->width);
			Sys_ProfileSaveInt("Default", "Image Height", opts->height);
		}
	}
	else
		ret = LC_CANCEL;

	gtk_widget_destroy(dlg);

	return ret;
}

// =============================================================================
// POV-Ray Export Dialog

struct LC_POVRAYDLG_STRUCT
{
	void* data;
	GtkWidget* dlg;
	GtkWidget *lgeo, *pov, *output, *render;
};

static void povraydlg_browse_output(GtkWidget *widget, gpointer data)
{
	LC_POVRAYDLG_STRUCT* s = (LC_POVRAYDLG_STRUCT*)data;
	GtkFileFilter* filter;
	GtkWidget* dlg;

	dlg = gtk_file_chooser_dialog_new("Export As", GTK_WINDOW(s->dlg), GTK_FILE_CHOOSER_ACTION_SAVE,
	                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), gtk_entry_get_text(GTK_ENTRY(s->output)));
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), TRUE);

	filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_filter_set_name(GTK_FILE_FILTER(filter), "All Files");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.[pP][oO][vV]");
	gtk_file_filter_set_name(GTK_FILE_FILTER(filter), "POV-Ray files (*.pov)");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filter);

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
	{
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
		gtk_entry_set_text(GTK_ENTRY(s->output), filename);
		g_free(filename);
	}

	gtk_widget_destroy(dlg);
}

static void povraydlg_browse_povray(GtkWidget *widget, gpointer data)
{
	LC_POVRAYDLG_STRUCT* s = (LC_POVRAYDLG_STRUCT*)data;
	GtkWidget* dlg;

	dlg = gtk_file_chooser_dialog_new("Select POV-Ray executable", GTK_WINDOW(s->dlg), GTK_FILE_CHOOSER_ACTION_OPEN,
	                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), gtk_entry_get_text(GTK_ENTRY(s->pov)));

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
	{
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
		gtk_entry_set_text(GTK_ENTRY(s->pov), filename);
		g_free(filename);
	}

	gtk_widget_destroy(dlg);
}

static void povraydlg_browse_lgeo(GtkWidget *widget, gpointer data)
{
	LC_POVRAYDLG_STRUCT* s = (LC_POVRAYDLG_STRUCT*)data;
	GtkWidget* dlg;

	dlg = gtk_file_chooser_dialog_new("Select LGEO path", GTK_WINDOW(s->dlg), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
	                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), gtk_entry_get_text(GTK_ENTRY(s->lgeo)));

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
	{
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
		gtk_entry_set_text(GTK_ENTRY(s->lgeo), filename);
		g_free(filename);
	}

	gtk_widget_destroy(dlg);
}

int povraydlg_execute(void* param)
{
	GtkWidget *dlg;
	GtkWidget *vbox, *hbox2;
	GtkWidget *label, *button;
	LC_POVRAYDLG_STRUCT s;
	s.data = param;
	int ret;

	dlg = gtk_dialog_new_with_buttons("POV-Ray Export", GTK_WINDOW(((GtkWidget*)(*main_window))),
	                                  (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
	                                  GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(dlg), 5);
	s.dlg = dlg;

	vbox = GTK_DIALOG(dlg)->vbox;
	gtk_box_set_spacing(GTK_BOX(vbox), 10);

	hbox2 = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, TRUE, FALSE, 0);

	label = gtk_label_new("Output File");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, TRUE, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	hbox2 = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, TRUE, FALSE, 0);

	s.output = gtk_entry_new();
	gtk_widget_show(s.output);
	gtk_box_pack_start(GTK_BOX(hbox2), s.output, TRUE, TRUE, 0);
	gtk_entry_set_width_chars(GTK_ENTRY(s.output), 40);

	button = gtk_button_new_with_label("...");
	gtk_widget_show(button);
	gtk_box_pack_end(GTK_BOX(hbox2), button, FALSE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(povraydlg_browse_output), &s);

	hbox2 = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, TRUE, FALSE, 0);

	label = gtk_label_new("POV-Ray Executable");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, TRUE, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	hbox2 = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, TRUE, FALSE, 0);

	s.pov = gtk_entry_new();
	gtk_widget_show(s.pov);
	gtk_box_pack_start(GTK_BOX(hbox2), s.pov, TRUE, TRUE, 0);
	gtk_entry_set_width_chars(GTK_ENTRY(s.pov), 40);

	button = gtk_button_new_with_label("...");
	gtk_widget_show(button);
	gtk_box_pack_end(GTK_BOX(hbox2), button, FALSE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(povraydlg_browse_povray), &s);

	s.render = gtk_check_button_new_with_label("Render Scene");
	gtk_widget_show(s.render);
	gtk_box_pack_start(GTK_BOX(vbox), s.render, TRUE, FALSE, 0);

	hbox2 = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, TRUE, FALSE, 0);

	label = gtk_label_new("LGEO Path");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, TRUE, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	hbox2 = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, TRUE, FALSE, 0);

	s.lgeo = gtk_entry_new();
	gtk_widget_show(s.lgeo);
	gtk_box_pack_start(GTK_BOX(hbox2), s.lgeo, TRUE, TRUE, 0);
	gtk_entry_set_width_chars(GTK_ENTRY(s.lgeo), 40);

	button = gtk_button_new_with_label("...");
	gtk_widget_show(button);
	gtk_box_pack_end(GTK_BOX(hbox2), button, FALSE, TRUE, 0);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(povraydlg_browse_lgeo), &s);

	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.render), Sys_ProfileLoadInt("Settings", "POV Render", 1) ? TRUE : FALSE);
	gtk_entry_set_text(GTK_ENTRY(s.pov), Sys_ProfileLoadString("Settings", "POV-Ray", ""));
	gtk_entry_set_text(GTK_ENTRY(s.lgeo), Sys_ProfileLoadString("Settings", "LGEO", ""));

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
	{
		ret = LC_OK;

		LC_POVRAYDLG_OPTS* opts = (LC_POVRAYDLG_OPTS*)s.data;

		opts->render =  (GTK_TOGGLE_BUTTON(s.render)->active) ? true : false;
		strcpy(opts->povpath, gtk_entry_get_text(GTK_ENTRY(s.pov)));
		strcpy(opts->outpath, gtk_entry_get_text(GTK_ENTRY(s.output)));
		strcpy(opts->libpath, gtk_entry_get_text(GTK_ENTRY(s.lgeo)));

		Sys_ProfileSaveInt("Settings", "POV Render", opts->render);
		Sys_ProfileSaveString("Settings", "POV-Ray", opts->povpath);
		Sys_ProfileSaveString("Settings", "LGEO", opts->libpath);
	}
	else
		ret = LC_CANCEL;

	gtk_widget_destroy(dlg);

	return ret;
}

// =============================================================================
// Wavefront Dialog

int wavefrontdlg_execute(void* param)
{
	GtkFileFilter* filter;
	GtkWidget* dlg;
	int ret;

	dlg = gtk_file_chooser_dialog_new("Export As", GTK_WINDOW(((GtkWidget*)(*main_window))), GTK_FILE_CHOOSER_ACTION_SAVE,
	                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);

	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), (char*)param);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), TRUE);

	filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_filter_set_name(GTK_FILE_FILTER(filter), "All Files");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.[oO][bB][jJ]");
	gtk_file_filter_set_name(GTK_FILE_FILTER(filter), "Wavefront Files (*.obj)");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filter);

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
	{
		char *filename;

		ret = LC_OK;

		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
		strcpy((char*)param, filename);
		g_free(filename);
	}
	else
		ret = LC_CANCEL;

	gtk_widget_destroy(dlg);

	return ret;
}

// =============================================================================
// Preferences Dialog

struct LC_PREFERENCESDLG_STRUCT
{
	void* data;
	GtkWidget *dlg;
	GtkWidget *gen_updates, *gen_mouse;
	GtkWidget *det_edges, *det_lighting, *det_smooth;
	GtkWidget *det_antialias, *det_fast, *det_width;
	GtkWidget *draw_grid, *draw_gridunits, *draw_axis;
	GtkWidget *draw_snapx, *draw_snapy, *draw_snapz, *draw_angle;
	GtkWidget *draw_anglesnap, *draw_centimeter, *draw_relative;
	GtkWidget *draw_move, *draw_fixed, *draw_lockx, *draw_locky, *draw_lockz;
	GtkWidget *scn_solid, *scn_gradient, *scn_image, *scn_imagename;
	GtkWidget *scn_tile, *scn_fog, *scn_floor, *scn_density;
	GtkWidget *scn_clrbackground, *scn_clrgrad1, *scn_clrgrad2, *scn_clrfog, *scn_clrambient;
};

bool preferencesdlg_getopts(LC_PREFERENCESDLG_STRUCT* s, LC_PREFERENCESDLG_OPTS* opts)
{
	int Fog, GridSize, AngleSnap;

	opts->nMouse = (int)gtk_range_get_value(GTK_RANGE(s->gen_mouse));

	opts->nDetail = 0;
	if (GTK_TOGGLE_BUTTON(s->det_edges)->active) opts->nDetail |= LC_DET_BRICKEDGES;
	if (GTK_TOGGLE_BUTTON(s->det_lighting)->active) opts->nDetail |= LC_DET_LIGHTING;
	if (GTK_TOGGLE_BUTTON(s->det_smooth)->active) opts->nDetail |= LC_DET_SMOOTH;
	if (GTK_TOGGLE_BUTTON(s->det_antialias)->active) opts->nDetail |= LC_DET_ANTIALIAS;
	if (GTK_TOGGLE_BUTTON(s->det_fast)->active) opts->nDetail |= LC_DET_FAST;
	if (!read_float(s->det_width, &opts->fLineWidth, 0.5f, 5.0f))
		return false;

	opts->nSnap = 0;
	if (GTK_TOGGLE_BUTTON(s->draw_grid)->active) opts->nSnap |= LC_DRAW_GRID;
	if (GTK_TOGGLE_BUTTON(s->draw_axis)->active) opts->nSnap |= LC_DRAW_AXIS;
	if (GTK_TOGGLE_BUTTON(s->draw_snapx)->active) opts->nSnap |= LC_DRAW_SNAP_X;
	if (GTK_TOGGLE_BUTTON(s->draw_snapy)->active) opts->nSnap |= LC_DRAW_SNAP_Y;
	if (GTK_TOGGLE_BUTTON(s->draw_snapz)->active) opts->nSnap |= LC_DRAW_SNAP_Z;
	if (GTK_TOGGLE_BUTTON(s->draw_angle)->active) opts->nSnap |= LC_DRAW_SNAP_A;
	if (GTK_TOGGLE_BUTTON(s->draw_centimeter)->active) opts->nSnap |= LC_DRAW_CM_UNITS;
	if (GTK_TOGGLE_BUTTON(s->draw_move)->active) opts->nSnap |= LC_DRAW_MOVE;
	if (GTK_TOGGLE_BUTTON(s->draw_fixed)->active) opts->nSnap |= LC_DRAW_MOVEAXIS;
	if (GTK_TOGGLE_BUTTON(s->draw_lockx)->active) opts->nSnap |= LC_DRAW_LOCK_X;
	if (GTK_TOGGLE_BUTTON(s->draw_locky)->active) opts->nSnap |= LC_DRAW_LOCK_Y;
	if (GTK_TOGGLE_BUTTON(s->draw_lockz)->active) opts->nSnap |= LC_DRAW_LOCK_Z;
	if (GTK_TOGGLE_BUTTON(s->draw_relative)->active) opts->nSnap |= LC_DRAW_GLOBAL_SNAP;
	if (!read_int(s->draw_gridunits, &GridSize, 2, 1000))
		return false;
	opts->nGridSize = GridSize;
	if (!read_int(s->draw_anglesnap, &AngleSnap, 1, 180))
		return false;
	opts->nAngleSnap = AngleSnap;

	opts->nScene = 0;
	if (GTK_TOGGLE_BUTTON(s->scn_gradient)->active) opts->nScene |= LC_SCENE_GRADIENT;
	if (GTK_TOGGLE_BUTTON(s->scn_image)->active) opts->nScene |= LC_SCENE_BG;
	if (GTK_TOGGLE_BUTTON(s->scn_tile)->active) opts->nScene |= LC_SCENE_BG_TILE;
	if (GTK_TOGGLE_BUTTON(s->scn_fog)->active) opts->nScene |= LC_SCENE_FOG;
	if (GTK_TOGGLE_BUTTON(s->scn_floor)->active) opts->nScene |= LC_SCENE_FLOOR;
	read_int(s->scn_density, &Fog, 1, 100);
	opts->fDensity = (float)Fog/100.0f;
	strcpy(opts->strBackground, gtk_entry_get_text(GTK_ENTRY(s->scn_imagename)));

	memcpy(opts->fBackground, gtk_object_get_data(GTK_OBJECT(s->scn_clrbackground), "color_flt"), 3 * sizeof(float));
	memcpy(opts->fFog, gtk_object_get_data(GTK_OBJECT(s->scn_clrfog), "color_flt"), 3 * sizeof(float));
	memcpy(opts->fAmbient, gtk_object_get_data(GTK_OBJECT(s->scn_clrambient), "color_flt"), 3 * sizeof(float));
	memcpy(opts->fGrad1, gtk_object_get_data(GTK_OBJECT(s->scn_clrgrad1), "color_flt"), 3 * sizeof(float));
	memcpy(opts->fGrad2, gtk_object_get_data(GTK_OBJECT(s->scn_clrgrad2), "color_flt"), 3 * sizeof(float));
	
	// int nSaveInterval;
	// char strPath[LC_MAXPATH];

	return true;
}

static void preferencesdlg_default(GtkWidget *widget, gpointer data)
{
	LC_PREFERENCESDLG_STRUCT* s = (LC_PREFERENCESDLG_STRUCT*)data;
	LC_PREFERENCESDLG_OPTS Opts;

	if (!preferencesdlg_getopts(s, &Opts))
		return;

	Sys_ProfileSaveInt("Default", "Mouse", (int)Opts.nMouse);
	Sys_ProfileSaveInt("Default", "Detail", Opts.nDetail);
	Sys_ProfileSaveInt("Default", "Line", (int)(Opts.fLineWidth * 100));
	Sys_ProfileSaveInt("Default", "Snap", Opts.nSnap);
	Sys_ProfileSaveInt("Default", "Angle", Opts.nAngleSnap);
	Sys_ProfileSaveInt("Default", "Grid", Opts.nGridSize);
	Sys_ProfileSaveInt("Default", "Scene", Opts.nScene);
	Sys_ProfileSaveInt("Default", "Density", (int)(Opts.fDensity * 100));
	Sys_ProfileSaveString("Default", "BMP", Opts.strBackground);
	Sys_ProfileSaveInt("Default", "Background", RGB(Opts.fBackground[0]*255, Opts.fBackground[1]*255, Opts.fBackground[2]*255));
	Sys_ProfileSaveInt("Default", "Fog", RGB(Opts.fFog[0]*255, Opts.fFog[1]*255, Opts.fFog[2]*255));
	Sys_ProfileSaveInt("Default", "Ambient", RGB(Opts.fAmbient[0]*255, Opts.fAmbient[1]*255, Opts.fAmbient[2]*255));
	Sys_ProfileSaveInt("Default", "Gradient1", RGB(Opts.fGrad1[0]*255, Opts.fGrad1[1]*255, Opts.fGrad1[2]*255));
	Sys_ProfileSaveInt("Default", "Gradient2", RGB(Opts.fGrad2[0]*255, Opts.fGrad2[1]*255, Opts.fGrad2[2]*255));
}

static void preferencesdlg_color(GtkWidget *widget, gpointer data)
{
	GtkWidget* dlg;
	float* color = (float*)data;
	double dbl[3] = { color[0], color[1], color[2] };

	dlg = gtk_color_selection_dialog_new("Choose Color");
	gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(gtk_widget_get_toplevel(widget)));
	gtk_color_selection_set_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(dlg)->colorsel), dbl);

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
	{
		float* color = (float*)data;

		gtk_color_selection_get_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(dlg)->colorsel), dbl);
		color[0] = dbl[0];
		color[1] = dbl[1];
		color[2] = dbl[2];

		set_button_pixmap(widget, color);
	}

	gtk_widget_destroy(dlg);
}

static void preferencesdlg_colorbutton_map(GtkWidget *widget, gpointer d)
{
	set_button_pixmap(widget, (float*)gtk_object_get_data(GTK_OBJECT(widget), "color_flt"));
}

int preferencesdlg_execute(void* param)
{
	GtkWidget *dlg;
	GtkWidget *vbox, *hbox;
	GtkWidget *frame, *label, *button, *table, *notebook;
	GSList *table_group = NULL;
	LC_PREFERENCESDLG_STRUCT s;
	LC_PREFERENCESDLG_OPTS* opts = (LC_PREFERENCESDLG_OPTS*)param;
	s.data = param;
	int ret;

	dlg = gtk_dialog_new_with_buttons("Preferences", GTK_WINDOW(((GtkWidget*)(*main_window))),
	                                  (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
	                                  GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(dlg), 5);
	s.dlg = dlg;

	vbox = GTK_DIALOG(dlg)->vbox;
	gtk_box_set_spacing(GTK_BOX(vbox), 10);

	notebook = gtk_notebook_new();
	gtk_widget_show(notebook);
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	table = gtk_table_new(6, 1, TRUE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(notebook), table);
	gtk_container_border_width(GTK_CONTAINER(table), 5);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 0, 1, 0, 1);

	label = gtk_label_new("Mouse sensitivity:");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	s.gen_mouse = gtk_hscale_new_with_range(1, 20, 1);
	gtk_widget_show(s.gen_mouse);
	gtk_box_pack_start(GTK_BOX(hbox), s.gen_mouse, TRUE, TRUE, 0);
	gtk_scale_set_draw_value(GTK_SCALE(s.gen_mouse), FALSE);

	s.gen_updates = gtk_check_button_new_with_label("Check for updates on startup");
//	gtk_widget_show(s.gen_updates);
	gtk_table_attach_defaults(GTK_TABLE(table), s.gen_updates, 0, 1, 1, 2);

	table = gtk_table_new(6, 1, TRUE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(notebook), table);
	gtk_container_border_width(GTK_CONTAINER(table), 5);

	s.det_edges = gtk_check_button_new_with_label("Draw edges");
	gtk_widget_show(s.det_edges);
	gtk_table_attach_defaults(GTK_TABLE(table), s.det_edges, 0, 1, 0, 1);

	s.det_lighting = gtk_check_button_new_with_label("Lighting");
	gtk_widget_show(s.det_lighting);
	gtk_table_attach_defaults(GTK_TABLE(table), s.det_lighting, 0, 1, 1, 2);

	s.det_smooth = gtk_check_button_new_with_label("Smooth shading");
	gtk_widget_show(s.det_smooth);
	gtk_table_attach_defaults(GTK_TABLE(table), s.det_smooth, 0, 1, 2, 3);

	s.det_antialias = gtk_check_button_new_with_label("Anti-aliasing");
	gtk_widget_show(s.det_antialias);
	gtk_table_attach_defaults(GTK_TABLE(table), s.det_antialias, 0, 1, 3, 4);

	s.det_fast = gtk_check_button_new_with_label("Fast rendering");
	gtk_widget_show(s.det_fast);
	gtk_table_attach_defaults(GTK_TABLE(table), s.det_fast, 0, 1, 4, 5);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 0, 1, 5, 6);

	label = gtk_label_new("Line width");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	s.det_width = gtk_entry_new();
	gtk_widget_show(s.det_width);
	gtk_box_pack_start(GTK_BOX(hbox), s.det_width, FALSE, FALSE, 0);
	gtk_entry_set_width_chars(GTK_ENTRY(s.det_width), 4);

	table = gtk_table_new(7, 2, TRUE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(notebook), table);
	gtk_container_border_width(GTK_CONTAINER(table), 5);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 0, 1, 0, 1);

	s.draw_grid = gtk_check_button_new_with_label("Base grid");
	gtk_widget_show(s.draw_grid);
	gtk_box_pack_start(GTK_BOX(hbox), s.draw_grid, FALSE, FALSE, 0);

	s.draw_gridunits = gtk_entry_new();
	gtk_widget_show(s.draw_gridunits);
	gtk_box_pack_start(GTK_BOX(hbox), s.draw_gridunits, FALSE, FALSE, 0);
	gtk_entry_set_width_chars(GTK_ENTRY(s.draw_gridunits), 4);

	label = gtk_label_new("units");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	s.draw_axis = gtk_check_button_new_with_label("Axis icon");
	gtk_widget_show(s.draw_axis);
	gtk_table_attach_defaults(GTK_TABLE(table), s.draw_axis, 0, 1, 1, 2);

	s.draw_centimeter = gtk_check_button_new_with_label("Centimeter units");
	gtk_widget_show(s.draw_centimeter);
	gtk_table_attach_defaults(GTK_TABLE(table), s.draw_centimeter, 0, 1, 2, 3);

	s.draw_snapx = gtk_check_button_new_with_label("Snap X");
	gtk_widget_show(s.draw_snapx);
	gtk_table_attach_defaults(GTK_TABLE(table), s.draw_snapx, 0, 1, 3, 4);

	s.draw_snapy = gtk_check_button_new_with_label("Snap Y");
	gtk_widget_show(s.draw_snapy);
	gtk_table_attach_defaults(GTK_TABLE(table), s.draw_snapy, 0, 1, 4, 5);

	s.draw_snapz = gtk_check_button_new_with_label("Snap Z");
	gtk_widget_show(s.draw_snapz);
	gtk_table_attach_defaults(GTK_TABLE(table), s.draw_snapz, 0, 1, 5, 6);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 0, 1, 6, 7);

	s.draw_angle = gtk_check_button_new_with_label("Angle snap");
	gtk_widget_show(s.draw_angle);
	gtk_box_pack_start(GTK_BOX(hbox), s.draw_angle, FALSE, FALSE, 0);

	s.draw_anglesnap = gtk_entry_new();
	gtk_widget_show(s.draw_anglesnap);
	gtk_box_pack_start(GTK_BOX(hbox), s.draw_anglesnap, FALSE, FALSE, 0);
	gtk_entry_set_width_chars(GTK_ENTRY(s.draw_anglesnap), 4);

	label = gtk_label_new("degrees");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	s.draw_relative = gtk_check_button_new_with_label("Don't allow relative snap");
	gtk_widget_show(s.draw_relative);
	gtk_table_attach_defaults(GTK_TABLE(table), s.draw_relative, 1, 2, 0, 1);

	s.draw_move = gtk_check_button_new_with_label("Switch to move after insert");
	gtk_widget_show(s.draw_move);
	gtk_table_attach_defaults(GTK_TABLE(table), s.draw_move, 1, 2, 1, 2);

	s.draw_fixed = gtk_check_button_new_with_label("Fixed direction keys");
	gtk_widget_show(s.draw_fixed);
	gtk_table_attach_defaults(GTK_TABLE(table), s.draw_fixed, 1, 2, 2, 3);

	s.draw_lockx = gtk_check_button_new_with_label("Lock X");
	gtk_widget_show(s.draw_lockx);
	gtk_table_attach_defaults(GTK_TABLE(table), s.draw_lockx, 1, 2, 3, 4);

	s.draw_locky = gtk_check_button_new_with_label("Lock Y");
	gtk_widget_show(s.draw_locky);
	gtk_table_attach_defaults(GTK_TABLE(table), s.draw_locky, 1, 2, 4, 5);

	s.draw_lockz = gtk_check_button_new_with_label("Lock Z");
	gtk_widget_show(s.draw_lockz);
	gtk_table_attach_defaults(GTK_TABLE(table), s.draw_lockz, 1, 2, 5, 6);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(notebook), hbox);
	gtk_container_border_width(GTK_CONTAINER(hbox), 5);

	frame = gtk_frame_new("Background");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);

	table = gtk_table_new(4, 3, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	s.scn_solid = gtk_radio_button_new_with_label(table_group, "Solid color");
	table_group = gtk_radio_button_group(GTK_RADIO_BUTTON(s.scn_solid));
	gtk_widget_show(s.scn_solid);
	gtk_table_attach_defaults(GTK_TABLE(table), s.scn_solid, 0, 1, 0, 1);

	s.scn_gradient = gtk_radio_button_new_with_label(table_group, "Gradient");
	table_group = gtk_radio_button_group(GTK_RADIO_BUTTON(s.scn_gradient));
	gtk_widget_show(s.scn_gradient);
	gtk_table_attach_defaults(GTK_TABLE(table), s.scn_gradient, 0, 1, 1, 2);

	s.scn_image = gtk_radio_button_new_with_label(table_group, "Image");
	table_group = gtk_radio_button_group(GTK_RADIO_BUTTON(s.scn_image));
	gtk_widget_show(s.scn_image);
	gtk_table_attach_defaults(GTK_TABLE(table), s.scn_image, 0, 1, 2, 3);

	s.scn_imagename = gtk_entry_new();
	gtk_widget_show(s.scn_imagename);
	gtk_table_attach_defaults(GTK_TABLE(table), s.scn_imagename, 0, 3, 3, 4);

	s.scn_clrbackground = button = gtk_button_new_with_label("");
	gtk_widget_show(button);
	gtk_table_attach(GTK_TABLE(table), button, 1, 2, 0, 1, (GtkAttachOptions)GTK_EXPAND, (GtkAttachOptions)GTK_EXPAND, 0, 0);
	gtk_widget_set_usize(button, 50, 30);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(preferencesdlg_color), opts->fBackground);
	gtk_signal_connect(GTK_OBJECT(button), "map", GTK_SIGNAL_FUNC(preferencesdlg_colorbutton_map), NULL);
	gtk_object_set_data(GTK_OBJECT(button), "color_flt", opts->fBackground);

	s.scn_clrgrad1 = button = gtk_button_new_with_label("");
	gtk_widget_show(button);
	gtk_table_attach(GTK_TABLE(table), button, 1, 2, 1, 2, (GtkAttachOptions)GTK_EXPAND, (GtkAttachOptions)GTK_EXPAND, 0, 0);
	gtk_widget_set_usize(button, 50, 30);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(preferencesdlg_color), opts->fGrad1);
	gtk_signal_connect(GTK_OBJECT(button), "map", GTK_SIGNAL_FUNC(preferencesdlg_colorbutton_map), NULL);
	gtk_object_set_data(GTK_OBJECT(button), "color_flt", opts->fGrad1);

	s.scn_clrgrad2 = button = gtk_button_new_with_label("");
	gtk_widget_show(button);
	gtk_table_attach(GTK_TABLE(table), button, 2, 3, 1, 2, (GtkAttachOptions)GTK_EXPAND, (GtkAttachOptions)GTK_EXPAND, 0, 0);
	gtk_widget_set_usize(button, 50, 30);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(preferencesdlg_color), opts->fGrad2);
	gtk_signal_connect(GTK_OBJECT(button), "map", GTK_SIGNAL_FUNC(preferencesdlg_colorbutton_map), NULL);
	gtk_object_set_data(GTK_OBJECT(button), "color_flt", opts->fGrad2);

	s.scn_tile = gtk_check_button_new_with_label("Tile");
	gtk_widget_show(s.scn_tile);
	gtk_table_attach(GTK_TABLE(table), s.scn_tile, 1, 2, 2, 3, (GtkAttachOptions)GTK_EXPAND, (GtkAttachOptions)GTK_EXPAND, 0, 0);

	frame = gtk_frame_new("Environment");
	gtk_widget_show(frame);
	gtk_box_pack_end(GTK_BOX(hbox), frame, TRUE, TRUE, 0);

	table = gtk_table_new(5, 3, TRUE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_container_border_width(GTK_CONTAINER(table), 5);

	s.scn_fog = gtk_check_button_new_with_label("Fog");
	gtk_widget_show(s.scn_fog);
	gtk_table_attach_defaults(GTK_TABLE(table), s.scn_fog, 0, 3, 0, 1);

	label = gtk_label_new("Color");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 1, 2);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	label = gtk_label_new("Density");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 2, 3);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 1.93715e-07, 0.5);

	label = gtk_label_new("Ambient light");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 2, 3, 4);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

	s.scn_floor = gtk_check_button_new_with_label("Draw floor");
	gtk_widget_show(s.scn_floor);
	gtk_table_attach_defaults(GTK_TABLE(table), s.scn_floor, 0, 3, 4, 5);

	s.scn_clrfog = button = gtk_button_new_with_label("");
	gtk_widget_show(button);
	gtk_table_attach(GTK_TABLE(table), button, 2, 3, 1, 2, (GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0);
	gtk_widget_set_usize(button, 50, 30);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(preferencesdlg_color), opts->fFog);
	gtk_signal_connect(GTK_OBJECT(button), "map", GTK_SIGNAL_FUNC(preferencesdlg_colorbutton_map), NULL);
	gtk_object_set_data(GTK_OBJECT(button), "color_flt", opts->fFog);

	s.scn_clrambient = button = gtk_button_new_with_label("");
	gtk_widget_show(button);
	gtk_table_attach(GTK_TABLE(table), button, 2, 3, 3, 4, (GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0);
	gtk_widget_set_usize(button, 50, 30);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(preferencesdlg_color), opts->fAmbient);
	gtk_signal_connect(GTK_OBJECT(button), "map", GTK_SIGNAL_FUNC(preferencesdlg_colorbutton_map), NULL);
	gtk_object_set_data(GTK_OBJECT(button), "color_flt", opts->fAmbient);

	s.scn_density = gtk_entry_new();
	gtk_widget_show(s.scn_density);
	gtk_table_attach(GTK_TABLE(table), s.scn_density, 2, 3, 2, 3, (GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0);
	gtk_entry_set_width_chars(GTK_ENTRY(s.scn_density), 4);

	label = gtk_label_new("General");
	gtk_widget_show(label);
	set_notebook_tab(notebook, 0, label);

	label = gtk_label_new("Details");
	gtk_widget_show(label);
	set_notebook_tab(notebook, 1, label);

	label = gtk_label_new("Drawing Aids");
	gtk_widget_show(label);
	set_notebook_tab(notebook, 2, label);

	label = gtk_label_new("Scene");
	gtk_widget_show(label);
	set_notebook_tab(notebook, 3, label);

	button = gtk_button_new_with_label("Make Default");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(preferencesdlg_default), &s);
	gtk_widget_show(button);
	gtk_box_pack_end(GTK_BOX(GTK_DIALOG(dlg)->action_area), button, FALSE, FALSE, 0);

	// Set initial values
	gtk_range_set_value(GTK_RANGE(s.gen_mouse), opts->nMouse);

	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.det_edges),(opts->nDetail & LC_DET_BRICKEDGES) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.det_lighting),(opts->nDetail & LC_DET_LIGHTING) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.det_smooth),(opts->nDetail & LC_DET_SMOOTH) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.det_antialias),(opts->nDetail & LC_DET_ANTIALIAS) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.det_fast),(opts->nDetail & LC_DET_FAST) ? TRUE : FALSE);
	write_float(s.det_width, opts->fLineWidth);

	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.draw_grid), (opts->nSnap & LC_DRAW_GRID) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.draw_axis), (opts->nSnap & LC_DRAW_AXIS) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.draw_snapx), (opts->nSnap & LC_DRAW_SNAP_X) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.draw_snapy), (opts->nSnap & LC_DRAW_SNAP_Y) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.draw_snapz), (opts->nSnap & LC_DRAW_SNAP_Z) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.draw_angle), (opts->nSnap & LC_DRAW_SNAP_A) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.draw_centimeter), (opts->nSnap & LC_DRAW_CM_UNITS) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.draw_move), (opts->nSnap & LC_DRAW_MOVE) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.draw_fixed), (opts->nSnap & LC_DRAW_MOVEAXIS) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.draw_lockx), (opts->nSnap & LC_DRAW_LOCK_X) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.draw_locky), (opts->nSnap & LC_DRAW_LOCK_Y) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.draw_lockz), (opts->nSnap & LC_DRAW_LOCK_Z) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.draw_relative), (opts->nSnap & LC_DRAW_GLOBAL_SNAP) ? TRUE : FALSE);
	write_int(s.draw_gridunits, opts->nGridSize);
	write_int(s.draw_anglesnap, opts->nAngleSnap);

	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.scn_gradient), (opts->nScene & LC_SCENE_GRADIENT) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.scn_image), (opts->nScene & LC_SCENE_BG) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.scn_tile), (opts->nScene & LC_SCENE_BG_TILE) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.scn_fog), (opts->nScene & LC_SCENE_FOG) ? TRUE : FALSE);
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(s.scn_floor), (opts->nScene & LC_SCENE_FLOOR) ? TRUE : FALSE);
	gtk_entry_set_text(GTK_ENTRY(s.scn_imagename), opts->strBackground);
	write_int(s.scn_density, (int)(opts->fDensity*100));

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
	{
		LC_PREFERENCESDLG_OPTS TempOpts;

		if (preferencesdlg_getopts(&s, &TempOpts))
		{
			ret = LC_OK;

			opts->nMouse = TempOpts.nMouse;
			opts->nDetail = TempOpts.nDetail;
			opts->fLineWidth = TempOpts.fLineWidth;
			opts->nSnap = TempOpts.nSnap;
			opts->nAngleSnap = TempOpts.nAngleSnap;
			opts->nGridSize = TempOpts.nGridSize;
			opts->nScene = TempOpts.nScene;
			opts->fDensity = TempOpts.fDensity;
			strcpy(opts->strBackground, TempOpts.strBackground);
		}
		else
			ret = LC_CANCEL;
	}
	else
		ret = LC_CANCEL;

	gtk_widget_destroy(dlg);

	return ret;
}

// =============================================================================
// Properties Dialog

struct LC_PROPERTIESDLG_STRUCT
{
	void* data;
	GtkWidget *dlg;
	GtkWidget *sum_author, *sum_description, *sum_comments;
};

int propertiesdlg_execute(void* param)
{
	GtkWidget *dlg;
	GtkWidget *vbox1, *vbox2, *hbox;
	GtkWidget *label, *table, *notebook, *list, *scroll_win;
	LC_PROPERTIESDLG_STRUCT s;
	LC_PROPERTIESDLG_OPTS* opts = (LC_PROPERTIESDLG_OPTS*)param;
	s.data = param;
	int ret;

	struct stat buf;
	bool exist = (stat(opts->strFilename, &buf) != -1);
	char* ptr = strrchr(opts->strFilename, '/');
	char text[LC_MAXPATH + 64];
	strcpy(text, opts->strTitle);
	strcat(text, " Properties");

	dlg = gtk_dialog_new_with_buttons(text, GTK_WINDOW(((GtkWidget*)(*main_window))),
	                                  (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
	                                  GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(dlg), 5);
	s.dlg = dlg;

	vbox1 = GTK_DIALOG(dlg)->vbox;
	gtk_box_set_spacing(GTK_BOX(vbox1), 10);

	notebook = gtk_notebook_new();
	gtk_widget_show(notebook);
	gtk_box_pack_start(GTK_BOX(vbox1), notebook, TRUE, TRUE, 0);

	table = gtk_table_new(6, 2, FALSE);
	gtk_widget_show(table);
	gtk_container_add(GTK_CONTAINER(notebook), table);
	gtk_container_border_width(GTK_CONTAINER(table), 15);
	gtk_table_set_col_spacings(GTK_TABLE(table), 40);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 2, 0, 1);

	label = gtk_label_new(ptr ? ptr+1 : "(not saved)");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	if (ptr)
	{
		hbox = gtk_hbox_new(FALSE, 0);
		gtk_widget_show(hbox);
		gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 2, 1, 2);
		*ptr = 0;
		label = gtk_label_new(opts->strFilename);
		*ptr = '/';

		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
		gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	}

	if (exist)
	{
		hbox = gtk_hbox_new(FALSE, 0);
		gtk_widget_show(hbox);
		gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 2, 2, 3);

		sprintf(text, "%.1fKB (%d bytes)", (float)buf.st_size/1024, (int)buf.st_size);
		label = gtk_label_new(text);
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
		gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_widget_show(hbox);
		gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 2, 3, 4);

		struct passwd *pwd = getpwuid(buf.st_uid);
		sprintf(text, "%s (%s)", pwd->pw_name, pwd->pw_gecos);

		label = gtk_label_new(text);
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
		gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_widget_show(hbox);
		gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 2, 4, 5);

		strcpy(text, ctime(&buf.st_mtime));
		while (text[strlen(text)-1] < '0')
			text[strlen(text)-1] = 0;
		label = gtk_label_new(text);
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
		gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

		hbox = gtk_hbox_new(FALSE, 0);
		gtk_widget_show(hbox);
		gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 2, 5, 6);

		strcpy(text, ctime(&buf.st_atime));
		while (text[strlen(text)-1] < '0')
			text[strlen(text)-1] = 0;
		label = gtk_label_new(text);
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
		gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	}

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 0, 1, (GtkAttachOptions)GTK_FILL, (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

	label = gtk_label_new("File name:");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 1, 2, (GtkAttachOptions) GTK_FILL, (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

	label = gtk_label_new("Location:");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 2, 3, (GtkAttachOptions) GTK_FILL, (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

	label = gtk_label_new("Size:");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 3, 4, (GtkAttachOptions) GTK_FILL, (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

	label = gtk_label_new("Owner:");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 4, 5, (GtkAttachOptions) GTK_FILL, (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

	label = gtk_label_new("Modified:");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 5, 6, (GtkAttachOptions) GTK_FILL, (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);

	label = gtk_label_new("Accessed:");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	vbox2 = gtk_vbox_new(FALSE, 3);
	gtk_widget_show(vbox2);
	gtk_container_add(GTK_CONTAINER(notebook), vbox2);
	gtk_container_border_width(GTK_CONTAINER(vbox2), 5);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, TRUE, 0);

	label = gtk_label_new("Author");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	s.sum_author = gtk_entry_new();
	gtk_widget_show(s.sum_author);
	gtk_box_pack_start(GTK_BOX(vbox2), s.sum_author, FALSE, FALSE, 0);
	gtk_entry_set_width_chars(GTK_ENTRY(s.sum_author), 50);
	gtk_entry_set_text(GTK_ENTRY(s.sum_author), opts->strAuthor);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

	label = gtk_label_new("Description");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	s.sum_description = gtk_entry_new();
	gtk_widget_show(s.sum_description);
	gtk_box_pack_start(GTK_BOX(vbox2), s.sum_description, FALSE, FALSE, 0);
	gtk_entry_set_width_chars(GTK_ENTRY(s.sum_description), 50);
	gtk_entry_set_text(GTK_ENTRY(s.sum_description), opts->strDescription);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);

	label = gtk_label_new("Comments");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	s.sum_comments = gtk_text_view_new();
	gtk_widget_show(s.sum_comments);
	gtk_box_pack_start(GTK_BOX(vbox2), s.sum_comments, TRUE, TRUE, 0);
	gtk_widget_set_usize(s.sum_comments, -2, 100);
	gtk_widget_realize(s.sum_comments);

	GtkTextBuffer *buffer;
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(s.sum_comments));
	gtk_text_buffer_set_text(buffer, opts->strComments, -1);

	int i, j, col[LC_MAXCOLORS], totalcount[LC_MAXCOLORS];
	memset(&totalcount, 0, sizeof(totalcount));
	for (i = 0; i < opts->lines; i++)
		for (j = 0; j < LC_MAXCOLORS; j++)
			totalcount[j] += opts->count[i*LC_MAXCOLORS+j];

	int ID = 2;
	for (i = 0; i < LC_MAXCOLORS; i++)
		if (totalcount[i])
			ID++;

	scroll_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win), 
	GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show(scroll_win);
	gtk_container_add(GTK_CONTAINER(notebook), scroll_win);

	list = gtk_clist_new(ID);
	gtk_widget_show(list);
	gtk_container_add(GTK_CONTAINER(scroll_win), list);
	gtk_container_border_width(GTK_CONTAINER(list), 5);
	gtk_clist_set_column_width(GTK_CLIST(list), 0, 80);
	gtk_clist_column_titles_show(GTK_CLIST(list));

	label = gtk_label_new("Piece");
	gtk_widget_show(label);
	gtk_clist_set_column_widget(GTK_CLIST(list), 0, label);

	for (ID = 0, i = 0; i < LC_MAXCOLORS; i++)
		if (totalcount[i])
		{
			ID++;
			col[i] = ID;

			label = gtk_label_new(colornames[i]);
			gtk_widget_show(label);
			gtk_clist_set_column_widget(GTK_CLIST(list), ID, label);
		}
	ID++;
	label = gtk_label_new("Total");
	gtk_widget_show(label);
	gtk_clist_set_column_widget(GTK_CLIST(list), ID, label);

	char* row[LC_MAXCOLORS+2];
	for (i = 0; i <= ID; i++)
		row[i] = (char*)malloc(256);

	for (i = 0; i < opts->lines; i++)
	{
		int total = 0;

		for (j = 0; j < LC_MAXCOLORS; j++)
			total += opts->count[i*LC_MAXCOLORS+j];

		if (total == 0)
			continue;

		strcpy(row[0], opts->names[i]);
		for (j = 1; j < ID; j++)
			row[j][0] = 0;

		for (j = 0; j < LC_MAXCOLORS; j++)
			if (opts->count[i*LC_MAXCOLORS+j])
				sprintf(row[col[j]], "%d", opts->count[i*LC_MAXCOLORS+j]);

		sprintf(row[ID], "%d", total);
		gtk_clist_append(GTK_CLIST(list), row);
	}
	gtk_clist_sort(GTK_CLIST(list));

	strcpy(row[0], "Total");
	int total = 0;

	for (i = 0; i < LC_MAXCOLORS; i++)
		if (totalcount[i])
		{
			sprintf(row[col[i]], "%d", totalcount[i]);
			total += totalcount[i];
		}

	sprintf(row[ID], "%d", total);
	gtk_clist_append(GTK_CLIST(list), row);

	for (i = 0; i <= ID; i++)
		free(row[i]);

	label = gtk_label_new("General");
	gtk_widget_show(label);
	set_notebook_tab(notebook, 0, label);

	label = gtk_label_new("Summary");
	gtk_widget_show(label);
	set_notebook_tab(notebook, 1, label);

	label = gtk_label_new("Pieces Used");
	gtk_widget_show(label);
	set_notebook_tab(notebook, 2, label);

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
	{
		ret = LC_OK;

		LC_PROPERTIESDLG_OPTS* opts = (LC_PROPERTIESDLG_OPTS*)s.data;

		strcpy(opts->strAuthor, gtk_entry_get_text(GTK_ENTRY(s.sum_author)));
		strcpy(opts->strDescription, gtk_entry_get_text(GTK_ENTRY(s.sum_description)));

		GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(s.sum_comments));
		GtkTextIter start;
		GtkTextIter end;
		gchar *text;

		gtk_text_buffer_get_start_iter(buffer, &start);
		gtk_text_buffer_get_end_iter(buffer, &end);
		text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
		strncpy(opts->strComments, text, sizeof(opts->strComments));
		opts->strComments[sizeof(opts->strComments)-1] = 0;
		g_free(text);
	}
	else
		ret = LC_CANCEL;

	gtk_widget_destroy(dlg);

	return ret;
}

// =============================================================================
// 'Edit Groups' dialog

typedef struct
{
	void* data;
	GtkWidget *dlg;
} LC_GROUPEDITDLG_STRUCT;

static void groupeditdlg_addchildren(GtkCTree *ctree, GtkCTreeNode *parent, Group *pGroup, LC_GROUPEDITDLG_OPTS *opts)
{
	GtkCTreeNode *newparent;
	const char *text;

	for (int i = 0; i < opts->groupcount; i++)
	{
		if (opts->groupsgroups[i] == pGroup)
		{
			text = opts->groups[i]->m_strName;
			newparent = gtk_ctree_insert_node(ctree, parent, NULL, (gchar**)&text, 0, NULL, NULL, NULL, NULL, FALSE, TRUE);
    
			groupeditdlg_addchildren(ctree, newparent, opts->groups[i], opts);
		}
	}

	for (int i = 0; i < opts->piececount; i++)
	{
		if (opts->piecesgroups[i] == pGroup)
		{
			text = opts->pieces[i]->GetName();
			gtk_ctree_insert_node(ctree, parent, NULL, (gchar**)&text, 0, NULL, NULL, NULL, NULL, TRUE, TRUE);
		}
	}
}

int groupeditdlg_execute(void* param)
{
	GtkWidget *dlg;
	GtkWidget *vbox;
	GtkWidget *ctree, *scr;
	LC_GROUPEDITDLG_STRUCT s;
	s.data = param;
	int ret;

	dlg = gtk_dialog_new_with_buttons("Edit Groups", GTK_WINDOW(((GtkWidget*)(*main_window))),
	                                  (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
	                                  GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(dlg), 5);
	gtk_widget_set_usize(dlg, 450, 280);
	s.dlg = dlg;

	vbox = GTK_DIALOG(dlg)->vbox;
	gtk_box_set_spacing(GTK_BOX(vbox), 10);

	scr = gtk_scrolled_window_new((GtkAdjustment*)NULL, (GtkAdjustment*)NULL);
	gtk_widget_show(scr);
	gtk_container_add(GTK_CONTAINER(vbox), scr);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scr), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

	ctree = gtk_ctree_new(1, 0);
	gtk_object_set_data(GTK_OBJECT(dlg), "ctree", ctree);
	gtk_widget_show(ctree);
	gtk_container_add(GTK_CONTAINER(scr), ctree);
	gtk_clist_column_titles_hide(GTK_CLIST(ctree));
	gtk_clist_set_selection_mode(GTK_CLIST(ctree), GTK_SELECTION_BROWSE);
	gtk_object_set_data(GTK_OBJECT(dlg), "tree", ctree);

	gtk_clist_freeze(GTK_CLIST(ctree));
	gtk_clist_clear(GTK_CLIST(ctree));

	groupeditdlg_addchildren(GTK_CTREE(ctree), NULL, NULL, (LC_GROUPEDITDLG_OPTS*)param);

	gtk_clist_thaw(GTK_CLIST(ctree));

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
	{
		ret = LC_OK;
	}
	else
		ret = LC_CANCEL;

	gtk_widget_destroy(dlg);

	return ret;
}

// =============================================================================
// Group Dialog

typedef struct
{
	void* data;
	GtkWidget *dlg;
	GtkWidget* entry;
} LC_GROUPDLG_STRUCT;

int groupdlg_execute(void* param)
{
	GtkWidget *dlg;
	GtkWidget *vbox;
	LC_GROUPDLG_STRUCT s;
	s.data = param;
	int ret;

	dlg = gtk_dialog_new_with_buttons("Group Name", GTK_WINDOW(((GtkWidget*)(*main_window))),
	                                  (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
	                                  GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(dlg), 5);
	s.dlg = dlg;

	vbox = GTK_DIALOG(dlg)->vbox;
	gtk_box_set_spacing(GTK_BOX(vbox), 10);

	s.entry =  gtk_entry_new_with_max_length(64);
	gtk_widget_show(s.entry);
	gtk_box_pack_start(GTK_BOX(vbox), s.entry, TRUE, FALSE, 0);
	gtk_entry_set_width_chars(GTK_ENTRY(s.entry), 30);
	gtk_entry_set_text(GTK_ENTRY(s.entry), (char*)param);

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
	{
		ret = LC_OK;

		char* name = (char*)s.data;
		strcpy(name, gtk_entry_get_text(GTK_ENTRY(s.entry)));
	}
	else
		ret = LC_CANCEL;

	gtk_widget_destroy(dlg);

	return ret;
}

// =============================================================================
// Piece Library Dialog

#include "library.h"
#include "pieceinf.h"
#include "lc_application.h"

static void librarydlg_update_list (GtkWidget *dlg)
{
	PiecesLibrary* Lib = g_App->GetPiecesLibrary();
	GtkCTree* ctree = GTK_CTREE(gtk_object_get_data(GTK_OBJECT(dlg), "tree"));
	GtkCList* clist = GTK_CLIST(gtk_object_get_data(GTK_OBJECT(dlg), "list"));
	int row;

	gchar* CategoryName = GTK_CELL_TEXT(GTK_CTREE_ROW(gtk_ctree_node_nth(ctree, GTK_CLIST(ctree)->focus_row))->row.cell[0])->text;
	int CategoryIndex = Lib->FindCategoryIndex((const char*)CategoryName);

	gtk_clist_freeze (clist);
	gtk_clist_clear (clist);

	if (CategoryIndex != -1)
	{
		PtrArray<PieceInfo> SinglePieces, GroupedPieces;

		Lib->GetCategoryEntries(CategoryIndex, false, SinglePieces, GroupedPieces);

		for (int i = 0; i < SinglePieces.GetSize(); i++)
		{
			PieceInfo* Info = SinglePieces[i];

			char *text = Info->m_strDescription;
			row = gtk_clist_append(clist, &text);
			gtk_clist_set_row_data(clist, row, Info);
		}
	}
	else
	{
		if (!strcmp(CategoryName, "Unassigned"))
		{
			// Test each piece against all categories.
			for (int i = 0; i < Lib->GetPieceCount(); i++)
			{
				PieceInfo* Info = Lib->GetPieceInfo(i);
				int j;

				for (j = 0; j < Lib->GetNumCategories(); j++)
				{
					if (Lib->PieceInCategory(Info, Lib->GetCategoryKeywords(j)))
						break;
				}

				if (j == Lib->GetNumCategories())
				{
					char *text = Info->m_strDescription;
					row = gtk_clist_append(clist, &text);
					gtk_clist_set_row_data(clist, row, Info);
				}
			}
		}
		else if (!strcmp(CategoryName, "Pieces"))
		{
			for (int i = 0; i < Lib->GetPieceCount(); i++)
			{
				PieceInfo* Info = Lib->GetPieceInfo(i);

				char *text = Info->m_strDescription;
				row = gtk_clist_append(clist, &text);
				gtk_clist_set_row_data(clist, row, Info);
			}
		}
	}

	gtk_clist_thaw(clist);
	clist->focus_row = 0;
	gtk_clist_select_row(clist, 0, 0);
}

static void librarydlg_update_tree (GtkWidget *dlg)
{
  PiecesLibrary *lib = g_App->GetPiecesLibrary();
  GtkCTree *ctree = GTK_CTREE (gtk_object_get_data (GTK_OBJECT (dlg), "tree"));
  GtkCTreeNode *parent;
  const char *text = "Pieces";

  gtk_clist_freeze (GTK_CLIST (ctree));
  gtk_clist_clear (GTK_CLIST (ctree));

  parent = gtk_ctree_insert_node (ctree, NULL, NULL, (gchar**)&text, 0, NULL, NULL, NULL, NULL, FALSE, TRUE);

  for (int i = 0; i < lib->GetNumCategories(); i++)
  {
    text = lib->GetCategoryName(i);
    gtk_ctree_insert_node (ctree, parent, NULL, (gchar**)&text, 0, NULL, NULL, NULL, NULL, TRUE, TRUE);
  }

  text = "Unassigned";
  gtk_ctree_insert_node (ctree, parent, NULL, (gchar**)&text, 0, NULL, NULL, NULL, NULL, TRUE, TRUE);

  gtk_clist_thaw (GTK_CLIST (ctree));
}

static void librarydlg_treefocus (GtkCTree *ctree, GtkCTreeNode *row, gint column, gpointer data)
{
  librarydlg_update_list (GTK_WIDGET (data));
}

static GtkWidget *last_dlg = NULL;

static void librarydlg_command (GtkWidget *widget, gpointer data)
{
  GtkWidget *parent = gtk_widget_get_toplevel (widget);
  LibraryDialog *dlg = (LibraryDialog*) gtk_object_get_data (GTK_OBJECT (parent), "menu_file_import_piece");
  int id = GPOINTER_TO_INT (data);

  dlg = (LibraryDialog *)last_dlg;

  dlg->HandleCommand (id);
}

int librarydlg_execute (void *param)
{
  GtkWidget *dlg, *vbox, *clist, *scr, *ctree, *hsplit, *item, *menu, *menubar, *handle;
  GtkAccelGroup *accel;
  int loop = 1, ret = LC_CANCEL;
  PiecesLibrary *lib = g_App->GetPiecesLibrary();

  dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (((GtkWidget*)(*main_window))));
  gtk_window_set_title (GTK_WINDOW (dlg), "Piece Library Manager");
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
                      GTK_SIGNAL_FUNC (dialog_delete_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
  gtk_object_set_data (GTK_OBJECT (dlg), "loop", &loop);
  gtk_object_set_data (GTK_OBJECT (dlg), "ret", &ret);
  gtk_window_set_default_size (GTK_WINDOW (dlg), 500, 250);

  accel = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(dlg), accel);
//  accel = gtk_accel_group_get_default ();

  vbox = gtk_vbox_new (FALSE, 5);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (dlg), vbox);

  handle = gtk_handle_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), handle, FALSE, FALSE, 0);
  gtk_widget_show (handle);

  menubar = gtk_menu_bar_new ();
  gtk_container_add (GTK_CONTAINER (handle), menubar);
  gtk_widget_show (menubar);

  // File menu
  menu = create_sub_menu (menubar, "_File", accel);
  menu_tearoff (menu);
/*
  create_menu_item (menu, "_Reset", accel,
		    GTK_SIGNAL_FUNC (librarydlg_command), GTK_OBJECT (dlg), LC_LIBDLG_FILE_RESET, "menu_file_reset");
  create_menu_item (menu, "_Open...", accel,
		    GTK_SIGNAL_FUNC (librarydlg_command), GTK_OBJECT (dlg), LC_LIBDLG_FILE_OPEN, "menu_file_open");
  create_menu_item (menu, "_Save", accel,
		    GTK_SIGNAL_FUNC (librarydlg_command), GTK_OBJECT (dlg), LC_LIBDLG_FILE_SAVE, "menu_file_save");
  create_menu_item (menu, "Save _As...", accel,
		    GTK_SIGNAL_FUNC (librarydlg_command), GTK_OBJECT (dlg), LC_LIBDLG_FILE_SAVEAS, "menu_file_save_as");
  menu_separator (menu);
  item = create_menu_item (menu, "_Print Catalog...", accel,
			   GTK_SIGNAL_FUNC (librarydlg_command), GTK_OBJECT (dlg), LC_LIBDLG_FILE_PRINTCATALOG, "menu_print_catalog");
  gtk_widget_set_sensitive (item, FALSE);
*/
  item = create_menu_item (menu, "Load _Update...", accel,
			   GTK_SIGNAL_FUNC (librarydlg_command), GTK_OBJECT (dlg), LC_LIBDLG_FILE_MERGEUPDATE, "menu_file_merge_update");
  item = create_menu_item (menu, "_Import Piece...", accel,
			   GTK_SIGNAL_FUNC (librarydlg_command), GTK_OBJECT (dlg), LC_LIBDLG_FILE_IMPORTPIECE, "menu_file_import_piece");
/*
  menu_separator (menu);
  item = create_menu_item (menu, "Re_turn", accel,
			   GTK_SIGNAL_FUNC (librarydlg_command), GTK_OBJECT (dlg), LC_LIBDLG_FILE_RETURN, "menu_file_return");
  item = create_menu_item (menu, "_Cancel", accel,
			   GTK_SIGNAL_FUNC (librarydlg_command), GTK_OBJECT (dlg), LC_LIBDLG_FILE_CANCEL, "menu_file_cancel");
  // Group menu
  menu = create_sub_menu (menubar, "_Group", accel);
  menu_tearoff (menu);
  create_menu_item (menu, "Insert...", accel,
		    GTK_SIGNAL_FUNC (librarydlg_command), GTK_OBJECT (dlg), LC_LIBDLG_GROUP_INSERT, "menu_group_insert");
  create_menu_item (menu, "Delete", accel,
		    GTK_SIGNAL_FUNC (librarydlg_command), GTK_OBJECT (dlg), LC_LIBDLG_GROUP_DELETE, "menu_group_delete");
  create_menu_item (menu, "Edit...", accel,
		    GTK_SIGNAL_FUNC (librarydlg_command), GTK_OBJECT (dlg), LC_LIBDLG_GROUP_EDIT, "menu_group_edit");
  menu_separator (menu);
  create_menu_item (menu, "Move Up", accel,
		    GTK_SIGNAL_FUNC (librarydlg_command), GTK_OBJECT (dlg), LC_LIBDLG_GROUP_MOVEUP, "menu_group_moveup");
  create_menu_item (menu, "Move Down", accel,
		    GTK_SIGNAL_FUNC (librarydlg_command), GTK_OBJECT (dlg), LC_LIBDLG_GROUP_MOVEDOWN, "menu_group_down");
  // Piece menu
  menu = create_sub_menu (menubar, "_Piece", accel);
  menu_tearoff (menu);
  item = create_menu_item (menu, "_New", accel,
			   GTK_SIGNAL_FUNC (librarydlg_command), GTK_OBJECT (dlg), LC_LIBDLG_PIECE_NEW, "menu_piece_new");
  gtk_widget_set_sensitive (item, FALSE);
  item = create_menu_item (menu, "_Edit", accel,
			   GTK_SIGNAL_FUNC (librarydlg_command), GTK_OBJECT (dlg), LC_LIBDLG_PIECE_EDIT, "menu_piece_edit");
  gtk_widget_set_sensitive (item, FALSE);
  create_menu_item (menu, "_Delete", accel,
		    GTK_SIGNAL_FUNC (librarydlg_command), GTK_OBJECT (dlg), LC_LIBDLG_PIECE_DELETE, "menu_piece_delete");
*/
  hsplit = gtk_hpaned_new ();
  gtk_paned_set_gutter_size (GTK_PANED (hsplit), 12);
  gtk_box_pack_start (GTK_BOX (vbox), hsplit, TRUE, TRUE, 0);
  gtk_widget_show (hsplit);
  gtk_container_set_border_width (GTK_CONTAINER (hsplit), 5);

  scr = gtk_scrolled_window_new ((GtkAdjustment*)NULL, (GtkAdjustment*)NULL);
  gtk_widget_show (scr);
  gtk_paned_add1 (GTK_PANED (hsplit), scr);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scr), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

  ctree = gtk_ctree_new (1, 0);
  gtk_object_set_data (GTK_OBJECT (dlg), "ctree", ctree);
  gtk_widget_show (ctree);
  gtk_container_add (GTK_CONTAINER (scr), ctree);
  gtk_clist_column_titles_hide (GTK_CLIST (ctree));
  gtk_clist_set_selection_mode (GTK_CLIST (ctree), GTK_SELECTION_BROWSE);
  gtk_signal_connect (GTK_OBJECT (ctree), "tree_select_row",
		      GTK_SIGNAL_FUNC (librarydlg_treefocus), dlg);
  gtk_widget_set_usize (ctree, -1, 200);
  gtk_object_set_data (GTK_OBJECT (dlg), "tree", ctree);

  scr = gtk_scrolled_window_new ((GtkAdjustment*)NULL, (GtkAdjustment*)NULL);
  gtk_widget_show (scr);
  gtk_paned_add2 (GTK_PANED (hsplit), scr);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scr), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

  clist = gtk_clist_new (1);
  gtk_widget_show (clist);
  gtk_clist_set_selection_mode (GTK_CLIST (clist), GTK_SELECTION_EXTENDED);
  gtk_clist_set_auto_sort (GTK_CLIST (clist), TRUE);
  gtk_container_add (GTK_CONTAINER (scr), clist);
  gtk_clist_column_titles_hide (GTK_CLIST (clist));
  gtk_paned_set_position (GTK_PANED (hsplit), 150);
  gtk_object_set_data (GTK_OBJECT (dlg), "list", clist);

  // Initialize
  gtk_object_set_data (GTK_OBJECT (dlg), "lib", &lib);

  librarydlg_update_tree (dlg);

  gtk_grab_add (dlg);
  gtk_widget_show (dlg);

  last_dlg = dlg;

  while (loop)
    gtk_main_iteration ();

  if (ret == LC_OK)
  {

  }

  gtk_grab_remove (dlg);
  gtk_widget_destroy (dlg);

  return ret;
}

// =============================================================================
// Modify Dialog

static GtkWidget* modifydlg;

static void modifydlg_update_list (Object *obj)
{
  GtkCList *clist = GTK_CLIST (gtk_object_get_data (GTK_OBJECT (modifydlg), "clist"));

  gtk_clist_freeze (clist);
  gtk_clist_clear (clist);

  if (obj != NULL)
  for (int i = 0; i < obj->GetKeyTypeCount (); i++)
  {
    const LC_OBJECT_KEY_INFO* info = obj->GetKeyTypeInfo (i);
    const float *value = obj->GetKeyTypeValue (i);
    const char *text[2];
    char buf[64], tmp[16];

    text[0] = info->description;
    text[1] = buf;

    for (int j = 0; j < info->size; j++)
    {
      if (j == 0)
        strcpy (buf, "");
      else
        strcat (buf, " ");

      sprintf (tmp, "%.2f", value[j]);
      strcat (buf, tmp);
    }

    gtk_clist_append (clist, (char**)text);
    //      row = gtk_clist_append (clist, &text);
    //      gtk_clist_set_row_data (clist, row, GINT_TO_POINTER (i));
  }

  gtk_clist_thaw (clist);
}

static void modifydlg_listener (int message, void *data, void *user)
{
  if (message == LC_MSG_FOCUS_CHANGED)
  {
    modifydlg_update_list ((Object*)data);
  }
}

static void modifydlg_create ()
{
  GtkWidget *dlg, *vbox, *hbox, *scr, *clist, *button, *entry;

  dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (((GtkWidget*)(*main_window))));
  gtk_window_set_title (GTK_WINDOW (dlg), "Modify");
  //  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
  //                      GTK_SIGNAL_FUNC (gtk_widget_hide), NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
  gtk_window_set_default_size (GTK_WINDOW (dlg), 250, 250);

  vbox = gtk_vbox_new (FALSE, 5);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (dlg), vbox);
  gtk_container_border_width (GTK_CONTAINER (vbox), 5);

  scr = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scr), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_show (scr);
  gtk_box_pack_start (GTK_BOX (vbox), scr, TRUE, TRUE, 0);

  clist = gtk_clist_new (2);
  gtk_widget_show (clist);
  gtk_container_add (GTK_CONTAINER (scr), clist);
  //  gtk_clist_set_column_width (GTK_CLIST (list), 0, 80);
  gtk_clist_column_titles_show (GTK_CLIST (clist));
  gtk_clist_set_column_title (GTK_CLIST (clist), 0, "Property");
  gtk_clist_set_column_title (GTK_CLIST (clist), 1, "Value");
  gtk_clist_set_column_auto_resize (GTK_CLIST (clist), 0, TRUE);
  gtk_clist_set_selection_mode (GTK_CLIST (clist), GTK_SELECTION_BROWSE);
  gtk_object_set_data (GTK_OBJECT (dlg), "clist", clist);

  hbox = gtk_hbox_new (TRUE, 5);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  entry = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, TRUE, 0);
  gtk_widget_set_usize (entry, 50, -2);

  entry = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, TRUE, 0);
  gtk_widget_set_usize (entry, 50, -2);

  entry = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, TRUE, 0);
  gtk_widget_set_usize (entry, 50, -2);

  entry = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, TRUE, 0);
  gtk_widget_set_usize (entry, 50, -2);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  button = gtk_button_new_with_label ("Close");
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  //  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (), );

  button = gtk_button_new_with_label ("Apply");
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  //  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (), );


  modifydlg = dlg;
  messenger->Listen (&modifydlg_listener, NULL);
}

void modifydlg_toggle ()
{
  if (modifydlg == NULL)
    modifydlg_create ();

  if (GTK_WIDGET_VISIBLE (modifydlg))
    gtk_widget_hide (modifydlg);
  else
    gtk_widget_show (modifydlg);
}
