#ifndef _PIECE_H_
#define _PIECE_H_

class PieceInfo;

#include "object.h"
#include "lc_colors.h"
#include "lc_math.h"

#define LC_PIECE_HIDDEN                   0x00001
#define LC_PIECE_PIVOT_POINT_VALID        0x00002
#define LC_PIECE_POSITION_SELECTED        0x00004
#define LC_PIECE_POSITION_FOCUSED         0x00008
#define LC_PIECE_CONTROL_POINT_1_SELECTED 0x00010
#define LC_PIECE_CONTROL_POINT_1_FOCUSED  0x00020
#define LC_PIECE_CONTROL_POINT_2_SELECTED 0x00040
#define LC_PIECE_CONTROL_POINT_2_FOCUSED  0x00080
#define LC_PIECE_CONTROL_POINT_3_SELECTED 0x00100
#define LC_PIECE_CONTROL_POINT_3_FOCUSED  0x00200
#define LC_PIECE_CONTROL_POINT_4_SELECTED 0x00400
#define LC_PIECE_CONTROL_POINT_4_FOCUSED  0x00800
#define LC_PIECE_CONTROL_POINT_5_SELECTED 0x01000
#define LC_PIECE_CONTROL_POINT_5_FOCUSED  0x02000
#define LC_PIECE_CONTROL_POINT_6_SELECTED 0x04000
#define LC_PIECE_CONTROL_POINT_6_FOCUSED  0x08000
#define LC_PIECE_CONTROL_POINT_7_SELECTED 0x10000
#define LC_PIECE_CONTROL_POINT_7_FOCUSED  0x20000
#define LC_PIECE_CONTROL_POINT_8_SELECTED 0x40000
#define LC_PIECE_CONTROL_POINT_8_FOCUSED  0x80000

#define LC_PIECE_SELECTION_MASK     (LC_PIECE_POSITION_SELECTED | LC_PIECE_CONTROL_POINT_1_SELECTED | LC_PIECE_CONTROL_POINT_2_SELECTED | LC_PIECE_CONTROL_POINT_3_SELECTED | LC_PIECE_CONTROL_POINT_4_SELECTED | LC_PIECE_CONTROL_POINT_5_SELECTED | LC_PIECE_CONTROL_POINT_6_SELECTED | LC_PIECE_CONTROL_POINT_7_SELECTED | LC_PIECE_CONTROL_POINT_8_SELECTED)
#define LC_PIECE_FOCUS_MASK         (LC_PIECE_POSITION_FOCUSED | LC_PIECE_CONTROL_POINT_1_FOCUSED | LC_PIECE_CONTROL_POINT_2_FOCUSED | LC_PIECE_CONTROL_POINT_3_FOCUSED | LC_PIECE_CONTROL_POINT_4_FOCUSED | LC_PIECE_CONTROL_POINT_5_FOCUSED | LC_PIECE_CONTROL_POINT_6_FOCUSED | LC_PIECE_CONTROL_POINT_7_FOCUSED | LC_PIECE_CONTROL_POINT_8_FOCUSED)

enum lcPieceSection
{
	LC_PIECE_SECTION_POSITION,
	LC_PIECE_SECTION_CONTROL_POINT_1,
	LC_PIECE_SECTION_CONTROL_POINT_2,
	LC_PIECE_SECTION_CONTROL_POINT_3,
	LC_PIECE_SECTION_CONTROL_POINT_4,
	LC_PIECE_SECTION_CONTROL_POINT_5,
	LC_PIECE_SECTION_CONTROL_POINT_6,
	LC_PIECE_SECTION_CONTROL_POINT_7,
	LC_PIECE_SECTION_CONTROL_POINT_8
};

#define LC_PIECE_SECTION_INVALID (~0U)

struct lcPieceControlPoint
{
	lcVector3 Position;
};

