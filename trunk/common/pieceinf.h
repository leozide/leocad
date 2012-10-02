#ifndef _PIECEINF_H_
#define _PIECEINF_H_

#include <stdio.h>
#ifndef GLuint
#include "opengl.h"
#endif
#include "lc_math.h"

#define LC_PIECE_HAS_DEFAULT        0x01 // Piece has triangles using the default color
#define LC_PIECE_HAS_SOLID          0x02 // Piece has triangles using a solid color
#define LC_PIECE_HAS_TRANSLUCENT    0x04 // Piece has triangles using a translucent color
#define LC_PIECE_HAS_LINES          0x08 // Piece has lines
#define LC_PIECE_PLACEHOLDER        0x10 // Placeholder for a piece not in the library.

#define LC_PIECE_NAME_LEN 256

class PieceInfo
{
public:
	PieceInfo(int ZipFileIndex);
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

	lcVector3 GetCenter() const
	{
		return lcVector3((m_fDimensions[0] + m_fDimensions[3]) * 0.5f,
		                 (m_fDimensions[1] + m_fDimensions[4]) * 0.5f,
		                 (m_fDimensions[2] + m_fDimensions[5]) * 0.5f);
	}

	// Operations
	void ZoomExtents(float Fov, float Aspect, float* EyePos = NULL) const;
	void RenderPiece(int nColor);

	// Implementation
	GLuint GetBoxDisplayList()
	{
		if (!m_nBoxList)
			CreateBoxDisplayList();
		return m_nBoxList;
	};
	void CreatePlaceholder(const char* Name);
	void AddRef();
	void DeRef();

public:
	lcMesh* mMesh;

	// Attributes
	char m_strName[LC_PIECE_NAME_LEN];
	char m_strDescription[128];
	float m_fDimensions[6];
	lcuint32 mZipFileIndex;
	lcuint32 mFlags;

protected:
	int m_nRef;
	GLuint m_nBoxList;

	void LoadInformation();
	void FreeInformation();
	void CreateBoxDisplayList();
};

#endif // _PIECEINF_H_
