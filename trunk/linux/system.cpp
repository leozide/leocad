#include "lc_global.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <X11/keysym.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include "opengl.h"
#include "gtkmisc.h"
#include "camera.h"
#include "project.h"
#include "system.h"
#include "main.h"
#include "toolbar.h"
#include "dialogs.h"
#include "globals.h"
#include "lc_application.h"

// =============================================================================
// Cursor functions

int Sys_MessageBox (const char* text, const char* caption, int type)
{
  return msgbox_execute (text, caption, type);
}

// =============================================================================
// Memory rendering

typedef struct
{
  int width, height;
  Display   *xdisplay;
  GLXPixmap glxpixmap;
  GLXContext glxcontext;
  GdkPixmap *pixmap;
  GdkVisual *visual;
} LC_RENDER;

void* Sys_StartMemoryRender(int width, int height)
{
  LC_RENDER* render = (LC_RENDER*) malloc (sizeof (LC_RENDER));
  int attrlist[] = { GLX_RGBA, GLX_DEPTH_SIZE, 16, 0 };
  XVisualInfo *vi;
  Pixmap xpixmap;

  render->width = width;
  render->height = height;
  render->xdisplay = GDK_DISPLAY();

  vi = pfnglXChooseVisual (render->xdisplay, DefaultScreen (render->xdisplay), attrlist);

  render->visual = gdkx_visual_get (vi->visualid);
  render->glxcontext = pfnglXCreateContext (render->xdisplay, vi, NULL, True);
  render->pixmap = gdk_pixmap_new (NULL, width, height, render->visual->depth);

  xpixmap = (Pixmap)GDK_DRAWABLE_XID(render->pixmap);
  render->glxpixmap = pfnglXCreateGLXPixmap (render->xdisplay, vi, xpixmap);

  XFree(vi);

  pfnglXMakeCurrent (render->xdisplay, render->glxpixmap, render->glxcontext);

  return render;
}

void Sys_FinishMemoryRender(void* param)
{
  LC_RENDER* render = (LC_RENDER*)param;

  //  gtk_gl_area_make_current (GTK_GL_AREA (drawing_area));

  if (render->glxcontext == pfnglXGetCurrentContext ())
    pfnglXMakeCurrent (render->xdisplay, None, NULL);
  pfnglXDestroyContext (render->xdisplay, render->glxcontext);

  pfnglXDestroyGLXPixmap (render->xdisplay, render->glxpixmap);
  pfnglXWaitGL();
  gdk_pixmap_unref (render->pixmap);
  pfnglXWaitX();
  free(render);
}

// =============================================================================
// Misc stuff

// FIXME: should have a table of LC_KEY_* defined
bool Sys_KeyDown(int Key)
{
	char keys[32];
	int x;

	XQueryKeymap(GDK_DISPLAY(), keys);

	if (Key == KEY_CONTROL)
	{
		x = XKeysymToKeycode(GDK_DISPLAY(), XK_Control_L);
		if (keys[x/8] & (1 << (x % 8)))
			return true;

		x = XKeysymToKeycode(GDK_DISPLAY(), XK_Control_R);
		if (keys[x/8] & (1 << (x % 8)))
			return true;
	}
	else if (Key == KEY_ALT)
	{
		x = XKeysymToKeycode(GDK_DISPLAY(), XK_Alt_L);
		if (keys[x/8] & (1 << (x % 8)))
			return true;

		x = XKeysymToKeycode(GDK_DISPLAY(), XK_Alt_R);
		if (keys[x/8] & (1 << (x % 8)))
			return true;
	}

	return false;
}

void Sys_GetFileList(const char* Path, ObjArray<String>& FileList)
{
	struct dirent* Entry;
	char DirPath[LC_MAXPATH], FilePath[LC_MAXPATH];

	strcpy(DirPath, Path);
	int Length = strlen(DirPath);
	if (DirPath[Length - 1] != '/')
		strcat(DirPath, "/");

	FileList.RemoveAll();

	DIR* Dir = opendir(DirPath);
	if (!Dir)
	{
		printf("Couldn't open directory.\n");
		return;
	}

	while ((Entry = readdir(Dir)))
	{
		int Length;

		if (Entry->d_type != DT_REG)
			continue;

		Length = strlen(Entry->d_name);

		if (Length < 5)
			continue;

		if (strcmp(Entry->d_name + Length - 4, ".dat"))
			continue;

		sprintf(FilePath, "%s%s", DirPath, Entry->d_name);
		FileList.Add(FilePath);
	}

	closedir(Dir);
}

