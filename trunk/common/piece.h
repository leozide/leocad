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

	QJsonObject Save();
	void Load(QJsonObject Piece);

	virtual void RayTest(lcObjectRayTest& ObjectRayTest) const;
	virtual void BoxTest(lcObjectBoxTest& ObjectBoxTest) const;

	void InsertTime(lcStep Start, lcStep Time);
	void RemoveTime(lcStep Start, lcStep Time);

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

	bool IsVisible(lcStep Step);
	void Initialize(const lcVector3& Position, const lcVector4& AxisAngle, lcStep Step);
	void CreateName(const lcArray<lcPiece*>& Pieces);
	void CompareBoundingBox(float box[6]);
	void SetPieceInfo(PieceInfo* pPieceInfo);
	bool FileLoad(lcFile& file);
	void FileSave(lcFile& file) const;

	void UpdatePosition(lcStep Step);
	void Move(lcStep Step, bool AddKey, const lcVector3& Distance);

	lcGroup* GetTopGroup();

	void SetGroup(lcGroup* Group)
	{
		mGroup = Group;
	}

	lcGroup* GetGroup()
	{
		return mGroup;
	}

	void SetName(char* name)
		{ strcpy(m_strName, name); }
	const char* GetName()
		{ return m_strName; }

	lcStep GetStepShow() const
	{
		return mStepShow;
	}

	lcStep GetStepHide() const
	{
		return mStepHide;
	}

	void SetStepHide(lcStep Step)
	{
		mStepHide = Step;
	}

	void SetStepShow(lcStep Step)
	{
		mStepShow = Step;
	}

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

	void SetPosition(const lcVector3& Position, lcStep Step, bool AddKey)
	{
		ChangeKey(mPositionKeys, Position, Step, AddKey);
	}

	void SetRotation(const lcVector4& Rotation, lcStep Step, bool AddKey)
	{
		ChangeKey(mRotationKeys, Rotation, Step, AddKey);
	}

public:
	PieceInfo* mPieceInfo;

	int mColorIndex;
	lcuint32 mColorCode;

	lcMatrix44 mModelWorld;
	lcVector3 mPosition;
	lcVector4 mRotation;

protected:
	lcArray<lcObjectKey<lcVector3>> mPositionKeys;
	lcArray<lcObjectKey<lcVector4>> mRotationKeys;

	// Atributes
	lcGroup* mGroup;

	lcStep mStepShow;
	lcStep mStepHide;

	lcuint8 mState;
	char m_strName[81];
};

#endif // _PIECE_H
