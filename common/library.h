#ifndef _LIBRARY_H_
#define _LIBRARY_H_

#include "defines.h"
#include "str.h"

class File;
class Texture;
class PieceInfo;

#define LC_PIECESLIB_MAXGROUPS    32

class PiecesLibrary
{
public:
  PiecesLibrary ();
  ~PiecesLibrary ();

  const char* GetLibraryPath() const
    { return m_LibraryPath; }
  int GetPieceCount () const
    { return m_nPieceCount; }
  int GetTextureCount () const
    { return m_nTextureCount; }
  int GetGroupCount () const
    { return m_nGroupCount; }
  String& GetGroup (int i)
    { return m_strGroups[i]; }
	bool NeedsReload () const
		{ return m_bNeedsReload; }

	void CheckReload ();
  bool Load (const char* libpath);
  void Unload ();
	bool LoadGroupConfig (const char* Filename);

	// Search for stuff
  PieceInfo* FindPieceInfo (const char* name) const;
  PieceInfo* GetPieceInfo (int index) const;
  int GetPieceIndex (PieceInfo *pInfo) const;
  Texture* FindTexture (const char* name) const;
  Texture* GetTexture (int index) const;

	// File operations
  bool DeletePieces (char** names, int numpieces);
  bool LoadUpdate (const char* update);
	bool DeleteTextures (char** Names, int NumTextures);
	bool ImportTexture (const char* Name);
	bool ImportLDrawPiece (const char* Filename);

	static unsigned long GetDefaultPieceGroup (const char* name);

protected:
  char m_LibraryPath[LC_MAXPATH];	// path to the library files

	int m_nMovedCount;       // number of moved pieces
  char* m_pMovedReference; // moved pieces list
	int m_nPieceCount;       // number of pieces
	PieceInfo* m_pPieceIdx;	 // pieces array
	int m_nTextureCount;     // number of textures
	Texture* m_pTextures;    // textures array

	// Groups stuff
	int m_nGroupCount;
	String m_strGroups[LC_PIECESLIB_MAXGROUPS];
	char m_GroupsFile[LC_MAXPATH];

	bool m_bNeedsReload;  // if the library files were changed and they need to be reloaded

	bool ValidatePiecesFile (File& IdxFile, File& BinFile) const;
	bool ValidateTexturesFile (File& IdxFile, File& BinFile) const;

	// File headers
	static const char PiecesBinHeader[32];
	static const char PiecesIdxHeader[32];
	static const int PiecesFileVersion;
	static const char TexturesBinHeader[32];
	static const char TexturesIdxHeader[32];
	static const int TexturesFileVersion;
};




// ============================================================================

// This should be cleaned and moved to the PiecesLibrary class
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

#endif // _LIBRARY_H_
