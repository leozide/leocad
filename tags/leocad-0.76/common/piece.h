#ifndef _PIECE_H_
#define _PIECE_H_

class File;
class lcPiece;
class lcGroup;
class PieceInfo;
class lcMesh;

#include "object.h"
#include "typedefs.h"
#include "defines.h"

#define LC_PIECE_HIDDEN		0x01
#define LC_PIECE_SELECTED	0x02
#define LC_PIECE_FOCUSED	0x04

typedef enum
{
	LC_PK_POSITION,
	LC_PK_ROTATION,
	LC_PK_COUNT
} LC_PK_TYPES;

class lcPiece : public lcObject
{
public:
	lcPiece(PieceInfo* pPieceInfo);
	~lcPiece();

	// Base class implementation.
	virtual void ClosestLineIntersect(lcClickLine& ClickLine) const;
	virtual bool IntersectsVolume(const Vector4* Planes, int NumPlanes) const;



	void Select(bool bSelecting, bool bFocus, bool bMultiple);
	virtual void InsertTime(u32 Start, u32 Time);
	virtual void RemoveTime(u32 Start, u32 Time);






	void Hide()
		{ m_nState = LC_PIECE_HIDDEN; }
	void UnHide()
		{ m_nState &= ~LC_PIECE_HIDDEN; }
	bool IsHidden()
		{ return (m_nState & LC_PIECE_HIDDEN) != 0; }
	bool IsSelected()
		{ return (m_nState & LC_PIECE_SELECTED) != 0; }
	bool IsFocused()
		{ return (m_nState & LC_PIECE_FOCUSED) != 0; }

	bool IsVisible(u32 Time);
	void Initialize(float x, float y, float z, u32 Time, int Color);
	void MergeBoundingBox(BoundingBox* Box);
	void SetPieceInfo(PieceInfo* pPieceInfo);
	bool FileLoad(File& file, char* name);
	void FileSave(File& file, lcGroup* Groups);

	void UpdatePosition(u32 Time);
	void Move(u32 Time, bool AddKey, const Vector3& Delta);

	void DoGroup(lcGroup* Group);
	void UnGroup(lcGroup* Group);
	lcGroup* GetTopGroup();
	void SetGroup(lcGroup* Group)
		{ m_Group = Group; }
	lcGroup* GetGroup()
		{ return m_Group; }

	void Render(bool bLighting, bool bEdges);
	void RenderBox(bool bHilite, float fLineWidth);

public:
	// Atributes
	PieceInfo* m_PieceInfo;
	lcGroup* m_Group;

	u32 m_TimeShow;
	u32 m_TimeHide;

	u32 m_Color;
	unsigned char m_nState;

	// Temporary variables
	Vector3 m_Position;
	Vector4 m_AxisAngle;
	Matrix44 m_ModelWorld;
};

#endif // _PIECE_H
