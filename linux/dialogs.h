#ifndef _DIALOGS_H_
#define _DIALOGS_H_

int msgbox_execute(char* text, int mode);
int filedlg_execute(char* caption, char* filename);
int arraydlg_execute(void* param);
int aboutdlg_execute(void* param);
int htmldlg_execute(void* param);
int imageoptsdlg_execute(void* param, bool from_htmldlg);
int povraydlg_execute(void* param);
int preferencesdlg_execute(void* param);
int propertiesdlg_execute(void* param);
int groupeditdlg_execute(void* param);
int groupdlg_execute(void* param);

#endif // _DIALOGS_H_






