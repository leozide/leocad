//
// Piece library management
//

#ifdef LC_WINDOWS
#include "stdafx.h"
#endif
#include <malloc.h>
#include <string.h>
#include <math.h>
#include "defines.h"
#include "globals.h"
#include "project.h"
#include "matrix.h"
#include "system.h"
#include "file.h"
#include "library.h"

// ========================================================

static unsigned long GetDefaultPieceGroup(char* name)
{
	char tmp[9];
	strncpy(tmp, name, 9);
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
		strstr(tmp,"Plant")		|| strstr(tmp,"Pannier")||
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

	if (piece->verts_count > 65535)
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
				m1.ConvertFromLDraw(fm);
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
			m1.ConvertFromLDraw(fm);
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
			m1.ConvertFromLDraw(fm);
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

	strcpy(file1, project->GetLibraryPath());
	strcat(file1, "pieces-b.old");
	remove(file1);
	strcpy(file2, project->GetLibraryPath());
	strcat(file2, "pieces.bin");
	rename(file2, file1);

	if ((!oldbin.Open(file1, "rb")) ||
		(!newbin.Open(file2, "wb")))
		return false;

	strcpy(file1, project->GetLibraryPath());
	strcat(file1, "pieces-i.old");
	remove(file1);
	strcpy(file2, project->GetLibraryPath());
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
				if (piece->verts_count > 65535)
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
			float tmp[1] = { tex->points[i] };
			short sh[1] = { (short)tmp[0] };
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

	bt = 0x01; // LC_PIECE_COUNT
	if (scale == 10000) bt |= 0x10; // LC_PIECE_SMALL
	if (scale == 1000) bt |= 0x20; // LC_PIECE_MEDIUM
	newidx.WriteByte(&bt, 1);

	i = GetDefaultPieceGroup(piece->description);
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

// Remove pieces from the library
bool DeletePiece(char** names, int numpieces)
{
	FileDisk newbin, newidx, oldbin, oldidx;
	char file1[LC_MAXPATH], file2[LC_MAXPATH], tmp[200];
	unsigned short count, deleted = 0, j;
	void* membuf;

	strcpy(file1, project->GetLibraryPath());
	strcat(file1, "pieces-b.old");
	remove(file1);
	strcpy(file2, project->GetLibraryPath());
	strcat(file2, "pieces.bin");
	rename(file2, file1);

	if ((!oldbin.Open(file1, "rb")) ||
		(!newbin.Open(file2, "wb")))
		return false;

	strcpy(file1, project->GetLibraryPath());
	strcat(file1, "pieces-i.old");
	remove(file1);
	strcpy(file2, project->GetLibraryPath());
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

		for (i = 0; i < numpieces; i++)
			if (strcmp(name, names[i]) == 0)
				break;

		if (i != numpieces)
		{
			oldidx.Seek(64+12+1+4+4+4, SEEK_CUR);
			deleted++;
			continue;
		}

		newidx.Write(name, 8);
		oldidx.Read(tmp, 64+12+1+4);
		newidx.Write(tmp, 64+12+1+4);

		unsigned long binoff = newbin.GetLength(), size;
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
	unsigned short moved, cs;

	oldidx.Seek(-(2+4+2), SEEK_END);
	oldidx.ReadShort(&moved, 1);
	cs = 2+(moved*16);
	oldidx.Seek(-(long)cs, SEEK_CUR);
	membuf = malloc(cs);
	oldidx.Read(membuf, cs);
	newidx.Write(membuf, cs);
	free(membuf);

	// info at the end
	unsigned long binoff = newbin.GetPosition();
	newidx.WriteLong(&binoff, 1);
	count -= deleted;
	newidx.WriteShort(&count, 1);

	oldidx.Close();
	oldbin.Close();
	newidx.Close();
	newbin.Close();

	return true;
}

// ========================================================

// Load update
bool LoadUpdate(const char* update)
{
	FileDisk newbin, newidx, oldbin, oldidx, up;
	char file1[LC_MAXPATH], file2[LC_MAXPATH], tmp[200];
	unsigned short changes, moved, count, i, j, newcount = 0;
	unsigned long cs, group, binoff;
	unsigned char bt;
	void* membuf;

	typedef struct
	{
		char name[9];
		unsigned char type;
		unsigned long offset;
	} LC_UPDATE_INFO;
	LC_UPDATE_INFO* upinfo;

	strcpy(file1, project->GetLibraryPath());
	strcat(file1, "pieces-b.old");
	remove(file1);
	strcpy(file2, project->GetLibraryPath());
	strcat(file2, "pieces.bin");
	rename(file2, file1);

	if ((!oldbin.Open(file1, "rb")) ||
		(!newbin.Open(file2, "wb")))
		return false;

	strcpy(file1, project->GetLibraryPath());
	strcat(file1, "pieces-i.old");
	remove(file1);
	strcpy(file2, project->GetLibraryPath());
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

	return true;
}
