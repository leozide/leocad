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
	virtual void InsertTime(unsigned short start, bool animation, unsigned short time);
	virtual void RemoveTime(unsigned short start, bool animation, unsigned short time);





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

	bool IsVisible(unsigned short nTime, bool bAnimation);
	void Initialize(float x, float y, float z, unsigned char nStep, unsigned short nFrame, unsigned char nColor);
	void CreateName(lcPiece* pPiece);
	void CompareBoundingBox(float box[6]);
	void SetPieceInfo(PieceInfo* pPieceInfo);
	bool FileLoad(File& file, char* name);
	void FileSave(File& file, Group* pGroups);

	void UpdatePosition(unsigned short nTime, bool bAnimation);
	void Move(unsigned short nTime, bool bAnimation, bool bAddKey, float dx, float dy, float dz);

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
	PieceInfo* GetPieceInfo()
		{ return m_pPieceInfo; }
	void SetStepShow(unsigned char step)
		{ m_nStepShow = step; }
	const unsigned char GetStepShow()
		{ return m_nStepShow; }
	void SetStepHide(unsigned char step)
		{ m_nStepHide = step; }
	const unsigned char GetStepHide()
		{ return m_nStepHide; }
	void SetFrameShow(unsigned short frame)
		{ m_nFrameShow = frame; }
	const unsigned short GetFrameShow()
		{ return m_nFrameShow; }
	void SetFrameHide(unsigned short frame)
		{ m_nFrameHide = frame; }
	const unsigned short GetFrameHide()
		{ return m_nFrameHide; }
	const float* GetConstPosition()
		{ return m_fPosition; }
	inline Vector3 GetPosition() const
		{ return Vector3(m_fPosition[0], m_fPosition[1], m_fPosition[2]); }
	void GetPosition (float* position)
		{ memcpy(position, m_fPosition, sizeof(m_fPosition)); }
	void GetRotation (float* rotation)
		{ memcpy(rotation, m_fRotation, sizeof(m_fRotation)); }

	void Render(bool bLighting, bool bEdges);
	void RenderBox(bool bHilite, float fLineWidth);

	// Temporary variables
	float m_fPosition[3];
	float m_fRotation[4];

protected:
	// Atributes
	PieceInfo* m_pPieceInfo;
	Group* m_pGroup;

	unsigned short m_nFrameShow;
	unsigned short m_nFrameHide;
	unsigned char m_nStepShow;
	unsigned char m_nStepHide;

	unsigned char m_nColor;
	unsigned char m_nState;
};


#endif // _PIECE_H
