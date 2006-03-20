//
// Pieces library management
//

#include <stdlib.h>
#include "library.h"
#include "file.h"
#include "texture.h"
#include "pieceinf.h"
#include "config.h"
#include "image.h"
#include "system.h"
#include "console.h"
#include "lc_application.h"

// =============================================================================
// PiecesLibrary class

const char PiecesLibrary::PiecesBinHeader[32] = "LeoCAD piece library data file\0";
const char PiecesLibrary::PiecesIdxHeader[32] = "LeoCAD piece library index file";
const int PiecesLibrary::PiecesFileVersion = 3;
const char PiecesLibrary::TexturesBinHeader[32] = "LeoCAD texture data file\0\0\0\0\0\0\0";
const char PiecesLibrary::TexturesIdxHeader[32] = "LeoCAD texture index file\0\0\0\0\0\0";
const int PiecesLibrary::TexturesFileVersion = 1;

PiecesLibrary::PiecesLibrary()
{
	strcpy(m_LibraryPath, "");
	strcpy(m_CategoriesFile, "");
	m_pMovedReference = NULL;
	m_nMovedCount = 0;
	m_pPieceIdx = NULL;
	m_nPieceCount = 0;
	m_pTextures = NULL;
	m_nTextureCount = 0;
	m_Modified = false;
	m_CategoriesModified = false;
}

PiecesLibrary::~PiecesLibrary()
{
	Unload();
}

void PiecesLibrary::Unload ()
{
	strcpy(m_LibraryPath, "");

	free(m_pMovedReference);
	m_pMovedReference = NULL;
	m_nMovedCount = 0;

	delete [] m_pPieceIdx;
	m_pPieceIdx = NULL;
	m_nPieceCount = 0;

	delete [] m_pTextures;
	m_pTextures = NULL;
	m_nTextureCount = 0;
}

bool PiecesLibrary::Load (const char *libpath)
{
	FileDisk idx, bin;
	char filename[LC_MAXPATH];
	lcuint16 count, movedcount;
	lcuint32 binsize;
	Texture* pTexture;
	int i;

	strcpy (m_LibraryPath, libpath);

	// Make sure that the path ends with a '/'
	i = strlen(m_LibraryPath)-1;
	if ((m_LibraryPath[i] != '\\') && (m_LibraryPath[i] != '/'))
		strcat(m_LibraryPath, "/");

	// Read the piece library index.
	strcpy (filename, m_LibraryPath);
	strcat (filename, "pieces.idx");

	if (!idx.Open (filename, "rb"))
	{
		console.PrintError ("Cannot open Pieces Library file: %s.\n", filename);
		return false;
	}

	strcpy (filename, m_LibraryPath);
	strcat (filename, "pieces.bin");

	if (!bin.Open (filename, "rb"))
	{
		console.PrintError ("Cannot open Pieces Library file: %s.\n", filename);
		return false;
	}

	if (!ValidatePiecesFile (idx, bin))
		return false;

	idx.Seek (-(long)(2*sizeof(count)+sizeof(binsize)), SEEK_END);
	idx.ReadShort (&movedcount, 1);
	idx.ReadLong (&binsize, 1);
	idx.ReadShort (&count, 1);
	idx.Seek (34, SEEK_SET);

	// Load piece indexes
	delete [] m_pPieceIdx;
	m_pPieceIdx = new PieceInfo[count];
	m_nPieceCount = count;

	for (PieceInfo *pElements = m_pPieceIdx; count--; pElements++)
		pElements->LoadIndex (idx);

	// Load moved files reference.
	if (m_pMovedReference != NULL)
		free(m_pMovedReference);
	m_pMovedReference = (char*)malloc(18*movedcount);
	memset (m_pMovedReference, 0, 18*movedcount);
	m_nMovedCount = movedcount;

	for (i = 0; i < movedcount; i++)
	{
		idx.Read (&m_pMovedReference[i*18], 8);
		idx.Read (&m_pMovedReference[i*18+9], 8);
	}

	idx.Close();
	bin.Close();

	// Load groups configuration
	const char* FileName = Sys_ProfileLoadString("Settings", "Categories", "");
	if (!strlen(FileName) || !LoadCategories(FileName))
		ResetCategories();

	// Read the texture index.
	strcpy(filename, m_LibraryPath);
	strcat(filename, "textures.idx");

	if (m_pTextures != NULL)
	{
		delete [] m_pTextures;
		m_pTextures = NULL;
		m_nTextureCount = 0;
	}

	if (!idx.Open(filename, "rb"))
	{
		console.PrintError ("Cannot open Textures Library file: %s.\n", filename);
		return false;
	}

	strcpy (filename, m_LibraryPath);
	strcat (filename, "textures.bin");

	if (!bin.Open (filename, "rb"))
	{
		console.PrintError ("Cannot open Textures Library file: %s.\n", filename);
		return false;
	}

	if (!ValidateTexturesFile (idx, bin))
		return false;

	idx.Seek(-(long)(sizeof(count)+sizeof(binsize)), SEEK_END);
	idx.ReadLong (&binsize, 1);
	idx.ReadShort (&count, 1);
	idx.Seek(34, SEEK_SET);

	m_pTextures = new Texture[count];
	m_nTextureCount = count;
	memset(m_pTextures, 0, count * sizeof(Texture));

	for (pTexture = m_pTextures; count--; pTexture++)
		pTexture->LoadIndex(&idx);

	idx.Close();
	bin.Close();

	SystemUpdateCategories(false);

	m_CategoriesModified = false;
	m_Modified = false;

	return true;
}

// Make sure the pieces library files are valid
bool PiecesLibrary::ValidatePiecesFile (File& IdxFile, File& BinFile) const
{
	lcuint32 binsize, IdxPos = IdxFile.GetPosition(), BinPos = BinFile.GetPosition();
	lcuint16 count, movedcount;
	lcuint8 version;
	char header[32];

	IdxFile.Seek (-(long)(2*sizeof(count)+sizeof(binsize)), SEEK_END);
	IdxFile.ReadShort (&movedcount, 1);
	IdxFile.ReadLong (&binsize, 1);
	IdxFile.ReadShort (&count, 1);
	IdxFile.Seek (0, SEEK_SET);
	IdxFile.Read (header, 32);
	IdxFile.ReadByte (&version, 1);
	IdxFile.Seek (IdxPos, SEEK_SET);

	if (memcmp (header, PiecesIdxHeader, 32) != 0)
	{
		console.PrintError ("Invalid Pieces Library file.\n");
		return false;
	}

	if (version != PiecesFileVersion)
	{
		console.PrintError ("Wrong version of the Pieces Library files.\n");
		return false;
	}

	BinFile.Seek (0, SEEK_SET);
	BinFile.Read (header, 32);
	BinFile.Seek (BinPos, SEEK_SET);

	if (memcmp (header, PiecesBinHeader, 32) != 0)
	{
		console.PrintError ("Invalid Pieces Library file.\n");
		return false;
	}

	if (binsize != BinFile.GetLength ())
	{
		console.PrintError ("Wrong size of the Pieces Library file.\n");
		return false;
	}

	return true;
}

// Make sure the textures library files are valid
bool PiecesLibrary::ValidateTexturesFile (File& IdxFile, File& BinFile) const
{
	lcuint32 binsize, IdxPos = IdxFile.GetPosition(), BinPos = BinFile.GetPosition();
	lcuint16 count;
	lcuint8 version;
	char header[32];

	IdxFile.Seek (-(long)(sizeof(count)+sizeof(binsize)), SEEK_END);
	IdxFile.ReadLong (&binsize, 1);
	IdxFile.ReadShort (&count, 1);
	IdxFile.Seek (0, SEEK_SET);
	IdxFile.Read (header, 32);
	IdxFile.ReadByte (&version, 1);
	IdxFile.Seek (IdxPos, SEEK_SET);

	if (memcmp (header, TexturesIdxHeader, 32) != 0)
	{
		console.PrintError ("Invalid Textures Library file.\n");
		return false;
	}

	if (version != TexturesFileVersion)
	{
		console.PrintError ("Wrong version of the Textures Library files.\n");
		return false;
	}

	BinFile.Seek (0, SEEK_SET);
	BinFile.Read (header, 32);
	BinFile.Seek (BinPos, SEEK_SET);

	if (memcmp (header, TexturesBinHeader, 32) != 0)
	{
		console.PrintError ("Invalid Textures Library file.\n");
		return false;
	}

	if (binsize != BinFile.GetLength ())
	{
		console.PrintError ("Wrong size of the Textures Library files.\n");
		return false;
	}

	return true;
}

// =============================================================================
// Search functions

// Remember to make 'name' uppercase.
PieceInfo* PiecesLibrary::FindPieceInfo (const char* name) const
{
	PieceInfo* pInfo;
	int i;

	for (i = 0, pInfo = m_pPieceIdx; i < m_nPieceCount; i++, pInfo++)
		if (!strcmp (name, pInfo->m_strName))
			return pInfo;

	for (i = 0; i < m_nMovedCount; i++)
	{
		if (!strcmp (&m_pMovedReference[i*18], name))
		{
			char* tmp = &m_pMovedReference[i*18+9];

			for (i = 0, pInfo = m_pPieceIdx; i < m_nPieceCount; i++, pInfo++)
				if (!strcmp (tmp, pInfo->m_strName))
					return pInfo;

			break; // something went wrong.
		}
	}

	return NULL;
}

PieceInfo* PiecesLibrary::GetPieceInfo(int index) const
{
	return &m_pPieceIdx[index];
}

int PiecesLibrary::GetPieceIndex(PieceInfo *pInfo) const
{
	return (((char*)pInfo - (char*)m_pPieceIdx) / sizeof (PieceInfo));
}

Texture* PiecesLibrary::FindTexture(const char* name) const
{
	for (int i = 0; i < m_nTextureCount; i++)
		if (!strcmp (name, m_pTextures[i].m_strName))
			return &m_pTextures[i];

		return NULL;
}

Texture* PiecesLibrary::GetTexture (int index) const
{
	return &m_pTextures[index];
}

