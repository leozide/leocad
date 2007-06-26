#ifndef _LC_MODELREF_H_
#define _LC_MODELREF_H_

#include "lc_pieceobj.h"

class lcModel;

class lcModelRef : public lcPieceObject
{
public:
	lcModelRef(lcModel* Model);
	lcModelRef(lcModelRef* ModelRef);
	virtual ~lcModelRef();

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
	lcModel* m_Model;
};

#endif // _LC_MODELREF_H_
