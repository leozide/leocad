// User preferences.
//

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "custom.h"

// Variables

userrc_t user_rc;


// Functions

static void create_rc (void);
static void save_rc (void);
static void read_rc (void);
static char* preferences_path (char *buf);


static char* preferences_path (char *buf)
{
  char *ptr;

  if ((ptr = getenv ("HOME")) == NULL)
  {
     printf ("Can't get Environment Variable HOME... Aborting.\n");
     exit (1);
  }

  sprintf (buf, "%s/.LeoCAD", ptr);
  return buf;
}

// Set the default preferences.
static void create_rc (void)
{
  memset (&user_rc, 0, sizeof (user_rc));

  user_rc.view_main_toolbar = 1;
  user_rc.view_tool_toolbar = 1;
  user_rc.view_anim_toolbar = 1;
  user_rc.toolbar_style = GTK_TOOLBAR_ICONS;
}

static void save_rc (void)
{
  FILE *preferences_file;
  char this_path[1024];

  preferences_path (this_path);
  if ((preferences_file = fopen (this_path, "w")) == NULL)
  {
//    alert_ok ("Error", "Can't save preferences ini file.", "Ok");
    return;
  }
  fprintf (preferences_file, "# LeoCAD preferences file.\n\n");

#define CUSTOM_SECTION(a)  fprintf (preferences_file, "\n\n[%s]\n", a)
#define CUSTOM_SAVE_INT(a,b)  fprintf (preferences_file, "%s=%i\n", a, (int) b)
#define CUSTOM_SAVE_BOOL(a,b)  fprintf (preferences_file, "%s=%i\n", a, b ? 1 : 0)
#define CUSTOM_SAVE_STRING(a,b)  fprintf (preferences_file, "%s=%s\n", a, b)

  // VERSION
//  CUSTOM_SECTION ("VERSION");
//  CUSTOM_SAVE_STRING ("PROGRAM_VERSION", user_rc.program_version);

  // VIEW
  CUSTOM_SECTION ("View");
  CUSTOM_SAVE_BOOL ("Standard", user_rc.view_main_toolbar);
  CUSTOM_SAVE_BOOL ("Tools", user_rc.view_tool_toolbar);
  CUSTOM_SAVE_BOOL ("Animation", user_rc.view_anim_toolbar);
  CUSTOM_SAVE_INT ("TOOLBAR_STYLE", user_rc.toolbar_style);

  fprintf (preferences_file, "\n\n# END OF LeoCAD preferences file.");
  fflush (preferences_file);
  fclose (preferences_file);
}

static void read_rc (void)
{
  FILE *preferences_file;
  char this_path[1024];
  char this_line[256];
  int this_line_number = 0;

  preferences_path (this_path);

  if (access (this_path, F_OK))
    return;

  if ((preferences_file = fopen (this_path, "r")) == NULL)
    return;

#define FIELD_INT (int) atoi(strchr(this_line,'=')+1)
#define FIELD_BOOL (FIELD_INT!=0)
#define FIELD_STRING strchr(this_line,'=')+1

#define CUSTOM_LOAD_INT(a,b);  if(strstr (this_line, a"=")){b = FIELD_INT;continue;}
#define CUSTOM_LOAD_BOOL(a,b);  if(strstr (this_line, a"=")){b = FIELD_BOOL;continue;}
#define CUSTOM_LOAD_STRING(a,b);  if(strstr (this_line, a"=")){strcpy(b, FIELD_STRING);continue;}

  while (fgets (this_line, sizeof (this_line), preferences_file) != NULL)
  {
    this_line_number++;

    if (this_line[strlen (this_line) - 1] == '\n')
      this_line[strlen (this_line) - 1] = 0;

    if (!strlen (this_line))
      continue;
    if (this_line[0] == '#')
      continue;
    if (this_line[0] == '[')
      continue;

    // VERSION
//    CUSTOM_LOAD_STRING ("PROGRAM_VERSION", user_rc.program_version);

    // VIEW
    CUSTOM_LOAD_BOOL ("Standard", user_rc.view_main_toolbar);
    CUSTOM_LOAD_BOOL ("Tools", user_rc.view_tool_toolbar);
    CUSTOM_LOAD_BOOL ("Animation", user_rc.view_anim_toolbar);
    CUSTOM_LOAD_INT ("TOOLBAR_STYLE", (int) user_rc.toolbar_style);

    printf ("LeoCAD : Syntax error in %s\n", this_path);
    printf ("Line %i : %s\n", this_line_number, this_line);
  }

  fclose (preferences_file);
}

void init_rc (void)
{
  char this_path[1024];

  preferences_path (this_path);

  create_rc ();
  access (this_path, F_OK) ? save_rc () : read_rc ();
}