// =============================================================================
// Category functions.

void PiecesLibrary::ResetCategories()
{
	struct CategoryEntry
	{
		const char* Name;
		const char* Keywords;
	};

	// Animal, Antenna, Arch, Arm, Bar, Baseplate, Belville, Boat, Bracket, Brick,
	// Car, Cone, Container, Conveyor, Crane, Cylinder, Door, Electric, Exhaust,
	// Fence, Flag, Forklift, Freestyle, Garage, Gate, Glass, Grab, Hinge, Homemaker,
	// Hose, Jack, Ladder, Lever, Magnet, Minifig, Minifig Accessory, Panel, Plane,
	// Plant, Plate, Platform, Propellor, Rack, Roadsign, Rock, Scala, Slope, Staircase,
	// Support, Tail, Tap, Technic, Tile, Tipper, Tractor, Trailer, Train, Turntable,
	// Tyre, Wedge, Wheel, Winch, Window, Windscreen, Wing
	CategoryEntry DefaultCategories[] =
	{
		{ "Animal", "^%Animal" },
		{ "Antenna", "^%Antenna" },
		{ "Arch", "^%Arch" },
		{ "Bar", "^%Bar" },
		{ "Baseplate", "^%Baseplate | ^%Platform" },
		{ "Boat", "^%Boat" },
		{ "Brick", "^%Brick" },
		{ "Container", "^%Container | ^%Box | ^Chest | ^%Storage | ^Mailbox" },
		{ "Door and Window", "^%Door | ^%Window | ^%Glass | ^%Freestyle | ^%Gate | ^%Garage | ^%Roller" },
		{ "Duplo", "^%Duplo | ^%Scala | ^%Belville" },
		{ "Electric", "^%Electric | ^%Light | ^%Excavator | ^%Exhaust" },
		{ "Hinge and Bracket", "^%Hinge | ^%Bracket | ^%Turntable" },
		{ "Hose", "^%Hose" },
		{ "Minifig", "^%Minifig" },
		{ "Miscellaneous", "^%Arm | ^%Barrel | ^%Brush | ^%Cockpit | ^%Conveyor | ^%Crane | ^%Cupboard | ^%Fabuland | ^%Fence | ^%Homemaker | ^%Jack | ^%Ladder | ^%Rock | ^%Staircase | ^%Stretcher | ^%Tap | ^%Tipper | ^%Trailer | ^%Winch" },
		{ "Panel", "^%Panel | ^%Castle Wall | ^%Castle Turret" },
		{ "Plant", "^%Plant" },
		{ "Plate", "^%Plate" },
		{ "Round", "^%Cylinder | ^%Cone | ^%Dish | ^%Round" },
		{ "Sign and Flag", "^%Flag | ^%Roadsign | ^%Streetlight | ^%Flagpost | ^%Lamppost | ^%Signpost" },
		{ "Slope", "^%Slope" },
		{ "Space", "^%Space" },
		{ "Sticker", "^%Sticker" },
		{ "Support", "^%Support" },
		{ "Technic", "^%Technic | ^%Rack" },
		{ "Tile", "^%Tile" },
		{ "Train", "^%Train | ^%Monorail | ^%Magnet" },
		{ "Tyre and Wheel", "^%Tyre | %^Wheel | ^%Castle Wagon" },
		{ "Vehicle", "^%Car | ^%Tractor | ^%Bike | ^%Plane | ^%Propellor | ^%Tail | ^%Landing | ^%Forklift | ^%Grab Jaw" },
		{ "Windscreen", "^%Windscreen" },
		{ "Wedge", "^%Wedge" },
		{ "Wing", "^%Wing" },
	};
	const int NumCategories = sizeof(DefaultCategories)/sizeof(DefaultCategories[0]);

	m_Categories.RemoveAll();
	for (int i = 0; i < NumCategories; i++)
	{
		PiecesLibraryCategory Cat;

		Cat.Name = DefaultCategories[i].Name;
		Cat.Keywords = DefaultCategories[i].Keywords;

		m_Categories.Add(Cat);
	}

	strcpy(m_CategoriesFile, "");
	Sys_ProfileSaveString("Settings", "Categories", m_CategoriesFile);
	m_CategoriesModified = false;
}

bool PiecesLibrary::LoadCategories(const char* FileName)
{
	char Path[LC_MAXPATH];

	if (FileName)
	{
		strcpy(Path, FileName);
	}
	else
	{
		LC_FILEOPENDLG_OPTS opts;

		opts.type = LC_FILEOPENDLG_LCF;
		strcpy(opts.path, m_CategoriesFile);

		if (!SystemDoDialog(LC_DLG_FILE_OPEN, &opts)) 
			return false;

		strcpy(Path, (char*)opts.filenames);

		free(opts.filenames);
	}

	// Load the file.
	FileDisk File;

	if (!File.Open(Path, "rb"))
		return false;

	lcuint32 i;

	File.ReadInt(&i);
	if (i != LC_FILE_ID)
		return false;

	File.ReadInt(&i);
	if (i != LC_CATEGORY_FILE_ID)
		return false;

	File.ReadInt(&i);
	if (i != LC_CATEGORY_FILE_VERSION)
		return false;

	m_Categories.RemoveAll();

	File.ReadInt(&i);
	while (i--)
	{
		PiecesLibraryCategory Cat;

		File.ReadString(Cat.Name);
		File.ReadString(Cat.Keywords);

		m_Categories.Add(Cat);
	}

	strcpy(m_CategoriesFile, Path);
	Sys_ProfileSaveString("Settings", "Categories", m_CategoriesFile);
	m_CategoriesModified = false;

	return true;
}

// Returns true if it's ok to continue.
bool PiecesLibrary::SaveCategories()
{
	if (m_CategoriesModified)
	{
		switch (SystemDoMessageBox("Save category changes ?", LC_MB_YESNOCANCEL|LC_MB_ICONQUESTION))
		{
			case LC_CANCEL:
				return false;

			case LC_YES:
				if (!DoSaveCategories(false))
					return false;
				break;

			case LC_NO:
				return true;
				break;
		}
	}

	return true;
}

bool PiecesLibrary::DoSaveCategories(bool AskName)
{
	// Get the file name.
	if (AskName || (strlen(m_CategoriesFile) == 0))
	{
		LC_FILESAVEDLG_OPTS opts;

		opts.type = LC_FILESAVEDLG_LCF;
		strcpy(opts.path, m_CategoriesFile);

		if (!SystemDoDialog(LC_DLG_FILE_SAVE, &opts)) 
			return false; 

		strcpy(m_CategoriesFile, opts.path);
	}

	// Save the file.
	FileDisk File;

	if (!File.Open(m_CategoriesFile, "wb"))
		return false;

	File.WriteInt(LC_FILE_ID);
	File.WriteInt(LC_CATEGORY_FILE_ID);
	File.WriteInt(LC_CATEGORY_FILE_VERSION);

	int NumCategories = m_Categories.GetSize();
	int i;

	for (i = 0; i < m_Categories.GetSize(); i++)
	{
		if (m_Categories[i].Name == "Search Results")
		{
			NumCategories--;
			break;
		}
	}


	File.WriteInt(NumCategories);
	for (i = 0; i < m_Categories.GetSize(); i++)
	{
		if (m_Categories[i].Name == "Search Results")
			continue;

		File.WriteString(m_Categories[i].Name);
		File.WriteString(m_Categories[i].Keywords);
	}

	Sys_ProfileSaveString("Settings", "Categories", m_CategoriesFile);
	m_CategoriesModified = false;

	return true;
}

// =============================================================================

// Check if the piece belongs to a category.
bool PiecesLibrary::PieceInCategory(PieceInfo* Info, const String& CategoryKeywords) const
{
	String PieceName = Info->m_strDescription;
	PieceName.MakeLower();

	String Keywords = CategoryKeywords;
	Keywords.MakeLower();

	return PieceName.Match(Keywords);
}

int PiecesLibrary::GetFirstCategory(PieceInfo* Info) const
{
	for (int i = 0; i < m_Categories.GetSize(); i++)
		if (PieceInCategory(Info, m_Categories[i].Keywords))
			return i;

	return -1;
}

void PiecesLibrary::GetCategoryEntries(int CategoryIndex, bool GroupPieces, PtrArray<PieceInfo>& SinglePieces, PtrArray<PieceInfo>& GroupedPieces) const
{
	SinglePieces.RemoveAll();
	GroupedPieces.RemoveAll();

	// Don't group entries in the search results category.
	if (m_Categories[CategoryIndex].Name == "Search Results")
		GroupPieces = false;

	for (int i = 0; i < m_nPieceCount; i++)
	{
		PieceInfo* Info = &m_pPieceIdx[i];

		if (!PieceInCategory(Info, m_Categories[CategoryIndex].Keywords))
			continue;

		if (!GroupPieces)
		{
			SinglePieces.Add(Info);
			continue;
		}

		// Check if it's a patterned piece.
		if (Info->IsPatterned())
		{
			PieceInfo* Parent;

			// Find the parent of this patterned piece.
			char ParentName[9];
			strcpy(ParentName, Info->m_strName);
			*strchr(ParentName, 'P') = '\0';

			Parent = FindPieceInfo(ParentName);

			if (Parent)
			{
				// Check if the parent was added as a single piece.
				int Index = SinglePieces.FindIndex(Parent);

				if (Index != -1)
					SinglePieces.RemoveIndex(Index);

				Index = GroupedPieces.FindIndex(Parent);

				if (Index == -1)
					GroupedPieces.Add(Parent);
			}
			else
			{
				// Patterned pieces should have a parent but in case they don't just add them anyway.
				SinglePieces.Add(Info);
			}
		}
		else
		{
			// Check if this piece has already been added to this category by one of its children.
			int Index = GroupedPieces.FindIndex(Info);

			if (Index == -1)
				SinglePieces.Add(Info);
		}
	}
}

