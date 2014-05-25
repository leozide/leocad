#ifndef _PIECE_H_
#define _PIECE_H_

class PieceInfo;

#include "object.h"
#include "lc_colors.h"
#include "lc_math.h"

#define LC_PIECE_HIDDEN             0x01
#define LC_PIECE_POSITION_SELECTED  0x02
#define LC_PIECE_POSITION_FOCUSED   0x04

#define LC_PIECE_SELECTION_MASK     (LC_PIECE_POSITION_SELECTED)
#define LC_PIECE_FOCUS_MASK         (LC_PIECE_POSITION_FOCUSED)

enum lcPieceSection
{
	LC_PIECE_SECTION_POSITION
};

enum LC_PK_TYPES
{
	LC_PK_POSITION,
	LC_PK_ROTATION,
	LC_PK_COUNT
};

class lcPiece : public lcObject
{
public:
	lcPiece(PieceInfo* pPieceInfo);
	~lcPiece();

	virtual bool IsSelected() const
	{
		return (mState & LC_PIECE_SELECTION_MASK) != 0;
	}

	virtual bool IsSelected(lcuint32 Section) const
	{
		return (mState & LC_PIECE_SELECTION_MASK) != 0;
	}

	virtual void SetSelected(bool Selected)
	{
		if (Selected)
			mState |= LC_PIECE_SELECTION_MASK;
		else
			mState &= ~(LC_PIECE_SELECTION_MASK | LC_PIECE_FOCUS_MASK);
	}

	virtual void SetSelected(lcuint32 Section, bool Selected)
	{
		if (Selected)
			mState |= LC_PIECE_POSITION_SELECTED;
		else
			mState &= ~(LC_PIECE_SELECTION_MASK | LC_PIECE_FOCUS_MASK);
	}

	virtual bool IsFocused() const
	{
		return (mState & LC_PIECE_FOCUS_MASK) != 0;
	}

	virtual bool IsFocused(lcuint32 Section) const
	{
		return (mState & LC_PIECE_FOCUS_MASK) != 0;
	}

	virtual void SetFocused(lcuint32 Section, bool Focused)
	{
		if (Focused)
			mState |= LC_PIECE_POSITION_SELECTED | LC_PIECE_POSITION_FOCUSED;
		else
			mState &= ~LC_PIECE_FOCUS_MASK;
	}

	virtual lcuint32 GetFocusSection() const
	{
		if (mState & LC_PIECE_POSITION_FOCUSED)
			return LC_PIECE_SECTION_POSITION;

		return ~0;
	}

	virtual lcVector3 GetSectionPosition(lcuint32 Section) const
	{
		switch (Section)
		{
		case LC_PIECE_SECTION_POSITION:
			return mPosition;
		}

		return lcVector3(0.0f, 0.0f, 0.0f);
	}

	virtual void RayTest(lcObjectRayTest& ObjectRayTest) const;
	virtual void BoxTest(lcObjectBoxTest& ObjectBoxTest) const;

	virtual void InsertTime(unsigned short start, unsigned short time);
	virtual void RemoveTime(unsigned short start, unsigned short time);

	bool IsHidden()
	{
		return (mState & LC_PIECE_HIDDEN) != 0;
	}

	void SetHidden(bool Hidden)
	{
		if (Hidden)
			mState |= LC_PIECE_HIDDEN;
		else
			mState &= ~LC_PIECE_HIDDEN;
	}

	const char* GetName() const
	{
		return m_strName;
	}

	bool IsVisible(unsigned short nTime);
	void Initialize(float x, float y, float z, unsigned char nStep);
	void CreateName(const lcArray<Piece*>& Pieces);
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

	lcuint8 mState;
	char m_strName[81];
};

#endif // _PIECE_H
