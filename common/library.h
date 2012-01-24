#ifndef _LIBRARY_H_
#define _LIBRARY_H_

#include "defines.h"
#include "str.h"
#include "array.h"

class File;
class FileDisk;
class Texture;
class PieceInfo;

#define LC_CATEGORY_FILE_ID       LC_FOURCC('C', 'A', 'T', 0)
#define LC_CATEGORY_FILE_VERSION  0x0100

struct PiecesLibraryCategory
{
	String Name;
	String Keywords;
};

class PiecesLibrary
{
public:
	PiecesLibrary();
	~PiecesLibrary();

	const char* GetLibraryPath() const
		{ return m_LibraryPath; }
	int GetPieceCount () const
		{ return m_nPieceCount; }
	int GetTextureCount () const
		{ return m_nTextureCount; }

	// Categories.
	bool PieceInCategory(PieceInfo* Info, const String& CategoryKeywords) const;
	int GetFirstCategory(PieceInfo* Info) const;
	void GetCategoryEntries(int CategoryIndex, bool GroupPieces, PtrArray<PieceInfo>& SinglePieces, PtrArray<PieceInfo>& GroupedPieces) const;
	void GetPatternedPieces(PieceInfo* Parent, PtrArray<PieceInfo>& Pieces) const;
	void SetCategory(int Index, const String& Name, const String& Keywords);
	void AddCategory(const String& Name, const String& Keywords);
	void RemoveCategory(int Index);
	void ResetCategories();
	bool SaveCategories();
	bool DoSaveCategories(bool AskName);
	bool LoadCategories(const char* FileName);

	const String& GetCategoryName(int Index) const
		{ return m_Categories[Index].Name; }

	const String& GetCategoryKeywords(int Index) const
		{ return m_Categories[Index].Keywords; }

	int GetNumCategories() const
		{ return m_Categories.GetSize(); }

	int FindCategoryIndex(const String& CategoryName) const
	{
		for (int i = 0; i < m_Categories.GetSize(); i++)
			if (m_Categories[i].Name == CategoryName)
				return i;

		return -1;
	}

	bool Load(const char* libpath);
	void Unload();

	// Search for pieces.
	PieceInfo* FindPieceInfo(const char* name) const;
	PieceInfo* GetPieceInfo(int index) const;
	int GetPieceIndex(PieceInfo *pInfo) const;
	Texture* FindTexture(const char* name) const;
	Texture* GetTexture(int index) const;

	// File operations.
	bool DeleteAllPieces();
	bool DeletePieces(PtrArray<const char>& Pieces);
	bool LoadUpdate(const char* update);
	bool DeleteTextures(char** Names, int NumTextures);
	bool ImportTexture(const char* Name);
	bool ImportLDrawPiece(const char* Filename, File* NewIdxFile, File* NewBinFile, File* OldIdxFile, File* OldBinFile);

	// Set when pieces are added/removed from the library.
	bool m_Modified;

protected:
	char m_LibraryPath[LC_MAXPATH];	// path to the library files

	int m_nMovedCount;       // number of moved pieces
	char* m_pMovedReference; // moved pieces list
	int m_nPieceCount;       // number of pieces
	PieceInfo* m_pPieceIdx;	 // pieces array
	int m_nTextureCount;     // number of textures
	Texture* m_pTextures;    // textures array

	// Categories.
	ObjArray<PiecesLibraryCategory> m_Categories;

	bool m_CategoriesModified;
	char m_CategoriesFile[LC_MAXPATH];

	bool ValidatePiecesFile(FileDisk& IdxFile, FileDisk& BinFile) const;
	bool ValidateTexturesFile(File& IdxFile, File& BinFile) const;

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
struct connection_t
{
	unsigned char type;
	float pos[3];
	float up[3];
	connection_t* next;
};

struct group_t
{
	connection_t* connections[5];
	void* drawinfo;
	unsigned long infosize;
	group_t* next;
};

struct lineinfo_t
{
	unsigned char type;
	unsigned char color;
	float points[12];
	lineinfo_t* next;
};

struct texture_t
{
	float points[20];
	unsigned char color;
	char name[9];
	texture_t* next;
};

struct LC_LDRAW_PIECE
{
	float* verts;
	unsigned int verts_count;
	bool long_info;
	connection_t* connections;
	group_t* groups;
	texture_t* textures;
	char name[LC_MAXPATH];
	char description[65];
};

bool ReadLDrawPiece(const char* filename, LC_LDRAW_PIECE* piece);
bool SaveLDrawPiece(LC_LDRAW_PIECE* piece, File* NewIdxFile, File* NewBinFile, File* OldIdxFile, File* OldBinFile);
void FreeLDrawPiece(LC_LDRAW_PIECE* piece);

#endif // _LIBRARY_H_
