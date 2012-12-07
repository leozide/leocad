#include "lc_global.h"
#include <gtk/gtk.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "system.h"
#include "dialogs.h"
#include "lc_file.h"
#include "image.h"
#include "main.h"

// =============================================================================
// Open Project Dialog

static void openprojectdlg_preview(GtkFileChooser* dlg, GtkPreview* preview)
{
	char *filename, *p;
	bool loaded = false;
	Image image;

	filename = gtk_file_chooser_get_preview_filename(dlg);

	p = strrchr (filename, '.');
	if ((p != NULL) && (g_strcasecmp (p+1, "lcd") == 0))
	{
		float fv;
		char id[32];
		lcDiskFile file;
		file.Open (filename, "rb");
		file.ReadBuffer (id, 32);
		sscanf (strchr(id, ' '), "%f", &fv);

		if (fv > 0.4f)
		{
			file.ReadFloats(&fv, 1);

			if (fv > 0.7f)
			{
				lcuint32 dwPosition;
				file.Seek (-4, SEEK_END);
				file.ReadU32 (&dwPosition, 1);
				file.Seek (dwPosition, SEEK_SET);

				if (dwPosition != 0)
				{
					if (fv < 1.0f)
					{
						file.Seek (54, SEEK_CUR);

						image.Allocate (120, 100, false);
						file.ReadBuffer (image.GetData (), 36000);

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

	g_free(filename);

	if (loaded == false)
	{
		GtkWidget *w = GTK_WIDGET(preview);
		guchar row[360];

		for (int x = 0; x < 120; x++)
		{
			row[x*3] = w->style->bg[0].red/0xFF;
			row[x*3+1] = w->style->bg[0].green/0xFF;
			row[x*3+2] = w->style->bg[0].blue/0xFF;
		}

		for (int y = 0; y < 100; y++)
			gtk_preview_draw_row(preview, row, 0, y, 120);
		gtk_widget_draw(w, NULL);
	}
	else
	{
		for (int y = 0; y < 100; y++)
			gtk_preview_draw_row (preview, image.GetData ()+y*360, 0, y, 120);
		gtk_widget_draw (GTK_WIDGET (preview), NULL);
	}

	gtk_file_chooser_set_preview_widget_active(dlg, loaded);
}

int openprojectdlg_execute (char* filename)
{
	GtkWidget* dlg;
	int ret;

	dlg = gtk_file_chooser_dialog_new("Open Project", GTK_WINDOW(((GtkWidget*)*main_window)), GTK_FILE_CHOOSER_ACTION_OPEN,
	                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), filename);

	GtkFileFilter* filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.lcd");
	gtk_file_filter_set_name(filter, "LeoCAD Files");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filter);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dlg), filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.dat");
	gtk_file_filter_add_pattern(filter, "*.ldr");
	gtk_file_filter_set_name(filter, "LDraw Files");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_filter_set_name(filter, "All Files");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filter);

	GtkWidget* preview = gtk_preview_new(GTK_PREVIEW_COLOR);
	gtk_preview_size(GTK_PREVIEW(preview), 120, 100);
	gtk_widget_show(preview);
	gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER(dlg), preview);
	g_signal_connect(dlg, "update-preview", G_CALLBACK(openprojectdlg_preview), preview);

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
	{
		char* dlgfilename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
		strcpy(filename, dlgfilename);
		g_free(dlgfilename);

		ret = LC_OK;
	}
	else
		ret = LC_CANCEL;

	gtk_widget_destroy(dlg);

	return ret;
}

// =============================================================================
// Save Project Dialog

int saveprojectdlg_execute(char* filename)
{
	GtkWidget* dlg;
	int ret;

	dlg = gtk_file_chooser_dialog_new("Save Project", GTK_WINDOW(((GtkWidget*)*main_window)), GTK_FILE_CHOOSER_ACTION_SAVE,
	                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), TRUE);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), filename);

	GtkFileFilter* filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.lcd");
	gtk_file_filter_set_name(filter, "LeoCAD Files");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filter);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dlg), filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.dat");
	gtk_file_filter_add_pattern(filter, "*.ldr");
	gtk_file_filter_set_name(filter, "LDraw Files");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_filter_set_name(filter, "All Files");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filter);

	GtkWidget* check = gtk_check_button_new_with_label("Save Preview");
	gtk_widget_show(check);
	gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dlg), check);

	if (Sys_ProfileLoadInt("Default", "Save Preview", 0))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), TRUE);

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
	{
		Sys_ProfileSaveInt("Default", "Save Preview", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check)));

		char* dlgfilename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
		strcpy(filename, dlgfilename);
		g_free(dlgfilename);

		ret = LC_OK;
	}
	else
		ret = LC_CANCEL;

	gtk_widget_destroy(dlg);

	return ret;
}

// =============================================================================
// Save Picture Dialog

static void savepicturedlg_options(GtkWidget *widget, gpointer data)
{
	imageoptsdlg_execute(gtk_widget_get_toplevel(widget), data, false);
}

int savepicturedlg_execute(void* param)
{
	GtkWidget* dlg;
	int ret;

	dlg = gtk_file_chooser_dialog_new("Save Picture", GTK_WINDOW(((GtkWidget*)*main_window)), GTK_FILE_CHOOSER_ACTION_SAVE,
	                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), TRUE);
//	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), filename);

	GtkWidget* hbox = gtk_hbox_new(FALSE, 10);
	gtk_widget_show(hbox);
	gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dlg), hbox);

	GtkWidget* button = gtk_button_new_with_label ("Image Options");
	gtk_widget_show(button);
//	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(savepicturedlg_options), param);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

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

	if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_OK)
	{
		char* dlgfilename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
		strcpy(opts->filename, dlgfilename);
		g_free(dlgfilename);

		char ext[5], *p;

		if (strlen (opts->filename) == 0)
			ret = LC_CANCEL;

		p = strrchr(opts->filename, '.');
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

		ret = LC_OK;
	}
	else
		ret = LC_CANCEL;

	gtk_widget_destroy(dlg);

	return ret;
}
