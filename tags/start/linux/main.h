#ifndef _MAIN_H_
#define _MAIN_H_

extern GtkWidget* main_window;
extern GtkWidget* drawing_area;

extern bool ignore_commands;
void OnCommand(GtkWidget *w, gpointer data);
void OnCommandDirect(GtkWidget *w, gpointer data);

#define ID_FILE_RECENT1           1
#define ID_FILE_RECENT2           2
#define ID_FILE_RECENT3           3
#define ID_FILE_RECENT4           4
#define ID_FILE_EXIT              5
#define ID_VIEW_VIEWPORTS_01      6
#define ID_VIEW_VIEWPORTS_14     19

#define ID_CAMERA_FIRST        1001
#define ID_CAMERA_LAST         1255


#define ID_ACTION_SELECT       100
#define ID_ACTION_INSERT       101
#define ID_ACTION_LIGHT        102
#define ID_ACTION_SPOTLIGHT    103
#define ID_ACTION_CAMERA       104
#define ID_ACTION_MOVE         105
#define ID_ACTION_ROTATE       106
#define ID_ACTION_ERASER       107
#define ID_ACTION_PAINT        108
#define ID_ACTION_ZOOM         109
#define ID_ACTION_ZOOM_REGION  110
#define ID_ACTION_PAN          111
#define ID_ACTION_ROTATE_VIEW  112
#define ID_ACTION_ROLL         113

#define ID_SNAP_A              130
#define ID_SNAP_X              131
#define ID_SNAP_Y              132
#define ID_SNAP_Z              133

#endif