// String
char* strupr(char* string)
{
  char *cp;
  for (cp=string; *cp; ++cp)
  {
    if ('a' <= *cp && *cp <= 'z')
      *cp += 'A' - 'a';
  }

  return string;
}

char* strlwr(char* string)
{
  char *cp;
  for (cp = string; *cp; ++cp)
  {
    if ('A' <= *cp && *cp <= 'Z')
      *cp += 'a' - 'A';
  }

  return string;
}

int stricmp(const char* str1, const char* str2)
{
  return g_ascii_strcasecmp(str1, str2);
}




void SystemPumpMessages()
{
  while (gtk_events_pending ())
    gtk_main_iteration ();
}

long SystemGetTicks()
{
  static int basetime = 0;
  struct timezone tzp;
  struct timeval tp;

  gettimeofday (&tp, &tzp);

  if (!basetime)
    basetime = tp.tv_sec;

  return (tp.tv_sec-basetime)*1000 + tp.tv_usec/1000;
}

// User Interface
void SystemUpdateCategories(bool SearchOnly)
{
}

static void create_bitmap_and_mask_from_xpm (GdkBitmap **bitmap, GdkBitmap **mask, const char **xpm)
{
  int height, width, colors;
  char pixmap_buffer [(32 * 32)/8];
  char mask_buffer [(32 * 32)/8];
  int x, y, pix;
  int transparent_color, black_color;

  sscanf (xpm [0], "%d %d %d %d", &height, &width, &colors, &pix);

  g_assert (height == 32);
  g_assert (width  == 32);
  g_assert (colors == 3);

  transparent_color = ' ';
  black_color = '.';

  for (y = 0; y < 32; y++)
    for (x = 0; x < 32;)
    {
      char value = 0, maskv = 0;

      for (pix = 0; pix < 8; pix++, x++)
	if (xpm [4+y][x] != transparent_color)
	{
	  maskv |= 1 << pix;

	  if (xpm [4+y][x] != black_color)
	    value |= 1 << pix;
	}

      pixmap_buffer [(y * 4 + x/8)-1] = value;
      mask_buffer [(y * 4 + x/8)-1] = maskv;
    }

  *bitmap = gdk_bitmap_create_from_data (NULL, pixmap_buffer, 32, 32);
  *mask   = gdk_bitmap_create_from_data (NULL, mask_buffer, 32, 32);
}

