#pragma once

class PieceInfo;

#include "object.h"
#include "lc_colors.h"
#include "lc_math.h"

#define LC_PIECE_HIDDEN                    0x000001
#define LC_PIECE_PIVOT_POINT_VALID         0x000002
#define LC_PIECE_POSITION_SELECTED         0x000004
#define LC_PIECE_POSITION_FOCUSED          0x000008
#define LC_PIECE_CONTROL_POINT_1_SELECTED  0x000010
#define LC_PIECE_CONTROL_POINT_1_FOCUSED   0x000020
#define LC_PIECE_CONTROL_POINT_2_SELECTED  0x000040
#define LC_PIECE_CONTROL_POINT_2_FOCUSED   0x000080
#define LC_PIECE_CONTROL_POINT_3_SELECTED  0x000100
#define LC_PIECE_CONTROL_POINT_3_FOCUSED   0x000200
#define LC_PIECE_CONTROL_POINT_4_SELECTED  0x000400
#define LC_PIECE_CONTROL_POINT_4_FOCUSED   0x000800
#define LC_PIECE_CONTROL_POINT_5_SELECTED  0x001000
#define LC_PIECE_CONTROL_POINT_5_FOCUSED   0x002000
#define LC_PIECE_CONTROL_POINT_6_SELECTED  0x004000
#define LC_PIECE_CONTROL_POINT_6_FOCUSED   0x008000
#define LC_PIECE_CONTROL_POINT_7_SELECTED  0x010000
#define LC_PIECE_CONTROL_POINT_7_FOCUSED   0x020000
#define LC_PIECE_CONTROL_POINT_8_SELECTED  0x040000
#define LC_PIECE_CONTROL_POINT_8_FOCUSED   0x080000
#define LC_PIECE_CONTROL_POINT_9_SELECTED  0x100000
#define LC_PIECE_CONTROL_POINT_9_FOCUSED   0x200000
#define LC_PIECE_CONTROL_POINT_10_SELECTED 0x400000
#define LC_PIECE_CONTROL_POINT_10_FOCUSED  0x800000

#define LC_MAX_CONTROL_POINTS 10

#define LC_PIECE_CONTROL_POINT_SELECTION_MASK (LC_PIECE_CONTROL_POINT_1_SELECTED | LC_PIECE_CONTROL_POINT_2_SELECTED | LC_PIECE_CONTROL_POINT_3_SELECTED | LC_PIECE_CONTROL_POINT_4_SELECTED | LC_PIECE_CONTROL_POINT_5_SELECTED | LC_PIECE_CONTROL_POINT_6_SELECTED | LC_PIECE_CONTROL_POINT_7_SELECTED | LC_PIECE_CONTROL_POINT_8_SELECTED | LC_PIECE_CONTROL_POINT_9_SELECTED | LC_PIECE_CONTROL_POINT_10_SELECTED)
#define LC_PIECE_CONTROL_POINT_FOCUS_MASK (LC_PIECE_CONTROL_POINT_1_FOCUSED | LC_PIECE_CONTROL_POINT_2_FOCUSED | LC_PIECE_CONTROL_POINT_3_FOCUSED | LC_PIECE_CONTROL_POINT_4_FOCUSED | LC_PIECE_CONTROL_POINT_5_FOCUSED | LC_PIECE_CONTROL_POINT_6_FOCUSED | LC_PIECE_CONTROL_POINT_7_FOCUSED | LC_PIECE_CONTROL_POINT_8_FOCUSED | LC_PIECE_CONTROL_POINT_9_FOCUSED | LC_PIECE_CONTROL_POINT_10_FOCUSED)

#define LC_PIECE_SELECTION_MASK     (LC_PIECE_POSITION_SELECTED | LC_PIECE_CONTROL_POINT_SELECTION_MASK)
#define LC_PIECE_FOCUS_MASK         (LC_PIECE_POSITION_FOCUSED | LC_PIECE_CONTROL_POINT_FOCUS_MASK)

