#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glx.h>
#include <gtk/gtk.h>
#include "gdkgl.h"
#include "gtkglarea.h"
#include "camera.h"
#include "project.h"
#include "system.h"
#include "main.h"
#include "menu.h"
#include "toolbar.h"
#include "dialogs.h"
#include "globals.h"

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



// Profile
int SystemGetProfileInt(const char* section, const char* entry, const int defaultvalue)
{
  return defaultvalue;
}

bool SystemSetProfileInt(const char* section, const char* entry, const int value)
{
  return true;
}

bool SystemSetProfileString(const char* section, const char* entry, const char* value)
{
  return true;
}

const char* SystemGetProfileString(const char* section, const char* entry, const char* defaultvalue)
{
  return defaultvalue;
}

// User Interface
void SystemUpdateViewport(int new_vp, int old_vp)
{
  ignore_commands = true;
  gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM (main_menu.view_viewports[new_vp]), TRUE);  
  ignore_commands = false;
}

static void create_bitmap_and_mask_from_xpm (GdkBitmap **bitmap, GdkBitmap **mask, gchar **xpm)
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

  GtkWidget* button;
  char** xpm = NULL;
  int x, y;

  switch (new_action)
  {
  case LC_ACTION_SELECT: button = tool_toolbar.select;
  {
    x = 0; y = 2;
    if (IsKeyDown(KEY_CONTROL))
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

  ignore_commands = true;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  ignore_commands = false;
}

void SystemSetGroup(int new_group)
{
  groupsbar_set(new_group);
}

void SystemUpdateColorList(int new_color)
{
  colorlist_set(new_color);
}

void SystemUpdateRenderingMode(bool bBackground, bool bFast)
{
  ignore_commands = true;
  if (bFast)
  {
    gtk_widget_set_sensitive (main_toolbar.bg, TRUE);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(main_toolbar.bg), bBackground);
  }
  else
  {
    gtk_widget_set_sensitive (main_toolbar.bg, FALSE);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(main_toolbar.bg), FALSE);
  }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(main_toolbar.fast), bFast);
  ignore_commands = false;
}

void SystemUpdateUndoRedo(char* undo, char* redo)
{
  char text[50];

  strcpy(text, "Undo ");
  if (undo)
    strcat(text, undo);
  gtk_label_set_text (GTK_LABEL(GTK_BIN(main_menu.edit_undo)->child), text);
  strcpy(text, "Redo ");
  if (redo)
    strcat(text, redo);
  gtk_label_set_text (GTK_LABEL(GTK_BIN(main_menu.edit_redo)->child), text);

  gtk_widget_set_sensitive (main_toolbar.undo, undo != NULL);
  gtk_widget_set_sensitive (main_toolbar.redo, redo != NULL);
  gtk_widget_set_sensitive (main_menu.edit_undo, undo != NULL);
  gtk_widget_set_sensitive (main_menu.edit_redo, redo != NULL);
}

void SystemUpdateSnap(const unsigned long snap)
{
  ignore_commands = true;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(main_toolbar.angle), (snap & LC_DRAW_SNAP_A) != 0);
  ignore_commands = false;

  // TODO: popup menu
}

void SystemUpdateCurrentCamera(Camera* pOld, Camera* pNew, Camera* pCamera)
{
  int i;

  for (i = 0; pCamera; i++, pCamera = pCamera->m_pNext)
    if (pNew == pCamera)
      break;

  ignore_commands = true;
  gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM (main_menu.view_cameras[i]), TRUE);  
  ignore_commands = false;
}

