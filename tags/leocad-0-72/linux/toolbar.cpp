// Toolbar creation and related functions
//

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <stdio.h>
#include "gtktools.h"
#include "gtkglarea.h"
#include "main.h"
#include "globals.h" 
#include "project.h"
#include "pieceinf.h"
#include "toolbar.h"
#include "custom.h"

// =========================================================

// Variables
GtkWidget *piecepreview;
GtkWidget *piecelist;
GtkWidget *piececombo;
GtkWidget *colorlist;
GtkWidget *grouptoolbar;

TOOL_TOOLBAR tool_toolbar;
MAIN_TOOLBAR main_toolbar;
ANIM_TOOLBAR anim_toolbar;

// =========================================================

void create_toolbars(GtkWidget *window, GtkWidget *vbox)
{
#include "pixmaps/ac-brick.xpm"
#include "pixmaps/ac-light.xpm"
#include "pixmaps/ac-spot.xpm"
#include "pixmaps/ac-cam.xpm"
#include "pixmaps/ac-sel.xpm"
#include "pixmaps/ac-move.xpm"
#include "pixmaps/ac-rot.xpm"
#include "pixmaps/ac-erase.xpm"
#include "pixmaps/ac-paint.xpm"
#include "pixmaps/ac-zoom.xpm"
#include "pixmaps/ac-pan.xpm"
#include "pixmaps/ac-rotv.xpm"
#include "pixmaps/ac-roll.xpm"
#include "pixmaps/ac-zoomr.xpm"
#include "pixmaps/ac-zoome.xpm"
#include "pixmaps/ac-prev.xpm"
#include "pixmaps/ac-next.xpm"
#include "pixmaps/an-anim.xpm"
#include "pixmaps/an-key.xpm"
#include "pixmaps/an-next.xpm"
#include "pixmaps/an-prev.xpm"
#include "pixmaps/an-first.xpm"
#include "pixmaps/an-last.xpm"
#include "pixmaps/an-play.xpm"
#include "pixmaps/an-stop.xpm"
#include "pixmaps/st-about.xpm"
#include "pixmaps/st-fast.xpm"
#include "pixmaps/st-paste.xpm"
#include "pixmaps/st-save.xpm"
#include "pixmaps/st-bg.xpm"
#include "pixmaps/st-help.xpm"
#include "pixmaps/st-prev.xpm"
#include "pixmaps/st-snap.xpm"
#include "pixmaps/st-copy.xpm"
#include "pixmaps/st-new.xpm"
#include "pixmaps/st-print.xpm"
#include "pixmaps/st-snapa.xpm"
#include "pixmaps/st-cut.xpm"
#include "pixmaps/st-open.xpm"
#include "pixmaps/st-redo.xpm"
#include "pixmaps/st-undo.xpm"

  GtkWidget *button;

  // Main Toolbar
  main_toolbar.handle_box = gtk_handle_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), main_toolbar.handle_box, FALSE, FALSE, 0);
  if (user_rc.view_main_toolbar)
    gtk_widget_show (main_toolbar.handle_box);
  main_toolbar.toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_style (GTK_TOOLBAR (main_toolbar.toolbar), user_rc.toolbar_style);
  gtk_container_add (GTK_CONTAINER (main_toolbar.handle_box), main_toolbar.toolbar);
  gtk_widget_show (main_toolbar.toolbar);

  gtk_container_border_width (GTK_CONTAINER (main_toolbar.toolbar), 2);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (main_toolbar.toolbar), GTK_RELIEF_NONE);
  //  gtk_toolbar_set_space_style (GTK_TOOLBAR (main_toolbar.toolbar), GTK_TOOLBAR_SPACE_LINE);
  //  gtk_toolbar_set_space_size (GTK_TOOLBAR (main_toolbar.toolbar), 10);

  gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar.toolbar), "New",
     "Create a new project", "", new_pixmap (window, st_new),
     GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_FILE_NEW);
  gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar.toolbar), "Open",
     "Open an existing project", "", new_pixmap (window, st_open),
     GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_FILE_OPEN);
  gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar.toolbar), "Save",
     "Save the active project", "", new_pixmap (window, st_save), 
     GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_FILE_SAVE);
  button = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar.toolbar), "Print",
     " ", "", new_pixmap (window, st_print), GTK_SIGNAL_FUNC (OnCommand), NULL);
  gtk_widget_set_sensitive (button, FALSE);
  button = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar.toolbar), "Preview",
     " ", "", new_pixmap (window, st_prev), GTK_SIGNAL_FUNC (OnCommand), NULL);
  gtk_widget_set_sensitive (button, FALSE);
  main_toolbar.cut = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar.toolbar), "Cut",
     "Cut the selection", "", new_pixmap (window, st_cut), 
     GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_EDIT_CUT);
  main_toolbar.copy = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar.toolbar), "Copy",
     "Copy the selection", "", new_pixmap (window, st_copy),
     GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_EDIT_COPY);
  main_toolbar.paste = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar.toolbar), "Paste",
     "Insert Clipboard contents", "", new_pixmap (window, st_paste),
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_EDIT_PASTE);
  main_toolbar.undo = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar.toolbar), "Undo",
     "Undo last action", "", new_pixmap (window, st_undo),
     GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_EDIT_UNDO);
  main_toolbar.redo = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar.toolbar), "Redo",
     "Redo the last undone action", "", new_pixmap (window, st_redo),
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_EDIT_REDO);
  main_toolbar.snap = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar.toolbar), "Snap",
     "Toggle 3D snap", "", new_pixmap (window, st_snap), GTK_SIGNAL_FUNC (OnCommand), NULL);
  main_toolbar.angle = gtk_toolbar_append_element (GTK_TOOLBAR (main_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL, "Angle", "Toggle angle snap", "", 
     new_pixmap (window, st_snapa), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_SNAP_A);
  main_toolbar.fast = gtk_toolbar_append_element (GTK_TOOLBAR (main_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL, "Fast", "Fast rendering", "", 
     new_pixmap (window, st_fast), GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_TOOLBAR_FASTRENDER);
  main_toolbar.bg = gtk_toolbar_append_element (GTK_TOOLBAR (main_toolbar.toolbar),
     GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL, "Backgnd", "Backgroung rendering", "", 
     new_pixmap (window, st_bg), GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_TOOLBAR_BACKGROUND);
  gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar.toolbar), "About", "About LeoCAD", "", 
     new_pixmap (window, st_about), GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_HELP_ABOUT);
  gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar.toolbar), "Help", "Help", "",
     new_pixmap (window, st_help), GTK_SIGNAL_FUNC (OnCommand), NULL);
  gtk_widget_set_sensitive (button, FALSE);

  // Tools Toolbar
  tool_toolbar.handle_box = gtk_handle_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox),tool_toolbar. handle_box, FALSE, FALSE, 0);
  if (user_rc.view_tool_toolbar)
    gtk_widget_show (tool_toolbar.handle_box);

  tool_toolbar.toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_style (GTK_TOOLBAR (tool_toolbar.toolbar), user_rc.toolbar_style);
  gtk_container_add (GTK_CONTAINER (tool_toolbar.handle_box), tool_toolbar.toolbar);
  gtk_widget_show (tool_toolbar.toolbar);

  gtk_container_border_width (GTK_CONTAINER (tool_toolbar.toolbar), 2);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (tool_toolbar.toolbar), GTK_RELIEF_NONE);

  tool_toolbar.brick = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, NULL, "Piece", "Insert Piece", "",
     new_pixmap (window, ac_brick), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_INSERT);
  tool_toolbar.light = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Light", "Insert Light", "", 
     new_pixmap (window, ac_light), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_LIGHT);
  tool_toolbar.spot = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Spot", "Insert Spotlight", "",
     new_pixmap (window, ac_spot), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_SPOTLIGHT);
  tool_toolbar.camera = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Camera", "Insert Camera", "",
     new_pixmap (window, ac_cam),  GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_CAMERA);
  tool_toolbar.select = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Select", "Select Objects", "",
     new_pixmap (window, ac_sel), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_SELECT);
  tool_toolbar.move = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Move", "Move Objects", "",
     new_pixmap (window, ac_move), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_MOVE);
  tool_toolbar.rotate = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Rotate", "Rotate Pieces", "",
     new_pixmap (window, ac_rot), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_ROTATE);
  tool_toolbar.erase = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Delete", "Remove Objects", "",
     new_pixmap (window, ac_erase), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_ERASER);
  tool_toolbar.paint = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Paint", "Paint Bricks", "",
     new_pixmap (window, ac_paint), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_PAINT);
  tool_toolbar.zoom = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Zoom", "Zoom", "",
     new_pixmap (window, ac_zoom), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_ZOOM);
  tool_toolbar.pan = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Pan", "Pan", "",
     new_pixmap (window, ac_pan), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_PAN);
  tool_toolbar.rotview = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Rot. View", "Rotate View", "",
     new_pixmap (window, ac_rotv), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_ROTATE_VIEW);
  tool_toolbar.roll = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Roll", "Roll", "",
     new_pixmap (window, ac_roll), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_ROLL);
  tool_toolbar.zoomreg = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Zoom Box", "Zoom Region", "",
     new_pixmap (window, ac_zoomr), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_ZOOM_REGION);
  gtk_toolbar_append_item (GTK_TOOLBAR (tool_toolbar.toolbar), "Zoom Ext.", "Zoom Extents", "",
     new_pixmap (window, ac_zoome), GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_VIEW_ZOOMEXTENTS);
  tool_toolbar.prev = gtk_toolbar_append_item (GTK_TOOLBAR (tool_toolbar.toolbar), "", "", "",
     new_pixmap (window, ac_prev), GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_PIECE_PREVIOUS);
  tool_toolbar.next = gtk_toolbar_append_item (GTK_TOOLBAR (tool_toolbar.toolbar), "", "", "",
     new_pixmap (window, ac_next), GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_PIECE_NEXT);

  // Animation Toolbar
  anim_toolbar.handle_box = gtk_handle_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), anim_toolbar.handle_box, FALSE, FALSE, 0);
  if (user_rc.view_anim_toolbar)
    gtk_widget_show (anim_toolbar.handle_box);

  anim_toolbar.toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
