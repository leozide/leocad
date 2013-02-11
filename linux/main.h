#ifndef _MAIN_H_
#define _MAIN_H_

// FIXME: clean up
//extern GtkWidget* main_window;
#include "mainwnd.h"
#include "globals.h"

extern GtkWidget* drawing_area;

extern bool ignore_commands;
void OnCommand(GtkWidget *w, gpointer data);
void OnCommandDirect(GtkWidget *w, gpointer data);

class PieceInfo;
extern PieceInfo *dragged_piece;
extern bool dragging_color;
extern const GtkTargetEntry drag_target_list[];

#define ID_FILE_RECENT1            1
#define ID_FILE_RECENT2            2
#define ID_FILE_RECENT3            3
#define ID_FILE_RECENT4            4
#define ID_FILE_EXIT               5
#define ID_VIEW_CREATE             6
#define ID_VIEW_VIEWPORTS_01       7
#define ID_VIEW_VIEWPORTS_02       8
#define ID_VIEW_VIEWPORTS_03       9
#define ID_VIEW_VIEWPORTS_04      10
#define ID_VIEW_VIEWPORTS_05      11
#define ID_VIEW_VIEWPORTS_06      12
#define ID_VIEW_VIEWPORTS_07      13
#define ID_VIEW_VIEWPORTS_08      14
#define ID_VIEW_VIEWPORTS_09      15
#define ID_VIEW_VIEWPORTS_10      16
#define ID_VIEW_VIEWPORTS_11      17
#define ID_VIEW_VIEWPORTS_12      18
#define ID_VIEW_VIEWPORTS_13      19
#define ID_VIEW_VIEWPORTS_14      20
#define ID_VIEW_TOOLBAR_STANDARD  21
#define ID_VIEW_TOOLBAR_DRAWING   22
#define ID_VIEW_TOOLBAR_ANIMATION 23
#define ID_VIEW_TOOLBAR_MODIFY    24
#define ID_VIEW_TOOLBAR_PIECES    25
#define ID_VIEW_TOOLBAR_FLOATING  26
#define ID_VIEW_TOOLBAR_BOTH      27
#define ID_VIEW_TOOLBAR_ICONS     28
#define ID_VIEW_TOOLBAR_TEXT      29

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
#define ID_SNAP_ALL            134
#define ID_SNAP_NONE           135
#define ID_SNAP_ON             136

#define ID_LOCK_X              140
#define ID_LOCK_Y              141
#define ID_LOCK_Z              142
#define ID_LOCK_NONE           143
#define ID_LOCK_ON             144

#endif



