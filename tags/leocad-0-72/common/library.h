#ifndef _MISC_H_
#define _MISC_H_

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

#endif // _MISC_H_
