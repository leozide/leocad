// Information about how to draw a piece and some more stuff.
//

#ifdef LC_WINDOWS
#include "stdafx.h"
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h> // TODO: remove, only needed by ZoomExtents()
#include "texture.h"
#include "pieceinf.h"
#include "project.h"
#include "globals.h"
#include "matrix.h"
#include "defines.h"

#define SIDES 16
static float sintbl[SIDES];
static float costbl[SIDES];

#define LC_MESH		1
#define LC_STUD		2
#define LC_STUD2	3
#define LC_STUD3	4
#define LC_STUD4	5

// measurements (in centimeters)
//#define LC_FLAT_HEIGHT  0.32f
//#define LC_BRICK_HEIGHT (3*LC_FLAT_HEIGHT)
//#define LC_BASEPLATE_HEIGHT (LC_FLAT_HEIGHT/2)
//#define LC_HALF_WIDE  0.4f
//#define LC_ONE_WIDE   0.8f
//#define LC_BRICK_WALL 0.125f
#define LC_STUD_HEIGHT 0.16f
#define LC_STUD_RADIUS 0.24f
#define LC_KNOB_RADIUS 0.32f
//#define LC_STUD_TECH_RADIUS (LC_FLAT_HEIGHT/2)


// Convert a color from LDraw to LeoCAD
unsigned char ConvertColor(int c)
{
	if (c > 255) c -= 256;
	switch (c)
	{
	case 0: return 9;	// black		(black)
	case 1: return 4;	// blue			(blue)
	case 2: return 2;	// green		(green)
	case 3: return 5;	// dark cyan
	case 4: return 0;	// red			(red)
	case 5: return 11;	// magenta
	case 6: return 10;	// brown		(brown)
	case 7: return 22;	// gray			(gray)
	case 8: return 8;	// dark gray	(dark gray)
	case 9: return 5;	// light blue	()
	case 10: return 3;	// light green	(light green)
	case 11: return 5;	// cyan			(light blue)
	case 12: return 1;	// light red
	case 13: return 11;	// pink			(pink)
	case 14: return 6;	// yellow		(yellow)
	case 15: return 7;	// white		(white)
	case 16: return LC_COL_DEFAULT; // special case
	case 24: return LC_COL_EDGES; // edge
	case 32: return 9;	// black
	case 33: return 18;	// clear blue
	case 34: return 16;	// clear green
	case 35: return 5;	// dark cyan
	case 36: return 14;	// clear red
	case 37: return 11;	// magenta
	case 38: return 10;	// brown
	case 39: return 21;	// clear white (clear gray)
	case 40: return 8;	// dark gray
	case 41: return 19;	// clear light blue
	case 42: return 17;	// clear light green
	case 43: return 19;	// clear cyan			(clear light blue)
	case 44: return 15;	// clear light red ??
	case 45: return 11;	// pink
	case 46: return 20;	// clear yellow
	case 47: return 21;	// clear white
	case 70: return 10; // maroon (326)
	case 78: return 13;	// gold (334)
	case 110: return 1; // orange (366 from fire logo pattern)
	case 126: return 23;// tan (382)
	case 127: return 27;// silver/chrome (383)
	case 175: return 3;	// mint green (431)
	case 206: return 1;	// orange (462)
	case 238: return 6;	// light yellow (494 eletric contacts)
	case 239: return 6;	// light yellow (495)
	case 247: return 27;// 503 chrome
	case 250: return 3; // 506 mint (Belville)
	case 253: return 11;// 509 rose (e.g. in Paradisa)

	// taken from l2p.doc but not verified
	case 178: return 11;// 434 dark cyan (e.g. in New Technic Models)
	case 254: return 6; // 510 light yellow (e.g. in Belville)
	}
	return 9; // black
}

/////////////////////////////////////////////////////////////////////////////
// PieceInfo construction/destruction

PieceInfo::PieceInfo()
{
	// Not called, initialize in LoadIndex().
}

PieceInfo::~PieceInfo()
{
	FreeInformation();
}

/////////////////////////////////////////////////////////////////////////////
// File I/O

void PieceInfo::LoadIndex(File* file)
{
	short sh[6];
	short scale;

	static bool init = false;
	if (!init)
	{
		for (int i = 0; i < SIDES; i++)
		{
			sintbl[i] = (float)sin((PI2*i)/(SIDES));
			costbl[i] = (float)cos((PI2*i)/(SIDES));
		}
		init = true;
	}

	// TODO: don't change ref. if we're reloading ?
	m_nRef = 0;
	m_nVertexCount = 0;
	m_fVertexArray = NULL;
	m_nConnectionCount = 0;
	m_pConnections = NULL;
	m_nGroupCount = 0;
	m_pGroups = NULL;
	m_nTextureCount = 0;
	m_pTextures = NULL;

	file->Read(m_strName, 8);
	file->Read(m_strDescription, 64);
	file->ReadShort(sh, 6);
	file->ReadByte(&m_nFlags, 1);
	file->ReadLong(&m_nGroups, 1);
	file->ReadLong(&m_nOffset, 1);
	file->ReadLong(&m_nSize, 1);

	scale = 100;
	if (m_nFlags & LC_PIECE_MEDIUM) scale = 1000;
	if (m_nFlags & LC_PIECE_SMALL)  scale = 10000;
	m_fDimensions[0] = (float)sh[0]/scale;
	m_fDimensions[1] = (float)sh[1]/scale;
	m_fDimensions[2] = (float)sh[2]/scale;
	m_fDimensions[3] = (float)sh[3]/scale;
	m_fDimensions[4] = (float)sh[4]/scale;
	m_fDimensions[5] = (float)sh[5]/scale;
}

GLuint PieceInfo::AddRef()
{
	if (m_nRef == 0)
		LoadInformation();
	m_nRef++;

	for (int i = 0; i < m_nTextureCount; i++)
		if (m_pTextures[i].texture != NULL)
			m_pTextures[i].texture->AddRef(false);
// TODO: get correct filter paramenter

	return m_nBoxList;
}

void PieceInfo::DeRef()
{
	m_nRef--;
	for (int i = 0; i < m_nTextureCount; i++)
		if (m_pTextures[i].texture != NULL)
			m_pTextures[i].texture->DeRef();

	if (m_nRef == 0)
		FreeInformation();
}

