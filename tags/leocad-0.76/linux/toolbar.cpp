// Toolbar creation and related functions
//

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include "lc_global.h"
#include "opengl.h"
#include "gtktools.h"
#include "main.h"
#include "project.h"
#include "pieceinf.h"
#include "toolbar.h"
#include "lc_message.h"
#include "preview.h"
#include "library.h"
#include "lc_application.h"
#include "lc_colors.h"

// =============================================================================
// Variables

GtkWidget *piecetree;
GtkWidget *pieceentry;
GtkWidget *piecemenu;
GtkWidget *colorlist;
GtkWidget *grouptoolbar;
PiecePreview *preview;

TOOL_TOOLBAR tool_toolbar;
MAIN_TOOLBAR main_toolbar;
ANIM_TOOLBAR anim_toolbar;

extern GtkWidget* create_snap_menu();

// =========================================================

void create_toolbars(GtkWidget *window, GtkWidget *vbox)
{
#include "pixmaps/ac-brick.xpm"
#include "pixmaps/ac-light.xpm"
#include "pixmaps/ac-spot.xpm"
#include "pixmaps/ac-cam.xpm"
#include "pixmaps/ac-sel.xpm"
#include "pixmaps/ac-move.xpm"
#include "pixmaps/ac-rot.xpm"
#include "pixmaps/ac-erase.xpm"
#include "pixmaps/ac-paint.xpm"
#include "pixmaps/ac-zoom.xpm"
#include "pixmaps/ac-pan.xpm"
#include "pixmaps/ac-rotv.xpm"
#include "pixmaps/ac-roll.xpm"
#include "pixmaps/ac-zoomr.xpm"
#include "pixmaps/ac-zoome.xpm"
#include "pixmaps/ac-prev.xpm"
#include "pixmaps/ac-next.xpm"
#include "pixmaps/an-anim.xpm"
#include "pixmaps/an-key.xpm"
#include "pixmaps/an-next.xpm"
#include "pixmaps/an-prev.xpm"
#include "pixmaps/an-first.xpm"
#include "pixmaps/an-last.xpm"
#include "pixmaps/an-play.xpm"
#include "pixmaps/an-stop.xpm"
//#include "pixmaps/st-about.xpm"
#include "pixmaps/st-fast.xpm"
//#include "pixmaps/st-paste.xpm"
//#include "pixmaps/st-save.xpm"
//#include "pixmaps/st-help.xpm"
//#include "pixmaps/st-prev.xpm"
#include "pixmaps/st-snap.xpm"
//#include "pixmaps/st-copy.xpm"
//#include "pixmaps/st-new.xpm"
//#include "pixmaps/st-print.xpm"
#include "pixmaps/st-snapa.xpm"
//#include "pixmaps/st-cut.xpm"
//#include "pixmaps/st-open.xpm"
//#include "pixmaps/st-redo.xpm"

	GtkToolItem* item;

	// Main Toolbar
	main_toolbar.handle_box = gtk_handle_box_new();
	gtk_box_pack_start(GTK_BOX(vbox), main_toolbar.handle_box, FALSE, FALSE, 0);
	gtk_widget_show(main_toolbar.handle_box);

	main_toolbar.toolbar = gtk_toolbar_new();
	gtk_toolbar_set_orientation(GTK_TOOLBAR(main_toolbar.toolbar), GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_style(GTK_TOOLBAR(main_toolbar.toolbar), GTK_TOOLBAR_ICONS);
	gtk_container_add(GTK_CONTAINER(main_toolbar.handle_box), main_toolbar.toolbar);

	item = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
	gtk_tool_item_set_tooltip_text(item, "Create a new project");
	g_signal_connect(item, "clicked", G_CALLBACK(OnCommandDirect), GINT_TO_POINTER(LC_FILE_NEW));
	gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar.toolbar), item, -1);

	item = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_tool_item_set_tooltip_text(item, "Open an existing project");
	g_signal_connect(item, "clicked", G_CALLBACK(OnCommandDirect), GINT_TO_POINTER(LC_FILE_OPEN));
	gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar.toolbar), item, -1);

	item = gtk_tool_button_new_from_stock(GTK_STOCK_SAVE);
	gtk_tool_item_set_tooltip_text(item, "Save the current project");
	g_signal_connect(item, "clicked", G_CALLBACK(OnCommandDirect), GINT_TO_POINTER(LC_FILE_SAVE));
	gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar.toolbar), item, -1);

	item = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar.toolbar), item, -1);

	item = gtk_tool_button_new_from_stock(GTK_STOCK_UNDO);
	gtk_tool_item_set_tooltip_text(item, "Undo the last action");
	g_signal_connect(item, "clicked", G_CALLBACK(OnCommandDirect), GINT_TO_POINTER(LC_EDIT_UNDO));
	gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar.toolbar), item, -1);
	main_toolbar.undo = (GtkWidget*)item;

	item = gtk_tool_button_new_from_stock(GTK_STOCK_REDO);
	gtk_tool_item_set_tooltip_text(item, "Redo the last undone action");
	g_signal_connect(item, "clicked", G_CALLBACK(OnCommandDirect), GINT_TO_POINTER(LC_EDIT_REDO));
	gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar.toolbar), item, -1);
	main_toolbar.redo = (GtkWidget*)item;

	item = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar.toolbar), item, -1);

	item = gtk_tool_button_new_from_stock(GTK_STOCK_CUT);
	gtk_tool_item_set_tooltip_text(item, "Cut the selection");
	g_signal_connect(item, "clicked", G_CALLBACK(OnCommandDirect), GINT_TO_POINTER(LC_EDIT_CUT));
	gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar.toolbar), item, -1);
	main_toolbar.cut = (GtkWidget*)item;

	item = gtk_tool_button_new_from_stock(GTK_STOCK_COPY);
	gtk_tool_item_set_tooltip_text(item, "Copy the selection");
	g_signal_connect(item, "clicked", G_CALLBACK(OnCommandDirect), GINT_TO_POINTER(LC_EDIT_COPY));
	gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar.toolbar), item, -1);
	main_toolbar.copy = (GtkWidget*)item;

	item = gtk_tool_button_new_from_stock(GTK_STOCK_PASTE);
	gtk_tool_item_set_tooltip_text(item, "Insert the clipboard contents");
	g_signal_connect(item, "clicked", G_CALLBACK(OnCommandDirect), GINT_TO_POINTER(LC_EDIT_PASTE));
	gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar.toolbar), item, -1);
	main_toolbar.paste = (GtkWidget*)item;

	item = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar.toolbar), item, -1);

	// TODO: add xyz lock

	item = gtk_menu_tool_button_new(new_pixmap(window, st_snap), "3D snap");
	gtk_tool_item_set_tooltip_text(item, "Toggle 3D snap");
	g_signal_connect(item, "clicked", G_CALLBACK(OnCommandDirect), GINT_TO_POINTER(ID_SNAP_ON));
	gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar.toolbar), item, -1);
	main_toolbar.snap = (GtkWidget*)item;

	GtkWidget* menu = create_snap_menu();
	gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(item), menu);
	main_toolbar.snap_menu = menu;

	item = gtk_toggle_tool_button_new();
	gtk_tool_item_set_tooltip_text(item, "Toggle angle snap");
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(item), "Angle snap");
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(item), new_pixmap(window, st_snapa));
	g_signal_connect(item, "clicked", G_CALLBACK(OnCommandDirect), GINT_TO_POINTER(ID_SNAP_A));
	gtk_toolbar_insert(GTK_TOOLBAR(main_toolbar.toolbar), item, -1);
	main_toolbar.angle = (GtkWidget*)item;

	item = gtk_toggle_tool_button_new();
	gtk_tool_item_set_tooltip_text(item, "Toggle fast rendering");
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(item), "Fast render");
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(item), new_pixmap(window, st_fast));
	g_signal_connect(item, "clicked", G_CALLBACK(OnCommandDirect), GINT_TO_POINTER(LC_TOOLBAR_FASTRENDER));
	gtk_toolbar_insert (GTK_TOOLBAR (main_toolbar.toolbar), item, -1);
	main_toolbar.fast = (GtkWidget*)item;

	gtk_widget_show_all(main_toolbar.handle_box);


  GtkWidget *button;

  // Tools Toolbar
  tool_toolbar.handle_box = gtk_handle_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), tool_toolbar.handle_box, FALSE, FALSE, 0);
  gtk_widget_show (tool_toolbar.handle_box);

  tool_toolbar.toolbar = gtk_toolbar_new();
  gtk_toolbar_set_orientation(GTK_TOOLBAR(tool_toolbar.toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style(GTK_TOOLBAR(tool_toolbar.toolbar), GTK_TOOLBAR_ICONS);
  gtk_container_add (GTK_CONTAINER (tool_toolbar.handle_box), tool_toolbar.toolbar);
  gtk_widget_show (tool_toolbar.toolbar);

  tool_toolbar.brick = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, NULL, "Piece", "Insert Piece", "",
     new_pixmap (window, ac_brick), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_INSERT);
  tool_toolbar.light = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Light", "Insert Light", "", 
     new_pixmap (window, ac_light), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_LIGHT);
  tool_toolbar.spot = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Spot", "Insert Spotlight", "",
     new_pixmap (window, ac_spot), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_SPOTLIGHT);
  tool_toolbar.camera = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Camera", "Insert Camera", "",
     new_pixmap (window, ac_cam),  GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_CAMERA);
  tool_toolbar.select = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Select", "Select Objects", "",
     new_pixmap (window, ac_sel), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_SELECT);
  tool_toolbar.move = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Move", "Move Objects", "",
     new_pixmap (window, ac_move), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_MOVE);
  tool_toolbar.rotate = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Rotate", "Rotate Pieces", "",
     new_pixmap (window, ac_rot), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_ROTATE);
  tool_toolbar.erase = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Delete", "Remove Objects", "",
     new_pixmap (window, ac_erase), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_ERASER);
  tool_toolbar.paint = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Paint", "Paint Bricks", "",
     new_pixmap (window, ac_paint), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_PAINT);
  tool_toolbar.zoom = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Zoom", "Zoom", "",
     new_pixmap (window, ac_zoom), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_ZOOM);
  tool_toolbar.pan = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Pan", "Pan", "",
     new_pixmap (window, ac_pan), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_PAN);
  tool_toolbar.rotview = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Rot. View", "Rotate View", "",
     new_pixmap (window, ac_rotv), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_ROTATE_VIEW);
  tool_toolbar.roll = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Roll", "Roll", "",
     new_pixmap (window, ac_roll), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_ROLL);
  tool_toolbar.zoomreg = button = gtk_toolbar_append_element (GTK_TOOLBAR (tool_toolbar.toolbar), 
     GTK_TOOLBAR_CHILD_RADIOBUTTON, button, "Zoom Box", "Zoom Region", "",
     new_pixmap (window, ac_zoomr), GTK_SIGNAL_FUNC (OnCommand), (void*)ID_ACTION_ZOOM_REGION);
  gtk_toolbar_append_item (GTK_TOOLBAR (tool_toolbar.toolbar), "Zoom Ext.", "Zoom Extents", "",
     new_pixmap (window, ac_zoome), GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_VIEW_ZOOMEXTENTS);
  tool_toolbar.prev = gtk_toolbar_append_item (GTK_TOOLBAR (tool_toolbar.toolbar), "Show Previous", "Show piece on previous step", "",
     new_pixmap (window, ac_prev), GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_PIECE_PREVIOUS);
  tool_toolbar.next = gtk_toolbar_append_item (GTK_TOOLBAR (tool_toolbar.toolbar), "Show Next", "Show Piece on next step", "",
     new_pixmap (window, ac_next), GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_PIECE_NEXT);

  // Animation Toolbar
  anim_toolbar.handle_box = gtk_handle_box_new ();
  gtk_box_pack_start (GTK_BOX (vbox), anim_toolbar.handle_box, FALSE, FALSE, 0);
  gtk_widget_show (anim_toolbar.handle_box);

  anim_toolbar.toolbar = gtk_toolbar_new();
  gtk_toolbar_set_orientation(GTK_TOOLBAR(anim_toolbar.toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style(GTK_TOOLBAR(anim_toolbar.toolbar), GTK_TOOLBAR_ICONS);
  gtk_container_add (GTK_CONTAINER (anim_toolbar.handle_box), anim_toolbar.toolbar);
  gtk_widget_show (anim_toolbar.toolbar);

  anim_toolbar.first = gtk_toolbar_append_item (GTK_TOOLBAR (anim_toolbar.toolbar),
     "First", "Go to the Start", "", new_pixmap (window, an_first), 
     GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_VIEW_STEP_FIRST);
  anim_toolbar.prev = gtk_toolbar_append_item (GTK_TOOLBAR (anim_toolbar.toolbar),
     "Previous", "Go Back", "", new_pixmap (window, an_prev),
     GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_VIEW_STEP_PREVIOUS);
  anim_toolbar.play = gtk_toolbar_append_item (GTK_TOOLBAR (anim_toolbar.toolbar),
     "Play", "Play Animation", "", new_pixmap (window, an_play),
     GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_VIEW_PLAY);
  anim_toolbar.stop = gtk_toolbar_append_item (GTK_TOOLBAR (anim_toolbar.toolbar),
     "Stop", "Stop Animation", "", new_pixmap (window, an_stop),
     GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_VIEW_STOP);
  anim_toolbar.next = gtk_toolbar_append_item (GTK_TOOLBAR (anim_toolbar.toolbar),
     "Next", "Go Forward", "", new_pixmap (window, an_next),
     GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_VIEW_STEP_NEXT);
  anim_toolbar.last = gtk_toolbar_append_item (GTK_TOOLBAR (anim_toolbar.toolbar),
     "Last", "Go to the End", "", new_pixmap (window, an_last),
     GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_VIEW_STEP_LAST);
  anim_toolbar.anim = gtk_toolbar_append_element (GTK_TOOLBAR (anim_toolbar.toolbar),
     GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL, "Mode", "Toggle Animation or Instructions", "",
     new_pixmap (window, an_anim), GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_TOOLBAR_ANIMATION);
  anim_toolbar.keys = gtk_toolbar_append_element (GTK_TOOLBAR (anim_toolbar.toolbar),
     GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL, "Keys", "Add Keys", "",
     new_pixmap (window, an_key), GTK_SIGNAL_FUNC (OnCommandDirect), (void*)LC_TOOLBAR_ADDKEYS);
}

// =========================================================
// Pieces toolbar

//static bool list_subparts = false;
static int cur_color = 0;
static GdkPixmap* colorlist_pixmap = NULL;

int PiecesSortFunc(const PieceInfo* a, const PieceInfo* b, void* SortData)
{
	if (a->IsSubPiece())
	{
		if (b->IsSubPiece())
		{
			return strcmp(a->m_strDescription, b->m_strDescription);
		}
		else
		{
			return 1;
		}
	}
	else
	{
		if (b->IsSubPiece())
		{
			return -1;
		}
		else
		{
			return strcmp(a->m_strDescription, b->m_strDescription);
		}
	}

	return 0;
}

void fill_piecetree()
{
  PiecesLibrary* Lib = lcGetPiecesLibrary();
  GtkTreeStore* model = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(piecetree)));

  for (int i = 0; i < Lib->GetNumCategories(); i++)
  {
    GtkTreeIter iter;
    gtk_tree_store_append(model, &iter, NULL);
    gtk_tree_store_set(model, &iter, 0, (const char*)Lib->GetCategoryName(i), 1, NULL, -1);

    lcPtrArray<PieceInfo> SinglePieces, GroupedPieces;

    Lib->GetCategoryEntries(i, true, SinglePieces, GroupedPieces);

    SinglePieces += GroupedPieces;
    SinglePieces.Sort(PiecesSortFunc, NULL);

    for (int j = 0; j < SinglePieces.GetSize(); j++)
    {
      PieceInfo* Info = SinglePieces[j];

      GtkTreeIter entry;

      gtk_tree_store_append(model, &entry, &iter);
      gtk_tree_store_set(model, &entry, 0, Info->m_strDescription, 1, Info, -1);

      if (GroupedPieces.FindIndex(Info) != -1)
      {
	lcPtrArray<PieceInfo> Patterns;
	Lib->GetPatternedPieces(Info, Patterns);

	for (int k = 0; k < Patterns.GetSize(); k++)
	{
	  GtkTreeIter pat;
	  PieceInfo* child = Patterns[k];

	  if (!Lib->PieceInCategory(child, Lib->GetCategoryKeywords(i)))
	    continue;

	  const char* desc = child->m_strDescription;
	  int len = strlen(Info->m_strDescription);

	  if (!strncmp(child->m_strDescription, Info->m_strDescription, len))
	    desc += len;

	  gtk_tree_store_append(model, &pat, &entry);
	  gtk_tree_store_set(model, &pat, 0, desc, 1, child, -1);
	}	
      }
    }
  }

  GtkTreeIter iter;
  gtk_tree_store_append(model, &iter, NULL);
  gtk_tree_store_set(model, &iter, 0, "Search Results", 1, NULL, -1);

  GtkTreeIter entry;
  gtk_tree_store_append(GTK_TREE_STORE(model), &entry, &iter);
  gtk_tree_store_set(GTK_TREE_STORE(model), &entry, 0, "No pieces found", 1, NULL, -1);
}