void SystemUpdateAction(int new_action, int old_action)
{
#include "pixmaps/cr_brick.xpm"
#include "pixmaps/cr_light.xpm"
#include "pixmaps/cr_spot.xpm"
#include "pixmaps/cr_cam.xpm"
#include "pixmaps/cr_sel.xpm"
#include "pixmaps/cr_selm.xpm"
#include "pixmaps/cr_move.xpm"
#include "pixmaps/cr_rot.xpm"
#include "pixmaps/cr_paint.xpm"
#include "pixmaps/cr_erase.xpm"
#include "pixmaps/cr_pan.xpm"
#include "pixmaps/cr_rotv.xpm"
#include "pixmaps/cr_roll.xpm"
#include "pixmaps/cr_zoom.xpm"
#include "pixmaps/cr_zoomr.xpm"

  if (!drawing_area)
    return;

  GtkWidget* button;
  const char** xpm = NULL;
  int x, y;

  switch (new_action)
  {
  case LC_ACTION_SELECT: button = tool_toolbar.select;
  {
    x = 0; y = 2;
    // TODO: FIX ME !!!
    if (Sys_KeyDown (KEY_CONTROL))
      xpm = cr_selm;
    else
      xpm = cr_sel;
  } break;
  case LC_ACTION_INSERT:
    button = tool_toolbar.brick; xpm = cr_brick; x = 8; y = 3; break;
  case LC_ACTION_LIGHT:
    button = tool_toolbar.light; xpm = cr_light; x = 15; y = 15; break;
  case LC_ACTION_SPOTLIGHT:
    button = tool_toolbar.spot; xpm = cr_spot; x = 7; y = 10; break;
  case LC_ACTION_CAMERA:
    button = tool_toolbar.camera; xpm = cr_cam; x = 15; y = 9; break;
  case LC_ACTION_MOVE:
    button = tool_toolbar.move; xpm = cr_move; x = 15; y = 15; break;
  case LC_ACTION_ROTATE:
    button = tool_toolbar.rotate; xpm = cr_rot; x = 15; y = 15; break;
  case LC_ACTION_ERASER:
    button = tool_toolbar.erase; xpm = cr_erase; x = 0; y = 10; break;
  case LC_ACTION_PAINT:
    button = tool_toolbar.paint; xpm = cr_paint; x = 14; y = 14; break;
  case LC_ACTION_ZOOM:
    button = tool_toolbar.zoom; xpm = cr_zoom; x = 15; y = 15; break;
  case LC_ACTION_ZOOM_REGION:
    button = tool_toolbar.zoomreg; xpm = cr_zoomr; x = 9; y = 9; break;
  case LC_ACTION_PAN:
    button = tool_toolbar.pan; xpm = cr_pan; x = 15; y = 15; break;
  case LC_ACTION_ROTATE_VIEW:
    button = tool_toolbar.rotview; xpm = cr_rotv; x = 15; y = 15; break;
  case LC_ACTION_ROLL:
    button = tool_toolbar.roll; xpm = cr_roll; x = 15; y = 15; break;
  default:
    return;
  }

  GdkBitmap *bitmap;
  GdkBitmap *mask;
  GdkCursor *cursor;
  GdkColor white = {0, 0xffff, 0xffff, 0xffff};
  GdkColor black = {0, 0x0000, 0x0000, 0x0000};

  if (GDK_IS_WINDOW(drawing_area))
  {
    if (xpm != NULL)
    {
      create_bitmap_and_mask_from_xpm (&bitmap, &mask, xpm);
      cursor = gdk_cursor_new_from_pixmap (bitmap, mask, &white, &black, x, y);
      gdk_window_set_cursor (drawing_area->window, cursor);
    }
    else
    {
      cursor = gdk_cursor_new (GDK_LEFT_PTR);
      gdk_window_set_cursor (drawing_area->window, cursor);
      gdk_cursor_destroy (cursor);
    }
  }

  ignore_commands = true;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  ignore_commands = false;
}

void SystemUpdateColorList(int new_color)
{
  colorlist_set(new_color);
}

void SystemUpdateRenderingMode(bool bFast)
{
  if (!main_toolbar.fast)
    return;

  ignore_commands = true;
  gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(main_toolbar.fast), bFast);
  ignore_commands = false;
}

void SystemUpdateUndoRedo(char* undo, char* redo)
{
  gpointer item;
  char text[50];

  strcpy(text, "Undo ");
  if (undo)
    strcat(text, undo);
  item = gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_edit_undo");

  if (!item)
    return;

  gtk_label_set_text (GTK_LABEL (GTK_BIN (item)->child), text);
  gtk_widget_set_sensitive (GTK_WIDGET (item), undo != NULL);

  strcpy(text, "Redo ");
  if (redo)
    strcat(text, redo);
  item = gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_edit_redo");
  gtk_label_set_text (GTK_LABEL (GTK_BIN (item)->child), text);
  gtk_widget_set_sensitive (GTK_WIDGET (item), redo != NULL);

  gtk_widget_set_sensitive (main_toolbar.undo, undo != NULL);
  gtk_widget_set_sensitive (main_toolbar.redo, redo != NULL);
}

void SystemUpdateSnap(const unsigned long snap)
{
  if (!main_toolbar.angle)
    return;

  ignore_commands = true;
  gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(main_toolbar.angle), (snap & LC_DRAW_SNAP_A) != 0);
  ignore_commands = false;

  void* item;

  ignore_commands = true;

  item = gtk_object_get_data(GTK_OBJECT(main_toolbar.snap_menu), "snap_x");
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), (snap & LC_DRAW_SNAP_X) ? TRUE : FALSE);
  item = gtk_object_get_data(GTK_OBJECT(main_toolbar.snap_menu), "snap_y");
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), (snap & LC_DRAW_SNAP_Y) ? TRUE : FALSE);
  item = gtk_object_get_data(GTK_OBJECT(main_toolbar.snap_menu), "snap_z");
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), (snap & LC_DRAW_SNAP_Z) ? TRUE : FALSE);

  item = gtk_object_get_data(GTK_OBJECT(main_toolbar.lock_menu), "lock_x");
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), (snap & LC_DRAW_LOCK_X) ? TRUE : FALSE);
  item = gtk_object_get_data(GTK_OBJECT(main_toolbar.lock_menu), "lock_y");
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), (snap & LC_DRAW_LOCK_Y) ? TRUE : FALSE);
  item = gtk_object_get_data(GTK_OBJECT(main_toolbar.lock_menu), "lock_z");
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), (snap & LC_DRAW_LOCK_Z) ? TRUE : FALSE);

  ignore_commands = false;
}