//  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), user_rc.toolbar_style);
  gtk_container_add (GTK_CONTAINER (anim_toolbar.handle_box), anim_toolbar.toolbar);
  gtk_widget_show (anim_toolbar.toolbar);

  gtk_container_border_width (GTK_CONTAINER (anim_toolbar.toolbar), 2);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (anim_toolbar.toolbar), GTK_RELIEF_NONE);

  anim_toolbar.first = gtk_toolbar_append_item (GTK_TOOLBAR (anim_toolbar.toolbar),
     "First", "Go to the Start", "", new_pixmap (window, an_first), 
     GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_VIEW_STEP_FIRST);
  anim_toolbar.prev = gtk_toolbar_append_item (GTK_TOOLBAR (anim_toolbar.toolbar),
     "Previous", "Go Back", "", new_pixmap (window, an_prev),
     GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_VIEW_STEP_PREVIOUS);
  anim_toolbar.play = gtk_toolbar_append_item (GTK_TOOLBAR (anim_toolbar.toolbar),
     "Play", "Play Animation", "", new_pixmap (window, an_play),
     GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_VIEW_PLAY);
  anim_toolbar.stop = gtk_toolbar_append_item (GTK_TOOLBAR (anim_toolbar.toolbar),
     "Stop", "Stop Animation", "", new_pixmap (window, an_stop),
     GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_VIEW_STOP);
  anim_toolbar.next = gtk_toolbar_append_item (GTK_TOOLBAR (anim_toolbar.toolbar),
     "Next", "Go Forward", "", new_pixmap (window, an_next),
     GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_VIEW_STEP_NEXT);
  anim_toolbar.last = gtk_toolbar_append_item (GTK_TOOLBAR (anim_toolbar.toolbar),
     "Last", "Go to the End", "", new_pixmap (window, an_last),
     GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_VIEW_STEP_LAST);
  anim_toolbar.anim = gtk_toolbar_append_element (GTK_TOOLBAR (anim_toolbar.toolbar),
     GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL, "Mode", "Toggle Animation or Instructions", "",
     new_pixmap (window, an_anim), GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_TOOLBAR_ANIMATION);
  anim_toolbar.keys = gtk_toolbar_append_element (GTK_TOOLBAR (anim_toolbar.toolbar),
     GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL, "Keys", "Add Keys", "",
     new_pixmap (window, an_key), GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_TOOLBAR_ADDKEYS);
}

