#include <stdio.h>
#include <io.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include "convert.h"

typedef struct CONNECTION
{
	unsigned char type;
	float pos[3];
	float up[3];
	struct CONNECTION* next;
} CONNECTION;

typedef struct GROUP
{
	CONNECTION* connections[5];
	void* drawinfo;
	unsigned long infosize;
	struct GROUP* next;
} GROUP;

typedef struct lineinfo_s
{
	unsigned char type;
	unsigned char color;
	float points[12];
	struct lineinfo_s* next;
} lineinfo_t;

typedef struct TEXTURE
{
	float points[20];
	unsigned char color;
	char name[9];
	struct TEXTURE* next;
} TEXTURE;

float* _fVerts;
unsigned int _nVertsCount;
CONNECTION* _pConnections;
GROUP* _pGroups;
TEXTURE* _pTextures;
char _strName[9];
char _strDescription[65];

// stud, technic stud, stud under 1x? plate, stud under ?x? plate
char* valid[12] = { "STUD.DAT", "STUD2.DAT", "STUD3.DAT", "STUD4.DAT" };
unsigned char numvalid = 4;

#define LC_MESH		1
#define LC_STUD		2
#define LC_STUD2	3
#define LC_STUD3	4
#define LC_STUD4	5

/////////////////////////////////////////////////////
// functions

static unsigned long GetDefaultPieceGroup(char* name)
{
	char tmp[9];
	strncpy (tmp, name, 9);
//	tmp[8] = 0;

	if (strstr(tmp,"Baseplate")	|| strstr(tmp,"Plate")	|| 
		strstr(tmp,"Platform")) 
		return 0x001;
	
	if (strstr(tmp,"Brick")	|| strstr(tmp,"Cylin") ||
		strstr(tmp,"Cone"))
		return 0x002;
	
	if (strstr(tmp,"Tile"))
		return 0x004;
	
	if (strstr(tmp,"Slope"))
		return 0x008;
	
	if (strstr(tmp,"Technic")	|| strstr(tmp,"Crane")	||
		strstr(tmp,"Wheel")		|| strstr(tmp,"Tyre")	||
		strstr(tmp,"Electric"))
		return 0x010;
	
	// space & plane
	if (strstr(tmp,"Space")		|| strstr(tmp,"Plane")	||
		strstr(tmp,"Windscr")	|| strstr(tmp,"~2421")	||
		strstr(tmp,"Wing")		|| strstr(tmp,"Wedge")	||
		strstr(tmp,"Propellor")	|| strstr(tmp,"Rotor")	||
		strstr(tmp,"Rack")		|| strstr(tmp,"Tail"))
		return 0x020;
	
	if (strstr(tmp,"Train"))
		return 0x040;
	
	// other parts
	if (strstr(tmp,"Arch")		|| strstr(tmp,"Panel")	||
		strstr(tmp,"Car")		|| strstr(tmp,"Window")	||
		strstr(tmp,"Freestyle")	|| strstr(tmp,"Support")||
		strstr(tmp,"Fence")		|| strstr(tmp,"Gate")	|| 
		strstr(tmp,"Garage")	|| strstr(tmp,"Stairs")	||
		strstr(tmp,"Bracket")	|| strstr(tmp,"Hinge")	||
		strstr(tmp,"Homemaker")	|| strstr(tmp,"Rock")	|| 
		strstr(tmp,"Cupboard")	|| strstr(tmp,"Storage")||
		strstr(tmp,"Scala")		|| strstr(tmp,"Boat")	||
		strstr(tmp,"Trailer")	|| strstr(tmp,"Box")	|| 
		strstr(tmp,"Turntab")	|| strstr(tmp,"Winch")	|| 
		strstr(tmp,"Door")		|| strstr(tmp,"Magnet"))
		return 0x080;
	
	// accessories
	if (strstr(tmp,"Minifig")	|| strstr(tmp,"Antenna")||
		strstr(tmp,"Ladder")	|| strstr(tmp,"Jack")	||
		strstr(tmp,"Exhaust")	|| strstr(tmp,"Lever")	||
		strstr(tmp,"Roadsign")	|| strstr(tmp,"Town")	|| 
		strstr(tmp,"Leaves")	|| strstr(tmp,"Horse")	||
		strstr(tmp,"Tree")		|| strstr(tmp,"Flower")	||
		strstr(tmp,"Plant")		|| 
		strstr(tmp,"Conveyor")	|| strstr(tmp,"Tractor")|| 
		strstr(tmp,"Grab")		|| strstr(tmp,"Roller")	|| 
		strstr(tmp,"Stretch")	|| strstr(tmp,"Tap ")	|| 
		strstr(tmp,"Forklift")	|| strstr(tmp,"Flag")	|| 
		strstr(tmp,"Belville")	|| strstr(tmp,"Light &")|| 
		strstr(tmp,"Hose")		|| strstr(tmp,"Arm P")	|| 
		strstr(tmp,"Brush")		|| strstr(tmp,"Castle")	||
		strstr(tmp,"Tipper")	|| strstr(tmp,"Bar"))
		return 0x100;
	
	return 1;
}