// Callback for the pieces list.
static void piecetree_changed(GtkTreeSelection* selection, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeModel* model;

  if (gtk_tree_selection_get_selected(selection, &model, &iter))
  {
    gpointer sel;

    gtk_tree_model_get(model, &iter, 1, &sel, -1);

    if (sel)
      g_App->m_PiecePreview->SetSelection((PieceInfo*)sel);
  }
}

static void piececombo_popup_position (GtkMenu *menu, gint *x, gint *y, gboolean* push, gpointer data)
{
  gdk_window_get_origin (pieceentry->window, x, y);
  *y += pieceentry->allocation.height;
  *push = true;
}

static void piececombo_popup (GtkWidget *widget, gpointer data)
{
  if (piecemenu != NULL)
    gtk_menu_popup (GTK_MENU (piecemenu), NULL, NULL, piececombo_popup_position, NULL, 1, GDK_CURRENT_TIME);
}

static void piececombo_selected (GtkWidget *widget, gpointer data)
{
  gchar *str;

  gtk_label_get (GTK_LABEL (GTK_BIN (widget)->child), &str);
  gtk_entry_set_text (GTK_ENTRY (pieceentry), str);
}

// Add a new piece to the combobox
void piececombo_add (const char* str)
{
  if (str == NULL)
  {
    // Clear the list
    if (piecemenu != NULL)
    {
      gtk_widget_destroy (piecemenu);
      piecemenu = NULL;
    }
  }
  else
  {
    GtkWidget *item;
    GList *children;
    int pos = 0;

    if (piecemenu == NULL)
      piecemenu = gtk_menu_new ();

    children = gtk_container_children (GTK_CONTAINER (piecemenu));

    while (children)
    {
      gchar *label;
      int i;

      gtk_label_get (GTK_LABEL (GTK_BIN (children->data)->child), &label);

      i = strcmp (str, label);

      if (i == 0)
        return;
      else if (i < 0)
        break;

      children = children->next;
      pos++;
    }

    item = gtk_menu_item_new_with_label (str);
    gtk_signal_connect (GTK_OBJECT (item), "activate", GTK_SIGNAL_FUNC (piececombo_selected), NULL);
    gtk_widget_show (item);
    gtk_menu_insert (GTK_MENU (piecemenu), item, pos);
  }
}