void PieceInfo::LoadInformation()
{
	FileDisk bin;
	char filename[LC_MAXPATH];
	void* buf;
	unsigned long verts, *longs, fixverts;
	unsigned char *bytes, *tmp, bt;
	unsigned short *ushorts, sh;
	float scale, shift;
	short* shorts;
	CONNECTIONINFO* pConnection;
	DRAWGROUP* pGroup;
	int i, j;

	// We don't want memory leaks.
	FreeInformation();

	// Create a display for the bounding box.
	m_nBoxList = glGenLists(1);
	glNewList(m_nBoxList, GL_COMPILE);
	glEnableClientState(GL_VERTEX_ARRAY);

	float box[24][3] = {
		{ m_fDimensions[0], m_fDimensions[1], m_fDimensions[2] }, 
		{ m_fDimensions[3], m_fDimensions[1], m_fDimensions[2] },
		{ m_fDimensions[3], m_fDimensions[4], m_fDimensions[2] },
		{ m_fDimensions[0], m_fDimensions[4], m_fDimensions[2] },
		{ m_fDimensions[0], m_fDimensions[1], m_fDimensions[5] },
		{ m_fDimensions[0], m_fDimensions[4], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[4], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[1], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[4], m_fDimensions[2] }, 
		{ m_fDimensions[3], m_fDimensions[1], m_fDimensions[2] },
		{ m_fDimensions[3], m_fDimensions[1], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[4], m_fDimensions[5] },
		{ m_fDimensions[0], m_fDimensions[4], m_fDimensions[5] },
		{ m_fDimensions[0], m_fDimensions[1], m_fDimensions[5] },
		{ m_fDimensions[0], m_fDimensions[1], m_fDimensions[2] },
		{ m_fDimensions[0], m_fDimensions[4], m_fDimensions[2] }, 
		{ m_fDimensions[0], m_fDimensions[1], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[1], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[1], m_fDimensions[2] },
		{ m_fDimensions[0], m_fDimensions[1], m_fDimensions[2] },
		{ m_fDimensions[0], m_fDimensions[4], m_fDimensions[2] },
		{ m_fDimensions[3], m_fDimensions[4], m_fDimensions[2] },
		{ m_fDimensions[3], m_fDimensions[4], m_fDimensions[5] },
		{ m_fDimensions[0], m_fDimensions[4], m_fDimensions[5] }
	};
	glVertexPointer (3, GL_FLOAT, 0, box);
	glDrawArrays(GL_QUADS, 0, 24);
	glEndList();

	// Open pieces.bin and buffer the information we need.
	strcpy(filename, project->GetLibraryPath());
	strcat(filename, "pieces.bin");
	if (!bin.Open(filename, "rb"))
		return;

	buf = malloc(m_nSize);
	bin.Seek(m_nOffset, SEEK_SET);
	bin.Read(buf, m_nSize);
	bin.Close();

	shift  = 1.0f/(1<<14);
	scale = 0.01f;
	if (m_nFlags & LC_PIECE_MEDIUM) scale = 0.001f;
	if (m_nFlags & LC_PIECE_SMALL)  scale = 0.0001f;
	longs = (unsigned long*)buf;
	fixverts = verts = *longs;
	bytes = (unsigned char*)(longs + 1);
	bytes += verts * sizeof(short) * 3;

	// Read connections
	m_nConnectionCount = *((unsigned short*)bytes);
	bytes += sizeof(unsigned short);
	m_pConnections = (CONNECTIONINFO*)malloc(m_nConnectionCount * sizeof(CONNECTIONINFO));

	sh = m_nConnectionCount;
	for (pConnection = m_pConnections; sh--; pConnection++)
	{
		pConnection->type = *bytes;
		bytes++;

		shorts = (short*)bytes;
		pConnection->center[0] = (float)(*shorts)*scale;
		shorts++;
		pConnection->center[1] = (float)(*shorts)*scale;
		shorts++;
		pConnection->center[2] = (float)(*shorts)*scale;
		shorts++;
		pConnection->normal[0] = (float)(*shorts)*shift;
		shorts++;
		pConnection->normal[1] = (float)(*shorts)*shift;
		shorts++;
		pConnection->normal[2] = (float)(*shorts)*shift;
		shorts++;

		bytes = (unsigned char*)shorts;
	}

	// Load textures
	m_nTextureCount = *bytes;
	if (m_nTextureCount > 0)
		m_pTextures = (TEXTURE*)malloc(m_nTextureCount*sizeof(TEXTURE));
	bytes++;

	for (sh = 0; sh < m_nTextureCount; sh++)
	{
		char name[9];
		TEXTURE* tex = &m_pTextures[sh];
		tex->color = ConvertColor(*bytes);
		bytes++;

		strcpy(name, (char*)bytes);
		tex->texture = project->FindTexture(name);

		shorts = (short*)(bytes + 8);
		for (i = 0; i < 4; i++)
		{
			tex->vertex[i][0] = (float)shorts[0]*scale;
			tex->vertex[i][1] = (float)shorts[1]*scale;
			tex->vertex[i][2] = (float)shorts[2]*scale;
			shorts += 3;
		}

		for (i = 0; i < 4; i++)
		{
			tex->coords[i][0] = (float)shorts[0];
			tex->coords[i][1] = (float)shorts[1];
			shorts += 2;
		}

		bytes += 8 + 20*sizeof(unsigned short);
	}

	// Read groups
	m_nGroupCount = *((unsigned short*)bytes);
	bytes += sizeof(unsigned short);
	m_pGroups = (DRAWGROUP*)malloc(sizeof(DRAWGROUP)*m_nGroupCount);
	memset(m_pGroups, 0, sizeof(DRAWGROUP)*m_nGroupCount);

	// First we need to know the number of vertexes
	tmp = bytes;
	sh = m_nGroupCount;
	unsigned long quads = 0;
	while (sh--)
	{
		bt = *bytes;
		bytes++;
		bytes += bt*sizeof(unsigned short);

		while (*bytes)
		{
			if (*bytes == LC_MESH)
			{
				if (fixverts > 65535)
				{
					unsigned long colors, *p;
					p = (unsigned long*)(bytes + 1);
					colors = *p;
					p++;

					while (colors--)
					{
						p++; // color code
						quads += *p;
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
						quads += *p;
						p += *p + 1;
						p += *p + 1;
						p += *p + 1;
					}

					bytes = (unsigned char*)p;
				}
			}

			if (*bytes == LC_STUD)
			{
				verts += (2*SIDES)+1;
				quads += 4*SIDES;
				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
			}

			if (*bytes == LC_STUD2)
			{
				verts += 4*SIDES;
				quads += 12*SIDES;
				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
			}
			
			if (*bytes == LC_STUD3)
			{
				verts += (2*SIDES)+1;
				quads += 4*SIDES;
				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
			}

			if (*bytes == LC_STUD4)
			{
				verts += 4*SIDES;
				quads += 12*SIDES;
				bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
			}
		}
		bytes++; // should be 0
	}

	m_fVertexArray = (float*)malloc(3*sizeof(float)*verts);
	m_nVertexCount = verts;
	if ((verts > 65535) || (quads > 65535))
		m_nFlags |= LC_PIECE_LONGDATA;
	else
		m_nFlags &= ~LC_PIECE_LONGDATA;

	// Copy the 'fixed' vertexes
	shorts = (short*)(longs + 1);
	for (verts = 0; verts < *longs; verts++)
	{
		m_fVertexArray[verts*3] = (float)(*shorts)*scale;
		shorts++;
		m_fVertexArray[verts*3+1] = (float)(*shorts)*scale;
		shorts++;
		m_fVertexArray[verts*3+2] = (float)(*shorts)*scale;
		shorts++;
	}

	// Read groups
	bytes = tmp;
	sh = m_nGroupCount;
	for (pGroup = m_pGroups; sh--; pGroup++)
	{
		bt = *bytes;
		bytes++;

		pGroup->connections[bt] = 0xFFFF;
		while(bt--)
		{
			unsigned short tmp = *bytes;
			pGroup->connections[bt] = tmp;
			bytes += sizeof(unsigned short);
		}

		// Currently there's only one type of drawinfo (mesh or stud)
		// per group but this will change in the future.
		switch (*bytes)
		{
		case LC_MESH:
			if (fixverts > 65535)
			{
				unsigned long colors, *p;
				bytes++;
				p = (unsigned long*)bytes;
				colors = *p;
				p++;

				while (colors--)
				{
					*p = ConvertColor(*p);
					p++; // color code
					p += *p + 1;
					p += *p + 1;
					p += *p + 1;
				}

				i = (unsigned char*)p - bytes;
				pGroup->drawinfo = malloc(i);
				memcpy(pGroup->drawinfo, bytes, i);
				bytes = (unsigned char*)p;
			}
			else
			{
				unsigned short colors, *p;
				bytes++;
				p = (unsigned short*)bytes;
				colors = *p;
				p++;

				while (colors--)
				{
					*p = ConvertColor(*p);
					p++; // color code
					p += *p + 1;
					p += *p + 1;
					p += *p + 1;
				}

				i = (unsigned char*)p - bytes;

				if (m_nFlags & LC_PIECE_LONGDATA)
				{
					pGroup->drawinfo = malloc(i*sizeof(unsigned long)/sizeof(unsigned short));
					longs = (unsigned long*)pGroup->drawinfo;

					for (ushorts = (unsigned short*)bytes; ushorts != p; ushorts++, longs++)
						*longs = *ushorts;
				}
				else
				{
					pGroup->drawinfo = malloc(i);
					memcpy(pGroup->drawinfo, bytes, i);
				}

				bytes = (unsigned char*)p;
			}
		break;

		case LC_STUD:
		{
			int size;
			Matrix mat((float*)(bytes+2));
			unsigned short color = ConvertColor(*(bytes+1));

			// Create the vertexes
			for (i = 0; i < SIDES; i++)
			{
				m_fVertexArray[(verts+i+SIDES)*3] = 
				m_fVertexArray[(verts+i)*3] = 
					LC_STUD_RADIUS * costbl[i];
				m_fVertexArray[(verts+i+SIDES)*3+1] = 
				m_fVertexArray[(verts+i)*3+1] = 
					LC_STUD_RADIUS * sintbl[i];
				m_fVertexArray[(verts+i)*3+2] = 0;
				m_fVertexArray[(verts+i+SIDES)*3+2] = LC_STUD_HEIGHT;
			}
			m_fVertexArray[(verts+2*SIDES)*3] = 0;
			m_fVertexArray[(verts+2*SIDES)*3+1] = 0;
			m_fVertexArray[(verts+2*SIDES)*3+2] = LC_STUD_HEIGHT;

			mat.TransformPoints(&m_fVertexArray[verts*3], 2*SIDES+1);
			// colors + 2*num_prim + sides*prims
			size = 9+SIDES*11;

			if (m_nFlags & LC_PIECE_LONGDATA)
			{
				pGroup->drawinfo = malloc(sizeof(unsigned long)*size);
				longs = (unsigned long*)pGroup->drawinfo;

				longs[0] = 2; // colors
				longs[1] = color;
				longs[2] = SIDES*4;
				j = 3;

				for (i = 0; i < SIDES; i++)
				{
					longs[3+i*4] = (unsigned long)verts + i;
					if (i == SIDES-1)
					{
						longs[4+i*4] = (unsigned long)verts;
						longs[5+i*4] = (unsigned long)verts + SIDES;
					}
					else
					{
						longs[4+i*4] = (unsigned long)verts + i + 1;
						longs[5+i*4] = (unsigned long)verts + SIDES + i + 1;
					}
					longs[6+i*4] = (unsigned long)verts + SIDES + i;
				}
				j += 4*SIDES;
				longs[j] = SIDES*3;
				j++;

				for (i = 0; i < SIDES; i++)
				{
					longs[j+i*3] = (unsigned short)verts + 2*SIDES;
					longs[1+j+i*3] = (unsigned short)verts + SIDES + i;
					if (i == SIDES-1)
						longs[2+j+i*3] = (unsigned short)verts + SIDES;
					else
						longs[2+j+i*3] = (unsigned short)verts + SIDES + i + 1;
				}

				j += 3*SIDES;
				longs[j] =  0; j++; // lines
				longs[j] =  LC_COL_EDGES; j++; // color
				longs[j] =  0; j++; // quads
				longs[j] =  0; j++; // tris
				longs[j] = 4*SIDES; j++;

				for (i = 0; i < SIDES; i++)
				{
					longs[j+i*4] = (unsigned long)verts + i;
					if (i == SIDES-1)
						longs[1+j+i*4] = (unsigned long)verts;
					else
						longs[1+j+i*4] = (unsigned long)verts + i + 1;

					longs[2+j+i*4] = longs[j+i*4] + SIDES;
					longs[3+j+i*4] = longs[1+j+i*4] + SIDES;
				}
			}
			else
			{
				pGroup->drawinfo = malloc(sizeof(unsigned short)*size);
				ushorts = (unsigned short*)pGroup->drawinfo;

				ushorts[0] = 2; // colors
				ushorts[1] = color;
				ushorts[2] = SIDES*4;
				j = 3;

				for (i = 0; i < SIDES; i++)
				{
					ushorts[3+i*4] = (unsigned short)(verts + i);
					if (i == SIDES-1)
					{
						ushorts[4+i*4] = (unsigned short)verts;
						ushorts[5+i*4] = (unsigned short)verts + SIDES;
					}
					else
					{
						ushorts[4+i*4] = (unsigned short)verts + i + 1;
						ushorts[5+i*4] = (unsigned short)verts + SIDES + i + 1;
					}
					ushorts[6+i*4] = (unsigned short)verts + SIDES + i;
				}
				j += 4*SIDES;
				ushorts[j] = SIDES*3;
				j++;

				for (i = 0; i < SIDES; i++)
				{
					ushorts[j+i*3] = (unsigned short)verts + 2*SIDES;
					ushorts[1+j+i*3] = (unsigned short)verts + SIDES + i;
					if (i == SIDES-1)
						ushorts[2+j+i*3] = (unsigned short)verts + SIDES;
					else
						ushorts[2+j+i*3] = (unsigned short)verts + SIDES + i + 1;
				}

				j += 3*SIDES;
				ushorts[j] =  0; j++; // lines
				ushorts[j] =  LC_COL_EDGES; j++; // color
				ushorts[j] =  0; j++; // quads
				ushorts[j] =  0; j++; // tris
				ushorts[j] = 4*SIDES; j++;

				for (i = 0; i < SIDES; i++)
				{
					ushorts[j+i*4] = (unsigned short)verts + i;
					if (i == SIDES-1)
						ushorts[1+j+i*4] = (unsigned short)verts;
					else
						ushorts[1+j+i*4] = (unsigned short)verts + i + 1;

					ushorts[2+j+i*4] = ushorts[j+i*4] + SIDES;
					ushorts[3+j+i*4] = ushorts[1+j+i*4] + SIDES;
				}
			}

			verts += 2*SIDES+1;
			bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
		} break;

		case LC_STUD2:
		{
			int size;
			Matrix mat((float*)(bytes+2));
			unsigned short color = ConvertColor(*(bytes+1));

			// Create the vertexes
			for (i = 0; i < SIDES; i++)
			{
				// outside
				m_fVertexArray[(verts+i+SIDES)*3] = 
				m_fVertexArray[(verts+i)*3] = 
					LC_STUD_RADIUS * costbl[i];
				m_fVertexArray[(verts+i+SIDES)*3+1] = 
				m_fVertexArray[(verts+i)*3+1] = 
					LC_STUD_RADIUS * sintbl[i];
				m_fVertexArray[(verts+i)*3+2] = LC_STUD_HEIGHT;
				m_fVertexArray[(verts+i+SIDES)*3+2] = 0;

				// inside
				m_fVertexArray[(verts+i+2*SIDES)*3] = 
				m_fVertexArray[(verts+i+3*SIDES)*3] = 
					0.16f * costbl[i];
				m_fVertexArray[(verts+i+2*SIDES)*3+1] = 
				m_fVertexArray[(verts+i+3*SIDES)*3+1] = 
					0.16f * sintbl[i];
				m_fVertexArray[(verts+i+3*SIDES)*3+2] = LC_STUD_HEIGHT;
				m_fVertexArray[(verts+i+2*SIDES)*3+2] = 0;
			}

			mat.TransformPoints(&m_fVertexArray[verts*3], 4*SIDES);
			// colors + 2*num_prim + sides*prims
			size = 9+SIDES*20;

			if (m_nFlags & LC_PIECE_LONGDATA)
			{
				pGroup->drawinfo = malloc(sizeof(unsigned long)*size);
				longs = (unsigned long*)pGroup->drawinfo;

				longs[0] = 2; // colors
				longs[1] = color;
				longs[2] = SIDES*12;
				j = 3;

				// outside
				for (i = 0; i < SIDES; i++)
				{
					longs[j+i*4] = (unsigned long)(verts + SIDES + i);
					if (i == SIDES-1)
					{
						longs[j+1+i*4] = (unsigned long)verts + SIDES;
						longs[j+2+i*4] = (unsigned long)verts;
					}
					else
					{
						longs[j+1+i*4] = (unsigned long)verts + SIDES + i + 1;
						longs[j+2+i*4] = (unsigned long)verts + i + 1;
					}
					longs[j+3+i*4] = (unsigned long)verts + i;
				}
				j += 4*SIDES;

				// inside
				for (i = 0; i < SIDES; i++)
				{
					longs[j+i*4] = (unsigned long)(verts + 2*SIDES + i);
					if (i == SIDES-1)
					{
						longs[j+1+i*4] = (unsigned long)verts + 2*SIDES;
						longs[j+2+i*4] = (unsigned long)verts + 3*SIDES;
					}
					else
					{
						longs[j+1+i*4] = (unsigned long)verts + 2*SIDES + i + 1;
						longs[j+2+i*4] = (unsigned long)verts + 3*SIDES + i + 1;
					}
					longs[j+3+i*4] = (unsigned long)verts + 3*SIDES + i;
				}
				j += 4*SIDES;

				// ring
				for (i = 0; i < SIDES; i++)
				{
					longs[j+i*4] = (unsigned long)(verts + i);
					if (i == SIDES-1)
					{
						longs[j+1+i*4] = (unsigned long)verts;
						longs[j+2+i*4] = (unsigned long)verts + 3*SIDES;
					}
					else
					{
						longs[j+1+i*4] = (unsigned long)verts + i + 1;
						longs[j+2+i*4] = (unsigned long)verts + 3*SIDES + i + 1;
					}
					longs[j+3+i*4] = (unsigned long)verts + 3*SIDES + i;
				}
				j += 4*SIDES;

				longs[j] =  0; j++; // tris
				longs[j] =  0; j++; // lines
				longs[j] =  LC_COL_EDGES; j++; // color
				longs[j] =  0; j++; // quads
				longs[j] =  0; j++; // tris
				longs[j] = 8*SIDES; j++;

				// outside
				for (i = 0; i < SIDES; i++)
				{
					longs[j+i*4] = (unsigned long)verts + i;
					if (i == SIDES-1)
						longs[1+j+i*4] = (unsigned long)verts;
					else
						longs[1+j+i*4] = (unsigned long)verts + i + 1;

					longs[2+j+i*4] = longs[j+i*4] + SIDES;
					longs[3+j+i*4] = longs[1+j+i*4] + SIDES;
				}
				j += 4*SIDES;

				// inside
				for (i = 0; i < SIDES; i++)
				{
					longs[j+i*4] = (unsigned long)verts + 2*SIDES + i;
					if (i == SIDES-1)
						longs[1+j+i*4] = (unsigned long)verts + 2*SIDES;
					else
						longs[1+j+i*4] = (unsigned long)verts + 2*SIDES + i + 1;

					longs[2+j+i*4] = longs[j+i*4] + SIDES;
					longs[3+j+i*4] = longs[1+j+i*4] + SIDES;
				}
			}
			else
			{
				pGroup->drawinfo = malloc(sizeof(unsigned short)*size);
				ushorts = (unsigned short*)pGroup->drawinfo;

				ushorts[0] = 2; // colors
				ushorts[1] = color;
				ushorts[2] = SIDES*12;
				j = 3;

				// outside
				for (i = 0; i < SIDES; i++)
				{
					ushorts[j+i*4] = (unsigned short)(verts + SIDES + i);
					if (i == SIDES-1)
					{
						ushorts[j+1+i*4] = (unsigned short)verts + SIDES;
						ushorts[j+2+i*4] = (unsigned short)verts;
					}
					else
					{
						ushorts[j+1+i*4] = (unsigned short)verts + SIDES + i + 1;
						ushorts[j+2+i*4] = (unsigned short)verts + i + 1;
					}
					ushorts[j+3+i*4] = (unsigned short)verts + i;
				}
				j += 4*SIDES;

				// inside
				for (i = 0; i < SIDES; i++)
				{
					ushorts[j+i*4] = (unsigned short)(verts + 3*SIDES + i);
					if (i == SIDES-1)
					{
						ushorts[j+1+i*4] = (unsigned short)verts + 3*SIDES;
						ushorts[j+2+i*4] = (unsigned short)verts + 2*SIDES;
					}
					else
					{
						ushorts[j+1+i*4] = (unsigned short)verts + 3*SIDES + i + 1;
						ushorts[j+2+i*4] = (unsigned short)verts + 2*SIDES + i + 1;
					}
					ushorts[j+3+i*4] = (unsigned short)verts + 2*SIDES + i;
				}
				j += 4*SIDES;

				// ring
				for (i = 0; i < SIDES; i++)
				{
					ushorts[j+i*4] = (unsigned short)(verts + i);
					if (i == SIDES-1)
					{
						ushorts[j+1+i*4] = (unsigned short)verts;
						ushorts[j+2+i*4] = (unsigned short)verts + 3*SIDES;
					}
					else
					{
						ushorts[j+1+i*4] = (unsigned short)verts + i + 1;
						ushorts[j+2+i*4] = (unsigned short)verts + 3*SIDES + i + 1;
					}
					ushorts[j+3+i*4] = (unsigned short)verts + 3*SIDES + i;
				}
				j += 4*SIDES;

				ushorts[j] =  0; j++; // tris
				ushorts[j] =  0; j++; // lines
				ushorts[j] =  LC_COL_EDGES; j++; // color
				ushorts[j] =  0; j++; // quads
				ushorts[j] =  0; j++; // tris
				ushorts[j] = 8*SIDES; j++;

				// outside
				for (i = 0; i < SIDES; i++)
				{
					ushorts[j+i*4] = (unsigned short)verts + i;
					if (i == SIDES-1)
						ushorts[1+j+i*4] = (unsigned short)verts;
					else
						ushorts[1+j+i*4] = (unsigned short)verts + i + 1;

					ushorts[2+j+i*4] = ushorts[j+i*4] + SIDES;
					ushorts[3+j+i*4] = ushorts[1+j+i*4] + SIDES;
				}
				j += 4*SIDES;

				// inside
				for (i = 0; i < SIDES; i++)
				{
					ushorts[j+i*4] = (unsigned short)verts + 2*SIDES + i;
					if (i == SIDES-1)
						ushorts[1+j+i*4] = (unsigned short)verts + 2*SIDES;
					else
						ushorts[1+j+i*4] = (unsigned short)verts + 2*SIDES + i + 1;

					ushorts[2+j+i*4] = ushorts[j+i*4] + SIDES;
					ushorts[3+j+i*4] = ushorts[1+j+i*4] + SIDES;
				}
			}

			verts += 4*SIDES;
			bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
		} break;

		case LC_STUD3:
		{
			int size;
			Matrix mat((float*)(bytes+2));
			unsigned short color = ConvertColor(*(bytes+1));

			// Create the vertexes
			for (i = 0; i < SIDES; i++)
			{
				m_fVertexArray[(verts+i+SIDES)*3] = 
				m_fVertexArray[(verts+i)*3] = 
					0.16f * costbl[i];
				m_fVertexArray[(verts+i+SIDES)*3+1] = 
				m_fVertexArray[(verts+i)*3+1] = 
					0.16f * sintbl[i];
				m_fVertexArray[(verts+i)*3+2] = 0;
				m_fVertexArray[(verts+i+SIDES)*3+2] = LC_STUD_HEIGHT;
			}
			m_fVertexArray[(verts+2*SIDES)*3] = 0;
			m_fVertexArray[(verts+2*SIDES)*3+1] = 0;
			m_fVertexArray[(verts+2*SIDES)*3+2] = LC_STUD_HEIGHT;

			mat.TransformPoints(&m_fVertexArray[verts*3], 2*SIDES+1);
			// colors + 2*num_prim + sides*prims
			size = 9+SIDES*11;

			if (m_nFlags & LC_PIECE_LONGDATA)
			{
				pGroup->drawinfo = malloc(sizeof(unsigned long)*size);
				longs = (unsigned long*)pGroup->drawinfo;

				longs[0] = 2; // colors
				longs[1] = color;
				longs[2] = SIDES*4;
				j = 3;

				for (i = 0; i < SIDES; i++)
				{
					longs[3+i*4] = (unsigned long)verts + SIDES + i;
					if (i == SIDES-1)
					{
						longs[4+i*4] = (unsigned long)verts + SIDES;
						longs[5+i*4] = (unsigned long)verts;
					}
					else
					{
						longs[4+i*4] = (unsigned long)verts + SIDES + i + 1;
						longs[5+i*4] = (unsigned long)verts + i + 1;
					}
					longs[6+i*4] = (unsigned long)verts + i;
				}
				j += 4*SIDES;
				longs[j] = SIDES*3;
				j++;

				for (i = 0; i < SIDES; i++)
				{
					if (i == SIDES-1)
						longs[j+i*3] = (unsigned short)verts + SIDES;
					else
						longs[j+i*3] = (unsigned short)verts + SIDES + i + 1;
					longs[1+j+i*3] = (unsigned short)verts + SIDES + i;
					longs[2+j+i*3] = (unsigned short)verts + 2*SIDES;
				}

				j += 3*SIDES;
				longs[j] =  0; j++; // lines
				longs[j] =  LC_COL_EDGES; j++; // color
				longs[j] =  0; j++; // quads
				longs[j] =  0; j++; // tris
				longs[j] = 4*SIDES; j++;

				for (i = 0; i < SIDES; i++)
				{
					longs[j+i*4] = (unsigned long)verts + i;
					if (i == SIDES-1)
						longs[1+j+i*4] = (unsigned long)verts;
					else
						longs[1+j+i*4] = (unsigned long)verts + i + 1;

					longs[2+j+i*4] = longs[j+i*4] + SIDES;
					longs[3+j+i*4] = longs[1+j+i*4] + SIDES;
				}
			}
			else
			{
				pGroup->drawinfo = malloc(sizeof(unsigned short)*size);
				ushorts = (unsigned short*)pGroup->drawinfo;

				ushorts[0] = 2; // colors
				ushorts[1] = color;
				ushorts[2] = SIDES*4;
				j = 3;

				for (i = 0; i < SIDES; i++)
				{
					ushorts[3+i*4] = (unsigned short)(verts + SIDES + i);
					if (i == SIDES-1)
					{
						ushorts[4+i*4] = (unsigned short)verts + SIDES;
						ushorts[5+i*4] = (unsigned short)verts;
					}
					else
					{
						ushorts[4+i*4] = (unsigned short)verts + SIDES + i + 1;
						ushorts[5+i*4] = (unsigned short)verts + i + 1;
					}
					ushorts[6+i*4] = (unsigned short)verts + i;
				}
				j += 4*SIDES;
				ushorts[j] = SIDES*3;
				j++;

				for (i = 0; i < SIDES; i++)
				{
					if (i == SIDES-1)
						ushorts[j+i*3] = (unsigned short)verts + SIDES;
					else
						ushorts[j+i*3] = (unsigned short)verts + SIDES + i + 1;
					ushorts[1+j+i*3] = (unsigned short)verts + SIDES + i;
					ushorts[2+j+i*3] = (unsigned short)verts + 2*SIDES;
				}

				j += 3*SIDES;
				ushorts[j] =  0; j++; // lines
				ushorts[j] =  LC_COL_EDGES; j++; // color
				ushorts[j] =  0; j++; // quads
				ushorts[j] =  0; j++; // tris
				ushorts[j] = 4*SIDES; j++;

				for (i = 0; i < SIDES; i++)
				{
					ushorts[j+i*4] = (unsigned short)verts + i;
					if (i == SIDES-1)
						ushorts[1+j+i*4] = (unsigned short)verts;
					else
						ushorts[1+j+i*4] = (unsigned short)verts + i + 1;

					ushorts[2+j+i*4] = ushorts[j+i*4] + SIDES;
					ushorts[3+j+i*4] = ushorts[1+j+i*4] + SIDES;
				}
			}

			verts += 2*SIDES+1;
			bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
		} break;

		case LC_STUD4:
		{
			int size;
			Matrix mat((float*)(bytes+2));
			unsigned short color = ConvertColor(*(bytes+1));

			// Create the vertexes
			for (i = 0; i < SIDES; i++)
			{
				// outside
				m_fVertexArray[(verts+i+SIDES)*3] = 
				m_fVertexArray[(verts+i)*3] = 
					LC_KNOB_RADIUS * costbl[i];
				m_fVertexArray[(verts+i+SIDES)*3+1] = 
				m_fVertexArray[(verts+i)*3+1] = 
					LC_KNOB_RADIUS * sintbl[i];
				m_fVertexArray[(verts+i)*3+2] = LC_STUD_HEIGHT;
				m_fVertexArray[(verts+i+SIDES)*3+2] = 0;

				// inside
				m_fVertexArray[(verts+i+2*SIDES)*3] = 
				m_fVertexArray[(verts+i+3*SIDES)*3] = 
					LC_STUD_RADIUS * costbl[i];
				m_fVertexArray[(verts+i+2*SIDES)*3+1] = 
				m_fVertexArray[(verts+i+3*SIDES)*3+1] = 
					LC_STUD_RADIUS * sintbl[i];
				m_fVertexArray[(verts+i+3*SIDES)*3+2] = LC_STUD_HEIGHT;
				m_fVertexArray[(verts+i+2*SIDES)*3+2] = 0;
			}

			mat.TransformPoints(&m_fVertexArray[verts*3], 4*SIDES);
			// colors + 2*num_prim + sides*prims
			size = 9+SIDES*20;

			if (m_nFlags & LC_PIECE_LONGDATA)
			{
				pGroup->drawinfo = malloc(sizeof(unsigned long)*size);
				longs = (unsigned long*)pGroup->drawinfo;

				longs[0] = 2; // colors
				longs[1] = color;
				longs[2] = SIDES*12;
				j = 3;

				// outside
				for (i = 0; i < SIDES; i++)
				{
					longs[j+i*4] = (unsigned long)(verts + i);
					if (i == SIDES-1)
					{
						longs[j+1+i*4] = (unsigned long)verts;
						longs[j+2+i*4] = (unsigned long)verts + SIDES;
					}
					else
					{
						longs[j+1+i*4] = (unsigned long)verts + i + 1;
						longs[j+2+i*4] = (unsigned long)verts + SIDES + i + 1;
					}
					longs[j+3+i*4] = (unsigned long)verts + SIDES + i;
				}
				j += 4*SIDES;

				// inside
				for (i = 0; i < SIDES; i++)
				{
					longs[j+i*4] = (unsigned long)(verts + 3*SIDES + i);
					if (i == SIDES-1)
					{
						longs[j+1+i*4] = (unsigned long)verts + 3*SIDES;
						longs[j+2+i*4] = (unsigned long)verts + 2*SIDES;
					}
					else
					{
						longs[j+1+i*4] = (unsigned long)verts + 3*SIDES + i + 1;
						longs[j+2+i*4] = (unsigned long)verts + 2*SIDES + i + 1;
					}
					longs[j+3+i*4] = (unsigned long)verts + 2*SIDES + i;
				}
				j += 4*SIDES;

				// ring
				for (i = 0; i < SIDES; i++)
				{
					longs[j+i*4] = (unsigned long)(verts + 3*SIDES + i);
					if (i == SIDES-1)
					{
						longs[j+1+i*4] = (unsigned long)verts + 3*SIDES;
						longs[j+2+i*4] = (unsigned long)verts;
					}
					else
					{
						longs[j+1+i*4] = (unsigned long)verts + 3*SIDES + i + 1;
						longs[j+2+i*4] = (unsigned long)verts + i + 1;
					}
					longs[j+3+i*4] = (unsigned long)verts + i;
				}
				j += 4*SIDES;

				longs[j] =  0; j++; // tris
				longs[j] =  0; j++; // lines
				longs[j] =  LC_COL_EDGES; j++; // color
				longs[j] =  0; j++; // quads
				longs[j] =  0; j++; // tris
				longs[j] = 8*SIDES; j++;

				// outside
				for (i = 0; i < SIDES; i++)
				{
					longs[j+i*4] = (unsigned long)verts + i;
					if (i == SIDES-1)
						longs[1+j+i*4] = (unsigned long)verts;
					else
						longs[1+j+i*4] = (unsigned long)verts + i + 1;

					longs[2+j+i*4] = longs[j+i*4] + SIDES;
					longs[3+j+i*4] = longs[1+j+i*4] + SIDES;
				}
				j += 4*SIDES;

				// inside
				for (i = 0; i < SIDES; i++)
				{
					longs[j+i*4] = (unsigned long)verts + 2*SIDES + i;
					if (i == SIDES-1)
						longs[1+j+i*4] = (unsigned long)verts + 2*SIDES;
					else
						longs[1+j+i*4] = (unsigned long)verts + 2*SIDES + i + 1;

					longs[2+j+i*4] = longs[j+i*4] + SIDES;
					longs[3+j+i*4] = longs[1+j+i*4] + SIDES;
				}
			}
			else
			{
				pGroup->drawinfo = malloc(sizeof(unsigned short)*size);
				ushorts = (unsigned short*)pGroup->drawinfo;

				ushorts[0] = 2; // colors
				ushorts[1] = color;
				ushorts[2] = SIDES*12;
				j = 3;

				// outside
				for (i = 0; i < SIDES; i++)
				{
					ushorts[j+i*4] = (unsigned short)(verts + i);
					if (i == SIDES-1)
					{
						ushorts[j+1+i*4] = (unsigned short)verts;
						ushorts[j+2+i*4] = (unsigned short)verts + SIDES;
					}
					else
					{
						ushorts[j+1+i*4] = (unsigned short)verts + i + 1;
						ushorts[j+2+i*4] = (unsigned short)verts + SIDES + i + 1;
					}
					ushorts[j+3+i*4] = (unsigned short)verts + SIDES + i;
				}
				j += 4*SIDES;

				// inside
				for (i = 0; i < SIDES; i++)
				{
					ushorts[j+i*4] = (unsigned short)(verts + 2*SIDES + i);
					if (i == SIDES-1)
					{
						ushorts[j+1+i*4] = (unsigned short)verts + 2*SIDES;
						ushorts[j+2+i*4] = (unsigned short)verts + 3*SIDES;
					}
					else
					{
						ushorts[j+1+i*4] = (unsigned short)verts + 2*SIDES + i + 1;
						ushorts[j+2+i*4] = (unsigned short)verts + 3*SIDES + i + 1;
					}
					ushorts[j+3+i*4] = (unsigned short)verts + 3*SIDES + i;
				}
				j += 4*SIDES;

				// ring
				for (i = 0; i < SIDES; i++)
				{
					ushorts[j+i*4] = (unsigned short)(verts + 3*SIDES + i);
					if (i == SIDES-1)
					{
						ushorts[j+1+i*4] = (unsigned short)verts + 3*SIDES;
						ushorts[j+2+i*4] = (unsigned short)verts;
					}
					else
					{
						ushorts[j+1+i*4] = (unsigned short)verts + 3*SIDES + i + 1;
						ushorts[j+2+i*4] = (unsigned short)verts + i + 1;
					}
					ushorts[j+3+i*4] = (unsigned short)verts + i;
				}
				j += 4*SIDES;

				ushorts[j] =  0; j++; // tris
				ushorts[j] =  0; j++; // lines
				ushorts[j] =  LC_COL_EDGES; j++; // color
				ushorts[j] =  0; j++; // quads
				ushorts[j] =  0; j++; // tris
				ushorts[j] = 8*SIDES; j++;

				// outside
				for (i = 0; i < SIDES; i++)
				{
					ushorts[j+i*4] = (unsigned short)verts + i;
					if (i == SIDES-1)
						ushorts[1+j+i*4] = (unsigned short)verts;
					else
						ushorts[1+j+i*4] = (unsigned short)verts + i + 1;

					ushorts[2+j+i*4] = ushorts[j+i*4] + SIDES;
					ushorts[3+j+i*4] = ushorts[1+j+i*4] + SIDES;
				}
				j += 4*SIDES;

				// inside
				for (i = 0; i < SIDES; i++)
				{
					ushorts[j+i*4] = (unsigned short)verts + 2*SIDES + i;
					if (i == SIDES-1)
						ushorts[1+j+i*4] = (unsigned short)verts + 2*SIDES;
					else
						ushorts[1+j+i*4] = (unsigned short)verts + 2*SIDES + i + 1;

					ushorts[2+j+i*4] = ushorts[j+i*4] + SIDES;
					ushorts[3+j+i*4] = ushorts[1+j+i*4] + SIDES;
				}
			}

			verts += 4*SIDES;
			bytes += 2*sizeof(unsigned char) + 12*sizeof(float);
		} break;
		}
		bytes++; // should be 0
	}

	free(buf);
	
/*
	// Now create the information for the CD
	// If the object is big this can block the program for serveral seconds.
	// ATTENTION: The RAPID CD library is based on triangles.

	if (pInfo->pRModel)
		delete pInfo->pRModel;

    pInfo->pRModel = new CRModel();
    pInfo->pRModel->BeginModel();

	UINT col, loc, j, i;
	int vert = 0;

	for (UINT c = 0; c < pInfo->cons; c++)
	{
		if (pInfo->connection[c].info == NULL)
			continue;
		if (pInfo->count > 65535)
		{
			UINT* info = (UINT*)pInfo->connection[c].info;
			loc = 1;
			col = info[0];
			while (col)
			{
				loc++;

				j = info[loc];
				for (i = 0; i < j; i+=4)
				{
					pInfo->pRModel->AddTri(&pInfo->vertex[info[loc+i+1]*3], &pInfo->vertex[info[loc+i+2]*3],
						&pInfo->vertex[info[loc+i+3]*3], vert);
					vert++;
					pInfo->pRModel->AddTri(&pInfo->vertex[info[loc+i+3]*3], &pInfo->vertex[info[loc+i+4]*3],
						&pInfo->vertex[info[loc+i+1]*3], vert);
					vert++;
				}
				loc += j+1;
				j = info[loc];
				for (i = 0; i < j; i+=3)
				{
					pInfo->pRModel->AddTri(&pInfo->vertex[info[loc+i+1]*3], &pInfo->vertex[info[loc+i+2]*3],
						&pInfo->vertex[info[loc+i+3]*3], vert);
					vert++;
				}
				loc += j+1;
				loc += info[loc]+1;

				col--;
			}
		}
		else
		{
			WORD* info = (WORD*)pInfo->connection[c].info;
			loc = 1;
			col = info[0];
			while (col)
			{
				loc++;
				
				j = info[loc];
				for (i = 0; i < j; i+=4)
				{
					pInfo->pRModel->AddTri(&pInfo->vertex[info[loc+i+1]*3], &pInfo->vertex[info[loc+i+2]*3],
						&pInfo->vertex[info[loc+i+3]*3], vert);
					vert++;
					pInfo->pRModel->AddTri(&pInfo->vertex[info[loc+i+3]*3], &pInfo->vertex[info[loc+i+4]*3],
						&pInfo->vertex[info[loc+i+1]*3], vert);
					vert++;
				}
				loc += j+1;
				j = info[loc];
				for (i = 0; i < j; i+=3)
				{
					pInfo->pRModel->AddTri(&pInfo->vertex[info[loc+i+1]*3], &pInfo->vertex[info[loc+i+2]*3],
						&pInfo->vertex[info[loc+i+3]*3], vert);
					vert++;
				}
				loc += j+1;
				loc += info[loc]+1;
				
				col--;
			}
		}
	}
    pInfo->pRModel->EndModel();
*/
}