void SystemUpdateCurrentCamera(Camera* pOld, Camera* pNew, const PtrArray<Camera>& Cameras)
{
	gpointer item = NULL;
	gpointer menu = gtk_object_get_data(GTK_OBJECT(((GtkWidget*)(*main_window))), "cameras_menu");

	if (!menu)
		return;

	GList *lst = gtk_container_children(GTK_CONTAINER(menu));
	Project* project = lcGetActiveProject();

	for (int CameraIdx = 0; CameraIdx < project->mCameras.GetSize(); CameraIdx++)
	{
		if (pNew != project->mCameras[CameraIdx])
			continue;

		item = g_list_nth_data(lst, CameraIdx);
		break;
	}

	if (item)
	{
		ignore_commands = true;
		gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM (item), TRUE);  
		ignore_commands = false;
	}
}

void SystemUpdateCameraMenu(const PtrArray<Camera>& Cameras)
{
	GtkWidget *menu = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT (((GtkWidget*)(*main_window))), "cameras_menu"));
	GtkWidget *item = NULL;
	GList *lst;

	if (!menu)
		return;

	while ((lst = gtk_container_children(GTK_CONTAINER(menu))) != NULL)
		gtk_container_remove(GTK_CONTAINER(menu), GTK_WIDGET(lst->data));

	Project* project = lcGetActiveProject();

	for (int CameraIdx = 0; CameraIdx < project->mCameras.GetSize(); CameraIdx++)
	{
		GSList* grp = item ? gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(item)) : NULL;
		item = gtk_radio_menu_item_new_with_label(grp, project->mCameras[CameraIdx]->GetName());
		gtk_menu_append(GTK_MENU(menu), item);
		gtk_widget_show(item);
		gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(OnCommand), GINT_TO_POINTER(CameraIdx + ID_CAMERA_FIRST));
	}

	if (project->mCameras.GetSize())
		menu_separator(menu);

//	create_menu_item(menu, "_Reset", accel, GTK_SIGNAL_FUNC(OnCommandDirect), window, LC_VIEW_CAMERA_RESET, "menu_cameras_reset");
}

void SystemUpdateTime(bool bAnimation, int nTime, int nTotal)
{
  GtkWidget *item;

  if (!anim_toolbar.first)
    return;

  gtk_widget_set_sensitive (anim_toolbar.first, nTime != 1);
  gtk_widget_set_sensitive (anim_toolbar.prev, nTime > 1);
  gtk_widget_set_sensitive (anim_toolbar.next, nTime < nTotal);
  gtk_widget_set_sensitive (anim_toolbar.last, nTime != nTotal);
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_view_step_first"));
  gtk_widget_set_sensitive (item, nTime != 1);
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_view_step_previous"));
  gtk_widget_set_sensitive (item, nTime > 1);
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_view_step_next"));
  gtk_widget_set_sensitive (item, nTime < nTotal);
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_view_step_last"));
  gtk_widget_set_sensitive (item, nTime != nTotal);

  char text[11];
  if (bAnimation)
    sprintf(text, "%i/%i", nTime, nTotal);
  else
    sprintf(text, " Step %i ", nTime);
  gtk_label_set (GTK_LABEL (label_step), text);

  // step dlg
}

void SystemUpdateAnimation(bool bAnimation, bool bAddKeys)
{
  if (!anim_toolbar.play)
    return;

  ignore_commands = true;
  gtk_widget_set_sensitive (anim_toolbar.play, bAnimation);
  gtk_widget_set_sensitive (anim_toolbar.stop, FALSE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(anim_toolbar.anim), bAnimation);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(anim_toolbar.keys), bAddKeys);
  gpointer item = gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_piece_copykeys");
  gtk_label_set_text (GTK_LABEL (GTK_BIN (item)->child), 
      bAnimation ? "Copy Keys from Instructions" : "Copy Keys from Animation");
  ignore_commands = false;
}

