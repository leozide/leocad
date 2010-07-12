//
// This file holds all the dialogs that are called
// from the 'File' submenu:
//
// - File Open Dialog
// - File Save Dialog
// - Save Picture Dialog
// - Piece Library Manager
//

#include <gtk/gtk.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "system.h"
#include "dialogs.h"
#include "file.h"
#include "image.h"
#include "main.h"

// =============================================================================
// Open Project Dialog

static void openprojectdlg_select (GtkCList *clist, gint row, gint col, GdkEvent *event, GtkPreview *preview)
{
  GtkWidget *parent = gtk_widget_get_toplevel (GTK_WIDGET (clist));
  const char *filename, *p;
  bool loaded = false;
  Image image;

  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (parent));

  p = strrchr (filename, '.');
  if ((p != NULL) && (g_strcasecmp (p+1, "lcd") == 0))
  {
    float fv;
    char id[32];
    FileDisk file;
    file.Open (filename, "rb");
    file.Read (id, 32);
    sscanf (strchr(id, ' '), "%f", &fv);

    if (fv > 0.4f)
    {
      file.Read(&fv, 4);

      if (fv > 0.7f)
      {
        unsigned long dwPosition;
        file.Seek (-4, SEEK_END);
        file.Read (&dwPosition, 4);
        file.Seek (dwPosition, SEEK_SET);

        if (dwPosition != 0)
        {
          if (fv < 1.0f)
          {
            file.Seek (54, SEEK_CUR);

            image.Allocate (120, 100, false);
            file.Read (image.GetData (), 36000);

            for (int y = 0; y < 50; y++)
              for (int x = 0; x < 120; x++)
              {
                unsigned char *from = image.GetData() + x*3 + y*360;
                unsigned char *to = image.GetData() + x*3 + (100-y-1)*360;
                unsigned char tmp[3] = { from[0], from[1], from[2] };

                from[0] = to[2];
                from[1] = to[1];
                from[2] = to[0];
                to[0] = tmp[2];
                to[1] = tmp[1];
                to[2] = tmp[0];
              }
            loaded = true;
          }
          else
          {
            loaded = image.FileLoad (file);
          }
        }
      }
    }
    file.Close();
  }

  if (loaded == false)
  {
    GtkWidget *w = GTK_WIDGET (preview);
    guchar row[360];

    for (int x = 0; x < 120; x++)
    {
      row[x*3] = w->style->bg[0].red/0xFF;
      row[x*3+1] = w->style->bg[0].green/0xFF;
      row[x*3+2] = w->style->bg[0].blue/0xFF;
    }

    for (int y = 0; y < 100; y++)
      gtk_preview_draw_row (preview, row, 0, y, 120);
    gtk_widget_draw (w, NULL);
  }
  else
  {
    for (int y = 0; y < 100; y++)
      gtk_preview_draw_row (preview, image.GetData ()+y*360, 0, y, 120);
    gtk_widget_draw (GTK_WIDGET (preview), NULL);
  }
}

