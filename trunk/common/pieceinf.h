#ifndef _PIECEINF_H_
#define _PIECEINF_H_

#include <stdio.h>
#ifndef GLuint
#include "opengl.h"
#endif
#include "algebra.h"

#define LC_PIECE_COUNT              0x001 // Count this piece in the totals ?
#define LC_PIECE_LONGDATA_FILE      0x002 // unsigned long/short index
#define LC_PIECE_CCW                0x004 // Use back-face culling
#define LC_PIECE_SMALL              0x010 // scale = 10000
#define LC_PIECE_MEDIUM             0x020 // scale = 1000 (otherwise = 100)
#define LC_PIECE_PLACEHOLDER        0x040 // Placeholder for a piece not in the library.
#define LC_PIECE_HAS_DEFAULT        0x100 // Piece has triangles using the default color
#define LC_PIECE_HAS_SOLID          0x200 // Piece has triangles using a solid color
#define LC_PIECE_HAS_TRANSLUCENT    0x400 // Piece has triangles using a translucent color
#define LC_PIECE_HAS_LINES          0x800 // Piece has lines

#define LC_PIECE_NAME_LEN 256

class PieceInfo
{
 public:
  PieceInfo ();
  ~PieceInfo ();

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

	lcVector3 GetCenter() const
	{
		return lcVector3((m_fDimensions[0] + m_fDimensions[3]) * 0.5f,
		                 (m_fDimensions[1] + m_fDimensions[4]) * 0.5f,
		                 (m_fDimensions[2] + m_fDimensions[5]) * 0.5f);
	}

	// Operations
	void ZoomExtents(float Fov, float Aspect, float* EyePos = NULL) const;
	void RenderOnce(int nColor);
	void RenderPiece(int nColor);

	// Implementation
	GLuint GetBoxDisplayList()
	{
		if (!m_nBoxList)
			CreateBoxDisplayList();
		return m_nBoxList;
	};
	void LoadIndex(lcFile& file);
	void CreatePlaceholder(const char* Name);
	void AddRef();
	void DeRef();

	template<typename DstType>
	void BuildMesh(void* Data, int* SectionIndices);

public:
	lcMesh* mMesh;

	// Attributes
	char m_strName[LC_PIECE_NAME_LEN];
	char m_strDescription[65];
	float m_fDimensions[6];
	lcuint32 m_nOffset;
	lcuint32 m_nSize;
	lcuint32 m_nFlags;

protected:
	int m_nRef;
	GLuint m_nBoxList;

	void LoadInformation();
	void FreeInformation();
	void CreateBoxDisplayList();
};

#endif // _PIECEINF_H_