void SystemUpdateCameraMenu(Camera* pCamera)
{
  Camera* pFirst = pCamera;
  gtk_menu_item_remove_submenu (GTK_MENU_ITEM (main_menu.view_cameras_popup));
  GtkWidget *menu_item = NULL, *menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (main_menu.view_cameras_popup), menu);

  int i;
  for (i = 0; pCamera; i++, pCamera = pCamera->m_pNext)
    if (i > 6)
    {
      GSList* grp = menu_item ? gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menu_item)) : NULL;
      menu_item = gtk_radio_menu_item_new_with_label (grp, pCamera->GetName());
      gtk_menu_append (GTK_MENU (menu), menu_item);
      gtk_widget_show (menu_item);
      main_menu.view_cameras[i] = menu_item;
      gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (OnCommand), (void*)(i + ID_CAMERA_FIRST));
    }

  if (i > 7)
  {
    GtkWidget* sep = gtk_menu_item_new ();
    gtk_menu_append (GTK_MENU (menu), sep);
    gtk_widget_set_sensitive (sep, FALSE);
    gtk_widget_show (sep);
  }

  for (pCamera = pFirst, i = 0; pCamera && (i < 7); i++, pCamera = pCamera->m_pNext)
  {
      GSList* grp = menu_item ? gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menu_item)) : NULL;
      menu_item = gtk_radio_menu_item_new_with_label (grp, pCamera->GetName());
      gtk_menu_append (GTK_MENU (menu), menu_item);
      gtk_widget_show (menu_item);
      main_menu.view_cameras[i] = menu_item;
      gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (OnCommand), (void*)(i + ID_CAMERA_FIRST));
  }
}

void SystemUpdateTime(bool bAnimation, int nTime, int nTotal)
{
  gtk_widget_set_sensitive (anim_toolbar.first, nTime != 1);
  gtk_widget_set_sensitive (anim_toolbar.prev, nTime > 1);
  gtk_widget_set_sensitive (anim_toolbar.next, nTime < nTotal);
  gtk_widget_set_sensitive (anim_toolbar.last, nTime != nTotal);
  gtk_widget_set_sensitive (main_menu.view_step_first, nTime != 1);
  gtk_widget_set_sensitive (main_menu.view_step_prev, nTime > 1);
  gtk_widget_set_sensitive (main_menu.view_step_next, nTime < nTotal);
  gtk_widget_set_sensitive (main_menu.view_step_last, nTime != nTotal);

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
  ignore_commands = true;
  gtk_widget_set_sensitive (anim_toolbar.play, bAnimation);
  gtk_widget_set_sensitive (anim_toolbar.stop, FALSE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(anim_toolbar.anim), bAnimation);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(anim_toolbar.keys), bAddKeys);
  gtk_label_set_text (GTK_LABEL(GTK_BIN(main_menu.piece_copy_keys)->child), 
      bAnimation ? "Copy Keys from Instructions" : "Copy Keys from Animation");
  ignore_commands = false;
}

void SystemUpdateMoveSnap(unsigned short move_snap)
{
  char text[11];
  if (move_snap)
    sprintf (text, "Move x%i", move_snap);
  else
    strcpy (text, "Move /2");

  gtk_label_set (GTK_LABEL (label_snap), text);
}

void SystemUpdateSelected(unsigned long flags)
{
  // select all/none/invert/by name (menu)
  gtk_widget_set_sensitive (main_menu.edit_select_all, (flags & LC_SEL_UNSELECTED) != 0);
  gtk_widget_set_sensitive (main_menu.edit_select_none, flags & 
			    (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT) != 0);
  gtk_widget_set_sensitive (main_menu.edit_select_invert, (flags & LC_SEL_NO_PIECES) == 0);
  gtk_widget_set_sensitive (main_menu.edit_select_byname, (flags & LC_SEL_NO_PIECES) == 0);

  // cut, copy (menu/toolbar)
  gtk_widget_set_sensitive (main_menu.edit_cut, (flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT)) != 0);
  gtk_widget_set_sensitive (main_menu.edit_copy, (flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT)) != 0);
  gtk_widget_set_sensitive (main_toolbar.cut, (flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT)) != 0);
  gtk_widget_set_sensitive (main_toolbar.copy, (flags & (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT)) != 0);

  // delete, array, hide sel/unsel, unhideall, copykeys (menu)
  gtk_widget_set_sensitive (main_menu.piece_delete, (flags & 
			    (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT)) != 0);
  gtk_widget_set_sensitive (main_menu.piece_array, (flags & LC_SEL_PIECE) != 0);
  gtk_widget_set_sensitive (main_menu.piece_hide_sel, (flags & LC_SEL_PIECE) != 0);
  gtk_widget_set_sensitive (main_menu.piece_hide_unsel, (flags & LC_SEL_UNSELECTED) != 0);
  gtk_widget_set_sensitive (main_menu.piece_unhide, (flags & LC_SEL_HIDDEN) != 0);
  gtk_widget_set_sensitive (main_menu.piece_copy_keys, (flags & 
			    (LC_SEL_PIECE|LC_SEL_CAMERA|LC_SEL_LIGHT)) != 0);

  // groups (menu)
  gtk_widget_set_sensitive (main_menu.piece_group, (flags & LC_SEL_CANGROUP) != 0);
  gtk_widget_set_sensitive (main_menu.piece_ungroup, (flags & LC_SEL_GROUP) != 0);
  gtk_widget_set_sensitive (main_menu.piece_group_add, (flags &
			    (LC_SEL_GROUP|LC_SEL_FOCUSGROUP)) == LC_SEL_GROUP);
  gtk_widget_set_sensitive (main_menu.piece_group_remove, (flags & LC_SEL_FOCUSGROUP) != 0);
  gtk_widget_set_sensitive (main_menu.piece_edit_groups, (flags & LC_SEL_NO_PIECES) == 0);

  gtk_widget_set_sensitive (tool_toolbar.prev, (flags & LC_SEL_PIECE) != 0);
  gtk_widget_set_sensitive (tool_toolbar.next, (flags & LC_SEL_PIECE) != 0);
}