static gint piececombo_key(GtkWidget* widget, GdkEventKey* event)
{
  if (event->keyval == GDK_Return)
  {
    const gchar* str = gtk_entry_get_text(GTK_ENTRY(pieceentry));
    PiecesLibrary* Lib = lcGetPiecesLibrary();

    // Save search.
    int Index = Lib->FindCategoryIndex("Search Results");

    if (Index == -1)
    {
      Lib->AddCategory("Search Results", (const char*)str);
      Index = Lib->GetNumCategories() - 1;
    }
    else
      Lib->SetCategory(Index, "Search Results", (const char*)str);

    // Find search category row.
    GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(piecetree));
    GtkTreeIter iter;

    if (!gtk_tree_model_get_iter_first(model, &iter))
      return FALSE;

    do
    {
      gchar* name;
      gtk_tree_model_get(model, &iter, 0, &name, -1);

      if (strcmp(name, "Search Results"))
	continue;

      GtkTreeIter child;

      // Remove all children.
      while (gtk_tree_model_iter_children(model, &child, &iter))
	gtk_tree_store_remove(GTK_TREE_STORE(model), &child);	

      // Perform search.
      lcPtrArray<PieceInfo> SinglePieces, GroupedPieces;
      Lib->GetCategoryEntries(Index, true, SinglePieces, GroupedPieces);

      // Merge and sort the arrays.
      SinglePieces += GroupedPieces;
      SinglePieces.Sort(PiecesSortFunc, NULL);

      // Add results.
      for (int i = 0; i < SinglePieces.GetSize(); i++)
      {
	PieceInfo* Info = SinglePieces[i];

	GtkTreeIter entry;
	gtk_tree_store_append(GTK_TREE_STORE(model), &entry, &iter);
	gtk_tree_store_set(GTK_TREE_STORE(model), &entry, 0, Info->m_strDescription, 1, Info, -1);
      }

      if (SinglePieces.GetSize() == 0)
      {
 	GtkTreeIter entry;
	gtk_tree_store_append(GTK_TREE_STORE(model), &entry, &iter);
	gtk_tree_store_set(GTK_TREE_STORE(model), &entry, 0, "No pieces found", 1, NULL, -1);
     }

      // Expand results.
      GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
      gtk_tree_view_expand_row(GTK_TREE_VIEW(piecetree), path, FALSE);
      gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(piecetree), path, NULL, TRUE, 0.5f, 0.0f);
      gtk_tree_path_free(path);

    } while (gtk_tree_model_iter_next(model, &iter));
  }

  return FALSE;
}

