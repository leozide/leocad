// Menu Creation.
//

#include <gtk/gtk.h>
#include <stdio.h>
#include "typedefs.h"
#include "main.h"
#include "gtkmisc.h"

void create_main_menu (GtkObject *window, GtkWidget *vbox)
{
  GtkWidget *handle_box, *menu_bar, *menu, *menu_in_menu, *item;
  GtkAccelGroup *accel, *menu_accel, *menu_in_menu_accel;

  accel = gtk_accel_group_get_default ();
  handle_box = gtk_handle_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), handle_box, FALSE, FALSE, 0);
  gtk_widget_show (handle_box);

  menu_bar = gtk_menu_bar_new ();
  gtk_container_add (GTK_CONTAINER (handle_box), menu_bar);
  gtk_widget_show (menu_bar);

  // File menu
  menu = create_sub_menu (menu_bar, "_File", accel, &menu_accel);
  menu_tearoff (menu);

  create_menu_item (menu, "_New", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_FILE_NEW, "menu_file_new");
  create_menu_item (menu, "_Open...", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_FILE_OPEN, "menu_file_open");
  create_menu_item (menu, "_Merge...", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_FILE_MERGE, "menu_file_merge");
  menu_separator (menu);

  create_menu_item (menu, "_Save", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_FILE_SAVE, "menu_file_save");
  create_menu_item (menu, "Save _As...", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_FILE_SAVEAS, "menu_file_saveas");
  create_menu_item (menu, "Save Pic_ture...", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_FILE_PICTURE, "menu_file_picture");

  menu_in_menu = create_menu_in_menu (menu, "Ex_port", menu_accel, &menu_in_menu_accel);
  create_menu_item (menu_in_menu, "_HTML...", menu_in_menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_FILE_HTML, "menu_file_html");
  create_menu_item (menu_in_menu, "_POV-Ray...", menu_in_menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_FILE_POVRAY, "menu_file_povray");
  create_menu_item (menu_in_menu, "_Wavefront...", menu_in_menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_FILE_WAVEFRONT, "menu_file_wavefront");
  menu_separator (menu);

  create_menu_item (menu, "Propert_ies...", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_FILE_PROPERTIES, "menu_file_properties");
  create_menu_item (menu, "Piece _Library Manager...", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_FILE_LIBRARY, "menu_file_library");
  create_menu_item (menu, "Terrain _Editor...", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_FILE_TERRAIN, "menu_file_terrain");
  menu_separator (menu);

  create_menu_item (menu, "1", menu_accel, GTK_SIGNAL_FUNC (OnCommand),
		    window, ID_FILE_RECENT1, "menu_file_recent1");
  create_menu_item (menu, "2", menu_accel, GTK_SIGNAL_FUNC (OnCommand),
		    window, ID_FILE_RECENT2, "menu_file_recent2");
  create_menu_item (menu, "3", menu_accel, GTK_SIGNAL_FUNC (OnCommand),
		    window, ID_FILE_RECENT3, "menu_file_recent3");
  create_menu_item (menu, "4", menu_accel, GTK_SIGNAL_FUNC (OnCommand),
		    window, ID_FILE_RECENT4, "menu_file_recent4");
  gtk_object_set_data (window, "file_menu_accel", menu_accel);

  menu_separator (menu);
  create_menu_item (menu, "E_xit...", menu_accel, GTK_SIGNAL_FUNC (OnCommand),
		    window, ID_FILE_EXIT, "menu_file_exit");

  // Edit menu
  menu = create_sub_menu (menu_bar, "_Edit", accel, &menu_accel);
  menu_tearoff (menu);

  create_menu_item (menu, "_Undo", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_EDIT_UNDO, "menu_edit_undo");
  create_menu_item (menu, "_Redo", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_EDIT_REDO, "menu_edit_redo");
  menu_separator (menu);

  create_menu_item (menu, "Cu_t", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_EDIT_CUT, "menu_edit_cut");
  create_menu_item (menu, "_Copy", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_EDIT_COPY, "menu_edit_copy");
  create_menu_item (menu, "_Paste", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_EDIT_PASTE, "menu_edit_paste");
  menu_separator (menu);

  create_menu_item (menu, "Select _All", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_EDIT_SELECT_ALL, "menu_edit_select_all");
  create_menu_item (menu, "Select _None", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_EDIT_SELECT_NONE, "menu_edit_select_none");
  create_menu_item (menu, "Select _Invert", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_EDIT_SELECT_INVERT, "menu_edit_select_invert");
  create_menu_item (menu, "_Select by Name...", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_EDIT_SELECT_BYNAME, "menu_edit_select_byname");

  // Piece menu
  menu = create_sub_menu (menu_bar, "_Piece", accel, &menu_accel);
  menu_tearoff (menu);

  create_menu_item (menu, "_Insert", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_PIECE_INSERT, "menu_piece_insert");
  create_menu_item (menu, "De_lete", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_PIECE_DELETE, "menu_piece_delete");
  create_menu_item (menu, "Minifig Wi_zard...", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_PIECE_MINIFIG, "menu_piece_minifig");
  create_menu_item (menu, "Ar_ray...", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_PIECE_ARRAY, "menu_piece_array");
  create_menu_item (menu, "_Copy Keys", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_PIECE_COPYKEYS, "menu_piece_copykeys");
  menu_separator (menu);

  create_menu_item (menu, "_Group", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_PIECE_GROUP, "menu_piece_group");
  create_menu_item (menu, "_Ungroup", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_PIECE_UNGROUP, "menu_piece_ungroup");
  create_menu_item (menu, "Re_move from Group", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_PIECE_GROUP_REMOVE, "menu_piece_group_remove");
  create_menu_item (menu, "A_dd to Group", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_PIECE_GROUP_ADD, "menu_piece_group_add");
  create_menu_item (menu, "Edi_t Groups...", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_PIECE_GROUP_EDIT, "menu_piece_group_edit");
  menu_separator (menu);
  
  create_menu_item (menu, "Hide _Selected", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_PIECE_HIDE_SELECTED, "menu_piece_hide_selected");
  create_menu_item (menu, "Hide U_nselected", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_PIECE_HIDE_UNSELECTED, "menu_piece_hide_unselected");
  create_menu_item (menu, "Unhide _All", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_PIECE_UNHIDE_ALL, "menu_piece_unhide_all");

  // View menu
  menu = create_sub_menu (menu_bar, "_View", accel, &menu_accel);
  menu_tearoff (menu);

  create_menu_item (menu, "_Preferences...", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_VIEW_PREFERENCES, "menu_view_preferences");
  menu_separator (menu);

  menu_in_menu = create_menu_in_menu (menu, "Tool_bars", menu_accel, &menu_in_menu_accel);
  create_check_menu_item (menu_in_menu, "S_tandard", menu_in_menu_accel, GTK_SIGNAL_FUNC (OnCommand), 
			  window, ID_VIEW_TOOLBAR_STANDARD, "menu_view_toolbar_standard");
  create_check_menu_item (menu_in_menu, "Dra_wing", menu_in_menu_accel, GTK_SIGNAL_FUNC (OnCommand), 
			  window, ID_VIEW_TOOLBAR_DRAWING, "menu_view_toolbar_drawing");
  create_check_menu_item (menu_in_menu, "Ani_mation", menu_in_menu_accel, GTK_SIGNAL_FUNC (OnCommand), 
			  window, ID_VIEW_TOOLBAR_ANIMATION, "menu_view_toolbar_animation");
  create_check_menu_item (menu_in_menu, "_Pieces", menu_in_menu_accel, GTK_SIGNAL_FUNC (OnCommand), 
			  window, ID_VIEW_TOOLBAR_PIECES, "menu_view_toolbar_pieces");
  menu_separator (menu_in_menu);
  create_check_menu_item (menu_in_menu, "_Floating Pieces", menu_in_menu_accel, GTK_SIGNAL_FUNC (OnCommand), 
			  window, ID_VIEW_TOOLBAR_FLOATING, "menu_view_toolbar_floating");
  item = create_radio_menu_item (menu_in_menu, NULL, "Icons _and Text", menu_in_menu_accel,
				 GTK_SIGNAL_FUNC (OnCommand), window,
				 ID_VIEW_TOOLBAR_BOTH, "menu_view_toolbar_both");
  item = create_radio_menu_item (menu_in_menu, item, "_Icons only", menu_in_menu_accel,
				 GTK_SIGNAL_FUNC (OnCommand), window,
				 ID_VIEW_TOOLBAR_ICONS, "menu_view_toolbar_icons");
  item = create_radio_menu_item (menu_in_menu, item, "Te_xt only", menu_in_menu_accel,
				 GTK_SIGNAL_FUNC (OnCommand), window,
				 ID_VIEW_TOOLBAR_TEXT, "menu_view_toolbar_text");

  create_menu_item (menu, "Zoom _In", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_VIEW_ZOOMIN, "menu_view_zoomin");
  create_menu_item (menu, "Zoom _Out", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_VIEW_ZOOMOUT, "menu_view_zoomout");
  create_menu_item (menu, "Zoom E_xtents", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_VIEW_ZOOMEXTENTS, "menu_view_zoomextents");
  menu_separator (menu);

  menu_in_menu = create_menu_in_menu (menu, "Vie_wports", menu_accel, &menu_in_menu_accel);
  item = create_radio_menu_pixmap (menu_in_menu, NULL, "vports01.xpm", menu_in_menu_accel,
				   GTK_SIGNAL_FUNC (OnCommand), window,
				   ID_VIEW_VIEWPORTS_01, "menu_view_viewports_01");
  item = create_radio_menu_pixmap (menu_in_menu, item, "vports02.xpm", menu_in_menu_accel,
				   GTK_SIGNAL_FUNC (OnCommand), window,
				   ID_VIEW_VIEWPORTS_02, "menu_view_viewports_02");
  item = create_radio_menu_pixmap (menu_in_menu, item, "vports03.xpm", menu_in_menu_accel,
				   GTK_SIGNAL_FUNC (OnCommand), window,
				   ID_VIEW_VIEWPORTS_03, "menu_view_viewports_03");
  item = create_radio_menu_pixmap (menu_in_menu, item, "vports04.xpm", menu_in_menu_accel,
				   GTK_SIGNAL_FUNC (OnCommand), window,
				   ID_VIEW_VIEWPORTS_04, "menu_view_viewports_04");
  item = create_radio_menu_pixmap (menu_in_menu, item, "vports05.xpm", menu_in_menu_accel,
				   GTK_SIGNAL_FUNC (OnCommand), window,
				   ID_VIEW_VIEWPORTS_05, "menu_view_viewports_05");
  item = create_radio_menu_pixmap (menu_in_menu, item, "vports06.xpm", menu_in_menu_accel,
				   GTK_SIGNAL_FUNC (OnCommand), window,
				   ID_VIEW_VIEWPORTS_06, "menu_view_viewports_06");
  item = create_radio_menu_pixmap (menu_in_menu, item, "vports07.xpm", menu_in_menu_accel,
				   GTK_SIGNAL_FUNC (OnCommand), window,
				   ID_VIEW_VIEWPORTS_07, "menu_view_viewports_07");
  item = create_radio_menu_pixmap (menu_in_menu, item, "vports08.xpm", menu_in_menu_accel,
				   GTK_SIGNAL_FUNC (OnCommand), window,
				   ID_VIEW_VIEWPORTS_08, "menu_view_viewports_08");
  item = create_radio_menu_pixmap (menu_in_menu, item, "vports09.xpm", menu_in_menu_accel,
				   GTK_SIGNAL_FUNC (OnCommand), window,
				   ID_VIEW_VIEWPORTS_09, "menu_view_viewports_09");
  item = create_radio_menu_pixmap (menu_in_menu, item, "vports10.xpm", menu_in_menu_accel,
				   GTK_SIGNAL_FUNC (OnCommand), window,
				   ID_VIEW_VIEWPORTS_10, "menu_view_viewports_10");
  item = create_radio_menu_pixmap (menu_in_menu, item, "vports11.xpm", menu_in_menu_accel,
				   GTK_SIGNAL_FUNC (OnCommand), window,
				   ID_VIEW_VIEWPORTS_11, "menu_view_viewports_11");
  item = create_radio_menu_pixmap (menu_in_menu, item, "vports12.xpm", menu_in_menu_accel,
				   GTK_SIGNAL_FUNC (OnCommand), window,
				   ID_VIEW_VIEWPORTS_12, "menu_view_viewports_12");
  item = create_radio_menu_pixmap (menu_in_menu, item, "vports13.xpm", menu_in_menu_accel,
				   GTK_SIGNAL_FUNC (OnCommand), window,
				   ID_VIEW_VIEWPORTS_13, "menu_view_viewports_13");
  item = create_radio_menu_pixmap (menu_in_menu, item, "vports14.xpm", menu_in_menu_accel,
				   GTK_SIGNAL_FUNC (OnCommand), window,
				   ID_VIEW_VIEWPORTS_14, "menu_view_viewports_14");

  menu_in_menu = create_menu_in_menu (menu, "_Cameras", menu_accel, &menu_in_menu_accel);
  gtk_object_set_data (window, "cameras_menu", menu_in_menu);

  menu_in_menu = create_menu_in_menu (menu, "S_tep", menu_accel, &menu_in_menu_accel);
  create_menu_item (menu_in_menu, "Fi_rst", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_VIEW_STEP_FIRST, "menu_view_step_first");
  create_menu_item (menu_in_menu, "Pre_vious", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_VIEW_STEP_PREVIOUS, "menu_view_step_previous");
  create_menu_item (menu_in_menu, "Ne_xt", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_VIEW_STEP_NEXT, "menu_view_step_next");
  create_menu_item (menu_in_menu, "_Last", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_VIEW_STEP_LAST, "menu_view_step_last");

  menu = create_sub_menu (menu_bar, "_Help", accel, &menu_accel);
  menu_tearoff (menu);

  create_menu_item (menu, "_About", menu_accel, GTK_SIGNAL_FUNC (OnCommandDirect),
		    window, LC_HELP_ABOUT, "menu_help_about");

  // TODO: read accelerators from a file
  item = GTK_WIDGET (gtk_object_get_data (window, "menu_file_new"));
  gtk_widget_add_accelerator (item, "activate", accel, 'N', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  item = GTK_WIDGET (gtk_object_get_data (window, "menu_file_open"));
  gtk_widget_add_accelerator (item, "activate", accel, 'O', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  item = GTK_WIDGET (gtk_object_get_data (window, "menu_file_save"));
  gtk_widget_add_accelerator (item, "activate", accel, 'S', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  item = GTK_WIDGET (gtk_object_get_data (window, "menu_file_exit"));
  gtk_widget_add_accelerator (item, "activate", accel, 'Q', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  item = GTK_WIDGET (gtk_object_get_data (window, "menu_edit_undo"));
  gtk_widget_add_accelerator (item, "activate", accel, 'Z', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  item = GTK_WIDGET (gtk_object_get_data (window, "menu_edit_redo"));
  gtk_widget_add_accelerator (item, "activate", accel, 'Y', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  item = GTK_WIDGET (gtk_object_get_data (window, "menu_edit_cut"));
  gtk_widget_add_accelerator (item, "activate", accel, 'X', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  item = GTK_WIDGET (gtk_object_get_data (window, "menu_edit_copy"));
  gtk_widget_add_accelerator (item, "activate", accel, 'C', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  item = GTK_WIDGET (gtk_object_get_data (window, "menu_edit_paste"));
  gtk_widget_add_accelerator (item, "activate", accel, 'V', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  item = GTK_WIDGET (gtk_object_get_data (window, "menu_piece_group"));
  gtk_widget_add_accelerator (item, "activate", accel, 'G', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  item = GTK_WIDGET (gtk_object_get_data (window, "menu_piece_ungroup"));
  gtk_widget_add_accelerator (item, "activate", accel, 'U', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
}