void SystemUpdateRecentMenu(char names[4][LC_MAXPATH])
{
  if (strlen(names[0]) == 0)
  {
    gtk_label_set_text (GTK_LABEL(GTK_BIN(main_menu.file_recent[0])->child), "Recent Files");
    gtk_widget_set_sensitive (main_menu.file_recent[0], FALSE);
  }
  else
  {
    gtk_label_set_text (GTK_LABEL(GTK_BIN(main_menu.file_recent[0])->child), names[0]);
    gtk_widget_set_sensitive (main_menu.file_recent[0], TRUE);
  }

  for (int i = 1; i < 4; i++)
  {
    GtkWidget* menu_item = main_menu.file_recent[i];
    if (strlen(names[i]) == 0)
      gtk_widget_hide (menu_item);
    else
    {
      gtk_widget_show (menu_item);
      gtk_label_set_text (GTK_LABEL(GTK_BIN(menu_item)->child), names[i]);
    }
  }
}

void SystemUpdatePaste(bool enable)
{
  gtk_widget_set_sensitive (main_toolbar.paste, enable);
  gtk_widget_set_sensitive (main_menu.edit_paste, enable);
}

void SystemUpdatePlay(bool play, bool stop)
{
  gtk_widget_set_sensitive (anim_toolbar.play, play);
  gtk_widget_set_sensitive (anim_toolbar.stop, stop);
}

void SystemUpdateFocus(void* object, unsigned char type)
{
  // TODO: modify dialog
  char text[32];
  float pos[3];
  project->GetFocusPosition(pos);
  sprintf (text, "X: %.2f Y: %.2f Z: %.2f", pos[0], pos[1], pos[2]);
 
  gtk_label_set (GTK_LABEL (label_position), text);
}

// Memory render
typedef struct
{
  int width, height;
  GdkGLPixmap *glpixmap;
  GdkGLContext *context;
  GdkPixmap *pixmap;
} LC_RENDER;

void* SystemStartRender(int width, int height)
{
  GdkVisual *visual;
  LC_RENDER* render = (LC_RENDER*)malloc(sizeof(LC_RENDER));
  int attrlist[] = { GLX_RGBA, GLX_DEPTH_SIZE, 16, 0 };
  visual = gdk_gl_choose_visual(attrlist);

  render->width = width;
  render->height = height;
  render->context = gdk_gl_context_new(visual);
  render->pixmap = gdk_pixmap_new(NULL, width, height, visual->depth);
  render->glpixmap = gdk_gl_pixmap_new(visual, render->pixmap);
  gdk_gl_pixmap_make_current(render->glpixmap, render->context);

  return render;
}

void SystemFinishRender(void* param)
{
  LC_RENDER* render = (LC_RENDER*)param;

  gtk_gl_area_make_current(GTK_GL_AREA(drawing_area));
  gdk_gl_context_unref(render->context);
  gdk_gl_pixmap_unref(render->glpixmap);
  gdk_pixmap_unref(render->pixmap);
  free(render);
}

