// LeoCAD
//
// Linux specific initialization
//

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <string.h>
#include "opengl.h"
#include "gtkglarea.h"
#include "project.h"
#include "toolbar.h"
#include "globals.h"
#include "main.h"
#include "system.h"
#include "config.h"

void create_main_menu (GtkObject *window, GtkWidget *vbox);

// Variables

GtkWidget *main_window;
GtkWidget *drawing_area;

static GtkWidget* main_hbox;
static GtkWidget* drawing_frame;
static GtkWidget* drawing_parent;
static GtkWidget* pieces_frame;
static GtkWidget* pieces_parent;
static gboolean   pieces_floating;
static gboolean   pieces_visible;
static gint       pieces_width;

static char app_path[PATH_MAX];
static char lib_path[] = LC_INSTALL_PREFIX"/share/leocad/";
bool ignore_commands = false;

static void update_window_layout ();
static gint main_quit (GtkWidget *widget, GdkEvent* event, gpointer data);

// =============================================================================
// Static functions

// try to find the path of the executable
static void init_paths (char *argv0)
{
  char temppath[PATH_MAX];
  char *home;

  home = getenv ("HOME");
  if (home == NULL)
  {
    uid_t id = getuid();
    struct passwd *pwd;

    setpwent();
    while ((pwd = getpwent()) != NULL)
      if (pwd->pw_uid == id)
      {
	home = pwd->pw_dir;
	break;
      }
    endpwent();
  }

  if (home == NULL)
    home = ".";

  strcpy (temppath, argv0);
  if (!strrchr(temppath, '/'))
  {
    char *path;
    char *last;
    int found;

    found = 0;
    path = getenv("PATH");
    do
    {
      temppath[0] = '\0';

      last = strchr(path, ':');
      if (!last)
	last = path + strlen (path);

      if (*path == '~')
      {
	strcpy(temppath, home);
	++path;
      }

      if (last > (path+1))
      {
	strncat(temppath, path, (last-path));
	strcat(temppath, "/");
      }
      strcat (temppath, "./");
      strcat (temppath, argv0);

      if (access(temppath, X_OK) == 0 )
	++found;
      path = last+1;

    } while (*last && !found);
  }
  else
    argv0 = strrchr (argv0, '/') + 1;

  if (realpath (temppath, app_path))
    *(strrchr (app_path, '/') + 1) = '\0';
}

// Functions

void OnCommandDirect(GtkWidget *w, gpointer data)
{
  if (ignore_commands)
    return;

  project->HandleCommand((LC_COMMANDS)(int)data, 0);
}

