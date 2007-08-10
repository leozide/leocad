#ifndef _LC_PIVOT_H_
#define _LC_PIVOT_H_

#include "lc_pieceobj.h"

class lcPivot : public lcPieceObject
{
public:
	lcPivot();
	virtual ~lcPivot();

	// Base class implementation.
	virtual void GetPieceList(ObjArray<struct LC_PIECELIST_ENTRY>& Pieces, int Color) const;
	virtual void ClosestRayIntersect(LC_CLICK_RAY* Ray) const;
	virtual bool IntersectsVolume(const Vector4* Planes, int NumPlanes) const;
	virtual void Update(u32 Time);
	virtual void AddToScene(lcScene* Scene, int Color);

	// Base class overrides.
	void Move(u32 Time, bool AddKey, const Vector3& Delta);
};

#endif // _LC_PIVOT_H_
