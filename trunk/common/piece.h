#ifndef _PIECE_H_
#define _PIECE_H_

class Piece;
class Group;
class PieceInfo;

#include "object.h"
#include "lc_colors.h"
#include "lc_math.h"

#define LC_PIECE_HIDDEN		0x01
#define LC_PIECE_SELECTED	0x02
#define LC_PIECE_FOCUSED	0x04

enum LC_PK_TYPES
{
	LC_PK_POSITION,
	LC_PK_ROTATION,
	LC_PK_COUNT
};

class Piece : public Object
{
public:
	Piece (PieceInfo* pPieceInfo);
	~Piece ();

	void Select(bool bSelecting, bool bFocus, bool bMultiple);
	virtual void InsertTime(unsigned short start, unsigned short time);
	virtual void RemoveTime(unsigned short start, unsigned short time);
	virtual bool IntersectsVolume(const lcVector4 Planes[6]) const;





	Piece* m_pNext;

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

	const char* GetName() const
	{ return m_strName; }

	virtual void MinIntersectDist(lcClickLine* ClickLine);
	bool IsVisible(unsigned short nTime);
	void Initialize(float x, float y, float z, unsigned char nStep);
	void CreateName(Piece* pPiece);
	void CompareBoundingBox(float box[6]);
	void SetPieceInfo(PieceInfo* pPieceInfo);
	bool FileLoad(lcFile& file);
	void FileSave(lcFile& file) const;

	void UpdatePosition(unsigned short nTime);
	void Move(unsigned short nTime, bool bAddKey, float dx, float dy, float dz);

	void DoGroup(Group* pGroup);
	void UnGroup(Group* pGroup);
	Group* GetTopGroup();
	void SetGroup(Group* pGroup)
		{ m_pGroup = pGroup; }
	Group* GetGroup()
		{ return m_pGroup; }
	void SetName(char* name)
		{ strcpy(m_strName, name); }
	const char* GetName()
		{ return m_strName; }
	void SetStepShow(unsigned char step)
		{ m_nStepShow = step; }
	unsigned char GetStepShow()
		{ return m_nStepShow; }
	void SetStepHide(unsigned char step)
		{ m_nStepHide = step; }
	unsigned char GetStepHide()
		{ return (unsigned char)m_nStepHide; }

	void SetColorCode(lcuint32 ColorCode)
	{
		mColorCode = ColorCode;
		mColorIndex = lcGetColorIndex(ColorCode);
	}

	void SetColorIndex(int ColorIndex)
	{
		mColorIndex = ColorIndex;
		mColorCode = lcGetColorCode(ColorIndex);
	}

public:
	PieceInfo* mPieceInfo;

	int mColorIndex;
	lcuint32 mColorCode;

	lcMatrix44 mModelWorld;
	lcVector3 mPosition;
	lcVector4 mRotation;

protected:
	// Atributes
	Group* m_pGroup;

	lcuint8 m_nStepShow;
	lcuint8 m_nStepHide;

	lcuint8 m_nState;
	char m_strName[81];
};

#endif // _PIECE_H