static __inline GROUP* NewGroup()
{
	GROUP* pGroup;

	if (_pGroups)
	{
	 	pGroup = _pGroups;
		while (pGroup->next)
			pGroup = pGroup->next;
		pGroup->next = (GROUP*)malloc(sizeof(GROUP));
		pGroup = pGroup->next;
	}
	else
	{
		_pGroups = (GROUP*)malloc(sizeof(GROUP));
		pGroup = _pGroups;
	}

	memset(pGroup, 0, sizeof(GROUP));

	return pGroup;
}

static __inline CONNECTION* AddConnection(CONNECTION* pNew)
{
	CONNECTION* pCon;

	pCon = _pConnections;
	while (pCon)
	{
		if ((pCon->type == pNew->type) &&
			FloatPointsClose(pCon->pos, pNew->pos) &&
			FloatPointsClose(pCon->up, pNew->up))
		{
			free(pNew);
			return pCon;
		}

		pCon = pCon->next;
	}

	if (_pConnections)
	{
		pCon = _pConnections;
		while (pCon->next)
			pCon = pCon->next;
		pCon->next = pNew;
	}
	else
	{
		_pConnections = pNew;
		pNew->next = NULL;
	}

	return pNew;
}

void CleanUp()
{
	GROUP *tmp, *pg = _pGroups;
	CONNECTION *ctmp, *pc = _pConnections;
	TEXTURE *ttmp, *pt = _pTextures;
	_nVertsCount = 0;
	_pGroups = NULL;
	_pConnections = NULL;
	_pTextures = NULL;

	if (_fVerts != NULL)
	{
		free(_fVerts);
		_fVerts = NULL;
	}

	while (pt)
	{
		ttmp = pt->next;
		free(pt);
		pt = ttmp;
	};

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

void CreateMesh(GROUP* pGroup, lineinfo_t* info)
{
	lineinfo_t *a, *b;
	int i, j, k, v;
	unsigned int count[256][3], vert = 0;
	unsigned char* bytes;
	memset (count, 0, sizeof(count));

	for (a = info->next; a; a = a->next)
	{
		count[a->color][a->type-2]++;
		vert += a->type;
	}

	k = 0;
	for (i = 0; i < 256; i++)
	{
		if (count[i][0] || count[i][1] || count[i][2])
			k++;
	}

	if (_nVertsCount > 65535)
	{
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

				printf("%d ", i);

				for (j = 4; j > 1; j--)
				{
					*drawinfo = count[i][j-2]*j;
					drawinfo++;
					printf("%d ", count[i][j-2]*j);

					if (count[i][j-2] != 0)
					{
						a = info->next;
						b = info;
						while(a)
						if ((a->type == j) && (a->color == i))
						{
							for (k = 0; k < a->type; k++)
							for (v = 0; v < (int)_nVertsCount; v++)
								if (FloatPointsClose(&_fVerts[v*3], &a->points[k*3]))
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

		printf("\n");
	}
	else
	{
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
						for (v = 0; v < (int)_nVertsCount; v++)
							if (FloatPointsClose(&_fVerts[v*3], &a->points[k*3]))
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

void decodefile (FILE *F, matrix *mat, unsigned char defcolor, lineinfo_t* info, char* dir)
{
	char buf[1024];
	int type, color;
	char fn[260], filename[32];
	float fm[12];
	unsigned char val;
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

			TEXTURE* tex;
			if (_pTextures)
			{
				tex = _pTextures;
				while (tex->next)
					tex = tex->next;
				tex->next = (TEXTURE*)malloc(sizeof(TEXTURE));
				tex = tex->next;
			}
			else
			{
				_pTextures = (TEXTURE*)malloc(sizeof(TEXTURE));
				tex = _pTextures;
			}
			memset(tex, 0, sizeof(TEXTURE));
			f = tex->points;

			sscanf (buf, "%d %d %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %s",
				&type, &color, &f[0], &f[1], &f[2], &f[3], &f[4], &f[5], &f[6], &f[7], &f[8], &f[9],
				&f[10], &f[11], &f[12], &f[13], &f[14], &f[15], &f[16], &f[17], &f[18], &f[19], tex->name);
			tex->color = color;
			ConvertPoints (f, 4);

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
			strcat (fn, "P\\");
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
				strcat (fn, "PARTS\\");
				strcat (fn, filename);
				tf = fopen (fn, "rt");
			}

			if (!tf)
			{
				strcpy (fn, dir);
				strcat (fn, "PARTS\\S\\");
				strcat (fn, filename);
				tf = fopen (fn, "rt");
			}

			if (tf)
			{
				matrix m1, m2;
				LoadIdentity(&m1);
				LoadIdentity(&m2);
				ConvertFromLDraw (&m1, fm);
				Multiply(&m2, mat, &m1);

				decodefile (tf, &m2, (unsigned char)color, info, dir);
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
			ConvertPoints (info->points, 2);
			TransformPoints (mat, info->points, 2);
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
			ConvertPoints (info->points, 3);
			TransformPoints (mat, info->points, 3);
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
			ConvertPoints (info->points, 4);
			TransformPoints (mat, info->points, 4);
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

void decodeconnections (FILE *F, matrix *mat, unsigned char defcolor, char* dir)
{
	char buf[1024];
	int type, color;
	char fn[260], filename[32];
	float fm[12];
	matrix m1, m2;
	unsigned char val;
	FILE *tf;
	GROUP* pGroup;
	CONNECTION* pCon;
	unsigned char* bytes;
	float* floats;

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
		strcat (fn, "P\\");
		strcat (fn, filename);

		if (color == 16) color = defcolor;

		strupr(filename);
		for (val = 0; val < numvalid; val++)
		if (strcmp(filename, valid[val]) == 0)
		{
			LoadIdentity(&m1);
			LoadIdentity(&m2);
			ConvertFromLDraw (&m1, fm);
			Multiply (&m2, mat, &m1);

			if (val == 0) // STUD.DAT
			{
				pGroup = NewGroup();
				pCon = (CONNECTION*)malloc(sizeof(CONNECTION));
				memset(pCon, 0, sizeof(CONNECTION));

				pGroup->infosize = 3*sizeof(unsigned char) + 12*sizeof(float);
				pGroup->drawinfo = malloc(pGroup->infosize);
				bytes = pGroup->drawinfo;
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
				bytes[pGroup->infosize-1] = 0; // end

				pCon->type = 0; // stud
				pCon->pos[0] = m2.m[12];
				pCon->pos[1] = m2.m[13];
				pCon->pos[2] = m2.m[14];
				pCon->up[2] = 1;
				TransformPoints(&m2, pCon->up, 1);
				pCon->up[0] -= m2.m[12];
				pCon->up[1] -= m2.m[13];
				pCon->up[2] -= m2.m[14];

				pCon = AddConnection(pCon);
				pGroup->connections[0] = pCon;
			}

			if (val == 1) // STUD2.DAT
			{
				pGroup = NewGroup();
				pCon = (CONNECTION*)malloc(sizeof(CONNECTION));
				memset(pCon, 0, sizeof(CONNECTION));

				pGroup->infosize = 3*sizeof(unsigned char) + 12*sizeof(float);
				pGroup->drawinfo = malloc(pGroup->infosize);
				bytes = pGroup->drawinfo;
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
				bytes[pGroup->infosize-1] = 0; // end

				pCon->type = 0; // stud
				pCon->pos[0] = m2.m[12];
				pCon->pos[1] = m2.m[13];
				pCon->pos[2] = m2.m[14];
				pCon->up[2] = 1;
				TransformPoints(&m2, pCon->up, 1);
				pCon->up[0] -= m2.m[12];
				pCon->up[1] -= m2.m[13];
				pCon->up[2] -= m2.m[14];

				pCon = AddConnection(pCon);
				pGroup->connections[0] = pCon;
			}

			if (val == 2) // STUD3.DAT
			{
				pGroup = NewGroup();
				pGroup->infosize = 3*sizeof(unsigned char) + 12*sizeof(float);
				pGroup->drawinfo = malloc(pGroup->infosize);
				bytes = pGroup->drawinfo;
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
				bytes[pGroup->infosize-1] = 0; // end
			}

			if (val == 3) // STUD4.DAT
			{
				float t[4][3] = { {0.4f,0.4f,0}, {-0.4f,0.4f,0}, {0.4f,-0.4f,0}, {-0.4f,-0.4f,0} };
				int c;

				pGroup = NewGroup();
				pGroup->infosize = 3*sizeof(unsigned char) + 12*sizeof(float);
				pGroup->drawinfo = malloc(pGroup->infosize);
				bytes = pGroup->drawinfo;
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
				bytes[pGroup->infosize-1] = 0; // end

				for (c = 0; c < 4; c++)
				{
					pCon = (CONNECTION*)malloc(sizeof(CONNECTION));
					memset(pCon, 0, sizeof(CONNECTION));
					pCon->type = 1; // inv stud
					TranslatePoint(&m2, pCon->pos, t[c]);
					pCon->pos[2] -= 0.16f;

					pCon->up[2] = 1;
					TransformPoints(&m2, pCon->up, 1);
					pCon->up[0] -= m2.m[12];
					pCon->up[1] -= m2.m[13];
					pCon->up[2] -= m2.m[14];

					pGroup->connections[c] = AddConnection(pCon);
				}

				pCon = (CONNECTION*)malloc(sizeof(CONNECTION));
				memset(pCon, 0, sizeof(CONNECTION));
				pCon->type = 1; // inv stud
				pCon->pos[2] -= 0.16f;

				pCon->up[2] = 1;
				TransformPoints(&m2, pCon->up, 1);
				pCon->up[0] -= m2.m[12];
				pCon->up[1] -= m2.m[13];
				pCon->up[2] -= m2.m[14];

				AddConnection(pCon);
			}

			memset (buf, 0, sizeof(buf));
			continue;
		}

		tf = fopen (fn, "rt");

		if (!tf)
		{
			strcpy (fn, dir);
			strcat (fn, "PARTS\\");
			strcat (fn, filename);
			tf = fopen (fn, "rt");
		}

		if (!tf)
		{
			strcpy (fn, dir);
			strcat (fn, "PARTS\\S\\");
			strcat (fn, filename);
			tf = fopen (fn, "rt");
		}

		if (tf)
		{
			matrix m1, m2;
			LoadIdentity(&m1);
			LoadIdentity(&m2);
			ConvertFromLDraw (&m1, fm);
			Multiply(&m2, mat, &m1);

			decodeconnections (tf, &m2, (unsigned char)color, dir);
//			while (info->next)
//				info = info->next;
			fclose(tf);
		}

		memset (buf, 0, sizeof(buf));
	}
}

void ReadFile(char* strFile)
{
	char fn[260], tmp[260], *ptr;
	matrix mat;
	float* verts;
	unsigned long i, j, unique;
	lineinfo_t info, *a;
	FILE *f;
//	drawgroup_t* ldg;

	CleanUp();
	memset(_strName, 0, sizeof(_strName));
	memset(_strDescription, 0, sizeof(_strDescription));

	strcpy(tmp, strFile);
	if (ptr = strrchr(tmp, '.')) *ptr = 0;
	ptr = strrchr(tmp, '\\');
	if (ptr == NULL) 
		ptr = tmp;
	else
		ptr++;
	strcpy (_strName, ptr);
	strupr (_strName);

//(&_fVerts, &_nVertsCount);
//(float** ppVerts, unsigned int* nCount)
	LoadIdentity(&mat);
	info.next = 0;
	info.type = 0;

	f = fopen (strFile, "rt");
	if (fgets (tmp, 100, f))
	{
		while (tmp[strlen(tmp)-1]==10||tmp[strlen(tmp)-1]==13||tmp[strlen(tmp)-1]==32)
			tmp[strlen(tmp)-1]=0;
		tmp[66] = 0;
		strcpy (_strDescription, tmp+2);
	}

	printf("File %s : %s\n", _strName, _strDescription);

	strcpy (fn, strFile);
	if (ptr = strrchr(fn, '.')) *ptr = 0;
	memset (tmp, 0, sizeof(char[100]));
	ptr = strrchr(fn, '\\');
	if (ptr == 0) 
		ptr = fn;
	else
		ptr++;
	strcpy (tmp, ptr);
	strupr (tmp);

	strcpy (fn, strFile);
	ptr = strrchr(fn, '\\');
	*ptr = 0;
	ptr = strrchr(fn, '\\');
	*(ptr+1) = 0;

	decodefile (f, &mat, 16, &info, fn);
	fclose (f);

	unique = 0;
	verts = (float*)malloc(sizeof(float)*1500);

	// Create array of unique vertices
	for (a = info.next; a; a = a->next)
	{
		for (j = 0; j < a->type; j++)
		{
			for (i = unique-1; i != -1; i--)
				if (FloatPointsClose(&verts[i*3], &a->points[j*3]))
					break;

			if (i == -1)
			{
				if ((unique % 500) == 0)
					verts = (float*)realloc(verts, sizeof(float)*3*(unique+500));
				memcpy(&verts[unique*3], &a->points[j*3], sizeof(float)*3);
				unique++;
			}
		}
	}

	_fVerts = verts;
	_nVertsCount = unique;
	printf ("%d vertexes\n", unique);

	// Main group
	_pGroups = (GROUP*)malloc(sizeof(GROUP));
	memset(_pGroups, 0, sizeof(GROUP));
	CreateMesh(_pGroups, &info);

	a = info.next;
	while (a)
	{
		lineinfo_t* b = a->next;
		free(a);
		a = b;
	}
	info.next = 0;

/*
	char filename[32], name[260];
	FILE *tf;
	matrix m1,m2;
	int type, color;
	char buf[1024];
	float fm[12];
*/
	// Included files
	f = fopen (strFile, "rt");
	LoadIdentity(&mat);
	decodeconnections(f, &mat, 16, fn);
	fclose(f);
/*
	while (fgets(buf, 1024, f))
	{
		while (buf[strlen(buf)-1] == 10 || buf[strlen(buf)-1] == 13 || buf[strlen(buf)-1] == 32)
			buf[strlen(buf)-1] = 0;

		type = -1;
		sscanf(buf, "%d", &type);

		if (type == 1)
		{
			sscanf (buf, "%d %d %f %f %f %f %f %f %f %f %f %f %f %f %s",
				&type, &color, &fm[0], &fm[1], &fm[2], &fm[3], &fm[4], &fm[5], &fm[6], &fm[7], &fm[8], &fm[9], &fm[10], &fm[11], filename);

			strupr(filename);
			LoadIdentity(&mat);
			info.next = 0;

			ldg->next = (drawgroup_t*)malloc(sizeof(drawgroup_t));
			ldg = ldg->next;
			memset(ldg, 0, sizeof(drawgroup_t));

			strcpy (name, fn);
			strcat (name, "P\\");
			strcat (name, filename);

			tf = fopen (name, "rt");

			if (!tf)
			{
				strcpy (name, fn);
				strcat (name, "PARTS\\");
				strcat (name, filename);
				tf = fopen (name, "rt");
			}
	
			if (!tf)
			{
				strcpy (name, fn);
				strcat (name, "PARTS\\S\\");
				strcat (name, filename);
				tf = fopen (name, "rt");
			}

			if (ptr = strrchr(filename, '.')) *ptr = 0;
			strcpy(ldg->name, filename);

			LoadIdentity(&m1);
			LoadIdentity(&m2);
			ConvertFromLDraw (&m1, fm);
			Multiply (&m2, &mat, &m1);
/*
			if (strcmp(ldg->name, "STUD") == 0)
			{
				ldg->pos[0] = m2.m[12];
				ldg->pos[1] = m2.m[13];
				ldg->pos[2] = m2.m[14];
				ldg->up[2] = 1;
				m2.TransformPoints(ldg->up, 1);
				ldg->up[0] -= m2.m[12];
				ldg->up[1] -= m2.m[13];
				ldg->up[2] -= m2.m[14];
				ldg->type = 1;
			}

			if (strcmp(ldg->name, "STUD4") == 0)
			{
				ldg->pos[0] = m2.m[12];
				ldg->pos[1] = m2.m[13];
				ldg->pos[2] = m2.m[14];
				ldg->up[2] = 1;
				m2.TransformPoints(ldg->up, 1);
				ldg->up[0] -= m2.m[12];
				ldg->up[1] -= m2.m[13];
				ldg->up[2] -= m2.m[14];
				ldg->type = 0;
			}

			decodefile (tf, &m2, (unsigned char)color, &info, fn);
			AddDrawInfo (ldg, &info, verts, unique);
			fclose(tf);
		}
	}
*/
}

void SaveFile(FILE *bin, FILE *idx, unsigned long *binoff)
{
	unsigned long i;
	float box[6] = { -100, -100, -100, 100, 100, 100 };
	float maxdim;
	GROUP* pGroup;
	CONNECTION* pCon;
	matrix mat;
	TEXTURE* tex;
	short scale;
	unsigned short s;
	unsigned char bt;
	short sb[6];

 	pGroup = _pGroups;
	while (pGroup)
	{
		unsigned char* bytes = (unsigned char*)pGroup->drawinfo;
		float* floats;

		while (*bytes)
		{
			if (*bytes == LC_MESH)
			{
				if (_nVertsCount > 65535)
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

				LoadIdentity(&mat);
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
				TransformPoints (&mat, stud, 2);

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

		pGroup = pGroup->next;
	}

	for (i = 0; i < _nVertsCount; i++)
	{
		if (_fVerts[(3*i)]   > box[0]) box[0] = _fVerts[(3*i)];
		if (_fVerts[(3*i)+1] > box[1]) box[1] = _fVerts[(3*i)+1];
		if (_fVerts[(3*i)+2] > box[2]) box[2] = _fVerts[(3*i)+2];
		if (_fVerts[(3*i)]   < box[3]) box[3] = _fVerts[(3*i)];
		if (_fVerts[(3*i)+1] < box[4]) box[4] = _fVerts[(3*i)+1];
		if (_fVerts[(3*i)+2] < box[5]) box[5] = _fVerts[(3*i)+2];
	}

	maxdim = 0;
	for (i = 0; i < 6; i++)
		if (fabs(box[i]) > maxdim) maxdim = (float)fabs(box[i]);
	scale = 10000;
	if (maxdim > 3.2f)
		scale = 1000;
	if (maxdim > 32.0f)
		scale = 100;

	// write the vertex data
	fwrite(&_nVertsCount, sizeof(_nVertsCount), 1, bin);
	for (i = 0; i < _nVertsCount; i++)
	{
		float tmp[3] = { scale*_fVerts[(i*3)], scale*_fVerts[(i*3)+1], scale*_fVerts[(i*3)+2] };
		short sh[3] = { (short)tmp[0], (short)tmp[1], (short)tmp[2] };
		fwrite(&sh, sizeof(sh), 1, bin);
	}

	s = 0;
 	pCon = _pConnections;
	while (pCon)
	{
		s++;
		pCon = pCon->next;
	}
	fwrite(&s, sizeof(s), 1, bin);

 	pCon = _pConnections;
	while (pCon)
	{
		float tmp[3] = { scale*pCon->pos[0], scale*pCon->pos[1], scale*pCon->pos[2] };
		short sh[3] = { (short)tmp[0], (short)tmp[1], (short)tmp[2] };

		fwrite(&pCon->type, sizeof(pCon->type), 1, bin);
		fwrite(&sh, sizeof(sh), 1, bin);

		sh[0] = (short)(pCon->up[0]*(1<<14));
		sh[1] = (short)(pCon->up[1]*(1<<14));
		sh[2] = (short)(pCon->up[2]*(1<<14));
		fwrite(&sh, sizeof(sh), 1, bin);

		pCon = pCon->next;
	}

	// Textures
	bt = 0;
	for (tex = _pTextures; tex; tex = tex->next)
		bt++;
	fwrite(&bt, 1, 1, bin);

	for (tex = _pTextures; tex; tex = tex->next)
	{
		fwrite(&tex->color, 1, 1, bin);
		fwrite(tex->name, 8, 1, bin);

		for (i = 0; i < 12; i++)
		{
			float tmp[1] = { tex->points[i]*scale };
			short sh[1] = { (short)tmp[0] };
			fwrite(&sh, sizeof(sh), 1, bin);
		}
		for (i = 12; i < 20; i++)
		{
			float tmp[1] = { tex->points[i] };
			short sh[1] = { (short)tmp[0] };
			fwrite(&sh, sizeof(sh), 1, bin);
		}
	}

	s = 0;
 	pGroup = _pGroups;
	while (pGroup)
	{
		s++;
		pGroup = pGroup->next;
	}
	fwrite(&s, sizeof(s), 1, bin);

 	pGroup = _pGroups;
	while (pGroup)
	{
		for (bt = 0; bt < 5; bt++)
			if (!pGroup->connections[bt])
				break;
		fwrite(&bt, sizeof(bt), 1, bin);

		for (bt = 0; bt < 5; bt++)
		{
			if (!pGroup->connections[bt])
				break;

			s = 0;
			pCon = _pConnections;
			while (pCon)
			{
				if (pCon == pGroup->connections[bt])
					break;
				pCon = pCon->next;
				s++;
			}
			fwrite(&s, sizeof(s), 1, bin);
		}

		fwrite(pGroup->drawinfo, pGroup->infosize, 1, bin);

		pGroup = pGroup->next;
	}

	fwrite(_strName, 8, 1, idx);
	fwrite(_strDescription, 64, 1, idx);
	for (i = 0; i < 6; i++)
	{
		float ff, f = 1.0f * box[i];
		f *= scale;
		ff = f;
//		sb[i] = scale*box[i];
		sb[i] = (short)ff;
	}
	fwrite(&sb, sizeof(sb), 1, idx);
	bt = 0x01; // LC_PIECE_COUNT
	if (scale == 10000) bt |= 0x10; // LC_PIECE_SMALL
	if (scale == 1000) bt |= 0x20; // LC_PIECE_MEDIUM
	fwrite(&bt, sizeof(bt), 1, idx);
	i = GetDefaultPieceGroup(_strDescription);
	fwrite(&i, sizeof(i), 1, idx);
	fwrite(binoff, sizeof(*binoff), 1, idx);
	i = ftell(bin) - *binoff;
	fwrite(&i, sizeof(i), 1, idx);
	*binoff = ftell(bin);
}

int main(int argc, char *argv[])
{
	struct _finddata_t c_file;
    long hFile;
	char tmp[260];
	FILE *bin, *idx;
	unsigned long binoff;
	unsigned short count, moved;

	if (argc != 2)
	{
		argv[1] = "f:\\ldraw\\parts\\";

//		printf("Usage: convert <ldraw/parts/>\n");
//		return 1;
	}

	_fVerts = 0;
	_nVertsCount = 0;
	_pGroups = 0;
	_pConnections = 0;
	_pTextures = 0;

	strcpy(tmp, argv[1]);
	strcat(tmp, "*.dat");
	
    if((hFile = _findfirst(tmp, &c_file )) == -1L)
		printf( "No *.dat files in current directory!\n");
	else
	{
		char strbin[32] = "LeoCAD piece library data file\0\0";
		char stridx[32] = "LeoCAD piece library index file\0";
		unsigned char bt;

		bin = fopen("PIECES.BIN", "wb");
		idx = fopen("PIECES.IDX", "wb");
		fwrite(strbin, 32, 1, bin);
		binoff = 32;
		fwrite(stridx, 32, 1, idx);
		bt = 3; // version
		fwrite(&bt, 1, 1, idx);
		bt = 3; // last update
		fwrite(&bt, 1, 1, idx);

		strcpy(tmp, argv[1]);
		strcat(tmp, c_file.name);
		count = 1;

		ReadFile(tmp);
		printf("Saving...\n");
		SaveFile(bin, idx, &binoff);

		while(_findnext(hFile, &c_file) == 0)
		{
			strcpy(tmp, argv[1]);
			strcat(tmp, c_file.name);
			count++;

			ReadFile(tmp);
			printf("Saving...\n");
			SaveFile(bin, idx, &binoff);
		}
		_findclose(hFile);

		fclose(bin);

		{
			char buf[100], f1[9], f2[9], *ptr, *ptr2;
			FILE* f;

			moved = 0;
			f = fopen("MOVED.TXT", "rt");

			while (fgets(buf, 100, f) != NULL)
			{
				if (ptr = strchr(buf, ' '))
				{
					*ptr = 0;
					memset(f1, 0, 9);
					strcpy(f1, buf);
					ptr++;
					if (ptr2 = strchr(ptr, ' '))
					{
						*ptr2 = 0;
						memset(f2, 0, 9);
						strcpy(f2, ptr);
						moved++;
						strupr(f1);
						strupr(f2);
						fwrite(f1, 8, 1, idx);
						fwrite(f2, 8, 1, idx);
                    }
				}
			}
			fclose(f);
		}

		fwrite(&moved, sizeof(moved), 1, idx);
		fwrite(&binoff, sizeof(binoff), 1, idx);
		fwrite(&count, sizeof(count), 1, idx);
		fclose(idx);
	}

	CleanUp();
	return 0;
}

/*
void Inline(drawgroup_t* pdg)
{
	unsigned int count[256][3], vert, j, k, v, loc;
	int i;
	unsigned int colcount;
	int curcol;
	unsigned int* drawinfo;
	drawgroup_t *ldg, *dg;

	if (pdg->info == NULL)
	{
		ldg = &_DrawGroup;
		dg = _DrawGroup.next;

		while (dg)
		{
			if (dg == pdg)
			{
				ldg->next = dg->next;
				free(pdg);
				return;
			}
			ldg = dg;
			dg = dg->next;
		}
	}


	vert = 0;
	memset (count, 0, sizeof(count));

	loc = 1;
	colcount = _DrawGroup.info[0];
	while (colcount)
	{
		curcol = _DrawGroup.info[loc];
		loc++;
		for (j = 0; j < 3; j++)
		{
			count[curcol][j] += _DrawGroup.info[loc];
			loc += _DrawGroup.info[loc]+1;
		}
		colcount--;
	}

	loc = 1;
	colcount = pdg->info[0];
	while (colcount)
	{
		int curcol = pdg->info[loc];
		loc++;
		for (j = 0; j < 3; j++)
		{
			count[curcol][j] += pdg->info[loc];
			loc += pdg->info[loc]+1;
		}
		colcount--;
	}

	k = 0;
	for (i = 0; i < 256; i++)
	{
		if (count[i][0] || count[i][1] || count[i][2])
		{
			k++;
			vert += count[i][0] + count[i][1] + count[i][2];
		}
	}
	vert += (k*4)+1;

	drawinfo = (unsigned int*)malloc(vert*sizeof(unsigned int));
	drawinfo[0] = k;
	vert = 1;

	for (i = 16; i < 256; i++)
	{
		if (count[i][0] || count[i][1] || count[i][2])
		{
			drawinfo[vert] = i;
			vert++;
		}
		
		for (j = 0; j < 3; j++)
		{
			if (count[i][0] || count[i][1] || count[i][2])
			{
				drawinfo[vert] = count[i][j];
				vert++;
			}
			
			if (count[i][j] != 0)
			{
				{
					unsigned int* info = _DrawGroup.info;
					loc = 1;
					colcount = info[0];
					while (colcount)
					{
						int curcol = info[loc];
						loc++;
						
						for (v = 0; v < 3; v++)
						{
							if ((v == j) && (curcol == i))
							{
								memcpy(&drawinfo[vert], &info[loc+1], info[loc]*sizeof(unsigned int));
								vert += info[loc];
							}
							loc += info[loc]+1;
						}
						colcount--;
					}
				}
				
				{
					unsigned int* info = pdg->info;
					loc = 1;
					colcount = info[0];
					while (colcount)
					{
						int curcol = info[loc];
						loc++;
						
						for (v = 0; v < 3; v++)
						{
							if ((v == j) && (curcol == i))
							{
								memcpy(&drawinfo[vert], &info[loc+1], info[loc]*sizeof(unsigned int));
								vert += info[loc];
							}
							loc += info[loc]+1;
						}
						colcount--;
					}
				}
			}
		}
		if (i == 16) i = -1;
		if (i == 15) i = 23;
	}
	free(_DrawGroup.info);
	_DrawGroup.info = drawinfo;
	_DrawGroup.infosize = vert;

	ldg = &_DrawGroup;
	dg = _DrawGroup.next;

	while (dg)
	{
		if (dg == pdg)
		{
			ldg->next = dg->next;
			free(pdg->info);
			free(pdg);
			break;
		}
		ldg = dg;
		dg = dg->next;
	}
}

void decode (char* strFile, float** ppVerts, unsigned int* nCount, char* strDescription)
{
	char tmp[100];
	matrix mat;
	lineinfo_t info;
	FILE *f;
	char *ptr;
	char fn[260];
	lineinfo_t *a;
	unsigned int i, j, unique;
	float* verts;
	drawgroup_t* ldg;
	char buf[1024];
	int type, color;
	char filename[32], name[260];
	float fm[12];
	FILE *tf;
	matrix m1,m2;

	LoadIdentity(&mat);
	info.next = 0;
	info.type = 0;

	f = fopen (strFile, "rt");
	if (fgets (tmp, 100, f))
	{
		while (tmp[strlen(tmp)-1]==10||tmp[strlen(tmp)-1]==13||tmp[strlen(tmp)-1]==32)
			tmp[strlen(tmp)-1]=0;
		tmp[66] = 0;
		strcpy (strDescription, tmp+2);
	}

	printf("Decoding %s : %s\n", _strName, _strDescription);

	strcpy (fn, strFile);
	if (ptr = strrchr(fn, '.')) *ptr = 0;
	memset (tmp, 0, sizeof(char[100]));
	ptr = strrchr(fn, '\\');
	if (ptr == 0) 
		ptr = fn;
	else
		ptr++;
	strcpy (tmp, ptr);
	strupr (tmp);

	strcpy (fn, strFile);
	ptr = strrchr(fn, '\\');
	*ptr = 0;
	ptr = strrchr(fn, '\\');
	*(ptr+1) = 0;

	decodefile (f, &mat, 16, &info, fn);
	fclose (f);

	unique = 0;
	verts = (float*)malloc(sizeof(float)*1500);

	// Create array of unique vertices
	for (a = info.next; a; a = a->next)
	{
		for (j = 0; j < a->type; j++)
		{
			for (i = unique-1; i != -1; i--)
				if (FloatPointsClose(&verts[i*3], &a->points[j*3]))
					break;

			if (i == -1)
			{
				if ((unique % 500) == 0)
					verts = (float*)realloc(verts, sizeof(float)*3*(unique+500));
				memcpy(&verts[unique*3], &a->points[j*3], sizeof(float)*3);
				unique++;
			}
		}
	}

	a = info.next;
	while (a)
	{
		lineinfo_t* b = a->next;
		free(a);
		a = b;
	}
	info.next = 0;

	*ppVerts = verts;
	*nCount = unique;

	printf ("%d vertexes\n", unique);

	// Main group
	f = fopen (strFile, "rt");
	LoadIdentity(&mat);
	decodefile (f, &mat, 16, &info, fn);
	fclose(f);
	AddDrawInfo (&_DrawGroup, &info, verts, unique);
	Inline(&_DrawGroup);
	ldg = &_DrawGroup;

	// Included files
	f = fopen (strFile, "rt");
	while (fgets(buf, 1024, f))
	{
		while (buf[strlen(buf)-1] == 10 || buf[strlen(buf)-1] == 13 || buf[strlen(buf)-1] == 32)
			buf[strlen(buf)-1] = 0;

		type = -1;
		sscanf(buf, "%d", &type);

		if (type == 1)
		{
			sscanf (buf, "%d %d %f %f %f %f %f %f %f %f %f %f %f %f %s",
				&type, &color, &fm[0], &fm[1], &fm[2], &fm[3], &fm[4], &fm[5], &fm[6], &fm[7], &fm[8], &fm[9], &fm[10], &fm[11], filename);

			strupr(filename);
			LoadIdentity(&mat);
			info.next = 0;

			ldg->next = (drawgroup_t*)malloc(sizeof(drawgroup_t));
			ldg = ldg->next;
			memset(ldg, 0, sizeof(drawgroup_t));

			strcpy (name, fn);
			strcat (name, "P\\");
			strcat (name, filename);

			tf = fopen (name, "rt");

			if (!tf)
			{
				strcpy (name, fn);
				strcat (name, "PARTS\\");
				strcat (name, filename);
				tf = fopen (name, "rt");
			}
	
			if (!tf)
			{
				strcpy (name, fn);
				strcat (name, "PARTS\\S\\");
				strcat (name, filename);
				tf = fopen (name, "rt");
			}

			if (ptr = strrchr(filename, '.')) *ptr = 0;
			strcpy(ldg->name, filename);

			LoadIdentity(&m1);
			LoadIdentity(&m2);
			ConvertFromLDraw (&m1, fm);
			Multiply (&m2, &mat, &m1);
#if 0
			if (strcmp(ldg->name, "STUD") == 0)
			{
				ldg->pos[0] = m2.m[12];
				ldg->pos[1] = m2.m[13];
				ldg->pos[2] = m2.m[14];
				ldg->up[2] = 1;
				m2.TransformPoints(ldg->up, 1);
				ldg->up[0] -= m2.m[12];
				ldg->up[1] -= m2.m[13];
				ldg->up[2] -= m2.m[14];
				ldg->type = 1;
			}

			if (strcmp(ldg->name, "STUD4") == 0)
			{
				ldg->pos[0] = m2.m[12];
				ldg->pos[1] = m2.m[13];
				ldg->pos[2] = m2.m[14];
				ldg->up[2] = 1;
				m2.TransformPoints(ldg->up, 1);
				ldg->up[0] -= m2.m[12];
				ldg->up[1] -= m2.m[13];
				ldg->up[2] -= m2.m[14];
				ldg->type = 0;
			}
#endif
			decodefile (tf, &m2, (unsigned char)color, &info, fn);
			AddDrawInfo (ldg, &info, verts, unique);
			fclose(tf);
		}
	}
	fclose(f);
}

void OpenFile(char* strFile)
{
	char tmp[260], *ptr;
	CleanUp();

//	if (stricmp() strFile.Right(4).CompareNoCase(_T(".DAT")) == 0)
	{
		memset(_strName, 0, sizeof(_strName));
		memset(_strDescription, 0, sizeof(_strDescription));

		strcpy(tmp, strFile);
		if (ptr = strrchr(tmp, '.')) *ptr = 0;
		ptr = strrchr(tmp, '\\');
		if (ptr == NULL) 
			ptr = tmp;
		else
			ptr++;
		strcpy (_strName, ptr);
		strupr (_strName);

		decode (strFile, &_fVerts, &_nVertsCount, _strDescription);
	}
}
*/

