// Helper functions for GTK
//

#include <gtk/gtk.h>
#include "gtktools.h"

GtkWidget* new_pixmap(GtkWidget *widget, const char **data)
{
  GdkPixmap *gdkpixmap;
  GdkBitmap *mask;
  GtkWidget *pixmap;

  gdkpixmap = gdk_pixmap_create_from_xpm_d(widget->window, &mask, &widget->style->bg[GTK_STATE_NORMAL], (gchar**)data);
  pixmap = gtk_pixmap_new (gdkpixmap, mask);

  gdk_pixmap_unref (gdkpixmap);
  gdk_pixmap_unref (mask);

  return pixmap;
}

GtkWidget* clist_title_with_arrow(GtkWidget* clist, char col, const char* label_text)
{
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_IN);
  GtkWidget *label = gtk_label_new (label_text);

  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (hbox), arrow, FALSE, TRUE, 0);
  gtk_widget_show (label);

  gtk_widget_show (hbox);
  gtk_clist_set_column_widget (GTK_CLIST (clist), col, hbox);

  return arrow;
}

void set_notebook_tab (GtkWidget *notebook, gint page_num, GtkWidget *widget)
{
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page_num), widget);
  /*
  GtkNotebookPage *page;
  GtkWidget *notebook_page;

  page = (GtkNotebookPage*) g_list_nth (GTK_NOTEBOOK (notebook)->children, page_num)->data;
  notebook_page = page->child;
  gtk_widget_ref (notebook_page);
  gtk_notebook_remove_page (GTK_NOTEBOOK (notebook), page_num);
  gtk_notebook_insert_page (GTK_NOTEBOOK (notebook), notebook_page,
                            widget, page_num);
  gtk_widget_unref (notebook_page);
  */
}

void set_button_pixmap (GtkWidget* widget, float* color)
{
  if (widget->window == NULL)
    return;

  GdkColor c;
  GdkGC* gc = gdk_gc_new(widget->window);
  GdkPixmap* pixmap = gdk_pixmap_new(widget->window, widget->allocation.width - 20,
				     widget->allocation.height - 20, -1);

  c.red = (gushort)(color[0]*0xFFFF);
  c.green = (gushort)(color[1]*0xFFFF);
  c.blue = (gushort)(color[2]*0xFFFF);
  gdk_color_alloc (gtk_widget_get_colormap(widget), &c);
  gdk_gc_set_foreground(gc, &c);

  gdk_draw_rectangle (pixmap, gc, TRUE, 0, 0,
		      widget->allocation.width - 20, widget->allocation.height - 20);

  GtkWidget* pixmapwid = gtk_pixmap_new (pixmap, (GdkBitmap*)NULL);
  gtk_widget_show (pixmapwid);

  gtk_container_remove (GTK_CONTAINER(widget), GTK_BIN(widget)->child);
  gtk_container_add (GTK_CONTAINER(widget), pixmapwid);
  gdk_gc_destroy(gc);
}

void set_button_pixmap2(GtkWidget* widget, float* color)
{
  GdkColor c;
  GdkGC* gc;
  GdkPixmap* pixmap;

  if (widget->window == NULL)
    return;

  if ((widget->allocation.width < 10) || (widget->allocation.height < 10))
    return;

  gc = gdk_gc_new (widget->window);
  pixmap = gdk_pixmap_new (widget->window, widget->allocation.width - 10,
			   widget->allocation.height - 10, -1);

  c.red = (gushort)(color[0]*0xFFFF);
  c.green = (gushort)(color[1]*0xFFFF);
  c.blue = (gushort)(color[2]*0xFFFF);
  gdk_color_alloc (gtk_widget_get_colormap(widget), &c);
  gdk_gc_set_foreground(gc, &c);

  gdk_draw_rectangle (pixmap, gc, TRUE, 0, 0,
		      widget->allocation.width - 5, widget->allocation.height - 5);

  GtkWidget* pixmapwid = gtk_pixmap_new (pixmap, (GdkBitmap*)NULL);
  gtk_widget_show (pixmapwid);

  gtk_container_remove (GTK_CONTAINER(widget), GTK_BIN(widget)->child);
  gtk_container_add (GTK_CONTAINER(widget), pixmapwid);
  gdk_gc_destroy(gc);
}