void PiecesLibrary::GetPatternedPieces(PieceInfo* Parent, PtrArray<PieceInfo>& Pieces) const
{
	char Name[9];
	strcpy(Name, Parent->m_strName);
	strcat(Name, "P");

	Pieces.RemoveAll();

	for (int i = 0; i < m_nPieceCount; i++)
	{
		PieceInfo* Info = &m_pPieceIdx[i];

		if (strncmp(Name, Info->m_strName, strlen(Name)) == 0)
			Pieces.Add(Info);
	}
}

void PiecesLibrary::SetCategory(int Index, const String& Name, const String& Keywords)
{
	m_Categories[Index].Name = Name;
	m_Categories[Index].Keywords = Keywords;

	SystemUpdateCategories(true);

	m_CategoriesModified = true;
}

void PiecesLibrary::AddCategory(const String& Name, const String& Keywords)
{
	PiecesLibraryCategory Cat;

	Cat.Name = Name;
	Cat.Keywords = Keywords;

	m_Categories.Add(Cat);

	SystemUpdateCategories(true);

	m_CategoriesModified = true;
}

void PiecesLibrary::RemoveCategory(int Index)
{
	m_Categories.RemoveIndex(Index);

	m_CategoriesModified = true;
}

// =============================================================================
// Pieces handling stuff

// Remove pieces from the library
bool PiecesLibrary::DeletePieces (PtrArray<PieceInfo>& Pieces)
{
	FileDisk newbin, newidx, oldbin, oldidx;
	char file1[LC_MAXPATH], file2[LC_MAXPATH], tmp[200];
	lcuint16 count, deleted = 0, j;
	void* membuf;

	strcpy(file1, m_LibraryPath);
	strcat(file1, "pieces-b.old");
	remove(file1);
	strcpy(file2, m_LibraryPath);
	strcat(file2, "pieces.bin");
	rename(file2, file1);

	if ((!oldbin.Open(file1, "rb")) ||
		(!newbin.Open(file2, "wb")))
		return false;

	strcpy(file1, m_LibraryPath);
	strcat(file1, "pieces-i.old");
	remove(file1);
	strcpy(file2, m_LibraryPath);
	strcat(file2, "pieces.idx");
	rename(file2, file1);

	if ((!oldidx.Open(file1, "rb")) ||
		(!newidx.Open(file2, "wb")))
		return false;

	oldidx.Seek(-2, SEEK_END);
	oldidx.ReadShort(&count, 1);
	oldidx.Seek(0, SEEK_SET);
	oldidx.Read(tmp, 34);
	newidx.Write(tmp, 34);
	oldbin.Read(tmp, 32);
	newbin.Write(tmp, 32);

//	CProgressDlg dlg("Deleting");
//	dlg.Create(this);
//	dlg.SetRange (0, count);

	for (j = 0; j < count; j++)
	{
//		dlg.StepIt();
//		dlg.SetStatus(m_Parts[j].info->m_strDescription);

//		if (dlg.CheckCancelButton())
//			if (AfxMessageBox(IDS_CANCEL_PROMPT, MB_YESNO) == IDYES)
//				break;

		char name[9];
		int i;
		name[8] = 0;
		oldidx.Read(&name, 8);

		for (i = 0; i < Pieces.GetSize(); i++)
			if (strcmp(name, Pieces[i]->m_strName) == 0)
				break;

		if (i != Pieces.GetSize())
		{
			oldidx.Seek(64+12+1+4+4+4, SEEK_CUR);
			deleted++;
			continue;
		}

		newidx.Write(name, 8);
		oldidx.Read(tmp, 64+12+1+4);
		newidx.Write(tmp, 64+12+1+4);

		lcuint32 binoff = newbin.GetLength(), size;
		newidx.WriteLong(&binoff, 1);
		oldidx.ReadLong(&binoff, 1);
		oldidx.ReadLong(&size, 1);
		newidx.WriteLong(&size, 1);

		membuf = malloc(size);
		oldbin.Seek(binoff, SEEK_SET);
		oldbin.Read(membuf, size);
		newbin.Write(membuf, size);
		free(membuf);
	}

	// list of moved pieces
	lcuint16 moved, cs;

	oldidx.Seek(-(2+4+2), SEEK_END);
	oldidx.ReadShort(&moved, 1);
	cs = 2+(moved*16);
	oldidx.Seek(-(long)cs, SEEK_CUR);
	membuf = malloc(cs);
	oldidx.Read(membuf, cs);
	newidx.Write(membuf, cs);
	free(membuf);

	// info at the end
	lcuint32 binoff = newbin.GetPosition();
	newidx.WriteLong(&binoff, 1);
	count -= deleted;
	newidx.WriteShort(&count, 1);

	oldidx.Close();
	oldbin.Close();
	newidx.Close();
	newbin.Close();

	m_Modified = true;

	return true;
}

// Load update
bool PiecesLibrary::LoadUpdate (const char* update)
{
	FileDisk newbin, newidx, oldbin, oldidx, up;
	char file1[LC_MAXPATH], file2[LC_MAXPATH], tmp[200];
	lcuint16 changes, moved, count, i, j, newcount = 0;
	lcuint32 cs, group, binoff;
	lcuint8 bt;
	void* membuf;

	typedef struct
	{
		char name[9];
		lcuint8 type;
		lcuint32 offset;
	} LC_UPDATE_INFO;
	LC_UPDATE_INFO* upinfo;

	strcpy(file1, m_LibraryPath);
	strcat(file1, "pieces-b.old");
	remove(file1);
	strcpy(file2, m_LibraryPath);
	strcat(file2, "pieces.bin");
	rename(file2, file1);

	if ((!oldbin.Open(file1, "rb")) ||
		(!newbin.Open(file2, "wb")))
		return false;

	strcpy(file1, m_LibraryPath);
	strcat(file1, "pieces-i.old");
	remove(file1);
	strcpy(file2, m_LibraryPath);
	strcat(file2, "pieces.idx");
	rename(file2, file1);

	if ((!oldidx.Open(file1, "rb")) ||
		(!newidx.Open(file2, "wb")))
		return false;

	if (!up.Open(update, "rb"))
		return false;

	up.Seek(32, SEEK_SET);
	up.ReadByte(&bt, 1);
	if (bt != 2)
		return false;	// wrong version

	up.ReadByte(&bt, 1); // update number

	up.Seek(-2, SEEK_END);
	up.ReadShort(&changes, 1);
	up.Seek(34, SEEK_SET);

	oldidx.Seek(-2, SEEK_END);
	oldidx.ReadShort(&count, 1);
	oldidx.Seek(0, SEEK_SET);
	oldidx.Read(tmp, 34);
	newidx.Write(tmp, 33); // skip update byte
	newidx.WriteByte(&bt, 1);
	oldbin.Read(tmp, 32);
	newbin.Write(tmp, 32);

	upinfo = (LC_UPDATE_INFO*)malloc(sizeof(LC_UPDATE_INFO)*changes);
	memset(upinfo, 0, sizeof(LC_UPDATE_INFO)*changes);

	for (i = 0; i < changes; i++)
	{
		up.Read(&upinfo[i].name, 8);
		up.Read(&upinfo[i].type, 1);
		upinfo[i].offset = up.GetPosition();

		if ((upinfo[i].type & LC_UPDATE_DESCRIPTION) ||
			(upinfo[i].type & LC_UPDATE_NEWPIECE))
			up.Seek(64+4, SEEK_CUR);

		if ((upinfo[i].type & LC_UPDATE_DRAWINFO) ||
			(upinfo[i].type & LC_UPDATE_NEWPIECE))
		{
			up.Seek(12+1, SEEK_CUR);
			up.ReadLong(&cs, 1);
			up.Seek(cs, SEEK_CUR);
		}
	}

//	CProgressDlg dlg(_T("Updating Library"));
//	dlg.Create(this);
//	dlg.SetRange (0, count);

	for (i = 0; i < count; i++)
	{
		char name[9];
		name[8] = 0;
		oldidx.Read (&name, 8);

//		dlg.StepIt();
//		if(dlg.CheckCancelButton())
//			if(AfxMessageBox(IDS_CANCEL_PROMPT, MB_YESNO) == IDYES)
//			{
//				free(upinfo);
//				return TRUE;
//			}

		for (j = 0; j < changes; j++)
		if (strcmp(name, upinfo[j].name) == 0)
		{
			if (upinfo[j].type == LC_UPDATE_DELETE)
			{
				oldidx.Seek(64+12+1+4+4+4, SEEK_CUR);
				break;
			}

			newcount++;
			up.Seek(upinfo[j].offset, SEEK_SET);
			newidx.Write(name, 8);

			// description
			if (upinfo[j].type & LC_UPDATE_DESCRIPTION)
			{
				up.Read(&tmp, 64);
				up.Read(&group, 4);
				oldidx.Seek(64, SEEK_CUR);
			}
			else
				oldidx.Read(&tmp, 64);
			newidx.Write(tmp, 64);
//			dlg.SetStatus(tmp);

			// bounding box & flags
			if (upinfo[j].type & LC_UPDATE_DRAWINFO)
			{
				up.Read(&tmp, 12+1);
				oldidx.Seek(12+1, SEEK_CUR);
			}
			else
				oldidx.Read(&tmp, 12+1);
			newidx.Write(tmp, 12+1);

			// group
			if (upinfo[j].type & LC_UPDATE_DESCRIPTION)
				oldidx.Seek(4, SEEK_CUR);
			else
				oldidx.Read(&group, 4);
			newidx.Write(&group, 4);

			binoff = newbin.GetLength();
			newidx.WriteLong(&binoff, 1);

			if (upinfo[j].type & LC_UPDATE_DRAWINFO)
			{
				up.ReadLong(&cs, 1);
				oldidx.Seek(4+4, SEEK_CUR);

				membuf = malloc(cs);
				up.Read(membuf, cs);
				newbin.Write(membuf, cs);
				free(membuf);
			}
			else
			{
				oldidx.ReadLong(&binoff, 1);
				oldidx.ReadLong(&cs, 1);

				membuf = malloc(cs);
				oldbin.Seek(binoff, SEEK_SET);
				oldbin.Read(membuf, cs);
				newbin.Write(membuf, cs);
				free(membuf);
			}
			newidx.WriteLong(&cs, 1);
			break;
		}

		// not changed, just copy
		if (j == changes)
		{
			newcount++;
			newidx.Write(name, 8);
			oldidx.Read(tmp, 64+12+1+4);
			newidx.Write(tmp, 64+12+1+4);
			binoff = newbin.GetLength();
			newidx.WriteLong(&binoff, 1);
			oldidx.ReadLong(&binoff, 1);
			oldidx.ReadLong(&cs, 1);
			newidx.WriteLong(&cs, 1);

//			tmp[64] = 0;
//			dlg.SetStatus(tmp);

			membuf = malloc(cs);
			oldbin.Seek(binoff, SEEK_SET);
			oldbin.Read(membuf, cs);
			newbin.Write(membuf, cs);
			free(membuf);
		}
	}

	// now add new pieces
	for (j = 0; j < changes; j++)
		if (upinfo[j].type == LC_UPDATE_NEWPIECE)
		{
			newcount++;
			newidx.Write(upinfo[j].name, 8);
			up.Seek(upinfo[j].offset, SEEK_SET);
			up.Read(&tmp, 64+12);
			newidx.Write(tmp, 64+12);
			up.Read(&group, 4);
			up.Read(&bt, 1);
			newidx.Write(&bt, 1);
			newidx.Write(&group, 4);
			binoff = newbin.GetLength();
			newidx.WriteLong(&binoff, 1);

			up.ReadLong(&cs, 1);
			membuf = malloc(cs);
			up.Read(membuf, cs);
			newbin.Write(membuf, cs);
			up.WriteLong(&cs, 1);
			newidx.WriteLong(&cs, 1);
			free (membuf);
		}

	up.Seek(-(2+2), SEEK_END);
	up.ReadShort(&moved, 1);
	cs = 2+moved*16;
	up.Seek(-(long)(cs), SEEK_CUR);
	membuf = malloc(cs);
	up.Read(membuf, cs);
	newidx.Write(membuf, cs);
	free(membuf);

	binoff = newbin.GetLength();
	newidx.WriteLong(&binoff, 1);
	newidx.WriteShort(&newcount, 1);

	free(upinfo);
	oldidx.Close();
	oldbin.Close();
	newidx.Close();
	newbin.Close();
	up.Close();

	m_Modified = true;

	return true;
}

