
#ifndef _GTKTOOLS_H_
#define _GTKTOOLS_H_

GtkWidget* new_pixmap(GtkWidget* widget, const char** data);
GtkWidget* clist_title_with_arrow(GtkWidget* clist, char col, const char* label_text);
void set_notebook_tab (GtkWidget *notebook, gint page_num, GtkWidget *widget);
void set_button_pixmap (GtkWidget* widget, float* color);
void set_button_pixmap2 (GtkWidget* widget, unsigned char* color);

#endif // _GTKTOOLS_H_