enum lcPieceSection
{
	LC_PIECE_SECTION_POSITION,
	LC_PIECE_SECTION_CONTROL_POINT_FIRST,
	LC_PIECE_SECTION_CONTROL_POINT_1 = LC_PIECE_SECTION_CONTROL_POINT_FIRST,
	LC_PIECE_SECTION_CONTROL_POINT_2,
	LC_PIECE_SECTION_CONTROL_POINT_3,
	LC_PIECE_SECTION_CONTROL_POINT_4,
	LC_PIECE_SECTION_CONTROL_POINT_5,
	LC_PIECE_SECTION_CONTROL_POINT_6,
	LC_PIECE_SECTION_CONTROL_POINT_7,
	LC_PIECE_SECTION_CONTROL_POINT_8,
	LC_PIECE_SECTION_CONTROL_POINT_9,
	LC_PIECE_SECTION_CONTROL_POINT_10,
	LC_PIECE_SECTION_CONTROL_POINT_LAST = LC_PIECE_SECTION_CONTROL_POINT_10
};

#define LC_PIECE_SECTION_INVALID (~0U)

struct lcPieceControlPoint
{
	lcMatrix44 Transform;
	float Scale;
};

class lcPiece : public lcObject
{
public:
	lcPiece(PieceInfo* Info);
	lcPiece(const lcPiece& Other);
	~lcPiece();

	lcPiece(lcPiece&&) = delete;
	lcPiece& operator=(const lcPiece&) = delete;
	lcPiece& operator=(lcPiece&&) = delete;

	bool IsSelected() const override
	{
		return (mState & LC_PIECE_SELECTION_MASK) != 0;
	}

	bool IsSelected(quint32 Section) const override
	{
		Q_UNUSED(Section);

		return (mState & LC_PIECE_SELECTION_MASK) != 0;
	}

	void SetSelected(bool Selected) override
	{
		if (Selected)
			mState |= LC_PIECE_SELECTION_MASK;
		else
			mState &= ~(LC_PIECE_SELECTION_MASK | LC_PIECE_FOCUS_MASK);
	}

	void SetSelected(quint32 Section, bool Selected) override
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

		case LC_PIECE_SECTION_CONTROL_POINT_9:
			if (Selected)
				mState |= LC_PIECE_CONTROL_POINT_9_SELECTED;
			else
				mState &= ~(LC_PIECE_CONTROL_POINT_9_SELECTED | LC_PIECE_CONTROL_POINT_9_FOCUSED);
			break;