int openprojectdlg_execute (char* filename)
{
  GtkWidget *dlg, *preview, *vbox, *frame, *frame2, *hbox;
  int ret = LC_CANCEL, loop = 1, len;

  dlg = gtk_file_selection_new ("Open Project");
  gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (((GtkWidget*)(*main_window))));
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
                      GTK_SIGNAL_FUNC (dialog_delete_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (dlg)->ok_button), "clicked",
                      GTK_SIGNAL_FUNC (dialog_button_callback), GINT_TO_POINTER (LC_OK));
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (dlg)->cancel_button), "clicked",
                      GTK_SIGNAL_FUNC (dialog_button_callback), GINT_TO_POINTER (LC_CANCEL));
  gtk_object_set_data (GTK_OBJECT (dlg), "loop", &loop);
  gtk_object_set_data (GTK_OBJECT (dlg), "ret", &ret);

  // add preview support
  hbox = GTK_FILE_SELECTION (dlg)->file_list->parent->parent;

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, TRUE, 0);

  frame = gtk_frame_new ("Preview");
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, FALSE, 0);

  frame2 = gtk_frame_new (NULL);
  gtk_widget_show (frame2);
  gtk_container_add (GTK_CONTAINER (frame), frame2);
  gtk_container_border_width (GTK_CONTAINER (frame2), 5);
  gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_NONE);

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_container_add (GTK_CONTAINER (frame2), preview);
  gtk_preview_size (GTK_PREVIEW (preview), 120, 100);
  gtk_widget_show (preview);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (dlg)->file_list), "select_row",
                      GTK_SIGNAL_FUNC (openprojectdlg_select), preview);

  len = strlen (filename);
  if (len != 0)
  {
    if (filename[len-1] != '/')
      strcat (filename, "/");
    gtk_file_selection_set_filename (GTK_FILE_SELECTION (dlg), filename);
  }

  gtk_widget_show (dlg);
  gtk_grab_add (dlg);

  while (loop)
    gtk_main_iteration ();

  if (ret == LC_OK)
    strcpy (filename, gtk_file_selection_get_filename (GTK_FILE_SELECTION (dlg)));

  gtk_grab_remove (dlg);
  gtk_widget_destroy (dlg);

  return ret;
}

// =============================================================================
// Save Project Dialog

static void saveprojectdlg_preview (GtkToggleButton *button, gpointer data)
{
  Sys_ProfileSaveInt ("Default", "Save Preview", gtk_toggle_button_get_active (button));
}

// used by the save project and save picture dialogs
static void savefiledlg_ok (GtkWidget *widget, gpointer data)
{
  GtkWidget *parent;
  int *loop, *ret;

  parent = gtk_widget_get_toplevel (widget);
  loop = (int*)gtk_object_get_data (GTK_OBJECT (parent), "loop");
  ret = (int*)gtk_object_get_data (GTK_OBJECT (parent), "ret");

  if ((GPOINTER_TO_INT (data) == LC_OK) &&
      (access (gtk_file_selection_get_filename (GTK_FILE_SELECTION (parent)), R_OK) == 0))
    if (Sys_MessageBox ("File already exists, overwrite ?", "LeoCAD", LC_MB_YESNO) == LC_NO)
      return;

  *loop = 0;
  *ret = GPOINTER_TO_INT (data);
}

int saveprojectdlg_execute (char* filename)
{
  GtkWidget *dlg, *check;
  int ret = LC_CANCEL, loop = 1;

  dlg = gtk_file_selection_new ("Save Project");
  gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (((GtkWidget*)(*main_window))));
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
                      GTK_SIGNAL_FUNC (dialog_delete_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (dlg)->ok_button), "clicked",
                      GTK_SIGNAL_FUNC (savefiledlg_ok), GINT_TO_POINTER (LC_OK));
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (dlg)->cancel_button), "clicked",
                      GTK_SIGNAL_FUNC (dialog_button_callback), GINT_TO_POINTER (LC_CANCEL));
  gtk_object_set_data (GTK_OBJECT (dlg), "loop", &loop);
  gtk_object_set_data (GTK_OBJECT (dlg), "ret", &ret);

  // add preview checkbox
  check = gtk_check_button_new_with_label ("Save Preview");
  gtk_widget_show (check);
  gtk_box_pack_start (GTK_BOX (GTK_FILE_SELECTION (dlg)->main_vbox), check, FALSE, FALSE, 0);

  int i = Sys_ProfileLoadInt ("Default", "Save Preview", 0);
  if (i != 0) 
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), TRUE);
  gtk_signal_connect (GTK_OBJECT (check), "toggled", GTK_SIGNAL_FUNC (saveprojectdlg_preview), NULL);

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (dlg), filename);

  gtk_widget_show (dlg);
  gtk_grab_add (dlg);

  while (loop)
    gtk_main_iteration ();

  if (ret == LC_OK)
    strcpy (filename, gtk_file_selection_get_filename (GTK_FILE_SELECTION (dlg)));

  gtk_grab_remove (dlg);
  gtk_widget_destroy (dlg);

  return ret;
}

