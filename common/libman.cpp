//
// Handle all commands from the Pieces Library Manager
//

#include <string.h>
#include "libman.h"
#include "library.h"
#include "pieceinf.h"
#include "globals.h"
#include "project.h"
#include "system.h"

LibraryManager::LibraryManager ()
{
  m_nPieces = 0;
  m_pPieces = NULL;
  m_nGroups = 0;
  m_bModified = false;
  strcpy (m_strFile, "");

	Initialize ();
}

LibraryManager::~LibraryManager ()
{
	free (m_pPieces);
}

bool LibraryManager::Initialize ()
{
  PiecesLibrary *pLib = project->GetPiecesLibrary ();
	int i;

  m_nPieces = pLib->GetPieceCount();
  m_pPieces = (LC_LIBDLG_PIECEINFO*) malloc (sizeof (LC_LIBDLG_PIECEINFO) * m_nPieces);

  for (i = 0; i < m_nPieces; i++)
  {
		m_pPieces[i].Info = pLib->GetPieceInfo (i);
    m_pPieces[i].CurrentGroups = m_pPieces[i].Info->m_nGroups;
  }

  m_nGroups = pLib->GetGroupCount();
	for (i = 0; i < m_nGroups; i++)
		m_strGroups[i] = pLib->GetGroup(i);

  return true;
}

void LibraryManager::LoadDefaults ()
{
  strcpy (m_strFile, "");

  strcpy (m_strGroups[0], "Plates");
  strcpy (m_strGroups[1], "Bricks");
  strcpy (m_strGroups[2], "Tiles");
  strcpy (m_strGroups[3], "Slope Bricks");
  strcpy (m_strGroups[4], "Technic");
  strcpy (m_strGroups[5], "Space");
  strcpy (m_strGroups[6], "Train");
  strcpy (m_strGroups[7], "Other Bricks");
  strcpy (m_strGroups[8], "Accessories");
  m_nGroups = 9;

	for (int i = 0; i < m_nPieces; i++)
		m_pPieces[i].CurrentGroups = PiecesLibrary::GetDefaultPieceGroup (m_pPieces[i].Info->m_strName);
}

void LibraryManager::HandleCommand (int id, int param)
{
	switch (id)
	{
		case LC_LIBDLG_FILE_RESET:
		{
			LoadDefaults ();
			m_bModified = true;
		} break;

		case LC_LIBDLG_FILE_OPEN:
			break;

		case LC_LIBDLG_FILE_SAVE:
			break;

		case LC_LIBDLG_FILE_SAVEAS:
			break;

		case LC_LIBDLG_FILE_MERGEUPDATE:
		{
			char filename[LC_MAXPATH];

			// FIXME: file extension
			if (!SystemDoDialog (LC_DLG_FILE_OPEN, filename))
				return;

			project->GetPiecesLibrary ()->LoadUpdate (filename);

			// FIXME: update m_pPieces
		} break;

		case LC_LIBDLG_FILE_IMPORTPIECE:
		{
/*
      char filename[LC_MAXPATH];
      LC_LDRAW_PIECE piece;

      //      CFileDialog dlg(TRUE, ".dat\0", NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
      //		      "LDraw Files (*.dat)|*.dat|All Files (*.*)|*.*||",this);
      if (!SystemDoDialog (LC_DLG_FILE_OPEN, filename))
	return;

      BeginWait ();

      if (ReadLDrawPiece(filename, &piece))
      {
	if (project->GetPiecesLibrary ()->FindPieceInfo(piece.name) != NULL)
	  Sys_MessageBox ("Piece already exists in the library !");

	if (SaveLDrawPiece(&piece))
	  Sys_MessageBox ("Piece successfully imported.");
	else
	  Sys_MessageBox ("Error saving library.");
      }
      else
	Sys_MessageBox ("Error reading file.");

      EndWait ();
      FreeLDrawPiece(&piece);
			// FIXME: update m_pPieces
*/
    } break;

		case LC_LIBDLG_FILE_RETURN:
			break;

		case LC_LIBDLG_FILE_CANCEL:
			break;

		case LC_LIBDLG_GROUP_INSERT:
			break;

		case LC_LIBDLG_GROUP_DELETE:
		{
			int i, j;

			if (SystemDoMessageBox ("Are you sure you want to delete this group ?", LC_MB_YESNO|LC_MB_ICONQUESTION) != LC_YES)
				return;

			for (i = param; i < (m_nGroups - 1); i++)
				m_strGroups[i] = m_strGroups[i+1];

			for (j = 0; j < m_nPieces; j++)
			{
				lcuint32 grp = m_pPieces[j].CurrentGroups;

				// Remove from the group
				if (grp & (1 << param))
					grp &= ~(1 << param);

				// Shift other groups
				for (i = param+1; i < m_nGroups; i++)
				{
					lcuint32 d = (1 << i);

					if (grp & d)
					{
						grp &= ~d;
						grp |= (1 << (i-1));
					}
				}
				m_pPieces[j].CurrentGroups = grp;
			}

			m_bModified = true;
			m_nGroups--;
		} break;

		case LC_LIBDLG_GROUP_EDIT:
			break;

		case LC_LIBDLG_GROUP_MOVEUP:
		{
			String tmp;
			int j;

			if (param < 1)
				return;

			tmp = m_strGroups[param];
			m_strGroups[param] = m_strGroups[param-1];
			m_strGroups[param-1] = tmp;

			for (j = 0; j < m_nPieces; j++)
			{
				lcuint32 grp = m_pPieces[j].CurrentGroups;
				bool g1 = (grp & (1 << param)) != 0;
				bool g2 = (grp & (1 << (param-1))) != 0;

				if (g1)
					grp |= (1 << (param-1));
				else
					grp &= ~(1 << (param-1));

				if (g2)
					grp |= (1 << param);
				else
					grp &= ~(1 << param);

				m_pPieces[j].CurrentGroups = grp;
			}

			m_bModified = true;
		} break;

		case LC_LIBDLG_GROUP_MOVEDOWN:
		{
			String tmp;
			int j;

			if (param < 1)
				return;

			tmp = m_strGroups[param];
			m_strGroups[param] = m_strGroups[param+1];
			m_strGroups[param+1] = tmp;

			for (j = 0; j < m_nPieces; j++)
			{
				lcuint32 grp = m_pPieces[j].CurrentGroups;
				bool g1 = (grp & (1 << param)) != 0;
				bool g2 = (grp & (1 << (param+1))) != 0;

				if (g1)
					grp |= (1 << (param+1));
				else
					grp &= ~(1 << (param+1));

				if (g2)
					grp |= (1 << param);
				else
					grp &= ~(1 << param);

				m_pPieces[j].CurrentGroups = grp;
			}

			m_bModified = true;
		} break;
	}
}

