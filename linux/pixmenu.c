/* Author: Dietmar Maurer <dm@vlsivie.tuwien.ac.at> */

#include "pixmenu.h"
#include <gtk/gtkaccellabel.h>
#include <gtk/gtksignal.h>

static void gtk_pixmap_menu_item_class_init    (GtkPixmapMenuItemClass *klass);
static void gtk_pixmap_menu_item_init          (GtkPixmapMenuItem      *menu_item);
static void gtk_pixmap_menu_item_draw          (GtkWidget              *widget,
					        GdkRectangle           *area);
static gint gtk_pixmap_menu_item_expose        (GtkWidget              *widget,
					        GdkEventExpose         *event);

/* we must override the following functions */

static void gtk_pixmap_menu_item_map           (GtkWidget        *widget);
static void gtk_pixmap_menu_item_size_allocate (GtkWidget        *widget,
						GtkAllocation    *allocation);
static void gtk_pixmap_menu_item_forall        (GtkContainer    *container,
						gboolean         include_internals,
						GtkCallback      callback,
						gpointer         callback_data);
static void gtk_pixmap_menu_item_size_request  (GtkWidget        *widget,
						GtkRequisition   *requisition);
static void gtk_pixmap_menu_item_remove        (GtkContainer *container,
						GtkWidget    *child);

					       
static GtkMenuItemClass *parent_class = NULL;

#define INDENT 18

GtkType
gtk_pixmap_menu_item_get_type (void)
{
  static GtkType pixmap_menu_item_type = 0;

  if (!pixmap_menu_item_type)
    {
      GtkTypeInfo pixmap_menu_item_info =
      {
        "GtkPixmapMenuItem",
        sizeof (GtkPixmapMenuItem),
        sizeof (GtkPixmapMenuItemClass),
        (GtkClassInitFunc) gtk_pixmap_menu_item_class_init,
        (GtkObjectInitFunc) gtk_pixmap_menu_item_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      pixmap_menu_item_type = gtk_type_unique (gtk_menu_item_get_type (), 
					       &pixmap_menu_item_info);
    }

  return pixmap_menu_item_type;
}

/**
 * gtk_pixmap_menu_item_new
 *
 * Creates a new pixmap menu item. Use gtk_pixmap_menu_item_set_pixmap() 
 * to set the pixmap wich is displayed at the left side.
 *
 * Returns:
 * &GtkWidget pointer to new menu item
 **/

GtkWidget*
gtk_pixmap_menu_item_new (void)
{
  return GTK_WIDGET (gtk_type_new (gtk_pixmap_menu_item_get_type ()));
}

static void
gtk_pixmap_menu_item_class_init (GtkPixmapMenuItemClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkMenuItemClass *menu_item_class;
  GtkContainerClass *container_class;

  object_class = (GtkObjectClass*) klass;
  widget_class = (GtkWidgetClass*) klass;
  menu_item_class = (GtkMenuItemClass*) klass;
  container_class = (GtkContainerClass*) klass;

  parent_class = gtk_type_class (gtk_menu_item_get_type ());

  widget_class->draw = gtk_pixmap_menu_item_draw;
  widget_class->expose_event = gtk_pixmap_menu_item_expose;
  widget_class->map = gtk_pixmap_menu_item_map;
  widget_class->size_allocate = gtk_pixmap_menu_item_size_allocate;
  widget_class->size_request = gtk_pixmap_menu_item_size_request;

  container_class->forall = gtk_pixmap_menu_item_forall;
  container_class->remove = gtk_pixmap_menu_item_remove;
  menu_item_class->toggle_size = INDENT;
}

static void
gtk_pixmap_menu_item_init (GtkPixmapMenuItem *menu_item)
{
  menu_item->pixmap = NULL;
}

static void
gtk_pixmap_menu_item_draw (GtkWidget    *widget,
			   GdkRectangle *area)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_PIXMAP_MENU_ITEM (widget));
  g_return_if_fail (area != NULL);

  if (GTK_WIDGET_CLASS (parent_class)->draw)
    (* GTK_WIDGET_CLASS (parent_class)->draw) (widget, area);

  if (GTK_WIDGET_DRAWABLE (widget) && 
      GTK_PIXMAP_MENU_ITEM(widget)->pixmap) {
    gtk_widget_draw(GTK_WIDGET(GTK_PIXMAP_MENU_ITEM(widget)->pixmap),NULL);
  }
}

static gint
gtk_pixmap_menu_item_expose (GtkWidget      *widget,
			     GdkEventExpose *event)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_PIXMAP_MENU_ITEM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (GTK_WIDGET_CLASS (parent_class)->expose_event)
    (* GTK_WIDGET_CLASS (parent_class)->expose_event) (widget, event);

  if (GTK_WIDGET_DRAWABLE (widget) && 
      GTK_PIXMAP_MENU_ITEM(widget)->pixmap) {
    gtk_widget_draw(GTK_WIDGET(GTK_PIXMAP_MENU_ITEM(widget)->pixmap),NULL);
  }

  return FALSE;
}

