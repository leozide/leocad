#ifndef _LC_PIECEOBJ_H_
#define _LC_PIECEOBJ_H_

#include "lc_object.h"

class lcMesh;
class PieceInfo;

enum LC_PIECEOBJ_KEY_TYPES
{
	LC_PIECEOBJ_POSITION,
	LC_PIECEOBJ_ROTATION,
	LC_PIECEOBJ_NUMKEYS
};

class lcPieceObject : public lcObject
{
public:
	lcPieceObject(u32 Type);
	virtual ~lcPieceObject();

	// Change this object's orientation.
	void SetRotation(u32 Time, bool AddKey, const Vector4& NewRotation);

	// Merges this object's bounding box with another box.
	void MergeBoundingBox(BoundingBox* Box);

	// Export this object to an LDraw file.
	virtual void ExportLDraw(class File& file)
	{
		// FIXME: LDraw export
	}

	// Base class overrides.
	virtual bool IsVisible(u32 Time) const
	{
		if ((Time < m_TimeShow) || (Time >= m_TimeHide && m_TimeHide != LC_MAX_TIME))
			return false;

		return lcObject::IsVisible(Time);
	}

	virtual void Update(u32 Time);
	virtual void InsertTime(u32 Time, u32 Frames);
	virtual void RemoveTime(u32 Time, u32 Frames);

public:
	// Piece Object properties.
	u8 m_Color;
	u32 m_TimeShow;
	u32 m_TimeHide;
	lcMesh* m_Mesh;

	// Temporary values.
	Matrix44 m_ModelWorld;
	Vector4 m_AxisAngle;
	BoundingBox m_BoundingBox;
};

#endif // _LC_PIECEOBJ_H_