// =============================================================================
// Textures handling stuff

bool PiecesLibrary::DeleteTextures (char** Names, int NumTextures)
{
	char file1[LC_MAXPATH], file2[LC_MAXPATH];
	FileDisk newbin, newidx, oldbin, oldidx;
	lcuint32 binsize, offset = 0;
  lcuint16 count, deleted = 0, i, j;
	lcuint8 version, bt;

	// Backup files
	strcpy(file1, m_LibraryPath);
	strcat(file1, "tex-b.old");
	remove(file1);
	strcpy(file2, m_LibraryPath);
	strcat(file2, "textures.bin");
	rename(file2, file1);

	if ((!oldbin.Open(file1, "rb")) || (!newbin.Open(file2, "wb")))
		return false;

	strcpy(file1, m_LibraryPath);
	strcat(file1, "tex-i.old");
	remove(file1);
	strcpy(file2, m_LibraryPath);
	strcat(file2, "textures.idx");
	rename(file2, file1);

	if ((!oldidx.Open(file1, "rb")) || (!newidx.Open(file2, "wb")))
		return false;

	// Write the headers
	newidx.Write (TexturesIdxHeader, sizeof (TexturesIdxHeader));
	bt = 1; // version
	newidx.WriteByte (&bt, 1);
	bt = 0; // last update (unused for now)
	newidx.WriteByte (&bt, 1);

	newbin.Write (TexturesBinHeader, sizeof (TexturesBinHeader));
	offset += sizeof (TexturesBinHeader);

	oldidx.Seek(-(long)(sizeof(count)+sizeof(binsize)), SEEK_END);
	oldidx.ReadLong (&binsize, 1);
	oldidx.ReadShort (&count, 1);
	oldidx.Seek(32, SEEK_SET);
	oldidx.ReadByte (&version, 1);

	if ((version != TexturesFileVersion) || (count == 0))
		return false;

	oldidx.Seek(34, SEEK_SET); // skip update byte

	for (i = 0; i < count; i++)
	{
		lcuint32 OldOffset, FileSize = 0;
		lcuint16 Width, Height;
		char TexName[9];
		TexName[8] = 0;

		oldidx.Read (TexName, 8);
		oldidx.ReadShort (&Width, 1);
		oldidx.ReadShort (&Height, 1);
		oldidx.ReadByte (&bt, 1);

		switch (bt)
		{
		case LC_INTENSITY:
			FileSize = Width*Height;
			break;

		case LC_RGB:
			FileSize = Width*Height*3;
			break;

		case LC_RGBA:
			FileSize = Width*Height*4;
			break;
		}

		oldidx.ReadLong(&OldOffset, 1);

		for (j = 0; j < NumTextures; j++)
			if (strcmp(TexName, Names[j]) == 0)
				break;

		if (j != NumTextures)
		{
			deleted++;
			continue;
		}

		// Write index for this texture
		newidx.Write (TexName, 8);
		newidx.WriteShort (&Width, 1);
		newidx.WriteShort (&Height, 1);
		newidx.WriteByte (&bt, 1);
		newidx.WriteLong (&offset, 1);

		offset += FileSize;

		// Copy texture data
		void *membuf = malloc (FileSize);
		oldbin.Seek (OldOffset, SEEK_SET);
		oldbin.Read (membuf, FileSize);
		newbin.Write (membuf, FileSize);
		free (membuf);
	}

	newidx.WriteLong (&offset, 1);
	count -= deleted;
	newidx.WriteShort (&count, 1);

	m_Modified = true;

	return true;
}

bool PiecesLibrary::ImportTexture (const char* Name)
{
	char file1[LC_MAXPATH], file2[LC_MAXPATH];
	FileDisk newbin, newidx, oldbin, oldidx;
	lcuint32 FileSize = 0, binsize, offset = 0;
	lcuint16 Width, Height, count, deleted = 0, i;
	lcuint8 version, bt;
	Image img;

	if (!img.FileLoad (Name))
		return false;

	// Backup files
	strcpy(file1, m_LibraryPath);
	strcat(file1, "tex-b.old");
	remove(file1);
	strcpy(file2, m_LibraryPath);
	strcat(file2, "textures.bin");
	rename(file2, file1);

	if ((!oldbin.Open(file1, "rb")) || (!newbin.Open(file2, "wb")))
		return false;

	strcpy(file1, m_LibraryPath);
	strcat(file1, "tex-i.old");
	remove(file1);
	strcpy(file2, m_LibraryPath);
	strcat(file2, "textures.idx");
	rename(file2, file1);

	if ((!oldidx.Open(file1, "rb")) || (!newidx.Open(file2, "wb")))
		return false;

	// Get the file name
	char* p, NewTexName[9];

	strcpy(file1, Name);
	p = strrchr(file1, '.');
	*p = 0;
	p = strrchr(file1, '\\');
	if (!p)
		p = strrchr(file1, '/');
	if (!p)
		p = file1;
	strupr(p);
	p++;

	memset(NewTexName, 0, 9);
	strcpy(NewTexName, p);

	if (FindTexture (NewTexName) != NULL)
	  Sys_MessageBox ("Texture already exists in the library !");

	// Write the headers
	newidx.Write (TexturesIdxHeader, sizeof (TexturesIdxHeader));
	bt = 1; // version
	newidx.WriteByte (&bt, 1);
	bt = 0; // last update (unused for now)
	newidx.WriteByte (&bt, 1);

	newbin.Write (TexturesBinHeader, sizeof (TexturesBinHeader));
	offset += sizeof (TexturesBinHeader);

	oldidx.Seek(-(long)(sizeof(count)+sizeof(binsize)), SEEK_END);
	oldidx.ReadLong (&binsize, 1);
	oldidx.ReadShort (&count, 1);
	oldidx.Seek(32, SEEK_SET);
	oldidx.ReadByte (&version, 1);

	if (version != TexturesFileVersion)
		return false;

	oldidx.Seek(34, SEEK_SET); // skip update byte

	for (i = 0; i < count; i++)
	{
		lcuint32 OldOffset;
		lcuint16 Width, Height;
		char TexName[9];
		TexName[8] = 0;

		oldidx.Read (TexName, 8);
		oldidx.ReadShort (&Width, 1);
		oldidx.ReadShort (&Height, 1);
		oldidx.ReadByte (&bt, 1);

		switch (bt)
		{
		case LC_INTENSITY:
			FileSize = Width*Height;
			break;

		case LC_RGB:
			FileSize = Width*Height*3;
			break;

		case LC_RGBA:
			FileSize = Width*Height*4;
			break;
		}

		oldidx.ReadLong(&OldOffset, 1);

		if (strcmp(TexName, NewTexName) == 0)
		{
			deleted++;
			continue;
		}

		// Write index for this texture
		newidx.Write (TexName, 8);
		newidx.WriteShort (&Width, 1);
		newidx.WriteShort (&Height, 1);
		newidx.WriteByte (&bt, 1);
		newidx.WriteLong (&offset, 1);

		offset += FileSize;

		// Copy texture data
		void *membuf = malloc (FileSize);
		oldbin.Seek (OldOffset, SEEK_SET);
		oldbin.Read (membuf, FileSize);
		newbin.Write (membuf, FileSize);
		free (membuf);
	}

	// Save the new texture
	Width = img.Width ();
	Height = img.Height ();
	count++;

	// TODO: The texture type should be an option when you choose the file but I'll leave it hardcoded for now.
	if (!strcmp (NewTexName, "SYSFONT"))
	{
		lcuint8* buf = img.GetData();
		int w = img.Alpha () ? 4 : 3;

		for (i = 0; i < Width*Height; i++)
		{
			if (buf[i*w] > 0 || buf[i*w+1] > 0 || buf[i*w+2] > 0)
				bt = 255;
			else
				bt = 0;

			newbin.WriteByte (&bt, 1);
		}

		FileSize = Width*Height;
		bt = LC_INTENSITY;
	}
	else
	{
		if (img.Alpha ())
		{
			FileSize = Width*Height*4;
			bt = LC_RGBA;
		}
		else
		{
			FileSize = Width*Height*3;
			bt = LC_RGB;
		}

		newbin.Write (img.GetData(), FileSize);
	}

	newidx.Write (NewTexName, 8);
	newidx.WriteShort (&Width, 1);
	newidx.WriteShort (&Height, 1);
	newidx.WriteByte (&bt, 1);
	newidx.WriteLong (&offset, 1);

	offset += FileSize;

	newidx.WriteLong (&offset, 1);
	count -= deleted;
	newidx.WriteShort (&count, 1);

	m_Modified = true;

	return true;
}