class lcPiece : public lcObject
{
public:
	lcPiece(PieceInfo* Info);
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
		switch (Section)
		{
		case LC_PIECE_SECTION_POSITION:
			if (Selected)
				mState |= LC_PIECE_POSITION_SELECTED;
			else
				mState &= ~(LC_PIECE_POSITION_SELECTED | LC_PIECE_POSITION_FOCUSED);
			break;

		case LC_PIECE_SECTION_CONTROL_POINT_1:
			if (Selected)
				mState |= LC_PIECE_CONTROL_POINT_1_SELECTED;
			else
				mState &= ~(LC_PIECE_CONTROL_POINT_1_SELECTED | LC_PIECE_CONTROL_POINT_1_FOCUSED);
			break;

		case LC_PIECE_SECTION_CONTROL_POINT_2:
			if (Selected)
				mState |= LC_PIECE_CONTROL_POINT_2_SELECTED;
			else
				mState &= ~(LC_PIECE_CONTROL_POINT_2_SELECTED | LC_PIECE_CONTROL_POINT_2_FOCUSED);
			break;

		case LC_PIECE_SECTION_CONTROL_POINT_3:
			if (Selected)
				mState |= LC_PIECE_CONTROL_POINT_3_SELECTED;
			else
				mState &= ~(LC_PIECE_CONTROL_POINT_3_SELECTED | LC_PIECE_CONTROL_POINT_3_FOCUSED);
			break;

		case LC_PIECE_SECTION_CONTROL_POINT_4:
			if (Selected)
				mState |= LC_PIECE_CONTROL_POINT_4_SELECTED;
			else
				mState &= ~(LC_PIECE_CONTROL_POINT_4_SELECTED | LC_PIECE_CONTROL_POINT_4_FOCUSED);
			break;

		case LC_PIECE_SECTION_CONTROL_POINT_5:
			if (Selected)
				mState |= LC_PIECE_CONTROL_POINT_5_SELECTED;
			else
				mState &= ~(LC_PIECE_CONTROL_POINT_5_SELECTED | LC_PIECE_CONTROL_POINT_5_FOCUSED);
			break;

		case LC_PIECE_SECTION_CONTROL_POINT_6:
			if (Selected)
				mState |= LC_PIECE_CONTROL_POINT_6_SELECTED;
			else
				mState &= ~(LC_PIECE_CONTROL_POINT_6_SELECTED | LC_PIECE_CONTROL_POINT_6_FOCUSED);
			break;

		case LC_PIECE_SECTION_CONTROL_POINT_7:
			if (Selected)
				mState |= LC_PIECE_CONTROL_POINT_7_SELECTED;
			else
				mState &= ~(LC_PIECE_CONTROL_POINT_7_SELECTED | LC_PIECE_CONTROL_POINT_7_FOCUSED);
			break;

		case LC_PIECE_SECTION_CONTROL_POINT_8:
			if (Selected)
				mState |= LC_PIECE_CONTROL_POINT_8_SELECTED;
			else
				mState &= ~(LC_PIECE_CONTROL_POINT_8_SELECTED | LC_PIECE_CONTROL_POINT_8_FOCUSED);
			break;
		}
	}

	virtual bool IsFocused() const
	{
		return (mState & LC_PIECE_FOCUS_MASK) != 0;
	}

	virtual bool IsFocused(lcuint32 Section) const
	{
		switch (Section)
		{
		case LC_PIECE_SECTION_POSITION:
			return (mState & LC_PIECE_POSITION_FOCUSED) != 0;

		case LC_PIECE_SECTION_CONTROL_POINT_1:
			return (mState & LC_PIECE_CONTROL_POINT_1_FOCUSED) != 0;

		case LC_PIECE_SECTION_CONTROL_POINT_2:
			return (mState & LC_PIECE_CONTROL_POINT_2_FOCUSED) != 0;

		case LC_PIECE_SECTION_CONTROL_POINT_3:
			return (mState & LC_PIECE_CONTROL_POINT_3_FOCUSED) != 0;

		case LC_PIECE_SECTION_CONTROL_POINT_4:
			return (mState & LC_PIECE_CONTROL_POINT_4_FOCUSED) != 0;

		case LC_PIECE_SECTION_CONTROL_POINT_5:
			return (mState & LC_PIECE_CONTROL_POINT_5_FOCUSED) != 0;

		case LC_PIECE_SECTION_CONTROL_POINT_6:
			return (mState & LC_PIECE_CONTROL_POINT_6_FOCUSED) != 0;

		case LC_PIECE_SECTION_CONTROL_POINT_7:
			return (mState & LC_PIECE_CONTROL_POINT_7_FOCUSED) != 0;

		case LC_PIECE_SECTION_CONTROL_POINT_8:
			return (mState & LC_PIECE_CONTROL_POINT_8_FOCUSED) != 0;
		}

		return false;
	}

	virtual void SetFocused(lcuint32 Section, bool Focused)
	{
		switch (Section)
		{
		case LC_PIECE_SECTION_POSITION:
			if (Focused)
				mState |= (LC_PIECE_POSITION_SELECTED | LC_PIECE_POSITION_FOCUSED);
			else
				mState &= ~LC_PIECE_POSITION_FOCUSED;
			break;

		case LC_PIECE_SECTION_CONTROL_POINT_1:
			if (Focused)
				mState |= (LC_PIECE_CONTROL_POINT_1_SELECTED | LC_PIECE_CONTROL_POINT_1_FOCUSED);
			else
				mState &= ~LC_PIECE_CONTROL_POINT_1_FOCUSED;
			break;

		case LC_PIECE_SECTION_CONTROL_POINT_2:
			if (Focused)
				mState |= (LC_PIECE_CONTROL_POINT_2_SELECTED | LC_PIECE_CONTROL_POINT_2_FOCUSED);
			else
				mState &= ~LC_PIECE_CONTROL_POINT_2_FOCUSED;
			break;

		case LC_PIECE_SECTION_CONTROL_POINT_3:
			if (Focused)
				mState |= (LC_PIECE_CONTROL_POINT_3_SELECTED | LC_PIECE_CONTROL_POINT_3_FOCUSED);
			else
				mState &= ~LC_PIECE_CONTROL_POINT_3_FOCUSED;
			break;

		case LC_PIECE_SECTION_CONTROL_POINT_4:
			if (Focused)
				mState |= (LC_PIECE_CONTROL_POINT_4_SELECTED | LC_PIECE_CONTROL_POINT_4_FOCUSED);
			else
				mState &= ~LC_PIECE_CONTROL_POINT_4_FOCUSED;
			break;

		case LC_PIECE_SECTION_CONTROL_POINT_5:
			if (Focused)
				mState |= (LC_PIECE_CONTROL_POINT_5_SELECTED | LC_PIECE_CONTROL_POINT_5_FOCUSED);
			else
				mState &= ~LC_PIECE_CONTROL_POINT_5_FOCUSED;
			break;

		case LC_PIECE_SECTION_CONTROL_POINT_6:
			if (Focused)
				mState |= (LC_PIECE_CONTROL_POINT_6_SELECTED | LC_PIECE_CONTROL_POINT_6_FOCUSED);
			else
				mState &= ~LC_PIECE_CONTROL_POINT_6_FOCUSED;
			break;

		case LC_PIECE_SECTION_CONTROL_POINT_7:
			if (Focused)
				mState |= (LC_PIECE_CONTROL_POINT_7_SELECTED | LC_PIECE_CONTROL_POINT_7_FOCUSED);
			else
				mState &= ~LC_PIECE_CONTROL_POINT_7_FOCUSED;
			break;

		case LC_PIECE_SECTION_CONTROL_POINT_8:
			if (Focused)
				mState |= (LC_PIECE_CONTROL_POINT_8_SELECTED | LC_PIECE_CONTROL_POINT_8_FOCUSED);
			else
				mState &= ~LC_PIECE_CONTROL_POINT_8_FOCUSED;
			break;
		}
	}

	virtual lcuint32 GetFocusSection() const
	{
		if (mState & LC_PIECE_POSITION_FOCUSED)
			return LC_PIECE_SECTION_POSITION;

		if (mState & LC_PIECE_CONTROL_POINT_1_FOCUSED)
			return LC_PIECE_SECTION_CONTROL_POINT_1;

		if (mState & LC_PIECE_CONTROL_POINT_2_FOCUSED)
			return LC_PIECE_SECTION_CONTROL_POINT_2;

		if (mState & LC_PIECE_CONTROL_POINT_3_FOCUSED)
			return LC_PIECE_SECTION_CONTROL_POINT_3;

		if (mState & LC_PIECE_CONTROL_POINT_4_FOCUSED)
			return LC_PIECE_SECTION_CONTROL_POINT_4;

		if (mState & LC_PIECE_CONTROL_POINT_5_FOCUSED)
			return LC_PIECE_SECTION_CONTROL_POINT_5;

		if (mState & LC_PIECE_CONTROL_POINT_6_FOCUSED)
			return LC_PIECE_SECTION_CONTROL_POINT_6;

		if (mState & LC_PIECE_CONTROL_POINT_7_FOCUSED)
			return LC_PIECE_SECTION_CONTROL_POINT_7;

		if (mState & LC_PIECE_CONTROL_POINT_8_FOCUSED)
			return LC_PIECE_SECTION_CONTROL_POINT_8;

		return LC_PIECE_SECTION_INVALID;
	}

	virtual lcVector3 GetSectionPosition(lcuint32 Section) const
	{
		switch (Section)
		{
		case LC_PIECE_SECTION_POSITION:
			if (mState & LC_PIECE_PIVOT_POINT_VALID)
				return lcMul(mPivotMatrix, mModelWorld).GetTranslation();
			else
				return mModelWorld.GetTranslation();

		case LC_PIECE_SECTION_CONTROL_POINT_1:
			return lcMul31(mControlPoints[0].Position, mModelWorld);

		case LC_PIECE_SECTION_CONTROL_POINT_2:
			return lcMul31(mControlPoints[1].Position, mModelWorld);

		case LC_PIECE_SECTION_CONTROL_POINT_3:
			return lcMul31(mControlPoints[2].Position, mModelWorld);

		case LC_PIECE_SECTION_CONTROL_POINT_4:
			return lcMul31(mControlPoints[3].Position, mModelWorld);

		case LC_PIECE_SECTION_CONTROL_POINT_5:
			return lcMul31(mControlPoints[4].Position, mModelWorld);

		case LC_PIECE_SECTION_CONTROL_POINT_6:
			return lcMul31(mControlPoints[5].Position, mModelWorld);

		case LC_PIECE_SECTION_CONTROL_POINT_7:
			return lcMul31(mControlPoints[6].Position, mModelWorld);

		case LC_PIECE_SECTION_CONTROL_POINT_8:
			return lcMul31(mControlPoints[7].Position, mModelWorld);
		}

		return lcVector3(0.0f, 0.0f, 0.0f);
	}

	void SaveLDraw(QTextStream& Stream) const;
	bool ParseLDrawLine(QTextStream& Stream);

	void SetFileLine(int Line)
	{
		mFileLine = Line;
	}

	int GetFileLine() const
	{
		return mFileLine;
	}

	virtual void RayTest(lcObjectRayTest& ObjectRayTest) const;
	virtual void BoxTest(lcObjectBoxTest& ObjectBoxTest) const;
	virtual void DrawInterface(lcContext* Context) const;

	void InsertTime(lcStep Start, lcStep Time);
	void RemoveTime(lcStep Start, lcStep Time);

	bool IsHidden() const
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

	const char* GetName() const;
	bool IsVisible(lcStep Step);
	void Initialize(const lcMatrix44& WorldMatrix, lcStep Step);
	void CompareBoundingBox(float box[6]);
	void SetPieceInfo(PieceInfo* Info);
	bool FileLoad(lcFile& file);

	void UpdatePosition(lcStep Step);
	void Move(lcStep Step, bool AddKey, const lcVector3& Distance);
	void Rotate(lcStep Step, bool AddKey, const lcMatrix33& RotationMatrix, const lcVector3& Center, const lcMatrix33& RotationFrame);
	void MovePivotPoint(const lcVector3& Distance);
	void RotatePivotPoint(const lcMatrix33& RotationMatrix);

	lcGroup* GetTopGroup();

	void SetGroup(lcGroup* Group)
	{
		mGroup = Group;
	}

	lcGroup* GetGroup()
	{
		return mGroup;
	}

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
		mFileLine = -1;
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

	void SetRotation(const lcMatrix33& Rotation, lcStep Step, bool AddKey)
	{
		ChangeKey(mRotationKeys, Rotation, Step, AddKey);
	}

	lcVector3 GetRotationCenter() const
	{
		if (mState & LC_PIECE_PIVOT_POINT_VALID)
			return lcMul31(mPivotMatrix.GetTranslation(), mModelWorld);
		else
			return mModelWorld.GetTranslation();
	}

	lcMatrix33 GetRelativeRotation() const
	{
		if (mState & LC_PIECE_PIVOT_POINT_VALID)
			return lcMatrix33(lcMul(mModelWorld, mPivotMatrix));
		else
			return lcMatrix33(mModelWorld);
	}

	void ResetPivotPoint()
	{
		mState &= ~LC_PIECE_PIVOT_POINT_VALID;
	}

public:
	PieceInfo* mPieceInfo;

	int mColorIndex;
	lcuint32 mColorCode;

	lcMatrix44 mModelWorld;
	lcMatrix44 mPivotMatrix;

protected:
	bool IsPivotPointVisible() const
	{
		return (mState & LC_PIECE_PIVOT_POINT_VALID) && IsFocused();
	}

	bool AreControlPointsVisible() const
	{
		return IsSelected();
	}

	lcArray<lcObjectKey<lcVector3>> mPositionKeys;
	lcArray<lcObjectKey<lcMatrix33>> mRotationKeys;

	int mFileLine;

	// Atributes
	lcGroup* mGroup;

	lcStep mStepShow;
	lcStep mStepHide;

	lcuint8 mState;
	lcArray<lcPieceControlPoint> mControlPoints;
};

#endif // _PIECE_H_
