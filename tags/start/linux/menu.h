#ifndef _MENU_H_
#define _MENU_H_

void create_main_menu(GtkWidget *window, GtkWidget *vbox);

typedef struct
{
  GtkWidget* menu_bar;

  GtkWidget* file_menu;
//  GtkWidget* file_send;
//  GtkWidget* file_print;
//  GtkWidget* file_print_pieces;
//  GtkWidget* file_print_preview;
//  GtkWidget* file_print_setup;
  GtkWidget* file_recent[4];

  GtkWidget* edit_undo;
  GtkWidget* edit_redo;
  GtkWidget* edit_cut;
  GtkWidget* edit_copy;
  GtkWidget* edit_paste;
  GtkWidget* edit_select_all;
  GtkWidget* edit_select_none;
  GtkWidget* edit_select_invert;
  GtkWidget* edit_select_byname;

  GtkWidget* piece_delete;
  GtkWidget* piece_array;
  GtkWidget* piece_copy_keys;
  GtkWidget* piece_group;
  GtkWidget* piece_ungroup;
  GtkWidget* piece_group_remove;
  GtkWidget* piece_group_add;
  GtkWidget* piece_edit_groups;
  GtkWidget* piece_hide_sel;
  GtkWidget* piece_hide_unsel;
  GtkWidget* piece_unhide;

  //  GtkWidget* view_modify;
  GtkWidget* view_viewports[14];
  GtkWidget* view_cameras_popup;
  GtkWidget* view_cameras[25];
  GtkWidget* view_step_popup;
  GtkWidget* view_step_first;
  GtkWidget* view_step_prev;
  GtkWidget* view_step_next;
  GtkWidget* view_step_last;

} MAINMENU;

extern MAINMENU main_menu;

#endif