// =============================================================================
// LDraw support

bool PiecesLibrary::ImportLDrawPiece (const char* Filename)
{
	LC_LDRAW_PIECE piece;

	SystemDoWaitCursor(1);

	if (ReadLDrawPiece (Filename, &piece))
	{
//		if (FindPieceInfo (piece.name) != NULL)
//			Sys_MessageBox ("Piece already exists in the library !");

		if (SaveLDrawPiece (&piece))
			Sys_MessageBox ("Piece successfully imported.");
		else
			Sys_MessageBox ("Error saving library.");
	}
	else
		Sys_MessageBox ("Error reading file.");

	FreeLDrawPiece(&piece);

	SystemDoWaitCursor(-1);

	return true;
}





























// =============================================================================
// Stuff that needs to be reorganized


#include <string.h>
#include <math.h>
#include "globals.h"
#include "project.h"
#include "matrix.h"


// =============================================================================
// LibraryDialog class

static const char ver_str[32] = "LeoCAD Group Configuration File";
static const float ver_flt = 0.3f;




// ========================================================
// Import LDraw piece

#define LC_MESH		1
#define LC_STUD		2
#define LC_STUD2	3
#define LC_STUD3	4
#define LC_STUD4	5

// stud, technic stud, stud under 1x? plate, stud under ?x? plate
static const char* valid[12] = { "STUD.DAT", "STUD2.DAT", "STUD3.DAT", "STUD4.DAT" };
static const unsigned char numvalid = 4;

static int FloatPointsClose(float pt1[], float pt2[])
{
	if (fabs(pt1[0] - pt2[0]) > 0.01)
		return 0;
	if (fabs(pt1[1] - pt2[1]) > 0.01)
		return 0;
	if (fabs(pt1[2] - pt2[2]) > 0.01)
		return 0;
	return 1;
}

static void ConvertPoints(float pts[], int count)
{
	float tmp;
	int i;

	for (i = 0; i < count; i++)
	{
		pts[3*i] /= 25;
		tmp = pts[3*i+1];
		pts[3*i+1] = pts[3*i+2]/25;
		pts[3*i+2] = -tmp/25;
	}
}

static void Resequence(float v[4][3], int a, int b, int c, int d)
{
	float o[4][3];
	memcpy(o, v, sizeof(o));
	memcpy(v[0], o[a], sizeof(o[0]));
	memcpy(v[1], o[b], sizeof(o[0]));
	memcpy(v[2], o[c], sizeof(o[0]));
	memcpy(v[3], o[d], sizeof(o[0]));
}

static void Sub3(float v[], float q1[], float q2[])
{
	v[0] = q1[0]-q2[0];
	v[1] = q1[1]-q2[1];
	v[2] = q1[2]-q2[2];
}

static float Dot3(float q1[], float q2[])
{
	return q1[0]*q2[0]+q1[1]*q2[1]+q1[2]*q2[2];
}

static void Cross3(float v[], float q1[], float q2[])
{
	v[0] = (q1[1]*q2[2]) - (q1[2]*q2[1]);
	v[1] = (q1[2]*q2[0]) - (q1[0]*q2[2]);
	v[2] = (q1[0]*q2[1]) - (q1[1]*q2[0]);
}

static void TestQuads(float quad[4][3])
{
	float v01[3], v02[3], v03[3];
	float v12[3], v13[3], v23[3];
	float cp1[3], cp2[3];
	float dotA, dotB, dotC;
	int A, B, C;
	
	// Calculate A
	Sub3(v01, quad[1], quad[0]);
	Sub3(v02, quad[2], quad[0]);
	Sub3(v03, quad[3], quad[0]);
	Cross3(cp1, v01, v02);
	Cross3(cp2, v02, v03);
	dotA = Dot3(cp1, cp2);
	A = (dotA > 0.0f);
	
	if (A)
	{
		// 3 is in I, typical case, OK: 0123 D02 (convex/concave)
		// CONVEXINFO: quad is convex if (!B && !C): OK: 0123 D02/13 (convex)
	}
	else
	{
		// Calculate B and C (may be postponed/discarded)
		// NOTE: postponed !
		Sub3(v12, quad[2], quad[1]);
		Sub3(v13, quad[3], quad[1]);
		Sub3(v23, quad[3], quad[2]);
		Cross3(cp1, v12, v01);
		Cross3(cp2, v01, v13);
		dotB = Dot3(cp1, cp2);
		B = (dotB > 0.0f);
		Cross3(cp1, v02, v12);
		Cross3(cp2, v12, v23);
		dotC = -Dot3(cp1, cp2);
		C = (dotC > 0.0f);

		// 3 is in II, III, IV or V. Calculation of B and C could be postponed
		//	to here if CONVEXINFO (above) is not needed
		if (B)
		{
			// 3 is in II or III
			if (C)
			{
				// 3 is in II, OK: 0123 D13 (concave)
				Resequence(quad, 1, 2, 3, 0); // just to shift diagonal
			}
			else
			{
				// 3 is in III, bow-tie error: using 0312 D01/D23 (convex)
				Resequence(quad, 0, 3, 1, 2);
			}
		}
		else
		{
			// 3 is in IV or V
			if (C)
			{
				// 3 is in IV, bow-tie error: using 0132 D12/D03 (convex)
				Resequence(quad, 0, 1, 3, 2);
			}
			else
			{
				// 3 is in V, OK: 0123 D13 (concave)
				Resequence(quad, 1, 2, 3, 0); // just to shift diagonal
			}
		}
	}
	// The four vertices quad[0], quad[1], quad[2] and quad[3] now have
	// the correct sequence, the polygon can be divided by the diagonal 02
	// into two triangles, 012 and 230.
}

static void FixQuads(float quad[])
{
	float t[4][3];
	memcpy(t, quad, sizeof(t));
	TestQuads(t);
	memcpy(quad, t, sizeof(t));
}

static group_t* NewGroup(LC_LDRAW_PIECE* piece)
{
	group_t* group;

	if (piece->groups)
	{
	 	group = piece->groups;
		while (group->next)
			group = group->next;
		group->next = (group_t*)malloc(sizeof(group_t));
		group = group->next;
	}
	else
	{
		group = piece->groups = (group_t*)malloc(sizeof(group_t));
	}

	memset(group, 0, sizeof(group_t));

	return group;
}

static connection_t* AddConnection(connection_t* newcon, LC_LDRAW_PIECE* piece)
{
	connection_t* con;

	con = piece->connections;
	while (con)
	{
		if ((con->type == newcon->type) &&
			FloatPointsClose(con->pos, newcon->pos) &&
			FloatPointsClose(con->up, newcon->up))
		{
			free(newcon);
			return con;
		}

		con = con->next;
	}

	if (piece->connections)
	{
		con = piece->connections;
		while (con->next)
			con = con->next;
		con->next = newcon;
	}
	else
	{
		piece->connections = newcon;
		newcon->next = NULL;
	}

	return newcon;
}

static void CreateMesh(group_t* pGroup, lineinfo_t* info, LC_LDRAW_PIECE* piece)
{
	lineinfo_t *a, *b;
	int i, j, k, v;
	unsigned int count[256][3], vert = 0;
	unsigned int quads = 0;
	unsigned char* bytes;
	memset (count, 0, sizeof(count));

	for (a = info->next; a; a = a->next)
	{
		// Fix the 'extended colors' that shouldn't be there in the first place.
		if ((a->color > 16) && (a->color < 24))
			a->color = 0;

		count[a->color][a->type-2]++;
		vert += a->type;
	}

	k = 0;
	for (i = 16; i < 256; i++)
	{
		if (count[i][0] || count[i][1] || count[i][2])
			k++;

		quads += count[i][2] * 4;

		if (i == 16) i = -1;
		if (i == 15) i = 23;
	}

	if (piece->verts_count > 65535 || quads > 65535)
	{
		piece->long_info = true;
		unsigned long* drawinfo;
		pGroup->infosize = sizeof(unsigned long)*(vert + (k*4)+1) + 2;
		pGroup->drawinfo = malloc(pGroup->infosize);
		bytes = (unsigned char*)pGroup->drawinfo;
		drawinfo = (unsigned long*)(bytes + 1);
		*bytes = LC_MESH;
		*drawinfo = k; // number colors
		drawinfo++;

		for (i = 16; i < 256; i++)
		{
			if (count[i][0] || count[i][1] || count[i][2])
			{
				*drawinfo = i;
				drawinfo++;

				for (j = 4; j > 1; j--)
				{
					*drawinfo = count[i][j-2]*j;
					drawinfo++;

					if (count[i][j-2] != 0)
					{
						a = info->next;
						b = info;
						while(a)
						if ((a->type == j) && (a->color == i))
						{
							for (k = 0; k < a->type; k++)
							for (v = 0; v < (int)piece->verts_count; v++)
								if (FloatPointsClose(&piece->verts[v*3], &a->points[k*3]))
								{
									*drawinfo = v;
									drawinfo++;
									break;
								}
									
							b->next = a->next;
							free(a);
							a = b->next;
						}
						else
						{
							b = a;
							a = a->next;
						}
					}
				}
			}

			if (i == 16) i = -1;
			if (i == 15) i = 23;
		}
	}
	else
	{
		piece->long_info = false;
		unsigned short* drawinfo;
		pGroup->infosize = sizeof(unsigned short)*(vert + (k*4)+1) + 2;
		pGroup->drawinfo = malloc(pGroup->infosize);
		bytes = (unsigned char*)pGroup->drawinfo;
		drawinfo = (unsigned short*)(bytes + 1);
		*bytes = LC_MESH;
		*drawinfo = k; // number colors
		drawinfo++;

		for (i = 16; i < 256; i++)
		{
			if (count[i][0] || count[i][1] || count[i][2])
			{
				*drawinfo = i;
				drawinfo++;
			}

			for (j = 4; j > 1; j--)
			{
				if (count[i][0] || count[i][1] || count[i][2])
				{
					*drawinfo = count[i][j-2]*j;
					drawinfo++;
				}

				if (count[i][j-2] != 0)
				{
					a = info->next;
					b = info;
					while(a)
					if ((a->type == j) && (a->color == i))
					{
						for (k = 0; k < a->type; k++)
						for (v = 0; v < (int)piece->verts_count; v++)
							if (FloatPointsClose(&piece->verts[v*3], &a->points[k*3]))
							{
								*drawinfo = v;
								drawinfo++;
								break;
							}
									
						b->next = a->next;
						free(a);
						a = b->next;
					}
					else
					{
						b = a;
						a = a->next;
					}
				}
			}

			if (i == 16) i = -1;
			if (i == 15) i = 23;
		}
	}

	bytes[pGroup->infosize-1] = 0; // End
}