void PieceInfo::FreeInformation()
{
  if (m_nBoxList != 0)
	glDeleteLists(m_nBoxList, 1);
	m_nBoxList = 0;

	if (m_fVertexArray != NULL)
	{
		free(m_fVertexArray);
		m_fVertexArray = NULL;
		m_nVertexCount = 0;
	}

	if (m_pConnections != NULL)
	{
		free(m_pConnections);
		m_pConnections = NULL;
		m_nConnectionCount = 0;
	}

	if (m_pGroups != NULL)
	{
		while (m_nGroupCount--)
			if (m_pGroups[m_nGroupCount].drawinfo)
				free(m_pGroups[m_nGroupCount].drawinfo);

		free(m_pGroups);
		m_pGroups = NULL;
	}

	if (m_pTextures != NULL)
	{
//		while (m_nTextureCount--)
//			if (m_pTextures[m_nTextureCount].texture)
//				m_pTextures[m_nTextureCount].texture->DeRef();

		free(m_pTextures);
		m_pTextures = NULL;
	}
}

// Zoom extents for the preview window & print catalog
void PieceInfo::ZoomExtents()
{
	// TODO: Calculate this in the right way
	bool out = false;
	GLdouble modelMatrix[16], projMatrix[16];
	GLdouble obj1x, obj1y, obj1z, obj2x, obj2y, obj2z;
	double View[3] = { -5, -5, 3 };
	GLint viewport[4];
	float v[24] = {
		m_fDimensions[0], m_fDimensions[1], m_fDimensions[5],
		m_fDimensions[3], m_fDimensions[1], m_fDimensions[5],
		m_fDimensions[0], m_fDimensions[1], m_fDimensions[2],
		m_fDimensions[3], m_fDimensions[4], m_fDimensions[5],
		m_fDimensions[3], m_fDimensions[4], m_fDimensions[2],
		m_fDimensions[0], m_fDimensions[4], m_fDimensions[2],
		m_fDimensions[0], m_fDimensions[4], m_fDimensions[5],
		m_fDimensions[3], m_fDimensions[1], m_fDimensions[2] };

	gluLookAt (View[0], View[1], View[2], (m_fDimensions[0] + m_fDimensions[3])/2, (m_fDimensions[1] + m_fDimensions[4])/2, (m_fDimensions[2] + m_fDimensions[5])/2, 0, 0, 1);
	glGetDoublev(GL_MODELVIEW_MATRIX,modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX,projMatrix);
	glGetIntegerv(GL_VIEWPORT,viewport);

	gluUnProject((double)(viewport[2]/2),(double)(viewport[3]/2),0,
		modelMatrix,projMatrix,viewport,&obj1x,&obj1y,&obj1z);
	gluUnProject((double)(viewport[2]/2),(double)(viewport[3]/2),1,
		modelMatrix,projMatrix,viewport,&obj2x,&obj2y,&obj2z);

	double d = 2*sqrt((obj2x-obj1x)*(obj2x-obj1x)+(obj2y-obj1y)*(obj2y-obj1y)+(obj2z-obj1z)*(obj2z-obj1z));
	while (!out) // Zoom in
	{
		View[0] += (obj2x-obj1x)/d;
		View[1] += (obj2y-obj1y)/d;
		View[2] += (obj2z-obj1z)/d;
		glLoadIdentity();
		gluLookAt (View[0], View[1], View[2], (m_fDimensions[0] + m_fDimensions[3])/2, (m_fDimensions[1] + m_fDimensions[4])/2, (m_fDimensions[2] + m_fDimensions[5])/2, 0, 0, 1);
		
		for (int i = 0; i < 24; i+=3)
		{
			double winx, winy, winz;
			if (gluProject (v[i], v[i+1], v[i+2], modelMatrix, projMatrix, viewport, &winx, &winy, &winz) == GL_TRUE)
				if ((winx < viewport[0] + 1) || (winy < viewport[1] + 1) || 
					(winx > viewport[2] - 1) || (winy > viewport[3] - 1))
					out = true;
		}
		
		glGetDoublev(GL_MODELVIEW_MATRIX,modelMatrix);
		glGetDoublev(GL_PROJECTION_MATRIX,projMatrix);
		glGetIntegerv(GL_VIEWPORT,viewport);
	}

	while (out)
	{
		out = false;
		View[0] -= (obj2x-obj1x)/d;
		View[1] -= (obj2y-obj1y)/d;
		View[2] -= (obj2z-obj1z)/d;
		glLoadIdentity();
		gluLookAt (View[0], View[1], View[2], (m_fDimensions[0] + m_fDimensions[3])/2, (m_fDimensions[1] + m_fDimensions[4])/2, (m_fDimensions[2] + m_fDimensions[5])/2, 0, 0, 1);
		
		for (int i = 0; i < 24; i+=3)
		{
			double winx, winy, winz;
			if (gluProject (v[i], v[i+1], v[i+2], modelMatrix, projMatrix, viewport, &winx, &winy, &winz) == GL_TRUE)
				if ((winx < viewport[0] + 1) || (winy < viewport[1] + 1) || 
					(winx > viewport[2] - 1) || (winy > viewport[3] - 1))
					out = true;
		}
		glGetDoublev(GL_MODELVIEW_MATRIX,modelMatrix);
		glGetDoublev(GL_PROJECTION_MATRIX,projMatrix);
		glGetIntegerv(GL_VIEWPORT,viewport);
	}
}

