// Menu routines.
//

#include <gtk/gtk.h>
#include <stdio.h>
#include "typedefs.h"
#include "menu.h"
#include "main.h"
#include "toolbar.h"
#include "custom.h"

typedef enum { TB_BOTH, TB_ICONS, TB_TEXT, TB_TOOLBAR, TB_DRAWING, TB_ANIMATION, TB_STATUS };

void toolbar_set_style (GtkWidget* widget, gpointer data)
{
  if (main_toolbar.toolbar == NULL)
    return;

  int id = (int)data;

  switch (id)
  {
    case TB_BOTH: {
      user_rc.toolbar_style = GTK_TOOLBAR_BOTH;
    } break;

    case TB_ICONS: {
      user_rc.toolbar_style = GTK_TOOLBAR_ICONS;
    } break;

    case TB_TEXT: {
      user_rc.toolbar_style = GTK_TOOLBAR_TEXT;
    } break;

    case TB_TOOLBAR:
    {
      if (user_rc.view_main_toolbar)
      {
	user_rc.view_main_toolbar = 0;
	gtk_widget_hide (main_toolbar.handle_box);
      }
      else
      {
	user_rc.view_main_toolbar = 1;
	gtk_widget_show (main_toolbar.handle_box);
      }
    } break;

    case TB_DRAWING:
    {
      if (user_rc.view_tool_toolbar)
      {
	user_rc.view_tool_toolbar = 0;
	gtk_widget_hide (tool_toolbar.handle_box);
      }
      else
      {
	user_rc.view_tool_toolbar = 1;
	gtk_widget_show (tool_toolbar.handle_box);
      }
    } break;

    case TB_ANIMATION:
    {
       if (user_rc.view_anim_toolbar)
      {
	user_rc.view_anim_toolbar = 0;
	gtk_widget_hide (anim_toolbar.handle_box);
      }
      else
      {
	user_rc.view_anim_toolbar = 1;
	gtk_widget_show (anim_toolbar.handle_box);
      }
   } break;

    case TB_STATUS:
    {
    } break;
  }

  if (id == TB_BOTH || id == TB_ICONS || id ==TB_TEXT)
  {
    gtk_toolbar_set_style (GTK_TOOLBAR (main_toolbar.toolbar), user_rc.toolbar_style);
    gtk_toolbar_set_style (GTK_TOOLBAR (tool_toolbar.toolbar), user_rc.toolbar_style);
    gtk_toolbar_set_style (GTK_TOOLBAR (anim_toolbar.toolbar), user_rc.toolbar_style);
    gtk_widget_set_usize (main_toolbar.handle_box, -1, -1);
    gtk_widget_set_usize (tool_toolbar.handle_box, -1, -1);
    gtk_widget_set_usize (anim_toolbar.handle_box, -1, -1);
  }
}


static void menu_separator (GtkWidget * menu)
{
  GtkWidget *menu_item = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menu_item);
  gtk_widget_set_sensitive (menu_item, FALSE);
  gtk_widget_show (menu_item);
}

static void menu_tearoff (GtkWidget * menu)
{
  GtkWidget *menu_item = gtk_tearoff_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menu_item);
  gtk_widget_set_sensitive (menu_item, FALSE);
  gtk_widget_show (menu_item);
}

static GtkWidget* create_sub_menu (GtkWidget * menu_bar, char *label, char RIGTH_JUSTIFY)
{
  GtkWidget *sub_menu = gtk_menu_item_new_with_label (label);
  GtkWidget *menu = gtk_menu_new ();
  gtk_menu_bar_append (GTK_MENU_BAR (menu_bar), sub_menu);
  gtk_widget_show (sub_menu);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (sub_menu), menu);
  if (RIGTH_JUSTIFY)
    gtk_menu_item_right_justify (GTK_MENU_ITEM (sub_menu));
  return menu;
}

static GtkWidget* create_menu_item (GtkWidget * menu, gchar * label, gint sensitive_flag, 
				    GtkSignalFunc this_func, gpointer this_func_data)
{
  GtkWidget *menu_item = gtk_menu_item_new_with_label (label);
  gtk_menu_append (GTK_MENU (menu), menu_item);
  gtk_widget_set_sensitive (menu_item, sensitive_flag);
  gtk_widget_show (menu_item);
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (this_func), this_func_data);
  return menu_item;
}