void OnCommand(GtkWidget* widget, gpointer data)
{
  int id = (int)data;

  if (ignore_commands)
    return;

  if (id >= ID_FILE_RECENT1 && id <= ID_FILE_RECENT4)
  {
    project->HandleCommand(LC_FILE_RECENT, id - ID_FILE_RECENT1);
    return;
  }
  
  if (id >= ID_ACTION_SELECT && id <= ID_ACTION_ROLL)
  {
    project->SetAction(id - ID_ACTION_SELECT);
    return;
  }

  if (id >= ID_VIEW_VIEWPORTS_01 && id <= ID_VIEW_VIEWPORTS_14)
  {
    project->HandleCommand(LC_VIEW_VIEWPORTS, id - ID_VIEW_VIEWPORTS_01);
    return;
  }

  if (id >= ID_CAMERA_FIRST && id <= ID_CAMERA_LAST)
  {
    project->HandleCommand(LC_VIEW_CAMERA_MENU, id - ID_CAMERA_FIRST);
    return;
  }

  switch (id)
  {
    case ID_FILE_EXIT:
    {
      main_quit (NULL, NULL, NULL);
    } break;

    case ID_SNAP_A:
    {
      project->HandleCommand(LC_TOOLBAR_SNAPMENU, 5);
    } break;

    case ID_VIEW_TOOLBAR_STANDARD:
    {
      if (GTK_WIDGET_VISIBLE (main_toolbar.handle_box))
	gtk_widget_hide (main_toolbar.handle_box);
      else
	gtk_widget_show (main_toolbar.handle_box);
    } break;

    case ID_VIEW_TOOLBAR_DRAWING:
    {
      if (GTK_WIDGET_VISIBLE (tool_toolbar.handle_box))
	gtk_widget_hide (tool_toolbar.handle_box);
      else
	gtk_widget_show (tool_toolbar.handle_box);
    } break;

    case ID_VIEW_TOOLBAR_ANIMATION:
    {
      if (GTK_WIDGET_VISIBLE (anim_toolbar.handle_box))
	gtk_widget_hide (anim_toolbar.handle_box);
      else
	gtk_widget_show (anim_toolbar.handle_box);
    } break;

    case ID_VIEW_TOOLBAR_PIECES:
    {
      gpointer widget = gtk_object_get_data (GTK_OBJECT (main_window), "menu_view_toolbar_floating");

      if (pieces_visible)
      {
	pieces_visible = FALSE;
	gtk_widget_set_sensitive (GTK_WIDGET (widget), FALSE);
      }
      else
      {
	pieces_visible = TRUE;
	gtk_widget_set_sensitive (GTK_WIDGET (widget), TRUE);
      }
      update_window_layout ();
    } break;

    case ID_VIEW_TOOLBAR_FLOATING:
    {
      if (pieces_floating)
	pieces_floating = FALSE;
      else
	pieces_floating = TRUE;
      update_window_layout ();
    } break;

    case ID_VIEW_TOOLBAR_BOTH:
    {
      gtk_toolbar_set_style (GTK_TOOLBAR (main_toolbar.toolbar), GTK_TOOLBAR_BOTH);
      gtk_toolbar_set_style (GTK_TOOLBAR (tool_toolbar.toolbar), GTK_TOOLBAR_BOTH);
      gtk_toolbar_set_style (GTK_TOOLBAR (anim_toolbar.toolbar), GTK_TOOLBAR_BOTH);
      gtk_widget_set_usize (main_toolbar.handle_box, -1, -1);
      gtk_widget_set_usize (tool_toolbar.handle_box, -1, -1);
      gtk_widget_set_usize (anim_toolbar.handle_box, -1, -1);
    } break;

    case ID_VIEW_TOOLBAR_ICONS:
    {
      gtk_toolbar_set_style (GTK_TOOLBAR (main_toolbar.toolbar), GTK_TOOLBAR_ICONS);
      gtk_toolbar_set_style (GTK_TOOLBAR (tool_toolbar.toolbar), GTK_TOOLBAR_ICONS);
      gtk_toolbar_set_style (GTK_TOOLBAR (anim_toolbar.toolbar), GTK_TOOLBAR_ICONS);
      gtk_widget_set_usize (main_toolbar.handle_box, -1, -1);
      gtk_widget_set_usize (tool_toolbar.handle_box, -1, -1);
      gtk_widget_set_usize (anim_toolbar.handle_box, -1, -1);
    } break;

    case ID_VIEW_TOOLBAR_TEXT:
    {
      gtk_toolbar_set_style (GTK_TOOLBAR (main_toolbar.toolbar), GTK_TOOLBAR_TEXT);
      gtk_toolbar_set_style (GTK_TOOLBAR (tool_toolbar.toolbar), GTK_TOOLBAR_TEXT);
      gtk_toolbar_set_style (GTK_TOOLBAR (anim_toolbar.toolbar), GTK_TOOLBAR_TEXT);
      gtk_widget_set_usize (main_toolbar.handle_box, -1, -1);
      gtk_widget_set_usize (tool_toolbar.handle_box, -1, -1);
      gtk_widget_set_usize (anim_toolbar.handle_box, -1, -1);
    } break;
  }
}

static gint button_press_event (GtkWidget *widget, GdkEventButton *event)
{
  int x, y;
  x = (int)event->x;
  y = widget->allocation.height - (int)event->y - 1;

  if (event->type == GDK_BUTTON_PRESS)
  {
    if (event->button == 1)
      project->OnLeftButtonDown(x, y, (event->state & GDK_CONTROL_MASK) != 0, 
				(event->state & GDK_SHIFT_MASK) != 0);
    if (event->button == 3)
      project->OnRightButtonDown(x, y, (event->state & GDK_CONTROL_MASK) != 0, 
				 (event->state & GDK_SHIFT_MASK) != 0);
  }
  else if (event->type == GDK_2BUTTON_PRESS)
  {
    project->OnLeftButtonDoubleClick(x, y, (event->state & GDK_CONTROL_MASK) != 0, 
				     (event->state & GDK_SHIFT_MASK) != 0);
  }

  gtk_window_set_focus(GTK_WINDOW(main_window), drawing_area);
  
  return TRUE;
}

