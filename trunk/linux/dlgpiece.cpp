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
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <stdio.h>
#include "gtkglarea.h"
#include "gtktools.h"
#include "system.h"
#include "typedefs.h"
#include "globals.h"
#include "dialogs.h"
#include "matrix.h"
#include "pieceinf.h"
#include "project.h"
#include "main.h"

// =========================================================
// Minifig Wizard

typedef enum {
  MFW_HAT,
  MFW_HEAD,
  MFW_TORSO,
  MFW_NECK,
  MFW_LEFT_ARM,
  MFW_RIGHT_ARM,
  MFW_LEFT_HAND,
  MFW_RIGHT_HAND,
  MFW_LEFT_TOOL,
  MFW_RIGHT_TOOL,
  MFW_HIPS,
  MFW_LEFT_LEG,
  MFW_RIGHT_LEG,
  MFW_LEFT_SHOE,
  MFW_RIGHT_SHOE,
  MFW_NUMITEMS
};

typedef struct
{
  LC_MINIFIGDLG_OPTS* opts;
  GtkWidget *pieces[MFW_NUMITEMS];
  GtkWidget *colors[MFW_NUMITEMS];
  GtkWidget *preview;
} LC_MINIFIGDLG_STRUCT;

static gint minifigdlg_redraw (GtkWidget *widget, GdkEventExpose *event)
{
  LC_MINIFIGDLG_STRUCT* data;
  int i;

  // Draw only last expose.
  if (event->count > 0)
    return TRUE;

  data = (LC_MINIFIGDLG_STRUCT*)gtk_object_get_data (GTK_OBJECT (widget), "minifig");

  if (!data)
    return true;

  if (!gtk_gl_area_make_current(GTK_GL_AREA(widget)))
    return TRUE;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  for (i = 0; i < 15; i++)
  {
    if (data->opts->info[i] == NULL)
      continue;

    glPushMatrix();
    Matrix mat;
    float rot[4];
    mat.CreateOld(0,0,0, data->opts->rot[i][0], data->opts->rot[i][1], data->opts->rot[i][2]);
    mat.ToAxisAngle(rot);
    glTranslatef(data->opts->pos[i][0], data->opts->pos[i][1], data->opts->pos[i][2]);
    glRotatef(rot[3], rot[0], rot[1], rot[2]);
    data->opts->info[i]->RenderPiece(data->opts->colors[i]);
    glPopMatrix();
  }

  glFinish();
  gtk_gl_area_swapbuffers(GTK_GL_AREA(widget));
  gtk_gl_area_make_current(GTK_GL_AREA(drawing_area));                          

  return TRUE;
}

