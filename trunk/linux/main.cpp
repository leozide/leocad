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

void create_main_menu (GtkObject *window, GtkWidget *vbox);

// Variables

GtkWidget *main_window;
GtkWidget *drawing_area;

static char default_path[PATH_MAX];
bool ignore_commands = false;

void init_paths (char *argv0)
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
    argv0 = strrchr(argv0, '/')+1;

  if (realpath (temppath, default_path))
    *(strrchr (default_path, '/')) = '\0';
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
      if (!project->SaveModified())
	break;

      gtk_main_quit();
    } break;

    case ID_SNAP_A: {
      project->HandleCommand(LC_TOOLBAR_SNAPMENU, 5);
    } break;

    case ID_VIEW_TOOLBAR_STANDARD: {
      if (GTK_WIDGET_VISIBLE (main_toolbar.handle_box))
	gtk_widget_hide (main_toolbar.handle_box);
      else
	gtk_widget_show (main_toolbar.handle_box);
    } break;

    case ID_VIEW_TOOLBAR_DRAWING: {
      if (GTK_WIDGET_VISIBLE (tool_toolbar.handle_box))
	gtk_widget_hide (tool_toolbar.handle_box);
      else
	gtk_widget_show (tool_toolbar.handle_box);
    } break;

    case ID_VIEW_TOOLBAR_ANIMATION: {
       if (GTK_WIDGET_VISIBLE (anim_toolbar.handle_box))
	 gtk_widget_hide (anim_toolbar.handle_box);
       else
	 gtk_widget_show (anim_toolbar.handle_box);
    } break;

    case ID_VIEW_TOOLBAR_BOTH: {
      gtk_toolbar_set_style (GTK_TOOLBAR (main_toolbar.toolbar), GTK_TOOLBAR_BOTH);
      gtk_toolbar_set_style (GTK_TOOLBAR (tool_toolbar.toolbar), GTK_TOOLBAR_BOTH);
      gtk_toolbar_set_style (GTK_TOOLBAR (anim_toolbar.toolbar), GTK_TOOLBAR_BOTH);
      gtk_widget_set_usize (main_toolbar.handle_box, -1, -1);
      gtk_widget_set_usize (tool_toolbar.handle_box, -1, -1);
      gtk_widget_set_usize (anim_toolbar.handle_box, -1, -1);
    } break;

    case ID_VIEW_TOOLBAR_ICONS: {
      gtk_toolbar_set_style (GTK_TOOLBAR (main_toolbar.toolbar), GTK_TOOLBAR_ICONS);
      gtk_toolbar_set_style (GTK_TOOLBAR (tool_toolbar.toolbar), GTK_TOOLBAR_ICONS);
      gtk_toolbar_set_style (GTK_TOOLBAR (anim_toolbar.toolbar), GTK_TOOLBAR_ICONS);
      gtk_widget_set_usize (main_toolbar.handle_box, -1, -1);
      gtk_widget_set_usize (tool_toolbar.handle_box, -1, -1);
      gtk_widget_set_usize (anim_toolbar.handle_box, -1, -1);
    } break;

    case ID_VIEW_TOOLBAR_TEXT: {
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
  project->SetViewSize(allocation->width, allocation->height);

  return TRUE;
}

static gint main_quit (GtkWidget *widget, GdkEvent* event, gpointer data)
{
  if (!project->SaveModified())
    return TRUE;

  delete project;
  project = NULL;

  gtk_main_quit ();
  return FALSE;
}

int main(int argc, char* argv[])
{
  GtkWidget *vbox, *hbox, *frame;
  int attrlist[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 16, 0 };
  int i, j, k;
  char* libgl = NULL;

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
//  gtk_window_set_policy (GTK_WINDOW (window), TRUE, TRUE, FALSE);
  gtk_widget_realize (main_window);
// read preferences
  gtk_window_set_default_size (GTK_WINDOW (main_window), 600, 400);
 
  gtk_signal_connect (GTK_OBJECT (main_window), "delete_event", (GtkSignalFunc) main_quit, NULL);
  gtk_signal_connect (GTK_OBJECT (main_window), "destroy", (GtkSignalFunc) gtk_main_quit, NULL);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (main_window), vbox);
  gtk_widget_show (vbox);

//  startup_message ("Creating Main Menu ...");
  create_main_menu (GTK_OBJECT (main_window), vbox);

//  startup_message ("Creating Toolbars ...");
  create_toolbars (main_window, vbox);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (vbox), hbox);
  gtk_widget_show (hbox);

  /* You should always delete gtk_gl_widget widgets before exit or else
     GLX contexts are left undeleted, this may cause problems (=core dump)
     in some systems.
     Destroy method of objects is not automatically called on exit.
     You need to manually enable this feature. Do gtk_quit_add_destroy()
     for all your top level windows unless you are certain that they get
     destroy signal by other means.
  */
  //  gtk_quit_add_destroy(1, GTK_OBJECT(main_window));

  // Create new OpenGL widget.
  drawing_area = GTK_WIDGET(gtk_gl_area_new(attrlist));

  GTK_WIDGET_SET_FLAGS(drawing_area, GTK_CAN_FOCUS);

  gtk_widget_set_events(GTK_WIDGET(drawing_area),
			GDK_EXPOSURE_MASK |
			GDK_KEY_PRESS_MASK |
			GDK_BUTTON_PRESS_MASK |
			GDK_BUTTON_RELEASE_MASK |
			GDK_POINTER_MOTION_MASK |
			GDK_POINTER_MOTION_HINT_MASK);

  // Connect signal handlers
  gtk_signal_connect(GTK_OBJECT(drawing_area), "expose_event", 
      GTK_SIGNAL_FUNC(draw_view), NULL);
  gtk_signal_connect(GTK_OBJECT(drawing_area), "size_allocate",//configure_event",
      GTK_SIGNAL_FUNC(reshape_view), NULL);
  gtk_signal_connect(GTK_OBJECT(drawing_area), "realize",
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
  gtk_widget_set_usize(GTK_WIDGET(drawing_area), 100,100);

  frame = gtk_frame_new ((char*)NULL);
  gtk_widget_show (frame);
  gtk_container_add (GTK_CONTAINER (hbox), frame);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);               

  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET(drawing_area));
  gtk_widget_show(GTK_WIDGET(drawing_area));

  create_piecebar(main_window, hbox);
  create_statusbar(main_window, vbox);
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

  gtk_widget_show(GTK_WIDGET(main_window));

  char* path;
  path = getenv("LEOCAD_LIB");
  if (path == NULL)
    path = default_path;

  if (project->Initialize(argc, argv, path) == false)
  {
    delete project;
    //    return 1;
    _exit (1);
  }

  gtk_main();

  //  delete project;
  _exit (0); // FIXME !
  return 0;
}