static gint button_release_event (GtkWidget *widget, GdkEventButton *event)
{
  int x, y;
  x = (int)event->x;
  y = widget->allocation.height - (int)event->y - 1;

  if (event->button == 1)
    project->OnLeftButtonUp(x, y, (event->state & GDK_CONTROL_MASK) != 0, 
			    (event->state & GDK_SHIFT_MASK) != 0);
  if (event->button == 3)
    project->OnRightButtonUp(x, y, (event->state & GDK_CONTROL_MASK) != 0, 
			     (event->state & GDK_SHIFT_MASK) != 0);

  return TRUE;
}

static gint motion_notify_event (GtkWidget *widget, GdkEventMotion *event)
{
  int x, y;
  GdkModifierType state;

  if (event->is_hint)
    gdk_window_get_pointer (event->window, &x, &y, &state);
  else
  {
    x = (int)event->x;
    y = (int)event->y;
    state = (GdkModifierType)event->state;
  }

  y = widget->allocation.height - (int)event->y - 1;

  //  if (state)
  project->OnMouseMove(x, y, (event->state & GDK_CONTROL_MASK) != 0, 
		       (event->state & GDK_SHIFT_MASK) != 0);

  return TRUE;
}

static gint key_press_event(GtkWidget* widget, GdkEventKey* event, gpointer data)
{
  char code = 0;

  if (event->keyval < 0x100)
    code = gdk_keyval_to_upper(event->keyval);
  else
  {
    switch (event->keyval)
    {
    case GDK_Shift_L: case GDK_Shift_R: code = KEY_SHIFT; break;
    case GDK_Control_L: case GDK_Control_R: code = KEY_CONTROL; break;
    case GDK_Escape: code = KEY_ESCAPE; break;
    case GDK_Tab: code = KEY_TAB; break;
    case GDK_Insert: code = KEY_INSERT; break;
    case GDK_Delete: code = KEY_DELETE; break;
    case GDK_Up: code = KEY_UP; break;
    case GDK_Down: code = KEY_DOWN; break;
    case GDK_Left: code = KEY_LEFT; break;
    case GDK_Right: code = KEY_RIGHT; break;
    case GDK_Prior: code = KEY_PRIOR; break;
    case GDK_Next: code = KEY_NEXT; break;
    }
  }

  if (code != 0)
  {
    if (project->OnKeyDown(code, (event->state & GDK_CONTROL_MASK) != 0, 
			   (event->state & GDK_SHIFT_MASK) != 0))
	gtk_signal_emit_stop_by_name (GTK_OBJECT(widget), "key_press_event");
  }

  return TRUE;
}

static gint init (GtkWidget *widget)
{
  // OpenGL functions can be called only if make_current returns true
  if (gtk_gl_area_make_current (GTK_GL_AREA (widget)))
  {
    GL_InitializeExtensions ();
  }
  
  return TRUE;
}

// When widget is exposed it's contents are redrawn.
static gint draw_view(GtkWidget *widget, GdkEventExpose *event)
{
  // Draw only last expose.
  if (event->count > 0)
    return TRUE;

  project->Render(false);

  return TRUE;
}

// Save the new size of the window.
static gint reshape_view(GtkWidget *widget, GtkAllocation *allocation, gpointer data)
{
  project->SetViewSize (allocation->width, allocation->height);

  return TRUE;
}

static void main_destroy ()
{
  gpointer item;
  int i = 0;

  // Save window position/size
  Sys_ProfileSaveInt ("Window", "Width", main_window->allocation.width);
  Sys_ProfileSaveInt ("Window", "Height", main_window->allocation.height);

  // Save toolbar state
  Sys_ProfileSaveInt ("Toolbars", "Standard", ((GTK_WIDGET_VISIBLE (main_toolbar.handle_box)) ? 1 : 0));
  Sys_ProfileSaveInt ("Toolbars", "Drawing", ((GTK_WIDGET_VISIBLE (tool_toolbar.handle_box)) ? 1 : 0));
  Sys_ProfileSaveInt ("Toolbars", "Animation", ((GTK_WIDGET_VISIBLE (anim_toolbar.handle_box)) ? 1 : 0));
  Sys_ProfileSaveInt ("Toolbars", "Pieces", (pieces_visible ? 1 : 0));
  Sys_ProfileSaveInt ("Toolbars", "Floating", (pieces_floating ? 1 : 0));

  if (pieces_parent != NULL)
    Sys_ProfileSaveInt ("Toolbars", "PiecesWidth", pieces_frame->allocation.width);

  item = gtk_object_get_data (GTK_OBJECT (main_window), "menu_view_toolbar_icons");
  if (GTK_CHECK_MENU_ITEM (item)->active)
    i = 0;
  else
  {
    item = gtk_object_get_data (GTK_OBJECT (main_window), "menu_view_toolbar_both");
    if (GTK_CHECK_MENU_ITEM (item)->active)
      i = 1;
    else
    {
      item = gtk_object_get_data (GTK_OBJECT (main_window), "menu_view_toolbar_text");
      if (GTK_CHECK_MENU_ITEM (item)->active)
	i = 2;
    }
  }
  Sys_ProfileSaveInt ("Toolbars", "Style", i);

  gtk_main_quit ();
}

