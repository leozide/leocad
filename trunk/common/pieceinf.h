//
//	pieceinf.h
////////////////////////////////////////////////////

#ifndef _PIECEINF_H_
#define _PIECEINF_H_

#include <stdio.h>
#ifndef GLuint
#include "opengl.h"
#endif

#define LC_PIECE_COUNT		0x01 // Count this piece in the totals ?
#define LC_PIECE_LONGDATA	0x02 // unsigned long/short index
#define LC_PIECE_CCW		0x04 // Use back-face culling
#define LC_PIECE_SMALL		0x10 // scale = 10000
#define LC_PIECE_MEDIUM		0x20 // scale = 1000 (otherwise = 100)

class File;
class Texture;

typedef struct
{
	unsigned char type;
	float center[3];
	float normal[3];
} CONNECTIONINFO;

typedef struct
{
	unsigned short connections[6];
	void* drawinfo;
} DRAWGROUP;

typedef struct TEXTURE
{
	Texture* texture;
	unsigned char color;
	float vertex[4][3];
	float coords[4][2];
} TEXTURE;

unsigned char ConvertColor(int c);

class PieceInfo
{
 public:
  PieceInfo ();
  ~PieceInfo ();

	// Operations
	void ZoomExtents();
	void RenderOnce(int nColor);
	void RenderPiece(int nColor);
	void WriteWavefront(FILE* file, unsigned char color, unsigned long* start);

	// Implementation
	void LoadIndex (File& file);
	GLuint AddRef();
	void DeRef();

public:
	// Attributes
	char m_strName[9];
	char m_strDescription[65];
	float m_fDimensions[6];
	unsigned long m_nOffset;
	unsigned long m_nSize;
	unsigned long m_nGroups;

	// Nobody should change these
	unsigned char	m_nFlags;
	unsigned long 	m_nVertexCount;
	float*			m_fVertexArray;
	unsigned short	m_nConnectionCount;
	CONNECTIONINFO*	m_pConnections;
	unsigned short	m_nGroupCount;
	DRAWGROUP*		m_pGroups;
	unsigned char	m_nTextureCount;
	TEXTURE*		m_pTextures;

protected:
	int m_nRef;
	GLuint m_nBoxList;

	void LoadInformation();
	void FreeInformation();
/*
	CRModel*	m_pRModel;
*/
};

#endif // _PIECEINF_H_