bool LibraryManager::DeletePieces (char** Names, int Count)
{
	if (SystemDoMessageBox ("Are you sure you want to permanently delete the selected pieces?", LC_MB_YESNO|LC_MB_ICONQUESTION) != LC_YES)
		return false;

	project->GetPiecesLibrary ()->DeletePieces (Names, Count);

	// Delete pieces from our list
	for (int i = 0; i < Count; i++)
	{
		for (int j = 0; j < m_nPieces; j++)
		{
			if (!strcmp (Names[i], m_pPieces[j].Info->m_strName))
			{
				for (int k = j; k < m_nPieces; k++)
				{
					m_pPieces[k].Info = m_pPieces[k+1].Info;
					m_pPieces[k].CurrentGroups = m_pPieces[k+1].CurrentGroups;
				}

				m_nPieces--;
				break;
			}
		}
	}

	return true;
}

// Returns true if it's ok to continue.
bool LibraryManager::SaveModified ()
{
	if (m_bModified)
	{
		switch (SystemDoMessageBox ("Save changes ?", LC_MB_YESNOCANCEL|LC_MB_ICONQUESTION))
		{
		case LC_CANCEL:
			return false;       // don't continue
			break;

		case LC_YES:
			// If so, either Save or Update, as appropriate
			if (!DoSave (false))
				return false;     // don't continue
			break;

		case LC_NO:
			// Not saving changes
			return true;
			break;
		}

		// Apply changes
		Sys_ProfileSaveString ("Settings", "Groups", m_strFile);
		project->GetPiecesLibrary ()->LoadGroupConfig (m_strFile);
	}

	project->GetPiecesLibrary ()->CheckReload ();

	return true;    // keep going
}

bool LibraryManager::DoSave (bool bAskName)
{
	/*
  if (bAskName || (strlen (m_strFile) == 0))
  {
    char filename[LC_MAXPATH];

    //    CFileDialog dlg(FALSE, ".lgf\0", NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
    //		    "LeoCAD Group Files (*.lgf)|*.lgf|All Files (*.*)|*.*||",this);
// FIXME: file extension
    if (!SystemDoDialog (LC_DLG_FILE_SAVE, filename))
      return false;
    strcpy (m_strFile, filename);
  }
*/
  /*
  Sys_BeginWait ();
  FileDisk f;
  int i;

  if (!f.Open (m_strFile, "wb"))
    return false;

  f.Write (ver_str, sizeof (ver_str));
  f.Write (ver_flt, sizeof (ver_flt));
  f.WriteByte (&m_nMaxGroups);

  for (i = 0; i < m_nMaxGroups; i++)
  {
    f.Write (m_strGroups[i], sizeof(m_strGroups[i]));
    ar << m_nBitmaps[i];
  }

	m_ImageList.Write(&ar);

	ar << (int) m_Parts.GetSize();
	for (i = 0; i < m_Parts.GetSize(); i++)
	{
		ar.Write (m_Parts[i].info->m_strName, sizeof(m_Parts[i].info->m_strName));
		ar << m_Parts[i].group;
	}
	ar.Close();
	f.Close();
	m_bModified = FALSE;
//		m_bLoaded = TRUE;

	fclose (f);
	Sys_EndWait ();
  */
  return true;
}
