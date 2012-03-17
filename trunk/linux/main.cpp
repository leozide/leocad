// LeoCAD
//
// Linux specific initialization
//

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <string.h>
#include "opengl.h"
#include "project.h"
#include "toolbar.h"
#include "globals.h"
#include "main.h"
#include "system.h"
#include "config.h"
#include "dialogs.h"
#include "view.h"
#include "lc_application.h"
#include "library.h"
#include "preview.h"
#include "camera.h"

View* view;

void create_main_menu (GtkObject *window, GtkWidget *vbox);

// Variables

//GtkWidget *main_window;
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
				if (home)
	strcpy(temppath, home);
				else
					strcpy(temppath, ".");
				path++;
      }

      if (last > (path+1))
      {
	strncat(temppath, path, (last-path));
	strcat(temppath, "/");
      }
      strcat (temppath, "./");
      strcat (temppath, argv0);

      if (access(temppath, X_OK) == 0 )
				found++;
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

  lcGetActiveProject()->HandleCommand((LC_COMMANDS)GPOINTER_TO_INT(data), 0);
}

static void view_destroy (GtkWidget *widget, gpointer data)
{
  delete (View*)data;
}

void OnCommand(GtkWidget* widget, gpointer data)
{
  Project* project = lcGetActiveProject();
  int id = GPOINTER_TO_INT(data);

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

    case ID_SNAP_X:
    {
      project->HandleCommand(LC_TOOLBAR_SNAPMENU, 0);
    } break;

    case ID_SNAP_Y:
    {
      project->HandleCommand(LC_TOOLBAR_SNAPMENU, 1);
    } break;

    case ID_SNAP_Z:
    {
      project->HandleCommand(LC_TOOLBAR_SNAPMENU, 2);
    } break;

    case ID_SNAP_ALL:
    {
      project->HandleCommand(LC_TOOLBAR_SNAPMENU, 3);
    } break;

    case ID_SNAP_NONE:
    {
      project->HandleCommand(LC_TOOLBAR_SNAPMENU, 4);
    } break;

    case ID_SNAP_ON:
    {
      project->HandleCommand(LC_TOOLBAR_SNAPMENU, 6);
    } break;

    case ID_LOCK_X:
    {
      project->HandleCommand(LC_TOOLBAR_LOCKMENU, 0);
    } break;

    case ID_LOCK_Y:
    {
      project->HandleCommand(LC_TOOLBAR_LOCKMENU, 1);
    } break;

    case ID_LOCK_Z:
    {
      project->HandleCommand(LC_TOOLBAR_LOCKMENU, 2);
    } break;

    case ID_LOCK_NONE:
    {
      project->HandleCommand(LC_TOOLBAR_LOCKMENU, 3);
    } break;

    case ID_LOCK_ON:
    {
      project->HandleCommand(LC_TOOLBAR_LOCKMENU, 4);
    } break;

    case ID_VIEW_CREATE:
    {
      GtkWidget *wnd, *w, *frame;

      wnd = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      //      gtk_window_set_transient_for (GTK_WINDOW (wnd), GTK_WINDOW (((GtkWidget*)(*main_window))));
      gtk_window_set_title (GTK_WINDOW (wnd), "View");
      //      gtk_window_set_default_size (GTK_WINDOW (pieces_parent), pieces_width, -1);

      frame = gtk_frame_new (NULL);
      gtk_widget_show (frame);
      gtk_container_add (GTK_CONTAINER (wnd), frame);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);               

      View *v = new View (project, view);
      v->CreateFromWindow (&w);

      gtk_container_add (GTK_CONTAINER (frame), w);
      gtk_widget_show (w);

      gtk_signal_connect (GTK_OBJECT (wnd), "destroy", GTK_SIGNAL_FUNC (view_destroy), v);
      gtk_widget_show (wnd);
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

    case ID_VIEW_TOOLBAR_MODIFY:
    {
      modifydlg_toggle ();
    } break;

    case ID_VIEW_TOOLBAR_PIECES:
    {
      gpointer widget = gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_view_toolbar_floating");

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

static gint key_press_event(GtkWidget* widget, GdkEventKey* event, gpointer data)
{
  char code = 0;

  if (event->keyval < 0x100)
    code = gdk_keyval_to_upper(event->keyval);
  else
  {
    switch (event->keyval)
    {
    case GDK_KP_0: code = '0'; break;
    case GDK_KP_1: code = '1'; break;
    case GDK_KP_2: code = '2'; break;
    case GDK_KP_3: code = '3'; break;
    case GDK_KP_4: code = '4'; break;
    case GDK_KP_5: code = '5'; break;
    case GDK_KP_6: code = '6'; break;
    case GDK_KP_7: code = '7'; break;
    case GDK_KP_8: code = '8'; break;
    case GDK_KP_9: code = '9'; break;
    case GDK_KP_Add: code = KEY_PLUS; break;
    case GDK_KP_Subtract: code = KEY_MINUS; break;
    case GDK_Shift_L: case GDK_Shift_R: code = KEY_SHIFT; break;
    case GDK_Control_L: case GDK_Control_R: code = KEY_CONTROL; break;
    case GDK_Escape: code = KEY_ESCAPE; break;
    case GDK_Tab: code = KEY_TAB; break;
    case GDK_Insert: case GDK_KP_Insert: code = KEY_INSERT; break;
    case GDK_Delete: case GDK_KP_Delete: code = KEY_DELETE; break;
    case GDK_Up: case GDK_KP_Up: code = KEY_UP; break;
    case GDK_Down: case GDK_KP_Down: code = KEY_DOWN; break;
    case GDK_Left: case GDK_KP_Left: code = KEY_LEFT; break;
    case GDK_Right: case GDK_KP_Right: code = KEY_RIGHT; break;
    case GDK_Prior: case GDK_KP_Prior: code = KEY_PRIOR; break;
    case GDK_Next:  case GDK_KP_Next: code = KEY_NEXT; break;
    }
  }

  if ((code >= '0') && (code <= '9') && ((event->state & GDK_CONTROL_MASK) == 0))
  {
    if (event->state & GDK_SHIFT_MASK)
      lcGetActiveProject()->HandleCommand((LC_COMMANDS)(LC_EDIT_MOVEZ_SNAP_0 + code - '0'), 0);
    else
      lcGetActiveProject()->HandleCommand((LC_COMMANDS)(LC_EDIT_MOVEXY_SNAP_0 + code - '0'), 0);

    return TRUE;
  }

  if (code != 0)
  {
    if (lcGetActiveProject()->OnKeyDown(code, (event->state & GDK_CONTROL_MASK) != 0, 
			   (event->state & GDK_SHIFT_MASK) != 0))
	gtk_signal_emit_stop_by_name (GTK_OBJECT(widget), "key_press_event");
  }

  return TRUE;
}

static void main_destroy ()
{
  gpointer item;
  int i = 0;

  // Save window position/size
  Sys_ProfileSaveInt ("Window", "Width", ((GtkWidget*)(*main_window))->allocation.width);
  Sys_ProfileSaveInt ("Window", "Height", ((GtkWidget*)(*main_window))->allocation.height);

  // Save toolbar state
  Sys_ProfileSaveInt ("Toolbars", "Standard", ((GTK_WIDGET_VISIBLE (main_toolbar.handle_box)) ? 1 : 0));
  Sys_ProfileSaveInt ("Toolbars", "Drawing", ((GTK_WIDGET_VISIBLE (tool_toolbar.handle_box)) ? 1 : 0));
  Sys_ProfileSaveInt ("Toolbars", "Animation", ((GTK_WIDGET_VISIBLE (anim_toolbar.handle_box)) ? 1 : 0));
  Sys_ProfileSaveInt ("Toolbars", "Pieces", (pieces_visible ? 1 : 0));
  Sys_ProfileSaveInt ("Toolbars", "Floating", (pieces_floating ? 1 : 0));

  if (pieces_parent != NULL)
    Sys_ProfileSaveInt ("Toolbars", "PiecesWidth", pieces_frame->allocation.width);

  item = gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_view_toolbar_icons");
  if (GTK_CHECK_MENU_ITEM (item)->active)
    i = 0;
  else
  {
    item = gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_view_toolbar_both");
    if (GTK_CHECK_MENU_ITEM (item)->active)
      i = 1;
    else
    {
      item = gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_view_toolbar_text");
      if (GTK_CHECK_MENU_ITEM (item)->active)
	i = 2;
    }
  }
  Sys_ProfileSaveInt ("Toolbars", "Style", i);

  gtk_main_quit ();
}

static gint main_quit (GtkWidget *widget, GdkEvent* event, gpointer data)
{
  if (!lcGetActiveProject()->SaveModified())
    return TRUE;

  g_App->Shutdown();
  delete g_App;

  // save window position
  gint x, y;
  gdk_window_get_root_origin (((GtkWidget*)(*main_window))->window, &x, &y);
  Sys_ProfileSaveInt ("Window", "PositionX", x);
  Sys_ProfileSaveInt ("Window", "PositionY", y);

  gtk_widget_destroy (((GtkWidget*)(*main_window)));

  delete main_window;
  main_window = NULL;

  return FALSE;
}

static gint pieces_close (GtkWidget *widget, GdkEvent* event, gpointer data)
{
  gpointer item;

  pieces_visible = FALSE;

  item = gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_view_toolbar_pieces");
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), FALSE);

  item = gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_view_toolbar_floating");
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
    view->CreateFromWindow (&drawing_area);

    gtk_widget_set_events (GTK_WIDGET (drawing_area), GDK_EXPOSURE_MASK | GDK_KEY_PRESS_MASK |
			   GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
			   GDK_POINTER_MOTION_HINT_MASK);

    gtk_signal_connect (GTK_OBJECT (drawing_area), "key_press_event",
			GTK_SIGNAL_FUNC (key_press_event), NULL);
 
    // set minimum size
    gtk_widget_set_usize (GTK_WIDGET (drawing_area), 100, 100);

    drawing_frame = gtk_frame_new (NULL);
    gtk_widget_show (drawing_frame);
    //    gtk_container_add (GTK_CONTAINER (hbox), drawing_frame);
    gtk_frame_set_shadow_type (GTK_FRAME (drawing_frame), GTK_SHADOW_IN);               

    gtk_container_add (GTK_CONTAINER (drawing_frame), GTK_WIDGET (drawing_area));
    gtk_widget_show (GTK_WIDGET (drawing_area));

    // now create the pieces bar
    pieces_frame = create_piecebar (((GtkWidget*)(*main_window)), view);
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
      gtk_window_set_transient_for (GTK_WINDOW (pieces_parent), GTK_WINDOW (((GtkWidget*)(*main_window))));
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
	gtk_paned_set_position (GTK_PANED (pieces_parent), ((GtkWidget*)(*main_window))->allocation.width -
				pieces_width);
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
  GtkWidget *vbox;
  int x, y;

  init_paths (argv[0]);

  gtk_set_locale ();
  gtk_init (&argc, &argv);

  atexit (GL_Shutdown);

  // Initialize the application.
  g_App = new lcApplication();
  main_window = new MainWnd();

  if (!g_App->Initialize(argc, argv, lib_path))
    return 1;

  if (pfnglXQueryExtension (GDK_DISPLAY (), NULL, NULL) != True)
  {
    g_print("ERROR: OpenGL not supported\n");
    return 1;
  }

  view = new View (lcGetActiveProject(), NULL);
  view->m_Camera = lcGetActiveProject()->GetCamera(LC_CAMERA_MAIN);

  //  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (((GtkWidget*)(*main_window))), "LeoCAD");
  gtk_container_border_width (GTK_CONTAINER (((GtkWidget*)(*main_window))), 0);
  gtk_widget_realize (((GtkWidget*)(*main_window)));

  // Read window position and size
  x = Sys_ProfileLoadInt ("Window", "Width", 600);
  y = Sys_ProfileLoadInt ("Window", "Height", 400);
  gtk_window_set_default_size (GTK_WINDOW (((GtkWidget*)(*main_window))), x, y);

  x = Sys_ProfileLoadInt ("Window", "PositionX", -1);
  y = Sys_ProfileLoadInt ("Window", "PositionY", -1);
  if ((x != -1 && y != -1) &&
      (x < gdk_screen_width () && y < gdk_screen_height ()))
    gtk_widget_set_uposition (((GtkWidget*)(*main_window)), x, y);

  gtk_signal_connect (GTK_OBJECT (((GtkWidget*)(*main_window))), "delete_event", (GtkSignalFunc) main_quit, NULL);
  gtk_signal_connect (GTK_OBJECT (((GtkWidget*)(*main_window))), "destroy", (GtkSignalFunc) main_destroy, NULL);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (((GtkWidget*)(*main_window))), vbox);
  gtk_widget_show (vbox);

