#ifndef _MAIN_H_
#define _MAIN_H_

extern GtkWidget* main_window;
extern GtkWidget* drawing_area;

extern bool ignore_commands;
void OnCommand(GtkWidget *w, gpointer data);
void OnCommandDirect(GtkWidget *w, gpointer data);

#define ID_FILE_RECENT1            1
#define ID_FILE_RECENT2            2
#define ID_FILE_RECENT3            3
#define ID_FILE_RECENT4            4
#define ID_FILE_EXIT               5
#define ID_VIEW_VIEWPORTS_01       6
#define ID_VIEW_VIEWPORTS_02       7
#define ID_VIEW_VIEWPORTS_03       8
#define ID_VIEW_VIEWPORTS_04       9
#define ID_VIEW_VIEWPORTS_05      10
#define ID_VIEW_VIEWPORTS_06      11
#define ID_VIEW_VIEWPORTS_07      12
#define ID_VIEW_VIEWPORTS_08      13
#define ID_VIEW_VIEWPORTS_09      14
#define ID_VIEW_VIEWPORTS_10      15
#define ID_VIEW_VIEWPORTS_11      16
#define ID_VIEW_VIEWPORTS_12      17
#define ID_VIEW_VIEWPORTS_13      18
#define ID_VIEW_VIEWPORTS_14      19
#define ID_VIEW_TOOLBAR_STANDARD  20
#define ID_VIEW_TOOLBAR_DRAWING   21
#define ID_VIEW_TOOLBAR_ANIMATION 22
#define ID_VIEW_TOOLBAR_PIECES    23
#define ID_VIEW_TOOLBAR_FLOATING  24
#define ID_VIEW_TOOLBAR_BOTH      25
#define ID_VIEW_TOOLBAR_ICONS     26
#define ID_VIEW_TOOLBAR_TEXT      27

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



