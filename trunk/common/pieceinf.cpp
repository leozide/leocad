// Information about how to draw a piece and some more stuff.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "opengl.h"
#include "texture.h"
#include "pieceinf.h"
#include "project.h"
#include "globals.h"
#include "matrix.h"
#include "vector.h"
#include "defines.h"
#include "config.h"
#include "library.h"
#include "lc_application.h"

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

static void GetFrustumPlanes (float planes[6][4])
{
  // Storage for the Modelview, Projection and their multiplication (Frustum) matrix.
  float mv[16], pj[16], fm[16];

  glGetFloatv(GL_MODELVIEW_MATRIX, mv);
  glGetFloatv(GL_PROJECTION_MATRIX, pj);

  fm[0]  = pj[0] * mv[0]  + pj[4] * mv[1]  + pj[8]  * mv[2]  + pj[12] * mv[3];
  fm[4]  = pj[0] * mv[4]  + pj[4] * mv[5]  + pj[8]  * mv[6]  + pj[12] * mv[7];
  fm[8]  = pj[0] * mv[8]  + pj[4] * mv[9]  + pj[8]  * mv[10] + pj[12] * mv[11];
  fm[12] = pj[0] * mv[12] + pj[4] * mv[13] + pj[8]  * mv[14] + pj[12] * mv[15];
  fm[1]  = pj[1] * mv[0]  + pj[5] * mv[1]  + pj[9]  * mv[2]  + pj[13] * mv[3];
  fm[5]  = pj[1] * mv[4]  + pj[5] * mv[5]  + pj[9]  * mv[6]  + pj[13] * mv[7];
  fm[9]  = pj[1] * mv[8]  + pj[5] * mv[9]  + pj[9]  * mv[10] + pj[13] * mv[11];
  fm[13] = pj[1] * mv[12] + pj[5] * mv[13] + pj[9]  * mv[14] + pj[13] * mv[15];
  fm[2]  = pj[2] * mv[0]  + pj[6] * mv[1]  + pj[10] * mv[2]  + pj[14] * mv[3];
  fm[6]  = pj[2] * mv[4]  + pj[6] * mv[5]  + pj[10] * mv[6]  + pj[14] * mv[7];
  fm[10] = pj[2] * mv[8]  + pj[6] * mv[9]  + pj[10] * mv[10] + pj[14] * mv[11];
  fm[14] = pj[2] * mv[12] + pj[6] * mv[13] + pj[10] * mv[14] + pj[14] * mv[15];
  fm[3]  = pj[3] * mv[0]  + pj[7] * mv[1]  + pj[11] * mv[2]  + pj[15] * mv[3];
  fm[7]  = pj[3] * mv[4]  + pj[7] * mv[5]  + pj[11] * mv[6]  + pj[15] * mv[7];
  fm[11] = pj[3] * mv[8]  + pj[7] * mv[9]  + pj[11] * mv[10] + pj[15] * mv[11];
  fm[15] = pj[3] * mv[12] + pj[7] * mv[13] + pj[11] * mv[14] + pj[15] * mv[15];

  planes[0][0] = (fm[0] - fm[3]) * -1;
  planes[0][1] = (fm[4] - fm[7]) * -1;
  planes[0][2] = (fm[8] - fm[11]) * -1;
  planes[0][3] = (fm[12] - fm[15]) * -1;
  planes[1][0] = fm[0] + fm[3];
  planes[1][1] = fm[4] + fm[7];
  planes[1][2] = fm[8] + fm[11];
  planes[1][3] = fm[12] + fm[15];
  planes[2][0] = (fm[1] - fm[3]) * -1;
  planes[2][1] = (fm[5] - fm[7]) * -1;
  planes[2][2] = (fm[9] - fm[11]) * -1;
  planes[2][3] = (fm[13] - fm[15]) * -1;
  planes[3][0] = fm[1] + fm[3];
  planes[3][1] = fm[5] + fm[7];
  planes[3][2] = fm[9] + fm[11];
  planes[3][3] = fm[13] + fm[15];
  planes[4][0] = (fm[2] - fm[3]) * -1;
  planes[4][1] = (fm[6] - fm[7]) * -1;
  planes[4][2] = (fm[10] - fm[11]) * -1;
  planes[4][3] = (fm[14] - fm[15]) * -1;
  planes[5][0] = fm[2] + fm[3];
  planes[5][1] = fm[6] + fm[7];
  planes[5][2] = fm[10] + fm[11];
  planes[5][3] = fm[14] + fm[15];
}

bool BoxOutsideFrustum (float Dimensions[6])
{
  float d, planes[6][4], verts[8][3] = {
    { Dimensions[0], Dimensions[1], Dimensions[5] },
    { Dimensions[3], Dimensions[1], Dimensions[5] },
    { Dimensions[0], Dimensions[1], Dimensions[2] },
    { Dimensions[3], Dimensions[4], Dimensions[5] },
    { Dimensions[3], Dimensions[4], Dimensions[2] },
    { Dimensions[0], Dimensions[4], Dimensions[2] },
    { Dimensions[0], Dimensions[4], Dimensions[5] },
    { Dimensions[3], Dimensions[1], Dimensions[2] } };

  GetFrustumPlanes (planes);

  for (int i = 0; i < 6; i++)
    for (int j = 0; j < 8; j++)
    {
      d = verts[j][0]*planes[i][0] + verts[j][1]*planes[i][1] + verts[j][2]*planes[i][2] + planes[i][3];
      if (d < -0.001f)
	return true;
    }
  return false;
}

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

PieceInfo::PieceInfo ()
{
  // Do nothing, initialization is done by LoadIndex ()
}

PieceInfo::~PieceInfo ()
{
  FreeInformation ();
}

/////////////////////////////////////////////////////////////////////////////
// File I/O