static gint main_quit (GtkWidget *widget, GdkEvent* event, gpointer data)
{
  if (!project->SaveModified())
    return TRUE;

  delete project;
  project = NULL;

  // save window position
  gint x, y;
  gdk_window_get_root_origin (main_window->window, &x, &y);
  Sys_ProfileSaveInt ("Window", "PositionX", x);
  Sys_ProfileSaveInt ("Window", "PositionY", y);

  gtk_widget_destroy (main_window);

  return FALSE;
}

static gint pieces_close (GtkWidget *widget, GdkEvent* event, gpointer data)
{
  gpointer item;

  pieces_visible = FALSE;

  item = gtk_object_get_data (GTK_OBJECT (main_window), "menu_view_toolbar_pieces");
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), FALSE);

  item = gtk_object_get_data (GTK_OBJECT (main_window), "menu_view_toolbar_floating");
  gtk_widget_set_sensitive (GTK_WIDGET (item), FALSE);

  update_window_layout ();

  return FALSE;
}

// this function takes care of showing the pieces bar correctly
static void update_window_layout ()
{
  // first thing we need to create the widgets
  if (drawing_area == NULL)
  {
    int attrlist[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 16, 0 };

    // Create a new OpenGL widget
    drawing_area = GTK_WIDGET (gtk_gl_area_new (attrlist));

    GTK_WIDGET_SET_FLAGS (drawing_area, GTK_CAN_FOCUS);

    gtk_widget_set_events (GTK_WIDGET (drawing_area), GDK_EXPOSURE_MASK | GDK_KEY_PRESS_MASK |
			   GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
			   GDK_POINTER_MOTION_HINT_MASK);

    // Connect signal handlers
    gtk_signal_connect (GTK_OBJECT (drawing_area), "expose_event", 
			GTK_SIGNAL_FUNC(draw_view), NULL);
    gtk_signal_connect (GTK_OBJECT (drawing_area), "size_allocate",
			GTK_SIGNAL_FUNC(reshape_view), NULL);
    gtk_signal_connect (GTK_OBJECT (drawing_area), "realize",
			GTK_SIGNAL_FUNC(init), NULL);
    gtk_signal_connect (GTK_OBJECT (drawing_area), "motion_notify_event",
			GTK_SIGNAL_FUNC(motion_notify_event), NULL);
    gtk_signal_connect (GTK_OBJECT (drawing_area), "button_press_event",
			GTK_SIGNAL_FUNC(button_press_event), NULL);
    gtk_signal_connect (GTK_OBJECT (drawing_area), "button_release_event",
			GTK_SIGNAL_FUNC(button_release_event), NULL);
    gtk_signal_connect (GTK_OBJECT (drawing_area), "key_press_event",
			GTK_SIGNAL_FUNC(key_press_event), NULL);
 
    // set minimum size
    gtk_widget_set_usize (GTK_WIDGET (drawing_area), 100, 100);

    drawing_frame = gtk_frame_new (NULL);
    gtk_widget_show (drawing_frame);
    //    gtk_container_add (GTK_CONTAINER (hbox), drawing_frame);
    gtk_frame_set_shadow_type (GTK_FRAME (drawing_frame), GTK_SHADOW_IN);               

    gtk_container_add (GTK_CONTAINER (drawing_frame), GTK_WIDGET (drawing_area));
    gtk_widget_show (GTK_WIDGET (drawing_area));

    // now create the pieces bar
    pieces_frame = create_piecebar (main_window);
  }
  else
  {
    // if the widgets already exist, remove their parents
    if (pieces_parent)
      pieces_width = pieces_frame->allocation.width;

    gtk_container_remove (GTK_CONTAINER (drawing_parent), drawing_frame);
    if (pieces_parent != NULL)
      gtk_container_remove (GTK_CONTAINER (pieces_parent), pieces_frame);

    if (drawing_parent != main_hbox)
      gtk_widget_destroy (drawing_parent);
    else if (pieces_parent != NULL)
      gtk_widget_destroy (pieces_parent);

    pieces_parent = NULL;
    drawing_parent = NULL;
  }

  // now add the widgets to their new parents
  if (pieces_floating)
  {
    gtk_box_pack_start (GTK_BOX (main_hbox), drawing_frame, TRUE, TRUE, 0);
    drawing_parent = main_hbox;

    if (pieces_visible)
    {
      pieces_parent = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_signal_connect (GTK_OBJECT (pieces_parent), "delete_event",
			  GTK_SIGNAL_FUNC (pieces_close), NULL);
      gtk_signal_connect (GTK_OBJECT (pieces_parent), "destroy",
			  GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
      gtk_window_set_title (GTK_WINDOW (pieces_parent), "Pieces");
      gtk_window_set_default_size (GTK_WINDOW (pieces_parent), pieces_width, -1);

      gtk_container_add (GTK_CONTAINER (pieces_parent), pieces_frame);

      gtk_widget_show (pieces_parent);
    }
  }
  else
  {
    if (pieces_visible)
    {
      pieces_parent = drawing_parent = gtk_hpaned_new ();
      gtk_paned_pack1 (GTK_PANED (drawing_parent), drawing_frame, TRUE, TRUE);
      gtk_paned_pack2 (GTK_PANED (pieces_parent), pieces_frame, FALSE, FALSE);
      gtk_widget_show (pieces_parent);
      gtk_box_pack_start (GTK_BOX (main_hbox), pieces_parent, TRUE, TRUE, 0);

      if (pieces_floating == FALSE)
	gtk_paned_set_position (GTK_PANED (pieces_parent), main_window->allocation.width -
				pieces_width - GTK_PANED (pieces_parent)->gutter_size);
    }
    else
    {
      gtk_box_pack_start (GTK_BOX (main_hbox), drawing_frame, TRUE, TRUE, 0);
      drawing_parent = main_hbox;
    }
  }
}

int main (int argc, char* argv[])
{
  char* libgl = NULL;
  GtkWidget *vbox;
  int i, j, k, x, y;

  // Parse and remove Linux arguments
  for (i = 1; i < argc; i++)
  {
    char* param = argv[i];

    if (param[0] == '-' && param[1] == '-')
    {
      param += 2;

      if ((strcmp (param, "libgl") == 0) && (i != argc))
      {
	libgl = argv[i+1];
	argv[i] = argv[i+1] = NULL;
	i++;
      }
    }
  }

  for (i = 1; i < argc; i++)
  {
    for (k = i; k < argc; k++)
      if (argv[k] != NULL)
	break;

    if (k > i)
    {
      k -= i;
      for (j = i + k; j < argc; j++)
	argv[j-k] = argv[j];
      argc -= k;
    }
  }

  init_paths (argv[0]);

  gtk_set_locale ();
  gtk_init (&argc, &argv);

  atexit (GL_Shutdown);
  // Check if OpenGL is supported.
  if (!GL_Initialize (libgl))
    return 1;

  if (gdk_gl_query() == FALSE)
  {
    g_print("ERROR: OpenGL not supported\n");
    return 1;
  }

//  startup_message ("Loading user preferences ...");
  project = new Project();

  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (main_window), "LeoCAD");
  gtk_container_border_width (GTK_CONTAINER (main_window), 0);
  gtk_widget_realize (main_window);

  // Read window position and size
  x = Sys_ProfileLoadInt ("Window", "Width", 600);
  y = Sys_ProfileLoadInt ("Window", "Height", 400);
  gtk_window_set_default_size (GTK_WINDOW (main_window), x, y);

  x = Sys_ProfileLoadInt ("Window", "PositionX", -1);
  y = Sys_ProfileLoadInt ("Window", "PositionY", -1);
  if (x != -1 && y != -1)
    gtk_widget_set_uposition (main_window, x, y);

  gtk_signal_connect (GTK_OBJECT (main_window), "delete_event", (GtkSignalFunc) main_quit, NULL);
  gtk_signal_connect (GTK_OBJECT (main_window), "destroy", (GtkSignalFunc) main_destroy, NULL);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (main_window), vbox);
  gtk_widget_show (vbox);