static void decodefile(FILE *F, Matrix *mat, unsigned char defcolor, lineinfo_t* info, char* dir, LC_LDRAW_PIECE* piece)
{
	char buf[1024], fn[LC_MAXPATH], filename[32];
	unsigned char val;
	int type, color;
	float fm[12];
	FILE *tf;

	while (fgets(buf, 1024, F))
	{
		while (buf[strlen(buf)-1] == 10 || buf[strlen(buf)-1] == 13 || buf[strlen(buf)-1] == 32)
			buf[strlen(buf)-1] = 0;

		type = -1;
		sscanf(buf, "%d", &type);

		if (type == 6)
		{
			float* f;

			texture_t* tex;
			if (piece->textures)
			{
				tex = piece->textures;
				while (tex->next)
					tex = tex->next;
				tex->next = (texture_t*)malloc(sizeof(texture_t));
				tex = tex->next;
			}
			else
			{
				piece->textures = (texture_t*)malloc(sizeof(texture_t));
				tex = piece->textures;
			}
			memset(tex, 0, sizeof(texture_t));
			f = tex->points;

			sscanf (buf, "%d %d %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %s",
				&type, &color, &f[0], &f[1], &f[2], &f[3], &f[4], &f[5], &f[6], &f[7], &f[8], &f[9],
				&f[10], &f[11], &f[12], &f[13], &f[14], &f[15], &f[16], &f[17], &f[18], &f[19], tex->name);
			tex->color = color;
			ConvertPoints(f, 4);

			continue;
		}

		if (type > 1 && type < 5)
		{
			lineinfo_t* newinfo = (lineinfo_t*)malloc(sizeof(lineinfo_t));
			info->next = newinfo;
			newinfo->next = 0;
			newinfo->type = type;
			info = newinfo;
		}

		switch (type)
		{
		case 1:
		{
			sscanf (buf, "%d %d %f %f %f %f %f %f %f %f %f %f %f %f %s",
				&type, &color, &fm[0], &fm[1], &fm[2], &fm[3], &fm[4], &fm[5], &fm[6], &fm[7], &fm[8], &fm[9], &fm[10], &fm[11], filename);

			strcpy (fn, dir);
			strcat (fn, "p/");
			strcat (fn, filename);

			strupr(filename);
			for (val = 0; val < numvalid; val++)
				if (strcmp(filename, valid[val]) == 0)
					break;
			if (val != numvalid)
				break;

			if (color == 16) color = defcolor;

			tf = fopen (fn, "rt");

			if (!tf)
			{
				strcpy (fn, dir);
				strcat (fn, "parts/");
				strcat (fn, filename);
				tf = fopen (fn, "rt");
			}

			if (!tf)
			{
				strcpy (fn, dir);
				strcat (fn, "parts/s/");
				strcat (fn, filename);
				tf = fopen (fn, "rt");
			}

			if (tf)
			{
				Matrix m1, m2;
				m1.FromLDraw(fm);
				m2.Multiply(*mat, m1);

				decodefile(tf, &m2, (unsigned char)color, info, dir, piece);
				while (info->next)
					info = info->next;
				fclose(tf);
			}
		} break;

		case 2:
		{
			sscanf (buf, "%d %d %f %f %f %f %f %f", &type, &color, 
				&info->points[0], &info->points[1], &info->points[2],
				&info->points[3], &info->points[4], &info->points[5]);
			if (color == 16) color = defcolor;
			if (color > 256) color -= 256;
			info->color = color;
			ConvertPoints(info->points, 2);
			mat->TransformPoints(info->points, 2);
		} break;

		case 3:
		{
			sscanf (buf, "%d %d %f %f %f %f %f %f %f %f %f", &type, &color, 
				&info->points[0], &info->points[1], &info->points[2],
				&info->points[3], &info->points[4], &info->points[5],
				&info->points[6], &info->points[7], &info->points[8]);
			if (color == 16) color = defcolor;
			if (color > 256) color -= 256;
			info->color = color;
			ConvertPoints(info->points, 3);
			mat->TransformPoints(info->points, 3);
		} break;

		case 4:
		{
			sscanf (buf, "%d %d %f %f %f %f %f %f %f %f %f %f %f %f", &type, &color, 
				&info->points[0], &info->points[1], &info->points[2],
				&info->points[3], &info->points[4], &info->points[5],
				&info->points[6], &info->points[7], &info->points[8],
				&info->points[9], &info->points[10], &info->points[11]);
			if (color == 16) color = defcolor;
			if (color > 256) color -= 256;
			info->color = color;
			ConvertPoints(info->points, 4);
			mat->TransformPoints(info->points, 4);
			FixQuads(info->points);

#ifdef TRIANGULATE
			LINEINFO* newinfo = (LINEINFO*)malloc(sizeof(LINEINFO));
			info->next = newinfo;
			info->type = 3;
			newinfo->next = NULL;
			newinfo->type = 3;
			newinfo->color = color;
			newinfo->points[0] = info->points[6];
			newinfo->points[1] = info->points[7];
			newinfo->points[2] = info->points[8];
			newinfo->points[3] = info->points[9];
			newinfo->points[4] = info->points[10];
			newinfo->points[5] = info->points[11];
			newinfo->points[6] = info->points[0];
			newinfo->points[7] = info->points[1];
			newinfo->points[8] = info->points[2];
			info = newinfo;
#endif
		} break;

		}
		memset (buf, 0, sizeof(buf));
	}
}