void PieceInfo::LoadIndex (File& file)
{
  static bool init = false;
  short sh[6];
  short scale;

  // Initialize sin/cos table
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
  m_nBoxList = 0;

  file.Read (m_strName, LC_PIECE_NAME_LEN);
  file.Read (m_strDescription, 64);
  m_strDescription[64] = '\0';
  file.ReadShort (sh, 6);
  file.ReadByte (&m_nFlags, 1);
  lcuint32 Groups; file.ReadLong (&Groups, 1);
  file.ReadLong (&m_nOffset, 1);
  file.ReadLong (&m_nSize, 1);

  if (m_nFlags & LC_PIECE_SMALL)
    scale = 10000;
  else if (m_nFlags & LC_PIECE_MEDIUM)
    scale = 1000;
  else
    scale = 100;

  m_fDimensions[0] = (float)sh[0]/scale;
  m_fDimensions[1] = (float)sh[1]/scale;
  m_fDimensions[2] = (float)sh[2]/scale;
  m_fDimensions[3] = (float)sh[3]/scale;
  m_fDimensions[4] = (float)sh[4]/scale;
  m_fDimensions[5] = (float)sh[5]/scale;
}

void PieceInfo::AddRef()
{
	if (m_nRef == 0)
		LoadInformation();
	m_nRef++;

	for (int i = 0; i < m_nTextureCount; i++)
		if (m_pTextures[i].texture != NULL)
			m_pTextures[i].texture->AddRef(false);
// TODO: get correct filter paramenter
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

void PieceInfo::CreateBoxDisplayList()
{
	if (m_nBoxList)
		return;

	// Create a display for the bounding box.
	m_nBoxList = glGenLists(1);
	glNewList(m_nBoxList, GL_COMPILE);
	glEnableClientState(GL_VERTEX_ARRAY);

	float box[24][3] =
	{
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

	glVertexPointer(3, GL_FLOAT, 0, box);
	glDrawArrays(GL_QUADS, 0, 24);
	glEndList();
}

void PieceInfo::LoadInformation()
{
  FileDisk bin;
  char filename[LC_MAXPATH];
  CONNECTIONINFO* pConnection;
  DRAWGROUP* pGroup;
  void* buf;
  lcuint32 verts, *longs, fixverts;
  lcuint16 *ushorts, sh;
  lcuint8 *bytes, *tmp, bt;
  float scale, shift;
  lcint16* shorts;
  int i, j;

  // We don't want memory leaks.
  FreeInformation ();

  // Open pieces.bin and buffer the information we need.
  strcpy (filename, lcGetPiecesLibrary()->GetLibraryPath());
  strcat (filename, "pieces.bin");
  if (!bin.Open (filename, "rb"))
    return;

  buf = malloc(m_nSize);
  bin.Seek(m_nOffset, SEEK_SET);
  bin.Read(buf, m_nSize);
  bin.Close();

  shift  = 1.0f/(1<<14);
  scale = 0.01f;
  if (m_nFlags & LC_PIECE_MEDIUM) scale = 0.001f;
  if (m_nFlags & LC_PIECE_SMALL)  scale = 0.0001f;
  longs = (lcuint32*)buf;
  fixverts = verts = LCUINT32(*longs);
  bytes = (unsigned char*)(longs + 1);
  bytes += verts * sizeof(lcint16) * 3;

  // Read connections
  m_nConnectionCount = LCUINT16(*((lcuint16*)bytes));
  bytes += sizeof (lcuint16);
  m_pConnections = (CONNECTIONINFO*)malloc (m_nConnectionCount * sizeof(CONNECTIONINFO));

  sh = m_nConnectionCount;
  for (pConnection = m_pConnections; sh--; pConnection++)
  {
    pConnection->type = *bytes;
    bytes++;

    shorts = (lcint16*)bytes;
    pConnection->center[0] = (float)(LCINT16(*shorts))*scale;
    shorts++;
    pConnection->center[1] = (float)(LCINT16(*shorts))*scale;
    shorts++;
    pConnection->center[2] = (float)(LCINT16(*shorts))*scale;
    shorts++;
    pConnection->normal[0] = (float)(LCINT16(*shorts))*shift;
    shorts++;
    pConnection->normal[1] = (float)(LCINT16(*shorts))*shift;
    shorts++;
    pConnection->normal[2] = (float)(LCINT16(*shorts))*shift;
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
    tex->texture = lcGetPiecesLibrary()->FindTexture(name);

    shorts = (lcint16*)(bytes + 8);
    for (i = 0; i < 4; i++)
    {
      tex->vertex[i][0] = (float)LCINT16(shorts[0])*scale;
      tex->vertex[i][1] = (float)LCINT16(shorts[1])*scale;
      tex->vertex[i][2] = (float)LCINT16(shorts[2])*scale;
      shorts += 3;
    }

    for (i = 0; i < 4; i++)
    {
      tex->coords[i][0] = (float)LCINT16(shorts[0]);
      tex->coords[i][1] = (float)LCINT16(shorts[1]);
      shorts += 2;
    }

    bytes += 8 + 20*sizeof(lcuint16);
  }

  // Read groups
  m_nGroupCount = LCUINT16(*((lcuint16*)bytes));
  bytes += sizeof(lcuint16);
  m_pGroups = (DRAWGROUP*)malloc(sizeof(DRAWGROUP)*m_nGroupCount);
  memset(m_pGroups, 0, sizeof(DRAWGROUP)*m_nGroupCount);

  // First we need to know the number of vertexes
  tmp = bytes;
  sh = m_nGroupCount;
  lcuint32 quads = 0, fixquads = 0;
  while (sh--)
  {
    bt = *bytes;
    bytes++;
    bytes += bt*sizeof(lcuint16);

		while (*bytes)
		{
      if (*bytes == LC_MESH)
      {
				if ((fixverts > 65535) || (m_nFlags & LC_PIECE_LONGDATA))
				{
					lcuint32 colors, *p;
					p = (lcuint32*)(bytes + 1);
					colors = LCUINT32(*p);
					p++;

					while (colors--)
					{
						p++; // color code
						quads += LCUINT32(*p);
						fixquads += LCUINT32(*p);
						p += LCUINT32(*p) + 1;
						p += LCUINT32(*p) + 1;
						p += LCUINT32(*p) + 1;
					}

					bytes = (unsigned char*)p;
				}
				else
				{
					lcuint16 colors, *p;
					p = (lcuint16*)(bytes + 1);
					colors = LCUINT16(*p);
					p++;

					while (colors--)
					{
						p++; // color code
						quads += LCUINT16(*p);
						fixquads += LCUINT16(*p);
						p += LCUINT16(*p) + 1;
						p += LCUINT16(*p) + 1;
						p += LCUINT16(*p) + 1;
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
	if ((verts > 65535) || (quads > 65535) || (fixquads > 65535))
	{
		if ((m_nFlags & LC_PIECE_LONGDATA) == 0)
		{
			m_nFlags |= LC_PIECE_LONGDATA | LC_PIECE_LONGDATA_RUNTIME;
		}
	}
	else
		m_nFlags &= ~(LC_PIECE_LONGDATA | LC_PIECE_LONGDATA_RUNTIME);

  // Copy the 'fixed' vertexes
  shorts = (lcint16*)(longs + 1);
  for (verts = 0; verts < LCUINT32(*longs); verts++)
  {
    m_fVertexArray[verts*3] = (float)LCINT16(*shorts)*scale;
    shorts++;
    m_fVertexArray[verts*3+1] = (float)LCINT16(*shorts)*scale;
    shorts++;
    m_fVertexArray[verts*3+2] = (float)LCINT16(*shorts)*scale;
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
      lcuint16 tmp = LCUINT16(*((lcuint16*)bytes));
      pGroup->connections[bt] = tmp;
      bytes += sizeof(lcuint16);
    }

    // Currently there's only one type of drawinfo (mesh or stud)
    // per group but this will change in the future.
    switch (*bytes)
    {
    case LC_MESH:
      if ((fixverts > 65535) || (fixquads > 65535))
      {
				lcuint32 colors, *p;
				bytes++;
				p = (lcuint32*)bytes;
        *p = LCUINT32(*p);
				colors = *p;
				p++;

				while (colors--)
				{
					*p = ConvertColor(LCUINT32(*p));
					p++; // color code
#ifdef LC_BIG_ENDIAN
					int f;
					f = LCUINT32(*p) + 1;
					while (f--) { *p = LCUINT32(*p); p++; };
					f = LCUINT32(*p) + 1;
					while (f--) { *p = LCUINT32(*p); p++; };
					f = LCUINT32(*p) + 1;
					while (f--) { *p = LCUINT32(*p); p++; };
#else
					p += LCUINT32(*p) + 1;
					p += LCUINT32(*p) + 1;
					p += LCUINT32(*p) + 1;
#endif
				}

				i = (unsigned char*)p - bytes;
				pGroup->drawinfo = malloc(i);
				memcpy(pGroup->drawinfo, bytes, i);
				bytes = (unsigned char*)p;
      }
      else
      {
				lcuint16 colors, *p;
				bytes++;
				p = (lcuint16*)bytes;
				*p = LCUINT16(*p);
				colors = *p;
				p++;

				while (colors--)
				{
					*p = ConvertColor(LCUINT16(*p));
					p++; // color code
#ifdef LC_BIG_ENDIAN
					int f;
					f = LCUINT16(*p) + 1;
					while (f--) { *p = LCUINT16(*p); p++; };
					f = LCUINT16(*p) + 1;
					while (f--) { *p = LCUINT16(*p); p++; };
					f = LCUINT16(*p) + 1;
					while (f--) { *p = LCUINT16(*p); p++; };
#else
					p += *p + 1;
					p += *p + 1;
					p += *p + 1;
#endif
				}

				i = (unsigned char*)p - bytes;

				if (m_nFlags & LC_PIECE_LONGDATA)
				{
					pGroup->drawinfo = malloc(i*sizeof(lcuint32)/sizeof(lcuint16));
					longs = (lcuint32*)pGroup->drawinfo;

					for (ushorts = (lcuint16*)bytes; ushorts != p; ushorts++, longs++)
						*longs = *ushorts;//LCUINT16(*ushorts);
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
      Matrix mat;

      for (i = 0; i < 12; i++)
        ((float*)(bytes+2))[i] = LCFLOAT (((float*)(bytes+2))[i]);
      mat.FromPacked ((float*)(bytes+2));
      lcuint16 color = ConvertColor(*(bytes+1));

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
	pGroup->drawinfo = malloc(sizeof(lcuint32)*size);
	longs = (lcuint32*)pGroup->drawinfo;

	longs[0] = 2; // colors
	longs[1] = color;
	longs[2] = SIDES*4;
	j = 3;

	for (i = 0; i < SIDES; i++)
	{
	  longs[3+i*4] = (lcuint32)verts + i;
	  if (i == SIDES-1)
	  {
	    longs[4+i*4] = (lcuint32)verts;
	    longs[5+i*4] = (lcuint32)verts + SIDES;
	  }
	  else
	  {
	    longs[4+i*4] = (lcuint32)verts + i + 1;
	    longs[5+i*4] = (lcuint32)verts + SIDES + i + 1;
	  }
	  longs[6+i*4] = (lcuint32)verts + SIDES + i;
	}
	j += 4*SIDES;
	longs[j] = SIDES*3;
	j++;

	for (i = 0; i < SIDES; i++)
	{
	  longs[j+i*3] = (lcuint16)verts + 2*SIDES;
	  longs[1+j+i*3] = (lcuint16)verts + SIDES + i;
	  if (i == SIDES-1)
	    longs[2+j+i*3] = (lcuint16)verts + SIDES;
	  else
	    longs[2+j+i*3] = (lcuint16)verts + SIDES + i + 1;
	}

	j += 3*SIDES;
	longs[j] =  0; j++; // lines
	longs[j] =  LC_COL_EDGES; j++; // color
	longs[j] =  0; j++; // quads
	longs[j] =  0; j++; // tris
	longs[j] = 4*SIDES; j++;

	for (i = 0; i < SIDES; i++)
	{
	  longs[j+i*4] = (lcuint32)verts + i;
	  if (i == SIDES-1)
	    longs[1+j+i*4] = (lcuint32)verts;
	  else
	    longs[1+j+i*4] = (lcuint32)verts + i + 1;

	  longs[2+j+i*4] = longs[j+i*4] + SIDES;
	  longs[3+j+i*4] = longs[1+j+i*4] + SIDES;
	}
      }
      else
      {
	pGroup->drawinfo = malloc(sizeof(lcuint16)*size);
	ushorts = (lcuint16*)pGroup->drawinfo;

	ushorts[0] = 2; // colors
	ushorts[1] = color;
	ushorts[2] = SIDES*4;
	j = 3;

	for (i = 0; i < SIDES; i++)
	{
	  ushorts[3+i*4] = (lcuint16)(verts + i);
	  if (i == SIDES-1)
	  {
	    ushorts[4+i*4] = (lcuint16)verts;
	    ushorts[5+i*4] = (lcuint16)verts + SIDES;
	  }
	  else
	  {
	    ushorts[4+i*4] = (lcuint16)verts + i + 1;
	    ushorts[5+i*4] = (lcuint16)verts + SIDES + i + 1;
	  }
	  ushorts[6+i*4] = (lcuint16)verts + SIDES + i;
	}
	j += 4*SIDES;
	ushorts[j] = SIDES*3;
	j++;

	for (i = 0; i < SIDES; i++)
	{
	  ushorts[j+i*3] = (lcuint16)verts + 2*SIDES;
	  ushorts[1+j+i*3] = (lcuint16)verts + SIDES + i;
	  if (i == SIDES-1)
	    ushorts[2+j+i*3] = (lcuint16)verts + SIDES;
	  else
	    ushorts[2+j+i*3] = (lcuint16)verts + SIDES + i + 1;
	}

	j += 3*SIDES;
	ushorts[j] =  0; j++; // lines
	ushorts[j] =  LC_COL_EDGES; j++; // color
	ushorts[j] =  0; j++; // quads
	ushorts[j] =  0; j++; // tris
	ushorts[j] = 4*SIDES; j++;

	for (i = 0; i < SIDES; i++)
	{
	  ushorts[j+i*4] = (lcuint16)verts + i;
	  if (i == SIDES-1)
	    ushorts[1+j+i*4] = (lcuint16)verts;
	  else
	    ushorts[1+j+i*4] = (lcuint16)verts + i + 1;

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
      Matrix mat;

      for (i = 0; i < 12; i++)
        ((float*)(bytes+2))[i] = LCFLOAT (((float*)(bytes+2))[i]);
      mat.FromPacked ((float*)(bytes+2));
      lcuint16 color = ConvertColor(*(bytes+1));

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
	pGroup->drawinfo = malloc(sizeof(lcuint32)*size);
	longs = (lcuint32*)pGroup->drawinfo;

	longs[0] = 2; // colors
	longs[1] = color;
	longs[2] = SIDES*12;
	j = 3;

	// outside
	for (i = 0; i < SIDES; i++)
	{
	  longs[j+i*4] = (lcuint32)(verts + SIDES + i);
	  if (i == SIDES-1)
	  {
	    longs[j+1+i*4] = (lcuint32)verts + SIDES;
	    longs[j+2+i*4] = (lcuint32)verts;
	  }
	  else
	  {
	    longs[j+1+i*4] = (lcuint32)verts + SIDES + i + 1;
	    longs[j+2+i*4] = (lcuint32)verts + i + 1;
	  }
	  longs[j+3+i*4] = (lcuint32)verts + i;
	}
	j += 4*SIDES;

	// inside
	for (i = 0; i < SIDES; i++)
	{
	  longs[j+i*4] = (lcuint32)(verts + 2*SIDES + i);
	  if (i == SIDES-1)
	  {
	    longs[j+1+i*4] = (lcuint32)verts + 2*SIDES;
	    longs[j+2+i*4] = (lcuint32)verts + 3*SIDES;
	  }
	  else
	  {
	    longs[j+1+i*4] = (lcuint32)verts + 2*SIDES + i + 1;
	    longs[j+2+i*4] = (lcuint32)verts + 3*SIDES + i + 1;
	  }
	  longs[j+3+i*4] = (lcuint32)verts + 3*SIDES + i;
	}
	j += 4*SIDES;

	// ring
	for (i = 0; i < SIDES; i++)
	{
	  longs[j+i*4] = (lcuint32)(verts + i);
	  if (i == SIDES-1)
	  {
	    longs[j+1+i*4] = (lcuint32)verts;
	    longs[j+2+i*4] = (lcuint32)verts + 3*SIDES;
	  }
	  else
	  {
	    longs[j+1+i*4] = (lcuint32)verts + i + 1;
	    longs[j+2+i*4] = (lcuint32)verts + 3*SIDES + i + 1;
	  }
	  longs[j+3+i*4] = (lcuint32)verts + 3*SIDES + i;
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
	  longs[j+i*4] = (lcuint32)verts + i;
	  if (i == SIDES-1)
	    longs[1+j+i*4] = (lcuint32)verts;
	  else
	    longs[1+j+i*4] = (lcuint32)verts + i + 1;

	  longs[2+j+i*4] = longs[j+i*4] + SIDES;
	  longs[3+j+i*4] = longs[1+j+i*4] + SIDES;
	}
	j += 4*SIDES;

	// inside
	for (i = 0; i < SIDES; i++)
	{
	  longs[j+i*4] = (lcuint32)verts + 2*SIDES + i;
	  if (i == SIDES-1)
	    longs[1+j+i*4] = (lcuint32)verts + 2*SIDES;
	  else
	    longs[1+j+i*4] = (lcuint32)verts + 2*SIDES + i + 1;

	  longs[2+j+i*4] = longs[j+i*4] + SIDES;
	  longs[3+j+i*4] = longs[1+j+i*4] + SIDES;
	}
      }
      else
      {
	pGroup->drawinfo = malloc(sizeof(lcuint16)*size);
	ushorts = (lcuint16*)pGroup->drawinfo;

	ushorts[0] = 2; // colors
	ushorts[1] = color;
	ushorts[2] = SIDES*12;
	j = 3;

	// outside
	for (i = 0; i < SIDES; i++)
	{
	  ushorts[j+i*4] = (lcuint16)(verts + SIDES + i);
	  if (i == SIDES-1)
	  {
	    ushorts[j+1+i*4] = (lcuint16)verts + SIDES;
	    ushorts[j+2+i*4] = (lcuint16)verts;
	  }
	  else
	  {
	    ushorts[j+1+i*4] = (lcuint16)verts + SIDES + i + 1;
	    ushorts[j+2+i*4] = (lcuint16)verts + i + 1;
	  }
	  ushorts[j+3+i*4] = (lcuint16)verts + i;
	}
	j += 4*SIDES;

	// inside
	for (i = 0; i < SIDES; i++)
	{
	  ushorts[j+i*4] = (lcuint16)(verts + 3*SIDES + i);
	  if (i == SIDES-1)
	  {
	    ushorts[j+1+i*4] = (lcuint16)verts + 3*SIDES;
	    ushorts[j+2+i*4] = (lcuint16)verts + 2*SIDES;
	  }
	  else
	  {
	    ushorts[j+1+i*4] = (lcuint16)verts + 3*SIDES + i + 1;
	    ushorts[j+2+i*4] = (lcuint16)verts + 2*SIDES + i + 1;
	  }
	  ushorts[j+3+i*4] = (lcuint16)verts + 2*SIDES + i;
	}
	j += 4*SIDES;

	// ring
	for (i = 0; i < SIDES; i++)
	{
	  ushorts[j+i*4] = (lcuint16)(verts + i);
	  if (i == SIDES-1)
	  {
	    ushorts[j+1+i*4] = (lcuint16)verts;
	    ushorts[j+2+i*4] = (lcuint16)verts + 3*SIDES;
	  }
	  else
	  {
	    ushorts[j+1+i*4] = (lcuint16)verts + i + 1;
	    ushorts[j+2+i*4] = (lcuint16)verts + 3*SIDES + i + 1;
	  }
	  ushorts[j+3+i*4] = (lcuint16)verts + 3*SIDES + i;
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
	  ushorts[j+i*4] = (lcuint16)verts + i;
	  if (i == SIDES-1)
	    ushorts[1+j+i*4] = (lcuint16)verts;
	  else
	    ushorts[1+j+i*4] = (lcuint16)verts + i + 1;

	  ushorts[2+j+i*4] = ushorts[j+i*4] + SIDES;
	  ushorts[3+j+i*4] = ushorts[1+j+i*4] + SIDES;
	}
	j += 4*SIDES;

	// inside
	for (i = 0; i < SIDES; i++)
	{
	  ushorts[j+i*4] = (lcuint16)verts + 2*SIDES + i;
	  if (i == SIDES-1)
	    ushorts[1+j+i*4] = (lcuint16)verts + 2*SIDES;
	  else
	    ushorts[1+j+i*4] = (lcuint16)verts + 2*SIDES + i + 1;

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
      Matrix mat;

      for (i = 0; i < 12; i++)
        ((float*)(bytes+2))[i] = LCFLOAT (((float*)(bytes+2))[i]);
      mat.FromPacked ((float*)(bytes+2));
      lcuint16 color = ConvertColor(*(bytes+1));

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
	pGroup->drawinfo = malloc(sizeof(lcuint32)*size);
	longs = (lcuint32*)pGroup->drawinfo;

	longs[0] = 2; // colors
	longs[1] = color;
	longs[2] = SIDES*4;
	j = 3;

	for (i = 0; i < SIDES; i++)
	{
	  longs[3+i*4] = (lcuint32)verts + SIDES + i;
	  if (i == SIDES-1)
	  {
	    longs[4+i*4] = (lcuint32)verts + SIDES;
	    longs[5+i*4] = (lcuint32)verts;
	  }
	  else
	  {
	    longs[4+i*4] = (lcuint32)verts + SIDES + i + 1;
	    longs[5+i*4] = (lcuint32)verts + i + 1;
	  }
	  longs[6+i*4] = (lcuint32)verts + i;
	}
	j += 4*SIDES;
	longs[j] = SIDES*3;
	j++;

	for (i = 0; i < SIDES; i++)
	{
	  if (i == SIDES-1)
	    longs[j+i*3] = (lcuint16)verts + SIDES;
	  else
	    longs[j+i*3] = (lcuint16)verts + SIDES + i + 1;
	  longs[1+j+i*3] = (lcuint16)verts + SIDES + i;
	  longs[2+j+i*3] = (lcuint16)verts + 2*SIDES;
	}

	j += 3*SIDES;
	longs[j] =  0; j++; // lines
	longs[j] =  LC_COL_EDGES; j++; // color
	longs[j] =  0; j++; // quads
	longs[j] =  0; j++; // tris
	longs[j] = 4*SIDES; j++;

	for (i = 0; i < SIDES; i++)
	{
	  longs[j+i*4] = (lcuint32)verts + i;
	  if (i == SIDES-1)
	    longs[1+j+i*4] = (lcuint32)verts;
	  else
	    longs[1+j+i*4] = (lcuint32)verts + i + 1;

	  longs[2+j+i*4] = longs[j+i*4] + SIDES;
	  longs[3+j+i*4] = longs[1+j+i*4] + SIDES;
	}
      }
      else
      {
	pGroup->drawinfo = malloc(sizeof(lcuint16)*size);
	ushorts = (lcuint16*)pGroup->drawinfo;

	ushorts[0] = 2; // colors
	ushorts[1] = color;
	ushorts[2] = SIDES*4;
	j = 3;

	for (i = 0; i < SIDES; i++)
	{
	  ushorts[3+i*4] = (lcuint16)(verts + SIDES + i);
	  if (i == SIDES-1)
	  {
	    ushorts[4+i*4] = (lcuint16)verts + SIDES;
	    ushorts[5+i*4] = (lcuint16)verts;
	  }
	  else
	  {
	    ushorts[4+i*4] = (lcuint16)verts + SIDES + i + 1;
	    ushorts[5+i*4] = (lcuint16)verts + i + 1;
	  }
	  ushorts[6+i*4] = (lcuint16)verts + i;
	}
	j += 4*SIDES;
	ushorts[j] = SIDES*3;
	j++;

	for (i = 0; i < SIDES; i++)
	{
	  if (i == SIDES-1)
	    ushorts[j+i*3] = (lcuint16)verts + SIDES;
	  else
	    ushorts[j+i*3] = (lcuint16)verts + SIDES + i + 1;
	  ushorts[1+j+i*3] = (lcuint16)verts + SIDES + i;
	  ushorts[2+j+i*3] = (lcuint16)verts + 2*SIDES;
	}

	j += 3*SIDES;
	ushorts[j] =  0; j++; // lines
	ushorts[j] =  LC_COL_EDGES; j++; // color
	ushorts[j] =  0; j++; // quads
	ushorts[j] =  0; j++; // tris
	ushorts[j] = 4*SIDES; j++;

	for (i = 0; i < SIDES; i++)
	{
	  ushorts[j+i*4] = (lcuint16)verts + i;
	  if (i == SIDES-1)
	    ushorts[1+j+i*4] = (lcuint16)verts;
	  else
	    ushorts[1+j+i*4] = (lcuint16)verts + i + 1;

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
      Matrix mat;

      for (i = 0; i < 12; i++)
        ((float*)(bytes+2))[i] = LCFLOAT (((float*)(bytes+2))[i]);
      mat.FromPacked ((float*)(bytes+2));
      lcuint16 color = ConvertColor(*(bytes+1));

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
	pGroup->drawinfo = malloc(sizeof(lcuint32)*size);
	longs = (lcuint32*)pGroup->drawinfo;

	longs[0] = 2; // colors
	longs[1] = color;
	longs[2] = SIDES*12;
	j = 3;

	// outside
	for (i = 0; i < SIDES; i++)
	{
	  longs[j+i*4] = (lcuint32)(verts + i);
	  if (i == SIDES-1)
	  {
	    longs[j+1+i*4] = (lcuint32)verts;
	    longs[j+2+i*4] = (lcuint32)verts + SIDES;
	  }
	  else
	  {
	    longs[j+1+i*4] = (lcuint32)verts + i + 1;
	    longs[j+2+i*4] = (lcuint32)verts + SIDES + i + 1;
	  }
	  longs[j+3+i*4] = (lcuint32)verts + SIDES + i;
	}
	j += 4*SIDES;

	// inside
	for (i = 0; i < SIDES; i++)
	{
	  longs[j+i*4] = (lcuint32)(verts + 3*SIDES + i);
	  if (i == SIDES-1)
	  {
	    longs[j+1+i*4] = (lcuint32)verts + 3*SIDES;
	    longs[j+2+i*4] = (lcuint32)verts + 2*SIDES;
	  }
	  else
	  {
	    longs[j+1+i*4] = (lcuint32)verts + 3*SIDES + i + 1;
	    longs[j+2+i*4] = (lcuint32)verts + 2*SIDES + i + 1;
	  }
	  longs[j+3+i*4] = (lcuint32)verts + 2*SIDES + i;
	}
	j += 4*SIDES;

	// ring
	for (i = 0; i < SIDES; i++)
	{
	  longs[j+i*4] = (lcuint32)(verts + 3*SIDES + i);
	  if (i == SIDES-1)
	  {
	    longs[j+1+i*4] = (lcuint32)verts + 3*SIDES;
	    longs[j+2+i*4] = (lcuint32)verts;
	  }
	  else
	  {
	    longs[j+1+i*4] = (lcuint32)verts + 3*SIDES + i + 1;
	    longs[j+2+i*4] = (lcuint32)verts + i + 1;
	  }
	  longs[j+3+i*4] = (lcuint32)verts + i;
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
	  longs[j+i*4] = (lcuint32)verts + i;
	  if (i == SIDES-1)
	    longs[1+j+i*4] = (lcuint32)verts;
	  else
	    longs[1+j+i*4] = (lcuint32)verts + i + 1;

	  longs[2+j+i*4] = longs[j+i*4] + SIDES;
	  longs[3+j+i*4] = longs[1+j+i*4] + SIDES;
	}
	j += 4*SIDES;

	// inside
	for (i = 0; i < SIDES; i++)
	{
	  longs[j+i*4] = (lcuint32)verts + 2*SIDES + i;
	  if (i == SIDES-1)
	    longs[1+j+i*4] = (lcuint32)verts + 2*SIDES;
	  else
	    longs[1+j+i*4] = (lcuint32)verts + 2*SIDES + i + 1;

	  longs[2+j+i*4] = longs[j+i*4] + SIDES;
	  longs[3+j+i*4] = longs[1+j+i*4] + SIDES;
	}
      }
      else
      {
	pGroup->drawinfo = malloc(sizeof(lcuint16)*size);
	ushorts = (lcuint16*)pGroup->drawinfo;

	ushorts[0] = 2; // colors
	ushorts[1] = color;
	ushorts[2] = SIDES*12;
	j = 3;

	// outside
	for (i = 0; i < SIDES; i++)
	{
	  ushorts[j+i*4] = (lcuint16)(verts + i);
	  if (i == SIDES-1)
	  {
	    ushorts[j+1+i*4] = (lcuint16)verts;
	    ushorts[j+2+i*4] = (lcuint16)verts + SIDES;
	  }
	  else
	  {
	    ushorts[j+1+i*4] = (lcuint16)verts + i + 1;
	    ushorts[j+2+i*4] = (lcuint16)verts + SIDES + i + 1;
	  }
	  ushorts[j+3+i*4] = (lcuint16)verts + SIDES + i;
	}
	j += 4*SIDES;

	// inside
	for (i = 0; i < SIDES; i++)
	{
	  ushorts[j+i*4] = (lcuint16)(verts + 2*SIDES + i);
	  if (i == SIDES-1)
	  {
	    ushorts[j+1+i*4] = (lcuint16)verts + 2*SIDES;
	    ushorts[j+2+i*4] = (lcuint16)verts + 3*SIDES;
	  }
	  else
	  {
	    ushorts[j+1+i*4] = (lcuint16)verts + 2*SIDES + i + 1;
	    ushorts[j+2+i*4] = (lcuint16)verts + 3*SIDES + i + 1;
	  }
	  ushorts[j+3+i*4] = (lcuint16)verts + 3*SIDES + i;
	}
	j += 4*SIDES;

	// ring
	for (i = 0; i < SIDES; i++)
	{
	  ushorts[j+i*4] = (lcuint16)(verts + 3*SIDES + i);
	  if (i == SIDES-1)
	  {
	    ushorts[j+1+i*4] = (lcuint16)verts + 3*SIDES;
	    ushorts[j+2+i*4] = (lcuint16)verts;
	  }
	  else
	  {
	    ushorts[j+1+i*4] = (lcuint16)verts + 3*SIDES + i + 1;
	    ushorts[j+2+i*4] = (lcuint16)verts + i + 1;
	  }
	  ushorts[j+3+i*4] = (lcuint16)verts + i;
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
	  ushorts[j+i*4] = (lcuint16)verts + i;
	  if (i == SIDES-1)
	    ushorts[1+j+i*4] = (lcuint16)verts;
	  else
	    ushorts[1+j+i*4] = (lcuint16)verts + i + 1;

	  ushorts[2+j+i*4] = ushorts[j+i*4] + SIDES;
	  ushorts[3+j+i*4] = ushorts[1+j+i*4] + SIDES;
	}
	j += 4*SIDES;

	// inside
	for (i = 0; i < SIDES; i++)
	{
	  ushorts[j+i*4] = (lcuint16)verts + 2*SIDES + i;
	  if (i == SIDES-1)
	    ushorts[1+j+i*4] = (lcuint16)verts + 2*SIDES;
	  else
	    ushorts[1+j+i*4] = (lcuint16)verts + 2*SIDES + i + 1;

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

	if (m_nFlags & LC_PIECE_LONGDATA_RUNTIME)
	{
		m_nFlags &= ~(LC_PIECE_LONGDATA | LC_PIECE_LONGDATA_RUNTIME);
	}
}

// Zoom extents for the preview window and print catalog
void PieceInfo::ZoomExtents(float Fov, float Aspect, float* EyePos) const
{
	float Eye[3] = { -100.0f, -100.0f, 50.0f };

	if (EyePos)
	{
		Eye[0] = EyePos[0];
		Eye[1] = EyePos[1];
		Eye[2] = EyePos[2];
	}

	// Get perspective information.
	float Alpha = Fov / 2.0f;
	float HalfFovY = Fov / 2.0f;
	HalfFovY = HalfFovY * 3.1415f / 180.0f;
	float HalfFovX = (float)atan(tan(HalfFovY) * Aspect);
	HalfFovX = HalfFovX * 180.0f / 3.1415f;
	float Beta = HalfFovX;

	// Get vectors from the position.
	float NonOrthoTop[3] = { 0.0f, 0.0f, 1.0f };
	float Target[3] = { (m_fDimensions[0] + m_fDimensions[3])*0.5f, (m_fDimensions[1] + m_fDimensions[4])*0.5f,
	                    (m_fDimensions[2] + m_fDimensions[5])*0.5f };
	float Front[3] = { Target[0] - Eye[0], Target[1] - Eye[1], Target[2] - Eye[2]};
	float Side[3];
	Side[0] = NonOrthoTop[1]*Front[2] - NonOrthoTop[2]*Front[1];
	Side[1] = NonOrthoTop[2]*Front[0] - NonOrthoTop[0]*Front[2];
	Side[2] = NonOrthoTop[0]*Front[1] - NonOrthoTop[1]*Front[0];
	
	// Make sure the up vector is orthogonal.
	float Top[3];
	Top[0] = Front[1]*Side[2] - Front[2]*Side[1];
	Top[1] = Front[2]*Side[0] - Front[0]*Side[2];
	Top[2] = Front[0]*Side[1] - Front[1]*Side[0];
	
	// Calculate the plane normals.
	Matrix Mat;
	float TopNormal[3] = { -Top[0], -Top[1], -Top[2] };
	Mat.FromAxisAngle(Side, -Alpha);
	Mat.TransformPoints(TopNormal, 1);

	float BottomNormal[3] = { Top[0], Top[1], Top[2] };
	Mat.FromAxisAngle(Side, Alpha);
	Mat.TransformPoints(BottomNormal, 1);

	float RightNormal[3] = { Side[0], Side[1], Side[2] };
	Mat.FromAxisAngle(Top, -Beta);
	Mat.TransformPoints(RightNormal, 1);

	float LeftNormal[3] = { -Side[0], -Side[1], -Side[2] };
	Mat.FromAxisAngle(Top, Beta);
	Mat.TransformPoints(LeftNormal, 1);

	// Calculate the plane offsets from the normals and the eye position.
	float TopD = Eye[0]*-TopNormal[0] + Eye[1]*-TopNormal[1] + Eye[2]*-TopNormal[2];
	float BottomD = Eye[0]*-BottomNormal[0] + Eye[1]*-BottomNormal[1] + Eye[2]*-BottomNormal[2];
	float LeftD = Eye[0]*-LeftNormal[0] + Eye[1]*-LeftNormal[1] + Eye[2]*-LeftNormal[2];
	float RightD = Eye[0]*-RightNormal[0] + Eye[1]*-RightNormal[1] + Eye[2]*-RightNormal[2];
	
	// Now generate the planes
	float Inv;
	Inv = 1.0f/(float)sqrt(TopNormal[0]*TopNormal[0]+TopNormal[1]*TopNormal[1]+TopNormal[2]*TopNormal[2]);
	float TopPlane[4] = { TopNormal[0]*Inv, TopNormal[1]*Inv, TopNormal[2]*Inv, TopD*Inv };
	Inv = 1.0f/(float)sqrt(BottomNormal[0]*BottomNormal[0]+BottomNormal[1]*BottomNormal[1]+BottomNormal[2]*BottomNormal[2]);
	float BottomPlane[4] = { BottomNormal[0]*Inv, BottomNormal[1]*Inv, BottomNormal[2]*Inv, BottomD*Inv };
	Inv = 1.0f/(float)sqrt(LeftNormal[0]*LeftNormal[0]+LeftNormal[1]*LeftNormal[1]+LeftNormal[2]*LeftNormal[2]);
	float LeftPlane[4] = { LeftNormal[0]*Inv, LeftNormal[1]*Inv, LeftNormal[2]*Inv, LeftD*Inv };
	Inv = 1.0f/(float)sqrt(RightNormal[0]*RightNormal[0]+RightNormal[1]*RightNormal[1]+RightNormal[2]*RightNormal[2]);
	float RightPlane[4] = { RightNormal[0]*Inv, RightNormal[1]*Inv, RightNormal[2]*Inv, RightD*Inv };

	float Verts[8][3] = {
		{ m_fDimensions[0], m_fDimensions[1], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[1], m_fDimensions[5] },
		{ m_fDimensions[0], m_fDimensions[1], m_fDimensions[2] },
		{ m_fDimensions[3], m_fDimensions[4], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[4], m_fDimensions[2] },
		{ m_fDimensions[0], m_fDimensions[4], m_fDimensions[2] },
		{ m_fDimensions[0], m_fDimensions[4], m_fDimensions[5] },
		{ m_fDimensions[3], m_fDimensions[1], m_fDimensions[2] } };

	float SmallestU = 10000.0f;

	for (int i = 0; i < 4; i++)
	{
		float* Plane;

		switch (i)
		{
		case 0: Plane = TopPlane; break;
		case 1: Plane = BottomPlane; break;
		case 2: Plane = LeftPlane; break;
		case 3: Plane = RightPlane; break;
		}

		for (int j = 0; j < 8; j++)
		{
			Plane[3] = Verts[j][0]*-Plane[0] + Verts[j][1]*-Plane[1] + Verts[j][2]*-Plane[2];

			// Intersect the eye line with the plane, NewEye = Eye + u * (Target - Eye)
			float u = Eye[0] * Plane[0] + Eye[1] * Plane[1] + Eye[2] * Plane[2] + Plane[3];
			u /= Front[0] * -Plane[0] + Front[1] * -Plane[1] + Front[2] * -Plane[2];

			if (u < SmallestU)
				SmallestU = u;
		}
	}

	float NewEye[3];
	NewEye[0] = Eye[0] + Front[0] * SmallestU;
	NewEye[1] = Eye[1] + Front[1] * SmallestU;
	NewEye[2] = Eye[2] + Front[2] * SmallestU;

	if (EyePos)
	{
		EyePos[0] = NewEye[0];
		EyePos[1] = NewEye[1];
		EyePos[2] = NewEye[2];
	}

	Vector FrontVec, RightVec, UpVec;

	// Calculate view matrix.
	UpVec = Vector(Top[0], Top[1], Top[2]);
	UpVec.Normalize();
	FrontVec = Vector(Front[0], Front[1], Front[2]);
	FrontVec.Normalize();
	RightVec = Vector(Side[0], Side[1], Side[2]);
	RightVec.Normalize();

  float ViewMat[16];
  ViewMat[0] = -RightVec[0]; ViewMat[4] = -RightVec[1]; ViewMat[8]  = -RightVec[2]; ViewMat[12] = 0.0;
  ViewMat[1] = UpVec[0];     ViewMat[5] = UpVec[1];     ViewMat[9]  = UpVec[2];     ViewMat[13] = 0.0;
  ViewMat[2] = -FrontVec[0]; ViewMat[6] = -FrontVec[1]; ViewMat[10] = -FrontVec[2]; ViewMat[14] = 0.0;
  ViewMat[3] = 0.0;          ViewMat[7] = 0.0;          ViewMat[11] = 0.0;          ViewMat[15] = 1.0;

  // Load ViewMatrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMultMatrixf(ViewMat);
  glTranslatef(-NewEye[0], -NewEye[1], -NewEye[2]);
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
	lcuint16 sh, curcolor;
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
			glEnable (GL_BLEND);
			glDepthMask (GL_FALSE);
		}
		else
		{
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
			lcuint32* info, colors;

			info = (lcuint32*)pGroup->drawinfo;
			colors = *info;
			info++;

			while (colors--)
			{
				if (*info == LC_COL_DEFAULT)
					curcolor = nColor;
				else
					curcolor = (unsigned short)*info;
				info++;

				if (curcolor > 13 && curcolor < 22)
				{
					glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					glEnable (GL_BLEND);
					glDepthMask (GL_FALSE);
					glColor4ubv (ColorArray[curcolor]);
				}
				else
				{
					glDepthMask (GL_TRUE);
					glDisable (GL_BLEND);
					glColor3ubv (FlatColorArray[curcolor]);
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
			lcuint16* info, colors;

			info = (lcuint16*)pGroup->drawinfo;
			colors = *info;
			info++;

			while (colors--)
			{
				if (*info == LC_COL_DEFAULT)
					curcolor = nColor;
				else
					curcolor = *info;
				info++;

				if (curcolor > 13 && curcolor < 22)
				{
					glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					glEnable (GL_BLEND);
					glDepthMask (GL_FALSE);
					glColor4ubv (ColorArray[curcolor]);
				}
				else
				{
					glDepthMask (GL_TRUE);
					glDisable (GL_BLEND);
					glColor3ubv(FlatColorArray[curcolor]);
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
	// if glDepthMask is GL_FALSE then glClearBuffer (GL_DEPTH_BUFFER_BIT) doesn't work
	glDepthMask (GL_TRUE);
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