static GtkWidget* create_check_menu_item (GtkWidget * menu, gchar * label, gint state_flag,
					  GtkSignalFunc this_func, gpointer this_func_data)
{
  GtkWidget *menu_item = gtk_check_menu_item_new_with_label (label);
  gtk_menu_append (GTK_MENU (menu), menu_item);
  gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM (menu_item), state_flag);
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menu_item), TRUE);
  gtk_widget_show (menu_item);
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (this_func), this_func_data);
  return menu_item;
}

static GtkWidget* create_menu_radio_item (GtkWidget * menu, GtkWidget * last_radio, gchar * label,
					  GtkSignalFunc this_func, gpointer this_func_data, gint state_flag)
{
  GtkWidget *menu_item;
  if (last_radio == NULL)
    menu_item = gtk_radio_menu_item_new_with_label ((GSList*)NULL, label);
  else
    menu_item = gtk_radio_menu_item_new_with_label (gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (last_radio)), label);
  gtk_menu_append (GTK_MENU (menu), menu_item);
  //gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (menu_item), TRUE);
  gtk_widget_show (menu_item);
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate", GTK_SIGNAL_FUNC (this_func), this_func_data);
  if (state_flag)
    gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM (menu_item), state_flag);

  return menu_item;
}

static GtkWidget* create_menu_in_menu (GtkWidget * menu, gchar * label)
{
  GtkWidget *menu_item = gtk_menu_item_new_with_label (label);
  GtkWidget *menu_in_menu = gtk_menu_new ();
  gtk_menu_append (GTK_MENU (menu), menu_item);
  gtk_widget_show (menu_item);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu_in_menu);
  return menu_in_menu;
}

MAINMENU main_menu;