static void decodeconnections(FILE *F, Matrix *mat, unsigned char defcolor, char* dir, LC_LDRAW_PIECE* piece)
{
	char buf[1024], fn[LC_MAXPATH], filename[32];
	unsigned char val, *bytes;
	float fm[12], *floats;
	int type, color;
	group_t* group;
	connection_t* con;
	Matrix m1, m2;
	FILE *tf;

	while (fgets(buf, 1024, F))
	{
		while (buf[strlen(buf)-1] == 10 || buf[strlen(buf)-1] == 13 || buf[strlen(buf)-1] == 32)
			buf[strlen(buf)-1] = 0;

		type = -1;
		sscanf(buf, "%d", &type);

		if (type != 1)
		{
			memset (buf, 0, sizeof(buf));
			continue;
		}

		sscanf (buf, "%d %d %f %f %f %f %f %f %f %f %f %f %f %f %s",
			&type, &color, &fm[0], &fm[1], &fm[2], &fm[3], &fm[4], &fm[5], &fm[6], &fm[7], &fm[8], &fm[9], &fm[10], &fm[11], filename);

		strcpy (fn, dir);
		strcat (fn, "P/");
		strcat (fn, filename);

		if (color == 16) color = defcolor;

		strupr(filename);
		for (val = 0; val < numvalid; val++)
		if (strcmp(filename, valid[val]) == 0)
		{
			m1.LoadIdentity();
			m2.LoadIdentity();
			m1.FromLDraw(fm);
			m2.Multiply(*mat, m1);

			if (val == 0) // STUD.DAT
			{
				group = NewGroup(piece);
				con = (connection_t*)malloc(sizeof(connection_t));
				memset(con, 0, sizeof(connection_t));

				group->infosize = 3*sizeof(unsigned char) + 12*sizeof(float);
				group->drawinfo = malloc(group->infosize);
				bytes = (unsigned char*)group->drawinfo;
				floats = (float*)(bytes+2);

				bytes[0] = LC_STUD;
				bytes[1] = color; // color
				floats[0] = m2.m[0];
				floats[1] = m2.m[1];
				floats[2] = m2.m[2];
				floats[3] = m2.m[4];
				floats[4] = m2.m[5];
				floats[5] = m2.m[6];
				floats[6] = m2.m[8];
				floats[7] = m2.m[9];
				floats[8] = m2.m[10];
				floats[9] = m2.m[12];
				floats[10] = m2.m[13];
				floats[11] = m2.m[14];
				bytes[group->infosize-1] = 0; // end

				con->type = 0; // stud
				con->pos[0] = m2.m[12];
				con->pos[1] = m2.m[13];
				con->pos[2] = m2.m[14];
				con->up[2] = 1;
				m2.TransformPoints(con->up, 1);
				con->up[0] -= m2.m[12];
				con->up[1] -= m2.m[13];
				con->up[2] -= m2.m[14];

				con = AddConnection(con, piece);
				group->connections[0] = con;
			}

			if (val == 1) // STUD2.DAT
			{
				group = NewGroup(piece);
				con = (connection_t*)malloc(sizeof(connection_t));
				memset(con, 0, sizeof(connection_t));

				group->infosize = 3*sizeof(unsigned char) + 12*sizeof(float);
				group->drawinfo = malloc(group->infosize);
				bytes = (unsigned char*)group->drawinfo;
				floats = (float*)(bytes+2);

				bytes[0] = LC_STUD2;
				bytes[1] = color; // color
				floats[0] = m2.m[0];
				floats[1] = m2.m[1];
				floats[2] = m2.m[2];
				floats[3] = m2.m[4];
				floats[4] = m2.m[5];
				floats[5] = m2.m[6];
				floats[6] = m2.m[8];
				floats[7] = m2.m[9];
				floats[8] = m2.m[10];
				floats[9] = m2.m[12];
				floats[10] = m2.m[13];
				floats[11] = m2.m[14];
				bytes[group->infosize-1] = 0; // end

				con->type = 0; // stud
				con->pos[0] = m2.m[12];
				con->pos[1] = m2.m[13];
				con->pos[2] = m2.m[14];
				con->up[2] = 1;
				m2.TransformPoints(con->up, 1);
				con->up[0] -= m2.m[12];
				con->up[1] -= m2.m[13];
				con->up[2] -= m2.m[14];

				con = AddConnection(con, piece);
				group->connections[0] = con;
			}

			if (val == 2) // STUD3.DAT
			{
				group = NewGroup(piece);
				group->infosize = 3*sizeof(unsigned char) + 12*sizeof(float);
				group->drawinfo = malloc(group->infosize);
				bytes = (unsigned char*)group->drawinfo;
				floats = (float*)(bytes+2);

				bytes[0] = LC_STUD3;
				bytes[1] = color; // color
				floats[0] = m2.m[0];
				floats[1] = m2.m[1];
				floats[2] = m2.m[2];
				floats[3] = m2.m[4];
				floats[4] = m2.m[5];
				floats[5] = m2.m[6];
				floats[6] = m2.m[8];
				floats[7] = m2.m[9];
				floats[8] = m2.m[10];
				floats[9] = m2.m[12];
				floats[10] = m2.m[13];
				floats[11] = m2.m[14];
				bytes[group->infosize-1] = 0; // end
			}

			if (val == 3) // STUD4.DAT
			{
				float t[4][3] = { {0.4f,0.4f,0}, {-0.4f,0.4f,0}, {0.4f,-0.4f,0}, {-0.4f,-0.4f,0} };
				int c;

				group = NewGroup(piece);
				group->infosize = 3*sizeof(unsigned char) + 12*sizeof(float);
				group->drawinfo = malloc(group->infosize);
				bytes = (unsigned char*)group->drawinfo;
				floats = (float*)(bytes+2);

				bytes[0] = LC_STUD4;
				bytes[1] = color; // color
				floats[0] = m2.m[0];
				floats[1] = m2.m[1];
				floats[2] = m2.m[2];
				floats[3] = m2.m[4];
				floats[4] = m2.m[5];
				floats[5] = m2.m[6];
				floats[6] = m2.m[8];
				floats[7] = m2.m[9];
				floats[8] = m2.m[10];
				floats[9] = m2.m[12];
				floats[10] = m2.m[13];
				floats[11] = m2.m[14];
				bytes[group->infosize-1] = 0; // end

				for (c = 0; c < 4; c++)
				{
					con = (connection_t*)malloc(sizeof(connection_t));
					memset(con, 0, sizeof(connection_t));
					con->type = 1; // inv stud
					m2.TransformPoint(con->pos, t[c]);
					con->pos[2] -= 0.16f;

					con->up[2] = 1;
					m2.TransformPoints(con->up, 1);
					con->up[0] -= m2.m[12];
					con->up[1] -= m2.m[13];
					con->up[2] -= m2.m[14];

					group->connections[c] = AddConnection(con, piece);
				}

				con = (connection_t*)malloc(sizeof(connection_t));
				memset(con, 0, sizeof(connection_t));
				con->type = 1; // inv stud
				con->pos[2] -= 0.16f;

				con->up[2] = 1;
				m2.TransformPoints(con->up, 1);
				con->up[0] -= m2.m[12];
				con->up[1] -= m2.m[13];
				con->up[2] -= m2.m[14];

				AddConnection(con, piece);
			}

			memset (buf, 0, sizeof(buf));
			continue;
		}

		tf = fopen (fn, "rt");

		if (!tf)
		{
			strcpy(fn, dir);
			strcat(fn, "parts/");
			strcat(fn, filename);
			tf = fopen(fn, "rt");
		}

		if (!tf)
		{
			strcpy(fn, dir);
			strcat(fn, "parts/s/");
			strcat(fn, filename);
			tf = fopen(fn, "rt");
		}

		if (tf)
		{
			m1.LoadIdentity();
			m2.LoadIdentity();
			m1.FromLDraw(fm);
			m2.Multiply(*mat, m1);

			decodeconnections (tf, &m2, (unsigned char)color, dir, piece);
//			while (info->next)
//				info = info->next;
			fclose(tf);
		}

		memset (buf, 0, sizeof(buf));
	}
}

bool ReadLDrawPiece(const char* filename, LC_LDRAW_PIECE* piece)
{
	unsigned long j, unique;
	char tmp[LC_MAXPATH], *ptr;
	lineinfo_t info, *lf;
	float* verts;
	Matrix mat;
	FILE *f;

	memset(piece, 0, sizeof(LC_LDRAW_PIECE));
	info.next = 0;
	info.type = 0;

	// First we read the name and description
	strcpy(tmp, filename);
	ptr = strrchr(tmp, '.');
	if (ptr != NULL)
		*ptr = 0;
	ptr = strrchr(tmp, '\\');
	if (ptr == NULL)
		ptr = strrchr(tmp, '/');

	if (ptr == NULL) 
		ptr = tmp;
	else
		ptr++;
	strcpy(piece->name, ptr);
	strupr(piece->name);

	f = fopen(filename, "rt");
	if (f == NULL)
		return false;

	if (fgets(tmp, 100, f))
	{
		while (tmp[strlen(tmp)-1]==10||tmp[strlen(tmp)-1]==13||tmp[strlen(tmp)-1]==32)
			tmp[strlen(tmp)-1]=0;
		tmp[66] = 0;
		strcpy(piece->description, tmp+2);
	}

	// Get LDraw directory, piece must be /ldraw/pieces/foo.dat
	strcpy(tmp, filename);
	ptr = strrchr(tmp, '\\');
	if (ptr == NULL)
		ptr = strrchr(tmp, '/');
	*ptr = 0;
	ptr = strrchr(tmp, '\\');
	if (ptr == NULL)
		ptr = strrchr(tmp, '/');
	*(ptr+1) = 0;

	decodefile(f, &mat, 16, &info, tmp, piece);
	fclose (f);

	// Create array of unique vertices
	verts = (float*)malloc(sizeof(float)*1500);
	unique = 0;

	for (lf = info.next; lf; lf = lf->next)
	{
		for (j = 0; j < lf->type; j++)
		{
		  int i;
			for (i = unique-1; i != -1; i--)
				if (FloatPointsClose(&verts[i*3], &lf->points[j*3]))
					break;

			if (i == -1)
			{
				if ((unique % 500) == 0)
					verts = (float*)realloc(verts, sizeof(float)*3*(unique+500));
				memcpy(&verts[unique*3], &lf->points[j*3], sizeof(float)*3);
				unique++;
			}
		}
	}

	piece->verts = verts;
	piece->long_info = false;
	piece->verts_count = unique;

	// Main group
	piece->groups = (group_t*)malloc(sizeof(group_t));
	memset(piece->groups, 0, sizeof(group_t));
	CreateMesh(piece->groups, &info, piece);

	lf = info.next;
	while (lf)
	{
		lineinfo_t* b = lf->next;
		free(lf);
		lf = b;
	}
	info.next = NULL;

	// Included files
	f = fopen (filename, "rt");
	mat.LoadIdentity();
	decodeconnections(f, &mat, 16, tmp, piece);
	fclose(f);

	return true;
}

