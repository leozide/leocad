#ifndef _LIBRARY_H_
#define _LIBRARY_H_

typedef enum {
  LC_LIBDLG_FILE_RESET,
  LC_LIBDLG_FILE_OPEN,
  LC_LIBDLG_FILE_SAVE,
  LC_LIBDLG_FILE_SAVEAS,
  LC_LIBDLG_FILE_PRINTCATALOG,
  LC_LIBDLG_FILE_MERGEUPDATE,
  LC_LIBDLG_FILE_IMPORTPIECE,
  LC_LIBDLG_FILE_RETURN,
  LC_LIBDLG_FILE_CANCEL,
  LC_LIBDLG_GROUP_INSERT,
  LC_LIBDLG_GROUP_DELETE,
  LC_LIBDLG_GROUP_EDIT,
  LC_LIBDLG_GROUP_MOVEUP,
  LC_LIBDLG_GROUP_MOVEDOWN,
  LC_LIBDLG_PIECE_NEW,
  LC_LIBDLG_PIECE_EDIT,
  LC_LIBDLG_PIECE_DELETE
} LC_LIBDLG_COMMANDS;

class PieceInfo;
class LibraryDialog;

typedef struct
{
  PieceInfo* info;
  unsigned long current_groups;
  unsigned long default_groups;
} LC_LIBDLG_PIECEINFO;

#define LC_LIBDLG_MAXGROUPS 32
#define LC_LIBDLG_MAXNAME   32

typedef void (*PFNLIBDLGUPDATELISTFUNC) (LC_LIBDLG_PIECEINFO *piece_info, int count, int group, void *data);
typedef void (*PFNLIBDLGUPDATETREEFUNC) (int num_groups, char str_groups[][LC_LIBDLG_MAXNAME+1], void *data);

class LibraryDialog
{
 public:
  LibraryDialog ();
  virtual ~LibraryDialog ();

  void HandleCommand (int id);
  bool DoSave (bool bAskName);
  bool Initialize ();

  void SetListFunc (PFNLIBDLGUPDATELISTFUNC func, void *data)
    {
      m_pUpdateListFunc = func;
      m_pUpdateListData = data;
    }

  void SetTreeFunc (PFNLIBDLGUPDATETREEFUNC func, void *data)
    {
      m_pUpdateTreeFunc = func;
      m_pUpdateTreeData = data;
    }

  void UpdateList ()
    {
      m_pUpdateListFunc (m_pPieces, m_nPieces, m_nCurrentGroup, m_pUpdateListData);
    }

  void UpdateTree ()
    {
      m_pUpdateTreeFunc (m_nGroups, m_strGroups, m_pUpdateTreeData);
    }

  void SetCurrentGroup (unsigned long group)
    {
      m_nCurrentGroup = group;
      UpdateList ();
    }

 protected:
  void *m_pUpdateListData;
  void *m_pUpdateTreeData;
  PFNLIBDLGUPDATELISTFUNC m_pUpdateListFunc;
  PFNLIBDLGUPDATETREEFUNC m_pUpdateTreeFunc;

  int m_nPieces;
  LC_LIBDLG_PIECEINFO* m_pPieces;

  unsigned char m_nGroups;
  char m_strGroups[LC_LIBDLG_MAXGROUPS][LC_LIBDLG_MAXNAME+1];
  int m_nCurrentGroup;

  bool m_bModified;
  bool m_bReload;
  char m_strFile[LC_MAXPATH];
};

typedef struct connection_s
{
	unsigned char type;
	float pos[3];
	float up[3];
	connection_s* next;
} connection_t;

typedef struct group_s
{
	connection_t* connections[5];
	void* drawinfo;
	unsigned long infosize;
	group_s* next;
} group_t;

typedef struct lineinfo_s
{
	unsigned char type;
	unsigned char color;
	float points[12];
	lineinfo_s* next;
} lineinfo_t;

typedef struct texture_s
{
	float points[20];
	unsigned char color;
	char name[9];
	texture_s* next;
} texture_t;

typedef struct
{
	float* verts;
	unsigned int verts_count;
	connection_t* connections;
	group_t* groups;
	texture_t* textures;
	char name[9];
	char description[65];
} LC_LDRAW_PIECE;

bool ReadLDrawPiece(const char* filename, LC_LDRAW_PIECE* piece);
bool SaveLDrawPiece(LC_LDRAW_PIECE* piece);
void FreeLDrawPiece(LC_LDRAW_PIECE* piece);
bool DeletePiece(char** names, int numpieces);
bool LoadUpdate(const char* update);

#endif // _LIBRARY_H_
