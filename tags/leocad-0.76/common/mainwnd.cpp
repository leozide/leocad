//
// Main LeoCAD window
//

#include <stdio.h>
#include "mainwnd.h"
#include "system.h"

MainWnd::MainWnd ()
  : BaseWnd (NULL, LC_MAINWND_NUM_COMMANDS)
{
  char entry[8];
  int i;

  for (i = 0; i < LC_MRU_MAX; i++)
  {
    sprintf (entry, "File%d", i+1);
    m_strMRU[i] = Sys_ProfileLoadString ("RecentFiles", entry, "");
  }
}

MainWnd::~MainWnd ()
{
  char entry[8];
  int i;

  for (i = 0; i < LC_MRU_MAX; i++)
  {
    sprintf (entry, "File%d", i+1);
    Sys_ProfileSaveString ("RecentFiles", entry, m_strMRU[i]);
  }
}

// =============================================================================
// recently used files

void MainWnd::UpdateMRU ()
{
#ifdef LC_WINDOWS
  // FIXME !!
  void SystemUpdateRecentMenu(char names[4][LC_MAXPATH]);
  char names[4][LC_MAXPATH];

  for (int i = 0; i < LC_MRU_MAX; i++)
    strcpy (names[i], m_strMRU[i]);

  SystemUpdateRecentMenu(names);
#else
  for (int i = 0; i < LC_MRU_MAX; i++)
  {
    if (m_strMRU[i].IsEmpty ())
    {
      if (i == 0)
      {
        SetMenuItemText (LC_MAINWND_RECENT1, "Recent Files");
        EnableMenuItem (LC_MAINWND_RECENT1, false);
      }
      else
        ShowMenuItem (LC_MAINWND_RECENT1+i, false);
    }
    else
    {
      char text[LC_MAXPATH+8];
      sprintf (text, "&%d- %s", i+1, (char*)m_strMRU[i]);

      ShowMenuItem (LC_MAINWND_RECENT1+i, true);
      EnableMenuItem (LC_MAINWND_RECENT1+i, true);
      SetMenuItemText (LC_MAINWND_RECENT1+i, text);
    }
  }
#endif
}

void MainWnd::AddToMRU(const char* Filename)
{
	// Make a copy of the string in case we're loading a file from the MRU menu.
	String str = Filename;
	int i;

	// Search for Filename in the MRU list.
	for (i = 0; i < (LC_MRU_MAX - 1); i++)
		if (m_strMRU[i] == Filename)
			break;

	// Move MRU strings before this one down.
	for (; i > 0; i--)
		m_strMRU[i] = m_strMRU[i-1];

	m_strMRU[0] = str;

	UpdateMRU();
}

void MainWnd::RemoveFromMRU(int index)
{
  for (int i = index; i < (LC_MRU_MAX - 1); i++)
    m_strMRU[i] = m_strMRU[i+1];
  m_strMRU[LC_MRU_MAX - 1].Empty ();

  UpdateMRU ();
}