static void piececombo_changed (GtkWidget *widget, gpointer data)
{
  PiecesLibrary *pLib = lcGetPiecesLibrary();
  const gchar* str;
  int i;

  str = gtk_entry_get_text (GTK_ENTRY (pieceentry));

  for (i = 0; i < pLib->GetPieceCount (); i++)
  {
    PieceInfo* pInfo = pLib->GetPieceInfo (i);

    if (strcmp (str, pInfo->m_strDescription) == 0)
    {
      // Select the piece
      //      i = gtk_clist_find_row_from_data (GTK_CLIST (piecelist), pInfo);
      //      gtk_clist_select_row (GTK_CLIST (piecelist), i, 0);
      //      if (gtk_clist_row_is_visible (GTK_CLIST (piecelist), i) != GTK_VISIBILITY_FULL)
      //	gtk_clist_moveto (GTK_CLIST (piecelist), i, 0, 0.5f, 0);

      return;
    }
  }
}

const u32 cols = 13;
const u32 rows = 6;
  
// Draw a pixmap for the colorlist control
static void colorlist_draw_pixmap(GtkWidget *widget)
{
  GdkGC* gc = gdk_gc_new(widget->window);
  u32 i;
  GdkRectangle rect;
  GdkColor c;

  gdk_gc_set_fill(gc, GDK_SOLID);

  rect.width = widget->allocation.width/cols;
  rect.height = widget->allocation.height/rows;

    c.red = 0;
    c.green = 0;
    c.blue = 0;
    gdk_gc_set_foreground(gc, &c);

  gdk_draw_rectangle (colorlist_pixmap, gc, TRUE,
      0, 0, widget->allocation.width, widget->allocation.height);

  for (i = 0; i < lcNumUserColors; i++)
  {
      rect.y = rect.height * (i / cols);
      rect.x = rect.width * (i % cols);

    c.red = (gushort)(g_ColorList[i].Value[0]*0xFFFF);
    c.green = (gushort)(g_ColorList[i].Value[1]*0xFFFF);
    c.blue = (gushort)(g_ColorList[i].Value[2]*0xFFFF);
    gdk_color_alloc(gtk_widget_get_colormap(widget), &c);
    gdk_gc_set_foreground(gc, &c);

    gdk_draw_rectangle (colorlist_pixmap, gc, TRUE,
		      rect.x, rect.y,
		      rect.width, rect.height);

    if (LC_COLOR_TRANSLUCENT(i))
    {
      int x, y;
      gdk_color_white(gtk_widget_get_colormap(widget), &c);
      gdk_gc_set_foreground(gc, &c);
      
      for (x = rect.x; x < rect.x + rect.width; x++)
      {
	for (y = rect.y + x%4; y < rect.y + rect.height; y += 4)
	  gdk_draw_point(colorlist_pixmap, gc, x, y);

	for (y = rect.y + rect.height - x%4; y > rect.y; y -= 4)
	  gdk_draw_point(colorlist_pixmap, gc, x, y);
      }
    }
  }

  gdk_color_black(gtk_widget_get_colormap(widget), &c);
  gdk_gc_set_foreground(gc, &c);
  gdk_gc_set_line_attributes(gc, 1,
	GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);

  for (i = 0; i <= cols; i++)
    gdk_draw_line (colorlist_pixmap, gc, rect.width * i, 0,
       rect.width * i, widget->allocation.height);
  
  for (i = 0; i <= rows; i++)
    gdk_draw_line(colorlist_pixmap, gc, 0, rect.height * i,
        widget->allocation.width, rect.height * i);

  c.red = (gushort)(0.4f*0xFFFF);
  c.green = (gushort)(0.8f*0xFFFF);
  c.blue = (gushort)(0.4f*0xFFFF);
  gdk_color_alloc(gtk_widget_get_colormap(widget), &c);
  gdk_gc_set_foreground(gc, &c);

  int l, r, t, b;
  l = rect.width * (cur_color % cols);
  r = rect.width * ((cur_color+1) % cols);
  t = rect.height * (cur_color / cols);
  b = rect.height * ((cur_color+1) / cols);

  gdk_draw_rectangle (colorlist_pixmap, gc, FALSE, l, t, r-l, b-t);

  gdk_gc_destroy(gc);
}