// Setup the OpenGL projection
static gint minifigdlg_resize (GtkWidget *widget, GdkEventConfigure *event)
{
  if (!gtk_gl_area_make_current(GTK_GL_AREA(widget)))
    return TRUE;

  float aspect = (float)widget->allocation.width/(float)widget->allocation.height;
  glViewport(0, 0, widget->allocation.width, widget->allocation.height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(30.0f, aspect, 1.0f, 20.0f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
	
  gluLookAt (0, -9, 4, 0, 5, 1, 0, 0, 1);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  float *bg = project->GetBackgroundColor();
  glClearColor(bg[0], bg[1], bg[2], bg[3]);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable (GL_DITHER);
  glShadeModel (GL_FLAT);

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

  info->opts->colors[i] = (int)data;
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

// Create a combo box with a color selection control
static void minifigdlg_createpair (LC_MINIFIGDLG_STRUCT* info, int num, GtkWidget* vbox)
{
  GtkWidget *hbox, *combo, *color;

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  combo = info->pieces[num] = gtk_combo_new ();
  gtk_widget_show (combo);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_set_usize (combo, 60, 25);
  gtk_entry_set_editable (GTK_ENTRY (GTK_COMBO (combo)->entry), FALSE);

  color = info->colors[num] = gtk_button_new_with_label ("");
  gtk_widget_set_events (color, GDK_EXPOSURE_MASK);
  gtk_widget_show (color);
  gtk_object_set_data (GTK_OBJECT (color), "color", &info->opts->colors[num]);
  gtk_object_set_data (GTK_OBJECT (color), "info", info);
  gtk_widget_set_usize (color, 40, 25);
  gtk_signal_connect (GTK_OBJECT (color), "expose_event",
		      GTK_SIGNAL_FUNC (minifigdlg_color_expose), NULL);
  gtk_signal_connect (GTK_OBJECT (color), "clicked",
		      GTK_SIGNAL_FUNC (minifigdlg_color_clicked), info);
  gtk_box_pack_start (GTK_BOX (hbox), color, FALSE, TRUE, 0);
}

int minifigdlg_execute(void* param)
{
  int attrlist[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 16, 0 };
  LC_MINIFIGDLG_STRUCT s;
  GtkWidget *dlg;
  GtkWidget *vbox1, *vbox2, *hbox;
  GtkWidget *button;
  int i;

  s.opts = (LC_MINIFIGDLG_OPTS*)param;

  dlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
		      GTK_SIGNAL_FUNC (dlg_delete_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
  gtk_widget_set_usize (dlg, 600, 360);
  gtk_window_set_title (GTK_WINDOW (dlg), "Minifig Wizard");
  gtk_window_set_policy (GTK_WINDOW (dlg), FALSE, FALSE, FALSE);
  gtk_widget_realize (dlg);

  vbox1 = gtk_vbox_new (FALSE, 10);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (dlg), vbox1);
  gtk_container_border_width (GTK_CONTAINER (vbox1), 5);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, TRUE, 0);

  vbox2 = gtk_vbox_new (FALSE, 5);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);

  minifigdlg_createpair (&s, MFW_HAT, vbox2);
  minifigdlg_createpair (&s, MFW_NECK, vbox2);
  minifigdlg_createpair (&s, MFW_RIGHT_ARM, vbox2);
  minifigdlg_createpair (&s, MFW_RIGHT_HAND, vbox2);
  minifigdlg_createpair (&s, MFW_RIGHT_TOOL, vbox2);
  minifigdlg_createpair (&s, MFW_HIPS, vbox2);
  minifigdlg_createpair (&s, MFW_RIGHT_LEG, vbox2);
  minifigdlg_createpair (&s, MFW_RIGHT_SHOE, vbox2);

  // Create new OpenGL widget.
  s.preview = gtk_gl_area_share_new (attrlist, GTK_GL_AREA (drawing_area));
  gtk_widget_set_events (GTK_WIDGET (s.preview), GDK_EXPOSURE_MASK);

  gtk_signal_connect (GTK_OBJECT (s.preview), "expose_event", 
      GTK_SIGNAL_FUNC (minifigdlg_redraw), NULL);
  gtk_signal_connect (GTK_OBJECT (s.preview), "configure_event",
      GTK_SIGNAL_FUNC (minifigdlg_resize), NULL);

  gtk_widget_set_usize (GTK_WIDGET (s.preview), 100, 300);
  gtk_container_add (GTK_CONTAINER (hbox), GTK_WIDGET (s.preview));
  gtk_widget_show (GTK_WIDGET (s.preview));
  gtk_object_set_data (GTK_OBJECT (s.preview), "minifig", &s);

  vbox2 = gtk_vbox_new (FALSE, 5);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);

  minifigdlg_createpair (&s, MFW_HEAD, vbox2);
  minifigdlg_createpair (&s, MFW_TORSO, vbox2);
  minifigdlg_createpair (&s, MFW_LEFT_ARM, vbox2);
  minifigdlg_createpair (&s, MFW_LEFT_HAND, vbox2);
  minifigdlg_createpair (&s, MFW_LEFT_TOOL, vbox2);
  minifigdlg_createpair (&s, MFW_LEFT_LEG, vbox2);
  minifigdlg_createpair (&s, MFW_LEFT_SHOE, vbox2);

  hbox = gtk_hbox_new (FALSE, 10);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, TRUE, 0);
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
  for (i = 0; i < MFW_PIECES; i++)
  {
    PieceInfo* piece_info = project->FindPieceInfo(mfwpieceinfo[i].name);

    if (piece_info != NULL)
    {
      GtkWidget *list, *item;
      int id;

      switch (mfwpieceinfo[i].type)
      {
      case MF_HAT:   id = MFW_HAT; break;
      case MF_HEAD:  id = MFW_HEAD; break;
      case MF_TORSO: id = MFW_TORSO; break;
      case MF_NECK:  id = MFW_NECK; break;
      case MF_ARML:  id = MFW_LEFT_ARM; break;
      case MF_ARMR:  id = MFW_RIGHT_ARM; break;
      case MF_HAND:  id = MFW_LEFT_HAND; break;
      case MF_TOOL:  id = MFW_LEFT_TOOL; break;
      case MF_HIPS:  id = MFW_HIPS; break;
      case MF_LEGL:  id = MFW_LEFT_LEG; break;
      case MF_LEGR:  id = MFW_RIGHT_LEG; break;
      case MF_SHOE:  id = MFW_LEFT_SHOE; break;
      default:
	continue;
      }

      if (i != 29)
      {
	list = GTK_COMBO (s.pieces[id])->list;
	item = gtk_list_item_new_with_label(mfwpieceinfo[i].description);
	gtk_object_set_data (GTK_OBJECT (item), "pieceinfo", piece_info);
	gtk_widget_show(item);
	gtk_container_add (GTK_CONTAINER(list), item);
      }

      if (id == MFW_LEFT_HAND || id == MFW_LEFT_TOOL || id == MFW_LEFT_SHOE)
      {
	list = GTK_COMBO (s.pieces[id+1])->list;
	item = gtk_list_item_new_with_label(mfwpieceinfo[i].description);
	gtk_object_set_data (GTK_OBJECT (item), "pieceinfo", piece_info);
	gtk_widget_show(item);
	gtk_container_add (GTK_CONTAINER(list), item);
      }

      if (i == 6) i++;
    }
  }

  return dlg_domodal(dlg, LC_CANCEL);
}





