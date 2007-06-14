#ifndef _LC_FLEXPIECE_H_
#define _LC_FLEXPIECE_H_

#include "lc_pieceobj.h"

class PieceInfo;

class lcFlexiblePiece : public lcPieceObject
{
public:
	lcFlexiblePiece(PieceInfo* Info);
	lcFlexiblePiece(lcFlexiblePiece* Piece);
	virtual ~lcFlexiblePiece();

	// Base class implementation.
	virtual void ClosestRayIntersect(LC_CLICK_RAY* Ray) const;
	virtual bool IntersectsVolume(const Vector4* Planes, int NumPlanes) const;
	virtual void Update(u32 Time);
	virtual void AddToScene(lcScene* Scene, const Matrix44& ParentWorld, int Color);

	// Base class overrides.
	void Move(u32 Time, bool AddKey, const Vector3& Delta);

protected:
	void BuildMesh();

public:
	// Piece properties.
	PieceInfo* m_PieceInfo;
};


// Flexible piece point key types.
enum LC_FLEXPIECE_POINT_KEY_TYPES
{
	LC_FLEXPIECE_POINT_POSITION,
	LC_FLEXPIECE_POINT_NUMKEYS
};

class lcFlexiblePiecePoint : public lcObject
{
public:
	lcFlexiblePiecePoint(lcFlexiblePiece* Parent);
	virtual ~lcFlexiblePiecePoint();

	// Base class implementation.
	virtual void ClosestRayIntersect(LC_CLICK_RAY* Ray) const;
	virtual bool IntersectsVolume(const Vector4* Planes, int NumPlanes) const;
	virtual void Update(u32 Time);
	virtual void AddToScene(lcScene* Scene, const Matrix44& ParentWorld, int Color);
};

#endif // _LC_FLEXPIECE_H_