static gint colorlist_configure(GtkWidget *widget, GdkEventConfigure *event)
{
  if (colorlist_pixmap)
    gdk_pixmap_unref(colorlist_pixmap);

  colorlist_pixmap = gdk_pixmap_new(widget->window, widget->allocation.width,
				    widget->allocation.height, -1);
  colorlist_draw_pixmap(widget);

  return TRUE;
}

// Redraw from the backing pixmap
static gint colorlist_expose(GtkWidget *widget, GdkEventExpose *event)
{
  gdk_draw_pixmap(widget->window, widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		  colorlist_pixmap, event->area.x, event->area.y,
		  event->area.x, event->area.y,
		  event->area.width, event->area.height);

  return FALSE;
}

static gint colorlist_key_press(GtkWidget* widget, GdkEventKey* event, gpointer data)
{
  int x;

  switch (event->keyval)
  {
    case GDK_Up: x = cur_color - cols; break;
    case GDK_Down: x = cur_color + cols; break;
    case GDK_Left: x = cur_color - 1; break;
    case GDK_Right: x = cur_color + 1; break;

  default:
    return TRUE;
  }

  if ((x > -1) && (x < (int)lcNumUserColors))
  {
    cur_color = x;
    colorlist_draw_pixmap(widget);
    lcPostMessage(LC_MSG_COLOR_CHANGED, GINT_TO_POINTER(x));
    gtk_widget_draw(widget, NULL);
    preview->Redraw ();
  }
  gtk_signal_emit_stop_by_name (GTK_OBJECT(widget), "key_press_event");

  return TRUE;
}

