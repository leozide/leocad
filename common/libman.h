#ifndef _LIBMAN_H_
#define _LIBMAN_H_

#include "defines.h"
#include "config.h"
#include "str.h"
#include "library.h"

class PieceInfo;

typedef enum
{
	LC_LIBDLG_FILE_RESET,
	LC_LIBDLG_FILE_OPEN,
	LC_LIBDLG_FILE_SAVE,
	LC_LIBDLG_FILE_SAVEAS,
	LC_LIBDLG_FILE_MERGEUPDATE,
	LC_LIBDLG_FILE_IMPORTPIECE,
	LC_LIBDLG_FILE_RETURN,
	LC_LIBDLG_FILE_CANCEL,
	LC_LIBDLG_GROUP_INSERT,
	LC_LIBDLG_GROUP_DELETE,
	LC_LIBDLG_GROUP_EDIT,
	LC_LIBDLG_GROUP_MOVEUP,
	LC_LIBDLG_GROUP_MOVEDOWN
} LC_LIBDLG_COMMANDS;

class LibraryManager
{
public:
	LibraryManager ();
	virtual ~LibraryManager ();

	void HandleCommand (int id, int param);
	bool Initialize ();
	bool DoSave (bool bAskName);
	bool SaveModified ();

	// Commands
	bool DeletePieces (char** Names, int Count);

	// Access
	int GetGroupCount () const
		{ return m_nGroups; }
	String& GetGroupName (int index)
		{ return m_strGroups[index]; }

	int GetPieceCount () const
		{ return m_nPieces; }
	void GetPieceInfo (int index, PieceInfo** Info, lcuint32* Group)
		{ *Info = m_pPieces[index].Info; *Group = m_pPieces[index].CurrentGroups; }
	void SetPieceGroup (int Index, int Group, bool Add)
		{
			if (Add)
				m_pPieces[Index].CurrentGroups = Group;
			else
				m_pPieces[Index].CurrentGroups |= Group;

			m_bModified = true;
		}

protected:
	void LoadDefaults ();

	typedef struct
	{
		PieceInfo* Info;
		lcuint32 CurrentGroups;
	} LC_LIBDLG_PIECEINFO;

	int m_nPieces;
	LC_LIBDLG_PIECEINFO* m_pPieces;

	int m_nGroups;
	String m_strGroups[LC_PIECESLIB_MAXGROUPS];

	bool m_bModified;
	char m_strFile[LC_MAXPATH];
};

#endif // _LIBMAN_H_