/**
 * gtk_pixmap_menu_item_set_pixmap
 * @menu_item: Pointer to the pixmap menu item
 * @pixmap: Pointer to a pixmap widget
 *
 * Set the pixmap of the menu item.
 *
 **/

void
gtk_pixmap_menu_item_set_pixmap (GtkPixmapMenuItem *menu_item,
				 GtkWidget         *pixmap)
{
  g_return_if_fail (menu_item != NULL);
  g_return_if_fail (pixmap != NULL);
  g_return_if_fail (GTK_IS_PIXMAP_MENU_ITEM (menu_item));
  g_return_if_fail (GTK_IS_WIDGET (pixmap));
  g_return_if_fail (menu_item->pixmap == NULL);

  gtk_widget_set_parent (pixmap, GTK_WIDGET (menu_item));
  menu_item->pixmap = pixmap;

  if (GTK_WIDGET_VISIBLE (pixmap->parent)) {
    if (GTK_WIDGET_REALIZED (pixmap->parent) &&
	!GTK_WIDGET_REALIZED (pixmap))
      gtk_widget_realize (pixmap);
      
    if (GTK_WIDGET_MAPPED (pixmap->parent) &&
	!GTK_WIDGET_MAPPED (pixmap))
      gtk_widget_map (pixmap);
  }
  
  if (GTK_WIDGET_VISIBLE (pixmap) && GTK_WIDGET_VISIBLE (menu_item))
    gtk_widget_queue_resize (pixmap);

}

static void
gtk_pixmap_menu_item_map (GtkWidget *widget)
{
  GtkPixmapMenuItem *menu_item;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_PIXMAP_MENU_ITEM (widget));

  menu_item = GTK_PIXMAP_MENU_ITEM(widget);

  GTK_WIDGET_CLASS(parent_class)->map(widget);

  if (menu_item->pixmap &&
      GTK_WIDGET_VISIBLE (menu_item->pixmap) &&
      !GTK_WIDGET_MAPPED (menu_item->pixmap))
    gtk_widget_map (menu_item->pixmap);
}

static void
gtk_pixmap_menu_item_size_allocate (GtkWidget        *widget,
				    GtkAllocation    *allocation)
{
  GtkPixmapMenuItem *menu_item;
  GtkAllocation child_allocation;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_PIXMAP_MENU_ITEM (widget));
  g_return_if_fail (allocation != NULL);

  menu_item = GTK_PIXMAP_MENU_ITEM(widget);

  if (menu_item->pixmap) {
    child_allocation.x = GTK_CONTAINER (widget)->border_width+
      widget->style->klass->xthickness + 3;
    child_allocation.y = GTK_CONTAINER (widget)->border_width+4;
    child_allocation.width = INDENT-child_allocation.x;
    child_allocation.height = allocation->height-2*child_allocation.y;
    gtk_widget_size_allocate (menu_item->pixmap, &child_allocation);
  }  

  GTK_WIDGET_CLASS(parent_class)->size_allocate(widget,allocation);
}

static void
gtk_pixmap_menu_item_forall (GtkContainer    *container,
			     gboolean         include_internals,
			     GtkCallback      callback,
			     gpointer         callback_data)
{
  GtkPixmapMenuItem *menu_item;

  g_return_if_fail (container != NULL);
  g_return_if_fail (GTK_IS_PIXMAP_MENU_ITEM (container));
  g_return_if_fail (callback != NULL);

  menu_item = GTK_PIXMAP_MENU_ITEM (container);

  if (menu_item->pixmap)
    (* callback) (menu_item->pixmap, callback_data);

  GTK_CONTAINER_CLASS(parent_class)->forall(container,include_internals,
					    callback,callback_data);
}

static void
gtk_pixmap_menu_item_size_request (GtkWidget      *widget,
				   GtkRequisition *requisition)
{
  GtkPixmapMenuItem *menu_item;
  GtkRequisition req;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MENU_ITEM (widget));
  g_return_if_fail (requisition != NULL);

  GTK_WIDGET_CLASS(parent_class)->size_request(widget,requisition);

  menu_item = GTK_PIXMAP_MENU_ITEM (widget);
  
  /* some widgets needs this */
  if (menu_item->pixmap)
    gtk_widget_size_request(menu_item->pixmap,&req);
    
}

static void
gtk_pixmap_menu_item_remove (GtkContainer *container,
			     GtkWidget    *child)
{
  GtkBin *bin;
  gboolean widget_was_visible;

  g_return_if_fail (container != NULL);
  g_return_if_fail (GTK_IS_PIXMAP_MENU_ITEM (container));
  g_return_if_fail (child != NULL);
  g_return_if_fail (GTK_IS_WIDGET (child));

  bin = GTK_BIN (container);
  g_return_if_fail ((bin->child == child || 
		     (GTK_PIXMAP_MENU_ITEM(container)->pixmap == child)));

  widget_was_visible = GTK_WIDGET_VISIBLE (child);
  
  gtk_widget_unparent (child);
  if (bin->child == child)
    bin->child = NULL; 
  else 
    GTK_PIXMAP_MENU_ITEM(container)->pixmap = NULL;
  
  if (widget_was_visible)
    gtk_widget_queue_resize (GTK_WIDGET (container));
}