static gint colorlist_button_press(GtkWidget *widget, GdkEventButton *event)
{
  if (event->button == 1 && colorlist_pixmap != NULL)
  {
    int x = (int)(event->x * 14 / widget->allocation.width);
    if (event->y > (widget->allocation.height/2))
      x += 14;

    if (x != cur_color)
    {
      cur_color = x;
      colorlist_draw_pixmap(widget);
      lcPostMessage(LC_MSG_COLOR_CHANGED, GINT_TO_POINTER(x));
      gtk_widget_draw(widget, NULL);
      preview->Redraw ();
    }
  }
  gtk_window_set_focus(GTK_WINDOW(((GtkWidget*)(*main_window))), widget);

  return TRUE;
}

void colorlist_set(int new_color)
{
  if (new_color != cur_color)
  {
    cur_color = new_color;
    if (colorlist)
    {
      colorlist_draw_pixmap(colorlist);
      gtk_widget_draw(colorlist, NULL);
    }
    if (preview)
      preview->Redraw();
  }
}

// Create the pieces toolbar
GtkWidget* create_piecebar (GtkWidget *window, GLWindow *share)
{
  GtkWidget *vbox1, *hbox, *vpan, *scroll_win, *frame, *button, *arrow;

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);               

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (frame), vbox1);
  gtk_container_border_width(GTK_CONTAINER (vbox1), 2);

  vpan = gtk_vpaned_new ();
  gtk_widget_show (vpan);
  gtk_box_pack_start (GTK_BOX (vbox1), vpan, TRUE, TRUE, 0);

  GtkWidget *w;
  preview = new PiecePreview(share);
  preview->CreateFromWindow(&w);
  gtk_widget_set_usize (w, 100, 100);
  gtk_widget_show (w);
  gtk_container_add (GTK_CONTAINER (vpan), w);

  scroll_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scroll_win), 
  				  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_widget_show (scroll_win);
  gtk_container_add (GTK_CONTAINER (vpan), scroll_win);

  GtkTreeStore* store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  piecetree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(piecetree), false);

  GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
  GtkTreeViewColumn* column;
  column = gtk_tree_view_column_new_with_attributes("Piece", renderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(piecetree), column);

  GtkTreeSelection* select;
  select = gtk_tree_view_get_selection(GTK_TREE_VIEW(piecetree));
  gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
  g_signal_connect(G_OBJECT(select), "changed", G_CALLBACK(piecetree_changed), NULL);

  gtk_container_add(GTK_CONTAINER(scroll_win), piecetree);
  gtk_widget_show(piecetree);

  // Piece combo
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox, FALSE, TRUE, 1);

  pieceentry = gtk_entry_new ();
  gtk_widget_show (pieceentry);
  gtk_box_pack_start(GTK_BOX(hbox), pieceentry, TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT(pieceentry), "changed", GTK_SIGNAL_FUNC(piececombo_changed), NULL);
  gtk_signal_connect(GTK_OBJECT(pieceentry), "key_press_event", GTK_SIGNAL_FUNC(piececombo_key), NULL);

  button = gtk_button_new();
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (piececombo_popup), NULL);

  arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT);
  gtk_widget_show (arrow);
  gtk_container_add (GTK_CONTAINER (button), arrow);

  // Color list
  colorlist = gtk_drawing_area_new ();
  gtk_widget_set_events (colorlist, GDK_EXPOSURE_MASK
			 | GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK);
  GTK_WIDGET_SET_FLAGS(colorlist, GTK_CAN_FOCUS);
  gtk_drawing_area_size(GTK_DRAWING_AREA(colorlist), 200, 30);
  gtk_box_pack_start(GTK_BOX(vbox1), colorlist, FALSE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT(colorlist), "expose_event",
		      (GtkSignalFunc) colorlist_expose, NULL);
  gtk_signal_connect (GTK_OBJECT(colorlist),"configure_event",
		      (GtkSignalFunc) colorlist_configure, NULL);
  gtk_signal_connect (GTK_OBJECT(colorlist), "button_press_event",
		      (GtkSignalFunc) colorlist_button_press, NULL);
  gtk_signal_connect (GTK_OBJECT(colorlist), "key_press_event",
		      GTK_SIGNAL_FUNC(colorlist_key_press), NULL);

  gtk_widget_show(colorlist);

  fill_piecetree();

  return frame;
}

