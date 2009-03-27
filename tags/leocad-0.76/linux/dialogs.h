#ifndef _DIALOGS_H_
#define _DIALOGS_H_

// Dialog support functions
void dialog_button_callback (GtkWidget *widget, gpointer data);
gint dialog_delete_callback (GtkWidget *widget, GdkEvent* event, gpointer data);

// obsolete
int dlg_domodal (GtkWidget* dlg, int def);
void dlg_end (int ret);
gint dlg_delete_callback (GtkWidget *widget, GdkEvent* event, gpointer data);
void dlg_default_callback(GtkWidget *widget, gpointer data);

void modifydlg_toggle ();

// All dialogs
int openprojectdlg_execute (char* filename);
int saveprojectdlg_execute (char* filename);
int savepicturedlg_execute (void* param);
int msgbox_execute (const char* text, const char *caption, int flags);
int filedlg_execute(const char* caption, char* filename);
int arraydlg_execute(void* param);
int aboutdlg_execute(void* param);
int htmldlg_execute(void* param);
int imageoptsdlg_execute(void* param, bool from_htmldlg);
int povraydlg_execute(void* param);
int preferencesdlg_execute(void* param);
int propertiesdlg_execute(void* param);
int groupeditdlg_execute(void* param);
int groupdlg_execute(void* param);
int minifigdlg_execute(void* param);
int librarydlg_execute(void* param);

#endif // _DIALOGS_H_