// Used by the print catalog and HTML instructions functions.
void PieceInfo::RenderOnce(int nColor)
{
	AddRef();
	RenderPiece(nColor);
	DeRef();
}

// Called by the piece preview and from RenderOnce()
void PieceInfo::RenderPiece(int nColor)
{
	unsigned short sh, curcolor;
	DRAWGROUP* pGroup;

	for (sh = 0; sh < m_nTextureCount; sh++)
	{
//		if (!m_pTextures[sh].texture->IsLoaded())
//			m_pTextures[sh].texture->Load(false);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		m_pTextures[sh].texture->MakeCurrent();

		if (m_pTextures[sh].color == LC_COL_DEFAULT)
			glColor3ubv(FlatColorArray[nColor]);
		if (nColor > 13 && nColor < 22)
		{
//			glEnable (GL_POLYGON_STIPPLE);
			glEnable (GL_BLEND);
			glDepthMask (GL_FALSE);
		}
		else
		{
//			glDisable (GL_POLYGON_STIPPLE);
			glDepthMask (GL_TRUE);
			glDisable (GL_BLEND);
		}

		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glTexCoord2fv(m_pTextures[sh].coords[0]);
		glVertex3fv(m_pTextures[sh].vertex[0]);
		glTexCoord2fv(m_pTextures[sh].coords[1]);
		glVertex3fv(m_pTextures[sh].vertex[1]);
		glTexCoord2fv(m_pTextures[sh].coords[2]);
		glVertex3fv(m_pTextures[sh].vertex[2]);
		glTexCoord2fv(m_pTextures[sh].coords[3]);
		glVertex3fv(m_pTextures[sh].vertex[3]);
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer (3, GL_FLOAT, 0, m_fVertexArray);

	sh = m_nGroupCount;
	for (pGroup = m_pGroups; sh--; pGroup++)
	{
		if (m_nFlags & LC_PIECE_LONGDATA)
		{
			unsigned long* info, colors;

			info = (unsigned long*)pGroup->drawinfo;
			colors = *info;
			info++;

			while (colors--)
			{
				if (*info == LC_COL_DEFAULT)
					curcolor = nColor;
				else
					curcolor = (unsigned short)*info;
				info++;

				glColor3ubv(FlatColorArray[curcolor]);
				if (curcolor > 13 && curcolor < 22)
				{
//					glEnable (GL_POLYGON_STIPPLE);
					glEnable (GL_BLEND);
					glDepthMask (GL_FALSE);
				}
				else
				{
//					glDisable (GL_POLYGON_STIPPLE);
					glDepthMask (GL_TRUE);
					glDisable (GL_BLEND);
				}

				if (*info)
					glDrawElements(GL_QUADS, *info, GL_UNSIGNED_INT, info+1);
				info += *info + 1;
				if (*info)
					glDrawElements(GL_TRIANGLES, *info, GL_UNSIGNED_INT, info+1);
				info += *info + 1;
				if (*info)
					glDrawElements(GL_LINES, *info, GL_UNSIGNED_INT, info+1);
				info += *info + 1;
			}
		}
		else
		{
			unsigned short* info, colors;

			info = (unsigned short*)pGroup->drawinfo;
			colors = *info;
			info++;

			while (colors--)
			{
				if (*info == LC_COL_DEFAULT)
					curcolor = nColor;
				else
					curcolor = *info;
				info++;

				glColor3ubv(FlatColorArray[curcolor]);
				if (curcolor > 13 && curcolor < 22)
				{
//					glEnable (GL_POLYGON_STIPPLE);
					glEnable (GL_BLEND);
					glDepthMask (GL_FALSE);
				}
				else
				{
//					glDisable (GL_POLYGON_STIPPLE);
					glDepthMask (GL_TRUE);
					glDisable (GL_BLEND);
				}

				if (*info)
					glDrawElements(GL_QUADS, *info, GL_UNSIGNED_SHORT, info+1);
				info += *info + 1;
				if (*info)
					glDrawElements(GL_TRIANGLES, *info, GL_UNSIGNED_SHORT, info+1);
				info += *info + 1;
				if (*info)
					glDrawElements(GL_LINES, *info, GL_UNSIGNED_SHORT, info+1);
				info += *info + 1;
			}
		}
	}
}

void PieceInfo::WriteWavefront(FILE* file, unsigned char color, unsigned long* start)
{
	unsigned short group;
	const char* colname;
	
	for (group = 0; group < m_nGroupCount; group++)
	{
		if (m_nFlags & LC_PIECE_LONGDATA)
		{
			unsigned long* info = (unsigned long*)m_pGroups[group].drawinfo;
			unsigned long count, colors = *info;
			info++;

			while (colors--)
			{
				if (*info == LC_COL_DEFAULT)
					colname = altcolornames[color];
				else
				{
					if (*info >= LC_MAXCOLORS)
					{
						info++;
						info += *info + 1;
						info += *info + 1;
						info += *info + 1;
						continue;
					}
					colname = altcolornames[*info];
				}
				info++;

				// skip if color only have lines
				if ((*info == 0) && (info[1] == 0))
				{
					info += 2;
					info += *info + 1;
					continue;
				}

				fprintf(file, "usemtl %s\n", colname);

				for (count = *info, info++; count; count -= 4)
				{
					fprintf(file, "f %ld %ld %ld %ld\n", 
						*info+*start, info[1]+*start, info[2]+*start, info[3]+*start);
					info += 4;
				}

				for (count = *info, info++; count; count -= 3)
				{
					fprintf(file, "f %ld %ld %ld\n", 
						*info+*start, info[1]+*start, info[2]+*start);
					info += 3;
				}
				info += *info + 1;
			}
		}
		else
		{
			unsigned short* info = (unsigned short*)m_pGroups[group].drawinfo;
			unsigned short count, colors = *info;
			info++;

			while (colors--)
			{
				if (*info == LC_COL_DEFAULT)
					colname = altcolornames[color];
				else
				{
					if (*info >= LC_MAXCOLORS)
					{
						info++;
						info += *info + 1;
						info += *info + 1;
						info += *info + 1;
						continue;
					}
					colname = altcolornames[*info];
				}
				info++;

				// skip if color only have lines
				if ((*info == 0) && (info[1] == 0))
				{
					info += 2;
					info += *info + 1;
					continue;
				}

				fprintf(file, "usemtl %s\n", colname);

				for (count = *info, info++; count; count -= 4)
				{
					fprintf(file, "f %ld %ld %ld %ld\n", 
						*info+*start, info[1]+*start, info[2]+*start, info[3]+*start);
					info += 4;
				}

				for (count = *info, info++; count; count -= 3)
				{
					fprintf(file, "f %ld %ld %ld\n", 
						*info+*start, info[1]+*start, info[2]+*start);
					info += 3;
				}
				info += *info + 1;
			}

		}
	}

	*start += m_nVertexCount;
	fputs("\n", file);
}
