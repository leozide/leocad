#pragma once

#include "lc_math.h"

struct lcInsertPieceInfo;
enum class lcObjectType;
enum class lcLightType;
enum class lcTool;

class lcModelAction
{
public:
	lcModelAction() = default;
	virtual ~lcModelAction() = default;
};

enum class lcModelActionSelectionMode
{
	Clear,
	Set,
	Save,
	Restore
};

class lcModelActionSelection : public lcModelAction
{
public:
	lcModelActionSelection(lcModelActionSelectionMode Mode);
	virtual ~lcModelActionSelection() = default;

	lcModelActionSelectionMode GetMode() const
	{
		return mMode;
	}

	void SetSelection(const std::vector<std::unique_ptr<lcPiece>>& Pieces, const std::vector<std::unique_ptr<lcCamera>>& Cameras, const std::vector<std::unique_ptr<lcLight>>& Lights);
	std::tuple<std::vector<lcObject*>, lcObject*, uint32_t> GetSelection(const std::vector<std::unique_ptr<lcPiece>>& Pieces, const std::vector<std::unique_ptr<lcCamera>>& Cameras, const std::vector<std::unique_ptr<lcLight>>& Lights) const;

protected:
	std::vector<size_t> mSelectedPieces;
	std::vector<size_t> mSelectedCameras;
	std::vector<size_t> mSelectedLights;
	size_t mFocusIndex = SIZE_MAX;
	uint32_t mFocusSection = 0;
	lcObjectType mFocusType = (lcObjectType)0;
	lcModelActionSelectionMode mMode = lcModelActionSelectionMode::Clear;
};

class lcModelActionMouseTool : public lcModelAction
{
public:
	lcModelActionMouseTool(lcTool Tool);
	virtual ~lcModelActionMouseTool() = default;

	void SaveSelectionStartState(const lcModel* Model);
	void LoadSelectionStartState(lcModel* Model) const;
	void SaveSelectionEndState(const lcModel* Model);
	void LoadSelectionEndState(lcModel* Model) const;

	void SetCameraStartState(const lcCamera* Camera);
	void SetCameraEndState(const lcCamera* Camera);

	const QByteArray& GetStartState() const
	{
		return mStartState;
	}

	const QByteArray& GetEndState() const
	{
		return mEndState;
	}

	lcTool GetTool() const
	{
		return mTool;
	}

	const QString& GetCameraName() const
	{
		return mCameraName;
	}

protected:
	static void SaveSelectionState(const lcModel* Model, QByteArray& State);
	static void LoadSelectionState(lcModel* Model, const QByteArray& State);

	lcTool mTool;
	QString mCameraName;
	QByteArray mStartState;
	QByteArray mEndState;
};

enum class lcModelActionAddPieceSelectionMode
{
	FocusLast,
	SelectAll,
	AddToSelection
};

class lcModelActionAddPieces : public lcModelAction
{
public:
	lcModelActionAddPieces(lcStep Step, lcModelActionAddPieceSelectionMode SelectionMode);
	virtual ~lcModelActionAddPieces() = default;

	struct PieceData
	{
		std::string PieceId;
		lcMatrix44 Transform;
		quint32 ColorCode;
	};

	void SetPieceData(const std::vector<lcInsertPieceInfo>& PieceInfoTransforms);

	const std::vector<PieceData>& GetPieceData() const
	{
		return mPieceData;
	}

	lcStep GetStep() const
	{
		return mStep;
	}

	lcModelActionAddPieceSelectionMode GetSelectionMode() const
	{
		return mSelectionMode;
	}

protected:
	std::vector<PieceData> mPieceData;
	lcStep mStep;
	lcModelActionAddPieceSelectionMode mSelectionMode;
};

class lcModelActionAddLight : public lcModelAction
{
public:
	lcModelActionAddLight(const lcVector3& Position, lcLightType LightType);
	virtual ~lcModelActionAddLight() = default;

	const lcVector3& GetPosition() const
	{
		return mPosition;
	}

	lcLightType GetLightType() const
	{
		return mLightType;
	}

protected:
	lcVector3 mPosition;
	lcLightType mLightType;
};

enum class lcModelActionGroupPiecesMode
{
	Group,
	Ungroup
};

class lcModelActionGroupPieces : public lcModelAction
{
public:
	lcModelActionGroupPieces(lcModelActionGroupPiecesMode Mode, const QString& GroupName);
	virtual ~lcModelActionGroupPieces() = default;

	lcModelActionGroupPiecesMode GetMode() const
	{
		return mMode;
	}

	const QString& GetGroupName() const
	{
		return mGroupName;
	}

protected:
	lcModelActionGroupPiecesMode mMode;
	QString mGroupName;
};

class lcModelActionDuplicatePieces : public lcModelAction
{
public:
	lcModelActionDuplicatePieces(lcStep Step);
	virtual ~lcModelActionDuplicatePieces() = default;

	lcStep GetStep() const
	{
		return mStep;
	}

protected:
	lcStep mStep;
};

enum class lcModelActionHidePiecesMode
{
	HideSelected,
	HideUnselected,
	UnhideSelected,
	UnhideAll
};

class lcModelActionHidePieces : public lcModelAction
{
public:
	lcModelActionHidePieces(lcModelActionHidePiecesMode Mode);
	virtual ~lcModelActionHidePieces() = default;

	lcModelActionHidePiecesMode GetMode() const
	{
		return mMode;
	}

	const std::vector<bool>& GetHiddenState() const
	{
		return mHiddenState;
	}

	void SaveHiddenState(const std::vector<std::unique_ptr<lcPiece>>& Pieces);

protected:
	std::vector<bool> mHiddenState;
	lcModelActionHidePiecesMode mMode;
};

enum class lcModelActionStepMode
{
	Insert,
	Remove
};

struct lcModelActionStepPieceState
{
	lcStep StepShow;
	lcStep StepHide;
	QByteArray KeyFrames;
};

struct lcModelActionStepCameraState
{
	QByteArray KeyFrames;
};

struct lcModelActionStepLightState
{
	QByteArray KeyFrames;
};

class lcModelActionStep : public lcModelAction
{
public:
	lcModelActionStep(lcModelActionStepMode Mode, lcStep Step);
	virtual ~lcModelActionStep() = default;

	lcModelActionStepMode GetMode() const
	{
		return mMode;
	}

	lcStep GetStep() const
	{
		return mStep;
	}

    void SaveState(const std::vector<std::unique_ptr<lcPiece>>& Pieces, const std::vector<std::unique_ptr<lcCamera>>& Cameras, const std::vector<std::unique_ptr<lcLight>>& Lights);

	const std::vector<lcModelActionStepPieceState>& GetPieceStates() const
	{
		return mPieceStates;
	}
	
	const std::vector<lcModelActionStepCameraState>& GetCameraStates() const
	{
		return mCameraStates;
	}
	
	const std::vector<lcModelActionStepLightState>& GetLightStates() const
	{
		return mLightStates;
	}
	
protected:
	std::vector<lcModelActionStepPieceState> mPieceStates;
	std::vector<lcModelActionStepCameraState> mCameraStates;
	std::vector<lcModelActionStepLightState> mLightStates;
	lcModelActionStepMode mMode;
	lcStep mStep;
};
