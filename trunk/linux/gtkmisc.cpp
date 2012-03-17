//
// Small functions to help with GTK
//

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <string.h>
#include "gtkmisc.h"
#include "globals.h"
#include "project.h"
//#include "pixmenu.h"
#include "gtktools.h"

// =============================================================================
// Pixmap functions

// Load a pixmap file from the disk
void load_pixmap (const char* filename, GdkPixmap **gdkpixmap, GdkBitmap **mask)
{
  *gdkpixmap = NULL;

  if (*gdkpixmap == NULL)
  {
    const char *dummy[] = { "1 1 1 1", "  c None", " " };
    *gdkpixmap = gdk_pixmap_create_from_xpm_d (GDK_ROOT_PARENT(), mask, NULL, (gchar**)dummy);
  }
}

// Load a xpm file and return a pixmap widget
GtkWidget* create_pixmap (const char* filename)
{
  GdkPixmap *gdkpixmap;
  GdkBitmap *mask;
  GtkWidget *pixmap;
 
  load_pixmap (filename, &gdkpixmap, &mask);
  pixmap = gtk_pixmap_new (gdkpixmap, mask);
  gtk_widget_show (pixmap);
 
  gdk_pixmap_unref (gdkpixmap);
  gdk_pixmap_unref (mask);
 
  return pixmap;
}

// =============================================================================
// Menu stuff

GtkWidget* menu_separator (GtkWidget *menu)
{
  GtkWidget *menu_item = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menu_item);
  gtk_widget_set_sensitive (menu_item, FALSE);
  gtk_widget_show (menu_item);
  return menu_item;
}

GtkWidget* menu_tearoff (GtkWidget *menu)
{
  GtkWidget *menu_item = gtk_tearoff_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menu_item);
  //  gtk_widget_set_sensitive (menu_item, FALSE);
  gtk_widget_show (menu_item);
  return menu_item;
}
 
GtkWidget* create_sub_menu(GtkWidget* bar, const char* label, GtkAccelGroup* accel)
{
  GtkWidget *item, *menu;

  item = gtk_menu_item_new_with_mnemonic(label);
  gtk_widget_show (item);
  gtk_container_add (GTK_CONTAINER (bar), item);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);

  return menu;
}

GtkWidget* create_menu_in_menu(GtkWidget* menu, const char* label, GtkAccelGroup* accel)
{
  GtkWidget *item, *submenu;

  item = gtk_menu_item_new_with_mnemonic(label);
  gtk_widget_show (item);
  gtk_container_add (GTK_CONTAINER (menu), item);

  submenu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);

  return submenu;
}

GtkWidget* create_menu_item(GtkWidget *menu, const char *label, GtkAccelGroup *menu_accel,
			     GtkSignalFunc func, GtkObject *window, int id, const char* data)
{
  GtkWidget *item;

  item = gtk_menu_item_new_with_mnemonic (label);
  gtk_widget_show (item);
  gtk_container_add (GTK_CONTAINER (menu), item);
  gtk_signal_connect (GTK_OBJECT (item), "activate", GTK_SIGNAL_FUNC (func), GINT_TO_POINTER (id));

  if (data != NULL)
    gtk_object_set_data (window, data, item);

  return item;
}

GtkWidget* create_pixmap_menu_item(GtkWidget *menu, const gchar *label, const char **pixmap, GtkAccelGroup *menu_accel,
                                    GtkSignalFunc func, GtkObject *window, int id, const char* data)
{
  GtkWidget *item, *pixmap_widget;

  item = gtk_image_menu_item_new_with_mnemonic(label);

  pixmap_widget = new_pixmap (GTK_WIDGET (window), pixmap);
  gtk_widget_show (pixmap_widget);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), pixmap_widget);

  gtk_widget_show (item);
  gtk_container_add (GTK_CONTAINER (menu), item);
  gtk_signal_connect (GTK_OBJECT (item), "activate", GTK_SIGNAL_FUNC (func), GINT_TO_POINTER (id));

  if (data != NULL)
    gtk_object_set_data (window, data, item);

  return item;
}

GtkWidget* create_check_menu_item(GtkWidget *menu, const char *label, GtkAccelGroup *menu_accel,
				   GtkSignalFunc func, GtkObject *window, int id, const char* data)
{
  GtkWidget *item;

  item = gtk_check_menu_item_new_with_mnemonic(label);
  gtk_widget_show (item);
  gtk_container_add (GTK_CONTAINER (menu), item);
  gtk_signal_connect (GTK_OBJECT (item), "activate", GTK_SIGNAL_FUNC (func), GINT_TO_POINTER (id));

  if (data != NULL)
    gtk_object_set_data (window, data, item);

  return item;
}

GtkWidget* create_radio_menu_item(GtkWidget *menu, GtkWidget *last, const char *label,
				   GtkAccelGroup *menu_accel, GtkSignalFunc func,
				   GtkObject *window, int id, const char* data)
{
  GtkWidget *item;
  GSList *group = NULL;

  if (last != NULL)
    group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (last));
  item = gtk_radio_menu_item_new_with_mnemonic(group, label);
  gtk_widget_show (item);
  gtk_container_add (GTK_CONTAINER (menu), item);
  gtk_signal_connect (GTK_OBJECT (item), "activate", GTK_SIGNAL_FUNC (func), GINT_TO_POINTER (id));

  if (data != NULL)
    gtk_object_set_data (window, data, item);

  return item;
}

GtkWidget* create_radio_menu_pixmap(GtkWidget *menu, GtkWidget *last, const char *filename,
				     GtkAccelGroup *menu_accel, GtkSignalFunc func,
				     GtkObject *window, int id, const char* data)
{
  GtkWidget *item, *pixmap;
  GSList *group = NULL;

  if (last != NULL)
    group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (last));

  item = gtk_radio_menu_item_new (group);
  gtk_widget_show (item);
  gtk_container_add (GTK_CONTAINER (menu), item);
  gtk_signal_connect (GTK_OBJECT (item), "activate", GTK_SIGNAL_FUNC (func), GINT_TO_POINTER (id));

  pixmap = create_pixmap (filename);
  gtk_container_add (GTK_CONTAINER (item), pixmap);

  if (data != NULL)
    gtk_object_set_data (window, data, item);

  return item;
}
