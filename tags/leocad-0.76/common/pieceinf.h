#ifndef _PIECEINF_H_
#define _PIECEINF_H_

#include <stdio.h>
#include "algebra.h"

#define LC_PIECE_COUNT              0x01 // Count this piece in the totals ?
#define LC_PIECE_LONGDATA_FILE      0x02 // unsigned long/short index
#define LC_PIECE_CCW                0x04 // Use back-face culling
#define LC_PIECE_SMALL              0x10 // scale = 10000
#define LC_PIECE_MEDIUM             0x20 // scale = 1000 (otherwise = 100)
#define LC_PIECE_MODEL              0x40 // This is a model instead of a piece from the library.

class File;
class Texture;
class lcMesh;
class lcModel;

struct CONNECTIONINFO
{
	unsigned char type;
	Vector3 center;
	Vector3 normal;
};

struct DRAWGROUP
{
	unsigned short connections[6];
	int NumSections;
};

struct TEXTURE
{
	Texture* texture;
	unsigned char color;
	float vertex[4][3];
	float coords[4][2];
};

unsigned char ConvertColor(int c);

class PieceInfo
{
public:
	PieceInfo();
	~PieceInfo();

	bool IsPatterned() const
	{
		const char* Name = m_strName;

		while (*Name)
		{
			if (*Name < '0' || *Name > '9')
				break;

			Name++;
		}

		if (*Name == 'P')
			return true;

		return false;
	}

	bool IsSubPiece() const
	{
		return (m_strDescription[0] == '~');
	}

	// Operations
	void ZoomExtents(float Fov, float Aspect, float* EyePos = NULL) const;
	void RenderPiece(int nColor);
	void RenderBox();
	void WriteWavefront(FILE* file, unsigned char color, unsigned long* start);

	// Implementation
	void LoadIndex(File& file);
	void CreateFromModel(lcModel* Model);
	void AddRef();
	void DeRef();

	void LoadInformation();
	void FreeInformation();

public:
	// Attributes
	char m_strName[9];
	char m_strDescription[65];
	float m_fDimensions[6];
	u32 m_nOffset;
	u32 m_nSize;

	// Nobody should change these
	u8 m_nFlags;
	u16 m_nConnectionCount;
	CONNECTIONINFO* m_pConnections;
	u16 m_nGroupCount;
	DRAWGROUP* m_pGroups;
	u8 m_nTextureCount;
	TEXTURE* m_pTextures;
	lcMesh* m_Mesh;
	BoundingBox m_BoundingBox;
	lcModel* m_Model;

protected:
	int m_nRef;

	template<typename T>
	void BuildMesh(void* Data, void* MeshStart, u32* SectionIndices);
};

#endif // _PIECEINF_H_