LC_IMAGE* SystemGetRenderImage(void* param)
{
  LC_RENDER* render = (LC_RENDER*)param;
  LC_IMAGE* image = (LC_IMAGE*)malloc(sizeof(LC_IMAGE)+
	      (render->width*render->height*3));

  image->width = render->width;
  image->height = render->height;
  image->bits = (char*)image + sizeof(LC_IMAGE);

  glFinish();

  int x, y;
  unsigned char* p = (unsigned char*)image->bits;

  GdkImage* gi = gdk_image_get(render->pixmap, 0, 0,
			       render->width, render->height);
  for (y = 0; y < render->height; y++)
  for (x = 0; x < render->width; x++)
  {
    guint32 ui = gdk_image_get_pixel (gi, x, y);
    *p = (ui & 0xFF0000) >> 16; p++;
    *p = (ui & 0x00FF00) >> 8; p++;
    *p = (ui & 0x0000FF); p++;
  }

  gdk_image_destroy(gi);

  return image;
}


void SystemInit()
{
}

void SystemFinish()
{
}

int SystemDoMessageBox(char* prompt, int mode)
{
  return msgbox_execute(prompt, mode);
}

bool SystemDoDialog(int mode, void* param)
{
  switch (mode)
  {
    case LC_DLG_FILE_OPEN: {
      return filedlg_execute("Open File", (char*)param) == LC_OK;
    } break;

    case LC_DLG_FILE_SAVE: {
      return filedlg_execute("Save File", (char*)param) == LC_OK;
    } break;

    case LC_DLG_FILE_MERGE: {
      return filedlg_execute("Merge File", (char*)param) == LC_OK;
    } break;

    case LC_DLG_ABOUT: {
      return aboutdlg_execute(param) == LC_OK;
    } break;

    case LC_DLG_ARRAY: {
      return arraydlg_execute(param) == LC_OK;
    } break;

    case LC_DLG_HTML: {
      return htmldlg_execute(param) == LC_OK;
    } break;

    case LC_DLG_POVRAY: {
      return povraydlg_execute(param) == LC_OK;
    } break;

    case LC_DLG_WAVEFRONT: {
      return filedlg_execute("Save File", (char*)param) == LC_OK;
    } break;

    case LC_DLG_PREFERENCES: {
      return preferencesdlg_execute(param) == LC_OK;
    } break;

    case LC_DLG_PICTURE_SAVE: {
    } break;

    case LC_DLG_MINIFIG: {
      return minifigdlg_execute(param) == LC_OK;
    } break;

    case LC_DLG_PROPERTIES: {
      return propertiesdlg_execute(param) == LC_OK;
    } break;

    case LC_DLG_SELECTBYNAME: {
    } break;

    case LC_DLG_STEPCHOOSE: {
    } break;

    case LC_DLG_EDITGROUPS: {
      return groupeditdlg_execute(param) == LC_OK;
    } break;

    case LC_DLG_GROUP: {
      return groupdlg_execute(param) == LC_OK;
    } break;
  }

  return false;
}

void SystemDoPopupMenu(int nMenu, int x, int y)
{
}

void SystemDoWaitCursor(int code)
{
  if (code == 1)
  {
    GdkCursor *cursor = gdk_cursor_new (GDK_WATCH);
    gdk_window_set_cursor (main_window->window, cursor);
    gdk_cursor_destroy (cursor);
  } 
  else
  {
    GdkCursor *cursor = gdk_cursor_new (GDK_LEFT_PTR);
    gdk_window_set_cursor (main_window->window, cursor);
    gdk_cursor_destroy (cursor);
  }
}

void SystemExportClipboard(File* clip)
{
}

File* SystemImportClipboard()
{
  return NULL;
}

void SystemSetWindowCaption(char* caption)
{
  gtk_window_set_title (GTK_WINDOW (main_window), caption);
}

void SystemRedrawView()
{
  gtk_widget_draw(drawing_area, NULL);
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

void SystemSwapBuffers()
{
  if (drawing_area)
    gtk_gl_area_swapbuffers (GTK_GL_AREA(drawing_area));
}