bool SaveLDrawPiece(LC_LDRAW_PIECE* piece)
{
	FileDisk newbin, newidx, oldbin, oldidx;
	char file1[LC_MAXPATH], file2[LC_MAXPATH];
	unsigned short count, moved;
	unsigned long i, j, cs, binoff = 0, delta;
	void* membuf;
	short scale, sb[6];
  PiecesLibrary *pLib = lcGetPiecesLibrary();

	strcpy(file1, pLib->GetLibraryPath());
	strcat(file1, "pieces-b.old");
	remove(file1);
	strcpy(file2, pLib->GetLibraryPath());
	strcat(file2, "pieces.bin");
	rename(file2, file1);

	if ((!oldbin.Open(file1, "rb")) ||
		(!newbin.Open(file2, "wb")))
		return false;

	strcpy(file1, pLib->GetLibraryPath());
	strcat(file1, "pieces-i.old");
	remove(file1);
	strcpy(file2, pLib->GetLibraryPath());
	strcat(file2, "pieces.idx");
	rename(file2, file1);

	if ((!oldidx.Open(file1, "rb")) ||
		(!newidx.Open(file2, "wb")))
		return false;


	oldidx.Seek(-2, SEEK_END);
	oldidx.ReadShort(&count, 1);
	oldidx.Seek(34, SEEK_SET);

	for (j = 0; j < count; j++)
	{
		char name[9];
		name[8] = 0;
		oldidx.Read(name, 8);
		if (strcmp(name, piece->name) == 0)
		{
			oldidx.Seek(64+12+1+4, SEEK_CUR);
			oldidx.ReadLong(&binoff, 1);
			oldidx.ReadLong(&delta, 1);
			oldidx.Seek(-(8+64+12+1+4+4+4), SEEK_CUR);
			delta += binoff;
			break;
		}
		oldidx.Seek(64+12+1+4+4+4, SEEK_CUR);
	}

	if (binoff == 0)
		binoff = oldbin.GetLength();

	cs = oldidx.GetPosition();
	membuf = malloc(cs);
	if (membuf == NULL)
	{
		SystemDoMessageBox("Not Enough Memory !", LC_MB_OK|LC_MB_ICONWARNING);
		return false;
	}

	oldidx.Seek(0, SEEK_SET);
	oldidx.Read(membuf, cs);
	newidx.Write(membuf, cs);
	free(membuf);

	membuf = malloc (binoff);
	if (membuf == NULL)
	{
		SystemDoMessageBox("Not Enough Memory !", LC_MB_OK|LC_MB_ICONWARNING);
		return false;
	}

	oldbin.Seek(0, SEEK_SET);
	oldbin.Read(membuf, binoff);
	newbin.Write(membuf, binoff);
	free(membuf);

	// Save piece
	float maxdim, box[6] = { -100, -100, -100, 100, 100, 100 };
	unsigned short s;
	unsigned char bt;
	connection_t* con;
	group_t* group;
	texture_t* tex;
	Matrix mat;

	// First we calculate the bounding box
 	group = piece->groups;
	while (group)
	{
		unsigned char* bytes = (unsigned char*)group->drawinfo;
		float* floats;

		while (*bytes)
		{
			if (*bytes == LC_MESH)
			{
				if (piece->long_info)
				{
					unsigned long colors, *p;
					p = (unsigned long*)(bytes + 1);
					colors = *p;
					p++;

					while (colors--)
					{
						p++; // color code
						p += *p + 1;
						p += *p + 1;
						p += *p + 1;
					}

					bytes = (unsigned char*)p;
				}
				else
				{
					unsigned short colors, *p;
					p = (unsigned short*)(bytes + 1);
					colors = *p;
					p++;

					while (colors--)
					{
						p++; // color code
						p += *p + 1;
						p += *p + 1;
						p += *p + 1;
					}

					bytes = (unsigned char*)p;
				}
			}

			if ((*bytes == LC_STUD) || (*bytes == LC_STUD2))
			{
				float stud[6] = { 0.16f, 0.16f, 0.16f, -0.16f, -0.16f, 0 };
				floats = (float*)(bytes+2);

				mat.LoadIdentity();
				mat.m[0] = floats[0];
				mat.m[1] = floats[1];
				mat.m[2] = floats[2];
				mat.m[4] = floats[3];
				mat.m[5] = floats[4];
				mat.m[6] = floats[5];
				mat.m[8] = floats[6];
				mat.m[9] = floats[7];
				mat.m[10] = floats[8];
				mat.m[12] = floats[9];
				mat.m[13] = floats[10];
				mat.m[14] = floats[11];
				mat.TransformPoints(stud, 2);

				for (i = 0; i < 2; i++)
				{
					if (stud[(3*i)]   > box[0]) box[0] = stud[(3*i)];
					if (stud[(3*i)+1] > box[1]) box[1] = stud[(3*i)+1];
					if (stud[(3*i)+2] > box[2]) box[2] = stud[(3*i)+2];
					if (stud[(3*i)]   < box[3]) box[3] = stud[(3*i)];
					if (stud[(3*i)+1] < box[4]) box[4] = stud[(3*i)+1];
					if (stud[(3*i)+2] < box[5]) box[5] = stud[(3*i)+2];
				}

				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
			}

			if ((*bytes == LC_STUD4) || (*bytes == LC_STUD3))
				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
		}

		group = group->next;
	}

	for (i = 0; i < piece->verts_count; i++)
	{
		if (piece->verts[(3*i)]   > box[0]) box[0] = piece->verts[(3*i)];
		if (piece->verts[(3*i)+1] > box[1]) box[1] = piece->verts[(3*i)+1];
		if (piece->verts[(3*i)+2] > box[2]) box[2] = piece->verts[(3*i)+2];
		if (piece->verts[(3*i)]   < box[3]) box[3] = piece->verts[(3*i)];
		if (piece->verts[(3*i)+1] < box[4]) box[4] = piece->verts[(3*i)+1];
		if (piece->verts[(3*i)+2] < box[5]) box[5] = piece->verts[(3*i)+2];
	}

	maxdim = 0;
	for (i = 0; i < 6; i++)
		if (fabs(box[i]) > maxdim) maxdim = (float)fabs(box[i]);
	scale = 10000;
	if (maxdim > 3.2f)
		scale = 1000;
	if (maxdim > 32.0f)
		scale = 100;

	// Write the vertex data
	newbin.WriteLong(&piece->verts_count, 1);
	for (i = 0; i < piece->verts_count; i++)
	{
		float tmp[3] = { scale*piece->verts[(i*3)], scale*piece->verts[(i*3)+1], scale*piece->verts[(i*3)+2] };
		short sh[3] = { (short)tmp[0], (short)tmp[1], (short)tmp[2] };
		newbin.WriteShort(&sh, 3);
	}

	// Write the connections information
	for (s = 0, con = piece->connections; con; con = con->next)
		s++;
	newbin.WriteShort(&s, 1);

	for (con = piece->connections; con; con = con->next)
	{
		float tmp[3] = { scale*con->pos[0], scale*con->pos[1], scale*con->pos[2] };
		short sh[3] = { (short)tmp[0], (short)tmp[1], (short)tmp[2] };

		newbin.WriteByte(&con->type, 1);
		newbin.WriteShort(sh, 3);

		sh[0] = (short)(con->up[0]*(1<<14));
		sh[1] = (short)(con->up[1]*(1<<14));
		sh[2] = (short)(con->up[2]*(1<<14));
		newbin.WriteShort(sh, 3);
	}

	// Textures
	for (bt = 0, tex = piece->textures; tex; tex = tex->next)
		bt++;
	newbin.WriteByte(&bt, 1);

	for (tex = piece->textures; tex; tex = tex->next)
	{
		newbin.WriteByte(&tex->color, 1);
		newbin.Write(tex->name, 8);

		for (i = 0; i < 12; i++)
		{
			float tmp[1] = { tex->points[i]*scale };
			short sh[1] = { (short)tmp[0] };
			newbin.WriteShort(sh, 1);
		}

		for (i = 12; i < 20; i++)
		{
			float tmp = tex->points[i];
			short sh[1] = { (short)tmp };
			newbin.WriteShort(sh, 1);
		}
	}

	for (s = 0, group = piece->groups; group; group = group->next)
		s++;
	newbin.WriteShort(&s, 1);

	for (group = piece->groups; group; group = group->next)
	{
		for (bt = 0; bt < 5; bt++)
			if (!group->connections[bt])
				break;
		newbin.WriteByte(&bt, 1);

		for (bt = 0; bt < 5; bt++)
		{
			if (!group->connections[bt])
				break;

			for (s = 0, con = piece->connections; con; con = con->next, s++)
				if (con == group->connections[bt])
					break;
			newbin.WriteShort(&s, 1);
		}
// TODO: make this endian-safe
		newbin.Write(group->drawinfo, group->infosize);
	}

	// Now write the index
	newidx.Write(piece->name, 8);
	newidx.Write(piece->description, 64);

	for (i = 0; i < 6; i++)
	{
		float ff, f = 1.0f * box[i];
		f *= scale;
		ff = f;
//		sb[i] = scale*box[i];
		sb[i] = (short)ff;
	}
	newidx.WriteShort(sb, 6);

	// Calculate flags.
	bt = LC_PIECE_COUNT;

	if (scale == 10000)
		bt |= LC_PIECE_SMALL;
	else if (scale == 1000)
		bt |= LC_PIECE_MEDIUM;

	if (piece->long_info)
		bt |= LC_PIECE_LONGDATA;

	newidx.WriteByte(&bt, 1);

	i = 0;//PiecesLibrary::GetDefaultPieceGroup(piece->description);
	newidx.WriteLong(&i, 1);
	newidx.WriteLong(&binoff, 1);

	i = newbin.GetLength() - binoff;
	newidx.WriteLong(&i, 1);

	// replacing a piece
	if (j != count)
	{
		unsigned long d = newbin.GetPosition() - delta;
		oldidx.Seek (8+64+12+1+4+4+4, SEEK_CUR);
		for (j++; j < count; j++)
		{
			unsigned long dw;
			char buf[8+64+12+1+4];
			oldidx.Read(buf, 8+64+12+1+4);
			oldidx.ReadLong(&dw, 1);
			dw += d;
			newidx.Write(buf, 8+64+12+1+4);
			newidx.WriteLong(&dw, 1);
			oldidx.ReadLong(&dw, 1);
			newidx.WriteLong(&dw, 1);
		}

		d = oldbin.GetLength()-delta;
		membuf = malloc (d);
		oldbin.Seek (delta, SEEK_SET);
		oldbin.Read (membuf, d);
		newbin.Write(membuf, d);
		free(membuf);
	}
	else
		count++;

	// Fix the end of the index
	oldidx.Seek(-(2+4+2), SEEK_END);
	oldidx.ReadShort(&moved, 1);
	cs = 2+(moved*16);
	oldidx.Seek(-(long)cs, SEEK_CUR);
	membuf = malloc(cs);
	oldidx.Read(membuf, cs);
	newidx.Write(membuf, cs);
	free(membuf);

	binoff = newbin.GetPosition();
	newidx.WriteLong(&binoff, 1);
	newidx.WriteShort(&count, 1);

	oldidx.Close();
	oldbin.Close();
	newidx.Close();
	newbin.Close();

	return true;
}

void FreeLDrawPiece(LC_LDRAW_PIECE* piece)
{
	group_t *tmp, *pg = piece->groups;
	connection_t *ctmp, *pc = piece->connections;
	texture_t *ttmp, *pt = piece->textures;

	free(piece->verts);

	while (pt)
	{
		ttmp = pt->next;
		free(pt);
		pt = ttmp;
	}

	while (pg != NULL)
	{
		free (pg->drawinfo);
		tmp = pg;
		pg = pg->next;
		free (tmp);
	}

	while (pc != NULL)
	{
		ctmp = pc;
		pc = pc->next;
		free (ctmp);
	}
}

// ========================================================

