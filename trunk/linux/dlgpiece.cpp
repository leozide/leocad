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
#include <string.h>
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

// New piece was selected
static void minifigdlg_piece_changed (GtkWidget *widget)
{
  LC_MINIFIGDLG_STRUCT* info;
  PieceInfo* piece_info = NULL;
  int i, j, piece_type;
  char* desc;

  info = (LC_MINIFIGDLG_STRUCT*)gtk_object_get_data (GTK_OBJECT (widget), "info");
  if (info == NULL)
    return;

  for (i = 0; i < 15; i++)
    if (GTK_COMBO (info->pieces[i])->entry == widget)
    {
      piece_type = i;
      break;
    }

  desc = gtk_entry_get_text (GTK_ENTRY (widget));

  for (j = 0; j < MFW_PIECES; j++)
  {
    if (strcmp (desc, mfwpieceinfo[j].description) == 0)
    {
      piece_info = project->FindPieceInfo(mfwpieceinfo[j].name);
      if (piece_info == NULL)
	continue;

      if (info->opts->info[i])
	info->opts->info[i]->DeRef();
      info->opts->info[i] = piece_info;
      piece_info->AddRef();
      break;
    }
  }

  // Piece not found ("None")
  if (j == MFW_PIECES)
  {
    if (info->opts->info[i])
      info->opts->info[i]->DeRef();
    info->opts->info[i] = NULL;
  }

  // Get the pieces in the right place
  // TODO: Find a way to make this cross-platform
  if (i == MFW_NECK)
  {
    if (info->opts->info[3] != NULL)
    {
      info->opts->pos[0][2] = 3.92f;
      info->opts->pos[1][2] = 3.92f;
 
      if (strcmp (piece_info->m_strName,"4498") == 0)
	info->opts->rot[3][2] = 180.0f;
      else
	info->opts->rot[3][2] = 0.0f;
    }
    else
    {
      info->opts->pos[0][2] = 3.84f;
      info->opts->pos[1][2] = 3.84f;
    }
  }

  if (i == MFW_LEFT_SHOE)
  {
    if (strcmp (desc, "Ski"))
      info->opts->pos[13][1] = 0;
    else
      info->opts->pos[13][1] = -0.12f;
  }

  if (i == MFW_RIGHT_SHOE)
  {
    if (strcmp (desc, "Ski"))
      info->opts->pos[14][1] = 0;
    else
      info->opts->pos[14][1] = -0.12f;
  }

  if ((i == MFW_LEFT_TOOL) || (i == MFW_RIGHT_TOOL))
    if (piece_info != NULL)
    {
      float rx = 45, ry = 0, rz = 0, x = 0.92f, y = -0.62f, z = 1.76f;

      if (strcmp (piece_info->m_strName,"4529") == 0)
	{ rx = -45; y = -1.14f; z = 2.36f; }
      if (strcmp (piece_info->m_strName,"3899") == 0)
	{ y = -1.64f; z = 1.38f; }
      if (strcmp (piece_info->m_strName,"4528") == 0)
	{ rx = -45; y = -1.26f; z = 2.36f; }
      if (strcmp (piece_info->m_strName,"4479") == 0)
	{ rz = 90; y = -1.22f; z = 2.44f; }
      if (strcmp (piece_info->m_strName,"3962") == 0)
	{ rz = 90; y = -0.7f; z = 1.62f; }
      if (strcmp (piece_info->m_strName,"4360") == 0)
	{ rz = -90; y = -1.22f; z = 2.44f; }
      if (strncmp (piece_info->m_strName,"6246",4) == 0)
	{ y = -1.82f; z = 2.72f; rz = 90; }
      if (strcmp (piece_info->m_strName,"4349") == 0)
	{ y = -1.16f; z = 2.0f; }
      if (strcmp (piece_info->m_strName,"4479") == 0)
	{ y = -1.42f; z = 2.26f; }
      if (strcmp (piece_info->m_strName,"3959") == 0)
	{ y = -1.0f; z = 1.88f; }
      if (strcmp (piece_info->m_strName,"4522") == 0)
	{ y = -1.64f; z = 2.48f; }
      if (strcmp (piece_info->m_strName,"194") == 0)
	{ rz = 180; y = -1.04f; z = 1.94f; }
      if (strcmp (piece_info->m_strName,"4006") == 0)
	{ rz = 180; y = -1.24f; z = 2.18f; }
      if (strcmp (piece_info->m_strName,"6246C") == 0)
	{ rx = 35; rz = 0; y = -2.36f; z = 1.08f; }
      if (strcmp (piece_info->m_strName,"4497") == 0)
	{ y = -2.16f; z = 3.08f; rz = 90; }
      if (strcmp (piece_info->m_strName,"30092") == 0)
	{ x = 0; rz = 180; }
      if (strcmp (piece_info->m_strName,"37") == 0)
	{ z = 1.52f; y = -0.64f; }
      if (strcmp (piece_info->m_strName,"38") == 0)
	{ z = 1.24f; y = -0.34f; }
      if (strcmp (piece_info->m_strName,"3841") == 0)
	{ z = 2.24f; y = -1.34f; rz = 180; }
      if (strcmp (piece_info->m_strName,"4499") == 0)
	{ rz = ((i == MFW_RIGHT_TOOL) ? 10 : -10); z = 1.52f; }
      if (strcmp (piece_info->m_strName,"3852") == 0)
	{ rz = -90; x = 0.90f; y = -0.8f; z = 1.84f; }
      if (strcmp (piece_info->m_strName,"30152") == 0)
	{ z = 3.06f; y = -2.16f; }                                      

      if (i == MFW_RIGHT_TOOL)
	x = -x;

      info->opts->pos[i][0] = x;
      info->opts->pos[i][1] = y;
      info->opts->pos[i][2] = z;
      info->opts->rot[i][0] = rx;
      info->opts->rot[i][1] = ry;
      info->opts->rot[i][2] = rz;
    }

  gtk_widget_draw (info->preview, NULL);
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
  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (combo)->entry), "changed",
                      GTK_SIGNAL_FUNC (minifigdlg_piece_changed), NULL);
  gtk_object_set_data (GTK_OBJECT (GTK_COMBO (combo)->entry), "info", info);

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

