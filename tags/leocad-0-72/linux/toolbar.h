#ifndef _TOOLBAR_H_
#define _TOOLBAR_H_

void create_toolbars(GtkWidget *window, GtkWidget *vbox);
void create_piecebar(GtkWidget *window, GtkWidget *hbox);
void create_statusbar(GtkWidget *window, GtkWidget *vbox);
void colorlist_set(int new_color);
void groupsbar_set(int new_group);
void piececombo_add(char* string);

extern GtkWidget *label_message, *label_position, *label_snap, *label_step;

typedef struct
{
  GtkWidget* toolbar;
  GtkWidget* handle_box;

  GtkWidget* cut;
  GtkWidget* copy;
  GtkWidget* paste;
  GtkWidget* undo;
  GtkWidget* redo;
  GtkWidget* snap;
  GtkWidget* angle;
  GtkWidget* fast;
  GtkWidget* bg;

} MAIN_TOOLBAR;


typedef struct
{
  GtkWidget* toolbar;
  GtkWidget* handle_box;

  GtkWidget* brick;
  GtkWidget* light;
  GtkWidget* spot;
  GtkWidget* camera;
  GtkWidget* select;
  GtkWidget* move;
  GtkWidget* rotate;
  GtkWidget* erase;
  GtkWidget* paint;
  GtkWidget* zoom;
  GtkWidget* pan;
  GtkWidget* rotview;
  GtkWidget* roll;
  GtkWidget* zoomreg;
  GtkWidget* prev;
  GtkWidget* next;

} TOOL_TOOLBAR;

typedef struct
{
  GtkWidget* toolbar;
  GtkWidget* handle_box;

  GtkWidget* first;
  GtkWidget* prev;
  GtkWidget* play;
  GtkWidget* stop;
  GtkWidget* next;
  GtkWidget* last;
  GtkWidget* anim;
  GtkWidget* keys;

} ANIM_TOOLBAR;

extern ANIM_TOOLBAR anim_toolbar;
extern TOOL_TOOLBAR tool_toolbar;
extern MAIN_TOOLBAR main_toolbar;

#endif