// =========================================================

// Pieces toolbar

static bool list_subparts = false;
static bool list_groups = true;
static int  list_curgroup;
static int  piecelist_col_sort = 0;
static bool piecelist_ascending = true; 
static PieceInfo* piece_info = NULL;
static int cur_color = 0;
static GdkPixmap* colorlist_pixmap = NULL;
static GtkWidget* list_arrows[2];
static GtkWidget* groups[9];

// piece_preview drawing
static gint draw_preview(GtkWidget *widget, GdkEventExpose *event)
{
  // Draw only last expose.
  if (event->count > 0)
    return TRUE;

  if (piece_info == NULL)
    return TRUE;

  if (!gtk_gl_area_make_current(GTK_GL_AREA(widget)))
    return TRUE;

  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(0.5f, 0.1f);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
  glDisable (GL_DITHER);
  glShadeModel (GL_FLAT);
  
  double aspect = (float)widget->allocation.width/(float)widget->allocation.height;
  glViewport(0,0, widget->allocation.width, widget->allocation.height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(30.0f, aspect, 1.0f, 100.0f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  piece_info->ZoomExtents();

  float pos[4] = { 0, 0, 10, 0 }, *bg = project->GetBackgroundColor();
  glLightfv(GL_LIGHT0, GL_POSITION, pos);
  glClearColor(bg[0], bg[1], bg[2], bg[3]);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  piece_info->RenderPiece(project->GetCurrentColor());
  
  glFinish();
  gtk_gl_area_swapbuffers(GTK_GL_AREA(widget));
  gtk_gl_area_make_current(GTK_GL_AREA(drawing_area));

  return TRUE;
}

// Called when the user clicked on the header of the piecelist
static void piecelist_setsort (GtkCList* clist, gint column)
{
  if (piecelist_col_sort == column)
    piecelist_ascending = !piecelist_ascending;
  else
  {
    gtk_widget_hide (list_arrows[piecelist_col_sort]);
    gtk_widget_show (list_arrows[column]);
    piecelist_ascending = true;
  }
  piecelist_col_sort = column;

  gtk_arrow_set (GTK_ARROW (list_arrows[column]), piecelist_ascending
		 ? GTK_ARROW_DOWN : GTK_ARROW_UP, GTK_SHADOW_IN);

  gtk_clist_set_sort_column (GTK_CLIST(piecelist), column);
  gtk_clist_set_sort_type (GTK_CLIST(piecelist), piecelist_ascending
			   ? GTK_SORT_ASCENDING : GTK_SORT_DESCENDING);
  gtk_clist_sort (GTK_CLIST(piecelist));
}

static void fill_piecelist(int group)
{
  gtk_clist_freeze(GTK_CLIST(piecelist));
  gtk_clist_clear(GTK_CLIST(piecelist));

  for (int i = 0; i < project->GetPieceLibraryCount(); i++)
  {
    PieceInfo* pInfo = project->GetPieceInfo(i);

    if ((pInfo->m_strDescription[0] == '~') && !list_subparts)
      continue;

    if ((!list_groups) || ((pInfo->m_nGroups & (long)(1 << group)) != 0))
    {
      char* dummy[] = { pInfo->m_strDescription, pInfo->m_strName };

      int idx = gtk_clist_append(GTK_CLIST(piecelist), dummy);
      gtk_clist_set_row_data(GTK_CLIST(piecelist), idx, pInfo);
    }
  }
  gtk_clist_thaw(GTK_CLIST(piecelist));
}

// Callback for the groups toolbar.
static void group_event(GtkWidget *widget, gpointer data)
{
  fill_piecelist((int)data);
  if (!ignore_commands)
    project->HandleNotify(LC_GROUP_CHANGED, (int)data);
}

void groupsbar_set(int new_group)
{
  ignore_commands = true;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(groups[new_group]), TRUE);
  ignore_commands = false;
  gtk_clist_select_row (GTK_CLIST(piecelist), 0, 0);
}

// Callback for the pieces list.
static void selection_made(GtkWidget *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
  if (piece_info != NULL)
    piece_info->DeRef();
  piece_info = (PieceInfo*)gtk_clist_get_row_data(GTK_CLIST(piecelist), row);
  piece_info->AddRef();
  project->SetCurrentPiece(piece_info);
  gtk_widget_draw(piecepreview, NULL);
}

// Add a new piece to the combobox
void piececombo_add(char* string)
{
  if (string == NULL)
  {
    // Clear the list
    gtk_list_clear_items (GTK_LIST(GTK_COMBO(piececombo)->list), 0, -1);
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(piececombo)->entry), "");
  }
  else
  {
    GtkWidget *li;
    GtkList *list = GTK_LIST(GTK_COMBO(piececombo)->list);
    GtkWidget *child;
    GList *children;
    gchar* str;

    // Check if the string is already in the list
    children = list->children;
    while (children)
    {
      child = (GtkWidget*)children->data;
      children = children->next;

      gtk_label_get(GTK_LABEL(GTK_BIN(child)->child), &str);
      if (strcmp(str, string) == 0)
	return;
    }

    // Add new entry
    li = gtk_list_item_new_with_label(string);
    gtk_widget_show(li);
    gtk_container_add(GTK_CONTAINER(list), li);
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(piececombo)->entry), "");
  }
}