void SystemUpdateSnap(unsigned short move_snap, unsigned short RotateSnap)
{
	if (!label_snap)
		return;

	char Text[256], xy[32], z[32];

	lcGetActiveProject()->GetSnapDistanceText(xy, z);

	sprintf(Text, " M: %s %s R: %d ", xy, z, RotateSnap);

	gtk_label_set (GTK_LABEL (label_snap), Text);
}

void SystemUpdateSelected(unsigned long flags, int SelectedCount, Object* Focus)
{
  GtkWidget *item;

  // select all/none/invert/by name (menu)
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_edit_select_all"));

  if (!item)
    return;

  gtk_widget_set_sensitive (item, (flags & LC_SEL_UNSELECTED) != 0);
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_edit_select_none"));
  gtk_widget_set_sensitive (item, (flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT)) != 0);
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_edit_select_invert"));
  gtk_widget_set_sensitive (item, (flags & LC_SEL_NO_PIECES) == 0);
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_edit_select_byname"));
  gtk_widget_set_sensitive (item, (flags & LC_SEL_NO_PIECES) == 0);

  // cut, copy (menu/toolbar)
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_edit_cut"));
  gtk_widget_set_sensitive (item, (flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT)) != 0);
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_edit_copy"));
  gtk_widget_set_sensitive (item, (flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT)) != 0);
  gtk_widget_set_sensitive (main_toolbar.cut, (flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT)) != 0);
  gtk_widget_set_sensitive (main_toolbar.copy, (flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT)) != 0);

  // delete, array, hide sel/unsel, unhideall, copykeys (menu)
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_piece_delete"));
  gtk_widget_set_sensitive (item, (flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT)) != 0);
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_piece_array"));
  gtk_widget_set_sensitive (item, (flags & LC_SEL_PIECE) != 0);
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_piece_hide_selected"));
  gtk_widget_set_sensitive (item, (flags & LC_SEL_PIECE) != 0);
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_piece_hide_unselected"));
  gtk_widget_set_sensitive (item, (flags & LC_SEL_UNSELECTED) != 0);
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_piece_unhide_all"));
  gtk_widget_set_sensitive (item, (flags & LC_SEL_HIDDEN) != 0);
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_piece_copykeys"));
  gtk_widget_set_sensitive (item, (flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT)) != 0);

  // groups (menu)
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_piece_group"));
  gtk_widget_set_sensitive (item, (flags & LC_SEL_CANGROUP) != 0);
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_piece_ungroup"));
  gtk_widget_set_sensitive (item, (flags & LC_SEL_GROUP) != 0);
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_piece_group_add"));
  gtk_widget_set_sensitive (item, (flags & (LC_SEL_GROUP|LC_SEL_FOCUSGROUP)) == LC_SEL_GROUP);
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_piece_group_remove"));
  gtk_widget_set_sensitive (item, (flags & LC_SEL_FOCUSGROUP) != 0);
  item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_piece_group_edit"));
  gtk_widget_set_sensitive (item, (flags & LC_SEL_NO_PIECES) == 0);

  gtk_widget_set_sensitive (tool_toolbar.prev, (flags & LC_SEL_PIECE) != 0);
  gtk_widget_set_sensitive (tool_toolbar.next, (flags & LC_SEL_PIECE) != 0);
}

void SystemUpdateRecentMenu (String names[4])
{
  GtkWidget *item;
  char buf[32];

  for (int i = 0; i < 4; i++)
  {
    sprintf (buf, "menu_file_recent%d", i+1);
    item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), buf));

    if (!names[i].IsEmpty ())
    {
      if (i == 0)
      {
	gtk_label_set_text (GTK_LABEL (GTK_BIN (item)->child), "Recent Files");
	gtk_widget_set_sensitive (item, FALSE);
      }
      else
	gtk_widget_hide (item);
    }
    else
    {
      char text[LC_MAXPATH+4];

      sprintf (text, "_%d- %s", i+1, (char*)names[i]);
      gtk_label_set_text_with_mnemonic(GTK_LABEL(GTK_BIN(item)->child), text);
      gtk_widget_show(item);
      gtk_widget_set_sensitive(item, TRUE);
    }
  }
}