// sort the names from the combo boxes
static gint minifigdlg_compare (gconstpointer a, gconstpointer b)
{
  return strcmp ((const char*)a, (const char*)b);
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
  GList* names[15];
  for (i = 0; i < 15; i++)
    names[i] = NULL;

  for (i = 0; i < MFW_PIECES; i++)
  {
    PieceInfo* piece_info;
    int id;

    piece_info = project->FindPieceInfo(mfwpieceinfo[i].name);
    if (piece_info == NULL)
      continue;

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
      names[id] = g_list_insert_sorted (names[id], mfwpieceinfo[i].description, minifigdlg_compare);

    if (id == MFW_LEFT_HAND || id == MFW_LEFT_TOOL || id == MFW_LEFT_SHOE)
      names[id+1] = g_list_insert_sorted (names[id+1], mfwpieceinfo[i].description, minifigdlg_compare);

    if (i == 6) i++;
  }

  names[MFW_HAT] = g_list_prepend (names[MFW_HAT], "None");
  names[MFW_NECK] = g_list_prepend (names[MFW_NECK], "None");
  names[MFW_LEFT_TOOL] = g_list_prepend (names[MFW_LEFT_TOOL], "None");
  names[MFW_RIGHT_TOOL] = g_list_prepend (names[MFW_RIGHT_TOOL], "None");
  names[MFW_LEFT_SHOE] = g_list_prepend (names[MFW_LEFT_SHOE], "None");
  names[MFW_RIGHT_SHOE] = g_list_prepend (names[MFW_RIGHT_SHOE], "None");

  for (i = 0; i < 15; i++)
    gtk_combo_set_popdown_strings ( GTK_COMBO (s.pieces[i]), names[i]);

  gtk_list_select_item ( GTK_LIST (GTK_COMBO (s.pieces[MFW_HAT])->list), 6);
  gtk_list_select_item ( GTK_LIST (GTK_COMBO (s.pieces[MFW_HEAD])->list), 4);
  gtk_list_select_item ( GTK_LIST (GTK_COMBO (s.pieces[MFW_TORSO])->list), 19);

  return dlg_domodal(dlg, LC_CANCEL);
}