//  startup_message ("Creating Main Menu ...");
  create_main_menu (GTK_OBJECT (((GtkWidget*)(*main_window))), vbox);

//  startup_message ("Creating Toolbars ...");
  create_toolbars (((GtkWidget*)(*main_window)), vbox);

  main_hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (vbox), main_hbox);
  gtk_widget_show (main_hbox);

  // load toolbar preferences and update the menu
  int show;
  ignore_commands = true;

  show = Sys_ProfileLoadInt ("Toolbars", "Standard", 1);
  if (show)
  {
    gpointer widget = gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_view_toolbar_standard");
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), TRUE);
  }
  else
    gtk_widget_hide (main_toolbar.handle_box);

  show = Sys_ProfileLoadInt ("Toolbars", "Drawing", 1);
  if (show)
  {
    gpointer widget = gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_view_toolbar_drawing");
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), TRUE);
  }
  else
    gtk_widget_hide (tool_toolbar.handle_box);

  show = Sys_ProfileLoadInt ("Toolbars", "Animation", 1);
  if (show)
  {
    gpointer widget = gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_view_toolbar_animation");
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), TRUE);
  }
  else
    gtk_widget_hide (anim_toolbar.handle_box);

  show = Sys_ProfileLoadInt ("Toolbars", "Pieces", 1);
  if (show)
  {
    gpointer widget = gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_view_toolbar_pieces");
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), TRUE);
    pieces_visible = TRUE;
  }
  else
  {
    gpointer widget = gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_view_toolbar_floating");
    gtk_widget_set_sensitive (GTK_WIDGET (widget), FALSE);
  }

  show = Sys_ProfileLoadInt ("Toolbars", "PiecesFloating", 0);
  if (show)
  {
    gpointer widget = gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_view_toolbar_floating");
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), TRUE);
    pieces_floating = TRUE;
  }

  ignore_commands = false;

  show = Sys_ProfileLoadInt ("Toolbars", "Style", 0);
  gpointer item = NULL;

  switch (show)
  {
  case 0:
    item = gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_view_toolbar_icons");
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
    OnCommand (GTK_WIDGET (item), GINT_TO_POINTER (ID_VIEW_TOOLBAR_ICONS));
    break;
  case 1:
    item = gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_view_toolbar_both");
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
    OnCommand (GTK_WIDGET (item), GINT_TO_POINTER (ID_VIEW_TOOLBAR_BOTH));
    break;
  case 2:
    item = gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_view_toolbar_text");
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
    OnCommand (GTK_WIDGET (item), GINT_TO_POINTER (ID_VIEW_TOOLBAR_TEXT));
    break;
  }

  pieces_width = Sys_ProfileLoadInt ("Toolbars", "PiecesWidth", 210);
  update_window_layout ();

  // increase the reference count to allow changing parents when hiding the pieces bar
  gtk_widget_ref (drawing_frame);
  gtk_widget_ref (pieces_frame);

  create_statusbar (((GtkWidget*)(*main_window)), vbox);