//  startup_message ("Creating Main Menu ...");
  create_main_menu (GTK_OBJECT (main_window), vbox);

//  startup_message ("Creating Toolbars ...");
  create_toolbars (main_window, vbox);

  main_hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (vbox), main_hbox);
  gtk_widget_show (main_hbox);

  // load toolbar preferences and update the menu
  int show;
  ignore_commands = true;

  show = Sys_ProfileLoadInt ("Toolbars", "Standard", 1);
  if (show)
  {
    gpointer widget = gtk_object_get_data (GTK_OBJECT (main_window), "menu_view_toolbar_standard");
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), TRUE);
  }
  else
    gtk_widget_hide (main_toolbar.handle_box);

  show = Sys_ProfileLoadInt ("Toolbars", "Drawing", 1);
  if (show)
  {
    gpointer widget = gtk_object_get_data (GTK_OBJECT (main_window), "menu_view_toolbar_drawing");
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), TRUE);
  }
  else
    gtk_widget_hide (tool_toolbar.handle_box);

  show = Sys_ProfileLoadInt ("Toolbars", "Animation", 1);
  if (show)
  {
    gpointer widget = gtk_object_get_data (GTK_OBJECT (main_window), "menu_view_toolbar_animation");
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), TRUE);
  }
  else
    gtk_widget_hide (anim_toolbar.handle_box);

  show = Sys_ProfileLoadInt ("Toolbars", "Pieces", 1);
  if (show)
  {
    gpointer widget = gtk_object_get_data (GTK_OBJECT (main_window), "menu_view_toolbar_pieces");
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), TRUE);
    pieces_visible = TRUE;
  }
  else
  {
    gpointer widget = gtk_object_get_data (GTK_OBJECT (main_window), "menu_view_toolbar_floating");
    gtk_widget_set_sensitive (GTK_WIDGET (widget), FALSE);
  }

  show = Sys_ProfileLoadInt ("Toolbars", "PiecesFloating", 0);
  if (show)
  {
    gpointer widget = gtk_object_get_data (GTK_OBJECT (main_window), "menu_view_toolbar_floating");
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), TRUE);
    pieces_floating = TRUE;
  }

  ignore_commands = false;

  show = Sys_ProfileLoadInt ("Toolbars", "Style", 0);
  gpointer item = NULL;

  switch (show)
  {
  case 0:
    item = gtk_object_get_data (GTK_OBJECT (main_window), "menu_view_toolbar_icons");
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
    OnCommand (GTK_WIDGET (item), GINT_TO_POINTER (ID_VIEW_TOOLBAR_ICONS));
    break;
  case 1:
    item = gtk_object_get_data (GTK_OBJECT (main_window), "menu_view_toolbar_both");
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
    OnCommand (GTK_WIDGET (item), GINT_TO_POINTER (ID_VIEW_TOOLBAR_BOTH));
    break;
  case 2:
    item = gtk_object_get_data (GTK_OBJECT (main_window), "menu_view_toolbar_text");
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
    OnCommand (GTK_WIDGET (item), GINT_TO_POINTER (ID_VIEW_TOOLBAR_TEXT));
    break;
  }

  pieces_width = Sys_ProfileLoadInt ("Toolbars", "PiecesWidth", 210);
  update_window_layout ();

  // increase the reference count to allow changing parents when hiding the pieces bar
  gtk_widget_ref (drawing_frame);
  gtk_widget_ref (pieces_frame);

  create_statusbar (main_window, vbox);