// TODO: make this work
static void piececombo_changed(GtkWidget *widget, GtkWidget *entry)
{
  char* str;
  str = gtk_entry_get_text(GTK_ENTRY(entry));
}

// TODO: sometimes this is called twice ?
static void piececombo_select(GtkList *list, GtkWidget *widget, gpointer data)
{
  int i;
  gchar* str;

  gtk_label_get(GTK_LABEL(GTK_BIN(widget)->child), &str);
  //  printf("%s\n", str);

  for (i = 0; i < project->GetPieceLibraryCount(); i++)
  {
    PieceInfo* pInfo = project->GetPieceInfo(i);
 
    if (strcmp (str, pInfo->m_strDescription) == 0)
    {
      // Check if we need to change the current group
      if ((list_groups) && (pInfo->m_nGroups != 0))
	if ((pInfo->m_nGroups & (1 << list_curgroup)) == 0)
	{
	  unsigned long d = 1;
	  for (int k = 1; k < 32; k++)
	  {
	    if ((pInfo->m_nGroups & d) != 0)
	    {
	      groupsbar_set(k-1);
	      k = 32;
	    }
	    else
	      d <<= 1;
	  }
	}

      // Select the piece
      i = gtk_clist_find_row_from_data (GTK_CLIST(piecelist), pInfo);
      gtk_clist_select_row (GTK_CLIST(piecelist), i, 0);

      return;
    }
  }
}

