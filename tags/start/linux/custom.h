#ifndef _CUSTOM_H_
#define _CUSTOM_H_

#include <gtk/gtk.h>

// User preferences
typedef struct
{
  char view_main_toolbar;
  char view_tool_toolbar;
  char view_anim_toolbar;

  GtkToolbarStyle toolbar_style;
} userrc_t;

extern userrc_t user_rc;


void init_rc (void);

#endif