//  gtk_box_pack_start (GTK_BOX (vbox), create_status_bar (), FALSE, TRUE, 2);
  //  GtkWidget* statusbar = gtk_statusbar_new ();
  //gtk_widget_show (statusbar);
  //gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, TRUE, 0);   

#include "pixmaps/icon32.xpm"

  GdkPixmap *gdkpixmap;
  GdkBitmap *mask;

  gdkpixmap = gdk_pixmap_create_from_xpm_d (main_window->window, &mask,
                 &main_window->style->bg[GTK_STATE_NORMAL], icon32);
  gdk_window_set_icon (main_window->window, NULL, gdkpixmap, mask);

  gtk_widget_show (GTK_WIDGET (main_window));

  // get the splitter in the correct size, must be done after the widget has been realized
  if ((pieces_floating == FALSE) && (pieces_visible == TRUE))
    gtk_paned_set_position (GTK_PANED (pieces_parent), main_window->allocation.width -
			    pieces_width - GTK_PANED (pieces_parent)->gutter_size);

  if (project->Initialize (argc, argv, app_path, lib_path) == false)
  {
    delete project;
    //    return 1;
    _exit (1);
  }

  gtk_main();

  gtk_widget_unref (drawing_frame);
  gtk_widget_unref (pieces_frame);

  //  delete project;
  _exit (0); // FIXME !
  return 0;
}