// Draw a pixmap for the colorlist control
static void colorlist_draw_pixmap(GtkWidget *widget)
{
  GdkGC* gc = gdk_gc_new(widget->window);
  int i;
  GdkRectangle rect;
  GdkColor c;

  gdk_gc_set_fill(gc, GDK_SOLID);
  rect.y = 0;
  rect.width = widget->allocation.width/14+1;
  rect.height = widget->allocation.height/2;

  for (i = 0; i < 28; i++)
  {
    if (i == 14)
      rect.y = rect.height;

    if (i < 14)
      rect.x = widget->allocation.width * i / 14;
    else
      rect.x = widget->allocation.width * (i-14) / 14;

    c.red = (gushort)(FlatColorArray[i][0]*0xFF);
    c.green = (gushort)(FlatColorArray[i][1]*0xFF);
    c.blue = (gushort)(FlatColorArray[i][2]*0xFF);
    gdk_color_alloc(gtk_widget_get_colormap(widget), &c);
    gdk_gc_set_foreground(gc, &c);

    gdk_draw_rectangle (colorlist_pixmap, gc, TRUE,
		      rect.x, rect.y,
		      rect.width, rect.height);

    if (i > 13 && i < 21)
    {
      int x, y;
      gdk_color_white(gtk_widget_get_colormap(widget), &c);
      gdk_gc_set_foreground(gc, &c);
      
      for (x = rect.x; x < rect.x + rect.width; x++)
      {
	for (y = rect.y + x%4; y < rect.y + rect.height; y += 4)
	  gdk_draw_point(colorlist_pixmap, gc, x, y);

	for (y = rect.y + rect.height - x%4; y > rect.y; y -= 4)
	  gdk_draw_point(colorlist_pixmap, gc, x, y);
      }
    }
  }

  gdk_color_black(gtk_widget_get_colormap(widget), &c);
  gdk_gc_set_foreground(gc, &c);
  gdk_gc_set_line_attributes(gc, 1,
	GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

  for (i = 0; i < 14; i++)
    gdk_draw_line (colorlist_pixmap, gc,
       widget->allocation.width * i / 14, 0,
       widget->allocation.width * i / 14, widget->allocation.height);
  
  gdk_draw_line (colorlist_pixmap, gc, 0, widget->allocation.height-1,
      widget->allocation.width, widget->allocation.height-1);
  gdk_draw_line (colorlist_pixmap, gc, 0, widget->allocation.height/2,
      widget->allocation.width, widget->allocation.height/2);
  gdk_draw_line (colorlist_pixmap, gc, widget->allocation.width-1, 0,
      widget->allocation.width-1, widget->allocation.height);
  gdk_draw_line (colorlist_pixmap, gc, 0, 0, widget->allocation.width, 0);

  c.red = (gushort)(0.4f*0xFFFF);
  c.green = (gushort)(0.8f*0xFFFF);
  c.blue = (gushort)(0.4f*0xFFFF);
  gdk_color_alloc(gtk_widget_get_colormap(widget), &c);
  gdk_gc_set_foreground(gc, &c);

  int l, r, t, b;
  i = cur_color;
  if (i > 13) i -= 14;
  l = widget->allocation.width * i / 14;
  r = widget->allocation.width * (i+1) / 14;
  t = (cur_color < 14) ? 0 : widget->allocation.height/2;
  b = (cur_color < 14) ? widget->allocation.height/2 : widget->allocation.height-1;

  gdk_draw_rectangle (colorlist_pixmap, gc, FALSE, l, t, r-l, b-t);

  gdk_gc_destroy(gc);
}

static gint colorlist_configure(GtkWidget *widget, GdkEventConfigure *event)
{
  if (colorlist_pixmap)
    gdk_pixmap_unref(colorlist_pixmap);

  colorlist_pixmap = gdk_pixmap_new(widget->window, widget->allocation.width,
				    widget->allocation.height, -1);
  colorlist_draw_pixmap(widget);

  return TRUE;
}

// Redraw from the backing pixmap
static gint colorlist_expose(GtkWidget *widget, GdkEventExpose *event)
{
  gdk_draw_pixmap(widget->window, widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		  colorlist_pixmap, event->area.x, event->area.y,
		  event->area.x, event->area.y,
		  event->area.width, event->area.height);

  return FALSE;
}

static gint colorlist_key_press(GtkWidget* widget, GdkEventKey* event, gpointer data)
{
  int x;

  switch (event->keyval)
  {
    case GDK_Up: x = cur_color - 14; break;
    case GDK_Down: x = cur_color + 14; break;
    case GDK_Left: x = cur_color - 1; break;
    case GDK_Right: x = cur_color + 1; break;

  default:
    return TRUE;
  }

  if ((x > -1) && (x < 28))
  {
    cur_color = x;
    colorlist_draw_pixmap(widget);
    project->HandleNotify(LC_COLOR_CHANGED, x);
    gtk_widget_draw(widget, NULL);
    gtk_widget_draw(piecepreview, NULL);
  }
  gtk_signal_emit_stop_by_name (GTK_OBJECT(widget), "key_press_event");

  return TRUE;
}

static gint colorlist_button_press(GtkWidget *widget, GdkEventButton *event)
{
  if (event->button == 1 && colorlist_pixmap != NULL)
  {
    int x = (int)(event->x * 14 / widget->allocation.width);
    if (event->y > (widget->allocation.height/2))
      x += 14;

    if (x != cur_color)
    {
      cur_color = x;
      colorlist_draw_pixmap(widget);
      project->HandleNotify(LC_COLOR_CHANGED, x);
      gtk_widget_draw(widget, NULL);
      gtk_widget_draw(piecepreview, NULL);
    }
  }
  gtk_window_set_focus(GTK_WINDOW(main_window), widget);

  return TRUE;
}

void colorlist_set(int new_color)
{
  if (new_color != cur_color)
  {
    cur_color = new_color;
    colorlist_draw_pixmap(colorlist);
    gtk_widget_draw(colorlist, NULL);
    gtk_widget_draw(piecepreview, NULL);
  }
}

// Create what is the pieces toolbar in the Windows version as fixed items.
void create_piecebar(GtkWidget *window, GtkWidget *hbox)
{
  gchar *titles[2] = { "Description", "Number" };
  int attrlist[] =
  {
    GLX_RGBA,
    GLX_DOUBLEBUFFER,
    GLX_DEPTH_SIZE, 16,
    0
  };

  GtkWidget *vbox1, *vpan, *scroll_win;

  GtkWidget* frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_box_pack_end (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (frame), vbox1);
  gtk_container_border_width(GTK_CONTAINER (vbox1), 2);

  vpan = gtk_vpaned_new ();
  gtk_widget_show (vpan);
  gtk_box_pack_start (GTK_BOX (vbox1), vpan, TRUE, TRUE, 0);

  piecepreview = GTK_WIDGET(gtk_gl_area_share_new(attrlist, GTK_GL_AREA(drawing_area)));
  gtk_widget_set_events(GTK_WIDGET(piecepreview), GDK_EXPOSURE_MASK);
  gtk_signal_connect(GTK_OBJECT(piecepreview), "expose_event",
		     GTK_SIGNAL_FUNC(draw_preview), NULL);

  gtk_widget_set_usize(GTK_WIDGET(piecepreview), 100, 100);
  gtk_widget_show (piecepreview);
  gtk_container_add (GTK_CONTAINER (vpan), piecepreview);

  scroll_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scroll_win), 
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_widget_show (scroll_win);
  gtk_container_add (GTK_CONTAINER (vpan), scroll_win);

  piecelist = gtk_clist_new_with_titles(2, titles);
  gtk_signal_connect(GTK_OBJECT(piecelist), "select_row",
		     GTK_SIGNAL_FUNC(selection_made), NULL);
  gtk_signal_connect (GTK_OBJECT (piecelist), "click_column",
		      GTK_SIGNAL_FUNC (piecelist_setsort), NULL);
  gtk_container_add (GTK_CONTAINER (scroll_win), piecelist);
  gtk_clist_set_column_width (GTK_CLIST(piecelist), 0, 90);
  gtk_clist_set_column_width (GTK_CLIST(piecelist), 1, 10);
  gtk_clist_set_column_auto_resize (GTK_CLIST(piecelist), 1, TRUE);
  list_arrows[0] = clist_title_with_arrow (piecelist, 0, titles[0]);
  list_arrows[1] = clist_title_with_arrow (piecelist, 1, titles[1]);
   //gtk_clist_set_shadow_type (GTK_CLIST(piecelist), GTK_SHADOW_IN);
  //  gtk_clist_column_titles_show (GTK_CLIST (piecelist));
  gtk_clist_set_auto_sort (GTK_CLIST(piecelist), TRUE);
  gtk_widget_show (list_arrows[0]);
  gtk_widget_show (piecelist);

  /*
  gtk_clist_set_hadjustment (GTK_CLIST (piecelist), NULL);
  gtk_clist_set_vadjustment (GTK_CLIST (piecelist), NULL);
  */

#include "pixmaps/pi-acces.xpm"
#include "pixmaps/pi-plate.xpm"
#include "pixmaps/pi-tile.xpm"
#include "pixmaps/pi-brick.xpm"
#include "pixmaps/pi-slope.xpm"
#include "pixmaps/pi-train.xpm"
#include "pixmaps/pi-space.xpm"
#include "pixmaps/pi-misc.xpm"
#include "pixmaps/pi-tech.xpm"

  grouptoolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_container_set_border_width (GTK_CONTAINER(grouptoolbar), 2);
  //gtk_toolbar_set_space_size (GTK_TOOLBAR(toolbar), 5);

  groups[0] = gtk_toolbar_append_element(GTK_TOOLBAR(grouptoolbar),
     GTK_TOOLBAR_CHILD_RADIOBUTTON, NULL, "", "Plates", "", new_pixmap(window, pi_plate),
     GTK_SIGNAL_FUNC(group_event), (void*)0);
  groups[1] = gtk_toolbar_append_element(GTK_TOOLBAR(grouptoolbar),
     GTK_TOOLBAR_CHILD_RADIOBUTTON, groups[0], "", "Bricks", "", new_pixmap(window, pi_brick),
     GTK_SIGNAL_FUNC(group_event), (void*)1);
  groups[2] = gtk_toolbar_append_element(GTK_TOOLBAR(grouptoolbar),
     GTK_TOOLBAR_CHILD_RADIOBUTTON, groups[1], "", "Tiles", "", new_pixmap(window, pi_tile),
     GTK_SIGNAL_FUNC(group_event), (void*)2);
  groups[3] = gtk_toolbar_append_element(GTK_TOOLBAR(grouptoolbar),
     GTK_TOOLBAR_CHILD_RADIOBUTTON, groups[2], "", "Slopes", "", new_pixmap(window, pi_slope),
     GTK_SIGNAL_FUNC(group_event), (void*)3);
  groups[4] = gtk_toolbar_append_element(GTK_TOOLBAR(grouptoolbar),
     GTK_TOOLBAR_CHILD_RADIOBUTTON, groups[3], "", "Technic", "", new_pixmap(window, pi_tech),
     GTK_SIGNAL_FUNC(group_event), (void*)4);
  groups[5] = gtk_toolbar_append_element(GTK_TOOLBAR(grouptoolbar),
     GTK_TOOLBAR_CHILD_RADIOBUTTON, groups[4], "", "Space", "", new_pixmap(window, pi_space),
     GTK_SIGNAL_FUNC(group_event), (void*)5);
  groups[6] = gtk_toolbar_append_element(GTK_TOOLBAR(grouptoolbar),
     GTK_TOOLBAR_CHILD_RADIOBUTTON, groups[5], "", "Train", "", new_pixmap(window, pi_train),
     GTK_SIGNAL_FUNC(group_event), (void*)6);
  groups[7] = gtk_toolbar_append_element(GTK_TOOLBAR(grouptoolbar),
     GTK_TOOLBAR_CHILD_RADIOBUTTON, groups[6], "", "Other", "", new_pixmap(window, pi_misc),
     GTK_SIGNAL_FUNC(group_event), (void*)7);
  groups[8] = gtk_toolbar_append_element(GTK_TOOLBAR(grouptoolbar),
     GTK_TOOLBAR_CHILD_RADIOBUTTON, groups[7], "", "Accessories", "", new_pixmap(window, pi_acces),
     GTK_SIGNAL_FUNC(group_event), (void*)8);

  // gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(both_button),TRUE);
  gtk_box_pack_start(GTK_BOX(vbox1), grouptoolbar, FALSE, TRUE, 0);
  gtk_widget_show(grouptoolbar);

  // Piece combo
  piececombo = gtk_combo_new();
  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(piececombo)->entry), "changed",
		     GTK_SIGNAL_FUNC(piececombo_changed), GTK_COMBO(piececombo)->entry);
  //  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(piececombo)->list), "select-child",
  //	     GTK_SIGNAL_FUNC(piececombo_select), GTK_COMBO(piececombo)->list);
  gtk_box_pack_start(GTK_BOX(vbox1), piececombo, FALSE, TRUE, 0);
  gtk_widget_show(piececombo);

  // Color list
  colorlist = gtk_drawing_area_new ();
  gtk_widget_set_events (colorlist, GDK_EXPOSURE_MASK
			 | GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK);
  GTK_WIDGET_SET_FLAGS(colorlist, GTK_CAN_FOCUS);
  gtk_drawing_area_size(GTK_DRAWING_AREA(colorlist), 200, 30);
  gtk_box_pack_start(GTK_BOX(vbox1), colorlist, FALSE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT(colorlist), "expose_event",
		      (GtkSignalFunc) colorlist_expose, NULL);
  gtk_signal_connect (GTK_OBJECT(colorlist),"configure_event",
		      (GtkSignalFunc) colorlist_configure, NULL);
  gtk_signal_connect (GTK_OBJECT(colorlist), "button_press_event",
		      (GtkSignalFunc) colorlist_button_press, NULL);
  gtk_signal_connect (GTK_OBJECT(colorlist), "key_press_event",
		      GTK_SIGNAL_FUNC(colorlist_key_press), NULL);

  gtk_widget_show(colorlist);
}

