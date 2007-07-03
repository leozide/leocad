#ifndef _LC_PIECE_H_
#define _LC_PIECE_H_

#include "lc_pieceobj.h"

class PieceInfo;

class lcPiece : public lcPieceObject
{
public:
	lcPiece(PieceInfo* Info);
	lcPiece(const lcPiece* Piece);
	virtual ~lcPiece();

	// Set or change the piece info used by this object.
	void SetPieceInfo(PieceInfo* Info);

	virtual void ExportLDraw(File& file) const;

	// Base class implementation.
	virtual void ClosestRayIntersect(LC_CLICK_RAY* Ray) const;
	virtual bool IntersectsVolume(const Vector4* Planes, int NumPlanes) const;
	virtual void Update(u32 Time);
	virtual void AddToScene(lcScene* Scene, int Color);

protected:
	// Build the mesh used for rendering.
	void BuildMesh();

	template <class T>
	struct TypeToType
	{
		typedef T Type;
	};

	template<class T>
	void BuildMesh(int SectionIndices[LC_COL_DEFAULT+1][3], TypeToType<T>);

public:
	// Piece properties.
	PieceInfo* m_PieceInfo;
};

#endif // _LC_PIECE_H_
