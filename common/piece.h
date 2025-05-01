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
	LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST = 1
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
	lcVector3 GetSectionPosition(quint32 Section) const override;

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
	QVariant GetPropertyValue(lcObjectPropertyId PropertyId) const override;
	bool SetPropertyValue(lcObjectPropertyId PropertyId, lcStep Step, bool AddKey, QVariant Value) override;
	bool HasKeyFrame(lcObjectPropertyId PropertyId, lcStep Time) const override;
	bool SetKeyFrame(lcObjectPropertyId PropertyId, lcStep Time, bool KeyFrame) override;
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

	const std::vector<lcPieceControlPoint>& GetControlPoints() const
	{
		return mControlPoints;
	}

	void SetControlPoints(const std::vector<lcPieceControlPoint>& ControlPoints)
	{
		mControlPoints = ControlPoints;
		UpdateMesh();
	}

	void SetControlPointScale(int ControlPointIndex, float Scale)
	{
		mControlPoints[ControlPointIndex].Scale = Scale;
		UpdateMesh();
	}

	bool IsTrainTrackConnected(int ConnectionIndex) const
	{
		return mTrainTrackConnections[ConnectionIndex];
	}

	void SetTrainTrackConnections(std::vector<bool>&& Connections)
	{
		mTrainTrackConnections = std::move(Connections);
	}

	const QString& GetID() const
	{
		return mID;
	}

	void UpdateID();

	QString GetName() const override;
	bool IsVisible(lcStep Step) const;
	bool IsVisibleInSubModel() const;

	bool AreTrainTrackConnectionsVisible() const
	{
		return IsFocused();
	}

	void GetModelParts(const lcMatrix44& WorldMatrix, int DefaultColorIndex, std::vector<lcModelPartsEntry>& ModelParts) const;
	void Initialize(const lcMatrix44& WorldMatrix, lcStep Step);
	const lcBoundingBox& GetBoundingBox() const;
	void CompareBoundingBox(lcVector3& Min, lcVector3& Max) const;
	void SetPieceInfo(PieceInfo* Info, const QString& ID, bool Wait);
	bool SetPieceId(PieceInfo* Info);
	bool FileLoad(lcFile& file);

	void UpdatePosition(lcStep Step) override;
	void MoveSelected(lcStep Step, bool AddKey, const lcVector3& Distance);
	void Rotate(lcStep Step, bool AddKey, const lcMatrix33& RotationMatrix, const lcVector3& Center, const lcMatrix33& RotationFrame);
	void MovePivotPoint(const lcVector3& Distance);
	void RotatePivotPoint(const lcMatrix33& RotationMatrix);

	bool CanAddControlPoint() const;
	bool CanRemoveControlPoint() const;

	bool InsertControlPoint(const lcVector3& WorldStart, const lcVector3& WorldEnd);
	bool RemoveFocusedControlPoint();
	void VerifyControlPoints(std::vector<lcPieceControlPoint>& ControlPoints) const;

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

	bool SetColorIndex(int ColorIndex)
	{
		if (mColorIndex == ColorIndex)
			return false;

		mColorIndex = ColorIndex;
		mColorCode = lcGetColorCode(ColorIndex);

		return true;
	}

	void SetPosition(const lcVector3& Position, lcStep Step, bool AddKey)
	{
		mPosition.ChangeKey(Position, Step, AddKey);
	}

	void SetRotation(const lcMatrix33& Rotation, lcStep Step, bool AddKey)
	{
		mRotation.ChangeKey(Rotation, Step, AddKey);
	}

	lcVector3 GetRotationCenter() const;
	lcMatrix33 GetRelativeRotation() const;

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

	void DrawSynthInterface(lcContext* Context, const lcMatrix44& WorldMatrix) const;

	lcObjectProperty<lcVector3> mPosition = lcObjectProperty<lcVector3>(lcVector3(0.0f, 0.0f, 0.0f));
	lcObjectProperty<lcMatrix33> mRotation = lcObjectProperty<lcMatrix33>(lcMatrix33Identity());

	int mFileLine = -1;
	QString mID;

	lcGroup* mGroup;

	int mColorIndex;
	quint32 mColorCode;

	lcStep mStepShow;
	lcStep mStepHide;

	bool mPivotPointValid = false;
	bool mHidden = false;
	bool mSelected = false;
	quint32 mFocusedSection = LC_PIECE_SECTION_INVALID;
	std::vector<lcPieceControlPoint> mControlPoints;
	std::vector<bool> mTrainTrackConnections;
	lcMesh* mMesh = nullptr;
};
