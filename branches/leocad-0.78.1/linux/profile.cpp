//
// Functions to read/write user preferences
// Everything is saved in the file ~/.leocad
//

#include "lc_global.h"
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "defines.h"
#include "lc_file.h"

// =============================================================================
// Static functions

static bool read_var (const char *section, const char *key, char *value)
{
  char line[1024], *ptr, filename[LC_MAXPATH];
  FILE *rc;

  sprintf (filename, "%s/.leocad", g_get_home_dir ());
  rc = fopen (filename, "rt");

  if (rc == NULL)
    return false;

  while (fgets (line, 1024, rc) != 0)
  {
    // First we find the section
    if (line[0] != '[')
      continue;

    ptr = strchr (line, ']');
    *ptr = '\0';

    if (strcmp (&line[1], section) == 0)
    {
      while (fgets (line, 1024, rc) != 0)
      {
        ptr = strchr (line, '=');

        if (ptr == NULL)
	{
	  // reached the end of the section
	  fclose (rc);
	  return false;
	}
        *ptr = '\0';

        if (strcmp (line, key) == 0)
        {
          strcpy (value, ptr+1);
	  fclose (rc);

	  while (value[strlen (value)-1] == 10 || 
		 value[strlen (value)-1] == 13 ||
		 value[strlen (value)-1] == 32)
	    value[strlen (value)-1] = 0; 
	  return true;
        }
      }
    }
  }

  fclose (rc);
  return false;
}

static bool save_var (const char *section, const char *key, const char *value)
{
  char line[1024], *ptr, filename[LC_MAXPATH];
  lcMemFile old_rc;
  bool found;
  FILE *rc;

  sprintf (filename, "%s/.leocad", g_get_home_dir ());
  rc = fopen (filename, "rb");

  if (rc != NULL)
  {
    guint32 len;
    void *buf;

    fseek (rc, 0, SEEK_END);
    len = ftell (rc);
    rewind (rc);
    buf = g_malloc (len);
    fread (buf, len, 1, rc);
    old_rc.WriteBuffer (buf, len);
    g_free (buf);
    fclose (rc);
    old_rc.Seek (0, SEEK_SET);
  }

  rc = fopen (filename, "wt");
  if (rc == NULL)
    return false;

  // First we need to find the section
  found = false;
  while (old_rc.ReadLine(line, 1024) != NULL)
  {
    fputs (line, rc);

    if (line[0] == '[')
    {
      ptr = strchr (line, ']');
      *ptr = '\0';

      if (strcmp (&line[1], section) == 0)
      {
	found = true;
	break;
      }
    }
  } 

  if (!found)
  {
    fputs ("\n", rc);
    fprintf (rc, "[%s]\n", section);
  }

  found = false;
  while (old_rc.ReadLine(line, 1024) != NULL)
  {
    ptr = strchr (line, '=');

    if (ptr != NULL)
    {
      *ptr = '\0';

      if (strcmp (line, key) == 0)
      {
	fprintf (rc, "%s=%s\n", key, value);
	found = true;
	break;
      }
      else
      {
	*ptr = '=';
	fputs (line, rc);
      }
    }
    else
      break; // reached end of section
  }

  if (!found)
  {
    fprintf (rc, "%s=%s\n", key, value);
    fputs ("\n", rc);
  }

  while (old_rc.ReadLine(line, 1024) != NULL)
    fputs (line, rc);

  fclose (rc);
  return true;
}

// =============================================================================
// Global functions

bool Sys_ProfileSaveInt (const char *section, const char *key, int value)
{
  char buf[16];
  sprintf (buf, "%d", value);
  return save_var (section, key, buf);
}

bool Sys_ProfileSaveString (const char *section, const char *key, const char *value)
{
  return save_var (section, key, value);
}

int Sys_ProfileLoadInt (const char *section, const char *key, int default_value)
{
  char value[1024];

  if (read_var (section, key, value))
    return atoi (value);
  else
    return default_value;
}

char* Sys_ProfileLoadString (const char *section, const char *key, const char *default_value)
{
  static char value[1024];

  if (!read_var (section, key, value))
    strcpy (value, default_value);

  return value;
}