//  gtk_box_pack_start (GTK_BOX (vbox), create_status_bar (), FALSE, TRUE, 2);
  //  GtkWidget* statusbar = gtk_statusbar_new ();
  //gtk_widget_show (statusbar);
  //gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, TRUE, 0);   

#include "pixmaps/icon32.xpm"

	lcGetActiveProject()->UpdateInterface();
  main_window->UpdateMRU ();

  GdkPixmap *gdkpixmap;
  GdkBitmap *mask;

  gdkpixmap = gdk_pixmap_create_from_xpm_d (((GtkWidget*)(*main_window))->window, &mask,
                                           &((GtkWidget*)(*main_window))->style->bg[GTK_STATE_NORMAL], (gchar**)icon32);
  gdk_window_set_icon (((GtkWidget*)(*main_window))->window, NULL, gdkpixmap, mask);

  gtk_widget_show (GTK_WIDGET (((GtkWidget*)(*main_window))));

  // get the splitter in the correct size, must be done after the widget has been realized
  if ((pieces_floating == FALSE) && (pieces_visible == TRUE))
    gtk_paned_set_position (GTK_PANED (pieces_parent), ((GtkWidget*)(*main_window))->allocation.width -
			    pieces_width);


  PieceInfo* Info = lcGetPiecesLibrary()->FindPieceInfo("3005");
  if (!Info)
    Info = lcGetPiecesLibrary()->GetPieceInfo(0);
  if (Info)
  {
    lcGetActiveProject()->SetCurrentPiece(Info);
    extern PiecePreview* preview;
    preview->SetCurrentPiece(Info);
  }

  gtk_main();

  gtk_widget_unref (drawing_frame);
  gtk_widget_unref (pieces_frame);

  //  delete project;
  _exit (0); // FIXME !
  return 0;
}
