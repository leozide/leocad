#pragma once

class PieceInfo;

#include "object.h"
#include "lc_colors.h"
#include "lc_math.h"

#define LC_MAX_CONTROL_POINTS 1000

enum lcPieceSection : quint32
{
	LC_PIECE_SECTION_INVALID = ~0U,
	LC_PIECE_SECTION_POSITION = 0,
	LC_PIECE_SECTION_CONTROL_POINT_FIRST,
	LC_PIECE_SECTION_CONTROL_POINT_LAST = LC_PIECE_SECTION_CONTROL_POINT_FIRST + LC_MAX_CONTROL_POINTS - 1,
};

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
		return mSelected;
	}

	bool IsSelected(quint32 Section) const override
	{
		Q_UNUSED(Section);

		return mSelected;
	}

	void SetSelected(bool Selected) override
	{
		mSelected = Selected;
		if (!Selected)
			mFocusedSection = LC_PIECE_SECTION_INVALID;
	}

	void SetSelected(quint32 Section, bool Selected) override
	{
		Q_UNUSED(Section);

		mSelected = Selected;
		if (!Selected)
			mFocusedSection = LC_PIECE_SECTION_INVALID;
	}

	bool IsFocused() const override
	{
		return mFocusedSection != LC_PIECE_SECTION_INVALID;
	}

	bool IsFocused(quint32 Section) const override
	{
		return mFocusedSection == Section;
	}

	void SetFocused(quint32 Section, bool Focused) override
	{
		if (Focused)
		{
			mFocusedSection = Section;
			mSelected = true;
		}
		else
			mFocusedSection = LC_PIECE_SECTION_INVALID;
	}

	quint32 GetFocusSection() const override
	{
		return mFocusedSection;
	}

	quint32 GetAllowedTransforms() const override;

	lcVector3 GetSectionPosition(quint32 Section) const override
	{
		if (Section == LC_PIECE_SECTION_POSITION)
		{
			if (mPivotPointValid)
				return lcMul(mPivotMatrix, mModelWorld).GetTranslation();
			else
				return mModelWorld.GetTranslation();
		}
		else
		{
			const int ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_FIRST;

			if (ControlPointIndex >= 0 && ControlPointIndex < mControlPoints.GetSize())
			{
				const lcMatrix44& Transform = mControlPoints[ControlPointIndex].Transform;
				return lcMul(Transform, mModelWorld).GetTranslation();
			}
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

	void AddMainModelRenderMeshes(lcScene* Scene, bool Highlight, bool Fade) const;
	void AddSubModelRenderMeshes(lcScene* Scene, const lcMatrix44& WorldMatrix, int DefaultColorIndex, lcRenderMeshState RenderMeshState, bool ParentActive) const;
	void SubModelCompareBoundingBox(const lcMatrix44& WorldMatrix, lcVector3& Min, lcVector3& Max) const;
	void SubModelAddBoundingBoxPoints(const lcMatrix44& WorldMatrix, std::vector<lcVector3>& Points) const;

	void InsertTime(lcStep Start, lcStep Time);
	void RemoveTime(lcStep Start, lcStep Time);

	bool IsHidden() const
	{
		return mHidden;
	}

	void SetHidden(bool Hidden)
	{
		mHidden = Hidden;
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

	QString GetName() const override;
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

	quint32 GetColorCode() const
	{
		return mColorCode;
	}

	void SetColorCode(quint32 ColorCode)
	{
		mColorCode = ColorCode;
		mColorIndex = lcGetColorIndex(ColorCode);
	}

	int GetColorIndex() const
	{
		return mColorIndex;
	}

	void SetColorIndex(int ColorIndex)
	{
		mColorIndex = ColorIndex;
		mColorCode = lcGetColorCode(ColorIndex);
	}

	void SetPosition(const lcVector3& Position, lcStep Step, bool AddKey)
	{
		mPositionKeys.ChangeKey(Position, Step, AddKey);
	}

	void SetRotation(const lcMatrix33& Rotation, lcStep Step, bool AddKey)
	{
		mRotationKeys.ChangeKey(Rotation, Step, AddKey);
	}

	lcVector3 GetRotationCenter() const
	{
		const quint32 Section = GetFocusSection();

		if (Section == LC_PIECE_SECTION_POSITION || Section == LC_PIECE_SECTION_INVALID)
		{
			if (mPivotPointValid)
				return lcMul31(mPivotMatrix.GetTranslation(), mModelWorld);
			else
				return mModelWorld.GetTranslation();
		}
		else
		{
			const int ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_FIRST;

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
			if (mPivotPointValid)
				return lcMatrix33(lcMul(mModelWorld, mPivotMatrix));
			else
				return lcMatrix33(mModelWorld);
		}
		else
		{
			const int ControlPointIndex = Section - LC_PIECE_SECTION_CONTROL_POINT_FIRST;

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
		mPivotPointValid = false;
		mPivotMatrix = lcMatrix44Identity();
	}

public:
	PieceInfo* mPieceInfo;

	lcMatrix44 mModelWorld;
	lcMatrix44 mPivotMatrix;

protected:
	void UpdateMesh();

	bool IsPivotPointVisible() const
	{
		return mPivotPointValid && IsFocused();
	}

	bool AreControlPointsVisible() const
	{
		return IsSelected();
	}

	lcObjectKeyArray<lcVector3> mPositionKeys;
	lcObjectKeyArray<lcMatrix33> mRotationKeys;

	int mFileLine;
	QString mID;

	lcGroup* mGroup;

	int mColorIndex;
	quint32 mColorCode;

	lcStep mStepShow;
	lcStep mStepHide;

	bool mPivotPointValid = false;
	bool mHidden = false;
	bool mSelected = false;
	quint32 mFocusedSection;
	lcArray<lcPieceControlPoint> mControlPoints;
	lcMesh* mMesh;
};