void SystemUpdatePaste(bool enable)
{
  gtk_widget_set_sensitive (main_toolbar.paste, enable);
  GtkWidget *item = GTK_WIDGET (gtk_object_get_data (GTK_OBJECT (((GtkWidget*)(*main_window))), "menu_edit_paste"));
  gtk_widget_set_sensitive (item, enable);
}

void SystemUpdatePlay(bool play, bool stop)
{
  gtk_widget_set_sensitive (anim_toolbar.play, play);
  gtk_widget_set_sensitive (anim_toolbar.stop, stop);
}

void SystemInit()
{
}

void SystemFinish()
{
}

// FIXME: remove 
int SystemDoMessageBox(const char* prompt, int mode)
{
  return msgbox_execute (prompt, "LeoCAD", mode);
}

bool SystemDoDialog(int mode, void* param)
{
  switch (mode)
  {
    case LC_DLG_FILE_OPEN_PROJECT:
      return openprojectdlg_execute ((char*)param) == LC_OK;

    case LC_DLG_FILE_SAVE_PROJECT:
      return saveprojectdlg_execute ((char*)param) == LC_OK;

    case LC_DLG_FILE_MERGE_PROJECT:
      return openprojectdlg_execute ((char*)param) == LC_OK;

    case LC_DLG_FILE_OPEN:
      return openprojectdlg_execute ((char*)param) == LC_OK;

    case LC_DLG_ABOUT:
      return aboutdlg_execute(param) == LC_OK;

    case LC_DLG_ARRAY:
      return arraydlg_execute(param) == LC_OK;

    case LC_DLG_HTML:
      return htmldlg_execute(param) == LC_OK;

    case LC_DLG_POVRAY:
      return povraydlg_execute(param) == LC_OK;

    case LC_DLG_WAVEFRONT:
      return wavefrontdlg_execute(param) == LC_OK;

    case LC_DLG_PREFERENCES:
      return preferencesdlg_execute(param) == LC_OK;

    case LC_DLG_PICTURE_SAVE:
      return savepicturedlg_execute (param) == LC_OK;

    case LC_DLG_MINIFIG:
      return minifigdlg_execute(param) == LC_OK;

    case LC_DLG_PROPERTIES:
      return propertiesdlg_execute(param) == LC_OK;

    case LC_DLG_LIBRARY:
      return librarydlg_execute(param) == LC_OK;

    case LC_DLG_SELECTBYNAME:
      break;

    case LC_DLG_STEPCHOOSE:
      break;

    case LC_DLG_EDITGROUPS:
      return groupeditdlg_execute(param) == LC_OK;

    case LC_DLG_GROUP:
      return groupdlg_execute(param) == LC_OK;
  }

  return false;
}

void SystemDoPopupMenu(int nMenu, int x, int y)
{
}

void SystemDoWaitCursor(int code)
{
  GdkWindow* window = ((GtkWidget*)(*main_window))->window;

  if (!GDK_IS_WINDOW(window))
    return;

  if (code == 1)
  {
    GdkCursor *cursor = gdk_cursor_new (GDK_WATCH);
    gdk_window_set_cursor(window, cursor);
    gdk_cursor_destroy (cursor);
  } 
  else
  {
    GdkCursor *cursor = gdk_cursor_new (GDK_LEFT_PTR);
    gdk_window_set_cursor(window, cursor);
    gdk_cursor_destroy (cursor);
  }
}

void SystemExportClipboard(lcFile* clip)
{
}

lcFile* SystemImportClipboard()
{
  return NULL;
}

void SystemSetWindowCaption(char* caption)
{
  gtk_window_set_title (GTK_WINDOW (((GtkWidget*)(*main_window))), caption);
}

void SystemPieceComboAdd(char* name)
{
  piececombo_add(name);
}


void SystemCaptureMouse()
{
}

void SystemReleaseMouse()
{
}

void SystemStartProgressBar(int nLower, int nUpper, int nStep, const char* Text)
{
}

void SytemEndProgressBar()
{
}

void SytemStepProgressBar()
{
}
