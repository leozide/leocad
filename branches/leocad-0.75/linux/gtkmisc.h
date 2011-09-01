#ifndef _GTK_MISC_H_
#define _GTK_MISC_H_

GtkWidget* create_pixmap (char* filename);
void load_pixmap (const char* filename, GdkPixmap **gdkpixmap, GdkBitmap **mask);

GtkWidget* menu_separator (GtkWidget *menu);
GtkWidget* menu_tearoff (GtkWidget *menu);
GtkWidget* create_sub_menu(GtkWidget *bar, const char *label, GtkAccelGroup *accel);
GtkWidget* create_menu_in_menu(GtkWidget *menu, const char *label, GtkAccelGroup *accel);
GtkWidget* create_menu_item(GtkWidget *menu, const char *label, GtkAccelGroup *accel,
			     GtkSignalFunc func, GtkObject *window, int id, const char* data);
GtkWidget* create_pixmap_menu_item(GtkWidget *menu, const char *label, const char **pixmap, GtkAccelGroup *accel,
                                    GtkSignalFunc func, GtkObject *window, int id, const char* data);
GtkWidget* create_check_menu_item(GtkWidget *menu, const char *label, GtkAccelGroup *menu_accel,
				   GtkSignalFunc func, GtkObject *window, int id, const char* data);
GtkWidget* create_radio_menu_item(GtkWidget *menu, GtkWidget *last, const char *label,
				   GtkAccelGroup *menu_accel, GtkSignalFunc func,
				   GtkObject *window, int id, const char* data);
GtkWidget* create_radio_menu_pixmap(GtkWidget *menu, GtkWidget *last, const char *filename,
				     GtkAccelGroup *menu_accel, GtkSignalFunc func,
				     GtkObject *window, int id, const char* data);


#endif // _GTKMISC_H_
