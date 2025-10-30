#pragma once

#include "lc_math.h"

struct lcInsertPieceInfo;
enum class lcObjectType;

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

enum class lcModelActionAddPieceSelectionMode
{
	FocusLast,
	SelectAll
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
};