void create_main_menu(GtkWidget *window, GtkWidget *vbox)
{
  GtkWidget *handle_box, *menu, *menu_in_menu, *item;
  GtkAccelGroup *accel = gtk_accel_group_get_default ();

  handle_box = gtk_handle_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), handle_box, FALSE, FALSE, 0);
  gtk_widget_show (handle_box);

  main_menu.menu_bar = gtk_menu_bar_new ();
  gtk_container_add (GTK_CONTAINER (handle_box), main_menu.menu_bar);
  gtk_widget_show (main_menu.menu_bar);

  // File menu
  main_menu.file_menu = menu = create_sub_menu (main_menu.menu_bar, "File", FALSE);
  menu_tearoff (menu);

  item = create_menu_item (menu, "New", TRUE,
         GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_FILE_NEW);
  gtk_widget_add_accelerator (item, "activate", accel, 'N',
         GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  item = create_menu_item (menu, "Open...", TRUE,
         GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_FILE_OPEN);
  gtk_widget_add_accelerator (item, "activate", accel, 'O',
         GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  create_menu_item (menu, "Merge...", TRUE,
         GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_FILE_MERGE);
  menu_separator (menu);

  item = create_menu_item (menu, "Save", TRUE,
         GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_FILE_SAVE);
  gtk_widget_add_accelerator (item, "activate", accel, 'S',
         GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  item = create_menu_item (menu, "Save As...", TRUE,
         GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_FILE_SAVEAS);
  item = create_menu_item (menu, "Save Picture...", TRUE,
         GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_FILE_PICTURE);

  menu_in_menu = create_menu_in_menu (menu, "Export");
  create_menu_item (menu_in_menu, "HTML...", TRUE,
         GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_FILE_HTML);
  create_menu_item (menu_in_menu, "POV-Ray...", TRUE,
         GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_FILE_POVRAY);
  create_menu_item (menu_in_menu, "Wavefront...", TRUE,
         GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_FILE_WAVEFRONT);
  menu_separator (menu);

  create_menu_item (menu, "Properties...", TRUE,
         GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_FILE_PROPERTIES);
  create_menu_item (menu, "Piece Library Manager...", TRUE,
         GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_FILE_LIBRARY);
  create_menu_item (menu, "Terrain Editor...", TRUE, 
         GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_FILE_TERRAIN);
  menu_separator (menu);

  main_menu.file_recent[0] = create_menu_item (menu, "Recent Files", FALSE, 
      GTK_SIGNAL_FUNC (OnCommand), (void*)ID_FILE_RECENT1);
  main_menu.file_recent[1] = create_menu_item (menu, "", TRUE, 
      GTK_SIGNAL_FUNC (OnCommand), (void*)ID_FILE_RECENT2);
  main_menu.file_recent[2] = create_menu_item (menu, "", TRUE, 
      GTK_SIGNAL_FUNC (OnCommand), (void*)ID_FILE_RECENT3);
  main_menu.file_recent[3] = create_menu_item (menu, "", TRUE,
      GTK_SIGNAL_FUNC (OnCommand), (void*)ID_FILE_RECENT4);
  gtk_widget_hide (main_menu.file_recent[1]);
  gtk_widget_hide (main_menu.file_recent[2]);
  gtk_widget_hide (main_menu.file_recent[3]);

  menu_separator (menu);
  item = create_menu_item (menu, "Exit...", TRUE,
      GTK_SIGNAL_FUNC (OnCommand), (void*)ID_FILE_EXIT);
  gtk_widget_add_accelerator (item, "activate", accel, 'Q',
       GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  // Edit menu
  menu = create_sub_menu (main_menu.menu_bar, "Edit", FALSE);
  menu_tearoff (menu);

  main_menu.edit_undo = create_menu_item (menu, "Undo", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_EDIT_UNDO);
  gtk_widget_add_accelerator (main_menu.edit_undo, "activate", accel, 'Z',
      GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  main_menu.edit_redo = create_menu_item (menu, "Redo", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_EDIT_REDO);
  gtk_widget_add_accelerator (main_menu.edit_redo, "activate", accel, 'Y',
      GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  menu_separator (menu);

  main_menu.edit_cut = create_menu_item (menu, "Cut", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_EDIT_CUT);
  gtk_widget_add_accelerator (main_menu.edit_cut, "activate", accel, 'X',
      GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  main_menu.edit_copy = create_menu_item (menu, "Copy", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_EDIT_COPY);
  gtk_widget_add_accelerator (main_menu.edit_copy, "activate", accel, 'C',
      GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  main_menu.edit_paste = create_menu_item (menu, "Paste", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_EDIT_PASTE);
  gtk_widget_add_accelerator (main_menu.edit_paste, "activate", accel, 'V',
      GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  menu_separator (menu);

  main_menu.edit_select_all = create_menu_item (menu, "Select All", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_EDIT_SELECT_ALL);
  main_menu.edit_select_none = create_menu_item (menu, "Select None", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_EDIT_SELECT_NONE);
  main_menu.edit_select_invert = create_menu_item (menu, "Select Invert", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_EDIT_SELECT_INVERT);
  main_menu.edit_select_byname = create_menu_item (menu, "Select by Name...", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_EDIT_SELECT_BYNAME);

  // Piece menu
  menu = create_sub_menu (main_menu.menu_bar, "Piece", FALSE);
  menu_tearoff (menu);

  create_menu_item (menu, "Insert", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_PIECE_INSERT);
  main_menu.piece_delete = create_menu_item (menu, "Delete", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_PIECE_DELETE);
  create_menu_item (menu, "Minifig Wizard...", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_PIECE_MINIFIG);
  main_menu.piece_array = create_menu_item (menu, "Array...", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_PIECE_ARRAY);
  main_menu.piece_copy_keys = create_menu_item (menu, "Copy Keys", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_PIECE_COPYKEYS);
  menu_separator (menu);

  main_menu.piece_group = create_menu_item (menu, "Group", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_PIECE_GROUP);
  gtk_widget_add_accelerator (main_menu.piece_group, "activate", accel, 'G',
      GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  main_menu.piece_ungroup = create_menu_item (menu, "Ungroup", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_PIECE_UNGROUP);
  gtk_widget_add_accelerator (main_menu.piece_ungroup, "activate", accel, 'U',
      GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  main_menu.piece_group_remove = create_menu_item (menu, "Remove from Group", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_PIECE_GROUP_REMOVE);
  main_menu.piece_group_add = create_menu_item (menu, "Add to Group", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_PIECE_GROUP_ADD);
  main_menu.piece_edit_groups = create_menu_item (menu, "Edit Groups...", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_PIECE_GROUP_EDIT);
  menu_separator (menu);
  
  main_menu.piece_hide_sel = create_menu_item (menu, "Hide Selected", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_PIECE_HIDE_SELECTED);
  main_menu.piece_hide_unsel = create_menu_item (menu, "Hide Unselected", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_PIECE_HIDE_UNSELECTED);
  main_menu.piece_unhide = create_menu_item (menu, "Unhide All", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_PIECE_UNHIDE_ALL);

  // View menu
  menu = create_sub_menu (main_menu.menu_bar, "View", FALSE);
  menu_tearoff (menu);

  create_menu_item (menu, "Preferences...", TRUE,
      GTK_SIGNAL_FUNC(OnCommandDirect), (void*)LC_VIEW_PREFERENCES);
  menu_separator (menu);

  menu_in_menu = create_menu_in_menu (menu, "Toolbars");
  create_check_menu_item (menu_in_menu, "Standard", user_rc.view_main_toolbar, 
			  GTK_SIGNAL_FUNC(toolbar_set_style), (void*)TB_TOOLBAR);
  create_check_menu_item (menu_in_menu, "Drawing", user_rc.view_tool_toolbar,
			  GTK_SIGNAL_FUNC(toolbar_set_style), (void*)TB_DRAWING);
  create_check_menu_item (menu_in_menu, "Animation", user_rc.view_anim_toolbar,
			  GTK_SIGNAL_FUNC(toolbar_set_style), (void*)TB_ANIMATION);
//  create_check_menu_item (menu_in_menu, "Modify", FALSE, GTK_SIGNAL_FUNC(OnCommand), NULL);
//  create_check_menu_item (menu_in_menu, "Status Bar", FALSE, GTK_SIGNAL_FUNC(OnCommand), NULL);
  menu_separator (menu_in_menu);

  item = create_menu_radio_item (menu_in_menu, (GtkWidget*)NULL, "Icons and Text", 
      GTK_SIGNAL_FUNC (toolbar_set_style), (void*)TB_BOTH, user_rc.toolbar_style == GTK_TOOLBAR_BOTH);
  item = create_menu_radio_item (menu_in_menu, item, "Icons only", 
      GTK_SIGNAL_FUNC (toolbar_set_style), (void*)TB_ICONS, user_rc.toolbar_style == GTK_TOOLBAR_ICONS);
  create_menu_radio_item (menu_in_menu, item, "Text only",
      GTK_SIGNAL_FUNC (toolbar_set_style), (void*)TB_TEXT, user_rc.toolbar_style == GTK_TOOLBAR_TEXT);

  create_menu_item (menu, "Zoom In", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_VIEW_ZOOMIN);
  create_menu_item (menu, "Zoom Out", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_VIEW_ZOOMOUT);
  create_menu_item (menu, "Zoom Extents", TRUE,
      GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_VIEW_ZOOMEXTENTS);
  menu_separator (menu);

  menu_in_menu = create_menu_in_menu (menu, "Viewports");

  for (int i = 0; i < 14; i++)
  {
    char str[20];
    sprintf(str, "Viewports %d", i + 1);
    main_menu.view_viewports[i] = create_menu_radio_item (menu_in_menu,
        i == 0 ? (GtkWidget*)NULL : main_menu.view_viewports[i-1], str,
        GTK_SIGNAL_FUNC(OnCommand), (void*)(i + ID_VIEW_VIEWPORTS_01), i == 0);
  }

  main_menu.view_cameras_popup = gtk_menu_item_new_with_label ("Cameras");
  menu_in_menu = gtk_menu_new ();
  gtk_menu_append (GTK_MENU (menu), main_menu.view_cameras_popup);
  gtk_widget_show (main_menu.view_cameras_popup);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (main_menu.view_cameras_popup), menu_in_menu);
  create_menu_item (menu_in_menu, "Dummy", TRUE, GTK_SIGNAL_FUNC(OnCommand), (void*)NULL);

  menu_in_menu = main_menu.view_step_popup = create_menu_in_menu (menu, "Step");
  main_menu.view_step_first = create_menu_item (menu_in_menu, "First", TRUE,
      GTK_SIGNAL_FUNC(OnCommandDirect), (void*)LC_VIEW_STEP_FIRST);
  main_menu.view_step_prev = create_menu_item (menu_in_menu, "Previous", TRUE,
      GTK_SIGNAL_FUNC(OnCommandDirect), (void*)LC_VIEW_STEP_PREVIOUS);
  main_menu.view_step_next = create_menu_item (menu_in_menu, "Next", TRUE,
      GTK_SIGNAL_FUNC(OnCommandDirect), (void*)LC_VIEW_STEP_NEXT);
  main_menu.view_step_last = create_menu_item (menu_in_menu, "Last", TRUE,
      GTK_SIGNAL_FUNC(OnCommandDirect), (void*)LC_VIEW_STEP_LAST);

  menu = create_sub_menu (main_menu.menu_bar, "Help", FALSE);
  menu_tearoff (menu);
  create_menu_item (menu, "About", TRUE, GTK_SIGNAL_FUNC(OnCommandDirect), (void*)LC_HELP_ABOUT);

  /*
  item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);

  gtk_item_factory_create_items(item_factory, nmenu_items, main_menu_items, NULL);

  gtk_accel_group_attach (accel_group, GTK_OBJECT (window));

  if (menubar)
    *menubar = gtk_item_factory_get_widget(item_factory, "<main>");

  gtk_container_add (GTK_CONTAINER (handle_box), *menubar);
  gtk_widget_show(*menubar);
  */
}