// =============================================================================
// Status bar

GtkWidget *label_message, *label_position, *label_snap, *label_step;

static void statusbar_listener (int message, void *data, void *user)
{
  if (message == LC_MSG_FOCUS_OBJECT_CHANGED)
  {
    char text[32];
    Vector3 pos;

    lcGetActiveProject()->GetFocusPosition(pos);
    lcGetActiveProject()->ConvertToUserUnits(pos);

    sprintf (text, "X: %.2f Y: %.2f Z: %.2f", pos[0], pos[1], pos[2]);
    gtk_label_set (GTK_LABEL (label_position), text);
  }
}

void create_statusbar(GtkWidget *window, GtkWidget *vbox)
{
  GtkWidget *hbox, *hbox1, *frame;

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_widget_set_usize (hbox, -1, 24);
  gtk_container_border_width (GTK_CONTAINER (hbox), 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 2);

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox1);
  gtk_container_border_width (GTK_CONTAINER (hbox1), 0);
  gtk_widget_show (hbox1);

  label_message = gtk_label_new (" ");
  gtk_widget_show (label_message);
  gtk_box_pack_start (GTK_BOX (hbox1), label_message, FALSE, TRUE, 0);
  gtk_label_set_justify (GTK_LABEL (label_message), GTK_JUSTIFY_LEFT);
  gtk_misc_set_padding (GTK_MISC (label_message), 3, 0);

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_widget_set_usize (frame, 150, -1);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox1);
  gtk_container_border_width (GTK_CONTAINER (hbox1), 0);
  gtk_widget_show (hbox1);

  label_position = gtk_label_new (" ");
  gtk_widget_show (label_position);
  gtk_box_pack_start (GTK_BOX (hbox1), label_position, TRUE, TRUE, 0);

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_widget_set_usize (frame, 120, -1);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox1);
  gtk_container_border_width (GTK_CONTAINER (hbox1), 0);
  gtk_widget_show (hbox1);

  label_snap = gtk_label_new (" ");
  gtk_widget_show (label_snap);
  gtk_box_pack_start (GTK_BOX (hbox1), label_snap, TRUE, TRUE, 0);

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_widget_set_usize (frame, 70, -1);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox1);
  gtk_container_border_width (GTK_CONTAINER (hbox1), 0);
  gtk_widget_show (hbox1);

  label_step = gtk_label_new (" ");
  gtk_widget_show (label_step);
  gtk_box_pack_start (GTK_BOX (hbox1), label_step, TRUE, TRUE, 0);

//  messenger->Listen (&statusbar_listener, NULL);
}