// =============================================================================
// Save Picture Dialog

static void savepicturedlg_options (GtkWidget *widget, gpointer data)
{
  imageoptsdlg_execute (data, false);
}

int savepicturedlg_execute (void* param)
{
  GtkWidget *dlg, *button;
  int ret = LC_CANCEL, loop = 1;

  unsigned long image = Sys_ProfileLoadInt ("Default", "Image Options", 1|LC_IMAGE_TRANSPARENT);
  LC_IMAGEDLG_OPTS* opts = (LC_IMAGEDLG_OPTS*)param;
  opts->width = Sys_ProfileLoadInt ("Default", "Image Width", gdk_screen_width ());
  opts->height = Sys_ProfileLoadInt ("Default", "Image Height", gdk_screen_height ());
  opts->imopts.quality = Sys_ProfileLoadInt ("Default", "JPEG Quality", 70);
  opts->imopts.interlaced = (image & LC_IMAGE_PROGRESSIVE) != 0;
  opts->imopts.transparent = (image & LC_IMAGE_TRANSPARENT) != 0;
  opts->imopts.truecolor = (image & LC_IMAGE_HIGHCOLOR) != 0;
  opts->imopts.pause = (float)Sys_ProfileLoadInt ("Default", "AVI Pause", 100)/100;
  opts->imopts.format = (unsigned char)(image & ~(LC_IMAGE_MASK));

  dlg = gtk_file_selection_new ("Save Picture");
  gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (((GtkWidget*)(*main_window))));
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
                      GTK_SIGNAL_FUNC (dialog_delete_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (dlg)->ok_button), "clicked",
                      GTK_SIGNAL_FUNC (savefiledlg_ok), GINT_TO_POINTER (LC_OK));
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (dlg)->cancel_button), "clicked",
                      GTK_SIGNAL_FUNC (dialog_button_callback), GINT_TO_POINTER (LC_CANCEL));
  gtk_object_set_data (GTK_OBJECT (dlg), "loop", &loop);
  gtk_object_set_data (GTK_OBJECT (dlg), "ret", &ret);

  // add the options button
  button = gtk_button_new_with_label ("Options");
  gtk_widget_show (button);
  gtk_box_pack_end (GTK_BOX (GTK_FILE_SELECTION (dlg)->ok_button->parent), button, TRUE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (savepicturedlg_options), param);

  /*
  gtk_file_selection_set_filename (GTK_FILE_SELECTION (dlg), filename);
  */
  gtk_widget_show (dlg);
  gtk_grab_add (dlg);

  while (loop)
    gtk_main_iteration ();

  if (ret == LC_OK)
  {
    char ext[5], *p;

    strcpy (opts->filename, gtk_file_selection_get_filename (GTK_FILE_SELECTION (dlg)));

    if (strlen (opts->filename) == 0)
      ret = LC_CANCEL;

    p = strrchr (opts->filename, '.');
    if (p != NULL)
    {
      strcpy (ext, p+1);
      strlwr (ext);
    }
    else
      ext[0] = '\0';

    if ((strcmp (ext, "jpg") != 0) && (strcmp (ext, "jpeg") != 0) &&
        (strcmp (ext, "bmp") != 0) && (strcmp (ext, "gif") != 0) &&
        (strcmp (ext, "png") != 0) && (strcmp (ext, "avi") != 0))
    {
      switch (opts->imopts.format)
      {
        case LC_IMAGE_BMP: strcat(opts->filename, ".bmp"); break;
        case LC_IMAGE_GIF: strcat(opts->filename, ".gif"); break;
        case LC_IMAGE_JPG: strcat(opts->filename, ".jpg"); break;
        case LC_IMAGE_PNG: strcat(opts->filename, ".png"); break;
        case LC_IMAGE_AVI: strcat(opts->filename, ".avi"); break;
      }
    }
  }

  gtk_grab_remove (dlg);
  gtk_widget_destroy (dlg);

  return ret;
}

// =============================================================================
// Piece Library Manager

