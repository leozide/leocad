#ifndef _PIECE_H_
#define _PIECE_H_

class File;
class lcPiece;
class Group;
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





	lcPiece* m_pLink;

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
	void CreateName(lcPiece* pPiece);
	void MergeBoundingBox(BoundingBox* Box);
	void SetPieceInfo(PieceInfo* pPieceInfo);
	bool FileLoad(File& file, char* name);
	void FileSave(File& file, Group* pGroups);

	void UpdatePosition(u32 Time);
	void Move(u32 Time, bool AddKey, const Vector3& Delta);

	void DoGroup(Group* pGroup);
	void UnGroup(Group* pGroup);
	Group* GetTopGroup();
	void SetGroup(Group* pGroup)
		{ m_pGroup = pGroup; }
	Group* GetGroup()
		{ return m_pGroup; }
	const unsigned char GetColor()
		{ return m_nColor; }
	void SetColor(unsigned char color)
		{ m_nColor = color; }
	void SetTimeShow(u32 Time)
		{ m_TimeShow = Time; }
	u32 GetTimeShow() const
		{ return m_TimeShow; }
	void SetTimeHide(u32 Time)
		{ m_TimeHide = Time; }
	u32 GetTimeHide() const
		{ return m_TimeHide; }

	void Render(bool bLighting, bool bEdges);
	void RenderBox(bool bHilite, float fLineWidth);

public:
	// Atributes
	PieceInfo* m_PieceInfo;
	Group* m_pGroup;

	u32 m_TimeShow;
	u32 m_TimeHide;

	int m_nColor;
	unsigned char m_nState;

	// Temporary variables
	Vector3 m_Position;
	Vector4 m_AxisAngle;
	Matrix44 m_ModelWorld;
};

#endif // _PIECE_H