// =========================================================

// Status bar

GtkWidget *label_message, *label_position, *label_snap, *label_step;

void create_statusbar(GtkWidget *window, GtkWidget *vbox)
{
  GtkWidget *hbox, *hbox1;
  GtkWidget *frame;
  //GtkStyle *style;

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_widget_set_usize (hbox, -1, 24);
  gtk_container_border_width (GTK_CONTAINER (hbox), 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 2);

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox1);
  gtk_container_border_width (GTK_CONTAINER (hbox1), 0);
  gtk_widget_show (hbox1);

  label_message = gtk_label_new (" ");
  gtk_widget_show (label_message);
  gtk_box_pack_start (GTK_BOX (hbox1), label_message, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label_message), GTK_JUSTIFY_LEFT);
  gtk_misc_set_padding (GTK_MISC (label_message), 3, 0);

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_widget_set_usize (frame, 150, -1);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox1);
  gtk_container_border_width (GTK_CONTAINER (hbox1), 0);
  gtk_widget_show (hbox1);

  label_position = gtk_label_new (" ");
  gtk_widget_show (label_position);
  gtk_box_pack_start (GTK_BOX (hbox1), label_position, TRUE, TRUE, 0);

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_widget_set_usize (frame, 70, -1);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox1);
  gtk_container_border_width (GTK_CONTAINER (hbox1), 0);
  gtk_widget_show (hbox1);

  label_snap = gtk_label_new (" ");
  gtk_widget_show (label_snap);
  gtk_box_pack_start (GTK_BOX (hbox1), label_snap, TRUE, TRUE, 0);

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_widget_set_usize (frame, 70, -1);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox1);
  gtk_container_border_width (GTK_CONTAINER (hbox1), 0);
  gtk_widget_show (hbox1);

  label_step = gtk_label_new (" ");
  gtk_widget_show (label_step);
  gtk_box_pack_start (GTK_BOX (hbox1), label_step, TRUE, TRUE, 0);
}