		case LC_PIECE_SECTION_CONTROL_POINT_10:
			if (Selected)
				mState |= LC_PIECE_CONTROL_POINT_10_SELECTED;
			else
				mState &= ~(LC_PIECE_CONTROL_POINT_10_SELECTED | LC_PIECE_CONTROL_POINT_10_FOCUSED);
			break;
		}
	}

	bool IsFocused() const override
	{
		return (mState & LC_PIECE_FOCUS_MASK) != 0;
	}

	bool IsFocused(quint32 Section) const override
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

		case LC_PIECE_SECTION_CONTROL_POINT_9:
			return (mState & LC_PIECE_CONTROL_POINT_9_FOCUSED) != 0;

		case LC_PIECE_SECTION_CONTROL_POINT_10:
			return (mState & LC_PIECE_CONTROL_POINT_10_FOCUSED) != 0;
		}

		return false;
	}

	void SetFocused(quint32 Section, bool Focused) override
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

		case LC_PIECE_SECTION_CONTROL_POINT_9:
			if (Focused)
				mState |= (LC_PIECE_CONTROL_POINT_9_SELECTED | LC_PIECE_CONTROL_POINT_9_FOCUSED);
			else
				mState &= ~LC_PIECE_CONTROL_POINT_9_FOCUSED;
			break;

		case LC_PIECE_SECTION_CONTROL_POINT_10:
			if (Focused)
				mState |= (LC_PIECE_CONTROL_POINT_10_SELECTED | LC_PIECE_CONTROL_POINT_10_FOCUSED);
			else
				mState &= ~LC_PIECE_CONTROL_POINT_10_FOCUSED;
			break;
		}
	}

	quint32 GetFocusSection() const override
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

		if (mState & LC_PIECE_CONTROL_POINT_9_FOCUSED)
			return LC_PIECE_SECTION_CONTROL_POINT_9;

		if (mState & LC_PIECE_CONTROL_POINT_10_FOCUSED)
			return LC_PIECE_SECTION_CONTROL_POINT_10;

		return LC_PIECE_SECTION_INVALID;
	}

	quint32 GetAllowedTransforms() const override;

	lcVector3 GetSectionPosition(quint32 Section) const override
	{
		switch (Section)
		{
		case LC_PIECE_SECTION_POSITION:
			if (mState & LC_PIECE_PIVOT_POINT_VALID)
				return lcMul(mPivotMatrix, mModelWorld).GetTranslation();
			else
				return mModelWorld.GetTranslation();

		case LC_PIECE_SECTION_CONTROL_POINT_1:
			return lcMul(mControlPoints[0].Transform, mModelWorld).GetTranslation();

		case LC_PIECE_SECTION_CONTROL_POINT_2:
			return lcMul(mControlPoints[1].Transform, mModelWorld).GetTranslation();

		case LC_PIECE_SECTION_CONTROL_POINT_3:
			return lcMul(mControlPoints[2].Transform, mModelWorld).GetTranslation();

		case LC_PIECE_SECTION_CONTROL_POINT_4:
			return lcMul(mControlPoints[3].Transform, mModelWorld).GetTranslation();

		case LC_PIECE_SECTION_CONTROL_POINT_5:
			return lcMul(mControlPoints[4].Transform, mModelWorld).GetTranslation();

		case LC_PIECE_SECTION_CONTROL_POINT_6:
			return lcMul(mControlPoints[5].Transform, mModelWorld).GetTranslation();

		case LC_PIECE_SECTION_CONTROL_POINT_7:
			return lcMul(mControlPoints[6].Transform, mModelWorld).GetTranslation();

		case LC_PIECE_SECTION_CONTROL_POINT_8:
			return lcMul(mControlPoints[7].Transform, mModelWorld).GetTranslation();

		case LC_PIECE_SECTION_CONTROL_POINT_9:
			return lcMul(mControlPoints[8].Transform, mModelWorld).GetTranslation();

		case LC_PIECE_SECTION_CONTROL_POINT_10:
			return lcMul(mControlPoints[9].Transform, mModelWorld).GetTranslation();
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

	void RayTest(lcObjectRayTest& ObjectRayTest) const override;
	void BoxTest(lcObjectBoxTest& ObjectBoxTest) const override;
	void DrawInterface(lcContext* Context, const lcScene& Scene) const override;
	void RemoveKeyFrames() override;

	void AddMainModelRenderMeshes(lcScene& Scene, bool Highlight, bool Fade) const;
	void AddSubModelRenderMeshes(lcScene& Scene, const lcMatrix44& WorldMatrix, int DefaultColorIndex, lcRenderMeshState RenderMeshState, bool ParentActive) const;
	void SubmodelCompareBoundingBox(const lcMatrix44& WorldMatrix, lcVector3& Min, lcVector3& Max) const;

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

	const lcArray<lcPieceControlPoint>& GetControlPoints() const
	{
		return mControlPoints;
	}

	void SetControlPoints(const lcArray<lcPieceControlPoint>& ControlPoints)
	{
		mControlPoints = ControlPoints;
		UpdateMesh();
	}

	void SetControlPointScale(int ControlPointIndex, float Scale)
	{
		mControlPoints[ControlPointIndex].Scale = Scale;
		UpdateMesh();
	}

	const QString& GetID() const
	{
		return mID;
	}

	void UpdateID();

	const char* GetName() const override;
	bool IsVisible(lcStep Step) const;
	bool IsVisibleInSubModel() const;
	void GetModelParts(const lcMatrix44& WorldMatrix, int DefaultColorIndex, std::vector<lcModelPartsEntry>& ModelParts) const;
	void Initialize(const lcMatrix44& WorldMatrix, lcStep Step);
	const lcBoundingBox& GetBoundingBox() const;
	void CompareBoundingBox(lcVector3& Min, lcVector3& Max) const;
	void SetPieceInfo(PieceInfo* Info, const QString& ID, bool Wait);
	bool FileLoad(lcFile& file);

	void UpdatePosition(lcStep Step);
	void MoveSelected(lcStep Step, bool AddKey, const lcVector3& Distance);
	void Rotate(lcStep Step, bool AddKey, const lcMatrix33& RotationMatrix, const lcVector3& Center, const lcMatrix33& RotationFrame);
	void MovePivotPoint(const lcVector3& Distance);
	void RotatePivotPoint(const lcMatrix33& RotationMatrix);

	bool CanAddControlPoint() const;
	bool CanRemoveControlPoint() const;

	bool InsertControlPoint(const lcVector3& WorldStart, const lcVector3& WorldEnd);
	bool RemoveFocusedControlPoint();
	void VerifyControlPoints(lcArray<lcPieceControlPoint>& ControlPoints) const;

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
		if (Step < 2)
			Step = 2;

		mStepHide = Step;

		if (mStepHide <= mStepShow)
			SetStepShow(mStepHide - 1);
	}

	void SetStepShow(lcStep Step)
	{
		if (Step == LC_STEP_MAX)
			Step = LC_STEP_MAX - 1;

		if (mStepShow != Step)
		{
			mFileLine = -1;
			mStepShow = Step;
		}

		if (mStepHide <= mStepShow)
			mStepHide = mStepShow + 1;
	}

	void SetColorCode(quint32 ColorCode)
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
		const quint32 Section = GetFocusSection();

		if (Section == LC_PIECE_SECTION_POSITION || Section == LC_PIECE_SECTION_INVALID)
		{
			if (mState & LC_PIECE_PIVOT_POINT_VALID)
				return lcMul31(mPivotMatrix.GetTranslation(), mModelWorld);
			else
				return mModelWorld.GetTranslation();
		}
		else
		{
			const int ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_1;

			if (ControlPointIndex >= 0 && ControlPointIndex < mControlPoints.GetSize())
			{
				const lcMatrix44& Transform = mControlPoints[ControlPointIndex].Transform;
				return lcMul31(Transform.GetTranslation(), mModelWorld);
			}

			return mModelWorld.GetTranslation();
		}
	}

	lcMatrix33 GetRelativeRotation() const
	{
		const quint32 Section = GetFocusSection();

		if (Section == LC_PIECE_SECTION_POSITION || Section == LC_PIECE_SECTION_INVALID)
		{
			if (mState & LC_PIECE_PIVOT_POINT_VALID)
				return lcMatrix33(lcMul(mModelWorld, mPivotMatrix));
			else
				return lcMatrix33(mModelWorld);
		}
		else
		{
			const int ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_1;

			if (ControlPointIndex >= 0 && ControlPointIndex < mControlPoints.GetSize())
			{
				const lcMatrix44& Transform = mControlPoints[ControlPointIndex].Transform;
				return lcMatrix33(lcMul(Transform, mModelWorld));
			}

			return lcMatrix33Identity();
		}
	}

	void ResetPivotPoint()
	{
		mState &= ~LC_PIECE_PIVOT_POINT_VALID;
		mPivotMatrix = lcMatrix44Identity();
	}

public:
	PieceInfo* mPieceInfo;

	int mColorIndex;
	quint32 mColorCode;

	lcMatrix44 mModelWorld;
	lcMatrix44 mPivotMatrix;

protected:
	void UpdateMesh();

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
	QString mID;

	lcGroup* mGroup;

	lcStep mStepShow;
	lcStep mStepHide;

	quint32 mState;
	lcArray<lcPieceControlPoint> mControlPoints;
	lcMesh* mMesh;
};
